/*
 * ============================================================================
 * File:        uart_only_send.c
 * Target:      STM32F303K8 (ARM Cortex-M4)
 * Description: Demonstrates blocking UART transmission (TX-only) via USART1
 *              at 115200 baud (8-N-1) using custom bare-metal peripheral drivers.
 *
 * ----------------------------------------------------------------------------
 *   STM32F303K8          ESP32 (Receiver)     Logic Analyzer (Saleae)
 *   -----------          ----------------     -----------------------
 *   PB6 (USART1_TX) ---> GPIO 16 (RX2)        CH0 (D0) - Monitor STM32 Tx
 *   PB7 (USART1_RX) <--- GPIO 17 (TX2)        (Configured for future RX use)
 *   GND             ---- GND                  GND (Common ground reference)
 *                        GPIO 1  (TX0)   ---> CH1 (D1) - Monitor ESP32 USB Out
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "stm32f303k8.h"

/* Transmit message buffer */
char msg[1024] = "UART Tx testing...\n\r";

/* Global USART1 peripheral handle */
USART_Handle_t usart1_handle;

/*
 * Configures USART1 peripheral parameters:
 * - Baud Rate: 115200 bps
 * - Frame Format: 8 data bits, No parity, 1 stop bit (8-N-1)
 * - Mode: Transmitter only, Hardware flow control disabled
 */
void USART1_Init(void)
{
    usart1_handle.pUSARTx = DRV_USART1;
    usart1_handle.USART_Config.USART_Baud = USART_STD_BAUD_115200;
    usart1_handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;
    usart1_handle.USART_Config.USART_Mode = USART_MODE_ONLY_TX;
    usart1_handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;
    usart1_handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
    usart1_handle.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;

    USART_Init(&usart1_handle);
}

/*
 * Configures GPIOB pins for USART1 Alternate Function (AF7):
 * - PB6: USART1_TX (Push-Pull, Pull-Up enabled, Fast speed)
 * - PB7: USART1_RX (Push-Pull, Pull-Up enabled, Fast speed)
 */
void USART1_GPIOInit(void)
{
    GPIO_Handle_t usart_gpios;

    usart_gpios.pGPIOx = DRV_GPIOB;
    usart_gpios.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    usart_gpios.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    usart_gpios.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
    usart_gpios.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    usart_gpios.GPIO_PinConfig.GPIO_PinAltFunMode = 7;

    /* Configure PB6 as USART1_TX */
    usart_gpios.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
    GPIO_Init(&usart_gpios);

    /* Configure PB7 as USART1_RX */
    usart_gpios.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
    GPIO_Init(&usart_gpios);
}

int main(void)
{
    /* 1. Initialize GPIO pins for USART1 */
    USART1_GPIOInit();

    /* 2. Configure USART1 peripheral settings */
    USART1_Init();

    /* 3. Enable the USART1 peripheral (UE bit in CR1) */
    USART_PeripheralControl(DRV_USART1, ENABLE);

    /* 4. Transmit the test string over USART1 */
    USART_SendData(&usart1_handle, (uint8_t*)msg, strlen(msg));

    return 0;
}
