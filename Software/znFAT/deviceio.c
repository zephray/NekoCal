#include "mytype.h"
#include "config.h"
#include "deviceio.h"

#include "sdcard.h" //存储设备驱动头文件

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

struct znFAT_IO_Ctl ioctl; //用于扇区读写的IO控制，尽量减少物理扇区操作，提高效率
extern UINT8 Dev_No; //设备号
extern UINT8 *znFAT_Buffer;

/*******************************************************************
 功能：存储设备初始化
 形参：无形参
 返回：存储设备初始化错误信息
 详解：znFAT支持多设备，即同时挂接多种存储设备，所有存储设备的初始
       化均在这里完成。返回值的某一位为1，则说明相应的存储设备初始
       化失败。例如：设备0失败，而其它设备均成功，则返回值为0X01、
       设备1与设备2失败，而其它设备成功，则返回值为0X06。为了使返回
       值如实的反映相应设备的初始化状态，请注意存储设备初始化函数
       调用的顺序。
*******************************************************************/

UINT8 znFAT_Device_Init(void) 
{
 UINT8 res=0,err=0;

 ioctl.just_dev=0;
 ioctl.just_sec=0;

 //以下为各存储设备的初始化函数调用，请沿袭以下格式

 res=SD_Init();	
 if(res!=SD_OK) err|=0X01;

 //res=Device1_Init();
 //if(res) err|=0X02;

 return err; //返回错误码，如果某一设备初始化失败，则err相应位为1
}

/*****************************************************************************
 功能：znFAT的存储设备物理扇区读取驱动接口
 形参：addr:物理扇区地址 buffer:数据缓冲区指针
 返回：0
 详解：各存储设备的物理扇区读取驱动函数放到case的各个分支中，分支序号就是此设
       备的设备号。 
*****************************************************************************/
UINT8 znFAT_Device_Read_Sector(UINT32 addr,UINT8 *buffer)
{
 if(buffer==znFAT_Buffer) //如果是针对znFAT内部缓冲区的操作
 {                        
  if(ioctl.just_dev==Dev_No  //如果现在要读取的扇区与内部缓冲所对应的扇区（即最近一次操作的扇区）是同一扇区
     && (ioctl.just_sec==addr && 0!=ioctl.just_sec)) //则不再进行读取，直接返回
  {                                           
   return 0;      
  }
  else //否则，就将最近一次操作的扇区标记为当前扇区
  {
   ioctl.just_dev=Dev_No; 
   ioctl.just_sec=addr; 
  }
 }

 switch(Dev_No) //有多少个存储设备，就有多少个case分支
 {
  case 0:
	       SD_ReadBlock(buffer, addr*512, 512);
  //case 1:
//	     while(SD2_Read_Sector(addr,buffer));
//		   break;
  //case...
  
 }

 return 0;
}

/*****************************************************************************
 功能：znFAT的存储设备物理扇区写入驱动接口
 形参：addr:物理扇区地址 buffer:数据缓冲区指针
 返回：0
 详解：各存储设备的物理扇区写入驱动函数放到case的各个分支中，分支序号就是此设
       备的设备号。 
*****************************************************************************/
UINT8 znFAT_Device_Write_Sector(UINT32 addr,UINT8 *buffer) 
{
 if(buffer==znFAT_Buffer) //如果数据缓冲区是内部缓冲
 {
  ioctl.just_dev=Dev_No; //更新为当前设备号
  ioctl.just_sec=addr; //更新为当前操作的扇区地址	
 }

 switch(Dev_No)
 {
  case 0:
	      SD_WriteBlock(buffer, addr*512, 512);
  //case 1:
//	     while(SD2_Write_Sector(addr,buffer));
//		   break;
  //case...
  
 } 

 return 0;
}

/***********************************************************************************
 功能：znFAT的存储设备物理扇区连续读取驱动接口
 形参：nsec:要读取的扇区数 addr:连续扇区读取时的开始扇区地址 buffer:数据缓冲区指针
 返回：0
 详解：此函数接口在znFAT中用于完成若干个连续扇区的一次性读取。此函数接口的实现有两种
       模式 1、单扇区读取驱动+循环 2、存储设备硬件上的连续扇区读取 使用2比1的效率要
       得多，在高速且数据量比较大的应用场合，建议使用者提供硬件级的连续扇区读取函数
***********************************************************************************/
UINT8 znFAT_Device_Read_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer)
{
 UINT32 i=0;

 if(0==nsec) return 0;

 #ifndef USE_MULTISEC_R //此宏决定是否使用硬件级连续扇区读取驱动

  switch(Dev_No)
  {
   case 0:
          for(i=0;i<nsec;i++) //如果不使用硬件级连续扇区读取，则使用单扇区读取+循环的方式
          {
           SD_ReadBlock(buffer, (addr+i)*512, 512);
           buffer+=512;
          }
          break;
  // case 1:
   //       for(i=0;i<nsec;i++) //如果不使用硬件级的连续扇区读取，则使用单扇区读取+循环的方式
  //        {
   //        while(SD2_Read_Sector(addr+i,buffer));
   //        buffer+=512;
   //       }
  //        break;
  }

 #else

  switch(Dev_No)
  {
   case 0:
          while(SD2_Read_nSector(nsec,addr,buffer));
   //case 1:
          //while(Device1_Read_nSector(nsec,addr,buffer));
  }
 #endif

 return 0;
}

/***********************************************************************************
 功能：znFAT的存储设备物理扇区连续写入驱动接口
 形参：nsec:要写入的扇区数 addr:连续扇区写入时的开始扇区地址 buffer:数据缓冲区指针
 返回：0
 详解：此函数接口与上面的连续读取驱动接口同理。
***********************************************************************************/
UINT8 znFAT_Device_Write_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer)
{
 UINT32 i=0;
 
 if(0==nsec) return 0;

 #ifndef USE_MULTISEC_W //此宏决定是否使用硬件的连续扇区写入函数

  switch(Dev_No)
  {
   case 0:
          for(i=0;i<nsec;i++)
          {
            SD_WriteBlock(buffer, (addr+i)*512, 512);
           buffer+=512;
          }
		  break;
   //case 1:
   //       for(i=0;i<nsec;i++)
   //       {
   //        while(SD2_Write_Sector(addr+i,buffer));
   //        buffer+=512;
    //      }
//		  break;
  }
  
 #else

  switch(Dev_No)
  {
   case 0:
          while(SD2_Write_nSector(nsec,addr,buffer));
		  break;
   //case 1:
          //while(Device1_Write_nSector(nsec,addr,buffer));
  }

 #endif

 return 0; 
}

/***********************************************************************************
 功能：znFAT的存储设备物理扇区连续清0驱动接口
 形参：nsec:要清0的扇区数 addr:连续扇区清0的开始扇区地址
 返回：0
 详解：在格式化功能中，最耗时的就是对FAT表扇区扇区的逐个清0，使用硬件级的连续扇区清0
       驱动函数将可以很大程度上加速这一过程。
***********************************************************************************/
UINT8 znFAT_Device_Clear_nSector(UINT32 nsec,UINT32 addr)
{
 #ifndef USE_MULTISEC_CLEAR  //此宏决定是否使用硬件级连续扇区清0函数，其主要用于格式化过程中FAT表的清0
  UINT32 i=0;

  for(i=0;i<512;i++) //清空内部缓冲区，用于连续扇区清0
  {
   znFAT_Buffer[i]=0;
  }

  switch(Dev_No)
  {
   case 0:
          for(i=0;i<nsec;i++)
          {
            SD_WriteBlock(znFAT_Buffer, (addr+i)*512, 512);
          }
          break;
   //case 1:
   //       for(i=0;i<nsec;i++)
   //       {
    //       while(SD2_Write_Sector(addr+i,znFAT_Buffer));
   //       }
   //       break;
  }

 #else

  switch(Dev_No)
  {
   case 0:
          return Device0_Clear_nSector(nsec,addr); //在使用硬件级的连续扇区清0的时候，请将连续扇区清0函数写在这里
   //case 1:
          //return Device1_Clear_nSector(nsec,addr);
  }

 #endif

 ioctl.just_dev=Dev_No; //更新为当前设备号
 ioctl.just_sec=(addr+nsec-1); //更新为当前操作的扇区地址	 

 return 0;  
}

//==============================【以上是设备驱动层】========================================================================

