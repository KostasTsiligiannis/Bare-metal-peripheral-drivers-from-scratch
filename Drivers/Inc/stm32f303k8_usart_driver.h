/**
 ******************************************************************************
 * @file    stm32f303k8_usart_driver.h
 * @author  panek
 * @brief   Bare-metal USART/UART peripheral driver header for STM32F303K8
 *          (Cortex-M4, RM0316).
 *
 * @details
 *  - Provides configuration structures, peripheral handle definitions, and API
 *    prototypes for peripheral clock gating, baud/frame initialization,
 *    blocking (polling) and interrupt-driven (IT) full-duplex transfers,
 *    hardware status/ICR flag management, and NVIC interrupt control.
 ******************************************************************************
 */

#ifndef INC_STM32F303K8_USART_DRIVER_H_
#define INC_STM32F303K8_USART_DRIVER_H_

#include "stm32f303k8.h"

/* ============================================================================
 * USART Configuration & Handle Structures
 * ============================================================================ */

/**
 * @brief  Configuration parameters for an STM32F303K8 USART peripheral instance.
 */
typedef struct
{
    uint8_t  USART_Mode;             /*!< Operating direction mode (TX only, RX only, or TX/RX full-duplex) */
    uint32_t USART_Baud;             /*!< Target serial baud rate selection in bps (programmed into BRR) */
    uint8_t  USART_NoOfStopBits;     /*!< Number of stop bits (1, 0.5, 2, or 1.5 stop bits in CR2[13:12]) */
    uint8_t  USART_WordLength;       /*!< Data frame word length (7-bit, 8-bit, or 9-bit via CR1 M[1:0]) */
    uint8_t  USART_ParityControl;    /*!< Parity mode selection (Disable, Even parity, or Odd parity) */
    uint8_t  USART_HWFlowControl;    /*!< Hardware flow control mode (None, CTS, RTS, or CTS/RTS) */
} USART_Config_t;

/**
 * @brief  Handle structure encapsulating a USART peripheral instance and its
 *         runtime configuration state and transfer buffers.
 */
typedef struct
{
    USART_RegDef_t *pUSARTx;         /*!< Base address of target USART peripheral (USART1/USART2/USART3) */
    USART_Config_t  USART_Config;    /*!< Peripheral frame, baud rate, and flow-control settings */
    uint8_t        *pTxBuffer;       /*!< Pointer to application transmit data buffer */
    uint8_t        *pRxBuffer;       /*!< Pointer to application receive data buffer */
    uint32_t        TxLen;           /*!< Remaining transmit transfer length */
    uint32_t        RxLen;           /*!< Remaining receive transfer length */
    uint8_t         TxBusyState;     /*!< Active transmit state machine status (@ref USART_APP_STATES) */
    uint8_t         RxBusyState;     /*!< Active receive state machine status (@ref USART_APP_STATES) */
} USART_Handle_t;

/* ============================================================================
 * USART Driver Configuration & State Macros
 * ============================================================================ */

/** @defgroup USART_MODE Operating Direction Modes
 *  @{
 */
#define USART_MODE_ONLY_TX           0   /*!< Enable transmitter only (TE) */
#define USART_MODE_ONLY_RX           1   /*!< Enable receiver only (RE) */
#define USART_MODE_TXRX              2   /*!< Enable full-duplex transmitter and receiver (TE | RE) */
/** @} */

/** @defgroup USART_BAUD Standard Asynchronous Baud Rates (bps)
 *  @{
 */
#define USART_STD_BAUD_1200          1200    /*!< 1200 bps serial baud rate */
#define USART_STD_BAUD_2400          2400    /*!< 2400 bps serial baud rate */
#define USART_STD_BAUD_9600          9600    /*!< 9600 bps serial baud rate */
#define USART_STD_BAUD_19200         19200   /*!< 19200 bps serial baud rate */
#define USART_STD_BAUD_38400         38400   /*!< 38400 bps serial baud rate */
#define USART_STD_BAUD_57600         57600   /*!< 57600 bps serial baud rate */
#define USART_STD_BAUD_115200        115200  /*!< 115200 bps serial baud rate */
#define USART_STD_BAUD_230400        230400  /*!< 230400 bps serial baud rate */
#define USART_STD_BAUD_460800        460800  /*!< 460800 bps serial baud rate */
#define USART_STD_BAUD_921600        921600  /*!< 921600 bps serial baud rate */
/** @} */

/** @defgroup USART_PARITY Parity Control Modes
 *  @{
 */
#define USART_PARITY_DISABLE         0       /*!< Parity generation and checking disabled */
#define USART_PARITY_EN_EVEN         1       /*!< Even parity enabled (PCE = 1, PS = 0) */
#define USART_PARITY_EN_ODD          2       /*!< Odd parity enabled (PCE = 1, PS = 1) */
/** @} */

/** @defgroup USART_WORDLEN Frame Word Length Options
 *  @{
 */
#define USART_WORDLEN_8BITS          0       /*!< 8-bit data frame (M[1:0] = 00) */
#define USART_WORDLEN_9BITS          1       /*!< 9-bit data frame (M[1:0] = 01) */
#define USART_WORDLEN_7BITS          2       /*!< 7-bit data frame (M[1:0] = 10) */
/** @} */

/** @defgroup USART_STOPBITS Stop Bits Configuration
 *  @{
 */
#define USART_STOPBITS_1             0       /*!< 1 stop bit */
#define USART_STOPBITS_0_5           1       /*!< 0.5 stop bits */
#define USART_STOPBITS_2             2       /*!< 2 stop bits */
#define USART_STOPBITS_1_5           3       /*!< 1.5 stop bits */
/** @} */

/** @defgroup USART_HW_FLOW Hardware Flow Control Options
 *  @{
 */
#define USART_HW_FLOW_CTRL_NONE      0       /*!< Hardware flow control disabled */
#define USART_HW_FLOW_CTRL_CTS       1       /*!< CTS flow control enabled (CTSE) */
#define USART_HW_FLOW_CTRL_RTS       2       /*!< RTS flow control enabled (RTSE) */
#define USART_HW_FLOW_CTRL_CTS_RTS   3       /*!< Both CTS and RTS flow control enabled (CTSE | RTSE) */
/** @} */

/** @defgroup USART_APP_STATES USART Driver State Codes
 *  @{
 */
#define USART_READY                  0       /*!< Peripheral idle and ready for transfer */
#define USART_BUSY_IN_RX             1       /*!< Interrupt-driven reception in progress */
#define USART_BUSY_IN_TX             2       /*!< Interrupt-driven transmission in progress */

#define USART_EVENT_TX_CMPLT   0
#define	USART_EVENT_RX_CMPLT   1
#define	USART_EVENT_IDLE       2
#define	USART_EVENT_CTS        3
#define	USART_EVENT_PE         4
#define	USART_ERR_FE     	   5
#define	USART_ERR_NE    	   6
#define	USART_ERR_ORE    	   7

/************************************************************************************************
 *                         APIs supported by this driver
 ************************************************************************************************/

/************************************************************************************************
 * @fn               - USART_PeriClockControl
 * @brief            - Enables or disables the peripheral clock for the specified USART instance.
 * @param[in]        - pUSARTx : Base address of the USART peripheral (e.g., DRV_USART1)
 * @param[in]        - EnorDi  : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Must be invoked prior to accessing USART registers.
 ************************************************************************************************/
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi);

/************************************************************************************************
 * @fn               - USART_Init
 * @brief            - Initializes CR1, CR2, CR3, and baud rate configuration for the specified
 *                     USART handle.
 * @param[in]        - pUSARTHandle : Pointer to the USART handle containing base address and config
 * @return           - None
 * @Note             - Configure frame/control fields with peripheral disabled (UE = 0).
 ************************************************************************************************/
void USART_Init(USART_Handle_t *pUSARTHandle);

/************************************************************************************************
 * @fn               - USART_DeInit
 * @brief            - Resets the specified USART peripheral registers to their reset values.
 * @param[in]        - pUSARTx : Base address of the USART peripheral
 * @return           - None
 * @Note             - Uses the RCC peripheral reset macro for the target USART instance.
 ************************************************************************************************/
void USART_DeInit(USART_RegDef_t *pUSARTx);

/************************************************************************************************
 * @fn               - USART_SendData
 * @brief            - Transmits a buffer of data frames in blocking (polling) mode.
 * @param[in]        - pUSARTHandle : Pointer to the USART handle structure
 * @param[in]        - pTxBuffer    : Pointer to the transmit data buffer
 * @param[in]        - Len          : Number of frames/bytes to transmit
 * @return           - None
 * @Note             - Polls ISR.TXE before writing TDR and waits for ISR.TC at completion.
 ************************************************************************************************/
void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len);

/************************************************************************************************
 * @fn               - USART_ReceiveData
 * @brief            - Receives a buffer of data frames in blocking (polling) mode.
 * @param[in]        - pUSARTHandle : Pointer to the USART handle structure
 * @param[out]       - pRxBuffer    : Pointer to the destination receive data buffer
 * @param[in]        - Len          : Number of frames/bytes to receive
 * @return           - None
 * @Note             - Polls ISR.RXNE before reading RDR and masks data/parity bits as configured.
 ************************************************************************************************/
void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len);

/************************************************************************************************
 * @fn               - USART_IRQInterruptConfig
 * @brief            - Enables or disables an interrupt line in the Cortex-M4 NVIC.
 * @param[in]        - IRQNumber : NVIC interrupt request number
 * @param[in]        - EnorDi    : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Configures NVIC ISER/ICER registers.
 ************************************************************************************************/
void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);

/************************************************************************************************
 * @fn               - USART_IRQPriorityConfig
 * @brief            - Configures the priority of an interrupt line in the Cortex-M4 NVIC.
 * @param[in]        - IRQNumber   : NVIC interrupt request number
 * @param[in]        - IRQPriority : Priority level value
 * @return           - None
 * @Note             - Aligns priority according to implemented NVIC priority bits.
 ************************************************************************************************/
void USART_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);

/************************************************************************************************
 * @fn               - USART_IRQHandling
 * @brief            - Handles USART interrupt events for the specified USART handle.
 * @param[in,out]    - pHandle : Pointer to the USART handle structure
 * @return           - None
 * @Note             - Dispatches TXE, TC, RXNE, and error/idle events and invokes callback.
 ************************************************************************************************/
void USART_IRQHandling(USART_Handle_t *pHandle);

/************************************************************************************************
 * @fn               - USART_PeripheralControl
 * @brief            - Enables or disables the USART peripheral engine (CR1.UE).
 * @param[in]        - pUSARTx : Base address of the USART peripheral
 * @param[in]        - EnorDi  : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Sets or clears the UE bit in USART_CR1.
 ************************************************************************************************/
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi);

/************************************************************************************************
 * @fn               - USART_GetFlagStatus
 * @brief            - Returns the status of the requested flag in USART_ISR.
 * @param[in]        - pUSARTx  : Base address of the USART peripheral
 * @param[in]        - FlagName : Bit-mask of the status flag in USART_ISR
 * @return           - uint8_t  : FLAG_SET or FLAG_RESET
 * @Note             - Non-destructive read of USART_ISR.
 ************************************************************************************************/
uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint32_t FlagName);

/************************************************************************************************
 * @fn               - USART_ClearFlag
 * @brief            - Clears a status flag in USART_ICR (write-1-to-clear).
 * @param[in]        - pUSARTx        : Base address of the USART peripheral
 * @param[in]        - StatusFlagName : Bit-mask of the clear bit in USART_ICR
 * @return           - None
 * @Note             - Uses STM32F3 USART v2 ICR write-1-to-clear semantics.
 ************************************************************************************************/
void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint32_t StatusFlagName);

/************************************************************************************************
 * @fn               - USART_SendDataIT
 * @brief            - Initiates non-blocking (interrupt-driven) USART data transmission.
 * @param[in,out]    - pUSARTHandle : Pointer to the USART handle structure
 * @param[in]        - pTxBuffer    : Pointer to the transmit data buffer
 * @param[in]        - Len          : Number of frames/bytes to transmit
 * @return           - uint8_t      : Previous TX state (USART_READY or USART_BUSY_IN_TX)
 * @Note             - Stores transfer state in handle and enables TXEIE/TCIE interrupts.
 ************************************************************************************************/
uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len);

/************************************************************************************************
 * @fn               - USART_ReceiveDataIT
 * @brief            - Initiates non-blocking (interrupt-driven) USART data reception.
 * @param[in,out]    - pUSARTHandle : Pointer to the USART handle structure
 * @param[out]       - pRxBuffer    : Pointer to the receive data buffer
 * @param[in]        - Len          : Number of frames/bytes to receive
 * @return           - uint8_t      : Previous RX state (USART_READY or USART_BUSY_IN_RX)
 * @Note             - Stores transfer state in handle and enables RXNEIE interrupt.
 ************************************************************************************************/
uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len);

/* ============================================================================
 * Application Event Callback
 * ============================================================================ */

/************************************************************************************************
 * @fn               - USART_ApplicationEventCallback
 * @brief            - Weakly defined callback notifying the application of async USART events.
 * @param[in]        - pUSARTHandle : Pointer to the USART handle structure
 * @param[in]        - AppEv        : Asynchronous event or error code
 * @return           - None
 * @Note             - Override in application code to handle transfer events/errors.
 ************************************************************************************************/
void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle, uint8_t AppEv);

void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate);

void USART_IRQHandling(USART_Handle_t *pUSARTHandle);

#endif /* INC_STM32F303K8_USART_DRIVER_H_ */
