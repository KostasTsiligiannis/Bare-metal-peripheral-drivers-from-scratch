/*
 * stm32f303k8_spi_driver.h
 *
 *  Created on: 21 Οκτ 2026
 *      Author: panek
 */

#ifndef INC_STM32F303K8_SPI_DRIVER_H_
#define INC_STM32F303K8_SPI_DRIVER_H_

#include "stm32f303k8.h"

typedef struct{
	uint8_t SPI_DeviceMode;
	uint8_t SPI_BusConfig;
	uint8_t SPI_SclkSpeed;
	uint8_t SPI_DFF;
	uint8_t SPI_CPOL;
	uint8_t SPI_CPHA;
	uint8_t SPI_SSM;
}SPI_Config_t;

/*
 *Handle structure for SPIx peripheral
 */
typedef struct
{
	SPI1_RegDef_t 	*pSPIx;   /*!< This holds the base address of SPIx(x:0,1,2) peripheral >*/
	SPI_Config_t 	SPIConfig;
	uint8_t 		*pTxBuffer; /* !< To store the app. Tx buffer address > */
	uint8_t 		*pRxBuffer;	/* !< To store the app. Rx buffer address > */
	uint32_t 		TxLen;		/* !< To store Tx len > */
	uint32_t 		RxLen;		/* !< To store Tx len > */
	uint8_t 		TxState;	/* !< To store Tx state > */
	uint8_t 		RxState;	/* !< To store Rx state > */
}SPI_Handle_t;

/*
 * SPI application states
 */
#define SPI_READY 					0
#define SPI_BUSY_IN_RX 				1
#define SPI_BUSY_IN_TX 				2

/*
 * Possible SPI Application events
 */
#define SPI_EVENT_TX_CMPLT   1
#define SPI_EVENT_RX_CMPLT   2
#define SPI_EVENT_OVR_ERR    3
#define SPI_EVENT_CRC_ERR    4

/*
 * @SPI_DeviceMode
 */
#define SPI_DEVICE_MODE_MASTER	1
#define SPI_DEVICE_MODE_SLAVE	0

/*
 * @SPI_BusConfig
 */
#define SPI_BUS_CONFIG_FD				1
#define SPI_BUS_CONFIG_HD				2
#define SPI_BUS_CONFIG_SIMPLE_RXONLY		3

/*
 * @SPI_SclkSpeed
 */
#define SPI_SCLK_SPEED_DIV2					0
#define SPI_SCLK_SPEED_DIV4					1
#define SPI_SCLK_SPEED_DIV8					2
#define SPI_SCLK_SPEED_DIV16				3
#define SPI_SCLK_SPEED_DIV32				4
#define SPI_SCLK_SPEED_DIV64 				5
#define SPI_SCLK_SPEED_DIV128 				6
#define SPI_SCLK_SPEED_DIV256 				7

/*
 * @SPI_DFF
 */
#define SPI_DFF_8BITS		0
#define SPI_DFF_16BITS		1

/*
 * @SPI_CPOL
 */
#define SPI_CPOL_HIGH		1
#define SPI_CPOL_LOW		0

/*
 * @SPI_CPHA
 */
#define SPI_CPHA_HIGH		1
#define SPI_CPHA_LOW		0

/*
 * @SPI_CPHA
 */
#define SPI_SSM_EN		1
#define SPI_SSM_DI		0

/*
 * SPI related status flag definitions
 */
#define SPI_TXE_FLAG    ( 1 << DRV_SPI_SR_TXE)
#define SPI_RXNE_FLAG   ( 1 << DRV_SPI_SR_RXNE)
#define SPI_BUSY_FLAG   ( 1 << DRV_SPI_SR_BSY)

/************************************************************************************************
 *                         APIs supported by this driver
 ************************************************************************************************/

/************************************************************************************************
 * @fn               - SPI_PeriClockControl
 * @brief            - This function enables or disables the peripheral clock for the given SPI peripheral.
 * @param[in]        - pSPIx : base address of the SPI peripheral
 * @param[in]        - EnorDi : ENABLE or DISABLE macros
 * @return           - none
 * @Note             - none
 ************************************************************************************************/
void SPI_PeriClockControl(SPI1_RegDef_t *pSPIx, uint8_t EnorDi);


/************************************************************************************************
 * @fn               - SPI_Init
 * @brief            - This function initializes the given SPI peripheral.
 * @param[in]        - pSPIHandle : pointer to the SPI handle structure containing configuration settings
 * @return           - none
 * @Note             - Configures the SPI peripheral based on the settings provided in the handle structure.
 ************************************************************************************************/
void SPI_Init(SPI_Handle_t *pSPIHandle);


/************************************************************************************************
 * @fn               - SPI_DeInit
 * @brief            - This function de-initializes the given SPI peripheral (resets it to its default configuration).
 * @param[in]        - pSPIx : base address of the SPI peripheral
 * @return           - none
 * @Note             - Uses the RCC peripheral reset register (APB reset) to force the SPI peripheral back to its reset state.
 ************************************************************************************************/
void SPI_DeInit(SPI_Handle_t *pSPIHandle);

/************************************************************************************************
 * @fn               - SPI_SendData
 * @brief            - This function sends data over the SPI peripheral.
 * @param[in]        - pSPIx : base address of the SPI peripheral
 * @param[in]        - pTxBuffer : pointer to the transmit data buffer
 * @param[in]        - Len : length of the data to be transmitted
 * @return           - none
 * @Note             - Transmits data using the SPI data register.
 ************************************************************************************************/
void SPI_SendData(SPI1_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len);


/************************************************************************************************
 * @fn               - SPI_ReceiveData
 * @brief            - This function receives data from the SPI peripheral.
 * @param[in]        - pSPIx : base address of the SPI peripheral
 * @param[in]        - pRxBuffer : pointer to the receive data buffer
 * @param[in]        - Len : length of the data to be received
 * @return           - none
 * @Note             - Receives data using the SPI data register.
 ************************************************************************************************/

void SPI_RecieveData(SPI1_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len);

/************************************************************************************************
 * @fn               - SPI_IRQInterruptConfig
 * @brief            - This function enables or disables an interrupt in the Nested Vectored Interrupt Controller (NVIC).
 * @param[in]        - IRQNumber : Interrupt Request number to configure
 * @param[in]        - EnorDi : ENABLE or DISABLE macros
 * @return           - none
 * @Note             - Enables or disables the specified IRQ in the NVIC.
 ************************************************************************************************/
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);


/************************************************************************************************
 * @fn               - SPI_IRQPriorityConfig
 * @brief            - This function configures the priority of an interrupt in the Nested Vectored Interrupt Controller (NVIC).
 * @param[in]        - IRQNumber : Interrupt Request number whose priority will be configured
 * @param[in]        - IRQPriority : Priority level to assign to the interrupt
 * @return           - none
 * @Note             - Configures the priority of the specified IRQ in the NVIC.
 ************************************************************************************************/
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);


/************************************************************************************************
 * @fn               - SPI_IRQHandling
 * @brief            - This function handles an SPI interrupt for the specified SPI handle.
 * @param[in]        - pHandle : pointer to the SPI handle structure
 * @return           - none
 * @Note             - Handles SPI interrupt events according to the enabled SPI interrupt sources and status flags.
 ************************************************************************************************/
void SPI_IRQHandling(SPI_Handle_t *pHandle);

/************************************************************************************************
 * @fn               - SPI_PeripheralControl
 * @brief            - This function enables or disables the SPI peripheral.
 * @param[in]        - pSPIx : base address of the SPI peripheral
 * @param[in]        - EnorDi : ENABLE or DISABLE macros
 * @return           - none
 * @Note             - Sets or clears the SPE bit in CR1 register.
 ************************************************************************************************/
void SPI_PeripheralControl(SPI1_RegDef_t *pSPIx, uint8_t EnorDi);
void SPI_SSIConfig(SPI1_RegDef_t *pSPIx, uint8_t EnOrDi);
void SPI_SSOEConfig(SPI1_RegDef_t *pSPIx, uint8_t EnOrDi);
uint8_t SPI_GetFlagStatus(SPI1_RegDef_t *pSPIx , uint32_t FlagName);
uint8_t SPI_TransferByte(SPI1_RegDef_t *pSPIx, uint8_t byte_to_send);
uint8_t Send_DataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len);
uint8_t Receive_DataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len);
void SPI_ClearOVRFlag(SPI1_RegDef_t *pSPIx);
void SPI_CloseTransmisson(SPI_Handle_t *pSPIHandle);
void SPI_CloseReception(SPI_Handle_t *pSPIHandle);

/*
 * Application callback
 */
void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle,uint8_t AppEv);

#endif /* INC_STM32F303K8_SPI_DRIVER_H_ */
