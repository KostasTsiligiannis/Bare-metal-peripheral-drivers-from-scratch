#include "stm32f303k8_i2c_driver.h"

#define I2C_TIMINGR_100KHZ_8MHZ    0x10460F13

/* Internal & public function prototypes */
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr);
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr);
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle);
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle);

void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/* ---------------------------------------------------------------------------
 * I2C Peripheral Clock Control
 * --------------------------------------------------------------------------- */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pI2Cx == DRV_I2C1)
        {
            DRV_I2C1_PCLK_EN();
        }
    }
    else
    {
    	DRV_I2C1_PCLK_DI();
    }
}

/* ---------------------------------------------------------------------------
 * I2C Initialization & De-initialization
 * --------------------------------------------------------------------------- */
void I2C_Init(I2C_Handle_t *pI2CHandle)
{
    uint32_t tempreg = 0;

    /* Enable peripheral clock before accessing I2C registers */
    I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

    /* Disable peripheral (PE = 0) prior to modifying timing and address registers */
    I2C_PeripheralControl(pI2CHandle->pI2Cx, DISABLE);

    /* Configure analog and digital noise filters in CR1 */
    tempreg = pI2CHandle->pI2Cx->CR1;
    tempreg &= ~(DRV_I2C_CR1_ANFOFF);     /* Enable analog noise filter (ANFOFF = 0) */
    tempreg &= ~(DRV_I2C_CR1_DNF_MASK);   /* Clear digital noise filter bits (DNF = 0) */
    pI2CHandle->pI2Cx->CR1 = tempreg;

    /* Configure SCL timing (TIMINGR) for 8 MHz HSI clock */
    if (pI2CHandle->I2C_Config.I2C_SCLSpeed == I2C_SCL_SPEED_SM)
    {
        pI2CHandle->pI2Cx->TIMINGR = I2C_TIMINGR_100KHZ_8MHZ;
    }
    else if (pI2CHandle->I2C_Config.I2C_SCLSpeed == I2C_SCL_SPEED_FM4K)
    {
        pI2CHandle->pI2Cx->TIMINGR = 0x00100413;
    }

    /* Configure own device address (7-bit addressing in OAR1[7:1]) */
    tempreg = 0;
    tempreg |= (pI2CHandle->I2C_Config.I2C_DeviceAddress << 1);
    tempreg |= DRV_I2C_OAR1_OA1EN;
    pI2CHandle->pI2Cx->OAR1 = tempreg;

    /* Re-enable peripheral (PE = 1) after configuration */
    I2C_PeripheralControl(pI2CHandle->pI2Cx, ENABLE);
}

void I2C_DeInit(I2C_RegDef_t *pI2Cx)
{
    if (pI2Cx == DRV_I2C1)
    {
        DRV_I2C1_REG_RESET();
    }
}
/* ---------------------------------------------------------------------------
 * Status & Control Helpers
 * --------------------------------------------------------------------------- */
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)
{
    if (pI2Cx->ISR & FlagName)
    {
        return FLAG_SET;
    }

    return FLAG_RESET;
}

void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        pI2Cx->CR1 |= DRV_I2C_CR1_PE;
    }
    else
    {
        pI2Cx->CR1 &= ~DRV_I2C_CR1_PE;
    }
}

/* ---------------------------------------------------------------------------
 * Blocking Master Transfers (Polling)
 * --------------------------------------------------------------------------- */
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    /* Wait until bus is free unless continuing from a Transfer Complete (Repeated Start) */
    if (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC))
    {
        while (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_BUSY));
    }

    /* Configure slave address, byte count, write direction, and AUTOEND behavior */
    I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr, Len, Sr);

    /* Trigger START (or Repeated START) condition */
    pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_START;

    /* Transmit payload byte-by-byte while monitoring NACK */
    while (Len > 0)
    {
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TXIS))
        {
            if (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_NACKF))
            {
                pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_NACKCF;
                pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_STOP;
                return;
            }
        }

        pI2CHandle->pI2Cx->TXDR = *pTxbuffer;
        pTxbuffer++;
        Len--;
    }

    /* Wait for automatic STOP (STOPF) or explicit Repeated Start ready state (TC) */
    if (Sr == I2C_DISABLE_SR)
    {
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_STOPF));
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_STOPCF;
    }
    else
    {
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC));
    }
}

void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    /* Wait until bus is free unless continuing from a Transfer Complete (Repeated Start) */
    if (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC))
    {
        while (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_BUSY));
    }

    /* Configure slave address, byte count, read direction, and AUTOEND behavior */
    I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr, Len, Sr);

    /* Trigger START (or Repeated START) condition */
    pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_START;

    /* Receive payload byte-by-byte while monitoring NACK */
    while (Len > 0)
    {
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_RXNE))
        {
            if (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_NACKF))
            {
                pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_NACKCF;
                pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_STOP;
                return;
            }
        }

        *pRxbuffer = (uint8_t)pI2CHandle->pI2Cx->RXDR;
        pRxbuffer++;
        Len--;
    }

    /* Wait for automatic STOP (STOPF) or explicit Repeated Start ready state (TC) */
    if (Sr == I2C_DISABLE_SR)
    {
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_STOPF));
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_STOPCF;
    }
    else
    {
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC));
    }
}

/* ---------------------------------------------------------------------------
 * CR2 Transfer Setup Helpers (STM32F3 I2C v2 Architecture)
 * --------------------------------------------------------------------------- */
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr)
{
    uint32_t tmpreg = pI2Cx->CR2;

    /* Clear previous transaction fields before programming a new transfer */
    tmpreg &= ~(DRV_I2C_CR2_SADD_MASK | DRV_I2C_CR2_NBYTES_MASK | DRV_I2C_CR2_RD_WRN | DRV_I2C_CR2_AUTOEND | DRV_I2C_CR2_START | DRV_I2C_CR2_STOP);

    /* Set 7-bit slave address in SADD[7:1] and select write mode (RD_WRN = 0) */
    tmpreg |= ((uint32_t)(SlaveAddr << 1) & DRV_I2C_CR2_SADD_MASK);
    tmpreg |= ((Len & 0xFFU) << DRV_I2C_CR2_NBYTES_POS);
    tmpreg &= ~(DRV_I2C_CR2_RD_WRN);

    /* Enable automatic STOP generation when no repeated start is requested */
    if (Sr == I2C_DISABLE_SR)
    {
        tmpreg |= DRV_I2C_CR2_AUTOEND;
    }

    pI2Cx->CR2 = tmpreg;
}

static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr)
{
    uint32_t tmpreg = pI2Cx->CR2;

    /* Clear previous transaction fields before programming a new transfer */
    tmpreg &= ~(DRV_I2C_CR2_SADD_MASK | DRV_I2C_CR2_NBYTES_MASK | DRV_I2C_CR2_RD_WRN | DRV_I2C_CR2_AUTOEND | DRV_I2C_CR2_START | DRV_I2C_CR2_STOP);

    /* Set 7-bit slave address in SADD[7:1] and select read mode (RD_WRN = 1) */
    tmpreg |= ((uint32_t)(SlaveAddr << 1) & DRV_I2C_CR2_SADD_MASK);
    tmpreg |= ((Len & 0xFFU) << DRV_I2C_CR2_NBYTES_POS);
    tmpreg |= DRV_I2C_CR2_RD_WRN;

    /* Enable automatic STOP generation when no repeated start is requested */
    if (Sr == I2C_DISABLE_SR)
    {
        tmpreg |= DRV_I2C_CR2_AUTOEND;
    }

    pI2Cx->CR2 = tmpreg;
}

/* ---------------------------------------------------------------------------
 * NVIC Interrupt Configuration
 * --------------------------------------------------------------------------- */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
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

void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
    uint8_t iprx = IRQNumber / 4;
    uint8_t iprx_section = IRQNumber % 4;
    uint8_t shift_amount = (8 * iprx_section) + (8 - DRV_NO_PR_BITS_IMPLEMENTED);
    uint32_t priority_mask = 0xF << shift_amount;

    *(DRV_NVIC_PR_BASE_ADDR + iprx) &= ~priority_mask;
    *(DRV_NVIC_PR_BASE_ADDR + iprx) |= ((IRQPriority & 0xF) << shift_amount);
}

/* ---------------------------------------------------------------------------
 * Non-Blocking Master Transfers (Interrupt Mode)
 * --------------------------------------------------------------------------- */
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    uint8_t busystate = pI2CHandle->TxRxState;

    if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
    {
        /* If not continuing from a Repeated Start (TC == 0), wait until bus is free */
        if (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC))
        {
            while (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_BUSY));
        }

        pI2CHandle->pTxBuffer = pTxBuffer;
        pI2CHandle->TxLen = Len;
        pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
        pI2CHandle->DevAddr = SlaveAddr;
        pI2CHandle->Sr = Sr;

        /* Program CR2 (SADD, NBYTES, RD_WRN=0, AUTOEND per Sr) before START */
        I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr, Len, Sr);

        /* Enable TX, TC, NACK, STOP, and Error interrupts in CR1 */
        pI2CHandle->pI2Cx->CR1 |= (DRV_I2C_CR1_TXIE | DRV_I2C_CR1_TCIE |
                                    DRV_I2C_CR1_NACKIE | DRV_I2C_CR1_STOPIE |
                                    DRV_I2C_CR1_ERRIE);

        /* Generate START (or Repeated START if TC was set) */
        pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_START;
    }

    return busystate;
}

uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    uint8_t busystate = pI2CHandle->TxRxState;

    if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
    {
        /* If not continuing from a Repeated Start (TC == 0), wait until bus is free */
        if (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC))
        {
            while (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_BUSY));
        }

        pI2CHandle->pRxBuffer = pRxBuffer;
        pI2CHandle->RxLen = Len;
        pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
        pI2CHandle->RxSize = Len;
        pI2CHandle->DevAddr = SlaveAddr;
        pI2CHandle->Sr = Sr;

        /* Program CR2 (SADD, NBYTES, RD_WRN=1, AUTOEND per Sr) before START */
        I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr, Len, Sr);

        /* Enable RX, TC, NACK, STOP, and Error interrupts in CR1 */
        pI2CHandle->pI2Cx->CR1 |= (DRV_I2C_CR1_RXIE | DRV_I2C_CR1_TCIE |
                                    DRV_I2C_CR1_NACKIE | DRV_I2C_CR1_STOPIE |
                                    DRV_I2C_CR1_ERRIE);

        /* Generate START (or Repeated START if TC was set) */
        pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_START;
    }

    return busystate;
}

/* ---------------------------------------------------------------------------
 * Transfer Cleanup Helpers
 * --------------------------------------------------------------------------- */
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
    /* Disable TX/event interrupt sources associated with transmission */
    pI2CHandle->pI2Cx->CR1 &= ~(DRV_I2C_CR1_TXIE | DRV_I2C_CR1_TCIE | DRV_I2C_CR1_NACKIE | DRV_I2C_CR1_STOPIE);

    /* Reset handle state and buffer pointers */
    pI2CHandle->TxRxState = I2C_READY;
    pI2CHandle->pTxBuffer = NULL;
    pI2CHandle->TxLen = 0;
}

void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
    /* Disable RX/event interrupt sources associated with reception */
    pI2CHandle->pI2Cx->CR1 &= ~(DRV_I2C_CR1_RXIE | DRV_I2C_CR1_TCIE | DRV_I2C_CR1_NACKIE | DRV_I2C_CR1_STOPIE);

    /* Reset handle state and buffer pointers */
    pI2CHandle->TxRxState = I2C_READY;
    pI2CHandle->pRxBuffer = NULL;
    pI2CHandle->RxLen = 0;
    pI2CHandle->RxSize = 0;
}

/* ---------------------------------------------------------------------------
 * Interrupt Service Logic (Events & Errors)
 * --------------------------------------------------------------------------- */
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    uint32_t cr1 = pI2CHandle->pI2Cx->CR1;
    uint32_t isr = pI2CHandle->pI2Cx->ISR;

    /* Handle TXIS: transmit data register ready for next byte (Master TX or Slave TX) */
	if ((cr1 & DRV_I2C_CR1_TXIE) && (isr & DRV_I2C_ISR_TXIS))
	{
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			if (pI2CHandle->TxLen > 0)
			{
				pI2CHandle->pI2Cx->TXDR = *(pI2CHandle->pTxBuffer);
				pI2CHandle->pTxBuffer++;
				pI2CHandle->TxLen--;
			}
		}
		else
		{
			/* Slave mode: master is requesting a byte from slave */
			I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_REQ);
		}
	}

	/* Handle RXNE: receive data register holds a new byte (Master RX or Slave RX) */
	if ((cr1 & DRV_I2C_CR1_RXIE) && (isr & DRV_I2C_ISR_RXNE))
	{
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			if (pI2CHandle->RxLen > 0)
			{
				*(pI2CHandle->pRxBuffer) = (uint8_t)pI2CHandle->pI2Cx->RXDR;
				pI2CHandle->pRxBuffer++;
				pI2CHandle->RxLen--;
			}
		}
		else
		{
			/* Slave mode: master sent a byte (e.g. command 0x51 / 0x52) to slave */
			I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_RCV);
		}
	}
    /* Handle NACKF: slave responded with NACK */
    if ((cr1 & DRV_I2C_CR1_NACKIE) && (isr & DRV_I2C_ISR_NACKF))
    {
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_NACKCF;

        if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
        {
            I2C_CloseSendData(pI2CHandle);
        }
        else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
        {
            I2C_CloseReceiveData(pI2CHandle);
        }

        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_AF);
    }

    /* Handle TC: NBYTES transfer completed (AUTOEND = 0, e.g. repeated start) */
    if ((cr1 & DRV_I2C_CR1_TCIE) && (isr & DRV_I2C_ISR_TC))
    {
        if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
        {
            I2C_CloseSendData(pI2CHandle);
            I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
        }
        else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
        {
            I2C_CloseReceiveData(pI2CHandle);
            I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_RX_CMPLT);
        }
    }

    /* Handle ADDR: address match with OAR1 in slave mode */
    if ((cr1 & DRV_I2C_CR1_ADDRIE) && (isr & DRV_I2C_ISR_ADDR))
    {
    	pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_ADDRCF;
    }

	/* Handle STOPF: STOP condition detected on the bus */
	if ((cr1 & DRV_I2C_CR1_STOPIE) && (isr & DRV_I2C_ISR_STOPF))
	{
		pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_STOPCF;

		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
			I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
		}
		else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
			I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_RX_CMPLT);
		}
		else
		{
			I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_STOP);
		}
	}
}

void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    uint32_t cr1 = pI2CHandle->pI2Cx->CR1;
    uint32_t isr = pI2CHandle->pI2Cx->ISR;

    if (!(cr1 & DRV_I2C_CR1_ERRIE))
    {
        return;
    }

    if (isr & DRV_I2C_ISR_BERR)
    {
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_BERRCF;
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_BERR);
    }

    if (isr & DRV_I2C_ISR_ARLO)
    {
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_ARLOCF;
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_ARLO);
    }

    if (isr & DRV_I2C_ISR_OVR)
    {
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_OVRCF;
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_OVR);
    }
}

void I2C_SlaveSendData(I2C_RegDef_t *pI2Cx, uint8_t data)
{
    pI2Cx->TXDR = (uint32_t)(data & DRV_I2C_TXDR_TXDATA_MASK);
}

uint8_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2Cx)
{
    return (uint8_t)(pI2Cx->RXDR & DRV_I2C_RXDR_RXDATA_MASK);
}

void I2C_SlaveEnableDisableCallbackEvents(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
    uint32_t slave_it_mask = (DRV_I2C_CR1_TXIE | DRV_I2C_CR1_RXIE | DRV_I2C_CR1_ADDRIE | DRV_I2C_CR1_NACKIE | DRV_I2C_CR1_STOPIE | DRV_I2C_CR1_ERRIE);

    if (EnorDi == ENABLE)
    {
        pI2Cx->CR1 |= slave_it_mask;
    }
    else
    {
        pI2Cx->CR1 &= ~slave_it_mask;
    }
}

