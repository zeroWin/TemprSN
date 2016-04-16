/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "hal_external_flash.h" /* Header file of existing FLASH contorl module */

/* Definitions of physical drive number for each drive */
#define ExFLASH           0       /* Map FLASH to physical drive 0 */

#define FLASH_SECTOR_SIZE   512      /* ������С512 ��������Լ��������ú�ϵͳʵ�ʵ�����һ�� */
#define FLASH_SECTOR_COUNT  2048*2   /* �� 4096 ������   */
#define FLASH_BLOCK_SIZE    8        /* ÿ��BLOCK��8������ ����BLOCKʵ����оƬ��sector*/

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{

  return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  uint8 res;
  switch(pdrv)
  {
    case ExFLASH://�ⲿflash
      res = 0;
      break;
    default:
      res = 1;
  }
  
  // ������ֵ
  if(res == 0x00) return RES_OK;
  else return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read ���512������*/
)
{
  uint8 res;
  if(!count)return RES_PARERR;  // count����Ϊ0�����򷵻ز�������
  
  switch(pdrv)
  {
    case ExFLASH://�ⲿflash
      while(count--)
      {
        HalExtFlashBufferRead(buff,sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
        sector++;
        buff += FLASH_SECTOR_SIZE;
      }
      res = 0;
      break;
    default:
      res = 1;
  }
  
  // ������ֵ
  if(res == 0x00) return RES_OK;
  else return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write ���512������*/
)
{
  uint8 res;
  if(!count)return RES_PARERR;  // count����Ϊ0�����򷵻ز�������
  
  switch(pdrv)
  {
    case ExFLASH://�ⲿflash
      while(count--)
      {
        HalExtFlashBufferWrite((uint8 *)buff,sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
        sector++;
        buff += FLASH_SECTOR_SIZE;
      }
      res = 0;
      break;
    default:
      res = 1;
  }
  
  // ������ֵ
  if(res == 0x00) return RES_OK;
  else return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  DRESULT res;
  if( pdrv == ExFLASH)
  {
    switch(cmd)
    {
      case CTRL_SYNC:
        res = RES_OK; 
        break;	 
      case GET_SECTOR_SIZE:
        *(WORD*)buff = FLASH_SECTOR_SIZE;
        res = RES_OK;
        break;	 
    case GET_BLOCK_SIZE:
      *(WORD*)buff = FLASH_BLOCK_SIZE;
      res = RES_OK;
      break;	 
    case GET_SECTOR_COUNT:
      *(DWORD*)buff = FLASH_SECTOR_COUNT;
      res = RES_OK;
      break;
    default:
      res = RES_PARERR;
      break;
    }
  }
  else res = RES_ERROR; //�����Ĳ�֧��
  
  return res;
}

