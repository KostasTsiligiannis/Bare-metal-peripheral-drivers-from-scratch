/**
 * @brief SPI Slave implementation for ESP32-based command handling.
 *
 * @details
 * This module implements the SPI Slave interface on the ESP32, enabling
 * bidirectional communication with an STM32 SPI Master.
 *
 * The communication protocol uses fixed-length 4-byte frames and supports
 * the following commands:
 *
 * - COMMAND_LED_CTRL: Controls the state of the ESP32 onboard LED.
 * - COMMAND_SENSOR_READ: Acquires an ADC measurement and returns its
 *   scaled 8-bit representation.
 *
 * Each transaction includes an acknowledgment (ACK) byte that allows
 * the Master to verify successful protocol communication.
 *
 * SPI Configuration:
 * - Interface: VSPI
 * - Operating Mode: SPI Slave
 * - Data Mode: SPI Mode 0 (CPOL = 0, CPHA = 0)
 * - Clock Pin (SCK): GPIO18
 * - Master Input / Slave Output (MISO): GPIO19
 * - Master Output / Slave Input (MOSI): GPIO23
 * - Slave Select (SS): GPIO5
 *
 * @note The SPI Master and Slave must use matching clock polarity,
 *       clock phase, and data frame configuration.
 */

#include <ESP32SPISlave.h>
#include <string.h>


// ============================================================================
// 1. SPI PIN CONFIGURATION
// ============================================================================

/**
 * @brief ESP32 VSPI pin assignments.
 *
 * These pins define the physical SPI interface used for communication
 * with the external STM32 Master.
 */
#define SPI_SCK     18  /**< SPI clock signal */
#define SPI_MISO    19  /**< Master In Slave Out */
#define SPI_MOSI    23  /**< Master Out Slave In */
#define SPI_SS      5   /**< Slave Select / Chip Select */


// ============================================================================
// 2. PROTOCOL COMMANDS AND RESPONSE DEFINITIONS
// ============================================================================

/**
 * @brief Command identifiers exchanged between the STM32 Master
 *        and the ESP32 Slave.
 */
#define COMMAND_LED_CTRL     0x50  /**< LED control command */
#define COMMAND_SENSOR_READ  0x51  /**< ADC sensor read command */

/**
 * @brief Protocol acknowledgment byte.
 *
 * The Slave transmits this value in the second byte of each response
 * frame to acknowledge the transaction.
 */
#define ACK                  0xF5

/**
 * @brief GPIO assignment for the onboard ESP32 LED.
 */
#define LED_PIN              2


// ============================================================================
// 3. SPI SLAVE INSTANCE AND COMMUNICATION BUFFERS
// ============================================================================

/**
 * @brief SPI Slave controller instance.
 *
 * Provides the interface for configuring the ESP32 as an SPI Slave
 * and handling queued data transactions.
 */
ESP32SPISlave slave;

/**
 * @brief Receive buffer.
 *
 * Stores the four bytes transmitted by the STM32 Master during
 * each SPI transaction.
 *
 * Frame structure:
 * rx_buffer[0] : Command identifier
 * rx_buffer[1] : Reserved / dummy byte
 * rx_buffer[2] : Command argument (e.g. GPIO or ADC pin)
 * rx_buffer[3] : Command argument (e.g. LED state)
 */
uint8_t rx_buffer[4];

/**
 * @brief Transmit buffer.
 *
 * Contains the four response bytes transmitted to the Master
 * during the corresponding SPI transaction.
 *
 * Frame structure:
 * tx_buffer[0] : Dummy response byte
 * tx_buffer[1] : Acknowledgment byte
 * tx_buffer[2] : Dummy response byte
 * tx_buffer[3] : Sensor measurement or default value
 */
uint8_t tx_buffer[4];

/**
 * @brief Stores the latest ADC measurement in 8-bit format.
 *
 * The raw ESP32 ADC reading is scaled from its 12-bit range
 * (0-4095) to an 8-bit range (0-255).
 */
uint8_t sensor_reply_data = 0;


// ============================================================================
// 4. INITIALIZATION
// ============================================================================

/**
 * @brief Initializes the ESP32 SPI Slave interface and GPIO peripherals.
 *
 * Configures the onboard LED as a digital output, initializes the
 * SPI interface in Mode 0, and prepares the Slave for incoming
 * communication from the STM32 Master.
 */
void setup()
{
    // Initialize the serial interface for runtime diagnostics.
    Serial.begin(115200);
    delay(1000);

    // Configure the onboard LED and set its initial state to OFF.
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Configure SPI clock polarity and phase (SPI Mode 0).
    slave.setDataMode(SPI_MODE0);

    // Initialize the ESP32 VSPI peripheral in Slave mode.
    slave.begin(VSPI, SPI_SCK, SPI_MISO, SPI_MOSI, SPI_SS);

    // Report successful initialization.
    Serial.println();
    Serial.println("ESP32 SPI Slave Initialized");
    Serial.println("Waiting for STM32 Master...");
    Serial.println("--------------------------------");
}


// ============================================================================
// 5. SPI TRANSACTION HANDLING AND COMMAND PROCESSING
// ============================================================================

/**
 * @brief Continuously receives and processes SPI command frames.
 *
 * Each iteration performs the following operations:
 *
 * 1. Clears the communication buffers.
 * 2. Prepares the four-byte response frame.
 * 3. Queues the SPI transaction.
 * 4. Waits for the Master to complete the transfer.
 * 5. Decodes the received command.
 * 6. Executes the corresponding operation.
 *
 * The function supports LED control and ADC sensor acquisition.
 */
void loop()
{
    // ------------------------------------------------------------------------
    // 5.1 Prepare communication buffers
    // ------------------------------------------------------------------------

    // Clear previous transaction data.
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(tx_buffer, 0, sizeof(tx_buffer));

    // Construct the four-byte response frame.
    tx_buffer[0] = 0x00;             // Dummy response to command byte.
    tx_buffer[1] = ACK;              // Protocol acknowledgment.
    tx_buffer[2] = 0x00;             // Dummy response to argument byte.
    tx_buffer[3] = sensor_reply_data; // Latest available sensor measurement.


    // ------------------------------------------------------------------------
    // 5.2 Queue and execute SPI transaction
    // ------------------------------------------------------------------------

    // Queue a full-duplex SPI transaction.
    // The Slave simultaneously transmits tx_buffer and receives rx_buffer.
    slave.queue(tx_buffer, rx_buffer, sizeof(rx_buffer));

    // Block execution until the Master completes the transaction.
    auto result = slave.wait(portMAX_DELAY);

    // Ignore unsuccessful or empty transaction results.
    if (result.empty())
    {
        return;
    }


    // ------------------------------------------------------------------------
    // 5.3 Decode received command
    // ------------------------------------------------------------------------

    // Extract the command identifier from the first received byte.
    uint8_t command = rx_buffer[0];


    // ========================================================================
    // 6. COMMAND: LED CONTROL (0x50)
    // ========================================================================

    if (command == COMMAND_LED_CTRL)
    {
        // Extract the target GPIO and requested output state.
        uint8_t pin = rx_buffer[2];
        uint8_t value = rx_buffer[3];

        Serial.println("RCVD: COMMAND_LED_CTRL");

        // Verify that the requested GPIO corresponds to the onboard LED.
        if (pin == LED_PIN)
        {
            // Set the LED output to HIGH.
            if (value == 1)
            {
                digitalWrite(LED_PIN, HIGH);
                Serial.println("LED ON");
            }

            // Set the LED output to LOW.
            else if (value == 0)
            {
                digitalWrite(LED_PIN, LOW);
                Serial.println("LED OFF");
            }
        }

        // Reset the sensor response field for non-sensor transactions.
        sensor_reply_data = 0x00;
    }


    // ========================================================================
    // 7. COMMAND: ADC SENSOR READ (0x51)
    // ========================================================================

    else if (command == COMMAND_SENSOR_READ)
    {
        // Extract the ADC input pin from the received command frame.
        uint8_t analog_pin = rx_buffer[2];

        Serial.println("RCVD: COMMAND_SENSOR_READ");

        // Acquire a raw ADC measurement.
        // ESP32 ADC resolution: 12 bits (0-4095).
        uint32_t adc_raw = analogRead(analog_pin);

        // Scale the 12-bit measurement to an 8-bit value (0-255).
        // This allows the measurement to be transmitted using one byte.
        sensor_reply_data = (uint8_t)map(adc_raw, 0, 4095, 0, 255);

        // Report the acquired and scaled ADC values.
        Serial.print("ADC Raw Value: ");
        Serial.print(adc_raw);
        Serial.print(" -> Mapped 8-bit Value: ");
        Serial.println(sensor_reply_data);
    }


    // ========================================================================
    // 8. UNKNOWN COMMAND HANDLING
    // ========================================================================

    else
    {
        // Report unsupported command identifiers.
        Serial.print("Unknown Command: 0x");
        Serial.println(command, HEX);
    }


    // ------------------------------------------------------------------------
    // 9. TRANSACTION COMPLETION
    // ------------------------------------------------------------------------

    // Print a separator between consecutive transactions.
    Serial.println("--------------------------------");
}
