/**
 * @file main.c
 * @brief Robust Interrupt-Driven SPI Master Implementation for STM32F303K8
 * @details Synchronizes via EXTI0 interrupt on PA0, then performs a continuous
 *          32-byte full-duplex block transfer to receive payloads from the ESP32 Slave.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "stm32f303k8.h"
#include "stm32f303k8_gpio_driver.h"
#include "stm32f303k8_spi_driver.h"

extern void initialise_monitor_handles(void);

// ============================================================================
// 1. CONFIGURATION CONSTANTS
// ============================================================================
#define MAX_LEN          32U
#define SPI_START_MARKER 0xA5U

// ============================================================================
// 2. GLOBAL VARIABLES
// ============================================================================
SPI_Handle_t SPI1Handle;

char RcvBuff[MAX_LEN];
uint8_t SPIFrame[MAX_LEN];

volatile uint8_t dataAvailable = 0; // Flag set by EXTI0 interrupt handler

// ============================================================================
// 3. SPI1 GPIO INITIALIZATION
// ============================================================================
void SPI1_GPIOInits(void)
{
    GPIO_Handle_t SPIPins;
    GPIO_PeriClockControl(DRV_GPIOA, ENABLE);

    SPIPins.pGPIOx = DRV_GPIOA;
    SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;
    SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

    // PA5 -> SPI1_SCK
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
    GPIO_Init(&SPIPins);

    // PA7 -> SPI1_MOSI
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&SPIPins);

    // PA6 -> SPI1_MISO
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&SPIPins);

    // PA4 -> SPI1_NSS (Hardware Managed)
    SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_4;
    GPIO_Init(&SPIPins);
}

// ============================================================================
// 4. SPI1 PERIPHERAL INITIALIZATION
// ============================================================================
void SPI1_Inits(void)
{
    SPI1Handle.pSPIx = DRV_SPI1;
    SPI1Handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
    SPI1Handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;

    // KEY FIX: Using DIV64 instead of DIV32 to enhance noise immunity over jumper wires
    SPI1Handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV64;

    SPI1Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
    SPI1Handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
    SPI1Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;

    // Hardware Slave Management (SSM = Disabled)
    SPI1Handle.SPIConfig.SPI_SSM = SPI_SSM_DI;

    SPI_Init(&SPI1Handle);
}

// ============================================================================
// 5. SLAVE INTERRUPT PIN INITIALIZATION (PA0 / EXTI0)
// ============================================================================
void Slave_GPIO_InterruptPinInit(void)
{
    GPIO_Handle_t spiIntPin;
    memset(&spiIntPin, 0, sizeof(spiIntPin));

    /* 1. Ενεργοποίηση ρολογιού SYSCFG μέσω του header σου (αντί για raw pointer) */
    DRV_SYSCFG_PCLK_EN();

    // Ενεργοποίηση ρολογιού GPIOA
    GPIO_PeriClockControl(DRV_GPIOA, ENABLE);

    /* 2. Ρύθμιση του PA0 ως Interrupt εισόδου (Falling Edge) */
    spiIntPin.pGPIOx = DRV_GPIOA;
    spiIntPin.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
    spiIntPin.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IT_FT;
    spiIntPin.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    spiIntPin.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;

    GPIO_Init(&spiIntPin);

    /* 3. Σύνδεση του EXTI0 με το PORT A χρησιμοποιώντας τη δομή SYSCFG του header σου */
    // Το EXTICR[0] αντιστοιχεί στα EXTI lines [3:0] (δηλαδή τα EXTI0, 1, 2, 3)
    uint8_t temp1 = DRV_GPIO_BASEADDR_TO_CODE(DRV_GPIOA); // Παίρνει το 0 για το Port A

    DRV_SYSCFG->EXTICR[0] &= ~(0xF << 0); // Καθαρισμός των bits 3:0 για το EXTI0
    DRV_SYSCFG->EXTICR[0] |=  (temp1 << 0); // Αντιστοίχιση του Port A στο EXTI0

    /* 4. Ρύθμιση NVIC Interrupts */
    GPIO_IRQPriorityConfig(DRV_IRQ_NO_EXTI0, 15U);
    GPIO_IRQInterruptConfig(DRV_IRQ_NO_EXTI0, ENABLE);
}

// ============================================================================
// 6. MAIN APPLICATION ENTRY
// ============================================================================
int main(void)
{
    initialise_monitor_handles();

    Slave_GPIO_InterruptPinInit();
    SPI1_GPIOInits();
    SPI1_Inits();

    // Enable hardware-automated NSS control (SSOE = 1)
    SPI_SSOEConfig(DRV_SPI1, ENABLE);

    printf("====================================\r\n");
    printf(" STM32F303K8 SPI MASTER (PROF LOGIC)\r\n");
    printf("====================================\r\n");
    printf("Waiting for ESP32 Data Available Interrupt...\r\n");

    while (1)
    {
        memset(RcvBuff, 0, sizeof(RcvBuff));
        memset(SPIFrame, 0, sizeof(SPIFrame));

        // 1. Block execution until ESP32 pulls PA0 LOW (Data Ready Interrupt)
        while (!dataAvailable);

        // Safety delay to allow the ESP32 RTOS task to enter slave.wait() safely
        for(volatile uint32_t i = 0; i < 15000; i++) { __asm("NOP"); }

        // Temporarily disable EXTI0 to prevent re-triggering artifacts during transaction
        GPIO_IRQInterruptConfig(DRV_IRQ_NO_EXTI0, DISABLE);

        // 2. Enable SPI peripheral (NSS automatically drops LOW)
        SPI_PeripheralControl(DRV_SPI1, ENABLE);

        // 3. Continuous 32-byte full-duplex block transfer without gaps
        for (uint32_t i = 0; i < MAX_LEN; i++)
        {
            SPIFrame[i] = SPI_TransferByte(DRV_SPI1, 0xFF);
        }

        // 4. Ensure transmission shift registers are completely flushed
        while (SPI_GetFlagStatus(DRV_SPI1, SPI_BUSY_FLAG) == FLAG_SET);

        // 5. Disable SPI peripheral (NSS automatically returns HIGH)
        SPI_PeripheralControl(DRV_SPI1, DISABLE);

        // Extract string payload, skipping the first marker byte
        memcpy(RcvBuff, &SPIFrame[1], MAX_LEN - 1);
        RcvBuff[MAX_LEN - 1] = '\0';

        // 6. Validate frame integrity via Start Marker check
        if (SPIFrame[0] == SPI_START_MARKER)
        {
            printf("\r\nMessage received from ESP32: %s\r\n", RcvBuff);
        }
        else
        {
            printf("Invalid Frame Marker: 0x%02X\r\n", SPIFrame[0]);
        }
        printf("------------------------------------\r\n");

        // Clear flag and re-enable interrupt for the next cycle
        dataAvailable = 0;
        GPIO_IRQInterruptConfig(DRV_IRQ_NO_EXTI0, ENABLE);
    }

    return 0;
}

// ============================================================================
// 7. INTERRUPT HANDLERS & CALLBACKS
// ============================================================================

/**
 * @brief External Interrupt Handler for EXTI Line 0 (PA0 input from ESP32)
 */
void EXTI0_IRQHandler(void)
{
    GPIO_IRQHandling(GPIO_PIN_NO_0);
    dataAvailable = 1;
}
