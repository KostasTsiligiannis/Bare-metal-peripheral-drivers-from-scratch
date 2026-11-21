/*
 * ============================================================================
 * File:        uart_case.c
 * Target:      STM32F303K8 (ARM Cortex-M4)
 * Description: Transmits an extensible list of strings over USART1 (blocking mode)
 *              and receives the case-toggled replies from the ESP32 using
 *              interrupt-driven reception (RXNEIE). Stops once all messages
 *              have been transmitted and verified.
 *
 * Hardware Connections (3.3V Logic - Direct Connection, No Level Shifter):
 * ----------------------------------------------------------------------------
 *   STM32F303K8          ESP32 (Case Converter)   Saleae Logic Analyzer
 *   -----------          ----------------------   ---------------------
 *   PB6 (USART1_TX) ---> GPIO 16 (RX2)       ---> Channel D0 (STM32 -> ESP32)
 *   PB7 (USART1_RX) <--- GPIO 17 (TX2)       ---> Channel D1 (ESP32 -> STM32)
 *   GND             ---- GND                 ---- GND (Common Ground Required)
 *
 * Protocol Configuration: 115200 Baud, 8 Data Bits, No Parity, 1 Stop Bit (8-N-1)
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "stm32f303k8.h"

/* Extensible array of test messages to transmit */
char *msg[] = {"hihihihihihi123", "Hello How are you ?" , "Today is Monday !"};

/* Buffer for storing incoming reply bytes from ESP32 */
char rx_buf[1024];

/* Global USART1 peripheral handle */
USART_Handle_t usart1_handle;

/* Flag set by the application callback when all expected bytes are received */
volatile uint8_t rxCmplt = RESET;

uint8_t g_data = 0;

/* Semihosting initialization prototype for printf console debugging */
extern void initialise_monitor_handles();

/*
 * Configures USART1 peripheral parameters:
 * - Baud Rate: 115200 bps
 * - Frame Format: 8 data bits, No parity, 1 stop bit (8-N-1)
 * - Mode: Full-duplex (TX & RX enabled), Hardware flow control disabled
 */
void USART1_Init(void)
{
	usart1_handle.pUSARTx = DRV_USART1;
	usart1_handle.USART_Config.USART_Baud = USART_STD_BAUD_115200;
	usart1_handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;
	usart1_handle.USART_Config.USART_Mode = USART_MODE_TXRX;
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
	uint32_t cnt = 0;
	uint32_t total_msgs = sizeof(msg) / sizeof(msg[0]);

	/* Initialize ARM Semihosting for printf output */
	initialise_monitor_handles();

	/* Initialize GPIO pins and USART1 peripheral */
	USART1_GPIOInit();
	USART1_Init();

	/* Enable USART1 interrupt in NVIC and activate the peripheral */
	USART_IRQInterruptConfig(IRQ_NO_USART1, ENABLE);
	USART_PeripheralControl(DRV_USART1, ENABLE);

	printf("Application is running\n");

	/* Transmit and verify all messages in the array, then stop */
	while (cnt < total_msgs)
	{
		/* Flush any leftover byte in RDR and clear Overrun flag before starting new reception */
		(void)DRV_USART1->RDR;
		DRV_USART1->ICR = DRV_USART_ICR_ORECF;

		/* Arm interrupt-driven reception for the expected reply length */
		while (USART_ReceiveDataIT(&usart1_handle, (uint8_t *)rx_buf, strlen(msg[cnt])) != USART_READY);

		/* Transmit current message in blocking mode */
		USART_SendData(&usart1_handle, (uint8_t *)msg[cnt], strlen(msg[cnt]));

		/* Wait until all reply bytes are received via ISR */
		while (rxCmplt != SET);

		printf("Transmitted : %s\n", msg[cnt]);

		/* Null-terminate the received buffer and print the reply */
		rx_buf[strlen(msg[cnt])] = '\0';
		printf("Received: %s\n", rx_buf);

		/* Reset reception flag and advance to the next message */
		rxCmplt = RESET;
		cnt++;

	}

	printf("Done!\n");

	return 0;
}

/*
 * USART1 Interrupt Service Routine (Vector defined in startup file)
 */
void USART1_EXTI25_IRQHandler(void)
{
	USART_IRQHandling(&usart1_handle);
}

/*
 * Application callback invoked by the USART driver upon event completion
 */
void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle, uint8_t ApEv)
{
	if (ApEv == USART_EVENT_RX_CMPLT)
	{
		rxCmplt = SET;
	}
	else if (ApEv == USART_EVENT_TX_CMPLT)
	{
		/* Placeholder for non-blocking TX completion handling */
	}
}
