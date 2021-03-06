/**************************************************************************************************
  Filename:       hal_spi_user.c
  Revised:        $Date: 2016-04-14 17:21:16 +0800 (Thus, 14 Apr 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to Spi Service.


  Copyright 2016 Bupt. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact kylinnevercry@gami.com. 
  SPI 底层驱动
**************************************************************************************************/

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
#include "hal_spi_user.h"

#if (defined HAL_SPI_USER) && (HAL_SPI_USER == TRUE)
/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
// UxCSR - USART Control and Status Register.
#define CSR_MODE                   0x80
#define CSR_RE                     0x40
#define CSR_SLAVE                  0x20
#define CSR_FE                     0x10
#define CSR_ERR                    0x08
#define CSR_RX_BYTE                0x04
#define CSR_TX_BYTE                0x02
#define CSR_ACTIVE                 0x01

// UxUCR - USART UART Control Register.
#define UCR_FLUSH                  0x80
#define UCR_FLOW                   0x40
#define UCR_D9                     0x20
#define UCR_BIT9                   0x10
#define UCR_PARITY                 0x08
#define UCR_SPB                    0x04
#define UCR_STOP                   0x02
#define UCR_START                  0x01

// User.
#define PxOUT                      P0
#define PxDIR                      P0DIR
#define PxSEL                      P0SEL
#define UxCSR                      U0CSR
#define UxUCR                      U0UCR
#define UxDBUF                     U0DBUF
#define UxBAUD                     U0BAUD
#define UxGCR                      U0GCR
#define URXxIE                     URX0IE
#define URXxIF                     URX0IF
#define UTXxIE                     UTX0IE
#define UTXxIF                     UTX0IF

#define HAL_UART_PERCFG_BIT        0x01         // USART0 on P0, Alt-1; so clear this bit.

/* Flash SPI CE is at P0.6 */
#define HAL_FLASH_SPI_CE_PORT  0
#define HAL_FLASH_SPI_CE_PIN   6

/* AD7793 SPI CE is at P0.4 */
#define HAL_AD7793_SPI_CE_PORT  0
#define HAL_AD7793_SPI_CE_PIN   4


/* SPI SCLK,MO,MI is at P0.5,P0.3,P0,2 */
#define HAL_SPI_SCLK_MO_MI             0x2C

/* SPI baud rate 115200*/
#define HAL_SPI_UxBAUD_NUM             216 
#define HAL_SPI_UxGCR_NUM              11

/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/

  
/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/
#define HAL_SPI_FLASH_ENABLE()      MCU_IO_OUTPUT(HAL_FLASH_SPI_CE_PORT, HAL_FLASH_SPI_CE_PIN, 0)
#define HAL_SPI_FLASH_DISABLE()     MCU_IO_OUTPUT(HAL_FLASH_SPI_CE_PORT, HAL_FLASH_SPI_CE_PIN, 1)

#define HAL_SPI_AD7793_ENABLE()      MCU_IO_OUTPUT(HAL_AD7793_SPI_CE_PORT, HAL_AD7793_SPI_CE_PIN, 0)
#define HAL_SPI_AD7793_DISABLE()     MCU_IO_OUTPUT(HAL_AD7793_SPI_CE_PORT, HAL_AD7793_SPI_CE_PIN, 1)

#define HAL_SPI_TX(x)               st(UxCSR &= ~CSR_TX_BYTE; UxDBUF = (x);)
#define HAL_SPI_RX()                UxDBUF
#define HAL_SPI_WAIT_RXRDY()        st(while (!(UxCSR & CSR_TX_BYTE));) //发送完，接收就完了。


/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/

/**************************************************************************************************
 * @fn      HalSpiUInit
 *
 * @brief   Initialize SPI
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalSpiUInit(void)
{
  /* Mode select UART0 SPI Mode as master. */
  UxCSR &= ~CSR_MODE; 
  
  /* Setup the baud. */
  UxGCR = HAL_SPI_UxGCR_NUM; 
  UxBAUD = HAL_SPI_UxBAUD_NUM; 
  
  /* Set bit order to MSB */
  UxGCR |= BV(5); 
  
  /* USE SPI MODE3 */
  UxGCR |= BV(6);
  UxGCR |= BV(7);
  
  /* Set UART0 I/O to alternate 1 location on P0 pins. */
  PERCFG &= ~HAL_UART_PERCFG_BIT;  /* U0CFG */
  
  /* Select peripheral function on I/O pins but SS is left as GPIO for separate control. */
  PxSEL |= HAL_SPI_SCLK_MO_MI;  /* SELP0_[2,3,5] */

  /* Give UART0 priority over UART1 priority over Timer1. */
  P2DIR &= ~0xC0;  /* PRIP0 */
  
  /* When SPI config is complete, enable it. */
  UxCSR |= CSR_RE; 

  /* Release XNV reset.AD7793 reset */
  HAL_SPI_FLASH_DISABLE(); 
  HAL_SPI_AD7793_DISABLE();
}


/**************************************************************************************************
 * @fn      HalSpiWriteReadByte
 *
 * @brief   Write/read a Byte
 *
 * @param   Write Byte
 *
 * @return  Read Byte
 **************************************************************************************************/
uint8 HalSpiWriteReadByte(uint8 TxData)
{
  uint8 RxData;
  
  HAL_SPI_TX(TxData);
  HAL_SPI_WAIT_RXRDY();
  RxData = HAL_SPI_RX();
  
  return RxData;
}

/**************************************************************************************************
 * @fn      HalSpiFlashEnable
 *
 * @brief   Enable Flash Deivce.CE=0
 *
 * @param  
 *
 * @return  
 **************************************************************************************************/
void HalSpiFlashEnable(void)
{
  HAL_SPI_FLASH_ENABLE();
}


/**************************************************************************************************
 * @fn      HalSpiFlashDisable
 *
 * @brief   Disable Flash Deivce.CE=1
 *
 * @param   
 *
 * @return  
 **************************************************************************************************/
void HalSpiFlashDisable(void)
{
  HAL_SPI_FLASH_DISABLE();
}


/**************************************************************************************************
 * @fn      HalSpiAD7793Enable
 *
 * @brief   Enable AD7793 Deivce.CE=0
 *
 * @param  
 *
 * @return  
 **************************************************************************************************/
void HalSpiAD7793Enable(void)
{
  HAL_SPI_AD7793_ENABLE();
}


/**************************************************************************************************
 * @fn      HalSpiAD7793Disable
 *
 * @brief   Enable AD7793 Deivce.CE=1
 *
 * @param  
 *
 * @return  
 **************************************************************************************************/
void HalSpiAD7793Disable(void)
{
  HAL_SPI_AD7793_DISABLE();
}

#else

void HalSpiUInit(void);
void HalSpiFlashEnable(void);
void HalSpiFlashDisable(void);
uint8 HalSpiWriteReadByte(uint8 TxData);
void HalSpiAD7793Enable(void);
void HalSpiAD7793Disable(void);

#endif