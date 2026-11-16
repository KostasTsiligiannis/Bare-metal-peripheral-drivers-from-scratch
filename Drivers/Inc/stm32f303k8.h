/*
 * stm32f303k8.h
 *
 *  Created on: 15 Οκτ 2026
 *      Author: panek
 */

#ifndef INC_STM32F303K8_H_
#define INC_STM32F303K8_H_

#include <stdint.h>
#include<stddef.h>

#define __vo volatile
#define __weak __attribute__((weak))

/**********************************START:Processor Specific Details **********************************/
/*
 * ARM Cortex Mx Processor NVIC ISERx register Addresses
 */
#define DRV_NVIC_ISER0          ( (__vo uint32_t*)0xE000E100 )
#define DRV_NVIC_ISER1          ( (__vo uint32_t*)0xE000E104 )
#define DRV_NVIC_ISER2          ( (__vo uint32_t*)0xE000E108 )
#define DRV_NVIC_ISER3          ( (__vo uint32_t*)0xE000E10c )

/*
 * ARM Cortex Mx Processor NVIC ICERx register Addresses
 */
#define DRV_NVIC_ICER0 			((__vo uint32_t*)0XE000E180)
#define DRV_NVIC_ICER1			((__vo uint32_t*)0XE000E184)
#define DRV_NVIC_ICER2  		((__vo uint32_t*)0XE000E188)
#define DRV_NVIC_ICER3			((__vo uint32_t*)0XE000E18C)


/*
 * macros for all the possible priority levels
 */
#define DRV_NVIC_IRQ_PRI0     0
#define DRV_NVIC_IRQ_PRI15    15

/*
 * ARM Cortex Mx Processor Priority Register Address Calculation
 */
#define DRV_NVIC_PR_BASE_ADDR 	((__vo uint32_t*)0xE000E400)
/*
 * ARM Cortex Mx Processor number of priority bits implemented in Priority Register
 */
#define DRV_NO_PR_BITS_IMPLEMENTED  4

// * base addresses of Flash and SRAM memories
#define DRV_FLASH_BASEADDR	0x08000000U  	// Flash memory //
#define DRV_SRAM_BASEADDR	0x20000000U		// One SRAM field //
#define DRV_ROM_BASEADDR	0x1FFFD800U		// System memory //

// AHBx & APBx Bus peripheral base addresses

#define DRV_PERIPH_BASEADDR		0x40000000U  	// Peripherals base address //
#define DRV_APB1PERIPH_BASEADDR	DRV_PERIPH_BASEADDR	// First peripheral register of APB1 is TIM2 //
#define DRV_APB2PERIPH_BASEADDR	0x40010000U		// APB2 //
#define DRV_AHB1PERIPH_BASEADDR	0x40020000U  	// AHB1 //
#define DRV_AHB2PERIPH_BASEADDR	0x48000000U		// AHB2 //

// Base address of RCC peripheral which is hanging on AHB1 Bus.//
#define DRV_RCC_BASEADDR	(DRV_AHB1PERIPH_BASEADDR + 0x1000)		// RCC -> 0x40021000 //


/* Base addresses of peripherals which are hanging on AHB2 Bus.
 Although the reference manual covers multiple STM32 device variants,
 the peripheral memory map for the specific MCU used in this project, STM32F303K8, refers that
 the memory region 0x48001000 - 0x480013FF is marked as "Reserved" rather than GPIOE.
 Therefore, GPIOE is not defined as an available GPIO peripheral
 */

#define DRV_GPIOA_BASEADDR	 DRV_AHB2PERIPH_BASEADDR				// GPIOA is the first peripheral register of AHB2//
#define DRV_GPIOB_BASEADDR	(DRV_AHB2PERIPH_BASEADDR + 0x0400)		// GPIOB //
#define DRV_GPIOC_BASEADDR	(DRV_AHB2PERIPH_BASEADDR + 0x0800)		// GPIOC //
#define DRV_GPIOD_BASEADDR	(DRV_AHB2PERIPH_BASEADDR + 0x0C00)		// GPIOD //
#define DRV_GPIOF_BASEADDR	(DRV_AHB2PERIPH_BASEADDR + 0x1400)		// GPIOF //

/* Base addresses of peripherals which are hanging on APB1 Bus.
 All other peripheral registers of APB1 is useless in this project
 */

#define DRV_USART2_BASEADDR	(DRV_APB1PERIPH_BASEADDR + 0x4400)		// USART2 1KB//
#define DRV_USART3_BASEADDR	(DRV_APB1PERIPH_BASEADDR + 0x4800)		// USART3 1KB //
#define DRV_I2C1_BASEADDR	(DRV_APB1PERIPH_BASEADDR + 0x5400)		// I2C1 1KB //

/* Base addresses of peripherals which are hanging on APB2 Bus.
 All other peripheral registers of APB2 is useless in this project
 */

#define DRV_SYSCFG_BASEADDR	 DRV_APB2PERIPH_BASEADDR				// SYSCFG 1KB//
#define DRV_EXTI_BASEADDR	(DRV_APB2PERIPH_BASEADDR + 0x0400)		// EXTI 1KB //
#define DRV_SPI1_BASEADDR	(DRV_APB2PERIPH_BASEADDR + 0x3000)		// SPI1 1KB //
#define DRV_USART1_BASEADDR	(DRV_APB2PERIPH_BASEADDR + 0x3800)		// USART1 1KB //

/*
 * @brief The volatile qualifier prevents the compiler from optimizing away
 *        necessary reads and writes to hardware registers whose values may
 *        change independently of the program.
 */

typedef struct
{
    __vo uint32_t MODER;    /*!< GPIO port mode register, configures the pin mode. 							Address offset: 0x00 */
    __vo uint32_t OTYPER;   /*!< GPIO port output type register, selects push-pull or open-drain. 			Address offset: 0x04 */
    __vo uint32_t OSPEEDR;  /*!< GPIO port output speed register, configures the output speed. 				Address offset: 0x08 */
    __vo uint32_t PUPDR;    /*!< GPIO port pull-up/pull-down register, configures internal pull resistors.  Address offset: 0x0C */
    __vo uint32_t IDR;      /*!< GPIO port input data register, reads the input state of pins. 				Address offset: 0x10 */
    __vo uint32_t ODR;      /*!< GPIO port output data register, sets or reads the output state of pins. 	Address offset: 0x14 */
    __vo uint32_t BSRR;     /*!< GPIO port bit set/reset register, sets or resets individual pins. 			Address offset: 0x18 */
    __vo uint32_t LCKR;     /*!< GPIO port configuration lock register, locks pin configuration. 			Address offset: 0x1C */
    __vo uint32_t AFR[2];   /*!< GPIO alternate function registers, AFR[1] : GPIO AFHR, AFR[0] : GPIO AFLR. Address offset: 0x20–0x24 */
} GPIO_RegDef_t;

#define DRV_GPIOA 		((GPIO_RegDef_t*)DRV_GPIOA_BASEADDR)
#define DRV_GPIOB 		((GPIO_RegDef_t*)DRV_GPIOB_BASEADDR)
#define DRV_GPIOC 		((GPIO_RegDef_t*)DRV_GPIOC_BASEADDR)
#define DRV_GPIOD 		((GPIO_RegDef_t*)DRV_GPIOD_BASEADDR)
#define DRV_GPIOF 		((GPIO_RegDef_t*)DRV_GPIOF_BASEADDR)

#define DRV_SPI1 		((SPI1_RegDef_t*)DRV_SPI1_BASEADDR)
#define DRV_I2C1        ((I2C_RegDef_t*)DRV_I2C1_BASEADDR)
/**
 * @brief Reset and Clock Control (RCC)
 */
typedef struct {
	__vo uint32_t CR;        /*!< Offset: 0x00 -  clock control register */
	__vo uint32_t CFGR;      /*!< Offset: 0x04 -  clock configuration register */
	__vo uint32_t CIR;       /*!< Offset: 0x08 -  clock interrupt register */
	__vo uint32_t APB2RSTR;  /*!< Offset: 0x0C -  APB2 peripheral reset register */
	__vo uint32_t APB1RSTR;  /*!< Offset: 0x10 -  APB1 peripheral reset register */
	__vo uint32_t AHBENR;    /*!< Offset: 0x14 -  AHB peripheral clock enable register */
	__vo uint32_t APB2ENR;   /*!< Offset: 0x18 -  APB2 peripheral clock enable register */
	__vo uint32_t APB1ENR;   /*!< Offset: 0x1C -  APB1 peripheral clock enable register */
	__vo uint32_t BDCR;      /*!< Offset: 0x20 -  RTC domain control register */
	__vo uint32_t CSR;       /*!< Offset: 0x24 -  control/status register */
	__vo uint32_t AHBRSTR;   /*!< Offset: 0x28 -  AHB peripheral reset register */
	__vo uint32_t CFGR2;     /*!< Offset: 0x2C -  clock configuration register 2 */
	__vo uint32_t CFGR3;     /*!< Offset: 0x30 -  clock configuration register 3 */
} RCC_RegDef_t;


/**
 * @brief External Interrupt/Event Controller (EXTI) register definition structure.
 */
typedef struct
{
    __vo uint32_t IMR;    /*!< Interrupt Mask Register. Offset: 0x00 */
    __vo uint32_t EMR;    /*!< Event Mask Register. Offset: 0x04 */
    __vo uint32_t RTSR;   /*!< Rising Trigger Selection Register. Offset: 0x08 */
    __vo uint32_t FTSR;   /*!< Falling Trigger Selection Register. Offset: 0x0C */
    __vo uint32_t SWIER;  /*!< Software Interrupt/Event Register. Offset: 0x10 */
    __vo uint32_t PR;     /*!< Pending Register. Offset: 0x14 */
} EXTI_RegDef_t;

/**
 * @brief SPI register definition structure.
 */
typedef struct
{
    __vo uint32_t CR1;       /*!< SPI Control Register 1. Offset: 0x00 */
    __vo uint32_t CR2;       /*!< SPI Control Register 2. Offset: 0x04 */
    __vo uint32_t SR;        /*!< SPI Status Register. Offset: 0x08 */
    __vo uint32_t DR;        /*!< SPI Data Register. Offset: 0x0C */
    __vo uint32_t CRCPR;     /*!< SPI CRC Polynomial Register. Offset: 0x10 */
    __vo uint32_t RXCRCR;    /*!< SPI RX CRC Register. Offset: 0x14 */
    __vo uint32_t TXCRCR;    /*!< SPI TX CRC Register. Offset: 0x18 */
    __vo uint32_t I2SCFGR;   /*!< SPI/I2S Configuration Register. Offset: 0x1C */
    __vo uint32_t I2SPR;     /*!< SPI/I2S Prescaler Register. Offset: 0x20 */
} SPI1_RegDef_t;

/**
 * @brief System Configuration Controller (SYSCFG) register definition structure.
 */
typedef struct
{
    __vo uint32_t CFGR1;      /*!< Configuration Register 1. Offset: 0x00 */
    __vo uint32_t RCR;        /*!< CCM SRAM protection/control register. Offset: 0x04 */
    __vo uint32_t EXTICR[4];  /*!< External Interrupt Configuration Registers. Offset: 0x08–0x14 */
    __vo uint32_t CFGR2;      /*!< Configuration Register 2. Offset: 0x18 */
} SYSCFG_RegDef_t;

/**
 * @brief SPI register definition structure.
 */
typedef struct
{
  __vo uint32_t CR1;        /*!< I2C Control Register 1,        Address offset: 0x00 */
  __vo uint32_t CR2;        /*!< I2C Control Register 2,        Address offset: 0x04 */
  __vo uint32_t OAR1;       /*!< I2C Own Address Register 1,    Address offset: 0x08 */
  __vo uint32_t OAR2;       /*!< I2C Own Address Register 2,    Address offset: 0x0C */
  __vo uint32_t TIMINGR;    /*!< I2C Timing Register,           Address offset: 0x10 */
  __vo uint32_t TIMEOUTR;   /*!< I2C Timeout Register,          Address offset: 0x14 */
  __vo uint32_t ISR;        /*!< I2C Interrupt and Status Reg,  Address offset: 0x18 */
  __vo uint32_t ICR;        /*!< I2C Interrupt Clear Register,  Address offset: 0x1C */
  __vo uint32_t PECR;       /*!< I2C PEC Register,              Address offset: 0x20 */
  __vo uint32_t RXDR;       /*!< I2C Receive Data Register,     Address offset: 0x24 */
  __vo uint32_t TXDR;       /*!< I2C Transmit Data Register,    Address offset: 0x28 */
} I2C_RegDef_t;

#define DRV_RCC ((RCC_RegDef_t*)DRV_RCC_BASEADDR)

#define DRV_EXTI  ((EXTI_RegDef_t*)DRV_EXTI_BASEADDR)

#define DRV_SYSCFG  ((SYSCFG_RegDef_t*)DRV_SYSCFG_BASEADDR)

/******** Clock enable macros for all peripherals *********/

// Clock enable macros for GPIOx peripherals //
#define DRV_GPIOA_PCLK_EN()	(DRV_RCC -> AHBENR |= (1 << 17))
#define DRV_GPIOB_PCLK_EN()	(DRV_RCC -> AHBENR |= (1 << 18))
#define DRV_GPIOC_PCLK_EN()	(DRV_RCC -> AHBENR |= (1 << 19))
#define DRV_GPIOD_PCLK_EN()	(DRV_RCC -> AHBENR |= (1 << 20))
#define DRV_GPIOF_PCLK_EN()	(DRV_RCC -> AHBENR |= (1 << 22))

// Clock enable macros for I2C peripheral //
#define DRV_I2C1_PCLK_EN()	(DRV_RCC -> APB1ENR |= (1 << 21))

// Clock enable macros for SPI peripheral //
#define DRV_SPI1_PCLK_EN()	(DRV_RCC -> APB2ENR |= (1 << 12))

 //Clock Enable Macros for I2Cx peripheral
#define I2C1_PCLK_EN() (RCC->APB1ENR |= (1 << 21))

// Clock enable macros for USARTx peripherals //
#define DRV_USART1_PCLK_EN()	(DRV_RCC -> APB2ENR |= (1 << 14))
#define DRV_USART2_PCLK_EN()	(DRV_RCC -> APB1ENR |= (1 << 17))
#define DRV_USART3_PCLK_EN()	(DRV_RCC -> APB1ENR |= (1 << 18))

// Clock enable macros for SYSCFG peripheral //
#define DRV_SYSCFG_PCLK_EN()	(DRV_RCC -> APB2ENR |= (1 << 0))

/*
 * This macro returns a code( between 0 to 7) for a given GPIO base address(x)
 */
#define DRV_GPIO_BASEADDR_TO_CODE(x)    ( (x == DRV_GPIOA)?0:\
										(x == DRV_GPIOB)?1:\
										(x == DRV_GPIOC)?2:\
										(x == DRV_GPIOD)?3:\
								        (x == DRV_GPIOF)?5:0)

// Interrupt Request Numbers of STM32f303k8 MCU
#define DRV_IRQ_NO_EXTI0		6
#define DRV_IRQ_NO_EXTI1		7
#define DRV_IRQ_NO_EXTI2		8
#define DRV_IRQ_NO_EXTI3		9
#define DRV_IRQ_NO_EXTI4		10
#define DRV_IRQ_NO_EXTI9_5		23
#define DRV_IRQ_NO_EXTI15_10	40
#define DRV_IRQ_NO_SPI1         35

// Generic macros
#define ENABLE				1
#define DISABLE				0
#define SET					ENABLE
#define RESET				DISABLE
#define GPIO_PIN_SET 		SET
#define GPIO_PIN_RESET 		RESET
#define FLAG_RESET          RESET
#define FLAG_SET 			SET

// Clock disable macros for GPIOx peripherals //
#define DRV_GPIOA_PCLK_DI()    (DRV_RCC->AHBENR &= ~(1 << 17))
#define DRV_GPIOB_PCLK_DI()    (DRV_RCC->AHBENR &= ~(1 << 18))
#define DRV_GPIOC_PCLK_DI()    (DRV_RCC->AHBENR &= ~(1 << 19))
#define DRV_GPIOD_PCLK_DI()    (DRV_RCC->AHBENR &= ~(1 << 20))
#define DRV_GPIOF_PCLK_DI()    (DRV_RCC->AHBENR &= ~(1 << 22))

//Macros to reset GPIOx peripherals
#define DRV_GPIOA_REG_RESET()	do{(DRV_RCC -> AHBRSTR |= (1 << 17)); (DRV_RCC -> AHBRSTR &= ~(1 << 17));} while(0)
#define DRV_GPIOB_REG_RESET()	do{(DRV_RCC -> AHBRSTR |= (1 << 18)); (DRV_RCC -> AHBRSTR &= ~(1 << 18));} while(0)
#define DRV_GPIOC_REG_RESET()	do{(DRV_RCC -> AHBRSTR |= (1 << 19)); (DRV_RCC -> AHBRSTR &= ~(1 << 19));} while(0)
#define DRV_GPIOD_REG_RESET()	do{(DRV_RCC -> AHBRSTR |= (1 << 20)); (DRV_RCC -> AHBRSTR &= ~(1 << 20));} while(0)
#define DRV_GPIOF_REG_RESET()	do{(DRV_RCC -> AHBRSTR |= (1 << 22)); (DRV_RCC -> AHBRSTR &= ~(1 << 22));} while(0)

//Macros to reset SPI1 peripheral
#define DRV_SPI1_REG_RESET() 	do{(DRV_RCC -> APB2RSTR |= (1 << 12)); (DRV_RCC -> APB2RSTR &= ~(1 << 12));} while(0)

//Macros to reset I2C1 peripheral
#define DRV_I2C1_REG_RESET() 	do{(DRV_RCC -> APB1RSTR |= (1 << 21)); (DRV_RCC -> APB2RSTR &= ~(1 << 21));} while(0)

/******************************************************************************************
 *Bit position definitions of SPI peripheral
 ******************************************************************************************/
/*
 * Bit position definitions SPI_CR1
 */
#define DRV_SPI_CR1_CPHA     				 0
#define DRV_SPI_CR1_CPOL      				 1
#define DRV_SPI_CR1_MSTR     				 2
#define DRV_SPI_CR1_BR   					 3
#define DRV_SPI_CR1_SPE     				 6
#define DRV_SPI_CR1_LSBFIRST   			 	 7
#define DRV_SPI_CR1_SSI     				 8
#define DRV_SPI_CR1_SSM      				 9
#define DRV_SPI_CR1_RXONLY      		 	10
#define DRV_SPI_CR1_DFF     			 	11
#define DRV_SPI_CR1_CRCNEXT   			 	12
#define DRV_SPI_CR1_CRCEN   			 	13
#define DRV_SPI_CR1_BIDIOE     			 	14
#define DRV_SPI_CR1_BIDIMODE      			15
/*
 * Bit position definitions SPI_CR2
 */
#define DRV_SPI_CR2_RXDMAEN		 			0
#define DRV_SPI_CR2_TXDMAEN				 	1
#define DRV_SPI_CR2_SSOE				 	2
#define DRV_SPI_CR2_FRF						4
#define DRV_SPI_CR2_ERRIE					5
#define DRV_SPI_CR2_RXNEIE				 	6
#define DRV_SPI_CR2_TXEIE					7
/*
 * Bit position definitions SPI_SR
 */
#define DRV_SPI_SR_RXNE						0
#define DRV_SPI_SR_TXE				 		1
#define DRV_SPI_SR_CHSIDE				 	2
#define DRV_SPI_SR_UDR					 	3
#define DRV_SPI_SR_CRCERR				 	4
#define DRV_SPI_SR_MODF					 	5
#define DRV_SPI_SR_OVR					 	6
#define DRV_SPI_SR_BSY					 	7
#define DRV_SPI_SR_FRE					 	8

/******************************************************************************************/
/*                         Bit Definitions for Advanced I2C                               */
/******************************************************************************************/

/*========================================================================================*/
/*                                  I2C_CR1 Register                                      */
/*========================================================================================*/
#define DRV_I2C_CR1_PE                 (1 << 0)       /* Peripheral Enable */
#define DRV_I2C_CR1_TXIE               (1 << 1)       /* TX Interrupt Enable */
#define DRV_I2C_CR1_RXIE               (1 << 2)       /* RX Interrupt Enable */
#define DRV_I2C_CR1_ADDRIE             (1 << 3)       /* Address Match Interrupt Enable */
#define DRV_I2C_CR1_NACKIE             (1 << 4)       /* NACK Interrupt Enable */
#define DRV_I2C_CR1_STOPIE             (1 << 5)       /* STOP Detection Interrupt Enable */
#define DRV_I2C_CR1_TCIE               (1 << 6)       /* Transfer Complete Interrupt Enable */
#define DRV_I2C_CR1_ERRIE              (1 << 7)       /* Error Interrupt Enable */
#define DRV_I2C_CR1_DNF_POS            8              /* Digital Noise Filter Position */
#define DRV_I2C_CR1_DNF_MASK           (0xF << 8)     /* Digital Noise Filter Mask */
#define DRV_I2C_CR1_ANFOFF             (1 << 12)      /* Analog Noise Filter OFF */
#define DRV_I2C_CR1_TXDMAEN            (1 << 14)      /* TX DMA Enable */
#define DRV_I2C_CR1_RXDMAEN            (1 << 15)      /* RX DMA Enable */
#define DRV_I2C_CR1_SBC                (1 << 16)      /* Slave Byte Control */
#define DRV_I2C_CR1_NOSTRETCH          (1 << 17)      /* Clock Stretching Disable */
#define DRV_I2C_CR1_WUPEN              (1 << 18)      /* Wakeup from Stop Mode Enable */
#define DRV_I2C_CR1_GCEN               (1 << 19)      /* General Call Enable */
#define DRV_I2C_CR1_SMBHEN             (1 << 20)      /* SMBus Host Address Enable */
#define DRV_I2C_CR1_SMBDEN             (1 << 21)      /* SMBus Device Default Address Enable */
#define DRV_I2C_CR1_ALERTEN            (1 << 22)      /* SMBus Alert Enable */
#define DRV_I2C_CR1_PECEN              (1 << 23)      /* PEC Enable */
/*========================================================================================*/
/*                                  I2C_CR2 Register                                      */
/*========================================================================================*/
#define DRV_I2C_CR2_SADD_POS           0              /* Slave Address Position */
#define DRV_I2C_CR2_SADD_MASK          (0x3FF << 0)   /* Slave Address Mask */
#define DRV_I2C_CR2_RD_WRN             (1 << 10)      /* Transfer Direction */
#define DRV_I2C_CR2_ADD10              (1 << 11)      /* 10-bit Addressing Mode */
#define DRV_I2C_CR2_HEAD10R            (1 << 12)      /* 10-bit Address Header Only Read */
#define DRV_I2C_CR2_START              (1 << 13)      /* Generate START Condition */
#define DRV_I2C_CR2_STOP               (1 << 14)      /* Generate STOP Condition */
#define DRV_I2C_CR2_NACK               (1 << 15)      /* Generate NACK */
#define DRV_I2C_CR2_NBYTES_POS         16             /* Number of Bytes Position */
#define DRV_I2C_CR2_NBYTES_MASK        (0xFF << 16)   /* Number of Bytes Mask */
#define DRV_I2C_CR2_RELOAD             (1 << 24)      /* NBYTES Reload Mode */
#define DRV_I2C_CR2_AUTOEND            (1 << 25)      /* Automatic End Mode */
#define DRV_I2C_CR2_PECBYTE            (1 << 26)      /* Packet Error Checking Byte */

/*========================================================================================*/
/*                                  I2C_OAR1 Register                                     */
/*========================================================================================*/
#define DRV_I2C_OAR1_OA1_POS           1              /* Own Address 1 Position */
#define DRV_I2C_OAR1_OA1_MASK          (0x3FF << 1)   /* Own Address 1 Mask */
#define DRV_I2C_OAR1_OA1MODE           (1 << 10)      /* Own Address 1 10-bit Mode */
#define DRV_I2C_OAR1_OA1EN             (1 << 15)      /* Own Address 1 Enable */
/*========================================================================================*/
/*                                  I2C_OAR2 Register                                     */
/*========================================================================================*/
#define DRV_I2C_OAR2_OA2_POS           1              /* Own Address 2 Position */
#define DRV_I2C_OAR2_OA2_MASK          (0x7F << 1)    /* Own Address 2 Mask */
#define DRV_I2C_OAR2_OA2MSK_POS        8              /* Own Address 2 Mask Position */
#define DRV_I2C_OAR2_OA2MSK_MASK       (0x7 << 8)     /* Own Address 2 Mask Bits */
#define DRV_I2C_OAR2_OA2EN             (1 << 15)      /* Own Address 2 Enable */
/*========================================================================================*/
/*                                  I2C_TIMINGR Register                                  */
/*========================================================================================*/
#define DRV_I2C_TIMINGR_SCLL_POS       0              /* SCL Low Period Position */
#define DRV_I2C_TIMINGR_SCLL_MASK      (0xFF << 0)    /* SCL Low Period Mask */
#define DRV_I2C_TIMINGR_SCLH_POS       8              /* SCL High Period Position */
#define DRV_I2C_TIMINGR_SCLH_MASK      (0xFF << 8)    /* SCL High Period Mask */
#define DRV_I2C_TIMINGR_SDADEL_POS     16             /* Data Hold Time Position */
#define DRV_I2C_TIMINGR_SDADEL_MASK    (0xF << 16)    /* Data Hold Time Mask */
#define DRV_I2C_TIMINGR_SCLDEL_POS     20             /* Data Setup Time Position */
#define DRV_I2C_TIMINGR_SCLDEL_MASK    (0xF << 20)    /* Data Setup Time Mask */
#define DRV_I2C_TIMINGR_PRESC_POS      28             /* Timing Prescaler Position */
#define DRV_I2C_TIMINGR_PRESC_MASK     (0xF << 28)    /* Timing Prescaler Mask */
/*========================================================================================*/
/*                                  I2C_TIMEOUTR Register                                 */
/*========================================================================================*/
#define DRV_I2C_TIMEOUTR_TIMEOUTA_POS  0              /* Timeout A Position */
#define DRV_I2C_TIMEOUTR_TIMEOUTA_MASK (0xFFF << 0)   /* Timeout A Mask */
#define DRV_I2C_TIMEOUTR_TIDLE         (1 << 12)      /* Idle Clock Timeout Detection */
#define DRV_I2C_TIMEOUTR_TIMOUTEN      (1 << 15)      /* Timeout A Enable */
#define DRV_I2C_TIMEOUTR_TIMEOUTB_POS  16             /* Timeout B Position */
#define DRV_I2C_TIMEOUTR_TIMEOUTB_MASK (0xFFF << 16)  /* Timeout B Mask */
#define DRV_I2C_TIMEOUTR_TEXTEN        (1 << 31)      /* Extended Clock Timeout Enable */
/*========================================================================================*/
/*                                  I2C_ISR Register                                      */
/*========================================================================================*/
#define DRV_I2C_ISR_TXE                (1 << 0)       /* Transmit Data Register Empty */
#define DRV_I2C_ISR_TXIS               (1 << 1)       /* Transmit Interrupt Status */
#define DRV_I2C_ISR_RXNE               (1 << 2)       /* Receive Data Register Not Empty */
#define DRV_I2C_ISR_ADDR               (1 << 3)       /* Address Matched */
#define DRV_I2C_ISR_NACKF              (1 << 4)       /* Not Acknowledge Received Flag */
#define DRV_I2C_ISR_STOPF              (1 << 5)       /* STOP Detection Flag */
#define DRV_I2C_ISR_TC                 (1 << 6)       /* Transfer Complete */
#define DRV_I2C_ISR_TCR                (1 << 7)       /* Transfer Complete Reload */
#define DRV_I2C_ISR_BERR               (1 << 8)       /* Bus Error */
#define DRV_I2C_ISR_ARLO               (1 << 9)       /* Arbitration Lost */
#define DRV_I2C_ISR_OVR                (1 << 10)      /* Overrun / Underrun */
#define DRV_I2C_ISR_PECERR             (1 << 11)      /* PEC Error */
#define DRV_I2C_ISR_TIMEOUT            (1 << 12)      /* Timeout Detection Flag */
#define DRV_I2C_ISR_ALERT              (1 << 13)      /* SMBus Alert */
#define DRV_I2C_ISR_BUSY               (1 << 15)      /* Bus Busy */
#define DRV_I2C_ISR_DIR                (1 << 16)      /* Transfer Direction */
#define DRV_I2C_ISR_ADDCODE_POS        17             /* Address Match Code Position */
#define DRV_I2C_ISR_ADDCODE_MASK       (0x7F << 17)   /* Address Match Code Mask */
/*========================================================================================*/
/*                                  I2C_ICR Register                                      */
/*========================================================================================*/
#define DRV_I2C_ICR_ADDRCF             (1 << 3)       /* Address Matched Flag Clear */
#define DRV_I2C_ICR_NACKCF             (1 << 4)       /* NACK Flag Clear */
#define DRV_I2C_ICR_STOPCF             (1 << 5)       /* STOP Detection Flag Clear */
#define DRV_I2C_ICR_BERRCF             (1 << 8)       /* Bus Error Flag Clear */
#define DRV_I2C_ICR_ARLOCF             (1 << 9)       /* Arbitration Lost Flag Clear */
#define DRV_I2C_ICR_OVRCF              (1 << 10)      /* Overrun / Underrun Flag Clear */
#define DRV_I2C_ICR_PECCF              (1 << 11)      /* PEC Error Flag Clear */
#define DRV_I2C_ICR_TIMOUTCF           (1 << 12)      /* Timeout Flag Clear */
#define DRV_I2C_ICR_ALERTCF            (1 << 13)      /* Alert Flag Clear */
/*========================================================================================*/
/*                                  I2C_PECR Register                                     */
/*========================================================================================*/
#define DRV_I2C_PECR_PEC_POS           0              /* Packet Error Code Position */
#define DRV_I2C_PECR_PEC_MASK          (0xFF << 0)    /* Packet Error Code Mask */
/*========================================================================================*/
/*                                  I2C_RXDR Register                                     */
/*========================================================================================*/
#define DRV_I2C_RXDR_RXDATA_POS        0              /* Receive Data Position */
#define DRV_I2C_RXDR_RXDATA_MASK       (0xFF << 0)    /* Receive Data Mask */
/*========================================================================================*/
/*                                  I2C_TXDR Register                                     */
/*========================================================================================*/
#define DRV_I2C_TXDR_TXDATA_POS        0              /* Transmit Data Position */
#define DRV_I2C_TXDR_TXDATA_MASK       (0xFF << 0)    /* Transmit Data Mask */

#include "stm32f303k8_gpio_driver.h"
#include "stm32f303k8_spi_driver.h"
#include "stm32f303k8_i2c_driver.h"

#endif /* INC_STM32F303K8_H_ */
