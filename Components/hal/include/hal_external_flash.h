/**************************************************************************************************
  Filename:       hal_external_flash.h
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
  使用SST25FV016B 芯片作为外置FALSH 16Mbit=2MB
**************************************************************************************************/

#ifndef HAL_EXTERNAL_FLASH_H
#define HAL_EXTERNAL_FLASH_H

#ifdef __cplusplus
extern "C"
{
#endif
  
/**************************************************************************************************
 *                                             INCLUDES
 **************************************************************************************************/
#include "hal_board.h"

/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/


/**************************************************************************************************
 *                                              MACROS
 **************************************************************************************************/

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/

  
/**************************************************************************************************
 *                                             FUNCTIONS - API
 **************************************************************************************************/

/*
 * Initialize Ext Flash.
 */
extern void HalExtFlashInit(void);

/*
 * Read Id.
 */
extern uint16 HalExtFlashReadId(void);

/*
 * Read JEDEC Id.
 */
extern uint32 HalExtFlashReadJEDECId(void);


/*
 * Read buffer data.
 * PS: 如果watch显示数组某些元素是0，可能是编译器的问题，其实值不是0。
 */
extern void HalExtFlashBufferRead(uint8 *pBuffer,uint32 readAddress,uint16 readLength);


/*
 * Read one Byte data.
 */
extern uint8 HalExtFlashByteRead(uint32 readAddress);

/*
 * Write one Byte data.
 */
extern void HalExtFlashByteWrite(uint32 writeAddress,uint8 writeData);

/*
 * Write buffer data.
 */
extern void HalExtFlashBufferWrite(uint8* writebuffer,uint32 writeAddress,uint16 writeLength);


#ifdef __cplusplus
}
#endif  
#endif