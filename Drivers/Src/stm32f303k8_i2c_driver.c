#include "stm32f303k8_i2c_driver.h"

#define I2C_TIMINGR_100KHZ_8MHZ    0x10460F13

/* Prototypes στην κορυφή του stm32f303k8_i2c_driver.c */
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr);
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr);
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/* -------------------------------------------------------
 * I2C Peripheral Clock Control
 * ------------------------------------------------------- */
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
        /* Clock disable configuration can be implemented here if needed */
    }
}

void I2C_Init(I2C_Handle_t *pI2CHandle)
{
    uint32_t tempreg = 0;

    /* 1. Enable the peripheral clock */
    I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

    /* 2. Disable the I2C peripheral before configuration (PE = 0) */
    I2C_PeripheralControl(pI2CHandle->pI2Cx, DISABLE);

    /* 3. Configure noise filters in CR1 */
    tempreg = pI2CHandle->pI2Cx->CR1;

    // Clear and enable the Analog Noise Filter (ANFOFF = 0 means enabled)
    tempreg &= ~(DRV_I2C_CR1_ANFOFF);

    // Clear and configure the Digital Noise Filter (DNF = 0)
    tempreg &= ~(DRV_I2C_CR1_DNF_MASK);

    pI2CHandle->pI2Cx->CR1 = tempreg;

    /* 4. Configure I2C timing (TIMINGR) for HSI @ 8MHz */
    if (pI2CHandle->I2C_Config.I2C_SCLSpeed == I2C_SCL_SPEED_SM)
    {
        pI2CHandle->pI2Cx->TIMINGR = I2C_TIMINGR_100KHZ_8MHZ; // 0x10420F13
    }
    else if (pI2CHandle->I2C_Config.I2C_SCLSpeed == I2C_SCL_SPEED_FM4K)
    {
        // Default TIMINGR value for 400kHz (Fast Mode) based on 8MHz HSI
        pI2CHandle->pI2Cx->TIMINGR = 0x00100413;
    }

    /* 5. Configure own device address (OAR1) */
    tempreg = 0;
    // Shift the 7-bit address to bits [7:1] based on I2C_OAR1_OA1_POS
    tempreg |= (pI2CHandle->I2C_Config.I2C_DeviceAddress << 1);
    // Enable Own Address 1 (OA1EN = 1)
    tempreg |= DRV_I2C_OAR1_OA1EN;
    pI2CHandle->pI2Cx->OAR1 = tempreg;

    /* 6. Re-enable the I2C peripheral (PE = 1) - MANDATORY! */
    I2C_PeripheralControl(pI2CHandle->pI2Cx, ENABLE);
}

void I2C_DeInit(I2C_Handle_t *pI2CHandle)
{
    if (pI2CHandle->pI2Cx == DRV_I2C1)
    {
        DRV_I2C1_REG_RESET();
    }
}

/* -------------------------------------------------------
 * I2C Get Flag Status
 * ------------------------------------------------------- */
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)
{
    if (pI2Cx->ISR & FlagName)
    {
        return FLAG_SET;
    }

    return FLAG_RESET;
}

void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    // 1. Αναμονή αν ο δίαυλος είναι απασχολημένος (μόνο σε νέα συναλλαγή, όχι σε Repeated Start)
    // Σημείωση: Αν προηγήθηκε Sr, το BUSY παραμένει 1 επίτηδες, γι' αυτό ελέγχουμε μόνο όταν TC == 0
    if (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC))
    {
        while (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_BUSY));
    }

    // 2. Ρύθμιση διεύθυνσης Slave, πλήθους bytes, Write mode και AUTOEND
    I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr, Len, Sr);

    // 3. Δημιουργία συνθήκης START (ή Repeated START)
    pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_START;

    // 4. Αποστολή δεδομένων byte προς byte
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

    // 5. Αναμονή ολοκλήρωσης ανάλογα με το Sr
    if (Sr == I2C_DISABLE_SR)
    {
        // Το AUTOEND=1 παράγει αυτόματα STOP: περιμένουμε STOPF και το καθαρίζουμε
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_STOPF));
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_STOPCF;
    }
    else
    {
        // Το AUTOEND=0 κρατάει το bus ανοιχτό (TC=1) για το επόμενο Repeated Start
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC));
    }
}

void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
    // 1. Αναμονή αν ο δίαυλος είναι απασχολημένος (παραλείπεται αν ερχόμαστε από Repeated Start όπου TC=1)
    if (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC))
    {
        while (I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_BUSY));
    }

    // 2. Ρύθμιση διεύθυνσης Slave, πλήθους bytes, Read mode και AUTOEND
    I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr, Len, Sr);

    // 3. Δημιουργία συνθήκης START (ή Repeated START)
    pI2CHandle->pI2Cx->CR2 |= DRV_I2C_CR2_START;

    // 4. Ανάγνωση δεδομένων byte προς byte
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

    // 5. Αναμονή ολοκλήρωσης ανάλογα με το Sr
    if (Sr == I2C_DISABLE_SR)
    {
        // Το AUTOEND=1 παράγει αυτόματα NACK + STOP: περιμένουμε STOPF και το καθαρίζουμε
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_STOPF));
        pI2CHandle->pI2Cx->ICR = DRV_I2C_ICR_STOPCF;
    }
    else
    {
        // Το AUTOEND=0 κρατάει το bus ανοιχτό (TC=1) για το επόμενο Repeated Start
        while (!I2C_GetFlagStatus(pI2CHandle->pI2Cx, DRV_I2C_ISR_TC));
    }
}


static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr)
{
    uint32_t tmpreg = pI2Cx->CR2;

    // Clear previous SADD, NBYTES, RD_WRN, AUTOEND, START, STOP configurations
    tmpreg &= ~(DRV_I2C_CR2_SADD_MASK | DRV_I2C_CR2_NBYTES_MASK | DRV_I2C_CR2_RD_WRN |
                DRV_I2C_CR2_AUTOEND | DRV_I2C_CR2_START | DRV_I2C_CR2_STOP);

    // Prepare slave address (left-shift by 1)
    tmpreg |= ((uint32_t)(SlaveAddr << 1) & DRV_I2C_CR2_SADD_MASK);

    // Set byte count (Len)
    tmpreg |= ((Len & 0xFFU) << DRV_I2C_CR2_NBYTES_POS);

    // Set transfer direction: Write mode (RD_WRN = 0)
    tmpreg &= ~(DRV_I2C_CR2_RD_WRN);

    // Configure AUTOEND based on Repeated Start (Sr)
    if (Sr == I2C_DISABLE_SR)
    {
        tmpreg |= DRV_I2C_CR2_AUTOEND;  // Hardware generates STOP automatically after NBYTES
    }

    pI2Cx->CR2 = tmpreg;
}

static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint32_t Len, uint8_t Sr)
{
    uint32_t tmpreg = pI2Cx->CR2;

    // Clear previous SADD, NBYTES, RD_WRN, AUTOEND, START, STOP configurations
    tmpreg &= ~(DRV_I2C_CR2_SADD_MASK | DRV_I2C_CR2_NBYTES_MASK | DRV_I2C_CR2_RD_WRN |
                DRV_I2C_CR2_AUTOEND | DRV_I2C_CR2_START | DRV_I2C_CR2_STOP);

    // Prepare slave address (left-shift by 1)
    tmpreg |= ((uint32_t)(SlaveAddr << 1) & DRV_I2C_CR2_SADD_MASK);

    // Set byte count (Len)
    tmpreg |= ((Len & 0xFFU) << DRV_I2C_CR2_NBYTES_POS);

    // Set transfer direction: Read mode (RD_WRN = 1)
    tmpreg |= DRV_I2C_CR2_RD_WRN;

    // Configure AUTOEND based on Repeated Start (Sr)
    if (Sr == I2C_DISABLE_SR)
    {
        tmpreg |= DRV_I2C_CR2_AUTOEND;  // Hardware generates NACK + STOP automatically after NBYTES
    }

    pI2Cx->CR2 = tmpreg;
}


void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (IRQNumber <= 31)
        {
            // Program ISER0 register
            *DRV_NVIC_ISER0 |= (1 << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ISER1 register
            *DRV_NVIC_ISER1 |= (1 << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            // Program ISER2 register
            *DRV_NVIC_ISER2 |= (1 << (IRQNumber % 64));
        }
    }
    else
    {
        if (IRQNumber <= 31)
        {
            // Program ICER0 register
            *DRV_NVIC_ICER0 |= (1 << IRQNumber);
        }
        else if (IRQNumber > 31 && IRQNumber < 64)
        {
            // Program ICER1 register
            *DRV_NVIC_ICER1 |= (1 << (IRQNumber % 32));
        }
        else if (IRQNumber >= 64 && IRQNumber < 96)
        {
            // Program ICER2 register
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

    /* Clear the priority field before setting the new priority */
    *(DRV_NVIC_PR_BASE_ADDR + iprx) &= ~priority_mask;

    /* Set the requested priority */
    *(DRV_NVIC_PR_BASE_ADDR + iprx) |= ((IRQPriority & 0xF) << shift_amount);
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
