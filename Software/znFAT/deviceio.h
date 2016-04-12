#ifndef _DEVICE_IO_H_
#define _DEVICE_IO_H_

#include "mytype.h"
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

struct znFAT_IO_Ctl //底层驱动接口的IO频度控制体 
{
 UINT32 just_sec;
 UINT8  just_dev;
};

UINT8 znFAT_Device_Init(void);
UINT8 znFAT_Device_Read_Sector(UINT32 addr,UINT8 *buffer);
UINT8 znFAT_Device_Write_Sector(UINT32 addr,UINT8 *buffer);
UINT8 znFAT_Device_Read_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer);
UINT8 znFAT_Device_Write_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer);
UINT8 znFAT_Device_Clear_nSector(UINT32 nsec,UINT32 addr);

#endif

