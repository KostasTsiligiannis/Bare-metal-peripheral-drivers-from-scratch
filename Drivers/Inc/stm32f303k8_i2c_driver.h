/**
 ******************************************************************************
 * @file    stm32f303k8_i2c_driver.h
 * @author  panek
 * @brief   Bare-metal I2C peripheral driver header for STM32F303K8
 *          (Cortex-M4, RM0316 I2C v2 architecture).
 *
 * @details
 *  - Defines configuration and handle structures for synchronous (polling) and
 *    asynchronous (interrupt-driven) Master/Slave I2C transactions.
 *  - Exposes bus speed macros, repeated-START control options, application
 *    event/error notification codes, and peripheral/NVIC/IRQ API prototypes.
 ******************************************************************************
 */

#ifndef INC_STM32F303K8_I2C_DRIVER_H_
#define INC_STM32F303K8_I2C_DRIVER_H_

#include "stm32f303k8.h"

/* ============================================================================
 * I2C Configuration & Handle Structures
 * ============================================================================ */

/**
 * @brief  Configuration parameters for an STM32F303K8 I2C peripheral instance.
 */
typedef struct
{
    uint32_t I2C_SCLSpeed;      /*!< Target bus clock frequency in Hz (Standard or Fast mode) */
    uint8_t  I2C_DeviceAddress; /*!< 7-bit own slave address programmed into I2C_OAR1[7:1] */
} I2C_Config_t;

/**
 * @brief  Handle structure tracking hardware instance, configuration, active
 *         transfer buffers, byte counters, state machine, and bus options.
 */
typedef struct
{
    I2C_RegDef_t *pI2Cx;        /*!< Base address of target I2C peripheral (e.g., DRV_I2C1) */
    I2C_Config_t  I2C_Config;   /*!< Peripheral clock speed and own slave address settings */
    uint8_t      *pTxBuffer;    /*!< Pointer to active application transmit buffer */
    uint8_t      *pRxBuffer;    /*!< Pointer to active application receive buffer */
    uint32_t      TxLen;        /*!< Remaining bytes to transmit in current transfer */
    uint32_t      RxLen;        /*!< Remaining bytes to receive in current transfer */
    uint8_t       TxRxState;    /*!< Current driver communication state (@ref I2C_APP_STATES) */
    uint8_t       DevAddr;      /*!< Target 7-bit slave/device address for active transfer */
    uint32_t      RxSize;       /*!< Total requested receive byte count for current transaction */
    uint8_t       Sr;           /*!< Repeated-START control flag (I2C_ENABLE_SR / I2C_DISABLE_SR) */
} I2C_Handle_t;

/* ============================================================================
 * I2C Driver State, Event & Protocol Macros
 * ============================================================================ */

/** @defgroup I2C_APP_STATES I2C Application State Codes
 *  @{
 */
#define I2C_READY               0   /*!< Peripheral idle and ready for a new transaction */
#define I2C_BUSY_IN_RX          1   /*!< Non-blocking receive transfer in progress */
#define I2C_BUSY_IN_TX          2   /*!< Non-blocking transmit transfer in progress */
/** @} */

/** @defgroup I2C_APP_EVENTS I2C Application Event & Error Codes
 *  @{
 */
#define I2C_EV_TX_CMPLT         0   /*!< Master/slave transmit sequence completed */
#define I2C_EV_RX_CMPLT         1   /*!< Master/slave receive sequence completed */
#define I2C_EV_STOP             2   /*!< STOP condition detected on the I2C bus */
#define I2C_ERROR_BERR          3   /*!< Bus error (misplaced START/STOP condition) */
#define I2C_ERROR_ARLO          4   /*!< Arbitration lost error in multi-master operation */
#define I2C_ERROR_AF            5   /*!< Acknowledge failure (unexpected NACK received) */
#define I2C_ERROR_OVR           6   /*!< Overrun / underrun error (RXDR unread or TXDR empty) */
#define I2C_ERROR_TIMEOUT       7   /*!< SCL low / bus timeout condition */
#define I2C_EV_DATA_REQ         8   /*!< Slave mode: master requests a byte to transmit (TXIS) */
#define I2C_EV_DATA_RCV         9   /*!< Slave mode: data byte received from master (RXNE) */
/** @} */

/** @defgroup I2C_SCL_SPEEDS Supported I2C Bus Clock Speeds
 *  @{
 */
#define I2C_SCL_SPEED_SM        100000  /*!< Standard Mode: 100 kHz */
#define I2C_SCL_SPEED_FM4K      400000  /*!< Fast Mode: 400 kHz */
#define I2C_SCL_SPEED_FM2K      200000  /*!< Fast Mode sub-rate: 200 kHz */
/** @} */

/** @defgroup I2C_ACK_CONTROL Acknowledge Control Options
 *  @{
 */
#define I2C_ACK_ENABLE          1       /*!< Enable hardware ACK generation */
#define I2C_ACK_DISABLE         0       /*!< Disable hardware ACK generation (send NACK) */
/** @} */

/** @defgroup I2C_FM_DUTY Fast-Mode SCL Duty Cycle Options
 *  @{
 */
#define I2C_FM_DUTY_2           0       /*!< T_low / T_high = 2 duty cycle */
#define I2C_FM_DUTY_16_9        1       /*!< T_low / T_high = 16/9 duty cycle */
/** @} */

/** @defgroup I2C_SR_CONTROL Repeated-START Control Macros
 *  @{
 */
#define I2C_DISABLE_SR          RESET   /*!< Terminate transaction with STOP condition */
#define I2C_ENABLE_SR           SET     /*!< Hold bus with Repeated-START instead of STOP */
/** @} */

/************************************************************************************************
 *                         APIs supported by this driver
 ************************************************************************************************/

/***************************************************************************************
 * @fn               - I2C_PeriClockControl
 * @brief            - Enables or disables the APB peripheral clock for the specified
 *                     I2C peripheral.
 * @param[in]        - pI2Cx  : Base address of the I2C peripheral
 * @param[in]        - EnorDi : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Must be called before accessing I2C registers.
 ***************************************************************************************/
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

/***************************************************************************************
 * @fn               - I2C_Init
 * @brief            - Initializes I2C slave address (OAR1), bus timing (TIMINGR),
 *                     and peripheral enable state based on the handle configuration.
 * @param[in]        - pI2CHandle : Pointer to the I2C handle structure
 * @return           - None
 * @Note             - Configures STM32F3 I2C v2 registers according to I2C_Config.
 ***************************************************************************************/
void I2C_Init(I2C_Handle_t *pI2CHandle);

/***************************************************************************************
 * @fn               - I2C_DeInit
 * @brief            - Resets all registers of the specified I2C peripheral to
 *                     their default hardware reset values via RCC.
 * @param[in]        - pI2Cx : Base address of the I2C peripheral
 * @return           - None
 * @Note             - Pulses the corresponding peripheral reset bit in RCC APB1RSTR.
 ***************************************************************************************/
void I2C_DeInit(I2C_RegDef_t *pI2Cx);

/***************************************************************************************
 * @fn               - I2C_IRQInterruptConfig
 * @brief            - Enables or disables the specified I2C IRQ line in the NVIC.
 * @param[in]        - IRQNumber : NVIC interrupt request number
 * @param[in]        - EnorDi    : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Updates NVIC ISER/ICER interrupt enable registers.
 ***************************************************************************************/
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);

/***************************************************************************************
 * @fn               - I2C_IRQPriorityConfig
 * @brief            - Configures the priority level of the specified I2C IRQ in NVIC.
 * @param[in]        - IRQNumber   : NVIC interrupt request number
 * @param[in]        - IRQPriority : Priority level value
 * @return           - None
 * @Note             - Writes to the corresponding NVIC IPR priority register.
 ***************************************************************************************/
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);

/***************************************************************************************
 * @fn               - I2C_PeripheralControl
 * @brief            - Enables or disables the I2C peripheral engine (CR1.PE).
 * @param[in]        - pI2Cx  : Base address of the I2C peripheral
 * @param[in]        - EnorDi : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Sets or clears the PE bit in I2C_CR1.
 ***************************************************************************************/
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

/***************************************************************************************
 * @fn               - I2C_SSIConfig
 * @brief            - Configures internal slave select signal control if applicable.
 * @param[in]        - pI2Cx  : Base address of the peripheral
 * @param[in]        - EnOrDi : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Retained for driver API consistency across serial modules.
 ***************************************************************************************/
void I2C_SSIConfig(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);

/***************************************************************************************
 * @fn               - I2C_SSOEConfig
 * @brief            - Configures slave select output enable control if applicable.
 * @param[in]        - pI2Cx  : Base address of the peripheral
 * @param[in]        - EnOrDi : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Retained for driver API consistency across serial modules.
 ***************************************************************************************/
void I2C_SSOEConfig(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);

/***************************************************************************************
 * @fn               - I2C_GetFlagStatus
 * @brief            - Checks whether a specified status flag is set in I2C_ISR.
 * @param[in]        - pI2Cx    : Base address of the I2C peripheral
 * @param[in]        - FlagName : Shifted bit-mask of the status flag in I2C_ISR
 * @return           - uint8_t  : SET or RESET
 * @Note             - Performs a non-destructive check against I2C_ISR.
 ***************************************************************************************/
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName);

/***************************************************************************************
 * @fn               - I2C_ApplicationEventCallback
 * @brief            - Weakly defined application callback invoked on asynchronous
 *                     I2C events and bus error conditions.
 * @param[in]        - pI2CHandle : Pointer to the I2C handle instance
 * @param[in]        - AppEv      : Event or error identifier (@ref I2C_APP_EVENTS)
 * @return           - None
 * @Note             - Implement in user application code to process events/errors.
 ***************************************************************************************/
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv);

/***************************************************************************************
 * @fn               - I2C_MasterSendData
 * @brief            - Transmits a buffer of bytes to a slave in blocking mode.
 * @param[in]        - pI2CHandle : Pointer to the I2C handle instance
 * @param[in]        - pTxbuffer  : Pointer to transmit data buffer
 * @param[in]        - Len        : Number of bytes to send
 * @param[in]        - SlaveAddr  : 7-bit target slave address
 * @param[in]        - Sr         : Repeated-START option (I2C_ENABLE_SR / I2C_DISABLE_SR)
 * @return           - None
 * @Note             - Blocks until TXIS/TC or NACK/STOP conditions complete.
 ***************************************************************************************/
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/***************************************************************************************
 * @fn               - I2C_MasterReceiveData
 * @brief            - Receives a buffer of bytes from a slave in blocking mode.
 * @param[in]        - pI2CHandle : Pointer to the I2C handle instance
 * @param[out]       - pTxbuffer  : Pointer to destination receive buffer
 * @param[in]        - Len        : Number of bytes to receive
 * @param[in]        - SlaveAddr  : 7-bit target slave address
 * @param[in]        - Sr         : Repeated-START option (I2C_ENABLE_SR / I2C_DISABLE_SR)
 * @return           - None
 * @Note             - Blocks while polling RXNE and reading RXDR until Len bytes arrive.
 ***************************************************************************************/
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/***************************************************************************************
 * @fn               - I2C_MasterSendDataIT
 * @brief            - Initiates a non-blocking, interrupt-driven master transmission.
 * @param[in,out]    - pI2CHandle : Pointer to the I2C handle instance
 * @param[in]        - pTxbuffer  : Pointer to transmit data buffer
 * @param[in]        - Len        : Number of bytes to send
 * @param[in]        - SlaveAddr  : 7-bit target slave address
 * @param[in]        - Sr         : Repeated-START option (I2C_ENABLE_SR / I2C_DISABLE_SR)
 * @return           - uint8_t    : Previous TxRxState (I2C_READY or I2C_BUSY_IN_TX)
 * @Note             - Configures CR2 transfer length/reload and enables TX/event IRQs.
 ***************************************************************************************/
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/***************************************************************************************
 * @fn               - I2C_MasterReceiveDataIT
 * @brief            - Initiates a non-blocking, interrupt-driven master reception.
 * @param[in,out]    - pI2CHandle : Pointer to the I2C handle instance
 * @param[out]       - pRxbuffer  : Pointer to destination receive buffer
 * @param[in]        - Len        : Number of bytes to receive
 * @param[in]        - SlaveAddr  : 7-bit target slave address
 * @param[in]        - Sr         : Repeated-START option (I2C_ENABLE_SR / I2C_DISABLE_SR)
 * @return           - uint8_t    : Previous TxRxState (I2C_READY or I2C_BUSY_IN_RX)
 * @Note             - Configures CR2 transfer length/AUTOEND/RELOAD and enables RX IRQs.
 ***************************************************************************************/
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxbuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/***************************************************************************************
 * @fn               - I2C_CloseReceiveData
 * @brief            - Cleans up an active non-blocking receive transfer and resets
 *                     handle receive state to I2C_READY.
 * @param[in,out]    - pI2CHandle : Pointer to the I2C handle instance
 * @return           - None
 * @Note             - Disables RX interrupt sources and clears RX buffer/length fields.
 ***************************************************************************************/
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle);

/***************************************************************************************
 * @fn               - I2C_CloseSendData
 * @brief            - Cleans up an active non-blocking transmit transfer and resets
 *                     handle transmit state to I2C_READY.
 * @param[in,out]    - pI2CHandle : Pointer to the I2C handle instance
 * @return           - None
 * @Note             - Disables TX interrupt sources and clears TX buffer/length fields.
 ***************************************************************************************/
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle);

/***************************************************************************************
 * @fn               - I2C_SlaveSendData
 * @brief            - Loads a single byte into the I2C transmit data register (TXDR)
 *                     during slave read transfers.
 * @param[in]        - pI2Cx : Base address of the I2C peripheral
 * @param[in]        - data  : Byte to transmit to the I2C master
 * @return           - None
 * @Note             - Invoked in response to slave data request events (TXIS).
 ***************************************************************************************/
void I2C_SlaveSendData(I2C_RegDef_t *pI2Cx, uint8_t data);

/***************************************************************************************
 * @fn               - I2C_SlaveReceiveData
 * @brief            - Reads a single received byte from the I2C receive data
 *                     register (RXDR) during slave write transfers.
 * @param[in]        - pI2Cx : Base address of the I2C peripheral
 * @return           - uint8_t : Received byte from RXDR
 * @Note             - Reading RXDR clears the RXNE flag on STM32F3 I2C v2 hardware.
 ***************************************************************************************/
uint8_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2Cx);

/***************************************************************************************
 * @fn               - I2C_SlaveEnableDisableCallbackEvents
 * @brief            - Enables or disables slave event and error interrupt sources
 *                     (ADDR, RXIE, TXIE, STOPIE, NACKIE, ERRIE).
 * @param[in]        - pI2Cx  : Base address of the I2C peripheral
 * @param[in]        - EnorDi : ENABLE or DISABLE macro
 * @return           - None
 * @Note             - Required for interrupt-driven slave command/data handling.
 ***************************************************************************************/
void I2C_SlaveEnableDisableCallbackEvents(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

/***************************************************************************************
 * @fn               - I2C_EV_IRQHandling
 * @brief            - Handles I2C event interrupts (ADDR, TXIS, RXNE, TC/TCR, STOPF,
 *                     and NACKF) for both Master and Slave modes.
 * @param[in,out]    - pI2CHandle : Pointer to the I2C handle instance
 * @return           - None
 * @Note             - Clears sticky flags via I2C_ICR where required by I2C v2 HW.
 ***************************************************************************************/
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle);

/***************************************************************************************
 * @fn               - I2C_ER_IRQHandling
 * @brief            - Handles I2C error interrupts (BERR, ARLO, OVR, TIMEOUT) and
 *                     clears error flags before notifying the application.
 * @param[in,out]    - pI2CHandle : Pointer to the I2C handle instance
 * @return           - None
 * @Note             - Invokes I2C_ApplicationEventCallback() with I2C_ERROR_* codes.
 ***************************************************************************************/
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle);

#endif /* INC_STM32F303K8_I2C_DRIVER_H_ */
