/*
 * ============================================================================
 * Description: Receives characters from STM32F303K8 via UART2 (RX2 = GPIO 16),
 *              toggles their letter case (uppercase <-> lowercase), sends
 *              them back to STM32 via UART2 (TX2 = GPIO 17), and prints each
 *              complete message on a new line in the PC Serial Monitor.
 *
 * Hardware Connections (3.3V Logic - Direct Connection):
 * ----------------------------------------------------------------------------
 *   STM32F303K8          ESP32                 Saleae Logic Analyzer
 *   -----------          -----                 ---------------------
 *   PB6 (USART1_TX) ---> GPIO 16 (RX2)   --->  Channel D0 (STM32 -> ESP32)
 *   PB7 (USART1_RX) <--- GPIO 17 (TX2)   --->  Channel D1 (ESP32 -> STM32)
 *   GND             ---- GND             ----  GND (Common Ground Required)
 * ============================================================================
 */

const int RX2_PIN = 16;  // Connects to STM32 PB6 (USART1_TX)
const int TX2_PIN = 17;  // Connects to STM32 PB7 (USART1_RX)

unsigned long lastRxTime = 0;
bool newLineNeeded = false;

char changeCase(char ch) {
  if (ch >= 'A' && ch <= 'Z') {
    return ch + 32;  // Convert uppercase to lowercase
  } else if (ch >= 'a' && ch <= 'z') {
    return ch - 32;  // Convert lowercase to uppercase
  }
  return ch;         // Leave numbers, spaces, and punctuation unchanged
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);

  Serial.println("ESP32 UART Case Converter Ready");
  Serial.println("-------------------------------");
}

void loop() {
  if (Serial2.available()) {
    char in_read = (char)Serial2.read();
    char converted = changeCase(in_read);

    // Send converted character back to STM32 (PB7 / D1)
    Serial2.write(converted);

    // Print character to PC Serial Monitor
    Serial.print(converted);

    // Update timestamp of the last received byte
    lastRxTime = millis();
    newLineNeeded = true;
  }

  // If 20 ms pass without new incoming bytes, print a newline on the PC monitor only
  if (newLineNeeded && (millis() - lastRxTime > 20)) {
    Serial.println();
    newLineNeeded = false;
  }
}
