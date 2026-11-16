/**
 ******************************************************************************
 * @file    esp32_i2c_slave_repeated_start.ino
 * @brief   ESP32 I2C Slave using the native ESP-IDF driver (driver/i2c.h) and
 *          direct I2C0 hardware FIFO inspection to support Repeated Start (Sr)
 *          transactions from an STM32F303K8 Master at 100 kHz.
 *
 * Pinout & Bus Configuration:
 *  - GPIO 21 : I2C SDA (connected to STM32 PB7)
 *  - GPIO 22 : I2C SCL (connected to STM32 PB6)
 *  - Slave Address : 0x68 (7-bit addressing)
 ******************************************************************************
 */

#include <Arduino.h>
#include <driver/i2c.h>
#include <string.h>
#include <soc/i2c_reg.h>
#include <soc/i2c_struct.h>

/* ============================================================================
 * Hardware & Protocol Definitions
 * ============================================================================ */
#define I2C_SLAVE_NUM           I2C_NUM_0   /* ESP32 I2C controller instance */
#define SLAVE_ADDR              0x68        /* 7-bit I2C slave address */
#define I2C_SDA_PIN             21          /* SDA pin assignment */
#define I2C_SCL_PIN             22          /* SCL pin assignment */

#define I2C_SLAVE_TX_BUF_LEN    256         /* Slave TX ring/FIFO buffer size */
#define I2C_SLAVE_RX_BUF_LEN    256         /* Slave RX ring/FIFO buffer size */

#define CMD_GET_LEN             0x51        /* Command requesting payload length */
#define CMD_GET_PAYLOAD         0x52        /* Command requesting payload data */

/* ============================================================================
 * Payload Buffer
 * ============================================================================ */
char name_msg[32] = "Welcome to FastBit EBA\n";

/* ============================================================================
 * Arduino Entry Points
 * ============================================================================ */

/**
 * @brief  Initializes serial logging and configures I2C0 in slave mode using
 *         the ESP-IDF driver with internal pull-ups enabled.
 */
void setup() {
    Serial.begin(115200);
    delay(1000);

    i2c_config_t conf_slave;
    memset(&conf_slave, 0, sizeof(conf_slave));

    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.sda_io_num = (gpio_num_t)I2C_SDA_PIN;
    conf_slave.scl_io_num = (gpio_num_t)I2C_SCL_PIN;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = SLAVE_ADDR;
    conf_slave.clk_flags = 0;

    /* Apply slave configuration and install the ESP-IDF I2C driver */
    i2c_param_config(I2C_SLAVE_NUM, &conf_slave);
    i2c_driver_install(I2C_SLAVE_NUM, conf_slave.mode,
                       I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);

    Serial.printf("ESP32 Native I2C Slave (Repeated Start ready) at 0x%02X\n",
                  SLAVE_ADDR);
}

/**
 * @brief  Polls the I2C0 hardware RX FIFO counter without waiting for a STOP
 *         condition, flushes stale TX FIFO data, and loads the requested
 *         response (length or payload) for the subsequent master read.
 */
void loop() {
    /* Inspect hardware RX FIFO count directly to react before a STOP condition */
    uint32_t rx_fifo_cnt = I2C0.status_reg.rx_fifo_cnt;

    if (rx_fifo_cnt > 0) {
        /* Pop the command byte directly from the I2C0 hardware FIFO */
        uint8_t cmd = I2C0.fifo_data.data;

        /* Flush stale TX FIFO bytes to prevent residual data from being clocked out */
        i2c_reset_tx_fifo(I2C_SLAVE_NUM);

        uint8_t payload_len = (uint8_t)strlen(name_msg);

        if (cmd == CMD_GET_LEN) {
            /* Queue 1-byte payload length for the master's length read phase */
            i2c_slave_write_buffer(I2C_SLAVE_NUM, &payload_len, 1, 0);
            Serial.printf("[RX] 0x51 -> Loaded len: %u\n", payload_len);
        } else if (cmd == CMD_GET_PAYLOAD) {
            /* Queue full payload string for the master's data read phase */
            i2c_slave_write_buffer(I2C_SLAVE_NUM, (uint8_t *)name_msg,
                                   payload_len, 0);
            Serial.printf("[RX] 0x52 -> Loaded payload (%u bytes)\n", payload_len);
        }
    }
}