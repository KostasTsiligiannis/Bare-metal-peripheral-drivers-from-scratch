/**
 ******************************************************************************
 * @file    esp32_i2c_slave_cmd_tx_rx.ino
 * @brief   ESP32 I2C Slave implementation supporting command-driven length
 *          (0x51) and payload (0x52) responses for an STM32F303K8 Master
 *          operating at 100 kHz Standard Mode.
 *
 * Pinout & Bus Configuration:
 *  - GPIO 21 : I2C SDA (connected to STM32 PB7)
 *  - GPIO 22 : I2C SCL (connected to STM32 PB6)
 *  - Slave Address : 0x68 (7-bit addressing)
 ******************************************************************************
 */

#include <Wire.h>
#include <string.h>

/* ============================================================================
 * Application Macros & Protocol Commands
 * ============================================================================ */
#define SLAVE_ADDR          0x68    /* 7-bit I2C slave address */
#define I2C_SDA             21      /* SDA pin assignment */
#define I2C_SCL             22      /* SCL pin assignment */

#define CMD_GET_LEN         0x51    /* Request 1-byte payload length */
#define CMD_GET_PAYLOAD     0x52    /* Request full payload buffer */
#define CMD_NONE            0xFF    /* Idle / uninitialized command state */

/* ============================================================================
 * Shared ISR/Callback State & Payload Buffer
 * ============================================================================ */
volatile uint8_t active_command = CMD_NONE;
volatile uint8_t last_cmd_logged = CMD_NONE;
volatile uint8_t last_sent_cmd = CMD_NONE;
volatile bool cmd_received_flag = false;
volatile bool data_sent_flag = false;

char name_msg[32] = "Welcome to FastBit EBA\n";

/**
 * @brief  Returns the byte length of the null-terminated payload string.
 * @retval Payload length in bytes (excluding null terminator).
 */
uint8_t get_len_of_data(void) {
    return (uint8_t)strlen(name_msg);
}

/**
 * @brief  Preloads the ESP32 I2C slave TX FIFO for a given command code.
 * @param  cmd Command byte received from the I2C master.
 */
static void prepareResponseForCommand(uint8_t cmd) {
    uint8_t len = get_len_of_data();

    if (cmd == CMD_GET_LEN) {
        Wire.slaveWrite(&len, 1);
    } else if (cmd == CMD_GET_PAYLOAD) {
        Wire.slaveWrite((const uint8_t *)name_msg, len);
    }
}

/* ============================================================================
 * I2C Slave Callbacks
 * ============================================================================ */

/**
 * @brief  Handles incoming write transactions (command bytes) from the master.
 * @param  bytes Number of bytes available in the RX buffer.
 */
void receiveEvent(int bytes) {
    (void)bytes;

    while (Wire.available()) {
        uint8_t cmd = (uint8_t)Wire.read();
        active_command = cmd;
        last_cmd_logged = cmd;
        cmd_received_flag = true;

        /* Preload TX buffer early to minimize response latency on master read */
        prepareResponseForCommand(cmd);
    }
}

/**
 * @brief  Handles master read requests (onRequest) and arms the appropriate
 *         response buffer based on the last received command.
 */
void requestEvent(void) {
    uint8_t len = get_len_of_data();

    if (active_command == CMD_GET_LEN) {
        Wire.slaveWrite(&len, 1);
        last_sent_cmd = CMD_GET_LEN;
        data_sent_flag = true;
        active_command = CMD_NONE;
    } else if (active_command == CMD_GET_PAYLOAD) {
        Wire.slaveWrite((const uint8_t *)name_msg, len);
        last_sent_cmd = CMD_GET_PAYLOAD;
        data_sent_flag = true;
        active_command = CMD_NONE;
    }
}

/* ============================================================================
 * Arduino Entry Points
 * ============================================================================ */

/**
 * @brief  Initializes serial logging and configures ESP32 as an I2C slave.
 */
void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin((uint8_t)SLAVE_ADDR);

    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    Serial.printf("ESP32 I2C Slave ready at address 0x%02X (SDA=%d, SCL=%d)\n",
                  SLAVE_ADDR, I2C_SDA, I2C_SCL);
}

/**
 * @brief  Deferred serial logging loop for received commands and sent frames.
 */
void loop() {
    if (cmd_received_flag) {
        cmd_received_flag = false;
        Serial.printf("[RX] Command received: 0x%02X\n", last_cmd_logged);
    }

    if (data_sent_flag) {
        data_sent_flag = false;
        if (last_sent_cmd == CMD_GET_LEN) {
            Serial.printf("[TX] Sent length: %u bytes\n", get_len_of_data());
        } else if (last_sent_cmd == CMD_GET_PAYLOAD) {
            Serial.printf("[TX] Sent payload (%u bytes): %s",
                          get_len_of_data(), name_msg);
        }
    }

    delay(10);
}