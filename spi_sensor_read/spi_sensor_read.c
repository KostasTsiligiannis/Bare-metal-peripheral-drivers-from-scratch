
#include "stm32f303k8.h"
#include <stdint.h>

/* ============================================================================
 * 1. SPI COMMANDS AND CONFIGURATION
 * ========================================================================== */

#define COMMAND_LED_CTRL       0x50
#define COMMAND_SENSOR_READ    0x51

#define LED_PIN                2
#define LED_ON                 1
#define LED_OFF                0

#define ANALOG_PIN34           34

#define ACK                    0xF5

#define SPI_FRAME_SIZE         4


/* ============================================================================
 * 2. DELAY FUNCTION
 * ========================================================================== */

void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < (ms * 1000U); i++)
    {
        __asm("NOP");
    }
}


/* ============================================================================
 * 3. SPI RESPONSE VERIFICATION
 * ========================================================================== */

uint8_t SPI_VerifyResponse(uint8_t ack_byte)
{
    return (ack_byte == ACK) ? 1U : 0U;
}


/* ============================================================================
 * 4. SPI GPIO INITIALIZATION
 *
 * STM32F303K8 SPI1:
 *
 * PA4 -> NSS
 * PA5 -> SCK
 * PA6 -> MISO
 * PA7 -> MOSI
 *
 * Alternate Function 5
 * ========================================================================== */

void SPI_GPIO_Inits(void)
{
    GPIO_Handle_t SPI_Pins = {0};

    /* Enable GPIOA peripheral clock. */
    GPIO_PeriClockControl(DRV_GPIOA, ENABLE);

    SPI_Pins.pGPIOx = DRV_GPIOA;

    SPI_Pins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    SPI_Pins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;
    SPI_Pins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    SPI_Pins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    SPI_Pins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    /* SCK - PA5 */
    SPI_Pins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
    GPIO_Init(&SPI_Pins);

    /* NSS - PA4 */
    SPI_Pins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_4;
    GPIO_Init(&SPI_Pins);

    /* MISO - PA6 */
    SPI_Pins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&SPI_Pins);

    /* MOSI - PA7 */
    SPI_Pins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&SPI_Pins);
}


/* ============================================================================
 * 5. SPI1 PERIPHERAL INITIALIZATION
 * ========================================================================== */

void SPI1_Inits(void)
{
    SPI_Handle_t SPIHandle = {0};

    SPIHandle.pSPIx = DRV_SPI1;

    /* Full-duplex communication. */
    SPIHandle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;

    /* STM32 operates as SPI Master. */
    SPIHandle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;

    /* SPI clock = PCLK / 32. */
    SPIHandle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV32;

    /* 8-bit data frame. */
    SPIHandle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;

    /*
     * SPI MODE 1:
     *
     * CPOL = 0 -> Clock idle LOW
     * CPHA = 1 -> Data captured on second clock edge
     */
    SPIHandle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
    SPIHandle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;

    /* Hardware NSS management. */
    SPIHandle.SPIConfig.SPI_SSM = SPI_SSM_DI;

    SPI_Init(&SPIHandle);
}


/* ============================================================================
 * 6. SPI FRAME TRANSFER
 *
 * Transfers exactly four bytes while NSS remains active.
 *
 * The SPI peripheral is disabled after the frame has completed.
 * With hardware NSS output enabled, this releases NSS (HIGH).
 *
 * The caller must re-enable SPI before starting the next frame.
 * ========================================================================== */

void SPI_TransferFrame(uint8_t *tx_buffer, uint8_t *rx_buffer)
{
    for (uint8_t i = 0; i < SPI_FRAME_SIZE; i++)
    {
        rx_buffer[i] = SPI_TransferByte(DRV_SPI1, tx_buffer[i]);
    }

    /* Wait until transmit buffer is empty. */
    while (SPI_GetFlagStatus(DRV_SPI1, SPI_TXE_FLAG) == FLAG_RESET);

    /* Wait until the SPI transfer is completely finished. */
    while (SPI_GetFlagStatus(DRV_SPI1, SPI_BUSY_FLAG) == FLAG_SET);

    /* End current SPI frame and release hardware NSS. */
    SPI_PeripheralControl(DRV_SPI1, DISABLE);
}


/* ============================================================================
 * 7. MAIN APPLICATION
 * ========================================================================== */

int main(void)
{
    uint8_t tx_frame[SPI_FRAME_SIZE];
    uint8_t rx_frame[SPI_FRAME_SIZE];

    uint8_t analog_read = 0;

    /* ------------------------------------------------------------------------
     * 7.1 Initialize GPIO and SPI
     * ---------------------------------------------------------------------- */

    SPI_GPIO_Inits();

    SPI1_Inits();

    /* Enable hardware NSS output. */
    SPI_SSOEConfig(DRV_SPI1, ENABLE);

    /* Allow ESP32 Slave to initialize. */
    delay_ms(1500);


    /* ========================================================================
     * 8. COMMAND: LED CONTROL
     * ====================================================================== */

    /*
     * STM32 -> ESP32:
     *
     * Byte 0: 0x50 -> COMMAND_LED_CTRL
     * Byte 1: 0xFF -> Dummy byte
     * Byte 2: 0x02 -> LED GPIO
     * Byte 3: 0x01 -> LED ON
     *
     * ESP32 -> STM32:
     *
     * Byte 0: 0x00 -> Dummy
     * Byte 1: 0xF5 -> ACK
     * Byte 2: 0x00 -> Dummy
     * Byte 3: 0x00 -> ADC response / previous value
     */

    tx_frame[0] = COMMAND_LED_CTRL;
    tx_frame[1] = 0xFF;
    tx_frame[2] = LED_PIN;
    tx_frame[3] = LED_ON;

    /* Start SPI communication. */
    SPI_PeripheralControl(DRV_SPI1, ENABLE);

    /* Transfer one complete four-byte frame. */
    SPI_TransferFrame(tx_frame, rx_frame);

    /* Verify protocol acknowledgment. */
    if (SPI_VerifyResponse(rx_frame[1]))
    {
        /* LED command acknowledged. */
    }
    else
    {
        /* Invalid ACK received. */
    }


    /* ========================================================================
     * 9. COMMAND: ADC SENSOR READ - FIRST FRAME
     * ====================================================================== */

    /*
     * STM32 -> ESP32:
     *
     * Byte 0: 0x51 -> COMMAND_SENSOR_READ
     * Byte 1: 0xFF -> Dummy / ACK clock
     * Byte 2: 0x22 -> GPIO34
     * Byte 3: 0xFF -> Dummy
     *
     * After this transaction completes, ESP32 executes analogRead().
     */

    delay_ms(10);

    tx_frame[0] = COMMAND_SENSOR_READ;
    tx_frame[1] = 0xFF;
    tx_frame[2] = ANALOG_PIN34;
    tx_frame[3] = 0xFF;

    /* Start a new SPI transaction. */
    SPI_PeripheralControl(DRV_SPI1, ENABLE);

    SPI_TransferFrame(tx_frame, rx_frame);

    /* Check ACK received in byte 1. */
    if (SPI_VerifyResponse(rx_frame[1]))
    {
        /*
         * ACK received.
         *
         * ESP32 has processed the ADC request after the
         * completion of the first frame.
         */
    }
    else
    {
        /* Invalid ACK: do not request ADC data. */
        analog_read = 0;
    }


    /* ========================================================================
     * 10. ADC SENSOR READ - SECOND FRAME
     * ====================================================================== */

    if (SPI_VerifyResponse(rx_frame[1]))
    {
        /*
         * The ESP32 has calculated sensor_reply_data.
         *
         * Request the latest ADC measurement using another
         * complete four-byte SPI transaction.
         */

        delay_ms(10);

        tx_frame[0] = 0x00;
        tx_frame[1] = 0xFF;
        tx_frame[2] = 0xFF;
        tx_frame[3] = 0xFF;

        /* Start the second SPI transaction. */
        SPI_PeripheralControl(DRV_SPI1, ENABLE);

        SPI_TransferFrame(tx_frame, rx_frame);

        /*
         * ADC measurement is transmitted in byte 3.
         *
         * rx_frame[3] contains an 8-bit value (0-255).
         *
         * In my example i connected analog pin 34 in GND as shown in logic analyzer' s figures
         */
        analog_read = rx_frame[3];
    }

    while (1)
    {
        /*
         * analog_read contains the latest ADC measurement.
         *
         * Range: 0-255
         *
         * You can use this value for further processing.
         */
    }
}
