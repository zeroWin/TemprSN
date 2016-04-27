/**************************************************************************************************
  Filename:       measTempr.h
  Revised:        $Date: 2016-04-25 20:59:16 +0800 (Mon, 25 Apr 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to the Algorithm of measure tempr.


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

  该文件提供计算体温的算法
**************************************************************************************************/

#ifndef MEAS_TEMPR_H
#define MEAS_TEMPR_H

#ifdef __cplusplus
extern "C"
{
#endif
  
/**************************************************************************************************
 *                                             INCLUDES
 **************************************************************************************************/
#include "hal_board.h"
#include "hal_rtc_ds1302.h"
/**************************************************************************************************
 * MACROS
 **************************************************************************************************/

  
/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/

  
/***************************************************************************************************
 *                                             TYPEDEFS
 ***************************************************************************************************/
typedef struct
{
  real32 fPtVolt;
  real32 fRefVolt;
  real32 fThermoVolt;

  real32 fColdEndDegree;
  real32 fWorkEndDegree;
  real32 reserved;
}measResult_t;

typedef struct
{
  RTCStruct_t MeasTime; // Byte[10];
  real32      fTempDegree;
  uint8       flag;
  uint8       reserved;
}ResultStore_t;

typedef struct
{
  real32 CalCoef_a;
  real32 CalCoef_b;
  real32 CalCoef_delta;
  real32 ReservedParm1;
  real32 ReservedParm2; 
  real32 ReservedParm3; // reserved to indicate the validation of data.
                        // invalid if it < 0.0f;
                        // valid   if it > 0.0f;
}SensorCalCoef_t;
/**************************************************************************************************
 *                                             FUNCTIONS - API
 **************************************************************************************************/

/*
 * Measure the temperature of work end.
 */
extern bool measWorkEndTemperature(measResult_t *pMeasRlt);

/*
 * Measure the temperature of cold end of thermo-couple.
 */
extern bool CheckMeasComplete(measResult_t *pMeasRlt, 
                              uint16    curr_result_idx, 
                              real32   *pfOuputDegree,
                              real32   *pfColdEndDegree);
#ifdef __cplusplus
}
#endif  
#endif