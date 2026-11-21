/**
 ******************************************************************************
 * @brief   Interrupt-driven STM32F303K8 (I2C1 v2) Slave command-response
 *          application serving payload length (0x51) and ASCII payload (0x52)
 *          to an external I2C Master (e.g., ESP32) at 100 kHz Standard Mode.
 *
 * @details
 *  - Hardware Mapping: PB6 (I2C1_SCL, AF4) and PB7 (I2C1_SDA, AF4) configured
 *    as Open-Drain with internal pull-ups enabled.
 *  - Protocol State Machine:
 *      1. Master writes 0x51 (CMD_GET_LEN) -> Slave latches command, resets
 *         transfer pointers, flushes TXDR, and shifts out 1-byte payload length
 *         upon TXIS (I2C_EV_DATA_REQ).
 *      2. Master writes 0x52 (CMD_GET_PAYLOAD) -> Slave latches command,
 *         resets w_ptr, flushes TXDR, and streams tx_buf[w_ptr++] byte-by-byte
 *         until master issues NACK (I2C_ERROR_AF) or STOP (I2C_EV_STOP).
 *  - STM32F3 I2C v2 Specifics: Uses ISR/ICR/TXDR/RXDR registers and flushes
 *    stale transmit data via write-1 to ISR.TXE (rc_w1) on command/bus
 *    boundaries so previous transfer bytes (such as 0x1B length) cannot leak
 *    into subsequent read transactions.
 ******************************************************************************
 */

#include <stdint.h>
#include <string.h>
#include "stm32f303k8.h"

/* ============================================================================
 * Protocol & Peripheral Configuration Macros
 * ============================================================================ */
#define MY_ADDR             0x68        /* 7-bit I2C1 slave address programmed into OAR1[7:1] */
#define CMD_GET_LEN         0x51        /* Command byte: master requests 1-byte payload length */
#define CMD_GET_PAYLOAD     0x52        /* Command byte: master requests payload byte stream */

/* ============================================================================
 * Global Buffers, Protocol State & Driver Handle
 * ============================================================================ */
static uint8_t tx_buf[32] = "STM32 slave mode testing..."; /* Null-terminated slave response payload */
static uint8_t data_len = 0;                                /* Cached payload byte length (excluding '\0') */
static volatile uint8_t command_code = 0xFF;                /* Active command latched from master write */
I2C_Handle_t I2C1Handle;                                    /* I2C1 peripheral driver handle instance */

/* ============================================================================
 * Static Helper Prototypes
 * ============================================================================ */
static void I2C1_GPIOInits(void);
static void I2C1_Inits(void);

/**
 * @brief  Configures PB6 (SCL) and PB7 (SDA) for I2C1 Alternate Function 4.
 * @note   Sets Open-Drain output type (mandatory for open-collector/wired-AND
 *         I2C bus lines) with fast slew rate and internal pull-up resistors.
 * @return None
 */
static void I2C1_GPIOInits(void)
{
    GPIO_Handle_t I2CPins;
    memset(&I2CPins, 0, sizeof(I2CPins));

    I2CPins.pGPIOx = DRV_GPIOB;
    I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
    I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
    I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4U;      /* AF4 maps PB6/PB7 to I2C1_SCL/I2C1_SDA */
    I2CPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

    /* Configure PB6 as I2C1_SCL */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&I2CPins);

    /* Configure PB7 as I2C1_SDA */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&I2CPins);
}

/**
 * @brief  Initializes I2C1 peripheral in 7-bit slave mode at 100 kHz Standard Mode.
 * @note   Programs OAR1 own address and TIMINGR timing parameters via I2C_Init().
 * @return None
 */
static void I2C1_Inits(void)
{
    memset(&I2C1Handle, 0, sizeof(I2C1Handle));

    I2C1Handle.pI2Cx = DRV_I2C1;
    I2C1Handle.I2C_Config.I2C_DeviceAddress = MY_ADDR;
    I2C1Handle.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;

    I2C_Init(&I2C1Handle);
}

/**
 * @brief  Application entry point initializing I2C1 slave events and error IRQs.
 * @note   Operates purely interrupt-driven after peripheral/NVIC setup; main loop
 *         remains idle while ISR state transitions service bus events.
 * @return int (non-returning bare-metal loop)
 */
int main(void)
{
    /* Compute payload length once prior to enabling bus interrupts */
    data_len = (uint8_t)strlen((char *)tx_buf);

    /* Initialize bus pins and I2C1 v2 peripheral registers */
    I2C1_GPIOInits();
    I2C1_Inits();

    /* Enable NVIC lines and I2C1 CR1 event/error interrupt sources for slave mode */
    I2C_IRQInterruptConfig(IRQ_NO_I2C1_EV, ENABLE);
    I2C_IRQInterruptConfig(IRQ_NO_I2C1_ER, ENABLE);
    I2C_SlaveEnableDisableCallbackEvents(DRV_I2C1, ENABLE);

    while (1)
    {
        /* All protocol handling executes asynchronously inside I2C EV/ER ISRs */
    }
}

/**
 * @brief  I2C1 Event IRQ handler (shared Cortex-M4 vector with EXTI line 23).
 * @return None
 */
void I2C1_EV_EXTI23_IRQHandler(void)
{
    I2C_EV_IRQHandling(&I2C1Handle);
}

/**
 * @brief  I2C1 Error IRQ handler servicing BERR, ARLO, and OVR fault flags.
 * @return None
 */
void I2C1_ER_IRQHandler(void)
{
    I2C_ER_IRQHandling(&I2C1Handle);
}

/**
 * @brief  Application-level I2C event/error callback implementing the slave
 *         command-response protocol state machine.
 * @param  pI2CHandle Pointer to the I2C peripheral handle instance.
 * @param  AppEv      Driver-reported event or error code (I2C_EV_* / I2C_ERROR_*).
 * @note   Keeps ISR execution deterministic and minimal; resets byte indices on
 *         command reception, STOP, and NACK (AF) boundaries and flushes TXDR
 *         via ISR.TXE (write-1) to prevent stale byte leakage.
 * @return None
 */
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
    (void)pI2CHandle;
    static uint32_t cnt = 0;
    static uint32_t w_ptr = 0;

    if (AppEv == I2C_EV_DATA_RCV)
    {
        /* Latch incoming command byte from RXDR and reset stream indices */
        command_code = I2C_SlaveReceiveData(DRV_I2C1);
        cnt = 0;
        w_ptr = 0;

        /* Flush TXDR via write-1 to TXE (rc_w1) so prior TX bytes (e.g. 0x1B) cannot shift out */
        DRV_I2C1->ISR = DRV_I2C_ISR_TXE;
    }
    else if (AppEv == I2C_EV_DATA_REQ)
    {
        /* Master initiated a read phase (TXIS): serve length or sequential payload byte */
        if (command_code == CMD_GET_LEN)
        {
            I2C_SlaveSendData(DRV_I2C1, data_len);
            cnt++;
        }
        else if (command_code == CMD_GET_PAYLOAD)
        {
            if (w_ptr < data_len)
            {
                I2C_SlaveSendData(DRV_I2C1, tx_buf[w_ptr++]);
            }
            else
            {
                /* Pad with 0x00 if master clocks more bytes than available payload length */
                I2C_SlaveSendData(DRV_I2C1, 0x00);
            }
        }
    }
    else if (AppEv == I2C_EV_STOP)
    {
        /* End of current I2C transaction: reset indices and flush stale TXDR data */
        cnt = 0;
        w_ptr = 0;
        DRV_I2C1->ISR = DRV_I2C_ISR_TXE;
    }
    else if (AppEv == I2C_ERROR_AF)
    {
        /* Master NACKed data byte (end of read): reset indices and clear TXDR */
        cnt = 0;
        w_ptr = 0;
        DRV_I2C1->ISR = DRV_I2C_ISR_TXE;

        if (command_code != CMD_GET_PAYLOAD)
        {
            command_code = 0xFF;
        }
    }
}
