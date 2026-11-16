/**
 ******************************************************************************
 * @file    010master_tx_testing.c
 * @brief   I2C1 Master Transmit (Tx) test application for STM32F303K8.
 *          Sends a test string to an I2C Slave device (ESP32 @ 0x68)
 *          using Standard Mode (100 kHz) over PB6 (SCL) and PB7 (SDA).
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "stm32f303k8.h"

/* ============================================================================
 * Application Macros & Global Variables
 * ============================================================================ */
#define MY_ADDR         0x61    /* Own 7-bit I2C device address (OAR1) */
#define SLAVE_ADDR      0x68    /* Target 7-bit I2C slave address (ESP32) */

/* Payload buffer to be transmitted over the I2C bus */
uint8_t some_data[] = "We are testing I2C master Tx\n";

/* Global handle structure for the I2C1 peripheral */
I2C_Handle_t I2C1Handle;

/* ============================================================================
 * 1. I2C1 GPIO Initialization
 * ============================================================================
 * Pin Mapping (STM32F303K8 Datasheet - Alternate Function Mapping):
 *  - PB6 : I2C1_SCL (AF4)
 *  - PB7 : I2C1_SDA (AF4)
 *
 * Electrical Requirements (RM0316 - I2C functional description):
 *  - Open-Drain output type is mandatory for wired-AND I2C bus arbitration
 *    and clock stretching support.
 *  - Pull-up resistors are enabled (external 4.7k pull-ups recommended on bus).
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


/* ============================================================================
 * 2. I2C1 Peripheral Initialization
 * ============================================================================
 * Configures I2C1 in Standard Mode (100 kHz) derived from the default 8 MHz
 * HSI clock source (RCC_CFGR3.I2C1SW = 0) and enables the peripheral (PE = 1).
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
 * 3. Main Application Entry Point
 * ============================================================================ */
int main(void)
{
    /* 1. Initialize GPIOB pins (PB6/PB7) for I2C1 Alternate Function Open-Drain */
    I2C1_GPIOInits();

    /* 2. Initialize I2C1 peripheral (Clock, Filters, TIMINGR, OAR1, PE = 1) */
    I2C1_Inits();

    /* 3. Execute blocking Master Transmit sequence (START -> SADD+W -> Data -> STOP) */
    I2C_MasterSendData(&I2C1Handle, (uint8_t*)some_data, strlen((char*)some_data), SLAVE_ADDR);

    /* 4. Idle loop after single packet transmission */
    while (1)
    {
    }

    return 0;
}
