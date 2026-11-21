/**
 ******************************************************************************
 * @brief   On-demand I2C Master controller running on ESP32 (Arduino Wire)
 *          that queries an STM32F303K8 I2C1 v2 Slave (0x68) at 100 kHz
 *          Standard Mode using a two-phase command-response protocol:
 *          CMD_GET_LEN (0x51) -> 1-byte length, followed by
 *          CMD_GET_PAYLOAD (0x52) -> <data_len> ASCII payload bytes.
 *
 * @details
 *  - Triggering: Waits for ASCII character 's' on UART (115200 baud) before
 *    initiating a single bus transaction sequence.
 *  - Timing & Bus Interoperability: Inserts a 1 ms settling delay after each
 *    STOP condition so the interrupt-driven STM32F303K8 slave can latch the
 *    command byte, flush TXDR, and prepare response bytes prior to the
 *    master's subsequent read phase.
 *  - Bounds & Buffer Safety: Validates received payload length against
 *    RX_BUF_SIZE and explicitly null-terminates rcv_buf before printing.
 ******************************************************************************
 */

#include <Arduino.h>
#include <Wire.h>

/* ============================================================================
 * Bus Topology & Protocol Command Definitions
 * ============================================================================ */
#define SLAVE_ADDR      0x68        /* 7-bit I2C target address of STM32F303K8 slave */
#define I2C_SDA_PIN     21          /* ESP32 GPIO assigned to I2C SDA line */
#define I2C_SCL_PIN     22          /* ESP32 GPIO assigned to I2C SCL line */
#define CMD_GET_LEN     0x51        /* Protocol opcode requesting 1-byte payload length */
#define CMD_GET_PAYLOAD 0x52        /* Protocol opcode requesting payload byte stream */
#define RX_BUF_SIZE     64          /* Static receive buffer capacity including null terminator */

/* ============================================================================
 * Receive Buffer & Transaction State
 * ============================================================================ */
static uint8_t rcv_buf[RX_BUF_SIZE]; /* Buffer holding received ASCII payload + '\0' */
static uint8_t data_len = 0;           /* Expected payload length reported by slave */

/**
 * @brief  Initializes UART serial console and configures ESP32 I2C master bus
 *         at 100 kHz Standard Mode.
 * @return None
 */
void setup()
{
    Serial.begin(115200);
    delay(1000);

    /* Initialize I2C master on SDA=GPIO21, SCL=GPIO22 at 100 kHz SM */
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000);
}

/**
 * @brief  Waits for serial trigger 's' and executes one complete
 *         length-then-payload I2C transaction against the STM32 slave.
 * @return None
 */
void loop()
{
    Serial.println("ESP32 Master");
    Serial.println("Send character \"s\" to begin");
    Serial.println("-----------------------------");

    /* Block until explicit trigger character 's' arrives over UART */
    char in_read = 0;
    do
    {
        while (!Serial.available())
        {
            /* Yield CPU cycles while waiting for serial command */
        }
        in_read = (char)Serial.read();
    } while (in_read != 's');

    Serial.println("Starting..");

    /* Phase 1: Write CMD_GET_LEN (0x51) to slave and release bus with STOP */
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(CMD_GET_LEN);
    Wire.endTransmission();

    /* Allow slave ISR to latch 0x51 and prepare length byte before master read */
    delay(1);

    /* Phase 2: Read 1-byte payload length from slave */
    Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)1);
    data_len = 0;
    if (Wire.available())
    {
        data_len = (uint8_t)Wire.read();
    }

    Serial.print("Data Length:");
    Serial.println(data_len, DEC);

    /* Guard against zero or buffer-overflowing lengths before requesting payload */
    if (data_len == 0 || data_len >= sizeof(rcv_buf))
    {
        Serial.println("Invalid length");
        Serial.println("*********************END*********************");
        return;
    }

    /* Phase 3: Write CMD_GET_PAYLOAD (0x52) to slave and release bus with STOP */
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(CMD_GET_PAYLOAD);
    Wire.endTransmission();

    /* Allow slave ISR to process 0x52, flush stale TXDR data, and stage payload */
    delay(1);

    /* Phase 4: Read <data_len> payload bytes and append C-string null terminator */
    Wire.requestFrom((uint8_t)SLAVE_ADDR, data_len);

    uint32_t i = 0;
    while (Wire.available() && i < data_len)
    {
        rcv_buf[i] = (uint8_t)Wire.read();
        i++;
    }
    rcv_buf[i] = '\0';

    Serial.print("Data:");
    Serial.println((char *)rcv_buf);
    Serial.println("*********************END*********************");
}