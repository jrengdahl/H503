/**

  This is a copy of the HAL SPI driver that inverts the data, for use with flash
  memories that erase to 1

  ******************************************************************************
  * @file    stm32h5xx_hal_spi.c
  * @author  MCD Application Team
  * @brief   SPI HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Serial Peripheral Interface (SPI) peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral Control functions
  *           + Peripheral State functions
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
    [..]
      The SPI HAL driver can be used as follows:

      (#) Declare a SPI_HandleTypeDef handle structure, for example:
          SPI_HandleTypeDef  hspi;

      (#)Initialize the SPI low level resources by implementing the HAL_SPI_MspInit() API:
          (##) Enable the SPIx interface clock
          (##) SPI pins configuration
              (+++) Enable the clock for the SPI GPIOs
              (+++) Configure these SPI pins as alternate function push-pull
          (##) NVIC configuration if you need to use interrupt process or DMA process
              (+++) Configure the SPIx interrupt priority
              (+++) Enable the NVIC SPI IRQ handle
          (##) DMA Configuration if you need to use DMA process
              (+++) Declare a DMA_HandleTypeDef handle structure for the transmit or receive Stream/Channel
              (+++) Enable the DMAx clock
              (+++) Configure the DMA handle parameters
              (+++) Configure the DMA Tx or Rx Stream/Channel
              (+++) Associate the initialized hdma_tx handle to the hspi DMA Tx or Rx handle
              (+++) Configure the priority and enable the NVIC for the transfer complete interrupt on the DMA Tx
                    or Rx Stream/Channel

      (#) Program the Mode, BidirectionalMode , Data size, Baudrate Prescaler, NSS
          management, Clock polarity and phase, FirstBit and CRC configuration in the hspi Init structure.

      (#) Initialize the SPI registers by calling the HAL_SPI_Init() API:
          (++) This API configures also the low level Hardware GPIO, CLOCK, CORTEX...etc)
              by calling the customized HAL_SPI_MspInit() API.
     [..]
       Callback registration:

      (#) The compilation flag USE_HAL_SPI_REGISTER_CALLBACKS when set to 1UL
          allows the user to configure dynamically the driver callbacks.
          Use Functions HAL_SPI_RegisterCallback() to register an interrupt callback.

          Function HAL_SPI_RegisterCallback() allows to register following callbacks:
            (+) TxCpltCallback        : SPI Tx Completed callback
            (+) RxCpltCallback        : SPI Rx Completed callback
            (+) TxRxCpltCallback      : SPI TxRx Completed callback
            (+) TxHalfCpltCallback    : SPI Tx Half Completed callback
            (+) RxHalfCpltCallback    : SPI Rx Half Completed callback
            (+) TxRxHalfCpltCallback  : SPI TxRx Half Completed callback
            (+) ErrorCallback         : SPI Error callback
            (+) AbortCpltCallback     : SPI Abort callback
            (+) SuspendCallback       : SPI Suspend callback
            (+) MspInitCallback       : SPI Msp Init callback
            (+) MspDeInitCallback     : SPI Msp DeInit callback
          This function takes as parameters the HAL peripheral handle, the Callback ID
          and a pointer to the user callback function.


      (#) Use function HAL_SPI_UnRegisterCallback to reset a callback to the default
          weak function.
          HAL_SPI_UnRegisterCallback takes as parameters the HAL peripheral handle,
          and the Callback ID.
          This function allows to reset following callbacks:
            (+) TxCpltCallback        : SPI Tx Completed callback
            (+) RxCpltCallback        : SPI Rx Completed callback
            (+) TxRxCpltCallback      : SPI TxRx Completed callback
            (+) TxHalfCpltCallback    : SPI Tx Half Completed callback
            (+) RxHalfCpltCallback    : SPI Rx Half Completed callback
            (+) TxRxHalfCpltCallback  : SPI TxRx Half Completed callback
            (+) ErrorCallback         : SPI Error callback
            (+) AbortCpltCallback     : SPI Abort callback
            (+) SuspendCallback       : SPI Suspend callback
            (+) MspInitCallback       : SPI Msp Init callback
            (+) MspDeInitCallback     : SPI Msp DeInit callback

       By default, after the HAL_SPI_Init() and when the state is HAL_SPI_STATE_RESET
       all callbacks are set to the corresponding weak functions:
       examples HAL_SPI_MasterTxCpltCallback(), HAL_SPI_MasterRxCpltCallback().
       Exception done for MspInit and MspDeInit functions that are
       reset to the legacy weak functions in the HAL_SPI_Init()/ HAL_SPI_DeInit() only when
       these callbacks are null (not registered beforehand).
       If MspInit or MspDeInit are not null, the HAL_SPI_Init()/ HAL_SPI_DeInit()
       keep and use the user MspInit/MspDeInit callbacks (registered beforehand) whatever the state.

       Callbacks can be registered/unregistered in HAL_SPI_STATE_READY state only.
       Exception done MspInit/MspDeInit functions that can be registered/unregistered
       in HAL_SPI_STATE_READY or HAL_SPI_STATE_RESET state,
       thus registered (user) MspInit/DeInit callbacks can be used during the Init/DeInit.
       Then, the user first registers the MspInit/MspDeInit user callbacks
       using HAL_SPI_RegisterCallback() before calling HAL_SPI_DeInit()
       or HAL_SPI_Init() function.

       When The compilation define USE_HAL_PPP_REGISTER_CALLBACKS is set to 0 or not defined,
       the callback registering feature is not available and weak callbacks are used.

       SuspendCallback restriction:
           SuspendCallback is called only when MasterReceiverAutoSusp is enabled and
       EOT interrupt is activated. SuspendCallback is used in relation with functions
       HAL_SPI_Transmit_IT, HAL_SPI_Receive_IT and HAL_SPI_TransmitReceive_IT.

    [..]
      Circular mode restriction:
      (+) The DMA circular mode cannot be used when the SPI is configured in these modes:
          (++) Master 2Lines RxOnly
          (++) Master 1Line Rx
      (+) The CRC feature is not managed when the DMA circular mode is enabled
      (+) The functions HAL_SPI_DMAPause()/ HAL_SPI_DMAResume() are not supported. Return always
          HAL_ERROR with ErrorCode set to HAL_SPI_ERROR_NOT_SUPPORTED.
          Those functions are maintained for backward compatibility reasons.

  @endverbatim
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

/** @addtogroup STM32H5xx_HAL_Driver
  * @{
  */

/** @defgroup SPI SPI
  * @brief SPI HAL module driver
  * @{
  */
#ifdef HAL_SPI_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @defgroup SPI_Private_Constants SPI Private Constants
  * @{
  */
#define SPI_DEFAULT_TIMEOUT 100UL
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup SPI_Private_Functions SPI Private Functions
  * @{
  */
static HAL_StatusTypeDef SPI_WaitOnFlagUntilTimeout(const SPI_HandleTypeDef *hspi, uint32_t Flag,
                                                    FlagStatus FlagStatus, uint32_t Timeout, uint32_t Tickstart);
static void SPI_CloseTransfer(SPI_HandleTypeDef *hspi);


/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup SPI_Exported_Functions SPI Exported Functions
  * @{
  */

/** @defgroup SPI_Exported_Functions_Group1 Initialization and de-initialization functions
  *  @brief    Initialization and Configuration functions
  *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]  This subsection provides a set of functions allowing to initialize and
          de-initialize the SPIx peripheral:

      (+) User must implement HAL_SPI_MspInit() function in which he configures
          all related peripherals resources (CLOCK, GPIO, DMA, IT and NVIC ).

      (+) Call the function HAL_SPI_Init() to configure the selected device with
          the selected configuration:
        (++) Mode
        (++) Direction
        (++) Data Size
        (++) Clock Polarity and Phase
        (++) NSS Management
        (++) BaudRate Prescaler
        (++) FirstBit
        (++) TIMode
        (++) CRC Calculation
        (++) CRC Polynomial if CRC enabled
        (++) CRC Length, used only with Data8 and Data16
        (++) FIFO reception threshold
        (++) FIFO transmission threshold

      (+) Call the function HAL_SPI_DeInit() to restore the default configuration
          of the selected SPIx peripheral.

@endverbatim
  * @{
  */


/** @defgroup SPI_Exported_Functions_Group2 IO operation functions
  *  @brief   Data transfers functions
  *
@verbatim
  ==============================================================================
                      ##### IO operation functions #####
 ===============================================================================
 [..]
    This subsection provides a set of functions allowing to manage the SPI
    data transfers.

    [..] The SPI supports master and slave mode :

    (#) There are two modes of transfer:
       (##) Blocking mode: The communication is performed in polling mode.
            The HAL status of all data processing is returned by the same function
            after finishing transfer.
       (##) No-Blocking mode: The communication is performed using Interrupts
            or DMA, These APIs return the HAL status.
            The end of the data processing will be indicated through the
            dedicated SPI IRQ when using Interrupt mode or the DMA IRQ when
            using DMA mode.
            The HAL_SPI_TxCpltCallback(), HAL_SPI_RxCpltCallback() and HAL_SPI_TxRxCpltCallback() user callbacks
            will be executed respectively at the end of the transmit or Receive process
            The HAL_SPI_ErrorCallback()user callback will be executed when a communication error is detected

    (#) APIs provided for these 2 transfer modes (Blocking mode or Non blocking mode using either Interrupt or DMA)
        exist for 1Line (simplex) and 2Lines (full duplex) modes.

@endverbatim
  * @{
  */

/**
  * @brief  Transmit an amount of data in blocking mode.
  * @param  hspi   : pointer to a SPI_HandleTypeDef structure that contains
  *                  the configuration information for SPI module.
  * @param  pData  : pointer to data buffer
  * @param  Size   : amount of data to be sent
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Transmit_Inv(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
#if defined (__GNUC__)
  __IO uint16_t *ptxdr_16bits = (__IO uint16_t *)(&(hspi->Instance->TXDR));
#endif /* __GNUC__ */

  uint32_t tickstart;

  /* Check Direction parameter */
  assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_TXONLY(hspi->Init.Direction));

  /* Check transfer size parameter */
  if (IS_SPI_LIMITED_INSTANCE(hspi->Instance))
  {
    assert_param(IS_SPI_LIMITED_TRANSFER_SIZE(Size));
  }
  else
  {
    assert_param(IS_SPI_TRANSFER_SIZE(Size));
  }

  /* Init tickstart for timeout management*/
  tickstart = HAL_GetTick();

  if (hspi->State != HAL_SPI_STATE_READY)
  {
    return HAL_BUSY;
  }

  if ((pData == NULL) || (Size == 0UL))
  {
    return HAL_ERROR;
  }

  /* Lock the process */
  __HAL_LOCK(hspi);

  /* Set the transaction information */
  hspi->State       = HAL_SPI_STATE_BUSY_TX;
  hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  hspi->pTxBuffPtr  = (const uint8_t *)pData;
  hspi->TxXferSize  = Size;
  hspi->TxXferCount = Size;

  /*Init field not used in handle to zero */
  hspi->pRxBuffPtr  = NULL;
  hspi->RxXferSize  = (uint16_t) 0UL;
  hspi->RxXferCount = (uint16_t) 0UL;
  hspi->TxISR       = NULL;
  hspi->RxISR       = NULL;

  /* Configure communication direction : 1Line */
  if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
  {
    SPI_1LINE_TX(hspi);
  }
  else
  {
    SPI_2LINES_TX(hspi);
  }

  /* Set the number of data at current transfer */
  MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, Size);

  /* Enable SPI peripheral */
  __HAL_SPI_ENABLE(hspi);

  if (hspi->Init.Mode == SPI_MODE_MASTER)
  {
    /* Master transfer start */
    SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
  }

  /* Transmit data in 32 Bit mode */
  if ((hspi->Init.DataSize > SPI_DATASIZE_16BIT) && (IS_SPI_FULL_INSTANCE(hspi->Instance)))
  {
    /* Transmit data in 32 Bit mode */
    while (hspi->TxXferCount > 0UL)
    {
      /* Wait until TXP flag is set to send data */
      if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP))
      {
        *((__IO uint32_t *)&hspi->Instance->TXDR) = ~*((const uint32_t *)hspi->pTxBuffPtr);
        hspi->pTxBuffPtr += sizeof(uint32_t);
        hspi->TxXferCount--;
      }
      else
      {
        /* Timeout management */
        if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
        {
          /* Call standard close procedure with error check */
          SPI_CloseTransfer(hspi);

          SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
          hspi->State = HAL_SPI_STATE_READY;

          /* Unlock the process */
          __HAL_UNLOCK(hspi);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  /* Transmit data in 16 Bit mode */
  else if (hspi->Init.DataSize > SPI_DATASIZE_8BIT)
  {
    /* Transmit data in 16 Bit mode */
    while (hspi->TxXferCount > 0UL)
    {
      /* Wait until TXP flag is set to send data */
      if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP))
      {
        if ((hspi->TxXferCount > 1UL) && (hspi->Init.FifoThreshold > SPI_FIFO_THRESHOLD_01DATA))
        {
          *((__IO uint32_t *)&hspi->Instance->TXDR) = ~*((const uint32_t *)hspi->pTxBuffPtr);
          hspi->pTxBuffPtr += sizeof(uint32_t);
          hspi->TxXferCount -= (uint16_t)2UL;
        }
        else
        {
#if defined (__GNUC__)
          *ptxdr_16bits = ~*((const uint16_t *)hspi->pTxBuffPtr);
#else
          *((__IO uint16_t *)&hspi->Instance->TXDR) = ~*((const uint16_t *)hspi->pTxBuffPtr);
#endif /* __GNUC__ */
          hspi->pTxBuffPtr += sizeof(uint16_t);
          hspi->TxXferCount--;
        }
      }
      else
      {
        /* Timeout management */
        if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
        {
          /* Call standard close procedure with error check */
          SPI_CloseTransfer(hspi);

          SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
          hspi->State = HAL_SPI_STATE_READY;

          /* Unlock the process */
          __HAL_UNLOCK(hspi);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  /* Transmit data in 8 Bit mode */
  else
  {
    while (hspi->TxXferCount > 0UL)
    {
      /* Wait until TXP flag is set to send data */
      if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP))
      {
        if ((hspi->TxXferCount > 3UL) && (hspi->Init.FifoThreshold > SPI_FIFO_THRESHOLD_03DATA))
        {
          *((__IO uint32_t *)&hspi->Instance->TXDR) = ~*((const uint32_t *)hspi->pTxBuffPtr);
          hspi->pTxBuffPtr += sizeof(uint32_t);
          hspi->TxXferCount -= (uint16_t)4UL;
        }
        else if ((hspi->TxXferCount > 1UL) && (hspi->Init.FifoThreshold > SPI_FIFO_THRESHOLD_01DATA))
        {
#if defined (__GNUC__)
          *ptxdr_16bits = ~*((const uint16_t *)hspi->pTxBuffPtr);
#else
          *((__IO uint16_t *)&hspi->Instance->TXDR) = ~*((const uint16_t *)hspi->pTxBuffPtr);
#endif /* __GNUC__ */
          hspi->pTxBuffPtr += sizeof(uint16_t);
          hspi->TxXferCount -= (uint16_t)2UL;
        }
        else
        {
          *((__IO uint8_t *)&hspi->Instance->TXDR) = ~*((const uint8_t *)hspi->pTxBuffPtr);
          hspi->pTxBuffPtr += sizeof(uint8_t);
          hspi->TxXferCount--;
        }
      }
      else
      {
        /* Timeout management */
        if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
        {
          /* Call standard close procedure with error check */
          SPI_CloseTransfer(hspi);

          SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
          hspi->State = HAL_SPI_STATE_READY;

          /* Unlock the process */
          __HAL_UNLOCK(hspi);

          return HAL_TIMEOUT;
        }
      }
    }
  }

  /* Wait for Tx (and CRC) data to be sent */
  if (SPI_WaitOnFlagUntilTimeout(hspi, SPI_FLAG_EOT, RESET, Timeout, tickstart) != HAL_OK)
  {
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
  }

  /* Call standard close procedure with error check */
  SPI_CloseTransfer(hspi);

  hspi->State = HAL_SPI_STATE_READY;

  /* Unlock the process */
  __HAL_UNLOCK(hspi);

  if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
  {
    return HAL_ERROR;
  }
  else
  {
    return HAL_OK;
  }
}

/**
  * @brief  Receive an amount of data in blocking mode.
  * @param  hspi   : pointer to a SPI_HandleTypeDef structure that contains
  *                  the configuration information for SPI module.
  * @param  pData  : pointer to data buffer
  * @param  Size   : amount of data to be received
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Receive_Inv(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint32_t tickstart;
  uint32_t temp_sr_reg;
  uint16_t init_max_data_in_fifo;
  init_max_data_in_fifo = (((uint16_t)(hspi->Init.FifoThreshold >> 5U) + 1U));
#if defined (__GNUC__)
  __IO uint16_t *prxdr_16bits = (__IO uint16_t *)(&(hspi->Instance->RXDR));
#endif /* __GNUC__ */

  /* Check Direction parameter */
  assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_RXONLY(hspi->Init.Direction));

  /* Check transfer size parameter */
  if (IS_SPI_LIMITED_INSTANCE(hspi->Instance))
  {
    assert_param(IS_SPI_LIMITED_TRANSFER_SIZE(Size));
  }
  else
  {
    assert_param(IS_SPI_TRANSFER_SIZE(Size));
  }

  /* Init tickstart for timeout management*/
  tickstart = HAL_GetTick();

  if (hspi->State != HAL_SPI_STATE_READY)
  {
    return HAL_BUSY;
  }

  if ((pData == NULL) || (Size == 0UL))
  {
    return HAL_ERROR;
  }

  /* Lock the process */
  __HAL_LOCK(hspi);

  /* Set the transaction information */
  hspi->State       = HAL_SPI_STATE_BUSY_RX;
  hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  hspi->pRxBuffPtr  = (uint8_t *)pData;
  hspi->RxXferSize  = Size;
  hspi->RxXferCount = Size;

  /*Init field not used in handle to zero */
  hspi->pTxBuffPtr  = NULL;
  hspi->TxXferSize  = (uint16_t) 0UL;
  hspi->TxXferCount = (uint16_t) 0UL;
  hspi->RxISR       = NULL;
  hspi->TxISR       = NULL;

  /* Configure communication direction: 1Line */
  if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
  {
    SPI_1LINE_RX(hspi);
  }
  else
  {
    SPI_2LINES_RX(hspi);
  }

  /* Set the number of data at current transfer */
  MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, Size);

  /* Enable SPI peripheral */
  __HAL_SPI_ENABLE(hspi);

  if (hspi->Init.Mode == SPI_MODE_MASTER)
  {
    /* Master transfer start */
    SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
  }

  /* Receive data in 32 Bit mode */
  if ((hspi->Init.DataSize > SPI_DATASIZE_16BIT) && (IS_SPI_FULL_INSTANCE(hspi->Instance)))
  {
    /* Transfer loop */
    while (hspi->RxXferCount > 0UL)
    {
      /* Evaluate state of SR register */
      temp_sr_reg = hspi->Instance->SR;

      /* Check the RXP flag */
      if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXP))
      {
        *((uint32_t *)hspi->pRxBuffPtr) = ~*((__IO uint32_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint32_t);
        hspi->RxXferCount--;
      }
      /* Check RXWNE flag if RXP cannot be reached */
      else if ((hspi->RxXferCount < init_max_data_in_fifo) && ((temp_sr_reg & SPI_SR_RXWNE_Msk) != 0UL))
      {
        *((uint32_t *)hspi->pRxBuffPtr) = ~*((__IO uint32_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint32_t);
        hspi->RxXferCount--;
      }
      else
      {
        /* Timeout management */
        if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
        {
          /* Call standard close procedure with error check */
          SPI_CloseTransfer(hspi);

          SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
          hspi->State = HAL_SPI_STATE_READY;

          /* Unlock the process */
          __HAL_UNLOCK(hspi);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  /* Receive data in 16 Bit mode */
  else if (hspi->Init.DataSize > SPI_DATASIZE_8BIT)
  {
    /* Transfer loop */
    while (hspi->RxXferCount > 0UL)
    {
      /* Evaluate state of SR register */
      temp_sr_reg = hspi->Instance->SR;

      /* Check the RXP flag */
      if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXP))
      {
#if defined (__GNUC__)
        *((uint16_t *)hspi->pRxBuffPtr) = ~*prxdr_16bits;
#else
        *((uint16_t *)hspi->pRxBuffPtr) = ~*((__IO uint16_t *)&hspi->Instance->RXDR);
#endif /* __GNUC__ */
        hspi->pRxBuffPtr += sizeof(uint16_t);
        hspi->RxXferCount--;
      }
      /* Check RXWNE flag if RXP cannot be reached */
      else if ((hspi->RxXferCount < init_max_data_in_fifo) && ((temp_sr_reg & SPI_SR_RXWNE_Msk) != 0UL))
      {
#if defined (__GNUC__)
        *((uint16_t *)hspi->pRxBuffPtr) = ~*prxdr_16bits;
#else
        *((uint16_t *)hspi->pRxBuffPtr) = ~*((__IO uint16_t *)&hspi->Instance->RXDR);
#endif /* __GNUC__ */
        hspi->pRxBuffPtr += sizeof(uint16_t);
#if defined (__GNUC__)
        *((uint16_t *)hspi->pRxBuffPtr) = ~*prxdr_16bits;
#else
        *((uint16_t *)hspi->pRxBuffPtr) = ~*((__IO uint16_t *)&hspi->Instance->RXDR);
#endif /* __GNUC__ */
        hspi->pRxBuffPtr += sizeof(uint16_t);
        hspi->RxXferCount -= (uint16_t)2UL;
      }
      /* Check RXPLVL flags when RXWNE cannot be reached */
      else if ((hspi->RxXferCount == 1UL) && ((temp_sr_reg & SPI_SR_RXPLVL_0) != 0UL))
      {
#if defined (__GNUC__)
        *((uint16_t *)hspi->pRxBuffPtr) = ~*prxdr_16bits;
#else
        *((uint16_t *)hspi->pRxBuffPtr) = ~*((__IO uint16_t *)&hspi->Instance->RXDR);
#endif /* __GNUC__ */
        hspi->pRxBuffPtr += sizeof(uint16_t);
        hspi->RxXferCount--;
      }
      else
      {
        /* Timeout management */
        if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
        {
          /* Call standard close procedure with error check */
          SPI_CloseTransfer(hspi);

          SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
          hspi->State = HAL_SPI_STATE_READY;

          /* Unlock the process */
          __HAL_UNLOCK(hspi);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  /* Receive data in 8 Bit mode */
  else
  {
    /* Transfer loop */
    while (hspi->RxXferCount > 0UL)
    {
      /* Evaluate state of SR register */
      temp_sr_reg = hspi->Instance->SR;

      /* Check the RXP flag */
      if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXP))
      {
        *((uint8_t *)hspi->pRxBuffPtr) = ~*((__IO uint8_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint8_t);
        hspi->RxXferCount--;
      }
      /* Check RXWNE flag if RXP cannot be reached */
      else if ((hspi->RxXferCount < init_max_data_in_fifo) && ((temp_sr_reg & SPI_SR_RXWNE_Msk) != 0UL))
      {
        *((uint8_t *)hspi->pRxBuffPtr) = ~*((__IO uint8_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint8_t);
        *((uint8_t *)hspi->pRxBuffPtr) = ~*((__IO uint8_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint8_t);
        *((uint8_t *)hspi->pRxBuffPtr) = ~*((__IO uint8_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint8_t);
        *((uint8_t *)hspi->pRxBuffPtr) = ~*((__IO uint8_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint8_t);
        hspi->RxXferCount -= (uint16_t)4UL;
      }
      /* Check RXPLVL flags when RXWNE cannot be reached */
      else if ((hspi->RxXferCount < 4UL) && ((temp_sr_reg & SPI_SR_RXPLVL_Msk) != 0UL))
      {
        *((uint8_t *)hspi->pRxBuffPtr) = ~*((__IO uint8_t *)&hspi->Instance->RXDR);
        hspi->pRxBuffPtr += sizeof(uint8_t);
        hspi->RxXferCount--;
      }
      else
      {
        /* Timeout management */
        if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
        {
          /* Call standard close procedure with error check */
          SPI_CloseTransfer(hspi);

          SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_TIMEOUT);
          hspi->State = HAL_SPI_STATE_READY;

          /* Unlock the process */
          __HAL_UNLOCK(hspi);

          return HAL_TIMEOUT;
        }
      }
    }
  }

#if (USE_SPI_CRC != 0UL)
  if (hspi->Init.CRCCalculation == SPI_CRCCALCULATION_ENABLE)
  {
    /* Wait for crc data to be received */
    if (SPI_WaitOnFlagUntilTimeout(hspi, SPI_FLAG_EOT, RESET, Timeout, tickstart) != HAL_OK)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
    }
  }
#endif /* USE_SPI_CRC */

  /* Call standard close procedure with error check */
  SPI_CloseTransfer(hspi);

  hspi->State = HAL_SPI_STATE_READY;

  /* Unlock the process */
  __HAL_UNLOCK(hspi);


  if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
  {
    return HAL_ERROR;
  }
  else
  {
    return HAL_OK;
  }
}

/**
  * @brief  Close Transfer and clear flags.
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval HAL_ERROR: if any error detected
  *         HAL_OK: if nothing detected
  */
static void SPI_CloseTransfer(SPI_HandleTypeDef *hspi)
{
  uint32_t itflag = hspi->Instance->SR;

  __HAL_SPI_CLEAR_EOTFLAG(hspi);
  __HAL_SPI_CLEAR_TXTFFLAG(hspi);

  /* Disable SPI peripheral */
  __HAL_SPI_DISABLE(hspi);

  /* Disable ITs */
  __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_TXP | SPI_IT_RXP | SPI_IT_DXP | SPI_IT_UDR | SPI_IT_OVR | \
                              SPI_IT_FRE | SPI_IT_MODF));

  /* Disable Tx DMA Request */
  CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

  /* Report UnderRun error for non RX Only communication */
  if (hspi->State != HAL_SPI_STATE_BUSY_RX)
  {
    if ((itflag & SPI_FLAG_UDR) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_UDR);
      __HAL_SPI_CLEAR_UDRFLAG(hspi);
    }
  }

  /* Report OverRun error for non TX Only communication */
  if (hspi->State != HAL_SPI_STATE_BUSY_TX)
  {
    if ((itflag & SPI_FLAG_OVR) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_OVR);
      __HAL_SPI_CLEAR_OVRFLAG(hspi);
    }

#if (USE_SPI_CRC != 0UL)
    /* Check if CRC error occurred */
    if (hspi->Init.CRCCalculation == SPI_CRCCALCULATION_ENABLE)
    {
      if ((itflag & SPI_FLAG_CRCERR) != 0UL)
      {
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_CRC);
        __HAL_SPI_CLEAR_CRCERRFLAG(hspi);
      }
    }
#endif /* USE_SPI_CRC */
  }

  /* SPI Mode Fault error interrupt occurred -------------------------------*/
  if ((itflag & SPI_FLAG_MODF) != 0UL)
  {
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_MODF);
    __HAL_SPI_CLEAR_MODFFLAG(hspi);
  }

  /* SPI Frame error interrupt occurred ------------------------------------*/
  if ((itflag & SPI_FLAG_FRE) != 0UL)
  {
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FRE);
    __HAL_SPI_CLEAR_FREFLAG(hspi);
  }

  hspi->TxXferCount = (uint16_t)0UL;
  hspi->RxXferCount = (uint16_t)0UL;
}

/**
  * @brief Handle SPI Communication Timeout.
  * @param hspi: pointer to a SPI_HandleTypeDef structure that contains
  *              the configuration information for SPI module.
  * @param Flag: SPI flag to check
  * @param Status: flag state to check
  * @param Timeout: Timeout duration
  * @param Tickstart: Tick start value
  * @retval HAL status
  */
static HAL_StatusTypeDef SPI_WaitOnFlagUntilTimeout(const SPI_HandleTypeDef *hspi, uint32_t Flag, FlagStatus Status,
                                                    uint32_t Timeout, uint32_t Tickstart)
{
  /* Wait until flag is set */
  while ((__HAL_SPI_GET_FLAG(hspi, Flag) ? SET : RESET) == Status)
  {
    /* Check for the Timeout */
    if ((((HAL_GetTick() - Tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}


#endif /* HAL_SPI_MODULE_ENABLED */

