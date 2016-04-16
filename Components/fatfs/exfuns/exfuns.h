/**************************************************************************************************
  Filename:       exfuns.h
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
#ifndef __EXFUNS_H
#define __EXFUNS_H 		   
#include "ff.h"
#include "hal_board.h"

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
// 主要定义一些全局变量，供FATFS的使用，同时实现一些如磁盘容量获取的功能函数
extern FATFS *fs;  
extern FIL *file;	 
extern FIL *ftemp;	 
extern UINT br,bw;
extern FILINFO fileinfo;
extern DIR dir;
extern uint8 *fatbuf;       //Flash数据缓存区

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/
//f_typetell返回的类型定义
//根据表FILE_TYPE_TBL获得.在exfuns.c里面定义
#define T_BIN		0X00	//bin文件
#define T_LRC		0X10	//lrc文件
#define T_NES		0X20	//nes文件
#define T_TEXT		0X30	//.txt文件
#define T_C			0X31	//.c文件
#define T_H			0X32    //.h文件
#define T_FLAC		0X4C	//flac文件
#define T_BMP		0X50	//bmp文件
#define T_JPG		0X51	//jpg文件
#define T_JPEG		0X52	//jpeg文件		 
#define T_GIF		0X53	//gif文件  

/**************************************************************************************************
 *                                             FUNCTIONS - API
 **************************************************************************************************/

/*
 * Initialize for fatfs.
 */
extern uint8 exfuns_init(void);		//申请内存

/*
 * Get file type.
 */
extern uint8 f_typetell(uint8 *fname);	//识别文件类型

/*
 * Get totol size and free size.
 */
extern uint8 exf_getfree(uint8 *drv,uint32 *total,uint32 *free);//得到磁盘总容量和剩余容量
//uint32_t exf_fdsize(uint8_t *fdname);																				   //得到文件夹大小
//uint8_t* exf_get_src_dname(uint8_t* dpfn);																		   
//uint8_t exf_copy(uint8_t(*fcpymsg)(uint8_t*pname,uint8_t pct,uint8_t mode),uint8_t *psrc,uint8_t *pdst,uint32_t totsize,uint32_t cpdsize,uint8_t fwmode);	   //文件复制
//uint8_t exf_fdcopy(uint8_t(*fcpymsg)(uint8_t*pname,uint8_t pct,uint8_t mode),uint8_t *psrc,uint8_t *pdst,uint32_t *totsize,uint32_t *cpdsize,uint8_t fwmode);//文件夹复制

#endif


