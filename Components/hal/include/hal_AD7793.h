/**************************************************************************************************
  Filename:       hal_AD7793.h
  Revised:        $Date: 2016-04-19 21:40:16 +0800 (Tus, 19 Apr 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to the AD7793.


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
  使用AD7793,通信协议采用SPI MODE3
**************************************************************************************************/

#ifndef HAL_AD7793_H
#define HAL_AD7793_H

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
typedef enum
{
  AD7793_RATE_4dot17 = 0,
  AD7793_RATE_8dot33,
  AD7793_RATE_16dot7,
  AD7793_RATE_33dot2,
  AD7793_RATE_62dot0,
  AD7793_RATE_NUM
} AD7793Rate_t; // update rate.


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
 * Initialize AD7793.
 */
extern void HalAD7793Init(void);

/*
 * Config the first channel (AIN1).
 */
extern void AD7793_AIN1_config_one(AD7793Rate_t OutUpdateRate);

/*
 * Config the second channel (AIN2).
 */
extern void AD7793_AIN2_config_one(AD7793Rate_t OutUpdateRate);

/*
 * Config the third channel (AIN3).
 */
extern void AD7793_AIN3_config_one(AD7793Rate_t OutUpdateRate);

/*
 * Get the first channel (AIN1) data.
 */
extern float AD7793_AIN1_fetch_one(void);

/*
 * Get the second channel (AIN2) data.
 */
extern float AD7793_AIN2_fetch_one(void);

/*
 * Get the third channel (AIN3) data.
 */
extern float AD7793_AIN3_fetch_one(void);

/*
 * Judge AD over or not.
 */
extern bool AD7793_IsReadyToFetch(void);

#ifdef __cplusplus
}
#endif  
#endif