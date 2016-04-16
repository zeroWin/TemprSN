/**************************************************************************************************
  Filename:       exfuns.c
  Revised:        $Date: 2016-04-16 23:49:16 +0800 (Sat, 16 Apr 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to the fatfs interface.


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
#include "exfuns.h"
#include "string.h"
#include "OSAL.h" 


/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/
//文件类型列表
const uint8 *FILE_TYPE_TBL[6][13]=
{
{"BIN"},			//BIN文件
{"LRC"},			//LRC文件
{"NES"},			//NES文件
{"TXT","C","H"},	//文本文件
{"MP1","MP2","MP3","MP4","M4A","3GP","3G2","OGG","ACC","WMA","WAV","MID","FLAC"},//音乐文件
{"BMP","JPG","JPEG","GIF"},//图片文件
};

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
FATFS *fs;  		//逻辑磁盘工作区.	 
FIL *file;	  		//文件1
FIL *ftemp;	  		//文件2.
UINT br,bw;			//读写变量
FILINFO fileinfo;	//文件信息
DIR dir;  			//目录

uint8 *fatbuf;			//flash数据缓存区

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
uint8 char_upper(uint8 c);

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/

/**************************************************************************************************
 * @fn      exfuns_init
 *
 * @brief   为exfuns申请内存
 *          
 * @param   none
 *
 * @return  返回值:0,成功  1,失败
 **************************************************************************************************/
uint8 exfuns_init(void)
{
	fs=(FATFS*)osal_mem_alloc(sizeof(FATFS));	//为磁盘0工作区申请内存	
	file=(FIL*)osal_mem_alloc(sizeof(FIL));	//为file申请内存
	ftemp=(FIL*)osal_mem_alloc(sizeof(FIL));	//为ftemp申请内存
	fatbuf=(uint8*)osal_mem_alloc(512);		//为fatbuf申请内存
	if(fs&&file&&ftemp&&fatbuf)return 0;  //申请有一个失败,即失败.
	else return 1;	
}


/**************************************************************************************************
 * @fn      char_upper
 *
 * @brief   将小写字母转为大写字母,如果是数字,则保持不变.
 *          
 * @param   input char
 *
 * @return  output char
 **************************************************************************************************/
uint8 char_upper(uint8 c)
{
	if(c<'A')return c;//数字,保持不变.
	if(c>='a')return c-0x20;//变为大写.
	else return c;//大写,保持不变
}	


/**************************************************************************************************
 * @fn      f_typetell
 *
 * @brief   报告文件的类型
 *
 * @param   fname:文件名
 *
 * @return  返回值:0XFF,表示无法识别的文件类型编号.
            其他,高四位表示所属大类,低四位表示所属小类.
 **************************************************************************************************/
uint8 f_typetell(uint8 *fname)
{
	uint8 tbuf[5];
	uint8 *attr='\0';//后缀名
	uint8 i=0,j;
	while(i<250)
	{
		i++;
		if(*fname=='\0')break;//偏移到了最后了.
		fname++;
	}
	if(i==250)return 0XFF;//错误的字符串.
 	for(i=0;i<5;i++)//得到后缀名
	{
		fname--;
		if(*fname=='.')
		{
			fname++;
			attr=fname;
			break;
		}
  	}
	strcpy((char *)tbuf,(const char*)attr);//copy
 	for(i=0;i<4;i++)tbuf[i]=char_upper(tbuf[i]);//全部变为大写 
	for(i=0;i<6;i++)
	{
		for(j=0;j<13;j++)
		{
			if(*FILE_TYPE_TBL[i][j]==0)break;//此组已经没有可对比的成员了.
			if(strcmp((const char *)FILE_TYPE_TBL[i][j],(const char *)tbuf)==0)//找到了
			{
				return (i<<4)|j;
			}
		}
	}
	return 0XFF;//没找到		 			   
}	 


/**************************************************************************************************
 * @fn      exf_getfree
 *
 * @brief   得到磁盘剩余容量
 *
 * @param   drv:磁盘编号("0:")
 *          total:总容量	 （单位KB）
 *          free:剩余容量	 （单位KB）
 *
 * @return  0,正常.其他,错误代码
 **************************************************************************************************/
uint8 exf_getfree(uint8 *drv,uint32 *total,uint32 *free)
{
	FATFS *fs1;
	uint8 res;
    DWORD fre_clust=0, fre_sect=0, tot_sect=0;
    //得到磁盘信息及空闲簇数量
    res = f_getfree((const TCHAR*)drv, &fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//得到总扇区数
	    fre_sect=fre_clust*fs1->csize;			//得到空闲扇区数	   
#if _MAX_SS!=512				  				//扇区大小不是512字节,则转换为512字节
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif	  
		*total=tot_sect>>1;	//单位为KB
		*free=fre_sect>>1;	//单位为KB 
 	}
	return res;
}		   

















