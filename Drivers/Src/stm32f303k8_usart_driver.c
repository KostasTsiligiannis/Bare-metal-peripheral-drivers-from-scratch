/*
 * stm32f303k8_usart_driver.c
 */
#include "stm32f303k8_usart_driver.h"

void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate)
{
    uint32_t PCLKx;
    uint32_t usartdiv;
    uint32_t tempreg = 0;

    /* Select the APB clock driving the target USART instance (STM32F3 RM0316) */
    if (pUSARTx == DRV_USART1)
    {
        /* USART1 is clocked from APB2 (PCLK2) */
        PCLKx = RCC_GetPCLK2Value();
    }
    else
    {
        /* USART2 / USART3 are clocked from APB1 (PCLK1) */
        PCLKx = RCC_GetPCLK1Value();
    }

    /* Calculate USART_BRR according to oversampling mode (OVER8) and rounding */
    if (pUSARTx->CR1 & DRV_USART_CR1_OVER8)
    {
        /*
         * OVER8 = 1 (8 samples per bit):
         * Higher divider resolution (2 * PCLKx) improves baud accuracy/max rate,
         * while BRR[15:4] holds usartdiv[15:4] and BRR[2:0] holds usartdiv[3:1]
         * (BRR[3] remains 0).
         */
        usartdiv = ((2 * PCLKx) + (BaudRate / 2)) / BaudRate;
        tempreg = (usartdiv & ~0x0F) | ((usartdiv & 0x0F) >> 1);
    }
    else
    {
        /*
         * OVER8 = 0 (16 samples per bit):
         * More samples per bit for noise immunity; BRR equals rounded PCLKx / BaudRate.
         */
        tempreg = (PCLKx + (BaudRate / 2)) / BaudRate;
    }

    /* Program the baud rate divider into USART_BRR (offset 0x0C) */
    pUSARTx->BRR = tempreg;
}
/* -------------------------------------------------------
 * USART Peripheral Clock Control
 * ------------------------------------------------------- */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pUSARTx == DRV_USART1)
        {
            DRV_USART1_PCLK_EN();
        }
    }
    else
    {
    	if (pUSARTx == DRV_USART1)
		{
			DRV_USART1_PCLK_DI();
		}
    }
}

void USART_Init(USART_Handle_t *pUSARTHandle)
{
	//Temporary variable
	uint32_t tempreg=0;

	//Enabling the Clock for given USART peripheral
    USART_PeriClockControl(pUSARTHandle->pUSARTx, ENABLE);

/******************************** Configuration of CR1******************************************/

	//Enabling USART Tx and Rx engines according to the USART_Mode configuration item
	if ( pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_RX)
	{
		//Enabling the Receiver bit field
		tempreg|= DRV_USART_CR1_RE;
	}else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_TX)
	{
		//Enabling the Transmitter bit field
		tempreg |= DRV_USART_CR1_TE;

	}else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_TXRX)
	{
		//Enabling the both Transmitter and Receiver bit fields
		tempreg |= ( DRV_USART_CR1_TE | DRV_USART_CR1_RE );
	}

	// Word length configuration (STM32F3 M[1:0]: M1=bit 28, M0=bit 12; 8-bit = 00)
	if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
	{
		tempreg |= DRV_USART_CR1_M0;
	}
	else if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_7BITS)
	{
		tempreg |= DRV_USART_CR1_M1;
	}
	// USART_WORDLEN_8BITS: M1 = 0, M0 = 0 (default)

    //Configuration of parity control bit fields
	if ( pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_EVEN)
	{
		//Enaling the parity control
		tempreg |= DRV_USART_CR1_PCE;

	}else if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_ODD )
	{
		//Enabling the parity control
	    tempreg |= DRV_USART_CR1_PCE;

	    //Enabling ODD parity
	    tempreg |= DRV_USART_CR1_PS;

	}
   //Programming the CR1 register
	pUSARTHandle->pUSARTx->CR1 = tempreg;

/******************************** Configuration of CR2******************************************/
	tempreg=0;

	//Configuration of number of stop bits inserted during USART frame transmission
	tempreg |= pUSARTHandle->USART_Config.USART_NoOfStopBits << DRV_USART_CR2_STOP_POS;

	//Programming the CR2 register
	pUSARTHandle->pUSARTx->CR2 = tempreg;

/******************************** Configuration of CR3******************************************/
	tempreg=0;

	//Configuration of USART hardware flow control
	if ( pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS)
	{
		//Enabling CTS flow control
		tempreg |= DRV_USART_CR3_CTSE;

	}else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_RTS)
	{
		//Enabling RTS flow control
		tempreg |= DRV_USART_CR3_RTSE;

	}else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS_RTS)
	{
		//Enabling both CTS and RTS Flow control
		tempreg |= (DRV_USART_CR3_CTSE | DRV_USART_CR3_RTSE);
	}

	pUSARTHandle->pUSARTx->CR3 = tempreg;

	//Configuration of baud rate
	USART_SetBaudRate(pUSARTHandle->pUSARTx, pUSARTHandle->USART_Config.USART_Baud);

}

void USART_DeInit(USART_RegDef_t *pUSARTx)
{
    if (pUSARTx == DRV_USART1)
    {
        DRV_USART1_REG_RESET();
    }
}

/* ---------------------------------------------------------------------------
 * Status & Control Helpers
 * --------------------------------------------------------------------------- */
uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint32_t FlagName)
{
    if (pUSARTx->ISR & FlagName)
    {
        return FLAG_SET;
    }

    return FLAG_RESET;
}

void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        pUSARTx->CR1 |= DRV_USART_CR1_UE;
        // Αναμονή μέχρι ο πομπός να είναι έτοιμος (STM32F3 ISR bit 21: TEACK)
        if (pUSARTx->CR1 & DRV_USART_CR1_TE)
        {
            while (!(pUSARTx->ISR & DRV_USART_ISR_TEACK));
        }
    }
    else
    {
        pUSARTx->CR1 &= ~DRV_USART_CR1_UE;
    }
}


/* ---------------------------------------------------------------------------
 * NVIC Interrupt Configuration
 * --------------------------------------------------------------------------- */
void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (IRQNumber <= 31)
        {
            *DRV_NVIC_ISER0 |= (1 << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            *DRV_NVIC_ISER1 |= (1 << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            *DRV_NVIC_ISER2 |= (1 << (IRQNumber % 64));
        }
    }
    else
    {
        if (IRQNumber <= 31)
        {
            *DRV_NVIC_ICER0 |= (1 << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            *DRV_NVIC_ICER1 |= (1 << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            *DRV_NVIC_ICER2 |= (1 << (IRQNumber % 64));
        }
    }
}

void USART_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
    uint8_t iprx = IRQNumber / 4;
    uint8_t iprx_section = IRQNumber % 4;
    uint8_t shift_amount = (8 * iprx_section) + (8 - DRV_NO_PR_BITS_IMPLEMENTED);
    uint32_t priority_mask = 0xF << shift_amount;

    *(DRV_NVIC_PR_BASE_ADDR + iprx) &= ~priority_mask;
    *(DRV_NVIC_PR_BASE_ADDR + iprx) |= ((IRQPriority & 0xF) << shift_amount);
}

void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len)
{
	uint16_t *pdata;
   //Loop over until "Len" number of bytes are transferred
	for(uint32_t i = 0 ; i < Len; i++)
	{
		//Wait until TXE flag is set in the SR
		while(! USART_GetFlagStatus(pUSARTHandle->pUSARTx,DRV_USART_ISR_TXE));

         //Check the USART_WordLength item for 9BIT or 8BIT in a frame
		if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
		{
			//if 9BIT, load the DR with 2bytes masking the bits other than first 9 bits
			pdata = (uint16_t*) pTxBuffer;
			pUSARTHandle->pUSARTx->TDR = (*pdata & (uint16_t)0x01FF);

			//check for USART_ParityControl
			if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
			{
				//No parity is used in this transfer. so, 9bits of user data will be sent
				//Implement the code to increment pTxBuffer twice
				pTxBuffer+=2;
			}
			else
			{
				//Parity bit is used in this transfer . so , 8bits of user data will be sent
				//The 9th bit will be replaced by parity bit by the hardware
				pTxBuffer++;
			}
		}
		else
		{
			//This is 8bit data transfer
			pUSARTHandle->pUSARTx->TDR = (*pTxBuffer  & (uint8_t)0xFF);

			//Implement the code to increment the buffer address
			pTxBuffer++;
		}
	}

	//Implement the code to wait till TC flag is set in the SR
	while( ! USART_GetFlagStatus(pUSARTHandle->pUSARTx,DRV_USART_ISR_TC));
}

void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    uint16_t rx_word;

    /* Loop over until "Len" frames are received */
    for (uint32_t i = 0; i < Len; i++)
    {
        /* Wait until RXNE flag is set in ISR */
        while (!USART_GetFlagStatus(pUSARTHandle->pUSARTx, DRV_USART_ISR_RXNE)){}

        /* Check word length configuration (9-bit vs 8-bit) */
        if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
        {
            if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                /* No parity: read 9 bits from RDR */
                rx_word = (uint16_t)(pUSARTHandle->pUSARTx->RDR & DRV_USART_RDR_RDR_MASK);
                pRxBuffer[0] = (uint8_t)(rx_word & 0xFF);
                pRxBuffer[1] = (uint8_t)((rx_word >> 8) & 0x01);
                pRxBuffer += 2;
            }
            else
            {
                /* Parity enabled: 8 bits of user data in RDR[7:0] */
                *pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->RDR & 0xFF);
                pRxBuffer++;
            }
        }
        else
        {
            /* 8-bit frame configuration */
            if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                /* No parity: all 8 bits are user data */
                *pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->RDR & 0xFF);
            }
            else
            {
                /* Parity enabled: mask out parity bit to keep 7 data bits */
                *pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->RDR & 0x7F);
            }

            pRxBuffer++;
        }
    }
}

uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer, uint32_t Len)
{
	uint8_t txstate = pUSARTHandle->TxBusyState;

	if(txstate != USART_BUSY_IN_TX)
	{
		pUSARTHandle->TxLen = Len;
		pUSARTHandle->pTxBuffer = pTxBuffer;
		pUSARTHandle->TxBusyState = USART_BUSY_IN_TX;

		//Implement the code to enable interrupt for TXE
		pUSARTHandle->pUSARTx-> CR1 |= DRV_USART_CR1_TXEIE;
		//Implement the code to enable interrupt for TC
		pUSARTHandle->pUSARTx-> CR1 |= DRV_USART_CR1_TCIE;
	}
	return txstate;
}

uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pRxBuffer, uint32_t Len)
{
	uint8_t rxstate = pUSARTHandle->RxBusyState;

	if(rxstate != USART_BUSY_IN_RX)
	{
		pUSARTHandle->RxLen = Len;
		pUSARTHandle->pRxBuffer = pRxBuffer;
		pUSARTHandle->RxBusyState = USART_BUSY_IN_RX;

		//Implement the code to enable interrupt for RXNE
		pUSARTHandle->pUSARTx-> CR1 |= DRV_USART_CR1_RXNEIE;
	}
	return rxstate;
}

void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint32_t StatusFlagName)
{
	pUSARTx->ICR = StatusFlagName;
}

__weak void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle,uint8_t event)
{
}

void USART_IRQHandling(USART_Handle_t *pUSARTHandle)
{
    uint32_t temp1, temp2;
    uint16_t *pdata;

    /* 1. Check for TC (Transmission Complete) flag */
    temp1 = pUSARTHandle->pUSARTx->ISR & DRV_USART_ISR_TC;
    temp2 = pUSARTHandle->pUSARTx->CR1 & DRV_USART_CR1_TCIE;

    if (temp1 && temp2)
    {
        if (pUSARTHandle->TxBusyState == USART_BUSY_IN_TX)
        {
            if (!pUSARTHandle->TxLen)
            {
                /* Clear TC flag via ICR and disable TC interrupt */
                pUSARTHandle->pUSARTx->ICR = DRV_USART_ICR_TCCF;
                pUSARTHandle->pUSARTx->CR1 &= ~DRV_USART_CR1_TCIE;

                pUSARTHandle->TxBusyState = USART_READY;
                pUSARTHandle->pTxBuffer = NULL;
                pUSARTHandle->TxLen = 0;

                USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_TX_CMPLT);
            }
        }
    }

    /* 2. Check for TXE (Transmit Data Register Empty) flag */
    temp1 = pUSARTHandle->pUSARTx->ISR & DRV_USART_ISR_TXE;
    temp2 = pUSARTHandle->pUSARTx->CR1 & DRV_USART_CR1_TXEIE;

    if (temp1 && temp2)
    {
        if (pUSARTHandle->TxBusyState == USART_BUSY_IN_TX)
        {
            if (pUSARTHandle->TxLen > 0)
            {
                if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
                {
                    pdata = (uint16_t *)pUSARTHandle->pTxBuffer;
                    pUSARTHandle->pUSARTx->TDR = (*pdata & 0x01FF);

                    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        pUSARTHandle->pTxBuffer += 2;
                        pUSARTHandle->TxLen -= 2;
                    }
                    else
                    {
                        pUSARTHandle->pTxBuffer++;
                        pUSARTHandle->TxLen--;
                    }
                }
                else
                {
                    pUSARTHandle->pUSARTx->TDR = (*pUSARTHandle->pTxBuffer & 0xFF);
                    pUSARTHandle->pTxBuffer++;
                    pUSARTHandle->TxLen--;
                }
            }

            if (pUSARTHandle->TxLen == 0)
            {
                /* Disable TXE interrupt once all bytes are loaded into TDR */
                pUSARTHandle->pUSARTx->CR1 &= ~DRV_USART_CR1_TXEIE;
            }
        }
    }

    /* 3. Check for RXNE (Read Data Register Not Empty) flag */
    temp1 = pUSARTHandle->pUSARTx->ISR & DRV_USART_ISR_RXNE;
    temp2 = pUSARTHandle->pUSARTx->CR1 & DRV_USART_CR1_RXNEIE;

    if (temp1 && temp2)
    {
        if (pUSARTHandle->RxBusyState == USART_BUSY_IN_RX)
        {
            if (pUSARTHandle->RxLen > 0)
            {
                if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
                {
                    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        *((uint16_t *)pUSARTHandle->pRxBuffer) = (uint16_t)(pUSARTHandle->pUSARTx->RDR & 0x01FF);
                        pUSARTHandle->pRxBuffer += 2;
                        pUSARTHandle->RxLen -= 2;
                    }
                    else
                    {
                        *pUSARTHandle->pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->RDR & 0xFF);
                        pUSARTHandle->pRxBuffer++;
                        pUSARTHandle->RxLen--;
                    }
                }
                else
                {
                    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        *pUSARTHandle->pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->RDR & 0xFF);
                    }
                    else
                    {
                        *pUSARTHandle->pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->RDR & 0x7F);
                    }

                    pUSARTHandle->pRxBuffer++;
                    pUSARTHandle->RxLen--;
                }
            }

            if (pUSARTHandle->RxLen == 0)
            {
                pUSARTHandle->pUSARTx->CR1 &= ~DRV_USART_CR1_RXNEIE;
                /* Clear any pending Overrun flag so it won't re-trigger the ISR */
                pUSARTHandle->pUSARTx->ICR = DRV_USART_ICR_ORECF;
                pUSARTHandle->RxBusyState = USART_READY;
                USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_RX_CMPLT);
            }

        }
    }

    /* 4. Check for IDLE flag */
    temp1 = pUSARTHandle->pUSARTx->ISR & DRV_USART_ISR_IDLE;
    temp2 = pUSARTHandle->pUSARTx->CR1 & DRV_USART_CR1_IDLEIE;

    if (temp1 && temp2)
    {
        pUSARTHandle->pUSARTx->ICR = DRV_USART_ICR_IDLECF;
        USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_IDLE);
    }

    /* 5. Check for Overrun error (ORE) */
    temp1 = pUSARTHandle->pUSARTx->ISR & DRV_USART_ISR_ORE;
    if (temp1)
    {
        /* Clear ORE flag via ICR and flush RDR so ISR does not loop forever */
        pUSARTHandle->pUSARTx->ICR = DRV_USART_ICR_ORECF;
        (void)pUSARTHandle->pUSARTx->RDR;
        USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_ORE);
    }


    /* 6. Check for Error flags (FE, NE, ORE) when EIE is enabled */
    temp2 = pUSARTHandle->pUSARTx->CR3 & DRV_USART_CR3_EIE;

    if (temp2)
    {
        temp1 = pUSARTHandle->pUSARTx->ISR;

        if (temp1 & DRV_USART_ISR_FE)
        {
            pUSARTHandle->pUSARTx->ICR = DRV_USART_ICR_FECF;
            USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_FE);
        }

        if (temp1 & DRV_USART_ISR_NF)
        {
            pUSARTHandle->pUSARTx->ICR = DRV_USART_ICR_NCF;
            USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_NE);
        }

        if (temp1 & DRV_USART_ISR_ORE)
        {
            pUSARTHandle->pUSARTx->ICR = DRV_USART_ICR_ORECF;
            USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_ORE);
        }
    }
}
