/**************************************************************************************************
  Filename:       hal_oled.c
  Revised:        $Date: 2016-03-12 19:37:16 +0800 (Sat, 12 Mar 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to the OLED Service.


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
#include "OLED_FRONT.h"
#include "hal_board.h"
#include "hal_oled.h"

#if (defined HAL_OLED) && (HAL_OLED == TRUE)
/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
/* OLED RST is at P1.3 */
#define OLED_RST_PORT 1
#define OLED_RST_PIN  3

/* OLED SCL is at P1.2 */
#define OLED_SCL_PORT 1
#define OLED_SCL_PIN  2

/* OLED SDA is at P1.1 */
#define OLED_SDA_PORT 1
#define OLED_SDA_PIN  1

/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/
/* ----------- RST's ---------- */
#define RST_H_OUT        MCU_IO_OUTPUT(OLED_RST_PORT, OLED_RST_PIN, 1)
#define RST_H            MCU_IO_SET_HIGH(OLED_RST_PORT, OLED_RST_PIN)
#define RST_L            MCU_IO_SET_LOW(OLED_RST_PORT, OLED_RST_PIN)

/* ----------- SCL's ---------- */
#define SCL_H_OUT        MCU_IO_OUTPUT(OLED_SCL_PORT, OLED_SCL_PIN, 1)
#define SCL_H            MCU_IO_SET_HIGH(OLED_SCL_PORT, OLED_SCL_PIN)
#define SCL_L            MCU_IO_SET_LOW(OLED_SCL_PORT, OLED_SCL_PIN)

/* ----------- SDA's ---------- */
#define SDA_H_OUT        MCU_IO_OUTPUT(OLED_SDA_PORT, OLED_SDA_PIN, 1)
#define SDA_H            MCU_IO_SET_HIGH(OLED_SDA_PORT, OLED_SDA_PIN)
#define SDA_L            MCU_IO_SET_LOW(OLED_SDA_PORT, OLED_SDA_PIN)

/* ----------- Delay's ---------- */
#define OLED_OP_DELAY  halMcuWaitUs(1)
/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/

/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void I2C_Start(void);
void I2C_Stop(void);
void I2C_O(unsigned char mcmd);
void I2C_Ack(void);
void writec(unsigned char x);
void writed(unsigned char d);
void halMcuWaitUs(uint16 microSecs);
uint32 oled_pow(uint8 m,uint8 n);

void Set_xy(uint8 x,uint8 y);
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/


/**************************************************************************************************
 * @fn      HalOledInit
 *
 * @brief   Initilize OLED Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalOledInit(void)
{
  SCL_H_OUT;
  SDA_H_OUT;
  RST_H_OUT;
  
  halMcuWaitUs(5);
  RST_H;   
  halMcuWaitUs(10);
  RST_L;   
  halMcuWaitUs(1000);//20000 = 35ms ,so 1 unit = 1.75us
  RST_H;
  
  /* 基本设置  参考UG-28HSWEG02手册 p10 */
  writec(0xAE);    //display off

  writec(0xd5);    //contract control
  writec(0x80);

  writec(0xA8); 
  writec(0x3F); 

  writec(0xD3);
  writec(0x00);

  writec(0x40);
  
  writec(0x8D);
  writec(0x14);
  
  writec(0xA1);   //A1h, X[0]=1b: column address 127 is mapped to SEG0

  writec(0xC8);   //C8h, X[3]=1b: remapped mode. Scan from COM[31] to COM0

  writec(0xDA);
  writec(0x12);

  writec(0x81);
  writec(0x8F);

  writec(0xD9);
  writec(0xF1);

  writec(0xDB);
  writec(0x40);

  writec(0xA4);
  writec(0xA6);
  
  writec(0xAF);    //display on
  HalOledClear();
}

/**************************************************************************************************
 * @fn      Set_xy
 *
 * @brief   Set write local
 *
 * @param   
 *
 * @return
 **************************************************************************************************/
void Set_xy(uint8 x,uint8 y)
{
    writec(y|0xb0);
    writec(x&0x0f);          //低四位
    writec(((x&0xf0)>>4)|0x10); //得到高四位

}

void HalOledShowChar(uint8 x,uint8 y,uint8 chr,uint8 size,uint8 mode)
{
  uint8 temp,t;
  uint8 x0=x;
  uint8 y0=y/8;
  chr=chr-' ';//得到偏移后的值
  if(size == 64)
  {
    // 设置开始位置
    Set_xy(x0,y0);
    // 写入数据
    I2C_Start();
    I2C_O(0x78);
    I2C_Ack();
    I2C_O(0x40);
    I2C_Ack();  
    
    for(t=0;t<16;++t)
    {
      if(chr == 0)temp = oled_testing_point_space[t]; //调用16X8大小的"-"
      else temp = oled_testing_point[t]; //调用16X8大小的"-"
      I2C_O(temp);
      I2C_Ack();
      ++x0;
      if(x0-x == 16)  // 该行写完
      {
        I2C_Stop();
        x0 = x;
        if(t != 15)
        {
          ++y0;
          // 设置开始位置
          Set_xy(x0,y0);
          I2C_Start();
          I2C_O(0x78);
          I2C_Ack();
          I2C_O(0x40);
          I2C_Ack();            
        }
      }
    }
  }
  else if(size == 32)
  {
    // 设置开始位置
    Set_xy(x0,y0);
    // 写入数据
    I2C_Start();
    I2C_O(0x78);
    I2C_Ack();
    I2C_O(0x40);
    I2C_Ack();  
    
    for(t=0;t<112;++t)
    {
      temp = oled_asc3_3216[chr][t]; //调用16X56字体
      I2C_O(temp);
      I2C_Ack();
      ++x0;
      if(x0-x == 16)  // 该行写完
      {
        I2C_Stop();
        x0 = x;
        if(t != 111)
        {
          ++y0;
          // 设置开始位置
          Set_xy(x0,y0);
          I2C_Start();
          I2C_O(0x78);
          I2C_Ack();
          I2C_O(0x40);
          I2C_Ack();            
        }
      }
    }   
  }
  else if(size == 12)
  {
    // 设置开始位置
    Set_xy(x0,y0);
    // 写入数据
    I2C_Start();
    I2C_O(0x78);
    I2C_Ack();
    I2C_O(0x40);
    I2C_Ack();  
    
    for(t=0;t<size;++t)
    {
      temp = oled_asc2_1206[chr][t]; //调用1206字体
      I2C_O(temp);
      I2C_Ack();
      ++x0;
      if(x0-x == size/2)  // 该行写完
      {
        I2C_Stop();
        x0 = x;
        if(t != size-1)
        {
          ++y0;
          // 设置开始位置
          Set_xy(x0,y0);
          I2C_Start();
          I2C_O(0x78);
          I2C_Ack();
          I2C_O(0x40);
          I2C_Ack();            
        }
      }
    }
  }
  
}


/**************************************************************************************************
 * @fn      HalOledShowNum
 *
 * @brief   Show a Num on OLED
 *
 * @param   size:12/16/32
 *
 * @return  None
 **************************************************************************************************/
void HalOledShowNum(uint8 x,uint8 y,uint32 num,uint8 len,uint8 size)
{
  uint8 t,temp;
  uint8 enshow=0;						   
  for(t=0;t<len;t++)
  {
    temp=(num/oled_pow(10,len-t-1))%10;
    if(enshow==0&&t<(len-1))
    {
      if(temp==0)
      {
        HalOledShowChar(x+(size/2)*t,y,' ',size,1);
        continue;
      }
      else enshow=1; 
    }
    HalOledShowChar(x+(size/2)*t,y,temp+'0',size,1); 
  }  
}


/**************************************************************************************************
 * @fn      HalOledShowString
 *
 * @brief   Show String on OLED
 *
 * @param   size:12/16/32
 *
 * @return  None
 **************************************************************************************************/
void HalOledShowString(uint8 x,uint8 y,uint8 size,const uint8 *p)
{
#define MAX_CHAR_POSX 122
#define MAX_CHAR_POSY 58    
  while(*p!='\0')
  {       
    if(x>MAX_CHAR_POSX){x=0;y+=16;}
    if(y>MAX_CHAR_POSY){y=x=0;HalOledClear();}
    HalOledShowChar(x,y,*p,size,1);	 
    if(size == 64) x += 20;
    else x+=size/2;
    p++;
  } 
} 


/**************************************************************************************************
 * @fn      halMcuWaitUs
 *
 * @brief   wait for x us. @ 32MHz MCU clock it takes 32 "nop"s for 1 us delay.
 *
 * @param   x us. range[0-65536]
 *
 * @return  None
 **************************************************************************************************/
void halMcuWaitUs(uint16 microSecs)
{
  while(microSecs--)
  {
    /* 32 NOPs == 1 usecs */
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop");
  }
}

/**************************************************************************************************
 * @fn      HalOledClear
 *
 * @brief   Clear the OLED
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalOledClear(void) 
{
  uint8 x,y;
  for(y = 0; y < 8; ++y)
  {
    // 设置开始位置
    Set_xy(x,y);
    // 写入数据
    I2C_Start();
    I2C_O(0x78);
    I2C_Ack();
    I2C_O(0x40);
    I2C_Ack();  
    
    for(x = 0; x < 128; ++x)
    {
      I2C_O(0x00);
      I2C_Ack();	
    }
    I2C_Stop();
    x = 0;
  }
}


/**************************************************************************************************
 * @fn      I2C_Start
 *
 * @brief   I2C start singal
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void I2C_Start(void)
{
  SDA_L;
  OLED_OP_DELAY;
  SCL_H;
  OLED_OP_DELAY;
  SCL_L;
  OLED_OP_DELAY;  
}


/**************************************************************************************************
 * @fn      I2C_Stop
 *
 * @brief   I2C Stop singal
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void I2C_Stop(void)
{
  SCL_H;
  
  SDA_L;
  OLED_OP_DELAY;
  SDA_H;
  OLED_OP_DELAY;  
}


/**************************************************************************************************
 * @fn      I2C_O
 *
 * @brief   I2C output a byte
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void I2C_O(unsigned char mcmd)
{
  unsigned char length = 8;		// Send Command

  while(length--)
  {
    if(mcmd & 0x80)
    {
      SDA_H;
    }
    else
    {
      SDA_L;
    }
    OLED_OP_DELAY;
    SCL_H;
    OLED_OP_DELAY;
    SCL_L;	
    OLED_OP_DELAY;
    mcmd = mcmd << 1;
  }  
}


/**************************************************************************************************
 * @fn      I2C_Ack
 *
 * @brief   I2C ack singal
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void I2C_Ack(void)
{
  SDA_H;
  
  SCL_H;
  OLED_OP_DELAY;
  SCL_L;
  OLED_OP_DELAY;
}


/**************************************************************************************************
 * @fn      writec
 *
 * @brief   OLED control data
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void writec(unsigned char x)
{
  I2C_Start();

  I2C_O(0x78);
  I2C_Ack();
  
  I2C_O(0x00);
  I2C_Ack();

  I2C_O(x);
  I2C_Ack();
  
  I2C_Stop();
}


/**************************************************************************************************
 * @fn      writed
 *
 * @brief   OLED common data
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void writed(unsigned char d)
{
  I2C_Start();

  I2C_O(0x78);
  I2C_Ack();
  
  I2C_O(0x40);
  I2C_Ack();

  I2C_O(d);
  I2C_Ack();
  
  I2C_Stop();
}


/***************************************************************************************************
 * @fn      HalOledOnOff
 *
 * @brief   Turns specified OLED ON or OFF
 *
 * @param   mode - OLED_ON,OLED_OFF,
 *
 * @return  none
 ***************************************************************************************************/
void HalOledOnOff(uint8 mode)
{
  if(mode == HAL_OLED_MODE_ON)
  {
    writec(0X8D);  //SET DCDC
    writec(0X14);  //DCDC ON
    writec(0XAF);  //DISPLAY ON
  }
  else
  {
    writec(0X8D);  //SET DCDC
    writec(0X10);  //DCDC OFF
    writec(0XAE);  //DISPLAY OFF
  }
}


/***************************************************************************************************
 * @fn      oled_pow
 *
 * @brief   m^n
 *
 * @param   
 *
 * @return  none
 ***************************************************************************************************/
uint32 oled_pow(uint8 m,uint8 n)
{
	uint32 result=1;	 
	while(n--)result*=m;    
	return result;
}	





/***************************************************************************************************
 * @fn      HalOledShowDegreeSymbol
 *
 * @brief   显示摄氏度符号
 *
 * @param   
 *
 * @return  none
 ***************************************************************************************************/
void HalOledShowDegreeSymbol(uint8 x,uint8 y)
{
    uint8 temp,t;
    uint8 x0=x;
    uint8 y0=y/8;
    
    // 设置开始位置
    Set_xy(x0,y0);
    // 写入数据
    I2C_Start();
    I2C_O(0x78);
    I2C_Ack();
    I2C_O(0x40);
    I2C_Ack();  
    
    for(t=0;t<80;++t)
    {
      temp = oled_degree_symbol[t]; //调用20X32℃符号
      I2C_O(temp);
      I2C_Ack();
      ++x0;
      if(x0-x == 20)  // 该行写完
      {
        I2C_Stop();
        x0 = x;
        if(t != 79)
        {
          ++y0;
          // 设置开始位置
          Set_xy(x0,y0);
          I2C_Start();
          I2C_O(0x78);
          I2C_Ack();
          I2C_O(0x40);
          I2C_Ack();            
        }
      }
    }
}


/***************************************************************************************************
 * @fn      HalOledShowPowerSymbol
 *
 * @brief   显示电量符号
 *
 * @param   
 *
 * @return  none
 ***************************************************************************************************/
void HalOledShowPowerSymbol(uint8 x,uint8 y,uint8 mode,uint8 power_num)
{
    uint8 temp,t;
    uint8 x0=x;
    uint8 y0=y/8;
    
    // 设置开始位置
    Set_xy(x0,y0);
    // 写入数据
    I2C_Start();
    I2C_O(0x78);
    I2C_Ack();
    I2C_O(0x40);
    I2C_Ack();  
    
    for(t=0;t<48;++t)
    {
      temp = oled_power_symbol[power_num][t]; //oled_power_symbol图标 power_num越小电量越低，10为最大100%,0最小0%
      I2C_O(temp);
      I2C_Ack();
      ++x0;
      if(x0-x == 24)  // 该行写完
      {
        I2C_Stop();
        x0 = x;
        if(t != 47)
        {
          ++y0;
          // 设置开始位置
          Set_xy(x0,y0);
          I2C_Start();
          I2C_O(0x78);
          I2C_Ack();
          I2C_O(0x40);
          I2C_Ack();            
        }
      }
    }
}
#else
void HalOledOnOff(uint8 mode);

void HalOledInit(void);
void HalOledClear(void);
void HalOledShowChar(uint8 x,uint8 y,uint8 chr,uint8 size,uint8 mode);
void HalOledShowNum(uint8 x,uint8 y,uint32 num,uint8 len,uint8 size);
void HalOledShowString(uint8 x,uint8 y,uint8 size,const uint8 *p);

void HalOledShowDegreeSymbol(uint8 x,uint8 y);
void HalOledShowPowerSymbol(uint8 x,uint8 y,uint8 mode,uint8 power_num);
#endif /* HAL_OLED */