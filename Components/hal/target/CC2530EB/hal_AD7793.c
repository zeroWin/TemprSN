/**************************************************************************************************
  Filename:       hal_AD7793.c
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

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
#include "hal_AD7793.h"
#include "hal_spi_user.h"

#if (defined HAL_AD7793) && (HAL_AD7793 == TRUE)
/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
/* control way 
 * communications register(W) + other register(W/R) */
/* AD7793 Registers */
#define AD7793_REG_COMM       0      /* Communications register(WO,8-bit) */
#define AD7793_REG_STAT       0      /* 0x40(R) Status register(RO,8-bit) Power-on/reset:0x88 */
#define AD7793_REG_MODE       1      /* 0x48(R) 0x08(W) Mode register(RW,16-bit) Power-on/reset:0x000A */
#define AD7793_REG_CONF       2      /* 0x50(R) 0x10(W) Configuration register(RW,16-bit) Power-on/reset:0x0710 */
#define AD7793_REG_DATA       3      /* 0x58(R) 0x18(W) Data register(RO,16-/24-bit) Power-on/reset:0x0000(00) */
#define AD7793_REG_ID         4      /* 0x60(R) ID register(RO,8-bit) Power-on/reset:0xXB */
#define AD7793_REG_IO         5      /* 0x68(R) 0x28(W) IO register(RW,8-bit) Power-on/reset:0x00 */
#define AD7793_REG_OFFSET     6      /* 0x70(R) 0x30(W) Offset register(RW,24-bit) Power-on/reset:0x800000 */
#define AD7793_REG_FULLSCALE  7      /* 0x78(R) 0x38(W) 0x Full-Scale register(RW,24-bit) Power-on/reset:0x5XXXX5 */

/* Communications Register Bit Designations (AD7793_REG_COMM) */
#define AD7793_COMM_WEN		(1 << 7) 		/* Write Enable */
#define AD7793_COMM_WRITE	(0 << 6) 	        /* Write Operation */
#define AD7793_COMM_READ        (1 << 6) 		/* Read Operation */
#define AD7793_COMM_ADDR(x)	(((x) & 0x7) << 3)	/* Register Address */
#define AD7793_COMM_CREAD	(1 << 2) 		/* Continuous Read of Data Register */

/* Status Register Bit Designations (AD7793_REG_STAT) */
#define AD7793_STAT_RDY		(1 << 7) /* Ready */
#define AD7793_STAT_ERR		(1 << 6) /* Error (Overrange, Underrange) */
#define AD7793_STAT_CH3		(1 << 2) /* Channel 3 */
#define AD7793_STAT_CH2		(1 << 1) /* Channel 2 */
#define AD7793_STAT_CH1		(1 << 0) /* Channel 1 */

/* Mode Register Bit Designations (AD7793_REG_MODE) */
#define AD7793_MODE_SEL(x)      (((x) & 0x7) << 13)   /* Operation Mode Select */
#define AD7793_MODE_CLKSRC(x)	(((x) & 0x3) << 6)    /* ADC Clock Source Select */
#define AD7793_MODE_RATE(x)	 ((x) & 0xF)          /* Filter Update Rate Select */

/* AD7793_MODE_SEL(x) options */
#define AD7793_MODE_CONT		 0 /* Continuous Conversion Mode */
#define AD7793_MODE_SINGLE		 1 /* Single Conversion Mode */
#define AD7793_MODE_IDLE		 2 /* Idle Mode */
#define AD7793_MODE_PWRDN		 3 /* Power-Down Mode */
#define AD7793_MODE_CAL_INT_ZERO         4 /* Internal Zero-Scale Calibration */
#define AD7793_MODE_CAL_INT_FULL         5 /* Internal Full-Scale Calibration */
#define AD7793_MODE_CAL_SYS_ZERO         6 /* System Zero-Scale Calibration */
#define AD7793_MODE_CAL_SYS_FULL         7 /* System Full-Scale Calibration */

/* AD7793_MODE_CLKSRC(x) options */
#define AD7793_CLK_INT		0 /* Internal 64 kHz Clk not available at the CLK pin */
#define AD7793_CLK_INT_CO	1 /* Internal 64 kHz Clk available at the CLK pin */
#define AD7793_CLK_EXT		2 /* External 64 kHz Clock */
#define AD7793_CLK_EXT_DIV2	3 /* External Clock divided by 2 */

/* AD7793_MODE_RATE(x) options */
/* 技术手册中的抑制指的是该采样速率下对50/60Hz的噪声的抑制 */
#define AD7793_FLRAT_500	0x01 /* filer rate 500Hz */
#define AD7793_FLRAT_250	0x02 /* filer rate 250Hz */
#define AD7793_FLRAT_125	0x03 /* filer rate 125Hz */
#define AD7793_FLRAT_62	        0x04 /* filer rate 62.5Hz */
#define AD7793_FLRAT_50	        0x05 /* filer rate 50Hz */
#define AD7793_FLRAT_41	        0x06 /* filer rate 41.6Hz */
#define AD7793_FLRAT_33	        0x07 /* filer rate 33.3Hz */
#define AD7793_FLRAT_19	        0x08 /* filer rate 19.6Hz */
#define AD7793_FLRAT_16_1	0x09 /* filer rate 16.6Hz */
#define AD7793_FLRAT_16_2       0x0A /* filer rate 16.6Hz */
#define AD7793_FLRAT_12	        0x0B /* filer rate 12.5Hz */
#define AD7793_FLRAT_10     	0x0C /* filer rate 10Hz */
#define AD7793_FLRAT_8	        0x0D /* filer rate 8.33Hz */
#define AD7793_FLRAT_6	        0x0E /* filer rate 6.25Hz */
#define AD7793_FLRAT_4	        0x0F /* filer rate 4.17Hz */

/* Configuration Register Bit Designations (AD7793_REG_CONF) */
#define AD7793_CONF_VBIAS(x)    (((x) & 0x3) << 14) 	/* Bias Voltage Generator Enable */
#define AD7793_CONF_BO_EN	(1 << 13) 		/* Burnout Current Enable */
#define AD7793_CONF_UNIPOLAR    (1 << 12) 		/* Unipolar/Bipolar Enable */
#define AD7793_CONF_BOOST	(1 << 11) 		/* Boost Enable */
#define AD7793_CONF_GAIN(x)	(((x) & 0x7) << 8) 	/* Gain Select */
#define AD7793_CONF_REFSEL(x)   (((x) & 0x1) << 7) 	/* INT/EXT Reference Select */
#define AD7793_CONF_BUF(x)      (((x) & 0x1) << 4) 	/* Buffered Mode Enable */
#define AD7793_CONF_CHAN(x)	((x) & 0x7) 		/* Channel select */

/* AD7793_CONF_VBIAS(x) options */
#define AD7793_VBIAS_DISABLE      0     /* Vbias disable */
#define AD7793_VBIAS_AIN1         1     /* Bias Voltage connected to AIN1(-) */
#define AD7793_VBIAS_AIN2         2     /* Bias Voltage connected to AIN2(-) */
#define AD7793_VBIAS_RESERVED     3     /* Reserved */

/* AD7793_CONF_GAIN(x) options */
#define AD7793_GAIN_1       0
#define AD7793_GAIN_2       1
#define AD7793_GAIN_4       2
#define AD7793_GAIN_8       3
#define AD7793_GAIN_16      4
#define AD7793_GAIN_32      5
#define AD7793_GAIN_64      6
#define AD7793_GAIN_128     7

/* AD7793_CONF_BUF(x) options */
#define AD7793_CONF_BUF_ON  1
#define AD7793_CONF_BUF_OFF  0

/* AD7793_CONF_REFSEL(x) options */
#define AD7793_REFSEL_INT   1	/* Internal Reference Selected. */
#define AD7793_REFSEL_EXT   0	/* External Reference Applied between REFIN(+) and REFIN(-). */

/* AD7793_CONF_CHAN(x) options */
#define AD7793_CH_AIN1P_AIN1M	0 /* AIN1(+) - AIN1(-) */
#define AD7793_CH_AIN2P_AIN2M	1 /* AIN2(+) - AIN2(-) */
#define AD7793_CH_AIN3P_AIN3M	2 /* AIN3(+) - AIN3(-) */
#define AD7793_CH_AIN1M_AIN1M	3 /* AIN1(-) - AIN1(-) */
#define AD7793_CH_TEMP		6 /* Temp Sensor */
#define AD7793_CH_AVDD_MONITOR	7 /* AVDD Monitor */

/* ID Register Bit Designations (AD7793_REG_ID) */
#define AD7793_ID		0xB
#define AD7793_ID_MASK		0xF

/* IO (Excitation Current Sources) Register Bit Designations (AD7793_REG_IO) */
#define AD7793_IEXCDIR(x)	(((x) & 0x3) << 2)
#define AD7793_IEXCEN(x)	(((x) & 0x3) << 0)

/* AD7793_IEXCDIR(x) options*/
#define AD7793_DIR_IEXC1_IOUT1_IEXC2_IOUT2	0  /* IEXC1 connect to IOUT1, IEXC2 connect to IOUT2 */
#define AD7793_DIR_IEXC1_IOUT2_IEXC2_IOUT1	1  /* IEXC1 connect to IOUT2, IEXC2 connect to IOUT1 */
#define AD7793_DIR_IEXC1_IEXC2_IOUT1		2  /* Both current sources IEXC1,2 connect to IOUT1  */
#define AD7793_DIR_IEXC1_IEXC2_IOUT2		3  /* Both current sources IEXC1,2 connect to IOUT2 */

/* AD7793_IEXCEN(x) options*/
#define AD7793_EN_IXCEN_DISABLE         0  /* Excitation Current Disable */
#define AD7793_EN_IXCEN_10uA		1  /* Excitation Current 10uA */
#define AD7793_EN_IXCEN_210uA		2  /* Excitation Current 210uA */
#define AD7793_EN_IXCEN_1mA		3  /* Excitation Current 1mA */

/* Dummy Byte */
#define DUMMY_BYTE     0xFF

/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/

/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/
void AD7793_Init_AIN1(void);
void AD7793_Init_AIN2(void);
void AD7793_Init_AIN3(void);
void AD7793_set_ModeReg(AD7793Rate_t OutUpdateRate);

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/

 
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/

/**************************************************************************************************
 * @fn      HalAD7793Init
 *
 * @brief   Initialize AD7793.
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalAD7793Init(void)
{
  HalSpiAD7793Disable();
}


/*********************************************************************
 * @fn      AD7793_AIN1_config_one()
 *
 * @brief   to config the first channel (AIN1) for sampling and reading with 
 *          the mode of one sample reading.
 *
 * @param   none
 *
 * @return  none
 */
void AD7793_AIN1_config_one(AD7793Rate_t OutUpdateRate)
{
  HalSpiAD7793Enable();       // AD7793 enable
  AD7793_Init_AIN1();
     
  //setup mode register
  AD7793_set_ModeReg(OutUpdateRate);
  
}


/*********************************************************************
 * @fn      AD7793_Init_AIN1()
 *
 * @brief   to initialize AIN1 (1st input channel) of AD7793, which is
 *          connected to thermo-coupler.
 *
 * @param   none
 *
 * @return  none
 */
void AD7793_Init_AIN1(void) //for thermo-coupler
{
  /* setup configuration register */
  // select configuration register
  HalSpiWriteReadByte(AD7793_COMM_WRITE | AD7793_COMM_ADDR(AD7793_REG_CONF)); //0001 0000 = 0x10
  
  //When the in-amp is active (gain ≥ 4), the common-mode voltage
  //(AIN(+) + AIN(–))/2 must be greater than or equal to 0.5 V.
    
  //Bias Voltage Generator : AIN1(-) 01
  //Burnout Current Enable Bit: False  0
  //Unipolar/Bipolar Bit: Bipolar  0
  //BOOST: Flase 0
  //Gain Select Bits: 128  111 
  //Reference Select Bit: Internal Reference applied    1
  //Buffered mode  00 1
  //Channel Select bits: AIN1(+)-AIN1(-) 0 000
  // 0x4790 = 0100 0111 1001 0000
  HalSpiWriteReadByte((AD7793_CONF_VBIAS(AD7793_VBIAS_AIN1) | AD7793_CONF_GAIN(AD7793_GAIN_128)) >> 8);
  HalSpiWriteReadByte(AD7793_CONF_REFSEL(AD7793_REFSEL_INT) | AD7793_CONF_BUF(AD7793_CONF_BUF_ON) | AD7793_CONF_CHAN(AD7793_CH_AIN1P_AIN1M));
    
  /* setup IO register */
  // select IO register
  HalSpiWriteReadByte(AD7793_COMM_WRITE | AD7793_COMM_ADDR(AD7793_REG_IO)); //0010 1000 = 0x28

  // Direction of Current Sources Select Bits: IEXC1 to IOUT1, IEXC2 to IOUT2 
  // Current Source Value: disable
  // 0000 0000 = 0x00
  HalSpiWriteReadByte(AD7793_IEXCDIR(AD7793_DIR_IEXC1_IOUT1_IEXC2_IOUT2) | AD7793_IEXCEN(AD7793_EN_IXCEN_DISABLE)); 
}


/*********************************************************************
 * @fn      AD7793_AIN1_fetch_one()
 *
 * @brief   to read one sample from AD7793.
 *
 * @param   none
 *
 * @return  none
 */
float AD7793_AIN1_fetch_one(void)
{
  uint8  data;
  uint8  idata;

  uint32 reg_num = 0x0;
  float fSample = 0.0f;
  
  //select data register
  //0101 1000 = 0x58
  HalSpiWriteReadByte(AD7793_COMM_READ | AD7793_COMM_ADDR(AD7793_REG_DATA));

  //read 3 times to get data
  for(idata=0;idata<3;idata++)
  {
    reg_num = reg_num << 8;
    data    = HalSpiWriteReadByte(DUMMY_BYTE);
    reg_num |= (uint32)data;
  }
          
  
  HalSpiAD7793Disable();       // AD7793 disable
          
  fSample = (((float)reg_num)/8388608.0 - 1.0) * 1.17/128;
  
  return fSample;
}


/*********************************************************************
 * @fn      AD7793_AIN2_config_one()
 *
 * @brief   to config the second channel (AIN2) for sampling and reading with 
 *          the mode of one sample reading.
 *
 * @param   none
 *
 * @return  none
 */
void AD7793_AIN2_config_one(AD7793Rate_t OutUpdateRate)
{
  HalSpiAD7793Enable();       // AD7793 enable
  AD7793_Init_AIN2();
  
  //setup mode register
  AD7793_set_ModeReg(OutUpdateRate);
}


/*********************************************************************
 * @fn      AD7793_Init_AIN2()
 *
 * @brief   to initialize AIN2 (1st input channel) of AD7793, which is
 *          connected to thermo-coupler.
 *
 * @param   none
 *
 * @return  none
 */
void AD7793_Init_AIN2(void) //for thermo-coupler
{
  /* setup configuration register */
  // select configuration register
  HalSpiWriteReadByte(AD7793_COMM_WRITE | AD7793_COMM_ADDR(AD7793_REG_CONF)); //0001 0000 = 0x10
  
  //When the in-amp is active (gain ≥ 4), the common-mode voltage
  //(AIN(+) + AIN(–))/2 must be greater than or equal to 0.5 V.
    
  //Bias Voltage Generator disable: AIN2(-) 00
  //Burnout Current Enable Bit: False  0
  //Unipolar/Bipolar Bit: Unipolar  1
  //BOOST: Flase 0
  //Gain Select Bits: 4  010 
  //Reference Select Bit: Internal Reference applied    1
  //Buffered mode  00 1
  //Channel Select bits: AIN2(+)-AIN2(-) 0 001
  // 0x1291 = 0001 0010 1001 0001
  HalSpiWriteReadByte((AD7793_CONF_VBIAS(AD7793_VBIAS_DISABLE) | AD7793_CONF_UNIPOLAR | AD7793_CONF_GAIN(AD7793_GAIN_4)) >> 8);
  HalSpiWriteReadByte(AD7793_CONF_REFSEL(AD7793_REFSEL_INT) | AD7793_CONF_BUF(AD7793_CONF_BUF_ON) | AD7793_CONF_CHAN(AD7793_CH_AIN2P_AIN2M));
    
  /* setup IO register */
  // select IO register
  HalSpiWriteReadByte(AD7793_COMM_WRITE | AD7793_COMM_ADDR(AD7793_REG_IO)); //0010 1000 = 0x28

  // Direction of Current Sources Select Bits: IEXC1 to IOUT1, IEXC2 to IOUT2 
  // Current Source Value: 0.21mA
  // 0000 0010 = 0x02
  HalSpiWriteReadByte(AD7793_IEXCDIR(AD7793_DIR_IEXC1_IOUT1_IEXC2_IOUT2) | AD7793_IEXCEN(AD7793_EN_IXCEN_210uA)); 
}


/*********************************************************************
 * @fn      AD7793_AIN2_fetch_one()
 *
 * @brief   to read one sample from AD7793.
 *
 * @param   none
 *
 * @return  none
 */
float AD7793_AIN2_fetch_one(void)
{
  uint8  data;
  uint8  idata;

  uint32 reg_num = 0x0;
  float fSample = 0.0f;
  
  //select data register
  //0101 1000 = 0x58
  HalSpiWriteReadByte(AD7793_COMM_READ | AD7793_COMM_ADDR(AD7793_REG_DATA));

  //read 3 times to get data
  for(idata=0;idata<3;idata++)
  {
    reg_num = reg_num << 8;
    data    = HalSpiWriteReadByte(DUMMY_BYTE);
    reg_num |= (uint32)data;
  }
          
  
  HalSpiAD7793Disable();       // AD7793 disable
          
  fSample = (((float)reg_num) * 1.17/16777215.0)/4;
  
  return fSample;
}


/*********************************************************************
 * @fn      AD7793_AIN3_config_one()
 *
 * @brief   to config the third channel (AIN3) for sampling and reading with 
 *          the mode of one sample reading.
 *
 * @param   none
 *
 * @return  none
 */
void AD7793_AIN3_config_one(AD7793Rate_t OutUpdateRate)
{
  HalSpiAD7793Enable();       // AD7793 enable
  AD7793_Init_AIN3();
  
  //setup mode register
  AD7793_set_ModeReg(OutUpdateRate);
}


/*********************************************************************
 * @fn      AD7793_Init_AIN3()
 *
 * @brief   to initialize AIN3 (1st input channel) of AD7793, which is
 *          connected to thermo-coupler.
 *
 * @param   none
 *
 * @return  none
 */
void AD7793_Init_AIN3(void) //for thermo-coupler
{
  /* setup configuration register */
  // select configuration register
  HalSpiWriteReadByte(AD7793_COMM_WRITE | AD7793_COMM_ADDR(AD7793_REG_CONF)); //0001 0000 = 0x10
  
  //When the in-amp is active (gain ≥ 4), the common-mode voltage
  //(AIN(+) + AIN(–))/2 must be greater than or equal to 0.5 V.
    
  //Bias Voltage Generator disable:    00
  //Burnout Current Enable Bit: False  0
  //Unipolar/Bipolar Bit: Unipolar  1
  //BOOST: Flase   0
  //Gain Select Bits: 0      000
  //Reference Select Bit: Internal Reference applied  1
  //uBuffered mode 00 1
  //Channel Select bits: AIN3(+)-AIN3(-)  0 010 
  // 0x1092 = 0001 0000 1001 0010
  HalSpiWriteReadByte((AD7793_CONF_VBIAS(AD7793_VBIAS_DISABLE) | AD7793_CONF_UNIPOLAR | AD7793_CONF_GAIN(AD7793_GAIN_1)) >> 8);
  HalSpiWriteReadByte(AD7793_CONF_REFSEL(AD7793_REFSEL_INT) | AD7793_CONF_BUF(AD7793_CONF_BUF_ON) | AD7793_CONF_CHAN(AD7793_CH_AIN3P_AIN3M));
    
  /* setup IO register */
  // select IO register
  HalSpiWriteReadByte(AD7793_COMM_WRITE | AD7793_COMM_ADDR(AD7793_REG_IO)); //0010 1000 = 0x28

  // Direction of Current Sources Select Bits: IEXC1 to IOUT1, IEXC2 to IOUT2 
  // Current Source Value: 0.21mA
  // 0000 0010 = 0x02
  HalSpiWriteReadByte(AD7793_IEXCDIR(AD7793_DIR_IEXC1_IOUT1_IEXC2_IOUT2) | AD7793_IEXCEN(AD7793_EN_IXCEN_210uA)); 
}


/*********************************************************************
 * @fn      AD7793_AIN3_fetch_one()
 *
 * @brief   to read one sample from AD7793.
 *
 * @param   none
 *
 * @return  none
 */
float AD7793_AIN3_fetch_one(void)
{
  uint8  data;
  uint8  idata;

  uint32 reg_num = 0x0;
  float fSample = 0.0f;
  
  //select data register
  //0101 1000 = 0x58
  HalSpiWriteReadByte(AD7793_COMM_READ | AD7793_COMM_ADDR(AD7793_REG_DATA));

  //read 3 times to get data
  for(idata=0;idata<3;idata++)
  {
    reg_num = reg_num << 8;
    data    = HalSpiWriteReadByte(DUMMY_BYTE);
    reg_num |= (uint32)data;
  }
          
  
  HalSpiAD7793Disable();       // AD7793 disable
          
  fSample = (((float)reg_num) * 1.17/16777215.0)/1;
  
  return fSample;
}


/*********************************************************************
 * @fn      AD7793_set_ModeReg()
 *
 * @brief   to configure AD7793 mode register.
 *
 * @param   none
 *
 * @return  none
 */
void AD7793_set_ModeReg(AD7793Rate_t OutUpdateRate)
{
  /* setup mode register */
  // Enable write to CR, next operation is to write and select mode register
  // 0000 1000 = 0x08
  HalSpiWriteReadByte(AD7793_COMM_WRITE | AD7793_COMM_ADDR(AD7793_REG_MODE));
  
  // Mode select: Single Conversion Mode
  // Clock select: Internal 64KHz Clock, not available at the CLK pin.
  // Filter update rate select: fadc=4.17Hz,Rejection 65dB
  switch(OutUpdateRate)
   {
   case AD7793_RATE_62dot0:
      // fadc = 62.0Hz;
      // H8:0010 0000=0x20 L8:0000 0100=0x04
      HalSpiWriteReadByte(AD7793_MODE_SEL(AD7793_MODE_SINGLE) >> 8);
      HalSpiWriteReadByte(AD7793_MODE_RATE(AD7793_FLRAT_62));
      break;
    
   case AD7793_RATE_33dot2:
      // fadc = 33.2Hz;
      // H8:0010 0000=0x20 L8:0000 0111=0x07
      HalSpiWriteReadByte(AD7793_MODE_SEL(AD7793_MODE_SINGLE) >> 8);
      HalSpiWriteReadByte(AD7793_MODE_RATE(AD7793_FLRAT_33));
      break;

   case AD7793_RATE_16dot7:
      // fadc = 16.7Hz;
      //H8:0010 0000=0x20 L8:0000 1010=0x0A
      HalSpiWriteReadByte(AD7793_MODE_SEL(AD7793_MODE_SINGLE) >> 8);
      HalSpiWriteReadByte(AD7793_MODE_RATE(AD7793_FLRAT_16_2));
      break;

   case AD7793_RATE_8dot33:
      // fadc = 8.33Hz;
      // H8:0010 0000=0x20 L8:0000 1101=0x0D
      HalSpiWriteReadByte(AD7793_MODE_SEL(AD7793_MODE_SINGLE) >> 8);
      HalSpiWriteReadByte(AD7793_MODE_RATE(AD7793_FLRAT_8));
      break;
   
   case AD7793_RATE_4dot17:
     // fadc = 4.17Hz;
     // H8:0010 0000=0x20 L8:0000 1111=0x0F
      HalSpiWriteReadByte(AD7793_MODE_SEL(AD7793_MODE_SINGLE) >> 8);
      HalSpiWriteReadByte(AD7793_MODE_RATE(AD7793_FLRAT_4));
      break;
       
   default:
      break;
   }
}

bool AD7793_IsReadyToFetch(void)
{
  uint8 statusData = 0;
  /* Select status register*/
  // 0100 0000=0x40
  HalSpiWriteReadByte(AD7793_COMM_READ | AD7793_COMM_ADDR(AD7793_REG_STAT));
  
  statusData = HalSpiWriteReadByte(DUMMY_BYTE);
  return (( (statusData&AD7793_STAT_RDY) == AD7793_STAT_RDY)?FALSE:TRUE);
}
#else
void HalAD7793Init(void);

#endif /* HAL_AD7793 */