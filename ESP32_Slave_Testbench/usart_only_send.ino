/*
 * ============================================================================
 * Description: Receives UART frames from STM32F303K8 via Serial2 (UART2) and
 *              forwards the received characters to the PC Serial Monitor via USB.
 *
 * ----------------------------------------------------------------------------
 *   STM32F303K8          ESP32                 Saleae Logic Analyzer
 *   -----------          -----                 ---------------------
 *   PB6 (USART1_TX) ---> GPIO 16 (RX2)   --->  Channel D0 (STM32 -> ESP32)
 *   PB7 (USART1_RX) <--- GPIO 17 (TX2)         (Optional: for future TX back)
 *   GND             ---  GND             ---   GND (Common Ground Required)
 *                                              Channel D1 -> GPIO 1 (TX0 / USB)
 *                                              (Monitors ESP32 forwarding to PC)
 *
 * Protocol Configuration: 115200 Baud, 8 Data Bits, No Parity, 1 Stop Bit (8-N-1)
 * ============================================================================
 */

// UART2 Pin Definitions for ESP32
const int RX2_PIN = 16;  // Connects to STM32 PB6 (USART1_TX)
const int TX2_PIN = 17;  // Connects to STM32 PB7 (USART1_RX)

void setup() {
  // Initialize USB Serial (UART0) for debugging output on the PC Serial Monitor
  Serial.begin(115200);

  // Initialize Hardware UART2 (Serial2) to communicate with the STM32F303K8
  // Frame format: 8 data bits, No parity, 1 stop bit (SERIAL_8N1) at 115200 bps
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // Print startup banner to confirm the ESP32 receiver is active
  Serial.println("ESP32 UART Receiver Ready");
  Serial.println("----------------------------");
}

void loop() {
  // Block execution until at least 1 byte arrives in the UART2 receive buffer
  while (!Serial2.available()) {
    // Wait for incoming data from STM32
  }

  // Read one incoming byte from UART2 and cast it to a character
  char in_read = (char)Serial2.read();

  // Forward the received character to the PC via USB Serial (also visible on TX0 / D1)
  Serial.print(in_read);
}
