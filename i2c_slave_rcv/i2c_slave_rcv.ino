/**
 ******************************************************************************
 * @file    esp32_i2c_slave_rx.ino
 * @author  Embedded Systems Firmware Development
 * @brief   ESP32 I2C Slave Receiver application validating Master Tx frames
 *          transmitted by an STM32F303K8 bare-metal I2C driver at 100 kHz.
 *
 * @note    Hardware Pinout & Requirements (ESP32 DevKit - 3.3V Logic):
 *          - GPIO 21 : I2C_SDA (Connected to STM32 PB7)
 *          - GPIO 22 : I2C_SCL (Connected to STM32 PB6)
 *          - External Pull-up resistors (4.7 kOhm to 3.3V) on both SDA & SCL
 *          - Common Ground (GND) between ESP32 Slave and STM32 Master
 ******************************************************************************
 */

#include <Wire.h>

/* ============================================================================
 * 1. HARDWARE CONFIGURATION & GLOBAL VARIABLES
 * ============================================================================ */

#define MY_ADDR         0x68    /* 7-bit I2C Slave address matching STM32 target */
#define I2C_SDA         21      /* Default ESP32 I2C Data pin */
#define I2C_SCL         22      /* Default ESP32 I2C Clock pin */

/* Shared reception buffer and ISR synchronization flag */
char rx_buffer[64];
volatile bool dataReceived = false;
uint32_t packetCount = 0;

/* ============================================================================
 * 2. I2C INTERRUPT CALLBACK (ISR CONTEXT)
 * ============================================================================ */

/**
 * @brief  Hardware I2C Receive Event Callback triggered upon Master Write.
 * @param  bytes  Number of bytes received in the current I2C transaction.
 * @note   Executes in Interrupt Service Routine (ISR) context. Keep execution
 *         minimal and avoid blocking calls (such as Serial.print) inside this
 *         handler to prevent I2C clock stretching timeouts or WDT resets.
 */
void receiveEvent(int bytes) {
    int cnt = 0;

    /* Read incoming bytes from the hardware I2C RX FIFO */
    while (Wire.available() && cnt < (sizeof(rx_buffer) - 1)) {
        char c = Wire.read();

        /* Strip trailing CR/LF characters sent by Master to keep Serial logs clean */
        if (c != '\r' && c != '\n') {
            rx_buffer[cnt++] = c;
        }
    }

    rx_buffer[cnt] = '\0';      /* Null-terminate as a valid C-string */
    dataReceived = true;        /* Signal main loop() for deferred printing */
}

/* ============================================================================
 * 3. SYSTEM INITIALIZATION & MAIN EXECUTION LOOP
 * ============================================================================ */

void setup() {
    /* Initialize UART debug console at 115200 baud */
    Serial.begin(115200);
    delay(1000);

    /* Configure I2C GPIO pins prior to initializing the Wire peripheral */
    Wire.setPins(I2C_SDA, I2C_SCL);

    /* Initialize ESP32 strictly in I2C Slave mode at 7-bit address 0x68 */
    Wire.begin((uint8_t)MY_ADDR);

    /* Register asynchronous receive callback handler */
    Wire.onReceive(receiveEvent);

    Serial.printf("ESP32 Slave ready at 0x%X. Waiting for packets...\n", MY_ADDR);
}

void loop() {
    /* Deferred processing outside ISR context when a full frame arrives */
    if (dataReceived) {
        dataReceived = false;
        packetCount++;

        Serial.printf("[Packet #%lu] Received: %s\n", packetCount, rx_buffer);
    }
}
