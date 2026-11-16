/**
 ******************************************************************************
 * @file    011_master_rx_testing.c
 * @brief   STM32F303K8 I2C1 Master command-response test application
 *          communicating with an ESP32 I2C Slave (0x68) at 100 kHz Standard
 *          Mode over PB6 (SCL) and PB7 (SDA) using a SysTick 1 ms timebase.
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

#define HSI_CLOCK_HZ        8000000    /* Default internal HSI clock frequency */
#define MS_PER_SEC          1000       /* Milliseconds per second for tick calc */

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
 * Global Buffers, Timing State & Peripheral Handles
 * ============================================================================ */
uint8_t rcv_buf[32];                            /* Master RX buffer for slave payload */
uint8_t some_data[] = "We are testing I2C master Tx\n";
volatile uint32_t g_ms_ticks = 0;               /* Monotonic 1 ms tick counter */

I2C_Handle_t I2C1Handle;                        /* I2C1 driver handle instance */

/* ============================================================================
 * SysTick Timebase & Delay Helpers
 * ============================================================================ */

/**
 * @brief  SysTick interrupt service routine incrementing the 1 ms tick counter.
 */
void SysTick_Handler(void)
{
    g_ms_ticks++;
}

/**
 * @brief  Configures the ARM Cortex-M4 SysTick timer for periodic 1 ms ticks.
 * @param  sysclk_hz System/AHB clock frequency in Hz.
 */
void SysTick_Init(uint32_t sysclk_hz)
{
    SYSTICK_RVR = (sysclk_hz / MS_PER_SEC) - 1;    /* 8000 - 1 = 7999 -> 1 ms @ 8 MHz */
    SYSTICK_CVR = 0;                               /* Clear current counter and flag */
    SYSTICK_CSR = SYSTICK_CSR_CLKSOURCE_Msk | SYSTICK_CSR_TICKINT_Msk | SYSTICK_CSR_ENABLE_Msk;
}

/**
 * @brief  Blocking millisecond delay using unsigned tick subtraction for wrap safety.
 * @param  ms Delay duration in milliseconds.
 */
static void delay_ms(uint32_t ms)
{
    uint32_t start = g_ms_ticks;

    while ((g_ms_ticks - start) < ms)
    {
        /* Wait until the requested number of 1 ms SysTick interrupts elapses */
    }
}

/* ============================================================================
 * I2C1 GPIO & Peripheral Initialization
 * ============================================================================ */

/**
 * @brief  Initializes PB6 (SCL) and PB7 (SDA) in Alternate Function Open-Drain
 *         mode (AF4) with internal pull-ups enabled for I2C1.
 *
 * Electrical Notes (RM0316):
 *  - Open-drain configuration is required for wired-AND bus arbitration and
 *    clock stretching.
 *  - External 4.7 kOhm pull-ups are recommended for heavier bus loads.
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

    /* Configure PB6 as I2C1_SCL */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&I2CPins);

    /* Configure PB7 as I2C1_SDA */
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&I2CPins);
}

/**
 * @note Hardware & Signal Integrity Observations (PulseView @ 100 kHz):
 *  - Pull-ups: Verified stable communication using only internal weak pull-ups (~40 kOhm)
 *    on STM32 (PB6/PB7) and ESP32 (GPIO 21/22) over short leads (no external resistors).
 *  - Handoff Glitch & Mitigation: A brief positive SDA spike (~300 ns) before the 9th clock
 *    (ACK) occurs when the 8th bit is '0' during Master-to-Slave open-drain bus release
 *    (valid per I2C spec, as SDA is sampled on SCL HIGH). Increasing TIMINGR.SDADEL
 *    (from 0x2 to 0x6) extended SDA hold time and visibly erased these spikes.
 */

/**
 * @brief  Initializes I2C1 in Standard Mode (100 kHz) using the 8 MHz HSI clock
 *         and enables the peripheral (PE = 1).
 */
void I2C1_Inits(void)
{
    memset(&I2C1Handle, 0, sizeof(I2C1Handle));

    I2C1Handle.pI2Cx = DRV_I2C1;
    I2C1Handle.I2C_Config.I2C_DeviceAddress = MY_ADDR;
    I2C1Handle.I2C_Config.I2C_SCLSpeed = I2C_SCL_SPEED_SM;      /* Standard Mode: 100 kHz */

    I2C_Init(&I2C1Handle);
}

/* ============================================================================
 * Main Application Entry Point
 * ============================================================================ */
int main(void)
{
    uint8_t command_code = 0;
    uint8_t len = 0;

    /* 1. Initialize 1 ms SysTick timebase at 8 MHz HSI */
    SysTick_Init(HSI_CLOCK_HZ);

    /* 2. Initialize I2C1 pins (PB6/PB7) and peripheral timing/state */
    I2C1_GPIOInits();
    I2C1_Inits();

    /* 3. Query payload length from slave using command 0x51 */
    command_code = CMD_GET_LEN;
    I2C_MasterSendData(&I2C1Handle, &command_code, 1, SLAVE_ADDR);
    delay_ms(5);
    I2C_MasterReceiveData(&I2C1Handle, &len, 1, SLAVE_ADDR);

    delay_ms(5);

    /* 4. Validate received length against RX buffer capacity before payload read */
    if ((len == 0) || (len >= sizeof(rcv_buf)))
    {
        /* Trap invalid or out-of-bounds length for debugger inspection */
        while (1)
        {
        }
    }

    /* 5. Request payload bytes using command 0x52 and null-terminate string */
    command_code = CMD_GET_PAYLOAD;
    I2C_MasterSendData(&I2C1Handle, &command_code, 1, SLAVE_ADDR);
    delay_ms(5);
    I2C_MasterReceiveData(&I2C1Handle, rcv_buf, len, SLAVE_ADDR);

    rcv_buf[len] = '\0';

    /* 6. Idle loop after completing single command-response transaction */
    while (1)
    {
    }
}
