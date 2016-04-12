#ifndef _MYTYPE_H_
#define _MYTYPE_H_

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

/*============================================================================================
  此文件用于进行数据类型的重新定义，请按照实际使用的硬件平台上的数据类型对宏进行重新定义
=============================================================================================*/

//加入类型相关头文件 比如AVR GCC中的 ROM类型在<AVR/pgmspace.h>中定义

#define UINT8   unsigned char
#define UINT16  unsigned short
#define UINT32  unsigned int 

#define INT8    char 
#define INT16   short
#define INT32   int

#define ROM_TYPE_UINT8   const unsigned char
#define ROM_TYPE_UINT16  const unsigned short
#define ROM_TYPE_UINT32  const unsigned int

#endif
