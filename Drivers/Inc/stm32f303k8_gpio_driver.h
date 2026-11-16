/*
 * stm32f303k8_gpio_driver.h
 *
 *  Created on: 16 Οκτ 2026
 *      Author: panek
 */

#ifndef INC_STM32F303K8_GPIO_DRIVER_H_
#define INC_STM32F303K8_GPIO_DRIVER_H_

#include "stm32f303k8.h"


/*
 * This is a Configuration structure for a GPIO pin
 */
typedef struct
{
    uint8_t GPIO_PinNumber;
    uint8_t GPIO_PinMode;			//!< possible modes from @GPIO_PIN_MODES >//
    uint8_t GPIO_PinSpeed;			/*!< possible values from @GPIO_PIN_SPEED >*/
    uint8_t GPIO_PinPuPdControl;
    uint8_t GPIO_PinOPType;
    uint8_t GPIO_PinAltFunMode;
} GPIO_PinConfig_t;

/*
 * This is a Handle structure for a GPIO pin
 */
typedef struct
{
    GPIO_RegDef_t *pGPIOx;             /*!< This holds the base address of the GPIO port to which the pin belongs >*/
    GPIO_PinConfig_t GPIO_PinConfig;   /*!< This holds GPIO pin configuration settings >*/
} GPIO_Handle_t;


/*
 * @GPIO_PIN_NUMBERS
 * GPIO pin numbers
 */
#define GPIO_PIN_NO_0  				0
#define GPIO_PIN_NO_1  				1
#define GPIO_PIN_NO_2  				2
#define GPIO_PIN_NO_3  				3
#define GPIO_PIN_NO_4  				4
#define GPIO_PIN_NO_5  				5
#define GPIO_PIN_NO_6  				6
#define GPIO_PIN_NO_7  				7
#define GPIO_PIN_NO_8  				8
#define GPIO_PIN_NO_9  				9
#define GPIO_PIN_NO_10  			10
#define GPIO_PIN_NO_11 				11
#define GPIO_PIN_NO_12  			12
#define GPIO_PIN_NO_13 				13
#define GPIO_PIN_NO_14 				14
#define GPIO_PIN_NO_15 				15

/*
 * GPIO_PIN_MODES
 * GPIO pin possible modes
 */
#define GPIO_MODE_IN 		0
#define GPIO_MODE_OUT 		1
#define GPIO_MODE_ALTFN		2
#define GPIO_MODE_ANALOG 	3
#define GPIO_MODE_IT_FT 	4
#define GPIO_MODE_IT_RT 	5
#define GPIO_MODE_IT_RFT 	6

// GPIO pin possible output types //
#define GPIO_OP_TYPE_PP		0
#define GPIO_OP_TYPE_OD		1

/*
 * @GPIO_PIN_SPEED
 * GPIO pin possible output speeds
 */
#define GPIO_SPEED_LOW		0
#define GPIO_SPEED_MEDIUM	1
#define GPIO_SPEED_FAST		2
#define GPIO_SPEED_HIGH		3

// GPIO pin pull up/ pull down configuration macros //
#define GPIO_NO_PUPD		0
#define GPIO_PIN_PU			1
#define GPIO_PIN_PD			2


/*****************************************************************************
 *                       APIs supported by this driver
 *****************************************************************************/

/*******************************************************************************
 * @fn               - GPIO_PeriClockControl
 * @brief            - This function enables or disables peripheral clock for the given GPIO port
 * @param[in]        - pGPIOx : base address of the gpio peripheral
 * @param[in]        - EnorDi : ENABLE or DISABLE macros
 * @return           - none
 * @Note             - none
 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi);


/*******************************************************************************
 * @fn               - GPIO_Init
 * @brief            - This function initializes the given GPIO port and pin configuration
 * @param[in]        - pGPIOHandle : pointer to the GPIO handle structure containing configuration settings
 * @return           - none
 * @Note             - Configures mode, speed, pull-up/pull-down, output type, and alternate functions based on the handle structure.
 */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle);


/*******************************************************************************
 * @fn               - GPIO_DeInit
 * @brief            - This function de-initializes the given GPIO port (resets it to default configuration)
 * @param[in]        - pGPIOx : base address of the gpio peripheral
 * @return           - none
 * @Note             - Uses the RCC peripheral reset register (AHB/APB reset) to force the GPIO port back to its power-on reset state.
 */
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx);


/*******************************************************************************
 * @fn               - GPIO_ReadFromInputPin
 * @brief            - This function reads data from a specific input pin of a GPIO port
 * @param[in]        - pGPIOx : base address of the gpio peripheral
 * @param[in]        - PinNumber : pin number from which data is to be read
 * @return           - 0 or 1 (input pin value)
 * @Note             - none
 */
uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber);


/*******************************************************************************
 * @fn               - GPIO_ReadFromInputPort
 * @brief            - This function reads data from an entire GPIO input port (16 pins)
 * @param[in]        - pGPIOx : base address of the gpio peripheral
 * @return           - 16-bit port input data value
 * @Note             - Reads directly from the Input Data Register (IDR) of the port in a single operation.
 */
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx);


/*******************************************************************************
 * @fn               - GPIO_WriteToOutputPin
 * @brief            - This function writes data (HIGH or LOW) to a specific output pin
 * @param[in]        - pGPIOx : base address of the gpio peripheral
 * @param[in]        - PinNumber : pin number to write to
 * @param[in]        - Value : output value (GPIO_PIN_SET or GPIO_PIN_RESET / 0 or 1)
 * @return           - none
 * @Note             - none
 */
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value);


/*******************************************************************************
 * @fn               - GPIO_WriteToOutputPort
 * @brief            - This function writes data to an entire GPIO output port (16 pins)
 * @param[in]        - pGPIOx : base address of the gpio peripheral
 * @param[in]        - Value : 16-bit value to be written to the port
 * @return           - none
 * @Note             - none
 */
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value);


/*******************************************************************************
 * @fn               - GPIO_ToggleOutputPin
 * @brief            - This function toggles the output state of a specific GPIO pin
 * @param[in]        - pGPIOx : base address of the gpio peripheral
 * @param[in]        - PinNumber : pin number to toggle
 * @return           - none
 * @Note             - none
 */
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber);

/******************************************************************************
 * @fn               - GPIO_IRQInterruptConfig
 * @brief            - This function enables or disables an interrupt in the
 *                     Nested Vectored Interrupt Controller (NVIC).
 * @param[in]        - IRQNumber : Interrupt Request number to configure.
 * @param[in]        - EnorDi : ENABLE or DISABLE macros.
 * @return           - none
 * @Note             - Enables or disables the specified IRQ in the NVIC.
 ******************************************************************************/
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);

/******************************************************************************
 * @fn               - GPIO_IRQPriorityConfig
 * @brief            - This function configures the priority of an interrupt
 *                     in the Nested Vectored Interrupt Controller (NVIC).
 * @param[in]        - IRQNumber : Interrupt Request number whose priority
 *                     will be configured.
 * @param[in]        - IRQPriority : Priority level to assign to the interrupt.
 * @return           - none
 * @Note             - Configures the priority of the specified IRQ in the NVIC.
 ******************************************************************************/
void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);

/******************************************************************************
 * @fn               - GPIO_IRQHandling
 * @brief            - This function handles an EXTI interrupt for a specific
 *                     GPIO pin.
 * @param[in]        - PinNumber : GPIO pin number associated with the interrupt.
 * @return           - none
 * @Note             - Clears the corresponding pending flag in the EXTI
 *                     Pending Register (PR) to prevent the interrupt from
 *                     remaining pending.
 ******************************************************************************/
void GPIO_IRQHandling(uint8_t PinNumber);

#endif /* INC_STM32F303K8_GPIO_DRIVER_H_ */
