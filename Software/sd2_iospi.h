#ifndef _SD2_IOSPI_H_
#define _SD2_IOSPI_H_

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
 ★程序模块：【振南ZN-X开发板】上『SD卡1』IO模拟SPI  〖STM32部分:STM32F103RBT6〗
 ★功能描述：IO模拟SPI，可调节SPI通信速度
 ★配套教程与参考资料：
   ●《振南的znFAT--嵌入式FAT32文件系统设计与实验》一书 下册 第11章《SD卡物理驱动》
	 ●《振南的单片机高级外设系列视频教程》之《SD卡专辑》
****************************************************************************************/

#include "sd_type.h"

//====================目标平台用户代码==============

//将IO操作与下面的宏连接
//#define SET_SPI_SCL_PIN(val) PBout(13)=val //操作SPI的时钟
//#define SET_SPI_SI_PIN(val)  PBout(15)=val //操作SPI的主出从入
//#define GET_SPI_SO_PIN()     PBin(14)      //获取SPI主入从出
#define GET_SPI_SO_PIN()        (GPIOC->IDR & GPIO_Pin_8)
#define SET_SPI_SI_PIN_HIGH()   GPIOD->BSRRL = GPIO_Pin_2
#define SET_SPI_SI_PIN_LOW()    GPIOD->BSRRH = GPIO_Pin_2
#define SET_SPI_SCL_PIN_HIGH()  GPIOC->BSRRL = GPIO_Pin_12
#define SET_SPI_SCL_PIN_LOW()   GPIOC->BSRRH = GPIO_Pin_12
   
#define DELAY_TIME 500 //SPI的延时参数，此值越大，SPI速度越慢

UINT8 SD1_IOSPI_Init(void); //IO模拟SPI相关初始化
UINT8 SD1_IOSPI_RWByte(UINT8 x); //IO模拟SPI字节读写

#endif