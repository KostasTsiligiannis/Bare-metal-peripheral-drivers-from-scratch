/*
 * stm32f303k8_gpio.c
 */


#include "stm32f303k8_gpio_driver.h"

void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi){
	// Check if clock enablement is requested (ENABLE)
	if(EnorDi == ENABLE){
		/* Compare the pGPIOx pointer with available ports to enable
		   the corresponding peripheral clock bit in the RCC AHBENR register */
		if(pGPIOx == DRV_GPIOA){
			DRV_GPIOA_PCLK_EN();
		} else if(pGPIOx == DRV_GPIOB){
			DRV_GPIOB_PCLK_EN();
		} else if(pGPIOx == DRV_GPIOC){
			DRV_GPIOC_PCLK_EN();
		} else if(pGPIOx == DRV_GPIOD){
			DRV_GPIOD_PCLK_EN();
		} else if(pGPIOx == DRV_GPIOF){
			DRV_GPIOF_PCLK_EN();
		}
	} else {
		/* Otherwise, if DISABLE is requested, execute the corresponding
		    clock clear macro */
		if(pGPIOx == DRV_GPIOA){
			DRV_GPIOA_PCLK_DI();
		} else if(pGPIOx == DRV_GPIOB){
			DRV_GPIOB_PCLK_DI();
		} else if(pGPIOx == DRV_GPIOC){
			DRV_GPIOC_PCLK_DI();
		} else if(pGPIOx == DRV_GPIOD){
			DRV_GPIOD_PCLK_DI();
		} else if(pGPIOx == DRV_GPIOF){
			DRV_GPIOF_PCLK_DI();
		}
	}
}

void GPIO_Init(GPIO_Handle_t *pGPIOHandle){

	uint32_t temp =0;
	//Enable the peripheral clock
	GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);

	if(pGPIOHandle -> GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_ANALOG){
		//non interrupt mode
		temp = (pGPIOHandle -> GPIO_PinConfig.GPIO_PinMode << (2 * pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber) );
		pGPIOHandle->pGPIOx->MODER &= ~( 0x3 << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber)); //clearing
		pGPIOHandle -> pGPIOx -> MODER |= temp; //setting

	} else {
		//interrupt mode
		if(pGPIOHandle -> GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_IT_FT){
			//configure the FTSR
			DRV_EXTI -> FTSR |= (1 << pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber);
			//clear the correspoding RTSR bit
			DRV_EXTI -> RTSR &= ~(1 << pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber);

		} else if(pGPIOHandle -> GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_IT_RT){
			//configure the RTSR
			DRV_EXTI -> RTSR |= (1 << pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber);
			//clear the correspoding FTSR bit
			DRV_EXTI -> FTSR &= ~(1 << pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber);

		} else if(pGPIOHandle -> GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_IT_RFT){

			//configure both the FTSR and RTSR
			DRV_EXTI -> RTSR |= (1 << pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber);
			DRV_EXTI -> FTSR |= (1 << pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber);
		}

		//configure the GPIO port selection in SYSCFG_EXTICR
			uint8_t temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 4 ;
			uint8_t temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 4;
			uint8_t portcode = DRV_GPIO_BASEADDR_TO_CODE(pGPIOHandle->pGPIOx);
			DRV_SYSCFG_PCLK_EN();
			DRV_SYSCFG->EXTICR[temp1] = portcode << ( temp2 * 4);

			//3 . enable the exti interrupt delivery using IMR
			DRV_EXTI->IMR |= 1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;
	}

	//configure the speed
	temp = 0;
	temp = (pGPIOHandle -> GPIO_PinConfig.GPIO_PinSpeed << (2* pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber) );
	pGPIOHandle->pGPIOx->OSPEEDR &= ~( 0x3 << ( 2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber)); //clearing
	pGPIOHandle -> pGPIOx -> OSPEEDR |= temp;

	//configure the pupd settings
	temp = 0;
	temp = (pGPIOHandle -> GPIO_PinConfig.GPIO_PinPuPdControl << (2* pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber) );
	pGPIOHandle->pGPIOx->PUPDR &= ~( 0x3 << ( 2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber)); //clearing
	pGPIOHandle -> pGPIOx -> PUPDR |= temp;

	//configure the otype
	temp = 0;
	temp = (pGPIOHandle -> GPIO_PinConfig.GPIO_PinOPType << pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber );
	pGPIOHandle->pGPIOx->OTYPER &= ~( 0x1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber); //clearing
	pGPIOHandle -> pGPIOx -> OTYPER |= temp;

	//configure the alt functionality
	temp = 0;
	if(pGPIOHandle -> GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_ALTFN){
		uint8_t temp1, temp2;
		temp1 = (pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber / 8);
		temp2 = (pGPIOHandle -> GPIO_PinConfig.GPIO_PinNumber % 8);
		pGPIOHandle -> pGPIOx -> AFR[temp1] &= ~(0xF << (4 * temp2)); //clearing
		pGPIOHandle -> pGPIOx -> AFR[temp1] |= (pGPIOHandle -> GPIO_PinConfig.GPIO_PinAltFunMode << (4 * temp2));
	}
}

void GPIO_DeInit(GPIO_RegDef_t *pGPIOx){
	/* Force reset the selected GPIO port back to its power-on reset state
	by utilizing the RCC Reset registers */
	if(pGPIOx == DRV_GPIOA){
			DRV_GPIOA_REG_RESET();
		} else if(pGPIOx == DRV_GPIOB){
			DRV_GPIOB_REG_RESET();
		} else if(pGPIOx == DRV_GPIOC){
			DRV_GPIOC_REG_RESET();
		} else if(pGPIOx == DRV_GPIOD){
			DRV_GPIOD_REG_RESET();
		} else if(pGPIOx == DRV_GPIOF){
			DRV_GPIOF_REG_RESET();
		}
}

uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber){

	uint8_t value;
	/* uint8_t is the minimum memory unit. Explicit (uint8_t) cast prevents
	   compiler truncation warnings when converting 32-bit IDR expression to 8-bit.
	   Shift IDR right by PinNumber, mask with 0x1, and return a clean 0 or 1. */
	value = (uint8_t) ((pGPIOx -> IDR >> PinNumber) & 0x00000001 );
	return value;

}

uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx){

	uint16_t value;
	value = (uint16_t) pGPIOx -> IDR ; // Read and return the entire 16-bit Input Data Register (IDR) of the port.
	return value;

}

void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value){

	if(Value == GPIO_PIN_SET){
		pGPIOx -> ODR |= (1 << PinNumber);
	} else {
		pGPIOx -> ODR &= ~(1 << PinNumber);
	}
}

void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value){

	pGPIOx -> ODR = Value;
}


void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber){

	pGPIOx -> ODR ^= (1 << PinNumber);
}

void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi){

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


void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
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


void GPIO_IRQHandling(uint8_t PinNumber){
	//clear the exti pr register corresponding to the pin number
	if(DRV_EXTI->PR & ( 1 << PinNumber))
	{
		// /* EXTI pending register is write-one-to-clear */
		DRV_EXTI->PR = ( 1 << PinNumber);
	}
}
