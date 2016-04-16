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
#include "exfuns.h"
#include "string.h"
#include "OSAL.h" 


/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/
//�ļ������б�
const uint8 *FILE_TYPE_TBL[6][13]=
{
{"BIN"},			//BIN�ļ�
{"LRC"},			//LRC�ļ�
{"NES"},			//NES�ļ�
{"TXT","C","H"},	//�ı��ļ�
{"MP1","MP2","MP3","MP4","M4A","3GP","3G2","OGG","ACC","WMA","WAV","MID","FLAC"},//�����ļ�
{"BMP","JPG","JPEG","GIF"},//ͼƬ�ļ�
};

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
FATFS *fs;  		//�߼����̹�����.	 
FIL *file;	  		//�ļ�1
FIL *ftemp;	  		//�ļ�2.
UINT br,bw;			//��д����
FILINFO fileinfo;	//�ļ���Ϣ
DIR dir;  			//Ŀ¼

uint8 *fatbuf;			//flash���ݻ�����

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
 * @brief   Ϊexfuns�����ڴ�
 *          
 * @param   none
 *
 * @return  ����ֵ:0,�ɹ�  1,ʧ��
 **************************************************************************************************/
uint8 exfuns_init(void)
{
	fs=(FATFS*)osal_mem_alloc(sizeof(FATFS));	//Ϊ����0�����������ڴ�	
	file=(FIL*)osal_mem_alloc(sizeof(FIL));	//Ϊfile�����ڴ�
	ftemp=(FIL*)osal_mem_alloc(sizeof(FIL));	//Ϊftemp�����ڴ�
	fatbuf=(uint8*)osal_mem_alloc(512);		//Ϊfatbuf�����ڴ�
	if(fs&&file&&ftemp&&fatbuf)return 0;  //������һ��ʧ��,��ʧ��.
	else return 1;	
}


/**************************************************************************************************
 * @fn      char_upper
 *
 * @brief   ��Сд��ĸתΪ��д��ĸ,���������,�򱣳ֲ���.
 *          
 * @param   input char
 *
 * @return  output char
 **************************************************************************************************/
uint8 char_upper(uint8 c)
{
	if(c<'A')return c;//����,���ֲ���.
	if(c>='a')return c-0x20;//��Ϊ��д.
	else return c;//��д,���ֲ���
}	


/**************************************************************************************************
 * @fn      f_typetell
 *
 * @brief   �����ļ�������
 *
 * @param   fname:�ļ���
 *
 * @return  ����ֵ:0XFF,��ʾ�޷�ʶ����ļ����ͱ��.
            ����,����λ��ʾ��������,����λ��ʾ����С��.
 **************************************************************************************************/
uint8 f_typetell(uint8 *fname)
{
	uint8 tbuf[5];
	uint8 *attr='\0';//��׺��
	uint8 i=0,j;
	while(i<250)
	{
		i++;
		if(*fname=='\0')break;//ƫ�Ƶ��������.
		fname++;
	}
	if(i==250)return 0XFF;//������ַ���.
 	for(i=0;i<5;i++)//�õ���׺��
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
 	for(i=0;i<4;i++)tbuf[i]=char_upper(tbuf[i]);//ȫ����Ϊ��д 
	for(i=0;i<6;i++)
	{
		for(j=0;j<13;j++)
		{
			if(*FILE_TYPE_TBL[i][j]==0)break;//�����Ѿ�û�пɶԱȵĳ�Ա��.
			if(strcmp((const char *)FILE_TYPE_TBL[i][j],(const char *)tbuf)==0)//�ҵ���
			{
				return (i<<4)|j;
			}
		}
	}
	return 0XFF;//û�ҵ�		 			   
}	 


/**************************************************************************************************
 * @fn      exf_getfree
 *
 * @brief   �õ�����ʣ������
 *
 * @param   drv:���̱��("0:")
 *          total:������	 ����λKB��
 *          free:ʣ������	 ����λKB��
 *
 * @return  0,����.����,�������
 **************************************************************************************************/
uint8 exf_getfree(uint8 *drv,uint32 *total,uint32 *free)
{
	FATFS *fs1;
	uint8 res;
    DWORD fre_clust=0, fre_sect=0, tot_sect=0;
    //�õ�������Ϣ�����д�����
    res = f_getfree((const TCHAR*)drv, &fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//�õ���������
	    fre_sect=fre_clust*fs1->csize;			//�õ�����������	   
#if _MAX_SS!=512				  				//������С����512�ֽ�,��ת��Ϊ512�ֽ�
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif	  
		*total=tot_sect>>1;	//��λΪKB
		*free=fre_sect>>1;	//��λΪKB 
 	}
	return res;
}		   

















