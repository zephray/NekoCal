#include "sd2_iospi.h"
#include "main.h"
#include "sd_type.h"

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
 ★程序模块：【振南ZN-X开发板】上『SD卡2』IO模拟SPI  〖STM32部分:STM32F103RBT6〗
 ★功能描述：IO模拟SPI，可调节SPI通信速度
 ★配套教程与参考资料：
   ●《振南的znFAT--嵌入式FAT32文件系统设计与实验》一书 下册 第11章《SD卡物理驱动》
	 ●《振南的单片机高级外设系列视频教程》之《SD卡专辑》
****************************************************************************************/

UINT8 Low_or_High2=0; //用于区分SPI为低速还是高速
                     //若此变量为1，则在IO模拟SPI时通过加入delay来降低速度
										 //DELAY_TIME越大，速度越慢
										 //SPI切换速度，主要是因为SD卡在初始化时要求较低的通信速度
										 //如果速度过高，则可能造成初始化失败。初始化成功之后，则要
										 //尽量提高SPI速度，以提高扇区读写的速度

void delay_us(u32 us)
{
    u32 time=us;   
    while(--time);  
}

/***********************************************************************
 - 功能描述：【振南ZN-X开发板】上『SD卡2』IO模拟SPI接口IO配置与初始化
 - 参数说明：无
 - 返回说明：0
 ***********************************************************************/

UINT8 SD2_IOSPI_Init(void) //对SPI IO有关初始化
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD,ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_11 |GPIO_Pin_12);
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
        GPIO_SetBits(GPIOD, GPIO_Pin_2);
        
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/***********************************************************************
 - 功能描述：【振南ZN-X开发板】上『SD卡2』IO模拟SPI字节读写
 - 参数说明：x:要发送的字节
 - 返回说明：读取到的字节
 ***********************************************************************/
 
UINT8 SD2_IOSPI_RWByte(UINT8 x) //IO模拟SPI
{
 UINT8 rbyte=0;	
	
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);
 
 if (x&0x80) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x80)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x80;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);

 if (x&0x40) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x40)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x40;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);

 if (x&0x20) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x20)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x20;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);

 if (x&0x10) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x10)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x10;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);
 
 if (x&0x08) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x08)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x08;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);
 
 if (x&0x04) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x04)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x04;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);
 
 if (x&0x02) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x02)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x02;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);
 
 if (x&0x01) SET_SPI_SI_PIN_HIGH(); else SET_SPI_SI_PIN_LOW();
 //SET_SPI_SI_PIN((x&0x01)?1:0);
 SET_SPI_SCL_PIN_LOW(); 
 if(Low_or_High2) delay_us(DELAY_TIME);
 if(GET_SPI_SO_PIN()) rbyte|=0x01;
 SET_SPI_SCL_PIN_HIGH();
 if(Low_or_High2) delay_us(DELAY_TIME);
 
 return rbyte;
}