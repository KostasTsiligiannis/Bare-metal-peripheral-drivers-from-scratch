/**
 ******************************************************************************
 * @details
 *  Protocol Sequence:
 *   1. Master transmits 1-byte command 0x51 (CMD_GET_LEN) followed by STOP.
 *   2. Master reads 1-byte payload length from Slave (0x68).
 *   3. Master validates length bounds (1 .. sizeof(rcv_buf) - 1).
 *   4. Master transmits 1-byte command 0x52 (CMD_GET_PAYLOAD) followed by STOP.
 *   5. Master reads <len> payload bytes, null-terminates the buffer, and prints
 *      the received string via ARM semihosting.
 *
 * @note Engineering & Interoperability Observations (STM32F3 <-> ESP32):
 *  - STOP vs. Repeated Start (Sr): Unlike AVR/ATmega hardware TWI slaves that
 *    automatically stretch SCL between a Write and Repeated Start Read, the
 *    ESP32 Arduino Wire Slave library dispatches its `onReceive` callback only
 *    after detecting a bus STOP condition. Therefore, `I2C_DISABLE_SR` is used
 *    after each command byte, paired with a 5 ms SysTick delay so the ESP32
 *    can process the command and stage its TX buffer before the next Read.
 *  - Pull-up & Timing Considerations: Verified using internal weak pull-ups
 *    (~40 kOhm) on both MCUs over short leads by adjusting `TIMINGR` to
 *    accommodate slower RC rise times. External 4.7 kOhm pull-ups to 3.3 V
 *    are recommended for longer wires or multi-device buses.
 *  - ISR Safety: All `printf()` semihosting calls are strictly deferred to
 *    `main()` via volatile completion/error flags (`txComplt`, `rxComplt`,
 *    `i2cError`) so semihosting breakpoints never stall I2C interrupt latency.
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "stm32f303k8.h"

/* ============================================================================
 * Application Macros & Protocol Commands
 * ============================================================================ */
#define MY_ADDR             0x61        /* Own 7-bit I2C device address (OAR1) */
#define SLAVE_ADDR          0x68        /* Target 7-bit I2C slave address (ESP32) */

#define CMD_GET_LEN         0x51        /* Command byte requesting payload length */
#define CMD_GET_PAYLOAD     0x52        /* Command byte requesting payload data */

#define HSI_CLOCK_HZ        8000000     /* Default internal HSI oscillator (8 MHz) */
#define MS_PER_SEC          1000        /* Milliseconds per second for SysTick */

/* ============================================================================
 * Cortex-M4 SysTick Register Definitions (1 ms Timebase @ 8 MHz HSI)
 * ============================================================================ */
#define SYSTICK_CSR         (*(volatile uint32_t *)0xE000E010) /* Control & Status */
#define SYSTICK_RVR         (*(volatile uint32_t *)0xE000E014) /* Reload Value */
#define SYSTICK_CVR         (*(volatile uint32_t *)0xE000E018) /* Current Value */

#define SYSTICK_CSR_CLKSOURCE_Msk   (1 << 2)   /* Processor clock (AHB) source */
#define SYSTICK_CSR_TICKINT_Msk     (1 << 1)   /* Enable SysTick exception request */
#define SYSTICK_CSR_ENABLE_Msk      (1 << 0)   /* Enable SysTick counter */

/* ============================================================================
 * Global Buffers, Synchronization Flags, Timing & Peripheral Handles
 * ============================================================================ */
extern void initialise_monitor_handles(void);

uint8_t rcv_buf[32];                            /* Master RX buffer for slave payload */
volatile uint32_t g_ms_ticks = 0;               /* Monotonic 1 ms SysTick counter */
volatile uint8_t txComplt = RESET;              /* Set by callback when TX completes */
volatile uint8_t rxComplt = RESET;              /* Set by callback when RX completes */
volatile uint8_t i2cError = RESET;              /* Set by callback on NACK/bus error */
volatile uint8_t lastErrEvent = 0;              /* Captured I2C error event code */
I2C_Handle_t I2C1Handle;                        /* I2C1 peripheral driver handle */

/* ============================================================================
 * SysTick Timebase & Delay Helpers
 * ============================================================================ */

/**
 * @brief  SysTick exception handler incrementing the global 1 ms tick counter.
 */
void SysTick_Handler(void)
{
    g_ms_ticks++;
}

/**
 * @brief  Initializes the ARM Cortex-M4 SysTick timer for 1 ms periodic ticks.
 * @param  sysclk_hz System/AHB clock frequency in Hz.
 */
void SysTick_Init(uint32_t sysclk_hz)
{
    SYSTICK_RVR = (sysclk_hz / MS_PER_SEC) - 1;    /* 8000 - 1 = 7999 -> 1 ms @ 8 MHz */
    SYSTICK_CVR = 0;                               /* Clear current counter and COUNTFLAG */
    SYSTICK_CSR = SYSTICK_CSR_CLKSOURCE_Msk | SYSTICK_CSR_TICKINT_Msk | SYSTICK_CSR_ENABLE_Msk;
}

/**
 * @brief  Blocking millisecond delay using unsigned subtraction for wrap-around safety.
 * @param  ms Delay duration in milliseconds.
 */
static void delay_ms(uint32_t ms)
{
    uint32_t start = g_ms_ticks;

    while ((g_ms_ticks - start) < ms)
    {
        /* Busy-wait until the requested number of 1 ms SysTick ticks elapses */
    }
}

/* ============================================================================
 * I2C1 GPIO & Peripheral Initialization
 * ============================================================================ */

/**
 * @brief  Configures PB6 (I2C1_SCL) and PB7 (I2C1_SDA) in Alternate Function
 *         Open-Drain mode (AF4) with internal pull-up resistors enabled.
 */
void I2C1_GPIOInits(void)
{
    GPIO_Handle_t I2CPins;
    memset(&I2CPins, 0, sizeof(I2CPins));

    I2CPins.pGPIOx = DRV_GPIOB;
    I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
    I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
    I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;              /* AF4: I2C1 */
    I2CPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

    /* PB6 -> I2C1_SCL */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&I2CPins);

    /* PB7 -> I2C1_SDA */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&I2CPins);
}

/**
 * @brief  Initializes I2C1 peripheral parameters and enables the peripheral.
 */
void I2C1_Inits(void)
{
    memset(&I2C1Handle, 0, sizeof(I2C1Handle));

    I2C1Handle.pI2Cx = DRV_I2C1;
    I2C1Handle.I2C_Config.I2C_DeviceAddress = MY_ADDR;
    I2C1Handle.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;      /* Standard Mode */

    I2C_Init(&I2C1Handle);
}

/* ============================================================================
 * Main Application Entry Point
 * ============================================================================ */
int main(void)
{
    uint8_t command_code = 0;
    uint8_t len = 0;

    /* Enable ARM semihosting console output */
    initialise_monitor_handles();

    /* 1. Initialize 1 ms SysTick timebase, I2C1 GPIOs, peripheral, and NVIC IRQs */
    SysTick_Init(HSI_CLOCK_HZ);
    I2C1_GPIOInits();
    I2C1_Inits();
    I2C_IRQInterruptConfig(IRQ_NO_I2C1_EV, ENABLE);
    I2C_IRQInterruptConfig(IRQ_NO_I2C1_ER, ENABLE);

    /* Allow ESP32 slave interface to settle before initiating bus traffic */
    delay_ms(5);

    /* 2. Transmit 0x51 (CMD_GET_LEN) and terminate with STOP so ESP32 onReceive fires */
    command_code = CMD_GET_LEN;
    txComplt = RESET;
    while (I2C_MasterSendDataIT(&I2C1Handle, &command_code, 1, SLAVE_ADDR, I2C_DISABLE_SR) != I2C_READY);
    while ((txComplt != SET) && (i2cError != SET));
    if (i2cError == SET)
    {
        printf("Error : I2C event %u during CMD_GET_LEN TX\n", (unsigned int)lastErrEvent);
        while (1);
    }
    delay_ms(5);
    printf("Tx (0x51) is completed\n");

    /* 3. Receive 1-byte payload length from slave */
    rxComplt = RESET;
    while (I2C_MasterReceiveDataIT(&I2C1Handle, &len, 1, SLAVE_ADDR, I2C_DISABLE_SR) != I2C_READY);
    while ((rxComplt != SET) && (i2cError != SET));
    if (i2cError == SET)
    {
        printf("Error : I2C event %u during LEN RX\n", (unsigned int)lastErrEvent);
        while (1);
    }
    delay_ms(5);
    printf("Rx (len=%u) is completed\n", (unsigned int)len);

    /* 4. Validate received length to prevent zero-length reads or buffer overflow */
    if ((len == 0) || (len >= sizeof(rcv_buf)))
    {
        printf("Error : Invalid payload length %u\n", (unsigned int)len);
        while (1);
    }

    /* 5. Transmit 0x52 (CMD_GET_PAYLOAD) and terminate with STOP */
    command_code = CMD_GET_PAYLOAD;
    txComplt = RESET;
    while (I2C_MasterSendDataIT(&I2C1Handle, &command_code, 1, SLAVE_ADDR, I2C_DISABLE_SR) != I2C_READY);
    while ((txComplt != SET) && (i2cError != SET));
    if (i2cError == SET)
    {
        printf("Error : I2C event %u during CMD_GET_PAYLOAD TX\n", (unsigned int)lastErrEvent);
        while (1);
    }
    delay_ms(5);
    printf("Tx (0x52) is completed\n");

    /* 6. Receive full payload string and close transaction with STOP */
    rxComplt = RESET;
    while (I2C_MasterReceiveDataIT(&I2C1Handle, rcv_buf, len, SLAVE_ADDR, I2C_DISABLE_SR) != I2C_READY);
    while ((rxComplt != SET) && (i2cError != SET));
    if (i2cError == SET)
    {
        printf("Error : I2C event %u during PAYLOAD RX\n", (unsigned int)lastErrEvent);
        while (1);
    }
    printf("Rx (payload) is completed\n");

    /* Null-terminate received ASCII payload and print result */
    rcv_buf[len] = '\0';
    printf("Data: %s\n", rcv_buf);

    while (1);
}

/* ============================================================================
 * IRQ Handlers & Application Event Callback (Semihosting-free ISR context)
 * ============================================================================ */

/**
 * @brief  I2C1 Event interrupt handler (shared vector with EXTI23 on STM32F303).
 */
void I2C1_EV_EXTI23_IRQHandler(void)
{
    I2C_EV_IRQHandling(&I2C1Handle);
}

/**
 * @brief  I2C1 Error interrupt handler (BERR, ARLO, OVR, TIMEOUT).
 */
void I2C1_ER_IRQHandler(void)
{
    I2C_ER_IRQHandling(&I2C1Handle);
}

/**
 * @brief  Application callback invoked by the I2C driver upon transfer completion
 *         or bus/NACK error detection.
 * @param  pI2CHandle Pointer to the active I2C handle.
 * @param  AppEv      Application event/error identifier (I2C_EV_* / I2C_ERROR_*).
 */
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
    if (AppEv == I2C_EV_TX_CMPLT)
    {
        txComplt = SET;
    }
    else if (AppEv == I2C_EV_RX_CMPLT)
    {
        rxComplt = SET;
    }
    else
    {
        lastErrEvent = AppEv;
        i2cError = SET;
        pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_STOP;
    }
}
