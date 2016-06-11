#include "sdx2.h"
#include "sd_type.h"
#include "sd2_iospi.h"

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

//变量定义
//--------------------------------------------------------------
extern UINT8 Low_or_High2; //在IOSPI中定义

UINT8 SD2_Addr_Mode=0; //SD2的寻址方式，1为块寻址，0为字节寻址
UINT8 SD2_Ver=SD_VER_ERR; //SD卡2的版本
//---------------------------------------------------------------

#define SD2_SPI_SPEED_HIGH() Low_or_High2=0

#define SD2_SPI_SPEED_LOW()  Low_or_High2=1

#define SD2_SPI_WByte(x) SD2_IOSPI_RWByte(x)

#define SD2_SPI_RByte()  SD2_IOSPI_RWByte(0XFF)

/********************************************************************
 - 功能描述：【振南ZN-X开发板】上『SD卡2』SPI接口初始化
 - 参数说明：无
 - 返回说明：0
 - 注：SPI接口初始化后，首先工作在低速模式。SD卡在初始化的过程中要求
       SPI速度要比较低，原则上不高于400KHZ，经验值为240KHZ。如果发现
			 SD卡初始化不成功，还可继续降低SPI速度，实现速度起决于电路与SD
			 卡品质。
 ********************************************************************/

UINT8 SD2_SPI_Init(void)
{
 SD2_IOSPI_Init(); //SPI接口初始化
	
 return 0;
}

/******************************************************************
 - 功能描述：向SD卡写命令
 - 参数说明：SD卡的命令是6个字节，pcmd是指向命令字节序列的指针
 - 返回说明：命令写入不成功，将返回0xff
 ******************************************************************/

UINT8 SD2_Write_Cmd(UINT8 *pcmd) 
{
 UINT8 r=0,time=0;
 
 SET_SD2_CS_PIN_HIGH();
 SD2_SPI_WByte(0xFF); //发送8个时钟，提高兼容性，如果没有这里，有些SD2卡可能不支持   
	
 SET_SD2_CS_PIN_LOW();
 while(0XFF!=SD2_SPI_RByte()); //等待SD2卡准备好，再向其发送命令

 //将6字节的命令序列写入SD2卡
 SD2_SPI_WByte(pcmd[0]);
 SD2_SPI_WByte(pcmd[1]);
 SD2_SPI_WByte(pcmd[2]);
 SD2_SPI_WByte(pcmd[3]);
 SD2_SPI_WByte(pcmd[4]);
 SD2_SPI_WByte(pcmd[5]);
	
 if(pcmd[0]==0X1C) SD2_SPI_RByte(); //如果是停止命令，跳过多余的字节

 do 
 {  
  r=SD2_SPI_RByte();
  time++;
 }while((r&0X80)&&(time<TRY_TIME)); //如果重试次数超过TRY_TIME则返回错误

 return r;
}

/******************************************************************
 - 功能描述：SD2卡初始化，针对于不同的SD2卡，如MMC、SD2或SD2HC，初始化
             方法是不同的
 - 参数说明：无
 - 返回说明：调用成功，返回0x00，否则返回错误码
 ******************************************************************/

UINT8 SD2_Init(void)
{
 UINT8 time=0,r=0,i=0;
	
 UINT8 rbuf[4]={0};
	
 UINT8 pCMD0[6] ={0x40,0x00,0x00,0x00,0x00,0x95}; //CMD0，将SD2卡从默认上电后的SD2模式切换到SPI模式，使SD2卡进入IDLE状态
 UINT8 pCMD1[6] ={0x41,0x00,0x00,0x00,0x00,0x01}; //CMD1，MMC卡使用CMD1命令进行初始化
 UINT8 pCMD8[6] ={0x48,0x00,0x00,0x01,0xAA,0x87}; //CMD8，用于鉴别SD2卡的版本，并可从应答得知SD2卡的工作电压
 UINT8 pCMD16[6]={0x50,0x00,0x00,0x02,0x00,0x01}; //CMD16，设置扇区大小为512字节，此命令用于在初始化完成之后进行试探性的操作，
                                                          //如果操作成功，说明初始化确实成功
 UINT8 pCMD55[6]={0x77,0x00,0x00,0x00,0x00,0x01}; //CMD55，用于告知SD2卡后面是ACMD，即应用层命令 CMD55+ACMD41配合使用
                                                          //MMC卡使用CMD1来进行初始化，而SD2卡则使用CMD55+ACMD41来进行初始化
 UINT8 pACMD41H[6]={0x69,0x40,0x00,0x00,0x00,0x01}; //ACMD41,此命令用于检测SD2卡是否初始化完成，MMC卡，不适用此命令，针对2.0的SD2卡
 UINT8 pACMD41S[6]={0x69,0x00,0x00,0x00,0x00,0x01}; //ACMD41,此命令用于检测SD2卡是否初始化完成，MMC卡，不适用此命令，针对1.0的SD2卡

 UINT8 pCMD58[6]={0x7A,0x00,0x00,0x00,0x00,0x01}; //CMD58，用于鉴别SD22.0到底是SD2HC，还是普通的SD2卡，二者对扇区地址的寻址方式不同
 
 SD2_SPI_Init(); //SPI相关初始化，SPI工作在低速模式

 SD2_SPI_SPEED_LOW(); //首先将SPI切为低速
	
 SET_SD2_CS_PIN_HIGH(); 
	
 for(i=0;i<0x0f;i++) //首先要发送最少74个时钟信号，这是必须的！激活SD卡
 {
  SD2_SPI_WByte(0xff); //120个时钟
 }

 time=0;
 do
 { 
  r=SD2_Write_Cmd(pCMD0);//写入CMD0
  time++;
  if(time>=TRY_TIME) 
  { 
   return(INIT_CMD0_ERROR);//CMD0写入失败
  }
 }while(r!=0x01);
 
 if(1==SD2_Write_Cmd(pCMD8))//写入CMD8，如果返回值为1，则SD2卡版本为2.0
 {
	rbuf[0]=SD2_SPI_RByte(); rbuf[1]=SD2_SPI_RByte(); //读取4个字节的R7回应，通过它可知此SD2卡是否支持2.7~3.6V的工作电压
	rbuf[2]=SD2_SPI_RByte(); rbuf[3]=SD2_SPI_RByte();
	 
	if(rbuf[2]==0X01 && rbuf[3]==0XAA)//SD2卡是否支持2.7~3.6V
	{		
	 time=0;
	 do
	 {
		SD2_Write_Cmd(pCMD55);//写入CMD55
		r=SD2_Write_Cmd(pACMD41H);//写入ACMD41，针对SD22.0
		time++;
    if(time>=TRY_TIME) 
    { 
     return(INIT_SDV2_ACMD41_ERROR);//对SD22.0使用ACMD41进行初始化时产生错误
    }
   }while(r!=0);	

   if(0==SD2_Write_Cmd(pCMD58)) //写入CMD58，开始鉴别SD22.0
   {
	  rbuf[0]=SD2_SPI_RByte(); rbuf[1]=SD2_SPI_RByte(); //读取4个字节的OCR，其中CCS指明了是SD2HC还是普通的SD2
	  rbuf[2]=SD2_SPI_RByte(); rbuf[3]=SD2_SPI_RByte();	

    if(rbuf[0]&0x40) 
		{
		 SD2_Ver=SD_VER_V2HC; //SD2HC卡	
		 SD2_Addr_Mode=1; //SD2HC卡的扇区寻址方式是扇区地址
		}	
    else SD2_Ver=SD_VER_V2; //普通的SD2卡，2.0的卡包含SD2HC和一些普通的卡				
   }
  }
 }
 else //SD2 V1.0或MMC 
 {
	//SD2卡使用ACMD41进行初始化，而MMC使用CMD1来进行初始化，依此来进一步判断是SD2还是MMC
	SD2_Write_Cmd(pCMD55);//写入CMD55
	r=SD2_Write_Cmd(pACMD41S);//写入ACMD41，针对SD21.0
    
  if(r<=1) //检查返回值是否正确，如果正确，说明ACMD41命令被接受，即为SD2卡
  {
	 SD2_Ver=SD_VER_V1; //普通的SD21.0卡，一般来说容量不会超过2G
			
	 time=0;
	 do
	 {
		SD2_Write_Cmd(pCMD55);//写入CMD55
		r=SD2_Write_Cmd(pACMD41S);//写入ACMD41，针对SD21.0
		time++;
    if(time>=TRY_TIME) 
    { 
     return(INIT_SDV1_ACMD41_ERROR);//对SD21.0使用ACMD41进行初始化时产生错误
    }
   }while(r!=0);			 
  }
  else //否则为MMC	
	{
	 SD2_Ver=SD_VER_MMC; //MMC卡，它不支持ACMD41命令，而是使用CMD1进行初始化
			
	 time=0;
   do
   { 
    r=SD2_Write_Cmd(pCMD1);//写入CMD1
    time++;
    if(time>=TRY_TIME) 
    { 
     return(INIT_CMD1_ERROR);//MMC卡使用CMD1命令进行初始化中产生错误
    }
   }while(r!=0);			
  }
 }
 
 if(0!=SD2_Write_Cmd(pCMD16)) //SD2卡的块大小必须为512字节
 {
	SD2_Ver=SD_VER_ERR; //如果不成功，则此卡为无法识别的卡
	return INIT_ERROR;
 }	
 
 SET_SD2_CS_PIN_HIGH();
 SD2_SPI_WByte(0xFF); //按照SD2卡的操作时序在这里补8个时钟 
 
 SD2_SPI_SPEED_HIGH(); //SPI切到高速
 
 return 0;//返回0,说明复位操作成功
}

/******************************************************************
 - 功能描述：对SD2卡若干个扇区进行擦除，擦除后扇区中的数据大部分情况
             下为全0（有些卡擦除后为全0XFF，如要使用此函数，请确认）
 - 参数说明：addr_sta：开始扇区地址   addr_end：结束扇区地址
 - 返回说明：调用成功，返回0x00，否则返回错误码
 ******************************************************************/

UINT8 SD2_Erase_nSector(UINT32 addr_sta,UINT32 addr_end)
{
 UINT8 r,time;
 UINT8 i=0;
 UINT8 pCMD32[]={0x60,0x00,0x00,0x00,0x00,0xff}; //设置擦除的开始扇区地址
 UINT8 pCMD33[]={0x61,0x00,0x00,0x00,0x00,0xff}; //设置擦除的结束扇区地址
 UINT8 pCMD38[]={0x66,0x00,0x00,0x00,0x00,0xff}; //擦除扇区

 if(!SD2_Addr_Mode) {addr_sta<<=9;addr_end<<=9;} //addr = addr * 512	将块地址（扇区地址）转为字节地址

 pCMD32[1]=addr_sta>>24; //将开始地址写入到CMD32字节序列中
 pCMD32[2]=addr_sta>>16;
 pCMD32[3]=addr_sta>>8;
 pCMD32[4]=addr_sta;	 

 pCMD33[1]=addr_end>>24; //将开始地址写入到CMD32字节序列中
 pCMD33[2]=addr_end>>16;
 pCMD33[3]=addr_end>>8;
 pCMD33[4]=addr_end;	

 time=0;
 do
 {  
  r=SD2_Write_Cmd(pCMD32);
  time++;
  if(time==TRY_TIME) 
  { 
   return(r); //命令写入失败
  }
 }while(r!=0);  
 
 time=0;
 do
 {  
  r=SD2_Write_Cmd(pCMD33);
  time++;
  if(time==TRY_TIME) 
  { 
   return(r); //命令写入失败
  }
 }while(r!=0);  
 
 time=0;
 do
 {  
  r=SD2_Write_Cmd(pCMD38);
  time++;
  if(time==TRY_TIME) 
  { 
   return(r); //命令写入失败
  }
 }while(r!=0);

 return 0; 

}

/****************************************************************************
 - 功能描述：将buffer指向的512个字节的数据写入到SD2卡的addr扇区中
 - 参数说明：addr:扇区地址
             buffer:指向数据缓冲区的指针
 - 返回说明：调用成功，返回0x00，否则返回错误码
 - 注：SD2卡初始化成功后，读写扇区时，尽量将SPI速度提上来，提高效率
 ****************************************************************************/

UINT8 SD2_Write_Sector(UINT32 addr,UINT8 *buffer)	//向SD2卡中的指定地址的扇区写入512个字节，使用CMD24（24号命令）
{  
 UINT8 r,time;
 UINT8 i=0;
 UINT8 pCMD24[]={0x58,0x00,0x00,0x00,0x00,0xff}; //向SD2卡中单个块（512字节，一个扇区）写入数据，用CMD24

 if(!SD2_Addr_Mode) addr<<=9; //addr = addr * 512	将块地址（扇区地址）转为字节地址

 pCMD24[1]=addr>>24; //将字节地址写入到CMD24字节序列中
 pCMD24[2]=addr>>16;
 pCMD24[3]=addr>>8;
 pCMD24[4]=addr;	

 time=0;
 do
 {  
  r=SD2_Write_Cmd(pCMD24);
  time++;
  if(time==TRY_TIME) 
  { 
   return(r); //命令写入失败
  }
 }while(r!=0); 

 while(0XFF!=SD2_SPI_RByte()); //等待SD2卡准备好，再向其发送命令及后续的数据
	
 SD2_SPI_WByte(0xFE);//写入开始字节 0xfe，后面就是要写入的512个字节的数据	
	
 for(i=0;i<4;i++) //将缓冲区中要写入的512个字节写入SD2卡，减少循环次数，提高数据写入速度
 {
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
 }
  
 SD2_SPI_WByte(0xFF); 
 SD2_SPI_WByte(0xFF); //两个字节的CRC校验码，不用关心
       
 r=SD2_SPI_RByte();   //读取返回值
 if((r & 0x1F)!=0x05) //如果返回值是 XXX00101 说明数据已经被SD2卡接受了
 {
  return(WRITE_BLOCK_ERROR); //写块数据失败
 }
 
 while(0xFF!=SD2_SPI_RByte());//等到SD2卡不忙（数据被接受以后，SD2卡要将这些数据写入到自身的FLASH中，需要一个时间）
						                 //忙时，读回来的值为0x00,不忙时，为0xff

 SET_SD2_CS_PIN_HIGH();
 SD2_SPI_WByte(0xFF); //按照SD2卡的操作时序在这里补8个时钟 
 
 return(0);		 //返回0,说明写扇区操作成功
} 

/****************************************************************************
 - 功能描述：读取addr扇区的512个字节到buffer指向的数据缓冲区
 - 参数说明：addr:扇区地址
             buffer:指向数据缓冲区的指针
 - 返回说明：调用成功，返回0x00，否则返回错误码
 - 注：SD2卡初始化成功后，读写扇区时，尽量将SPI速度提上来，提高效率
 ****************************************************************************/

UINT8 SD2_Read_Sector(UINT32 addr,UINT8 *buffer)//从SD2卡的指定扇区中读出512个字节，使用CMD17（17号命令）
{
 UINT8 i;
 UINT8 time,r;
	
 UINT8 pCMD17[]={0x51,0x00,0x00,0x00,0x00,0x01}; //CMD17的字节序列
   
 if(!SD2_Addr_Mode) addr<<=9; //sector = sector * 512	   将块地址（扇区地址）转为字节地址

 pCMD17[1]=addr>>24; //将字节地址写入到CMD17字节序列中
 pCMD17[2]=addr>>16;
 pCMD17[3]=addr>>8;
 pCMD17[4]=addr;	

 time=0;
 do
 {  
  r=SD2_Write_Cmd(pCMD17); //写入CMD17
  time++;
  if(time==TRY_TIME) 
  {
   return(READ_BLOCK_ERROR); //读块失败
  }
 }while(r!=0); 
   			
 while(SD2_SPI_RByte()!= 0xFE); //一直读，当读到0xfe时，说明后面的是512字节的数据了

 for(i=0;i<4;i++)	 //将数据写入到数据缓冲区中
 {	
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
 }

 SD2_SPI_RByte();
 SD2_SPI_RByte();//读取两个字节的CRC校验码，不用关心它们

 SET_SD2_CS_PIN_HIGH();
 SD2_SPI_WByte(0xFF); //按照SD2卡的操作时序在这里补8个时钟 

 return 0;
}

/****************************************************************************
 - 功能描述：向addr扇区开始的nsec个扇区写入数据（★硬件多扇区写入）
 - 参数说明：nsec:扇区数
             addr:开始扇区地址
             buffer:指向数据缓冲区的指针
 - 返回说明：调用成功，返回0x00，否则返回错误码
 - 注：SD2卡初始化成功后，读写扇区时，尽量将SPI速度提上来，提高效率
 ****************************************************************************/

UINT8 SD2_Write_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer)	
{  
 UINT8 r,time;
 UINT32 i=0,j=0;
	
 UINT8 pCMD25[6]={0x59,0x00,0x00,0x00,0x00,0x01}; //CMD25用于完成多块连续写
 UINT8 pCMD55[6]={0x77,0x00,0x00,0x00,0x00,0x01}; //CMD55，用于告知SD2卡后面是ACMD,CMD55+ACMD23
 UINT8 pACMD23[6]={0x57,0x00,0x00,0x00,0x00,0x01};//CMD23，多块连续预擦除

 if(!SD2_Addr_Mode) addr<<=9; 

 pCMD25[1]=addr>>24;
 pCMD25[2]=addr>>16;
 pCMD25[3]=addr>>8;
 pCMD25[4]=addr;

 pACMD23[1]=nsec>>24;
 pACMD23[2]=nsec>>16;
 pACMD23[3]=nsec>>8;
 pACMD23[4]=nsec; 

 if(SD2_Ver!=SD_VER_MMC) //如果不是MMC卡，则首先写入预擦除命令，CMD55+ACMD23，这样后面的连续块写的速度会更快
 {
	SD2_Write_Cmd(pCMD55);
	SD2_Write_Cmd(pACMD23);
 }

 time=0;
 do
 {  
  r=SD2_Write_Cmd(pCMD25);
  time++;
  if(time==TRY_TIME) 
  { 
   return(WRITE_CMD25_ERROR); //命令写入失败
  }
 }while(r!=0); 

 while(0XFF!=SD2_SPI_RByte()); //等待SD2卡准备好，再向其发送命令及后续的数据

 for(j=0;j<nsec;j++)
 {
  SD2_SPI_WByte(0xFC);//写入开始字节 0xfc，后面就是要写入的512个字节的数据	
	
  for(i=0;i<4;i++) //将缓冲区中要写入的512个字节写入SD2卡
  {
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
   SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));SD2_SPI_WByte(*(buffer++));
  }
  
  SD2_SPI_WByte(0xFF); 
  SD2_SPI_WByte(0xFF); //两个字节的CRC校验码，不用关心
       
  r=SD2_SPI_RByte();   //读取返回值
  if((r & 0x1F)!=0x05) //如果返回值是 XXX00DELAY_TIME1 说明数据已经被SD2卡接受了
  {
   return(WRITE_NBLOCK_ERROR); //写块数据失败
  }
 
  while(0xFF!=SD2_SPI_RByte());//等到SD2卡不忙（数据被接受以后，SD2卡要将这些数据写入到自身的FLASH中，需要一个时间）
						                  //忙时，读回来的值为0x00,不忙时，为0xff
 }

 SD2_SPI_WByte(0xFD);

 while(0xFF!=SD2_SPI_RByte());

 SET_SD2_CS_PIN_HIGH();//关闭片选

 SD2_SPI_WByte(0xFF);//按照SD2卡的操作时序在这里补8个时钟

 return(0);		 //返回0,说明写扇区操作成功
} 

/****************************************************************************
 - 功能描述：读取addr扇区开始的nsec个扇区的数据（★硬件多扇区读取）
 - 参数说明：nsec:扇区数
             addr:开始扇区地址
             buffer:指向数据缓冲区的指针
 - 返回说明：调用成功，返回0x00，否则返回错误码
 - 注：SD2卡初始化成功后，读写扇区时，尽量将SPI速度提上来，提高效率
 ****************************************************************************/

UINT8 SD2_Read_nSector(UINT32 nsec,UINT32 addr,UINT8 *buffer)
{
 UINT8 r,time;
 UINT32 i=0,j=0;
	
 UINT8 pCMD18[6]={0x52,0x00,0x00,0x00,0x00,0x01}; //CMD18的字节序列
 UINT8 pCMD12[6]={0x1C,0x00,0x00,0x00,0x00,0x01}; //CMD12，强制停止命令
   
 if(!SD2_Addr_Mode) addr<<=9; //sector = sector * 512	   将块地址（扇区地址）转为字节地址

 pCMD18[1]=addr>>24; //将字节地址写入到CMD17字节序列中
 pCMD18[2]=addr>>16;
 pCMD18[3]=addr>>8;
 pCMD18[4]=addr;	

 time=0;
 do
 {  
  r=SD2_Write_Cmd(pCMD18); //写入CMD18
  time++;
  if(time==TRY_TIME) 
  {
   return(READ_CMD18_ERROR); //写入CMD18失败
  }
 }while(r!=0); 
 
 for(j=0;j<nsec;j++)
 {  
  while(SD2_SPI_RByte()!= 0xFE); //一直读，当读到0xfe时，说明后面的是512字节的数据了
 
  for(i=0;i<4;i++)	 //将数据写入到数据缓冲区中
  {	
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
   *(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();*(buffer++)=SD2_SPI_RByte();
  }
 
  SD2_SPI_RByte();
  SD2_SPI_RByte();//读取两个字节的CRC校验码，不用关心它们
 }

 SD2_Write_Cmd(pCMD12); //写入CMD12命令，停止数据读取 

 SET_SD2_CS_PIN_HIGH();
 SD2_SPI_WByte(0xFF); //按照SD2卡的操作时序在这里补8个时钟 

 return 0;
}

/****************************************************************************
 - 功能描述：获取SD2卡的总扇区数（通过读取SD2卡的CSD寄器组计算得到总扇区数）
 - 参数说明：无
 - 返回说明：返回SD2卡的总扇区数
 - 注：无
 ****************************************************************************/

UINT32 SD2_GetTotalSec(void)
{
 UINT8 pCSD[16];
 UINT32 Capacity;  
 UINT8 n,i;
 UINT16 csize; 

 UINT8 pCMD9[6]={0x49,0x00,0x00,0x00,0x00,0x01}; //CMD9	

 if(SD2_Write_Cmd(pCMD9)!=0) //写入CMD9命令
 {
	return GET_CSD_ERROR; //获取CSD时产生错误
 }

 while(SD2_SPI_RByte()!= 0xFE); //一直读，当读到0xfe时，说明后面的是16字节的CSD数据

 for(i=0;i<16;i++) pCSD[i]=SD2_SPI_RByte(); //读取CSD数据

 SD2_SPI_RByte();
 SD2_SPI_RByte(); //读取两个字节的CRC校验码，不用关心它们

 SET_SD2_CS_PIN_HIGH();
 SD2_SPI_WByte(0xFF); //按照SD2卡的操作时序在这里补8个时钟 
	
 //如果为SDHC卡，即大容量卡，按照下面方式计算
 if((pCSD[0]&0xC0)==0x40)	 //SD22.0的卡
 {	
	csize=pCSD[9]+(((UINT16)(pCSD[8]))<<8)+1;
  Capacity=((UINT32)csize)<<10;//得到扇区数	 		   
 }
 else //SD1.0的卡
 {	
	n=(pCSD[5]&0x0F)+((pCSD[10]&0x80)>>7)+((pCSD[9]&0x03)<<1)+2;
	csize=(pCSD[8]>>6)+((UINT16)pCSD[7]<<2)+((UINT16)(pCSD[6]&0x03)<<0x0A)+1;
	Capacity=(UINT32)csize<<(n-9);//得到扇区数   
 }
 return Capacity;
}



