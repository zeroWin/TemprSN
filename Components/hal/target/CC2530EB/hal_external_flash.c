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
  使用SST25FV016B 芯片作为外置FALSH 16Mbit=2MB
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

#define F_INFO_VERIFI                   0xAA55BCDE  // verification info  低位先写入
/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/

/**************************************************************************************************
 *                                        INNER GLOBAL VARIABLES
 **************************************************************************************************/
 uint16 sectorEnd;    // 记录sector用到了第几个 1-511 第0个sector留给系统信息用
 uint16 sectorPos;    // 记录最后一个sector的偏移     0-4095
  
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
void HalExtFlashDataLenWrite(uint16 sectorEndTemp,uint16 sectorPosTemp);
void HalExtFlashDataLenRead(uint16 *sectorEndTemp,uint16 *sectorPosTemp);
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
  // 读取flash中的数据
  // 0-3 预留信息,用于校验flash数据是否有问题
  // 4-5 sectorEnd 低位在前
  // 6-7 sectorPos 低位在前
  if( HalExtFlashInfoRead() != F_INFO_VERIFI)
  {
    // 预留信息不对，全片擦除
    HalExtFlashChipErase();
    // 写入预留校验信息
    HalExtFlashInfoWrite();
    
    sectorEnd = 1;
    sectorPos = 0;
    
    // 写入初始长度
    HalExtFlashDataLenWrite(sectorEnd,sectorPos);
  }
  else  // 预留信息正确
  {
    // 读取数据长度
    HalExtFlashDataLenRead(&sectorEnd,&sectorPos);
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
  
}


/**************************************************************************************************
 * @fn      HalExtFlashDataRead
 *
 * @brief   Read data from flash
 *
 * @param   none
 *
 * @return  ExtFlashStruct_t
 **************************************************************************************************/
ExtFlashStruct_t HalExtFlashDataRead(void)
{
}


/**************************************************************************************************
 * @fn      HalExtFlashDataLenWrite
 *
 * @brief   write dataLength to flash
 *
 * @param   uint16 sectorEndTemp:which sector 1-511
 *          uint16 sectorPosTemp:0-4095
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashDataLenWrite(uint16 sectorEndTemp,uint16 sectorPosTemp)
{
  uint8 sectorEndPosBuffer[4];
  sectorEndPosBuffer[0] = sectorEndTemp & 0xFF;
  sectorEndPosBuffer[1] = (sectorEndTemp & 0xFF00) >> 8;
  sectorEndPosBuffer[2] = sectorPosTemp & 0xFF;
  sectorEndPosBuffer[3] = (sectorPosTemp & 0xFF00) >> 8;
  HalExtFlashBufferWrite(sectorEndPosBuffer,0x000004,4);
}


/**************************************************************************************************
 * @fn      HalExtFlashDataLenRead
 *
 * @brief   Read dataLength from flash
 *
 * @param   uint16 sectorEndTemp:which sector 1-511
 *          uint16 sectorPosTemp:0-4095
 *
 * @return  none
 **************************************************************************************************/
void HalExtFlashDataLenRead(uint16 *sectorEndTemp,uint16 *sectorPosTemp)
{
  uint8 sectorEndPosBuffer[4];
  
  HalExtFlashBufferRead(sectorEndPosBuffer,0x000004,4);
  
  *sectorEndTemp = ((uint16)sectorEndPosBuffer[1] << 8 | sectorEndPosBuffer[0] );
  *sectorPosTemp = ((uint16)sectorEndPosBuffer[3] << 8 | sectorEndPosBuffer[2] );
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
               ((uint32)sectorInfoBuffer[1] << 8) | sectorInfoBuffer[0];// 低位在前
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
  
  HalSpiFlashEnable(); // 选中芯片

  HalSpiWriteByte(F_BYTE_PROGRAM_COMMAND);    // 发送Byte program 命令
  HalExtFlashSendAddr(writeAddress);          // 发送数据写入地址
  HalSpiWriteByte(writeData);                 // 发送数据
  
  HalSpiFlashDisable(); // 不选中芯片  
  
  HalExtFlashWaitWriteEnd();    // wait for write end
}


/**************************************************************************************************
 * @fn      HalExtFlashBufferWrite
 *
 * @brief   buffer write
 *
 * @param   writebuffer - write data
 *          writeAddr - write data address
 *          writeLength - write length 必须是偶数，不做处理了，用的时候刻意都用偶数
 *
 * @return
 **************************************************************************************************/
void HalExtFlashBufferWrite(uint8* writebuffer,uint32 writeAddress,uint16 writeLength)
{
  uint16 writeNum = (writeLength/2) - 1;
  
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable(); // 选中芯片

  // write first 2 byte
  HalSpiWriteByte(F_AAI_WORD_PROGRAM_COMMAND);    // 发送AAI program 命令
  HalExtFlashSendAddr(writeAddress);              // 发送数据写入地址
  HalSpiWriteByte(*writebuffer++);                // 发送数据
  HalSpiWriteByte(*writebuffer++);                // 发送数据
  
  HalSpiFlashDisable(); // 不选中芯片  
  HalExtFlashWaitWriteEnd();    // wait for write end
  
  while(writeNum--)
  {
    HalSpiFlashEnable(); // 选中芯片
      
    HalSpiWriteByte(F_AAI_WORD_PROGRAM_COMMAND);    // 发送AAI program 命令
    HalSpiWriteByte(*writebuffer++);                // 发送数据
    HalSpiWriteByte(*writebuffer++);                // 发送数据
    
    HalSpiFlashDisable(); // 不选中芯片      
    HalExtFlashWaitWriteEnd();    // wait for write end
  }
  
  HalExtFlashWriteDisable();    //write disable
  while((HalExtFlashReadStatusRegister() & 0x03) != 0x00 ); // wait close
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
  HalSpiFlashEnable(); // 选中芯片

  HalSpiWriteByte(F_READ_COMMAND);    // 发送Read data 命令
  HalExtFlashSendAddr(readAddress);   // 发送数据读取地址
  
  // read data
  readData = HalSpiReadByte();
  
  HalSpiFlashDisable(); // 不选中芯片
  
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
  HalSpiFlashEnable(); // 选中芯片

  HalSpiWriteByte(F_READ_COMMAND);    // 发送Read data 命令
  HalExtFlashSendAddr(readAddress);   // 发送数据读取地址
  
  // read data
  while(readLength--)
  {
    *pBuffer = HalSpiReadByte();
    pBuffer++;
  }
  
  HalSpiFlashDisable(); // 不选中芯片
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
  
  HalSpiFlashEnable(); // 选中芯片
  
  HalSpiWriteByte(F_RDID_COMMAND);  // 发送Read ID 命令
  HalExtFlashSendAddr(0x000000);    // 发送24位的地址字节

  ManuID = HalSpiReadByte();
  DeviceID = HalSpiReadByte();
  
  HalSpiFlashDisable(); // 不选中芯片
  
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
 * @return  bit16-23  Manufacturer’s ID 0xBF is right
            bit8-15   Memory Type        0x25 is right
            bit0-7    Memory Capacity    0x41 is right
 **************************************************************************************************/
uint32 HalExtFlashReadJEDECId(void)
{
  uint32 JEDECId=0;
  uint8 ManuID,memoryType,memoryCap;
  
  HalSpiFlashEnable(); // 选中芯片
  
  HalSpiWriteByte(F_JEDEC_ID_COMMAND);  // 发送Read JEDEC ID 命令
  
  ManuID = HalSpiReadByte();
  memoryType = HalSpiReadByte();
  memoryCap = HalSpiReadByte();
    
  HalSpiFlashDisable(); // 不选中芯片
  
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
  HalSpiFlashEnable(); // 选中芯片
  
  HalSpiWriteByte(F_EWSR_COMMAND);  // 发送Write Status enable 命令
  
  HalSpiFlashDisable(); // 不选中芯片    
}


/**************************************************************************************************
 * @fn      HalExtFlash4KSectorErase
 *
 * @brief   4K sector erase
 *
 * @param   erase head addr - 每次擦数4096 - 0x1000
 *          0x000000 - 0x000FFF 4096 = 4KB
 *          0x001000 - 0x001FFF 4096 = 4KB
 *          0x002000 - 0x002FFF 4096 = 4KB
 *          因此从A23-A12 决定擦除的位置。一共2048KB/4KB=512个位置
 *          也就是addr的A23-A12位可选0x000xxx-0x1FFxxx 512个值
 *          芯片只会识别A23-A12位
 * @return  none
 **************************************************************************************************/
void HalExtFlash4KSectorErase(uint32 addr)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable();          // 选中芯片
  
  HalSpiWriteByte(F_4K_ERASE_COMMAND);  // 发送4K erase 命令
  HalExtFlashSendAddr(addr);             // 发送擦除首地址
  
  HalSpiFlashDisable();         // 不选中芯片   
  
  HalExtFlashWaitWriteEnd();    // wait for earse end
}


/**************************************************************************************************
 * @fn      HalExtFlash32KBlockErase
 *
 * @brief   32K block erase
 *
 * @param   erase head addr - 每次擦数32768 - 0x8000
 *          0x000000 - 0x007FFF 32768 = 32KB
 *          0x008000 - 0x00FFFF 32768 = 32KB
 *          0x010000 - 0x017FFF 32768 = 32KB
 *          因此从A23-A15 决定擦除的位置。一共2048KB/32KB=64个位置
 *          也就是addr的A23-A15位可选
 *          0 0000 0(B) - 1 1111 1(B) 64个值 0x000000-=0x1F8000
 *          芯片只会识别A23-A15位
 * @return  none
 **************************************************************************************************/
void HalExtFlash32KBlockErase(uint32 addr)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable();          // 选中芯片
  
  HalSpiWriteByte(F_32K_ERASE_COMMAND);  // 发送32K erase 命令
  HalExtFlashSendAddr(addr);             // 发送擦除首地址
  
  HalSpiFlashDisable();         // 不选中芯片   
  
  HalExtFlashWaitWriteEnd();    // wait for earse end
}


/**************************************************************************************************
 * @fn      HalExtFlash64KBlockErase
 *
 * @brief   64K block erase
 *
 * @param   erase head addr - 每次擦数65536 - 0xFFFF
 *          0x000000 - 0x00FFFF 65536 = 64KB
 *          0x010000 - 0x01FFFF 65536 = 64KB
 *          0x020000 - 0x02FFFF 65536 = 64KB
 *          因此从A23-A16 决定擦除的位置。一共2048KB/64KB=32个位置
 *          也就是addr可选0x00xxxx-0x1Fxxxx 32个值
 *          芯片只会识别A23-A16位
 * @return  none
 **************************************************************************************************/
void HalExtFlash64KBlockErase(uint32 addr)
{
  HalExtFlashWriteEnable();     //write enable
  
  HalSpiFlashEnable();          // 选中芯片
  
  HalSpiWriteByte(F_64K_ERASE_COMMAND);  // 发送64K erase 命令
  HalExtFlashSendAddr(addr);             // 发送擦除首地址
  
  HalSpiFlashDisable();         // 不选中芯片   
  
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
  
  HalSpiFlashEnable();          // 选中芯片
  
  HalSpiWriteByte(F_CHIP_ERASE_COMMAND);  // 发送Chip erase 命令
  
  HalSpiFlashDisable();         // 不选中芯片   
  
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
  HalSpiFlashEnable(); // 选中芯片
  
  HalSpiWriteByte(F_WREN_COMMAND);  // 发送Write enable 命令
  
  HalSpiFlashDisable(); // 不选中芯片    
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
  HalSpiFlashEnable(); // 选中芯片
  
  HalSpiWriteByte(F_WRDI_COMMAND);  // 发送Write disable 命令
  
  HalSpiFlashDisable(); // 不选中芯片    
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
  
  HalSpiFlashEnable(); // 选中芯片
  
  HalSpiWriteByte(F_RDSR_COMMAND);  // 发送Read status register 命令
  statusRegister = HalSpiReadByte();
  
  HalSpiFlashDisable(); // 不选中芯片  
  
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
  
  HalSpiFlashEnable(); // 选中芯片
  
  HalSpiWriteByte(F_WRSR_COMMAND);  // 发送Write Status命令
  HalSpiWriteByte(writeStatus);     // write data to status register
  
  HalSpiFlashDisable(); // 不选中芯片 
}


/**************************************************************************************************
 * @fn      HalExtFlashSendAddr
 *
 * @brief   Send 24byte address
 *
 * @param   addr - 0x000000 to 0x1FFFFF = 2MB的地址
 *
 * @return  
 **************************************************************************************************/
void HalExtFlashSendAddr(uint32 Addr)
{
  HalSpiWriteByte((Addr & 0xFF0000) >> 16); 
  HalSpiWriteByte((Addr & 0xFF00) >> 8);
  HalSpiWriteByte((Addr & 0xFF));
}
#else

void HalExtFlashInit(void);
uint16 HalExtFlashReadId(void);
uint32 HalExtFlashReadJEDECId(void);
void HalExtFlashBufferRead(uint8 *pBuffer,uint32 readAddress,uint16 readLength);
uint8 HalExtFlashByteRead(uint32 readAddress);

void HalExtFlashByteWrite(uint32 writeAddress,uint8 writeData);
void HalExtFlashBufferWrite(uint8* writebuffer,uint32 writeAddress,uint16 writeLength)
#endif /* HAL_EXTERNAL_FLASH */