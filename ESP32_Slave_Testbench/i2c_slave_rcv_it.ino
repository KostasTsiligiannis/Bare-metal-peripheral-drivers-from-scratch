/**
 * @details
 *  Hardware Connections:
 *   - GPIO 21 (SDA) <---> STM32 PB7 (I2C1_SDA)
 *   - GPIO 22 (SCL) <---> STM32 PB6 (I2C1_SCL)
 *   - GND           <---> STM32 GND (Common Ground Reference)
 *
 *  Command-Response Protocol:
 *   - 0x51 (CMD_GET_LEN)     : Master reads 1-byte ASCII payload length.
 *   - 0x52 (CMD_GET_PAYLOAD) : Master reads <len> bytes of payload string.
 *
 * @note Architecture & TX Ring-Buffer Pre-loading Notes:
 *  - Because the legacy ESP32 I2C hardware slave peripheral does not
 *    automatically clock-stretch between a Write phase and a Repeated Start
 *    Read phase to execute application logic, the response for the upcoming
 *    command is pre-staged into the driver TX ring buffer ahead of time:
 *     1. During `setup()`, the 1-byte payload length is pre-loaded so it is
 *        immediately available when the Master issues 0x51 (CMD_GET_LEN).
 *     2. Upon receiving 0x51, `loop()` stages the full ASCII payload string
 *        in advance for the subsequent 0x52 (CMD_GET_PAYLOAD) request.
 *     3. Upon receiving 0x52, `loop()` re-stages the 1-byte length so the
 *        slave remains synchronized across repeated master reset/test cycles.
 *  - Internal pull-ups (`GPIO_PULLUP_ENABLE`) are enabled for short bench
 *    connections; external 4.7 kOhm pull-up resistors to 3.3 V are recommended
 *    for longer wiring or higher bus capacitance.
 ******************************************************************************
 */

#include <Arduino.h>
#include <driver/i2c.h>
#include <string.h>

/* ============================================================================
 * Hardware & Protocol Configuration Macros
 * ============================================================================ */
#define I2C_SLAVE_NUM          I2C_NUM_0   /* ESP32 hardware I2C controller 0 */
#define SLAVE_ADDR             0x68        /* 7-bit I2C slave device address */
#define I2C_SDA_PIN            21          /* GPIO assigned to I2C SDA */
#define I2C_SCL_PIN            22          /* GPIO assigned to I2C SCL */

#define I2C_SLAVE_TX_BUF_LEN   256         /* Driver TX ring buffer size (bytes) */
#define I2C_SLAVE_RX_BUF_LEN   256         /* Driver RX ring buffer size (bytes) */

#define CMD_GET_LEN            0x51        /* Command requesting payload length */
#define CMD_GET_PAYLOAD        0x52        /* Command requesting payload string */

/* ============================================================================
 * Slave Response Payload
 * ============================================================================ */
static const char name_msg[] = "Welcome to our home\n";

/* ============================================================================
 * Arduino Entry Points
 * ============================================================================ */

/**
 * @brief  Initializes UART logging, configures I2C0 in native slave mode, and
 *         pre-loads the initial payload length byte into the TX buffer.
 */
void setup()
{
    Serial.begin(115200);

    /* Configure ESP32 I2C0 peripheral parameters in Slave mode */
    i2c_config_t conf_slave;
    memset(&conf_slave, 0, sizeof(conf_slave));

    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.sda_io_num = (gpio_num_t)I2C_SDA_PIN;
    conf_slave.scl_io_num = (gpio_num_t)I2C_SCL_PIN;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = SLAVE_ADDR;

    i2c_param_config(I2C_SLAVE_NUM, &conf_slave);
    i2c_driver_install(I2C_SLAVE_NUM, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);

    /* Pre-stage 1-byte payload length for the initial 0x51 (CMD_GET_LEN) read */
    uint8_t payload_len = (uint8_t)strlen(name_msg);
    i2c_slave_write_buffer(I2C_SLAVE_NUM, &payload_len, 1, portMAX_DELAY);

    Serial.printf("ESP32 Native I2C Slave (Repeated Start Ready) at 0x%02X\n", SLAVE_ADDR);
}

/**
 * @brief  Blocks on incoming master command bytes from the RX ring buffer and
 *         stages the next expected response in the TX ring buffer.
 */
void loop()
{
    uint8_t cmd = 0;
    int rx_len = i2c_slave_read_buffer(I2C_SLAVE_NUM, &cmd, 1, portMAX_DELAY);

    if (rx_len > 0)
    {
        uint8_t payload_len = (uint8_t)strlen(name_msg);

        if (cmd == CMD_GET_LEN)
        {
            /* Length byte already served from pre-load; stage payload for 0x52 */
            i2c_slave_write_buffer(I2C_SLAVE_NUM, (const uint8_t *)name_msg, payload_len, portMAX_DELAY);
        }
        else if (cmd == CMD_GET_PAYLOAD)
        {
            /* Payload already served; re-stage length byte for next cycle */
            i2c_slave_write_buffer(I2C_SLAVE_NUM, &payload_len, 1, portMAX_DELAY);
        }
    }
}
