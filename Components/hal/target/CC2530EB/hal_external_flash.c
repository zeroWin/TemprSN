/**************************************************************************************************
  Filename:       hal_external_flash.c
  Revised:        $Date: 2016-04-11 14:08:16 +0800 (Mon, 11 Apr 2016) $
  Revision:       $Revision: 1 $

  Description:    This file contains the interface to the External Flash Service.


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
  ʹ��SST25FV016B оƬ��Ϊ����FALSH 16Mbit=2MB
**************************************************************************************************/

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
#include "hal_external_flash.h"
#include "hal_spi_user.h"

#if (defined HAL_EXTERNAL_FLASH) && (HAL_EXTERNAL_FLASH == TRUE)
/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
#define F_READ_COMMAND                  0x03    // Read Memory at 25 MHz
#define F_HIGH_SPEED_READ_COMMAND       0x0B    // Read Memory at 50 MHz
#define F_4K_ERASE_COMMAND              0x20    // Erase 4 KByte of memory array
#define F_32K_ERASE_COMMAND             0x52    // Erase 32 KByte of memory array           
#define F_64K_ERASE_COMMAND             0xD8    // Erase 64 KByte of memory array
#define F_CHIP_ERASE_COMMAND            0x60    // Erase Full Memory Array
#define F_BYTE_PROGRAM_COMMAND          0x02    // To Program One Data Byte
#define F_AAI_WORD_PROGRAM_COMMAND      0xAD    // Auto Address Increment Programming
#define F_RDSR_COMMAND                  0x05    // Read-Status-Register
#define F_EWSR_COMMAND                  0x50    // Enable-Write-Status-Register
#define F_WRSR_COMMAND                  0x01    // Write-Status-Register
#define F_WREN_COMMAND                  0x06    // Write-Enable
#define F_WRDI_COMMAND                  0x04    // Write-Disable
#define F_RDID_COMMAND                  0x90    // Read-ID
#define F_JEDEC_ID_COMMAND              0x9F    // JEDEC ID read
#define F_EBSY_COMMAND                  0x70    // Enable SO to output RY/BY# status during AAI programming
#define F_DBSY_COMMAND                  0x80    // Disable SO to output RY/BY# status during AAI programming

#define F_INFO_VERIFI                   0xAA55BCDE  // verification info  ��λ��д��

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
uint16 sectorWriteEnd;    // ��¼sector�õ��˵ڼ��� 1-511 ��0��sector����ϵͳ��Ϣ��
uint16 sectorWritePos;    // ��¼���һ��sector��ƫ��     0-4095
  
uint16 sectorReadEnd;    // ��¼sector�õ��˵ڼ��� 1-511 ��0��sector����ϵͳ��Ϣ��
uint16 sectorReadPos;    // ��¼���һ��sector��ƫ��     0-4095
/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void HalExtFlashSendAddr(uint32 Addr);

uint8 HalExtFlashReadStatusRegister(void);
void HalExtFlashWriteStatusRegEnable(void);
void HalExtFlashWriteStatusRegister(uint8 writeStatus);

void HalExtFlashWriteDisable(void);
void HalExtFlashWriteEnable(void);

void HalExtFlashChipErase(void);
void HalExtFlash64KBlockErase(uint32 addr);
void HalExtFlash32KBlockErase(uint32 addr);
void HalExtFlash4KSectorErase(uint32 addr);
void HalExtFlashWaitWriteEnd(void);

void HalExtFlashInfoWrite(void);
uint32 HalExtFlashInfoRead(void);
void HalExtFlashDataLenWrite(uint16 sectorWriteEndTemp,uint16 sectorWritePosTemp);
void HalExtFlashDataLenRead(uint16 *sectorWriteEndTemp,uint16 *sectorWritePosTemp);

void HalExtFlashRTCWrite(RTCStruct_t *RTCStruct,uint32 addr);
void HalExtFlashRTCRead(RTCStruct_t *RTCStruct,uint32 addr);

void HalExtFlashSampleRead(uint8 *SampleData,uint32 addr);
void HalExtFlashSampleWrite(uint8 *SampleData,uint32 addr);
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/

/**************************************************************************************************
 * @fn      HalExtFlashInit
 *
 * @brief   Initialize Ext Flash.
 *          Must init SPI first.This project use HalSpiUInit init SPI
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalExtFlashInit(void)
{  
  // close all block protection
  HalExtFlashWriteStatusRegister(0x00);

  // ��ȡflash�е�����
  // 0-3 Ԥ����Ϣ,����У��flash�����Ƿ�������
  // 4-5 sectorWriteEnd ��λ��ǰ
  // 6-7 sectorWritePos ��λ��ǰ
  if( HalExtFlashInfoRead() != F_INFO_VERIFI)
  {
    // Ԥ����Ϣ���ԣ�ȫƬ����
    HalExtFlashChipErase();
    // д��Ԥ��У����Ϣ
    HalExtFlashInfoWrite();
    
    sectorWriteEnd = 1;
    sectorWritePos = 0;
    
    sectorReadEnd = 1;
    sectorReadPos = 0;
    // д���ʼ����
    HalExtFlashDataLenWrite(sectorWriteEnd,sectorWritePos);
  }
  else  // Ԥ����Ϣ��ȷ
  {
    // ��ȡ���ݳ���
    HalExtFlashDataLenRead(&sectorWriteEnd,&sectorWritePos);
    sectorReadEnd = sectorWriteEnd;
    sectorReadPos = sectorWritePos;
  }

}


/**************************************************************************************************
 * @fn      HalExtFlashDataWrite
 *
 * @brief   write data to flash
 *
 * @param   ExtFlashStruct_t
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashDataWrite(ExtFlashStruct_t ExtFlashStruct)
{
  // ����д���ַ
  uint32 writeAddress = sectorWriteEnd*4096 + sectorWritePos;
  
  // write RTC and sample data
  HalExtFlashRTCWrite(&(ExtFlashStruct.RTCStruct),writeAddress); // д��8BYTE ʱ��
  HalExtFlashSampleWrite(ExtFlashStruct.sampleData,writeAddress+8); // д��4BYTE �ɼ�����
  
  // ����д��ĵ�ַ
  sectorWritePos += 12;
  if(sectorWritePos > 4095) // ���������꣬������һ������
  {
    sectorWriteEnd++;
    sectorWritePos = sectorWritePos - 4096;
  }
  
  /* ����flash��������Ϣ */
  // ������ҳ��Ϣ����
  HalExtFlash4KSectorErase(0x000000);
  // ����flash��д���ַ
  HalExtFlashDataLenWrite(sectorWriteEnd,sectorWritePos);
  // д����ϢУ����Ϣ��flash
  HalExtFlashInfoWrite();
  
  /* ÿ��д�붼���¶�ȡ��ַ */
  sectorReadEnd = sectorWriteEnd;
  sectorReadPos = sectorWritePos;
  
}


/**************************************************************************************************
 * @fn      HalExtFlashDataRead
 *
 * @brief   Read data from flash
 *          ֻ�е�һ��������ȫ����ȡ���Ž���flash�е����ݲ���.
 *          ���û����ȫ��ȡ��˵��ͬ���������⣬�´�ͬ��������ͷ��ʼͬ��
 *          ��Ҫԭ����flash��СҲ��Ҫ����4K�Ŀռ�
 *
 * @param   none
 *
 * @return  0:data invalid 
            1:data effective
 **************************************************************************************************/
uint8 HalExtFlashDataRead(ExtFlashStruct_t *ExtFlashStruct)
{
  // �����ȡ��ַ
  uint32 readAddress = sectorReadEnd*4096 + sectorReadPos - 12;
  if( readAddress < 4096 )
    return DATA_READ_INVALID;
  
  // Read RTC and sample data
  HalExtFlashRTCRead(&(ExtFlashStruct->RTCStruct),readAddress); // ��ȡ8BYTE ʱ��
  HalExtFlashSampleRead(ExtFlashStruct->sampleData,readAddress+8); // ��ȡ4BYTE �ɼ����� 
  
  // ���¶�ȡд��
  if(sectorReadPos > 12)  // ������û����
  {
    sectorReadPos -= 12;
  }
  else  // ���������� 
  {
    if(sectorReadPos == 12)
      sectorReadPos = 0;
    else
    {
      sectorReadEnd--;
      sectorReadPos = 4095 - (11 - sectorReadPos);
    }
    // �������������
    HalExtFlash4KSectorErase(sectorWriteEnd*4096);
    /* ���»�����д��ַ */
    sectorWriteEnd = sectorReadEnd;
    sectorWritePos = sectorReadPos;
   
    /* ����flash��������Ϣ */
    // ������ҳ��Ϣ����
    HalExtFlash4KSectorErase(0x000000);
    // ����flash��д���ַ
    HalExtFlashDataLenWrite(sectorWriteEnd,sectorWritePos);
    // д����ϢУ����Ϣ��flash
    HalExtFlashInfoWrite();  
  }
  
  return DATA_READ_EFFECTIVE;
}


/**************************************************************************************************
 * @fn      HalExtFlashSampleWrite
 *
 * @brief   Write Sample data to flash 2Byte
 *
 * @param   SampleData: ���£���λ��С�����֣���λ����������
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashSampleWrite(uint8 *SampleData,uint32 addr)
{
  HalExtFlashBufferWrite(SampleData,addr,4);
}


/**************************************************************************************************
 * @fn      HalExtFlashSampleRead 
 *
 * @brief   Read Sample data from flash 2Byte
 *
 * @param   SampleData: ���£���λ��С�����֣���λ����������
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashSampleRead(uint8 *SampleData,uint32 addr)
{
  HalExtFlashBufferRead(SampleData,addr,4);
}


/**************************************************************************************************
 * @fn      HalExtFlashRTCWrite
 *
 * @brief   Write RTC time to flash 8BYTE
 *
 * @param   RTCStruct_t *RTCStrcut:time struct
 *          addr:write address
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashRTCWrite(RTCStruct_t *RTCStruct,uint32 addr)
{
  HalExtFlashBufferWrite((uint8 *)RTCStruct,addr,8);
}


/**************************************************************************************************
 * @fn      HalExtFlashRTCRead
 *
 * @brief   Read RTC time from flash 8BYTE
 *
 * @param   RTCStruct_t *RTCStrcut:time struct
 *          addr:read address
 *
 * @return  RTCStruct_t RTCStrcut:time struct
 **************************************************************************************************/
void HalExtFlashRTCRead(RTCStruct_t *RTCStruct,uint32 addr)
{
  HalExtFlashBufferRead((uint8 *)RTCStruct,addr,8);
}


/**************************************************************************************************
 * @fn      HalExtFlashDataLenWrite
 *
 * @brief   write dataLength to flash
 *
 * @param   uint16 sectorWriteEndTemp:which sector 1-511
 *          uint16 sectorWritePosTemp:0-4095
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashDataLenWrite(uint16 sectorWriteEndTemp,uint16 sectorWritePosTemp)
{
  uint8 sectorWriteEndPosBuffer[4];
  sectorWriteEndPosBuffer[0] = sectorWriteEndTemp & 0xFF;
  sectorWriteEndPosBuffer[1] = (sectorWriteEndTemp & 0xFF00) >> 8;
  sectorWriteEndPosBuffer[2] = sectorWritePosTemp & 0xFF;
  sectorWriteEndPosBuffer[3] = (sectorWritePosTemp & 0xFF00) >> 8;
  HalExtFlashBufferWrite(sectorWriteEndPosBuffer,0x000004,4);
}


/**************************************************************************************************
 * @fn      HalExtFlashDataLenRead
 *
 * @brief   Read dataLength from flash
 *
 * @param   uint16 sectorWriteEndTemp:which sector 1-511
 *          uint16 sectorWritePosTemp:0-4095
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashDataLenRead(uint16 *sectorWriteEndTemp,uint16 *sectorWritePosTemp)
{
  uint8 sectorWriteEndPosBuffer[4];
  
  HalExtFlashBufferRead(sectorWriteEndPosBuffer,0x000004,4);
  
  *sectorWriteEndTemp = ((uint16)sectorWriteEndPosBuffer[1] << 8 | sectorWriteEndPosBuffer[0] );
  *sectorWritePosTemp = ((uint16)sectorWriteEndPosBuffer[3] << 8 | sectorWriteEndPosBuffer[2] );
}


/**************************************************************************************************
 * @fn      HalExtFlashInfoWrite
 *
 * @brief   write verification info to flash
 *
 * @param   none
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashInfoWrite(void)
{
  uint8 sectorInfoBuffer[4];
  
  sectorInfoBuffer[0] = 0xDE;
  sectorInfoBuffer[1] = 0xBC;
  sectorInfoBuffer[2] = 0x55;
  sectorInfoBuffer[3] = 0xAA;
  
  HalExtFlashBufferWrite(sectorInfoBuffer,0x000000,4);
}


/**************************************************************************************************
 * @fn      HalExtFlashInfoRead
 *
 * @brief   Read verification info from flash
 *
 * @param   none
 *
 * @return  none
 **************************************************************************************************/
uint32 HalExtFlashInfoRead(void)
{
  uint8 sectorInfoBuffer[4];
  uint32 verifiInfo;
  
  HalExtFlashBufferRead(sectorInfoBuffer,0x000000,4);
  verifiInfo = ((uint32)sectorInfoBuffer[3] << 24) | 
               ((uint32)sectorInfoBuffer[2] << 16) | 
               ((uint32)sectorInfoBuffer[1] << 8) | sectorInfoBuffer[0];// ��λ��ǰ
  return verifiInfo;
}


/**************************************************************************************************
 * @fn      HalExtFlashByteWrite
 *
 * @brief   one byte write
 *
 * @param   writeAddr - write data address
 *          writeData - write data
 *
 * @return
 **************************************************************************************************/
void HalExtFlashByteWrite(uint32 writeAddress,uint8 writeData)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable(); // ѡ��оƬ

  HalSpiWriteReadByte(F_BYTE_PROGRAM_COMMAND);    // ����Byte program ����
  HalExtFlashSendAddr(writeAddress);          // ��������д���ַ
  HalSpiWriteReadByte(writeData);                 // ��������
  
  HalSpiFlashDisable(); // ��ѡ��оƬ  
  
  HalExtFlashWaitWriteEnd();    // wait for write end
}


/**************************************************************************************************
 * @fn      HalExtFlashBufferWrite
 *
 * @brief   buffer write
 *
 * @param   writebuffer - write data
 *          writeAddr - write data address
 *          writeLength - write length ������ż�������������ˣ��õ�ʱ����ⶼ��ż��
 *
 * @return
 **************************************************************************************************/
void HalExtFlashBufferWrite(uint8* writebuffer,uint32 writeAddress,uint16 writeLength)
{
  uint16 writeNum = (writeLength/2) - 1;
  
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable(); // ѡ��оƬ

  // write first 2 byte
  HalSpiWriteReadByte(F_AAI_WORD_PROGRAM_COMMAND);    // ����AAI program ����
  HalExtFlashSendAddr(writeAddress);              // ��������д���ַ
  HalSpiWriteReadByte(*writebuffer++);                // ��������
  HalSpiWriteReadByte(*writebuffer++);                // ��������
  
  HalSpiFlashDisable(); // ��ѡ��оƬ  
  HalExtFlashWaitWriteEnd();    // wait for write end
  
  while(writeNum--)
  {
    HalSpiFlashEnable(); // ѡ��оƬ
      
    HalSpiWriteReadByte(F_AAI_WORD_PROGRAM_COMMAND);    // ����AAI program ����
    HalSpiWriteReadByte(*writebuffer++);                // ��������
    HalSpiWriteReadByte(*writebuffer++);                // ��������
    
    HalSpiFlashDisable(); // ��ѡ��оƬ      
    HalExtFlashWaitWriteEnd();    // wait for write end
  }
  
  HalExtFlashWriteDisable();    //write disable
  HalExtFlashWaitWriteEnd();    // wait for write end // wait close
}


/**************************************************************************************************
 * @fn      HalExtFlashByteRead
 *
 * @brief   One Byte Read on 25Mhz
 *
 * @param   readAddr - read address
 *
 * @return  read data
 **************************************************************************************************/
uint8 HalExtFlashByteRead(uint32 readAddress)
{
  uint8 readData;
  HalSpiFlashEnable(); // ѡ��оƬ

  HalSpiWriteReadByte(F_READ_COMMAND);    // ����Read data ����
  HalExtFlashSendAddr(readAddress);   // �������ݶ�ȡ��ַ
  
  // read data
  readData = HalSpiWriteReadByte(DUMMY_BYTE);
  
  HalSpiFlashDisable(); // ��ѡ��оƬ
  
  return readData;
}


/**************************************************************************************************
 * @fn      HalExtFlashBufferRead
 *
 * @brief   buffer Read on 25Mhz
 *
 * @param   pBuffer - store the data
 *          readAddr - read start address
 *          readLength - read length
 *
 * @return  None
 **************************************************************************************************/
void HalExtFlashBufferRead(uint8 *pBuffer,uint32 readAddress,uint16 readLength)
{
  HalSpiFlashEnable(); // ѡ��оƬ

  HalSpiWriteReadByte(F_READ_COMMAND);    // ����Read data ����
  HalExtFlashSendAddr(readAddress);   // �������ݶ�ȡ��ַ
  
  // read data
  while(readLength--)
  {
    *pBuffer = HalSpiWriteReadByte(DUMMY_BYTE);
    pBuffer++;
  }
  
  HalSpiFlashDisable(); // ��ѡ��оƬ
}


/**************************************************************************************************
 * @fn      HalExtFlashReadId
 *
 * @brief   Read Manufacturers ID and Device ID
 *
 * @param   none
 *
 * @return  High Byte is manufacturer's ID  0xBF is right
            Low Byte is devcie ID           0x41 is right
 **************************************************************************************************/
uint16 HalExtFlashReadId(void)
{
  uint16 ID;
  uint8 ManuID,DeviceID;
  
  HalSpiFlashEnable(); // ѡ��оƬ
  
  HalSpiWriteReadByte(F_RDID_COMMAND);  // ����Read ID ����
  HalExtFlashSendAddr(0x000000);    // ����24λ�ĵ�ַ�ֽ�

  ManuID = HalSpiWriteReadByte(DUMMY_BYTE);
  DeviceID = HalSpiWriteReadByte(DUMMY_BYTE);
  
  HalSpiFlashDisable(); // ��ѡ��оƬ
  
  ID = (((uint16)ManuID << 8) | DeviceID );
  return ID;
}


/**************************************************************************************************
 * @fn      HalExtFlashReadJEDECId
 *
 * @brief   Read Manufacturers ID and Device ID
 *
 * @param   none
 *
 * @return  bit16-23  Manufacturer��s ID 0xBF is right
            bit8-15   Memory Type        0x25 is right
            bit0-7    Memory Capacity    0x41 is right
 **************************************************************************************************/
uint32 HalExtFlashReadJEDECId(void)
{
  uint32 JEDECId=0;
  uint8 ManuID,memoryType,memoryCap;
  
  HalSpiFlashEnable(); // ѡ��оƬ
  
  HalSpiWriteReadByte(F_JEDEC_ID_COMMAND);  // ����Read JEDEC ID ����
  
  ManuID = HalSpiWriteReadByte(DUMMY_BYTE);
  memoryType = HalSpiWriteReadByte(DUMMY_BYTE);
  memoryCap = HalSpiWriteReadByte(DUMMY_BYTE);
    
  HalSpiFlashDisable(); // ��ѡ��оƬ
  
  JEDECId = (((uint32)ManuID << 16) | ((uint32)memoryType << 8) | memoryCap );
  return JEDECId;
}


/**************************************************************************************************
 * @fn      HalExtFlashWriteStatusRegEnable
 *
 * @brief   Write status register enable
 *
 * @param   none
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashWriteStatusRegEnable(void)
{
  HalSpiFlashEnable(); // ѡ��оƬ
  
  HalSpiWriteReadByte(F_EWSR_COMMAND);  // ����Write Status enable ����
  
  HalSpiFlashDisable(); // ��ѡ��оƬ    
}


/**************************************************************************************************
 * @fn      HalExtFlash4KSectorErase
 *
 * @brief   4K sector erase
 *
 * @param   erase head addr - ÿ�β���4096 - 0x1000
 *          0x000000 - 0x000FFF 4096 = 4KB
 *          0x001000 - 0x001FFF 4096 = 4KB
 *          0x002000 - 0x002FFF 4096 = 4KB
 *          ��˴�A23-A12 ����������λ�á�һ��2048KB/4KB=512��λ��
 *          Ҳ����addr��A23-A12λ��ѡ0x000xxx-0x1FFxxx 512��ֵ
 *          оƬֻ��ʶ��A23-A12λ
 * @return  none
 **************************************************************************************************/
void HalExtFlash4KSectorErase(uint32 addr)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable();          // ѡ��оƬ
  
  HalSpiWriteReadByte(F_4K_ERASE_COMMAND);  // ����4K erase ����
  HalExtFlashSendAddr(addr);             // ���Ͳ����׵�ַ
  
  HalSpiFlashDisable();         // ��ѡ��оƬ   
  
  HalExtFlashWaitWriteEnd();    // wait for earse end
}


/**************************************************************************************************
 * @fn      HalExtFlash32KBlockErase
 *
 * @brief   32K block erase
 *
 * @param   erase head addr - ÿ�β���32768 - 0x8000
 *          0x000000 - 0x007FFF 32768 = 32KB
 *          0x008000 - 0x00FFFF 32768 = 32KB
 *          0x010000 - 0x017FFF 32768 = 32KB
 *          ��˴�A23-A15 ����������λ�á�һ��2048KB/32KB=64��λ��
 *          Ҳ����addr��A23-A15λ��ѡ
 *          0 0000 0(B) - 1 1111 1(B) 64��ֵ 0x000000-=0x1F8000
 *          оƬֻ��ʶ��A23-A15λ
 * @return  none
 **************************************************************************************************/
void HalExtFlash32KBlockErase(uint32 addr)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable();          // ѡ��оƬ
  
  HalSpiWriteReadByte(F_32K_ERASE_COMMAND);  // ����32K erase ����
  HalExtFlashSendAddr(addr);             // ���Ͳ����׵�ַ
  
  HalSpiFlashDisable();         // ��ѡ��оƬ   
  
  HalExtFlashWaitWriteEnd();    // wait for earse end
}


/**************************************************************************************************
 * @fn      HalExtFlash64KBlockErase
 *
 * @brief   64K block erase
 *
 * @param   erase head addr - ÿ�β���65536 - 0xFFFF
 *          0x000000 - 0x00FFFF 65536 = 64KB
 *          0x010000 - 0x01FFFF 65536 = 64KB
 *          0x020000 - 0x02FFFF 65536 = 64KB
 *          ��˴�A23-A16 ����������λ�á�һ��2048KB/64KB=32��λ��
 *          Ҳ����addr��ѡ0x00xxxx-0x1Fxxxx 32��ֵ
 *          оƬֻ��ʶ��A23-A16λ
 * @return  none
 **************************************************************************************************/
void HalExtFlash64KBlockErase(uint32 addr)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable();          // ѡ��оƬ
  
  HalSpiWriteReadByte(F_64K_ERASE_COMMAND);  // ����64K erase ����
  HalExtFlashSendAddr(addr);             // ���Ͳ����׵�ַ
  
  HalSpiFlashDisable();         // ��ѡ��оƬ   
  
  HalExtFlashWaitWriteEnd();    // wait for earse end
}


/**************************************************************************************************
 * @fn      HalExtFlashChipErase
 *
 * @brief   Chip erase
 *
 * @param   none
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashChipErase(void)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable();          // ѡ��оƬ
  
  HalSpiWriteReadByte(F_CHIP_ERASE_COMMAND);  // ����Chip erase ����
  
  HalSpiFlashDisable();         // ��ѡ��оƬ   
  
  HalExtFlashWaitWriteEnd();    // wait for earse end
}


/**************************************************************************************************
 * @fn      HalExtFlashWaitWriteEnd
 *
 * @brief   Wait write program end
 *
 * @param   none
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashWaitWriteEnd(void)
{
  while((HalExtFlashReadStatusRegister() & 0x01) == 0x01);
}


/**************************************************************************************************
 * @fn      HalExtFlashWriteEnable
 *
 * @brief   Write enable
 *
 * @param   none
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashWriteEnable(void)
{
  HalSpiFlashEnable(); // ѡ��оƬ
  
  HalSpiWriteReadByte(F_WREN_COMMAND);  // ����Write enable ����
  
  HalSpiFlashDisable(); // ��ѡ��оƬ    
}


/**************************************************************************************************
 * @fn      HalExtFlashWriteDisable
 *
 * @brief   Write disable
 *
 * @param   none
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashWriteDisable(void)
{
  HalSpiFlashEnable(); // ѡ��оƬ
  
  HalSpiWriteReadByte(F_WRDI_COMMAND);  // ����Write disable ����
  
  HalSpiFlashDisable(); // ��ѡ��оƬ    
}


/**************************************************************************************************
 * @fn      HalExtFlashReadStatusRegister
 *
 * @brief   Read status register
 *
 * @param   none
 *
 * @return  status resgister value
 **************************************************************************************************/
uint8 HalExtFlashReadStatusRegister(void)
{
  uint8 statusRegister;
  
  HalSpiFlashEnable(); // ѡ��оƬ
  
  HalSpiWriteReadByte(F_RDSR_COMMAND);  // ����Read status register ����
  statusRegister = HalSpiWriteReadByte(DUMMY_BYTE);
  
  HalSpiFlashDisable(); // ��ѡ��оƬ  
  
  return statusRegister;
}


/**************************************************************************************************
 * @fn      HalExtFlashWriteStatusRegister
 *
 * @brief   write status register
 *
 * @param   write status
 *
 * @return  
 **************************************************************************************************/
void HalExtFlashWriteStatusRegister(uint8 writeStatus)
{
  HalExtFlashWriteStatusRegEnable();
  
  HalSpiFlashEnable(); // ѡ��оƬ
  
  HalSpiWriteReadByte(F_WRSR_COMMAND);  // ����Write Status����
  HalSpiWriteReadByte(writeStatus);     // write data to status register
  
  HalSpiFlashDisable(); // ��ѡ��оƬ 
}


/**************************************************************************************************
 * @fn      HalExtFlashSendAddr
 *
 * @brief   Send 24byte address
 *
 * @param   addr - 0x000000 to 0x1FFFFF = 2MB�ĵ�ַ
 *
 * @return  
 **************************************************************************************************/
void HalExtFlashSendAddr(uint32 Addr)
{
  HalSpiWriteReadByte((Addr & 0xFF0000) >> 16); 
  HalSpiWriteReadByte((Addr & 0xFF00) >> 8);
  HalSpiWriteReadByte((Addr & 0xFF));
}


/**************************************************************************************************
 * @fn      HalExtFlashSendAddr
 *
 * @brief   Send 24byte address
 *
 * @param   addr - 0x000000 to 0x1FFFFF = 2MB�ĵ�ַ
 *
 * @return  
 **************************************************************************************************/
void HalExtFlashReset(void)
{
  HalExtFlashChipErase();
  
  // д��Ԥ��У����Ϣ
  HalExtFlashInfoWrite();
    
  sectorWriteEnd = 1;
  sectorWritePos = 0;
    
  // д���ʼ����
  HalExtFlashDataLenWrite(sectorWriteEnd,sectorWritePos);  
}


/**************************************************************************************************
 * @fn      HalExtFlashLoseNetwork
 *
 * @brief   Handle when lose network
 *
 * @param   
 *
 * @return  
 **************************************************************************************************/
void HalExtFlashLoseNetwork(void)
{
  // ����ȡд���ַ��λ
  HalExtFlashDataLenRead(&sectorWriteEnd,&sectorWritePos);
  sectorReadEnd = sectorWriteEnd;
  sectorReadPos = sectorWritePos;  
}
#else

void HalExtFlashInit(void);
uint16 HalExtFlashReadId(void);
uint32 HalExtFlashReadJEDECId(void);
void HalExtFlashBufferRead(uint8 *pBuffer,uint32 readAddress,uint16 readLength);
uint8 HalExtFlashByteRead(uint32 readAddress);

void HalExtFlashByteWrite(uint32 writeAddress,uint8 writeData);
void HalExtFlashBufferWrite(uint8* writebuffer,uint32 writeAddress,uint16 writeLength);

void HalExtFlashDataWrite(ExtFlashStruct_t ExtFlashStruct);
uint8 HalExtFlashDataRead(ExtFlashStruct_t *ExtFlashStruct);
void HalExtFlashReset(void);
void HalExtFlashLoseNetwork(void);
#endif /* HAL_EXTERNAL_FLASH */