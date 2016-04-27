/**************************************************************************************************
  Filename:       measTempr.c
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

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
#include "measTempr.h"
#include "math.h"

/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
const real32 f_real32_MINIMUM = -999999.0f;
const real32 f_real32_MAXIMUM =  999999.0f;
const real32 f_epsilon = 0.00000001;

/* cold end temperature measurement equation:
 *
 *       V_PT     R0_PT
 *      ------ = ------- * (1 + a*t + b*t^2)
 *       V_REF    R_REF
 *
 *      to define Volt_ratio = V_PT/V_REF;
 *                R0_ratio   = R0_PT/R_REF;
 *
 */
const real32 R0_ratio_default =  0.20006;           // 1000.3/5000 = 0.20006;
const real32 PT1000_b_coef    = -0.000000584884765; // derived from PT1000 ref table;
const real32 PT1000_a_coef    =  0.003909234732360; // derived from PT1000 ref table;

/* thermo-couple temperature measurement equation:
 * 
 * V_work = a*t_w^2 + b*t_w + c;
 * V_cold = a*t_c^2 + b*t_c + c;
 *
 * (a*t_w^2 + b*t_w) - (a*t_c^2 + b*t_c) - (V_work - V_cold) = 0;
 * 
 *  to redefine c1 = - (a*t_c^2 + b*t_c) - (V_work - V_cold);
 *
 */
const real32 thermo_a_default = 0.000041922545506; // derived from thermo-coupler ref table.
const real32 thermo_b_default = 0.038603329738281; // derived from thermo-coupler ref table.

/* the threshold utilized to determine the step change of measured temperature */
const real32 fSTEP_CHANGE_THRESHOLD = 1.9f; // 2.9 degree.


const real32 fSTABLE_TEMPR_GRADIENT_THRESHOLD = 0.005; // 0.0005; 
/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/


/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/

/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/
static bool   s_isStepChange  = FALSE;
static uint16 s_tmp_idx = 0;
static uint16 s_stepChangePos = 0;
static uint16 s_stepChangeNum = 0;
static real32 s_fPrevGradient = 0.0f;
static real32 s_fCurrGradient = 0.0f;
static real32 s_fOutputDegree = -1.0f;

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void measColdEndTemperature(measResult_t *pMeasRlt);
real32 LSEOfTenWorkEndT(measResult_t *pMeasRlt, real32 *pfResi);

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/
/*********************************************************************
 * @fn      measWorkEndTemperature()
 *
 * @brief   to measure the temperature of work end.
 *
 * @param   none
 *
 * @return 
 *
 * @note    1. the result is stored into the struct pointed by the input pointer.
 *          2. only positive value of temperature is valid.
 *          3. Here does not check it is human body temperation (35~42 degree)!
 */
bool measWorkEndTemperature(measResult_t *pMeasRlt)
{
  real32 fColdEndTmpr = 0.0f;
  real32 fWorkEndTmpr = 0.0f;

  bool isValid = FALSE;  
  
  real32 fa1 = 0.0f;
  real32 fb1 = 0.0f;
  real32 fc1 = 0.0f;

  measColdEndTemperature(pMeasRlt);
  fColdEndTmpr = pMeasRlt->fColdEndDegree;

  if (fColdEndTmpr < 0.0f)
  {
    isValid = FALSE;
    return (isValid);
  }

  // to calculate the work end temperature;
  // a1*t_w^2 + b1*t_w + c1 = 0;
  fa1 = thermo_a_default;
  fb1 = thermo_b_default;
  fc1 = -(thermo_a_default * fColdEndTmpr * fColdEndTmpr 
          + thermo_b_default * fColdEndTmpr) 
        // mili-volts is utilized in thermo-couple reference table.
        - 1000*pMeasRlt->fThermoVolt;

  real32 fSqrt = sqrt(fb1*fb1 - 4*fa1 * fc1);

  fWorkEndTmpr = (-1*fb1 + fSqrt) / (2*fa1);
  pMeasRlt->fWorkEndDegree = fWorkEndTmpr;

  if (fWorkEndTmpr < 0.0f)
    isValid = FALSE;
  else
    isValid = TRUE;
  
  return (isValid);
}


/*********************************************************************
 * @fn      measColdEndTemperature()
 *
 * @brief   to measure the temperature of cold end of thermo-couple.
 *
 * @param   none
 *
 * @return
 * 
 * @note    the result is stored into the struct pointed by the input pointer.
 */
void measColdEndTemperature(measResult_t *pMeasRlt)
{
  real32 fDegree = -1.0f;
  real32 fVPt  = pMeasRlt->fPtVolt;
  real32 fVRef = pMeasRlt->fRefVolt;

  // without calibration, have to utilize default value of R0_ratio.  
  // a1*x^2 + b1*x + c1 = 0;  
  real32 fTmp = R0_ratio_default * fVRef;
  real32 fa1 = PT1000_b_coef * fTmp;
  real32 fb1 = PT1000_a_coef * fTmp;
  real32 fc1 = 1*fTmp - fVPt;

  real32 fSqrt = sqrt(fb1*fb1 - 4*fa1 * fc1);

  fDegree = (-1*fb1 + fSqrt) / (2*fa1);
  pMeasRlt->fColdEndDegree = fDegree;

  return;
}


/*********************************************************************
 * @fn      CheckMeasComplete()
 *
 * @brief   to measure the temperature of cold end of thermo-couple.
 *
 * @param   none
 *
 * @return  none
 * 
 * @NOTE    the caller need to make sure curr_result_idx < MEAS_TEMPR_LOOP_COUNT_MAX.
 */
bool CheckMeasComplete(measResult_t *pMeasRlt, 
                       uint16 curr_result_idx, 
                       real32 *pfOutputDegree, // hot-end temperature degree.
                       real32 *pfColdEndDegree)
{
  bool isComplete = FALSE;

  real32 fSignP, fSignC;
  real32 fChange = 0.0f;
  real32 fResi   = 0.0f;
  
  *pfColdEndDegree = 0.0f;
  
  // to initialize all local static variabels.
  if (curr_result_idx == 0)
  {
    // only when s_isStepChange is FALSE, it will start temp step change
    // detection. so to set it to TRUE can disable detection;
    #if (defined(SLOW_MEAS) && (SLOW_MEAS == TRUE)) || ((defined(GO_TO_STABLE) && (GO_TO_STABLE == TRUE)))
    s_isStepChange  = TRUE; // FALSE;// to disable temp step change detection;
    #else
    s_isStepChange = FALSE;
    #endif
    
    s_tmp_idx = 0;
    s_stepChangePos = 0;
    s_stepChangeNum = 0;
    s_fPrevGradient = f_real32_MINIMUM;
    s_fCurrGradient = f_real32_MINIMUM;
    s_fOutputDegree = -1.0f;
  }

  // to identify the step change.
  if (s_isStepChange == FALSE)
  {
    // result interval is 3.
    if (curr_result_idx >= 3)
    {
      fChange = pMeasRlt[curr_result_idx].fWorkEndDegree 
                - pMeasRlt[curr_result_idx-3].fWorkEndDegree;
    }
    else
    {
      fChange = 0.0f;
    }

    if (fabs(fChange) > fSTEP_CHANGE_THRESHOLD)
      s_stepChangeNum++;
    else
      s_stepChangeNum = 0;

    // continuous 3 results' step change is over threshold, it shall be a real 
    // step change.
    if (s_stepChangeNum >= 3)
    {
      s_stepChangePos = curr_result_idx - s_stepChangeNum;
      
      s_isStepChange  = TRUE;
      s_fPrevGradient = f_real32_MINIMUM;
      s_fCurrGradient = f_real32_MINIMUM;
      s_fOutputDegree = -1.0f;
    }
  }

  // to identify the stable temperature.
  s_tmp_idx = curr_result_idx - s_stepChangePos;
  if ((s_tmp_idx >= 10) && ((s_tmp_idx % 5) == 0))
  {
    s_fPrevGradient = s_fCurrGradient;
    
    // least-squares estimation of 10 continuous results of 
    // work end temperature.
    s_fCurrGradient = LSEOfTenWorkEndT(&(pMeasRlt[curr_result_idx-9]), &fResi);

    fSignP = (s_fPrevGradient>0.0f)?1.0f:-1.0f;
    fSignC = (s_fCurrGradient>0.0f)?1.0f:-1.0f;
    
    // if (fabs(s_fCurrGradient) < fSTABLE_TEMPR_GRADIENT_THRESHOLD)
    if ((fSignC * s_fCurrGradient) < fSTABLE_TEMPR_GRADIENT_THRESHOLD)
    {
      // temperature is stable.
      s_fOutputDegree = fResi + s_fCurrGradient * 5;

      if (s_isStepChange == TRUE)
        isComplete = TRUE;
    }
    else if (((fSignP * s_fPrevGradient) < fSTEP_CHANGE_THRESHOLD) && (fSignP * fSignC < 0.0f))
    {
      s_fOutputDegree = fResi + s_fCurrGradient * 2;

      if (s_isStepChange == TRUE)
        isComplete = TRUE;
    }
    else
    {
      // if no stable result, to output the last workEndDegree as result;
      s_fOutputDegree = pMeasRlt[curr_result_idx].fWorkEndDegree; 
    }
  }

  // for debug
  pMeasRlt[curr_result_idx].reserved = s_fOutputDegree;

  // output
  *pfColdEndDegree = pMeasRlt[curr_result_idx].fColdEndDegree;
  *pfOutputDegree  = s_fOutputDegree;
  
  return (isComplete);
}


/*********************************************************************
 * @fn      CheckMeasComplete()
 *
 * @brief   to apply LSE (least square estimation) over 10 results of 
 *          work end temperature.
 *
 * @param   none
 *
 * @return  the gradient of LSE.
 * 
 * @NOTE    the caller need to avoid overstep the boundary of array.
 */
real32 LSEOfTenWorkEndT(measResult_t *pMeasRlt, real32 *pfResi)
{
  const uint16 N_num = 10;
  uint16 idx = 0;

  /*
   *   y = alpha + beta * x; (where x = 0, 1, ..., 9;)
   */
  real32 falpha = -1.0f;
  real32 fbeta  =  0.0f;
  real32 fxsum, fysum, fxysum; // , fxxsum;
  // real32 fx;
  real32 fy;
  real32 fbeta_coef, falpha_coef;

  fxsum  =  45.0f; // sum (0,...,9)
  // fxxsum = 285.0f; // sum (x.x)
  
  // fbeta_coef = 1/(N_num*fxxsum - fxsum*fxsum)
  fbeta_coef  = 0.001212121212121f;
  falpha_coef = 0.1f;


  fysum  = 0.0f;
  fxysum = 0.0f;

  for (idx = 0; idx < N_num; idx ++)
  {
    // fx = (real32)idx;
    fy = pMeasRlt[idx].fWorkEndDegree;

    fysum  += fy;
    fxysum += idx * fy;
  }

  // fbeta  = (N_num*fxysum - fxsum*fysum)/(N_num*fxxsum - fxsum*fxsum);
  // falpha = (fysum - fbeta*fxsum)/(real32)N_num;
  fbeta  = (N_num*fxysum - fxsum*fysum) * fbeta_coef;
  falpha = (fysum - fbeta*fxsum) * falpha_coef;
  

  *pfResi = falpha;
  
  return (fbeta);
}


