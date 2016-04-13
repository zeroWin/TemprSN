/**************************************************************************************************
  Filename:       hal_external_flash.c
  Revised:        $Date: 2016-04-11 14:08:16 +0800 (Mon, 11 Apr 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to the External Flash Service.


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
  使用SST25FV016B 芯片作为外置FALSH
**************************************************************************************************/

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
#include "hal_external_flash.h"

#if (defined HAL_EXTERNAL_FLASH) && (HAL_EXTERNAL_FLASH == TRUE)
/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
/* Ext Flash CE is at P0.6 */
#define EXT_FLASH_CE_PORT  0
#define EXT_FLASH_CE_PIN   6

/* Ext Flash SCLK is at P0.5 */
#define EXT_FLASH_SCLK_PORT   0
#define EXT_FLASH_SCLK_PIN    5

/* Ext Flash SI -- CC2530 SPI-MO is at P0.3 */
#define EXT_FLASH_MOSI_PORT 0
#define EXT_FLASH_MOSI_PIN  3

/* Ext Flash SO -- CC2530 SPI-MI is at P0.2 */
#define EXT_FLASH_MISO_PORT 0
#define EXT_FLASH_MISO_PIN  2

/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/

/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/

  
/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/

  
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/

/**************************************************************************************************
 * @fn      HalExtFlashInit
 *
 * @brief   Initialize Ext Flash
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/


void HalExtFlashInit(void)
{
  /* Mode select UART0 SPI Mode as master. */
  U0CSR = 0; 
  
  /* Setup for 115200 baud. */
  U0GCR = 11; 
  U0BAUD = 216; 
  
  /* Set bit order to MSB */
  U0GCR |= BV(5); 
  
  /* Set UART0 I/O to alternate 1 location on P0 pins. */
  PERCFG &= ~0x01;  /* U0CFG */
  
  /* Select peripheral function on I/O pins but SS is left as GPIO for separate control. */
  P0SEL |= 0x3C;  /* SELP0_[5:2] */
  /* P0.4 reset, XNV CS. */
  P0SEL &= ~0x10; 
  P0 |= 0x10; 
  P0_6 = 0; 
  P0DIR |= 0x40; 
  
  /* Give UART0 priority over UART1 priority over Timer1. */
  P2DIR &= ~0xC0;  /* PRIP0 */
  
  /* When SPI config is complete, enable it. */
  U0CSR |= 0x40; 
  /* Release XNV reset. */
  P0_6 = 1;   
}

#else


#endif /* HAL_EXTERNAL_FLASH */