#include "stm32f303k8_spi_driver.h"

static void  spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void  spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void  spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle);

/* -------------------------------------------------------
 * SPI Peripheral Clock Control
 * ------------------------------------------------------- */
void SPI_PeriClockControl(SPI1_RegDef_t *pSPIx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pSPIx == DRV_SPI1)
        {
            DRV_SPI1_PCLK_EN();
        }
    }
    else
    {
    	DRV_SPI1_PCLK_DI();
    }
}
/* -------------------------------------------------------
 * SPI Initialization
 *
 * Configuration is received through SPI_Handle_t.
 *
 * CPOL and CPHA are configured from main.c.
 * ------------------------------------------------------- */
void SPI_Init(SPI_Handle_t *pSPIHandle)
{
    uint32_t cr1_val = 0;
    uint32_t cr2_val = 0;
    /* Enable SPI peripheral clock */
    SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);
    /* Reset SPI registers before configuration */
    pSPIHandle->pSPIx->CR1 = 0;
    pSPIHandle->pSPIx->CR2 = 0;
    /* ---------------------------------------------------
     * CR1 CONFIGURATION
     * --------------------------------------------------- */
    /* 1. Device Mode: Master */
    cr1_val |= (1U << 2);
    /* 2. Baud Rate: PCLK / 32 */
    cr1_val |= (0x4U << 3);
    /* 3. Clock Polarity (CPOL)
     *
     * CPOL = 0 -> Clock idle LOW
     * CPOL = 1 -> Clock idle HIGH
     *
     * Value comes from main.c
     */
    if (pSPIHandle->SPIConfig.SPI_CPOL == SPI_CPOL_HIGH)
    {
        cr1_val |= (1U << 1);
    }
    else
    {
        cr1_val &= ~(1U << 1);
    }
    /* 4. Clock Phase (CPHA)
     *
     * CPHA = 0 -> First clock edge
     * CPHA = 1 -> Second clock edge
     *
     * Value comes from main.c
     */
    if (pSPIHandle->SPIConfig.SPI_CPHA == SPI_CPHA_HIGH)
    {
        cr1_val |= (1U << 0);
    }
    else
    {
        cr1_val &= ~(1U << 0);
    }

    /* 5. Bit Order: MSB First */
    cr1_val &= ~(1U << 7);
    /* 6. Software Slave Management
     *
     * SSM = 0 -> Hardware NSS
     */
    cr1_val &= ~(1U << 9);
    /* Write CR1 configuration */
    pSPIHandle->pSPIx->CR1 = cr1_val;
    /* ---------------------------------------------------
     * CR2 CONFIGURATION
     * --------------------------------------------------- */
    /* 1. Data Size: 8 bits
     *
     * STM32F303:
     * DS[3:0] = 0111 -> 8-bit data frame
     */
    cr2_val |= (0x7U << 8);

    /* 2. FIFO Reception Threshold: 8 bits */
    cr2_val |= (1U << 12);
    /* Write CR2 configuration */
    pSPIHandle->pSPIx->CR2 = cr2_val;
}

void SPI_DeInit(SPI_Handle_t *pSPIx){

		if(DRV_SPI1){
				DRV_SPI1_REG_RESET();
			}
}

/* -------------------------------------------------------
 * SPI Peripheral Control
 *
 * Enable / Disable SPI peripheral
 * ------------------------------------------------------- */
void SPI_PeripheralControl(SPI1_RegDef_t *pSPIx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        pSPIx->CR1 |= (1 << 6);
    }
    else
    {
        pSPIx->CR1 &= ~(1 << 6);
    }
}
/* -------------------------------------------------------
 * SPI SSOE Configuration
 *
 * Hardware NSS Output Enable
 * ------------------------------------------------------- */
void SPI_SSOEConfig(SPI1_RegDef_t *pSPIx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        pSPIx->CR2 |= (1 << 2);
    }
    else
    {
        pSPIx->CR2 &= ~(1 << 2);
    }
}
/* -------------------------------------------------------
 * SPI Get Flag Status
 * ------------------------------------------------------- */
uint8_t SPI_GetFlagStatus(SPI1_RegDef_t *pSPIx, uint32_t FlagName)
{
    if (pSPIx->SR & FlagName)
    {
        return FLAG_SET;
    }

    return FLAG_RESET;
}
/* -------------------------------------------------------
 * SPI Transfer Byte
 *
 * Full-duplex transfer:
 * Every transmitted byte also receives one byte.
 * ------------------------------------------------------- */
uint8_t SPI_TransferByte(SPI1_RegDef_t *pSPIx, uint8_t byte_to_send)
{
    uint8_t received_byte;
    /* Wait until transmit buffer is empty */
    while (SPI_GetFlagStatus(pSPIx, SPI_TXE_FLAG) == FLAG_RESET);
    /* Transmit one byte */
    *(__vo uint8_t *)&pSPIx->DR = byte_to_send;
    /* Wait until one byte is received */
    while (SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == FLAG_RESET);
    /* Read received byte */
    received_byte = *(__vo uint8_t *)&pSPIx->DR;
    return received_byte;
}

void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi){

	if(EnorDi == ENABLE){
			if(IRQNumber <= 31){
				//programm ISER0 register
				*DRV_NVIC_ISER0 |= (1 << IRQNumber);
			} else if(IRQNumber >31 && IRQNumber < 64){
				//programm ISER1 register
				*DRV_NVIC_ISER1 |= (1 << (IRQNumber % 32));
			}else if(IRQNumber >= 64 && IRQNumber < 96){
				//programm ISER2 register
				*DRV_NVIC_ISER2 |= (1 << (IRQNumber % 64));
			}
		} else {
			if(IRQNumber <= 31){
				//programm ICER0 register
				*DRV_NVIC_ICER0 |= (1 << IRQNumber);
			} else if(IRQNumber >31 && IRQNumber < 64){
				//programm ICER1 register
				*DRV_NVIC_ICER1 |= (1 << (IRQNumber % 32));
				}else if(IRQNumber >= 64 && IRQNumber < 96){
				//programm ICER2 register
				*DRV_NVIC_ICER2 |= (1 << (IRQNumber % 64));
			}
		}
}

void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority){

	 uint8_t iprx = IRQNumber / 4;
	 uint8_t iprx_section = IRQNumber % 4;
	 uint8_t shift_amount = (8 * iprx_section) + (8 - DRV_NO_PR_BITS_IMPLEMENTED);
	 uint32_t priority_mask = 0xF << shift_amount;
	 /* Clear the priority field before setting the new priority */
	 *(DRV_NVIC_PR_BASE_ADDR + iprx) &= ~priority_mask;
	 /* Set the requested priority */
	  *(DRV_NVIC_PR_BASE_ADDR + iprx) |= ((IRQPriority & 0xF) << shift_amount);
}

uint8_t Send_DataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len)
{
    // Store the current transmission state.
    // This allows us to check whether another transmission is already in progress.
    uint8_t state = pSPIHandle->TxState;

    // Check whether the SPI peripheral is not already busy transmitting.
    if (state != SPI_BUSY_IN_TX)
    {
        // Store the address of the transmission buffer in the SPI Handle.
        pSPIHandle->pTxBuffer = pTxBuffer;

        // Store the number of bytes that need to be transmitted.
        pSPIHandle->TxLen = Len;

        // Set the SPI transmission state to BUSY.
        pSPIHandle->TxState = SPI_BUSY_IN_TX;

        // Enable the TXE interrupt.
        // When the TXE flag becomes 1 (Transmit buffer empty),
        // the corresponding SPI Interrupt Handler is triggered
        // and takes care of transmitting the data.
        pSPIHandle->pSPIx->CR2 |= (1 << DRV_SPI_CR2_TXEIE);
    }
    // Return the previous transmission state.
    return state;
}

uint8_t Receive_DataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    // Store the current reception state.
    // This allows us to check whether another reception is already in progress.
    uint8_t state = pSPIHandle->RxState;

    // Check whether the SPI peripheral is not already busy receiving.
    if (state != SPI_BUSY_IN_RX)
    {
        // Store the address of the reception buffer in the SPI Handle.
        pSPIHandle->pRxBuffer = pRxBuffer;

        // Store the number of bytes that need to be received.
        pSPIHandle->RxLen = Len;

        // Set the SPI reception state to BUSY.
        pSPIHandle->RxState = SPI_BUSY_IN_RX;

        // Enable the RXNE interrupt.
        // When the RXNE flag becomes 1 (Receive buffer not empty),
        // the corresponding SPI Interrupt Handler is triggered
        // and takes care of reading the received data.
        pSPIHandle->pSPIx->CR2 |= (1 << DRV_SPI_CR2_RXNEIE);
    }
    // Return the previous reception state.
    return state;
}

void SPI_IRQHandling(SPI_Handle_t *pHandle)
{

	uint8_t temp1 , temp2;
	//first lets check for TXE
	temp1 = pHandle->pSPIx->SR & ( 1 << DRV_SPI_SR_TXE);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << DRV_SPI_CR2_TXEIE);
	if( temp1 && temp2)
	{
		//handle TXE
		spi_txe_interrupt_handle(pHandle);
	}
	// check for RXNE
	temp1 = pHandle->pSPIx->SR & ( 1 << DRV_SPI_SR_RXNE);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << DRV_SPI_CR2_RXNEIE);

	if( temp1 && temp2)
	{
		//handle RXNE
		spi_rxne_interrupt_handle(pHandle);
	}
	// check for ovr flag
	temp1 = pHandle->pSPIx->SR & ( 1 << DRV_SPI_SR_OVR);
	temp2 = pHandle->pSPIx->CR2 & ( 1 << DRV_SPI_CR2_ERRIE);
	if( temp1 && temp2)
	{
		//handle ovr error
		spi_ovr_err_interrupt_handle(pHandle);
	}
}

//some helper function implementations
static void  spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	// check the DFF bit in CR1
	if( (pSPIHandle->pSPIx->CR1 & ( 1 << DRV_SPI_CR1_DFF) ) )
	{
		//16 bit DFF
		//1. load the data in to the DR
		pSPIHandle->pSPIx->DR = *((uint16_t*)pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen -= 2;
		pSPIHandle->pTxBuffer += 2;;
	}else
	{
		//8 bit DFF
		pSPIHandle->pSPIx->DR = *pSPIHandle->pTxBuffer;
		pSPIHandle->TxLen--;
		pSPIHandle->pTxBuffer++;
	}

	if(! pSPIHandle->TxLen)
	{
		//TxLen is zero , so close the spi transmission and inform the application that
		//TX is over.
		//this prevents interrupts from setting up of TXE flag
		SPI_CloseTransmisson(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_TX_CMPLT);
	}
}

static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
    // Receive data in 8-bit mode
    if ((pSPIHandle->pSPIx->CR1 & (1 << DRV_SPI_CR1_DFF)))
    {
        // 16-bit reception
        *((uint16_t *)pSPIHandle->pRxBuffer) = (uint16_t)pSPIHandle->pSPIx->DR;
        pSPIHandle->RxLen -= 2;
        pSPIHandle->pRxBuffer += 2;
    }
    else
    {
        // 8-bit reception
        *pSPIHandle->pRxBuffer = (uint8_t)pSPIHandle->pSPIx->DR;
        pSPIHandle->RxLen--;
        pSPIHandle->pRxBuffer++;
    }

    // Check whether reception is complete
    if (pSPIHandle->RxLen == 0)
    {
        SPI_CloseReception(pSPIHandle);
        SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
    }
}


static void  spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle)
{

	uint8_t temp;
	//1. clear the ovr flag
	if(pSPIHandle->TxState != SPI_BUSY_IN_TX)
	{
		temp = pSPIHandle->pSPIx->DR;
		temp = pSPIHandle->pSPIx->SR;
	}
	(void)temp;
	//2. inform the application
	SPI_ApplicationEventCallback(pSPIHandle,SPI_EVENT_OVR_ERR);
}

void SPI_CloseTransmisson(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~( 1 << DRV_SPI_CR2_TXEIE);
	pSPIHandle->pTxBuffer = NULL;
	pSPIHandle->TxLen = 0;
	pSPIHandle->TxState = SPI_READY;
}

void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~( 1 << DRV_SPI_CR2_RXNEIE);
	pSPIHandle->pRxBuffer = NULL;
	pSPIHandle->RxLen = 0;
	pSPIHandle->RxState = SPI_READY;
}

void SPI_ClearOVRFlag(SPI1_RegDef_t *pSPIx)
{
	uint8_t temp;
	temp = pSPIx->DR;
	temp = pSPIx->SR;
	(void)temp;
}

__weak void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle,uint8_t AppEv)
{

	//This is a weak implementation . the user application may override this function.
}
