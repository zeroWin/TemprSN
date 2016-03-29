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
  PROVIDED �AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
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
/* OLED RST is at P0.3 */
#define OLED_RST_PORT 0
#define OLED_RST_PIN  4

/* OLED SCL is at P0.4 */
#define OLED_SCL_PORT 0
#define OLED_SCL_PIN  4

/* OLED SDA is at P0.5 */
#define OLED_SDA_PORT 0
#define OLED_SDA_PIN  5

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
/* OLED_2832HSWEG02 , 128 x 32 pixel 
   OLED���Դ�
   ��Ÿ�ʽ����.
   [0]0 1 2 3 ... 127	
   [1]0 1 2 3 ... 127	
   [2]0 1 2 3 ... 127	
   [3]0 1 2 3 ... 127
*/
unsigned char OLED_GRAM[128][4];

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

void HalOledDrawPoint(uint8 x,uint8 y,uint8 t);
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
  
  /* ��������  �ο�UG-28HSWEG02�ֲ� p10 */
  writec(0xAE);    //display off

  writec(0xd5);    //contract control
  writec(0x80);

  writec(0xA8); 
  writec(0x1F); 

  writec(0xD3);
  writec(0x00);

  writec(0x40);
  
  writec(0x8D);
  writec(0x14);
  
  writec(0xA1);   //A1h, X[0]=1b: column address 127 is mapped to SEG0

  writec(0xC8);   //C8h, X[3]=1b: remapped mode. Scan from COM[31] to COM0

  writec(0xDA);
  writec(0x02);

  writec(0x81);
  writec(0x8F);

  writec(0xD9);
  writec(0xF1);

  writec(0xDB);
  writec(0x40);

  writec(0xA4);
  writec(0xA6);


  /* address setting */
  writec(0x20);    //Set Memory Addressing Mode   2 bytes cmd
  writec(0x00);    //A[1:0] = 00b, Horizontal Addressing Mode

  writec(0x21);    //Setup column start and end address
  writec(0x00);    //A[6:0] : Column start address, range : 0-127d, (RESET=0d)
  writec(0x7f);    //B[6:0]: Column end address, range : 0-127d, (RESET =127d)

  writec(0x22);    //Setup page start and end address
  writec(0x00);	   //A[2:0] : Page start Address, range : 0-7d, (RESET=0d)
  writec(0x03);    //B[2:0] : Page end Address, range : 0-7d, (RESET = 7d)
	 
  writec(0xAF);    //display on
  HalOledClear();
}


/**************************************************************************************************
 * @fn      HalOledDrawPoint
 *
 * @brief   Draw a point on OLED
 *
 * @param   
 *
 * @return  None
 **************************************************************************************************/
void HalOledDrawPoint(uint8 x,uint8 y,uint8 t)
{
  uint8 pos,bx,temp=0;
  if(x>127||y>31)return;//������Χ��.
  pos=y/8;
  bx=y%8;
  temp=1<<(bx);
  if(t)OLED_GRAM[x][pos]|=temp;
  else OLED_GRAM[x][pos]&=~temp;
}


/**************************************************************************************************
 * @fn      HalOledShowChar
 *
 * @brief   Show a char on OLED
 *
 * @param   size:12/16/32
 *
 * @return  None
 **************************************************************************************************/
void HalOledShowChar(uint8 x,uint8 y,uint8 chr,uint8 size,uint8 mode)
{
  uint8 temp,t,t1;
  uint8 y0=y;
  chr=chr-' ';//�õ�ƫ�ƺ��ֵ
  if(size == 32)
  {      //�ĵ�ģʽ(32x16)������Ļ�е�һ���㣬�����4����
    for(t=0;t<16;t++)   //����1608���壬ÿ���ַ�16byte
    {   
      temp=oled_asc2_1608[chr][t];		 //����1608���� ,ÿ��Loadһ��byte�������ϵ��£�8��point	                          
      for(t1=0;t1<8;t1++)
      {
        if(temp&0x80)
        {
          HalOledDrawPoint(x,y,mode);	//(x,y),(x,y+1),(x+1,y),(x+1,y+1) ���ĸ���
          HalOledDrawPoint(x,y+1,mode);
          HalOledDrawPoint(x+1,y,mode);
          HalOledDrawPoint(x+1,y+1,mode);
        }
        else
        {
          HalOledDrawPoint(x,y,!mode);	//(x,y),(x,y+1),(x+1,y),(x+1,y+1) ���ĸ���
          HalOledDrawPoint(x,y+1,!mode);
          HalOledDrawPoint(x+1,y,!mode);
          HalOledDrawPoint(x+1,y+1,!mode);
        }
        temp<<=1;
        y=y+2;
        if((y-y0)==32)
        {
          y=y0;
          x=x+2;
          break;
        }
      }  	 
    }
  }
  else
  {	   //����ģʽ			   
    for(t=0;t<size;t++)
    {
      if(size==12)temp=oled_asc2_1206[chr][t];  //����1206����
      else temp=oled_asc2_1608[chr][t];		 //����1608���� ,ÿ��Loadһ��byte�������ϵ��£�8��point	                          
      for(t1=0;t1<8;t1++)
      {
        if(temp&0x80)HalOledDrawPoint(x,y,mode);
        else HalOledDrawPoint(x,y,!mode);
        temp<<=1;
        y++;
        if((y-y0)==size)
        {
          y=y0;
          x++;
          break;
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
#define MAX_CHAR_POSY 22    
  while(*p!='\0')
  {       
    if(x>MAX_CHAR_POSX){x=0;y+=16;}
    if(y>MAX_CHAR_POSY){y=x=0;HalOledClear();}
    HalOledShowChar(x,y,*p,size,1);	 
    x+=size/2;
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
 * @fn      HalOledRefreshGram
 *
 * @brief   Refresh the OLED Gram
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalOledRefreshGram(void)
{
  uint8 i,n;	
	
  I2C_Start();
        
  I2C_O(0x78);
  I2C_Ack();
  I2C_O(0x40);
  I2C_Ack();  
  
  for(i=0;i<4;i++)  
  {    
    for(n=0;n<128;n++)
    {
      I2C_O(OLED_GRAM[n][i]);
      I2C_Ack();	
    }
  }
  
  I2C_Stop();
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
  uint8 i,n;  
  for(i=0;i<4;i++)for(n=0;n<128;n++)OLED_GRAM[n][i]=0X00;  
  HalOledRefreshGram();//������ʾ  
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
    writec(0xAF);    //display on
  }
  else
  {
    writec(0xAE);    //display off 
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
#else
void HalOledRefreshGram(void);
void HalOledOnOff(uint8 mode);

void HalOledInit(void);
void HalOledClear(void);
void HalOledShowChar(uint8 x,uint8 y,uint8 chr,uint8 size,uint8 mode);
void HalOledShowNum(uint8 x,uint8 y,uint32 num,uint8 len,uint8 size);
void HalOledShowString(uint8 x,uint8 y,uint8 size,const uint8 *p);
#endif /* HAL_OLED */