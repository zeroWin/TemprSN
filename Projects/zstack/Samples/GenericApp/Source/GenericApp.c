/******************************************************************************
  Filename:       GenericApp.c
  Revised:        $Date: 2010-12-21 10:27:34 -0800 (Tue, 21 Dec 2010) $
  Revision:       $Revision: 24670 $

  Description:    Generic Application (no Profile).


  Copyright 2004-2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
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
  contact Texas Instruments Incorporated at www.TI.com.
******************************************************************************/

/*********************************************************************
  This application is for TemprSN
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "GenericApp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "hal_oled.h"
#include "hal_external_flash.h"
#include "hal_battery_monitor.h"
#include "hal_rtc_ds1302.h"
#include "hal_AD7793.h"
#include "measTempr.h"

#include "string.h"
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define MEAS_TEMPR_LOOP_COUNT_MAX     80

/*********************************************************************
 * TYPEDEFS
 */
typedef enum
{
  AD7793_IDLE,
  AD7793_SAMPLE
} AD7793State_t;


/*********************************************************************
 * GLOBAL VARIABLES
 */

// This list should be filled with Application specific Cluster IDs.
const cId_t GenericApp_InClusterList[GENERICAPP_IN_CLUSTERS] =
{
  GENERICAPP_CLUSTERID,
  GENERICAPP_CLUSTERID_START,
  GENERICAPP_CLUSTERID_SYNC
};

const cId_t GenericApp_OutClusterList[GENERICAPP_OUT_CLUSTERS] =
{
  GENERICAPP_CLUSTERID,
  GENETICAPP_CLUSTERID_TEMPR_SYNC_OVER,
  GENERICAPP_CLUSTERID_TEMPR_RESULT
};

const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
  GENERICAPP_ENDPOINT,                  //  int Endpoint;
  GENERICAPP_PROFID,                    //  uint16 AppProfId[2];
  GENERICAPP_DEVICEID,                  //  uint16 AppDeviceId[2];
  GENERICAPP_DEVICE_VERSION,            //  int   AppDevVer:4;
  GENERICAPP_FLAGS,                     //  int   AppFlags:4;
  GENERICAPP_IN_CLUSTERS,               //  byte  AppNumInClusters;
  (cId_t *)GenericApp_InClusterList,    //  byte *pAppInClusterList;
  GENERICAPP_OUT_CLUSTERS,              //  byte  AppNumOutClusters;
  (cId_t *)GenericApp_OutClusterList    //  byte *pAppOutClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in GenericApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t GenericApp_epDesc;

// This is for GenericApp state machine。
TemprSystemStatus_t TemprSystemStatus;
/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte GenericApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // GenericApp_Init() is called.
devStates_t GenericApp_NwkState;


byte GenericApp_TransID;  // This is the unique message ID (counter)

afAddrType_t GenericApp_DstAddr;

/* For measure */
AD7793State_t  appAD7793State;         // ADC状态
static uint16  ad7793RegSetDelay_ms;   // 启动采样后获取结果的延时时间
AD7793Rate_t   ad7793UpdateRate;       // 选择的采样频率
measResult_t   measRltArray[MEAS_TEMPR_LOOP_COUNT_MAX]; // 存储测量和计算中间值
ResultStore_t  OneRltStore;            // 存储一次的测量结果

static uint16 retryNumOfMeasTempr;  // 记录计算体温的次数
static SensorCalCoef_t   s_factoryCalCoef;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void GenericApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
void GenericApp_HandleKeys( byte shift, byte keys );
void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );


void GenericApp_DoMeasTempr(void);
void GenericApp_PtVoltSample(void);
void GenericApp_RefVoltSample(void);
void GenericApp_ThermoVoltSample(void);

void GenericApp_MeasTemprInit(void);
void GenericApp_InitMeasResultArray(void);
void MeasTemprComplete(real32 fOutputDegree, real32 fColdEndDegree, bool isStableRlt);
real32 CalWorkEndTemp(real32 fWorkEndDegree, real32 fColdEndDegree);

void GenericApp_HandleNetworkStatus( devStates_t GenericApp_NwkStateTemp);
void GenericApp_LeaveNetwork( void );
void GenericApp_SyncData(void);
void HalOledDispStaDurMeas(real32 data,TemprSystemStatus_t deviceStatus);
void HalOledDispTempr(real32 data);
/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      GenericApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void GenericApp_Init( byte task_id )
{
  GenericApp_TaskID = task_id;
  GenericApp_NwkState = DEV_INIT;
  GenericApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  GenericApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  GenericApp_DstAddr.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_DstAddr.addr.shortAddr = 0;

  // Fill out the endpoint description.
  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &GenericApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( GenericApp_TaskID );
  
  // Init system status 
  TemprSystemStatus = TEMPR_OFFLINE_IDLE;
  
  // Init Low power status
  TemprLowPower = TEMPR_WORK;
  
  // Update the display
#if defined ( LCD_SUPPORTED )
    HalLcdWriteString( "GenericApp", HAL_LCD_LINE_1 );
#endif
    
  // init OLED show
  HalOledShowString(TEMPR_RESULT_X,TEMPR_RESULT_Y,
                    TEMPR_RESULT_SIZE,TEMPR_RESULT_DEFAULT);
  HalOledShowDegreeSymbol(TEMPR_SYMBOL_X,TEMPR_SYMBOL_Y); //显示摄氏度符号
  HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                  DEVICE_INFO_SIZE,DEVICE_INFO_OFFLINE_IDLE);
  HalShowBattVol(BATTERY_MEASURE_SHOW);
  HalOledRefreshGram();
  
  ZDO_RegisterForZDOMsg( GenericApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( GenericApp_TaskID, Match_Desc_rsp );
}

/*********************************************************************
 * @fn      GenericApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
UINT16 GenericApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  // Data Confirmation message fields
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // This should match the value sent
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          GenericApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          GenericApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_DATA_CONFIRM_CMD:
          // This message is received as a confirmation of a data packet sent.
          // The status is of ZStatus_t type [defined in ZComDef.h]
          // The message fields are defined in AF.h
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // Action taken when confirmation is received.
          if ( sentStatus != ZSuccess )
          {
            // The data wasn't delivered -- Do something
          }
          break;

        case AF_INCOMING_MSG_CMD:
          GenericApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          GenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          
          GenericApp_HandleNetworkStatus(GenericApp_NwkState);
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // handle GENERICAPP_PT_VOLT_SAMPLE
  if(events & GENERICAPP_PT_VOLT_SAMPLE)
  {
    GenericApp_PtVoltSample();
    
    return (events ^ GENERICAPP_PT_VOLT_SAMPLE);
  }  
  
  // handle GENERICAPP_REF_VOLT_SAMPLE
  if(events & GENERICAPP_REF_VOLT_SAMPLE)
  {
    GenericApp_RefVoltSample();
    
    return (events ^ GENERICAPP_REF_VOLT_SAMPLE);
  }

  // handle GENERICAPP_THERMO_VOLT_SAMPLE
  if(events & GENERICAPP_THERMO_VOLT_SAMPLE)
  {
    GenericApp_ThermoVoltSample();
    
    return (events ^ GENERICAPP_THERMO_VOLT_SAMPLE);
  }  
  
  
  // handle meas tempr event
  if (events & GENERICAPP_DO_MEAS_TEMPR)
  {
    if(TemprSystemStatus == TEMPR_FIND_NETWORK) // 在线测量过程中突然断网
    { 
      // stop measure
      HalOledShowString(TEMPR_RESULT_X,TEMPR_RESULT_Y,
                        TEMPR_RESULT_SIZE,TEMPR_RESULT_DEFAULT);
      HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                        DEVICE_INFO_SIZE,DEVICE_INFO_FIND_NWK);
      HalShowBattVol(BATTERY_NO_MEASURE_SHOW);
      HalOledRefreshGram();      
      
    }
    else // start temperature measurement.
      GenericApp_DoMeasTempr();
    
    return (events ^ GENERICAPP_DO_MEAS_TEMPR);
  }  
  
  // SYNC DATA
  if (events &  GENERICAPP_TEMPR_SYNC)
  {
    // start temperature measurement.
    if(TemprSystemStatus == TEMPR_SYNC_DATA) // 只有同步状态才同步
      GenericApp_SyncData();
    else // 搜寻网络或其他状态
      HalExtFlashLoseNetwork();
    
    return (events ^  GENERICAPP_TEMPR_SYNC);
  } 
 
  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */

/*********************************************************************
 * @fn      GenericApp_ProcessZDOMsgs()
 *
 * @brief   Process response messages
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // Light LED
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
#if defined( BLINK_LEDS )
      else
      {
        // Flash LED to show failure
        HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
      }
#endif
      break;

    case Match_Desc_rsp:
      {
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        if ( pRsp )
        {
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            GenericApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            GenericApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // Take the first endpoint, Can be changed to search through endpoints
            GenericApp_DstAddr.endPoint = pRsp->epList[0];

            // Light LED
            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );
        }
      }
      break;
  }
}

/*********************************************************************
 * @fn      GenericApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_3
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
void GenericApp_HandleKeys( byte shift, byte keys )
{
  if(TemprLowPower == TEMPR_LOW_POWER) // 处于低功耗状态
  {
    HalOledOnOff(HAL_OLED_MODE_ON);
    HalShowBattVol(BATTERY_MEASURE_SHOW); 
    HalOledRefreshGram();
    TemprLowPower = TEMPR_WORK;
    return;
  }
  
  if(keys & HAL_KEY_SW_6) // Link key
  {
    switch(TemprSystemStatus)
    {
      case TEMPR_OFFLINE_IDLE:  // 离线-->寻找网络
      {
        if( ZDApp_StartJoiningCycle() == FALSE )
          if( ZDOInitDevice(0) == ZDO_INITDEV_LEAVE_NOT_STARTED) //Start Network
            ZDOInitDevice(0);
        TemprSystemStatus = TEMPR_FIND_NETWORK;
      }
      break;
      
      case TEMPR_ONLINE_IDLE: // 在线-->离线
      {
        // Leave Network
        GenericApp_LeaveNetwork(); 
        TemprSystemStatus = TEMPR_CLOSE;
       
        HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                          DEVICE_INFO_SIZE,DEVICE_INFO_CLOSING);
      }
      break;
      
      case TEMPR_FIND_NETWORK: // 寻找网络-->离线
      {
        // Stop search network
        ZDApp_StopJoiningCycle();
        TemprSystemStatus = TEMPR_OFFLINE_IDLE;
        
        HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                          DEVICE_INFO_SIZE,DEVICE_INFO_OFFLINE_IDLE);
      }
      break;
      default:// Online , Offline measure , closing , SYNC
        break;//do nothing
    }
  }
  
  if(keys & HAL_KEY_SW_7) // Work key
  {
    switch(TemprSystemStatus)
    {
      case TEMPR_ONLINE_IDLE:  // 在线空闲
      {
        TemprSystemStatus = TEMPR_ONLINE_MEASURE;
        
        GenericApp_MeasTemprInit();
        // to start from PT volt sampling
        osal_set_event(GenericApp_TaskID, GENERICAPP_PT_VOLT_SAMPLE);
      }
      break;
      
      case TEMPR_OFFLINE_IDLE: // 离线空闲
      {
        TemprSystemStatus = TEMPR_OFFLINE_MEASURE;
        
        GenericApp_MeasTemprInit();
        // to start from PT volt sampling
        osal_set_event(GenericApp_TaskID, GENERICAPP_PT_VOLT_SAMPLE);
      }
      break;
      default:// Online , Offline measure , closing , SYNC
        break;//do nothing
    }
    
  }

  HalOledRefreshGram();
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      GenericApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  switch ( pkt->clusterId )
  {
    case GENERICAPP_CLUSTERID:
      // "the" message
#if defined( LCD_SUPPORTED )
      HalLcdWriteScreen( (char*)pkt->cmd.Data, "rcvd" );
#elif defined( WIN32 )
      WPRINTSTR( pkt->cmd.Data );
#endif
      
      break;
    case GENERICAPP_CLUSTERID_START:
      if(TemprSystemStatus == TEMPR_ONLINE_IDLE)
      {
        TemprSystemStatus = TEMPR_ONLINE_MEASURE;
        
        GenericApp_MeasTemprInit();
        // to start from PT volt sampling
        osal_set_event(GenericApp_TaskID, GENERICAPP_PT_VOLT_SAMPLE);    
      }
      break;
      
    case GENERICAPP_CLUSTERID_SYNC:
      if(TemprSystemStatus == TEMPR_ONLINE_IDLE)
      {
        TemprSystemStatus = TEMPR_SYNC_DATA;
        osal_set_event(GenericApp_TaskID, GENERICAPP_TEMPR_SYNC);
      }
      break;
  }
}


/*********************************************************************
 * @fn      GenericApp_InitMeasResultArray
 *
 * @brief   none.
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_InitMeasResultArray(void)
{
  uint8 idx = 0;

  for (idx = 0; idx < MEAS_TEMPR_LOOP_COUNT_MAX; idx ++)
  {
    measRltArray[idx].fPtVolt = 0.0f;
    measRltArray[idx].fRefVolt = 0.0f;
    measRltArray[idx].fThermoVolt = 0.0f;
    measRltArray[idx].fColdEndDegree = -1.0f;
    measRltArray[idx].fWorkEndDegree = -1.0f;
  }
}


/*********************************************************************
 * @fn      GenericApp_PtVoltSample()
 *
 * @brief   none.
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_PtVoltSample(void)
{
  real32 fVolt = 0.0f;
  
  if (appAD7793State == AD7793_IDLE) // 配置并启动采样
  {
    AD7793_AIN2_config_one(ad7793UpdateRate);

    appAD7793State = AD7793_SAMPLE;
    osal_start_timerEx(GenericApp_TaskID, GENERICAPP_PT_VOLT_SAMPLE, ad7793RegSetDelay_ms);
  }
  else if ((appAD7793State == AD7793_SAMPLE) && AD7793_IsReadyToFetch())
  {
    fVolt = AD7793_AIN2_fetch_one(); // 获取结果
    measRltArray[retryNumOfMeasTempr].fPtVolt = fVolt;

    appAD7793State = AD7793_IDLE;
    osal_set_event(GenericApp_TaskID, GENERICAPP_REF_VOLT_SAMPLE);
  }
  else
  {
    osal_start_timerEx(GenericApp_TaskID, GENERICAPP_PT_VOLT_SAMPLE, ad7793RegSetDelay_ms/10);
  }
}


/*********************************************************************
 * @fn      GenericApp_RefVoltSample()
 *
 * @brief   none.
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_RefVoltSample(void)
{
  real32 fVolt = 0.0f;
  
  if (appAD7793State == AD7793_IDLE)
  {
    AD7793_AIN3_config_one(ad7793UpdateRate); // 配置并启动采样

    appAD7793State = AD7793_SAMPLE;
    osal_start_timerEx(GenericApp_TaskID, GENERICAPP_REF_VOLT_SAMPLE, ad7793RegSetDelay_ms);
  }
  else if ((appAD7793State == AD7793_SAMPLE) && AD7793_IsReadyToFetch())
  {
    fVolt = AD7793_AIN3_fetch_one();  // 获取结果
    measRltArray[retryNumOfMeasTempr].fRefVolt = fVolt;

    appAD7793State = AD7793_IDLE;
    osal_set_event(GenericApp_TaskID, GENERICAPP_THERMO_VOLT_SAMPLE);
  }
  else
  {
    osal_start_timerEx(GenericApp_TaskID, GENERICAPP_REF_VOLT_SAMPLE, ad7793RegSetDelay_ms/10);
  }
}


/*********************************************************************
 * @fn      GenericApp_ThermoVoltSample()
 *
 * @brief   none.
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_ThermoVoltSample(void)
{
  real32 fVolt = 0.0f;
  
  if (appAD7793State == AD7793_IDLE)
  {
    AD7793_AIN1_config_one(ad7793UpdateRate); // 配置并启动采样

    appAD7793State = AD7793_SAMPLE;
    osal_start_timerEx(GenericApp_TaskID, GENERICAPP_THERMO_VOLT_SAMPLE, ad7793RegSetDelay_ms);
  }
  else if ((appAD7793State == AD7793_SAMPLE) && AD7793_IsReadyToFetch())
  {
    fVolt = AD7793_AIN1_fetch_one(); // 获取结果
    measRltArray[retryNumOfMeasTempr].fThermoVolt = fVolt;

    appAD7793State = AD7793_IDLE;
    osal_set_event(GenericApp_TaskID, GENERICAPP_DO_MEAS_TEMPR);
  }
  else
  {
    osal_start_timerEx(GenericApp_TaskID, GENERICAPP_THERMO_VOLT_SAMPLE, ad7793RegSetDelay_ms/10);
  }  
}


/*********************************************************************
 * @fn      GenericApp_DoMeasTempr()
 *
 * @brief   Process do meas temperature event
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_DoMeasTempr(void)
{
  real32 fColdEndTempDegree = 0.0f;
  real32 fOutputDegree = -1.0f;
   
  bool isComplete = FALSE; 
  bool isValidRlt = FALSE;
  bool isStableRlt = FALSE;
  
  static uint8 num_point = 1;
  
  isValidRlt = measWorkEndTemperature(&measRltArray[retryNumOfMeasTempr]);
  
  switch(num_point)
  {
    case 1:HalOledShowString(10,8,64,"-");HalOledShowString(30,8,64,"-");HalOledShowString(50,8,64,"-");HalOledShowString(70,8,64,"-");num_point++;break;
    case 2:HalOledShowString(10,8,32,"     ");num_point=1;break;
  }
  HalOledDispStaDurMeas(measRltArray[retryNumOfMeasTempr].fWorkEndDegree,TemprSystemStatus);
  HalShowBattVol(BATTERY_NO_MEASURE_SHOW);
  HalOledRefreshGram();
  if (isValidRlt == FALSE)
  { // once get the invalid result, to stop and quit.
    isComplete = TRUE;
  }
  else
  {
    isComplete = CheckMeasComplete( &measRltArray[0], 
                                    retryNumOfMeasTempr, 
                                    &fOutputDegree,
                                    &fColdEndTempDegree); 
  }  
  // to start new loop
  retryNumOfMeasTempr ++;
  
  // simply reset to disable fast measurement, that is, slow_meas at least
  // to execute 60% loop of maximum loop number.
  #if (defined(SLOW_MEAS) && SLOW_MEAS == TRUE)
  
  uint16 count_slow_th = MEAS_TEMPR_LOOP_COUNT_MAX*2/3;   
  if (retryNumOfMeasTempr < count_slow_th)
    isComplete = FALSE;
  
  #endif
  
  if ((retryNumOfMeasTempr < MEAS_TEMPR_LOOP_COUNT_MAX) && (isComplete == FALSE))
  { // not complete, to start new loop of meas temperature;
    osal_set_event(GenericApp_TaskID, GENERICAPP_PT_VOLT_SAMPLE);
  }
  else
  { // 测量结束
    // GenericApp_AppState = APP_READY;
    // if it is completed as expected, the result shall be stable;
    isStableRlt = isComplete;
    MeasTemprComplete(fOutputDegree, fColdEndTempDegree, isStableRlt);
    
    return;
  }
  
  return;
}


/*********************************************************************
 * @fn      GenericApp_MeasTemprInit()
 *
 * @brief   Init Measure status and start event
 *          Be called when work pressed
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_MeasTemprInit(void)
{
  retryNumOfMeasTempr = 0;
  appAD7793State = AD7793_IDLE;
  
  // 480ms, 240ms, 120ms ,60ms or 32ms
  ad7793UpdateRate     = AD7793_RATE_33dot2;
  ad7793RegSetDelay_ms = (uint16)(AD7793_REG_SET_DELAY[ad7793UpdateRate]*1.05); 
    
  GenericApp_InitMeasResultArray();
}


/*********************************************************************
 * @fn      MeasTemprComplete()
 *
 * @brief   temperature measurement is completed, and to finalize the results.
 *
 * @param   none
 *
 * @return  none
 */
void MeasTemprComplete(real32 fOutputDegree, real32 fColdEndDegree, bool isStableRlt)
{
  real32 fOutputTmp = 0.0f;
  int32  iOutputTmp = 0;
  

  fOutputTmp = CalWorkEndTemp(fOutputDegree, fColdEndDegree);
    
  // to round the resolution to 0.05 degree.
  iOutputTmp = (int32)(fOutputTmp*100/5.0 + 0.5);
  OneRltStore.fTempDegree = (real32)iOutputTmp * 0.05f; 

  // continuously measure temperature until we get stable temp result;
  #if (defined(GO_TO_STABLE) && GO_TO_STABLE == TRUE)
  if (!isStableRlt) // 未稳定
  {
    GenericApp_MeasTemprInit();
    // pause with 500ms to wait ota msg transmition complete;
    osal_start_timerEx(GenericApp_TaskID, GENERICAPP_DO_MEAS_TEMPR, 500);
  }
  else // 稳定,进行状态切换
  {
    // 显示稳定温度
    HalOledDispTempr(OneRltStore.fTempDegree);
    HalShowBattVol(BATTERY_MEASURE_SHOW);
        
    ExtFlashStruct_t ExtFlashStruct;
    HalRTCGetOrSetFull(RTC_DS1302_GET,&ExtFlashStruct.RTCStruct);
    // 先存低位
    floatAndByteConv_t floatAndByteConv;
    floatAndByteConv.floatData = OneRltStore.fTempDegree;
    ExtFlashStruct.sampleData[0] =  floatAndByteConv.byteData[0];
    ExtFlashStruct.sampleData[1] =  floatAndByteConv.byteData[1];
    ExtFlashStruct.sampleData[2] =  floatAndByteConv.byteData[2];
    ExtFlashStruct.sampleData[3] =  floatAndByteConv.byteData[3];
    
    if(TemprSystemStatus == TEMPR_ONLINE_MEASURE) // 在线状态发送数据
    {
      TemprSystemStatus = TEMPR_ONLINE_IDLE;
      HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                        DEVICE_INFO_SIZE,DEVICE_INFO_ONLINE_IDLE);
      
      HalOledRefreshGram();
      // 如何测量结果错误，直接返回，不发送
      if((OneRltStore.fTempDegree < 0.0) || (OneRltStore.fTempDegree >= 100.0))
        return;
      
      // 发送
      AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID_TEMPR_RESULT,
                       TEMPR_RESULT_BYTE_PER_PACKET,
                       (uint8 *)&ExtFlashStruct,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS );
      
    }
    if(TemprSystemStatus == TEMPR_OFFLINE_MEASURE) // 离线状态存储数据
    {
      TemprSystemStatus = TEMPR_OFFLINE_IDLE;
      HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                          DEVICE_INFO_SIZE,DEVICE_INFO_OFFLINE_IDLE);
      
      HalOledRefreshGram();
      // 如何测量结果错误，直接返回，不存储
      if((OneRltStore.fTempDegree < 0.0) || (OneRltStore.fTempDegree >= 100.0))
        return;      
      
      // 存储 写入flash
      HalExtFlashDataWrite(ExtFlashStruct);
    }
  }
  #endif
  
  return;
}

/*********************************************************************
 * @fn      CalWorkEndTemp()
 *
 * @brief   none.
 *
 * @param   none
 *
 * @return  none
 */
real32 CalWorkEndTemp(real32 fWorkEndDegree, real32 fColdEndDegree)
{
  real32 fOutput = 0.0f;
  real32 fa, fb, fdelta;

  if (s_factoryCalCoef.ReservedParm3 > 0.0f)
  {
    fa = s_factoryCalCoef.CalCoef_a;
    fb = s_factoryCalCoef.CalCoef_b;
    fdelta = s_factoryCalCoef.CalCoef_delta;
  }
  else
  {
    fa = 0.0f;
    fb = 1.0f;
    fdelta = 0.0f;
  }
  
  fOutput = fa * fColdEndDegree + fb * fWorkEndDegree + fdelta;

  return (fOutput);
}


/*********************************************************************
 * @fn      GenericApp_HandleNetworkStatus
 *
 * @brief  According to EcgSystemStatus to do different thing
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_HandleNetworkStatus( devStates_t GenericApp_NwkStateTemp)
{
  if( GenericApp_NwkStateTemp == DEV_END_DEVICE) //connect to GW
  {
      TemprSystemStatus = TEMPR_ONLINE_IDLE;
      HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                          DEVICE_INFO_SIZE,DEVICE_INFO_ONLINE_IDLE);
  }
  else if( TemprSystemStatus != TEMPR_OFFLINE_IDLE) // Find network -- 1.coordinate lose 2.first connect to coordinate 
  { // 关闭搜索后，可能由于OSAL的timer事件设置，再进入一次ZDO_STATE_CHANGE，上面的判断就是为了排除这种情况
    if ( TemprSystemStatus == TEMPR_ONLINE_MEASURE ) // Online measure status
    {
    }
    
    TemprSystemStatus = TEMPR_FIND_NETWORK;
    HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                      DEVICE_INFO_SIZE,DEVICE_INFO_FIND_NWK);         
  }
    
  HalOledRefreshGram();
  
}


/*********************************************************************
 * @fn      GenericApp_LeaveNetwork
 *
 * @brief   Let device leave network.
 *
 * @param   none
 *
 * @return  none
 */
void GenericApp_LeaveNetwork( void )
{
  NLME_LeaveReq_t leaveReq;

  osal_memset((uint8 *)&leaveReq,0,sizeof(NLME_LeaveReq_t));
  osal_memcpy(leaveReq.extAddr,NLME_GetExtAddr(),Z_EXTADDR_LEN);

  leaveReq.removeChildren = FALSE; // Only false shoule be use.
  leaveReq.rejoin = FALSE;  
  leaveReq.silent = FALSE;

  NLME_LeaveReq( &leaveReq );
}


/*********************************************************************
 * @fn      GenericApp_SyncData
 *
 * @brief   Sync data.同步的状态机设计
 *
 * @param  
 *
 * @return  
 *
 */
void GenericApp_SyncData(void)
{
  ExtFlashStruct_t ExtFlashStruct;
  if(HalExtFlashDataRead(&ExtFlashStruct) == DATA_READ_EFFECTIVE) // 数据有效
  {
    // 显示同步状态
    HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                      DEVICE_INFO_SIZE,DEVICE_INFO_SYNC_DATA);
    
    // 发送数据
    AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                   GENERICAPP_CLUSTERID_TEMPR_RESULT,
                   TEMPR_RESULT_BYTE_PER_PACKET,
                   (uint8 *)&ExtFlashStruct,
                   &GenericApp_TransID,
                   AF_DISCV_ROUTE, AF_DEFAULT_RADIUS );    
    
    // 再次启动事件，1s后
    osal_start_timerEx( GenericApp_TaskID,
                      GENERICAPP_TEMPR_SYNC,
                      GENERICAPP_SEND_SYNC_DATA_TIMEOUT );
  }
  else // 数据无效
  {
    // 切换状态并显示
    TemprSystemStatus = TEMPR_ONLINE_IDLE;
    HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                      DEVICE_INFO_SIZE,DEVICE_INFO_ONLINE_IDLE);
    
    // 发送停止标志
    AF_DataRequest( &GenericApp_DstAddr, &GenericApp_epDesc,
                   GENETICAPP_CLUSTERID_TEMPR_SYNC_OVER,
                   0,
                   NULL,
                   &GenericApp_TransID,
                   AF_DISCV_ROUTE, AF_DEFAULT_RADIUS );
  }
  
  HalOledRefreshGram();
}


/*********************************************************************
 * @fn      HalOledDispStaDurMeas
 *
 * @brief   测量过程显示状态栏
 *
 * @param  
 *
 * @return  
 *
 */
void HalOledDispStaDurMeas(real32 data,TemprSystemStatus_t deviceStatus)
{
  switch(deviceStatus)
  {
    case TEMPR_ONLINE_MEASURE:
    {
      if((data >= 0.0) && (data < 100.0))
          HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                            DEVICE_INFO_SIZE,DEIVCE_INFO_ONLINE_MEASURE);
      else
          HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                            DEVICE_INFO_SIZE,DEVICE_INFO_ERROR);
    }
    break;
    
    case TEMPR_OFFLINE_MEASURE:
    {
      if((data >= 0.0) && (data < 100.0))
          HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                            DEVICE_INFO_SIZE,DEVICE_INFO_OFFLINE_MEASURE);
      else
          HalOledShowString(DEVICE_INFO_X,DEVICE_INFO_Y,
                            DEVICE_INFO_SIZE,DEVICE_INFO_ERROR);
    }
    break;
    
    default:break;
  }
}


/*********************************************************************
 * @fn      HalOledDispStaDurMeas
 *
 * @brief   HalOledDispTempr
 *
 * @param  
 *
 * @return  
 *
 */
void HalOledDispTempr(real32 data)
{
  uint8 t[4];
  uint32 aa;
  aa = (uint32) (data*100);           //保留3位小数
  
  if ((data >= 0.0) && (data < 100.0))
  {
    t[0]= aa/1000 ;        //分别获取各位上的数
    t[1]= aa%1000/100 ;
    t[2]= aa%100/10 ;
    t[3]= aa%10;

    HalOledShowNum(10,8,t[0],1,32); //十位  
    HalOledShowNum(26,8,t[1],1,32); //个位
    HalOledShowChar(42,8,'.',32,1); 
    HalOledShowNum(58,8,t[2],1,32); //.1位
    HalOledShowNum(74,8,t[3],1,32); //.01位
  }
  else
  {
    HalOledShowString(TEMPR_RESULT_X,TEMPR_RESULT_Y,
                    TEMPR_RESULT_SIZE,TEMPR_RESULT_DEFAULT);
  }
}
/*********************************************************************
*********************************************************************/
