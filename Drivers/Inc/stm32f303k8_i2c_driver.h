
#ifndef INC_STM32F303K8_I2C_DRIVER_H_
#define INC_STM32F303K8_I2C_DRIVER_H_

#include "stm32f303k8.h"

/*
 * Configuration structures for I2Cx peripheral
 */
typedef struct
{
	uint32_t I2C_SCLSpeed;
	uint8_t  I2C_DeviceAddress;
}I2C_Config_t;

typedef struct
{
	I2C_RegDef_t 	*pI2Cx;
	I2C_Config_t 	I2C_Config;
}I2C_Handle_t;


/*
 * @I2C_SCLSpeed
 */
#define I2C_SCL_SPEED_SM 	100000
#define I2C_SCL_SPEED_FM4K 	400000
#define I2C_SCL_SPEED_FM2K  200000
/*
 * @I2C_AckControl
 */
#define I2C_ACK_ENABLE        1
#define I2C_ACK_DISABLE       0
/*
 * @I2C_FMDutyCycle
 */
#define I2C_FM_DUTY_2        0
#define I2C_FM_DUTY_16_9     1
/*
 * I2C related status flags definitions
 */
#define I2C_DISABLE_SR  	RESET
#define I2C_ENABLE_SR   	SET

/************************************************************************************************
 *                         APIs supported by this driver
 ************************************************************************************************/

/***************************************************************************************
 * @fn               - I2C_PeriClockControl
 *
 * @brief            - Enables or disables the peripheral clock for the specified
 *                     I2C peripheral.
 *
 * @param[in]        - pI2Cx: Base address of the I2C peripheral.
 * @param[in]        - EnorDi: ENABLE or DISABLE macros.
 *
 * @return           - None.
 *
 * @Note             - None.
 ***************************************************************************************/
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

/***************************************************************************************
 * @fn               - I2C_Init
 *
 * @brief            - Initializes the specified I2C peripheral.
 *
 * @param[in]        - pI2CHandle: Pointer to the I2C handle structure containing
 *                     the peripheral configuration settings.
 *
 * @return           - None.
 *
 * @Note             - Configures the I2C peripheral according to the settings
 *                     provided in the handle structure.
 ***************************************************************************************/
void I2C_Init(I2C_Handle_t *pI2CHandle);

/***************************************************************************************
 * @fn               - I2C_DeInit
 *
 * @brief            - De-initializes the specified I2C peripheral by resetting
 *                     its registers to their default values.
 *
 * @param[in]        - pI2Cx: Base address of the I2C peripheral.
 *
 * @return           - None.
 *
 * @Note             - Uses the RCC peripheral reset register to reset the
 *                     specified I2C peripheral.
 ***************************************************************************************/
void I2C_DeInit(I2C_Handle_t *pI2Cx);

/***************************************************************************************
 * @fn               - I2C_IRQInterruptConfig
 *
 * @brief            - Enables or disables the specified interrupt in the
 *                     Nested Vectored Interrupt Controller (NVIC).
 *
 * @param[in]        - IRQNumber: Interrupt request number to configure.
 * @param[in]        - EnorDi: ENABLE or DISABLE macros.
 *
 * @return           - None.
 *
 * @Note             - None.
 ***************************************************************************************/
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);

/***************************************************************************************
 * @fn               - I2C_IRQPriorityConfig
 *
 * @brief            - Configures the priority of the specified interrupt
 *                     in the Nested Vectored Interrupt Controller (NVIC).
 *
 * @param[in]        - IRQNumber: Interrupt request number whose priority
 *                     is to be configured.
 * @param[in]        - IRQPriority: Priority level assigned to the interrupt.
 *
 * @return           - None.
 *
 * @Note             - None.
 ***************************************************************************************/
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);
/***************************************************************************************
 * @fn               - I2C_PeripheralControl
 *
 * @brief            - Enables or disables the specified I2C peripheral.
 *
 * @param[in]        - pI2Cx: Base address of the I2C peripheral.
 * @param[in]        - EnorDi: ENABLE or DISABLE macros.
 *
 * @return           - None.
 *
 * @Note             - Sets or clears the PE (Peripheral Enable) bit in the
 *                     I2C_CR1 register.
 ***************************************************************************************/
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);
void I2C_SSIConfig(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);
void I2C_SSOEConfig(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);
/***************************************************************************************
 * @fn               - I2C_GetFlagStatus
 *
 * @brief            - Returns the status of the specified I2C status flag.
 *
 * @param[in]        - pI2Cx: Base address of the I2C peripheral.
 * @param[in]        - FlagName: Name or bit mask of the status flag to check.
 *
 * @return           - Flag status (SET or RESET).
 *
 * @Note             - Reads the corresponding bit in the I2C_ISR register.
 ***************************************************************************************/
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx , uint32_t FlagName);

/*
 * Application callback
 */
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle,uint8_t AppEv);
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

#endif /* INC_STM32F303K8_I2C_DRIVER_H_ */
