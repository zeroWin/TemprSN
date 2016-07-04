/**************************************************************************************************
  Filename:       hal_battery_monitor.c
  Revised:        $Date: 2016-03-12 19:37:16 +0800 (Sat, 12 Mar 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to the battery monitor.


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
**************************************************************************************************/

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
#include "hal_battery_monitor.h"
#include "hal_adc.h"
#include "hal_oled.h"

#if (defined HAL_BATTERY_MONITOR) && (HAL_BATTERY_MONITOR == TRUE)
/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
/* Batter Monitor enable/disable at P0.1
*/
#define BATTER_MONITOR_EN_PORT 0
#define BATTER_MONIROT_EN_PIN  1

/* Set ADC channel and resolution P0.0
*/
#define BATTER_MONITOR_CHANNEL      HAL_ADC_CHANNEL_0
#define BATTER_MONITOR_RESOLUTION   HAL_ADC_RESOLUTION_14

/* Set Reference Voltages*/
#define BATTER_MONITOR_RefVol       HAL_ADC_REF_AVDD    //SET VDD=3.3V as Vref
/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/
#define BATTER_MINITOR_ENABLE       MCU_IO_OUTPUT(BATTER_MONITOR_EN_PORT,BATTER_MONIROT_EN_PIN,TRUE)
#define BATTER_MINITOR_DISABLE      MCU_IO_OUTPUT(BATTER_MONITOR_EN_PORT,BATTER_MONIROT_EN_PIN,FALSE)


/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/

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
 * @fn      HalBattMonInit
 *
 * @brief   Initilize Battery Monitor
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalBattMonInit(void)
{
//  //Setting ADC reference volage 所有ADC参考电压由HalAdcInit()统一设置。
//  // 这里保留代码但不使用
//  HalAdcSetReference(BATTER_MONITOR_RefVol)
  BATTER_MINITOR_DISABLE;
}


/**************************************************************************************************
 * @fn      HalGetBattVol
 *
 * @brief   Get Battery Volage
 *
 * @param   none
 *
 * @return  Battery Volage
 **************************************************************************************************/
float HalGetBattVol(void)
{
  float tempVol = 0;
  
  BATTER_MINITOR_ENABLE;    // Enable BATT_MON_EN, P0.1 high
  
  tempVol = HalAdcRead( BATTER_MONITOR_CHANNEL, BATTER_MONITOR_RESOLUTION ); 
  //max value = 0x3fff/2, battery voltage = input voltage x 2
  //ref volage=3.3V
  tempVol = tempVol*3.3*2*2/0x3fff;    
  
  BATTER_MINITOR_DISABLE;   // Disable BATT_MON_EN, P0.1 low
  
  return tempVol;
}

/**************************************************************************************************
 * @fn      HalGetBattVol
 *
 * @brief   Show Battery Volage on OLED
 *
 * @param   monitor and show or show last monitor
 *
 * @return  0 Battery volage enough
            1 warring--Battery volage 20%
            2 warring--Battery gone   0%-10%
 **************************************************************************************************/
uint8 HalShowBattVol(uint8 fThreshold)
{
  static float fThreshold_temp;
  float fBattV;
  
  if(fThreshold == BATTERY_MEASURE_SHOW)//测量
  {
     fBattV = HalGetBattVol();
     fThreshold_temp = fBattV;
  }
  
  if(fThreshold_temp >= 4.177)
  {
    HalOledShowString(72,0,12,"100%");
    HalOledShowPowerSymbol(100,0,1,10);  //100%
  }
  else if(fThreshold_temp >= 4.050)
  {
    HalOledShowString(72,0,12," 90%");
    HalOledShowPowerSymbol(100,0,1,9);  //90%
  }
  else if(fThreshold_temp >= 4.000)
  {
    HalOledShowString(72,0,12," 80%");
    HalOledShowPowerSymbol(100,0,1,8);   //80%
  }
  else if(fThreshold_temp >= 3.900)
  {
    HalOledShowString(72,0,12," 70%");
    HalOledShowPowerSymbol(100,0,1,7);   //70%
  }
  else if(fThreshold_temp >= 3.850)
  {
    HalOledShowString(72,0,12," 60%");
    HalOledShowPowerSymbol(100,0,1,6);   //60%
  }
  else if(fThreshold_temp >= 3.800)
  {
    HalOledShowString(72,0,12," 50%");
    HalOledShowPowerSymbol(100,0,1,5);   //50%
  }
  else if(fThreshold_temp >= 3.785)
  {
    HalOledShowString(72,0,12," 40%");
    HalOledShowPowerSymbol(100,0,1,4);   //40%
  }
  else if(fThreshold_temp >= 3.777)
  {
    HalOledShowString(72,0,12," 30%");
    HalOledShowPowerSymbol(100,0,1,3);   //30%
  }
  else if(fThreshold_temp >= 3.722)
  {
    HalOledShowString(72,0,12," 20%");
    HalOledShowPowerSymbol(100,0,1,2);   //20%
  }
  else if(fThreshold_temp >= 3.687)
  {
    HalOledShowString(72,0,12," 10%");
    HalOledShowPowerSymbol(100,0,1,1);  //10%---警告电量，屏幕只显示LowPower
    return 1;
  }
  else
  {
    HalOledShowString(72,0,12,"  0%");
    HalOledShowPowerSymbol(100,0,1,0);  //0%---屏幕黑屏
    return 2;
  }
  return 0;
}


#else
void HalBattMonInit(void);
float HalGetBattVol(void);
uint8 HalShowBattVol(uint8 fThreshold);

#endif /* HAL_BATTERY_MONITOR */