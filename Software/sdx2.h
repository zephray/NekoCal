#ifndef _SDX2_H_
#define _SDX2_H_

/*★★★★★★★★★★★★★★★★★★★★★★★★
  《振南的znFAT--嵌入式FAT32文件系统设计与实现》
	 一书[上下册]已经由北航出版社正式出版发行。
	 此书是振南历经3年多时间潜心编著，是现今市面上唯
	 一一套讲述FAT32文件系统、SD卡等嵌入式存储技术的
	 专著。书中还介绍了大量的编程技巧与振南的开发经验。
	 请在各大网络销售平台搜索“znFAT”，即可购买。
	 在全国各大书店也有售。
	 振南的ZN-X开发板，支持51、AVR、STM32(M0/M3/M4)等
	 CPU。此板可与书配套，板上各种精彩的实验实例请详见
	 振南网站www.znmcu.cn
  ★★★★★★★★★★★★★★★★★★★★★★★★*/

/***************************************************************************************
 ★程序模块：【振南ZN-X开发板】上『SD卡2』驱动程序  〖STM32部分:STM32F103RBT6〗
 ★功能描述：实现了SD卡的扇区读写、多扇区读写、扇区擦除、读取总物理扇区数等功能
             此驱动可支持几乎所有的SD卡，包括MMC/SD/SDHC
 ★配套教程与参考资料：
   ●《振南的znFAT--嵌入式FAT32文件系统设计与实验》一书 下册 第11章《SD卡物理驱动》
	 ●《振南的单片机高级外设系列视频教程》之《SD卡专辑》
****************************************************************************************/

#include "main.h"
#include "sd_type.h"

//#define SD2_CS PBout(9)
#define SET_SD2_CS_PIN_HIGH()  GPIOC->BSRRL = GPIO_Pin_11
#define SET_SD2_CS_PIN_LOW()  GPIOC->BSRRH = GPIO_Pin_11

#define TRY_TIME 10   //向SD卡写入命令之后，读取SD卡的回应次数，即读TRY_TIME次，如果在TRY_TIME次中读不到回应，产生超时错误，命令写入失败

//相关宏定义
//-------------------------------------------------------------
#define SD_VER_ERR     0X00
#define SD_VER_MMC     0X01
#define SD_VER_V1      0X02
#define SD_VER_V2      0X03
#define SD_VER_V2HC    0X04

#define INIT_ERROR                  0x01 //初始化错误
#define INIT_CMD0_ERROR             0x02 //CMD0错误
#define INIT_CMD1_ERROR             0x03 //CMD1错误
#define INIT_SDV2_ACMD41_ERROR	    0x04 //ACMD41错误
#define INIT_SDV1_ACMD41_ERROR	    0x05 //ACMD41错误

#define WRITE_CMD24_ERROR           0x06 //写块时产生CMD24错误
#define WRITE_BLOCK_ERROR           0x07 //写块错误

#define READ_BLOCK_ERROR            0x08 //读块错误

#define WRITE_CMD25_ERROR           0x09 //在连续多块写时产生CMD25错误
#define WRITE_NBLOCK_ERROR          0x0A //连续多块写失败

#define READ_CMD18_ERROR            0x0B //在连续多块读时产生CMD18错误
 
#define GET_CSD_ERROR               0x0C //读CSD失败

//-------------------------------------------------------------
UINT8 SD2_Init(void); //SD卡初始化

UINT8 SD2_Write_Sector(UINT32 addr,UINT8 *buffer); //将buffer数据缓冲区中的数据写入地址为addr的扇区中
UINT8 SD2_Read_Sector(UINT32 addr,UINT8 *buffer);	 //从地址为addr的扇区中读取数据到buffer数据缓冲区中
UINT8 SD2_Write_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer); //将buffer数据缓冲区中的数据写入起始地址为addr的nsec个连续扇区中
UINT8 SD2_Read_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer); //将buffer数据缓冲区中的数据写入起始地址为addr的nsec个连续扇区中
UINT8 SD2_Erase_nSector(UINT32 addr_sta,UINT32 addr_end);
UINT32 SD2_GetTotalSec(void); //获取SD卡的总扇区数

#endif
