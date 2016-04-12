#include "znfat.h"
#include "template.h"
#include "gb2uni.h"
#include "deviceio.h"

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

/*==================================================================================
  振南的znFAT 一种较为完备的嵌入式平台上的FAT32文件系统解决方案 V11.21
               敬请关注 振南的znFAT 网站 www.znfat.com
	           QQ 987582714
===================================================================================*/
/*----------------------------------------------------------------------------/
/  Here is znFAT -- a complete FAT32 FileSystem Solution  ver 11.21   
/-----------------------------------------------------------------------------/
/ znFAT is a complete FAT FileSystem Code Module for embeded system platform.
/ znFAT is Developped and Coded By ZN China who own the full copyright on it.
/ You are allowed to use it for study,research,and commerce purpose,to modify
/ the code and publish it freely.
/ znFAT is pleasure to be recommended to more E-amateur and Engineer.Thanks!!
/  
/              Copyright (C) 2010, ZN, all right reserved.
/
/               Technology Support http://www.znfat.com
/                  Welcome to ZN's FAT32 FS World!!
/-----------------------------------------------------------------------------/

/-----------------------------------------------------------------------------/
Function Table(功能函数表): 
 znFAT_Device_Init  : Storage device initialize (存储设备初始化)
 znFAT_Init         : File system initialize (文件系统初始化)
 znFAT_Select_Device: Select storage device (选择存储设备)
 znFAT_Open_File    : Open a file (打开文件)
 znFAT_ReadData     : Read data in a file (读取文件数据)
 znFAT_ReadDataX    : Read data and redirect it (读取文件数据+数据重定向)
 znFAT_Enter_Dir    : Enter a dir (进入目录)
 znFAT_WriteData    : Write data to a file,append it to the end (向文件写入数据)
 znFAT_Modify_Data  : Modify data in a file (修改文件数据)
 znFAT_Dump_Data    : Dump data of a file (丢弃，截断文件数据)
 znFAT_Create_File  : Create file (创建文件)
 znFAT_Create_Dir   : Create dir (创建目录)
 znFAT_Delete_File  : Delete file (删除文件)
 znFAT_Delete_Dir   : Delete dir (删除目录)
 znFAT_Make_FS      : Make a FAT32 FS on a storage device,even Format (格式化)
 znFAT_Close_File   : Close file (关闭文件)
 znFAT_Flush_FS     : Flush FS,Update FS information from RAM to Disk (刷新FS)
/----------------------------------------------------------------------------/
Configuration for znFAT's Functions,is necessary before the usage of them.
        (对znFAT中功能函数的配置，在使用它们之前请务必对其进行配置)

When you use a function in znFAT(but znFAT_Device_Init znFAT_Init and znFAT_\
Select_Device,because them must be used in every project),you must firstly
OPEN the MACROS as "#define ZNFAT_XXXX",So the relevant code of the function
is added to the compiling.Or,You will get a warning like "XXXX is undefined .."
For example:You now wanna use znFAT_Open_File,You must open the header file
config.h,OPEN the MACRO ZNFAT_OPEN_FILE,even delete the "//" before it.

(当你要使用znFAT中一个函数时(znFAT_Device_Init znFAT_Init and znFAT_Select_\
Device这三个函数除外，因为它们在任何时候都是必然被使用的)，必须首先把相应的宏
打开，比如"#define ZNFAT_XXXX"，这样与这个函数相关的代码才会被加入到编译之中，
否则，你可能会得到像这样的警告"XXXX is undefined.."。举例说明：要使用znFAT_\
Open_File函数，你必须要打开config.h，打开里面对应的宏ZNFAT_OPEN_FILE，即去掉
前面的"//"。)

Option for znFAT (znFAT中的工作方式选择)

 USE_LFN : Use the Long File Name (使用长文件名)
  MAX_LFN_LEN : Define the max Long File Name length (定义长文件名最大长度)
  USE_OEM_CHAR: Use OEM charactor in LFN,as CHN (在长文件名中使用OEM字符，如中文)
 USE_MULTISEC_R : Use hardware continuous Sector Read (使用硬件级连续扇区读)
 USE_MULTISEC_W : Use hardware continuous Sector Write (使用硬件级连续扇区写)
 USE_MULTISEC_CLEAR : Use hardware continuous Sector Clear (使用硬件级连续扇区清0)
 RT_UPDATE_FSINFO : Realtime update the information of FS (实时刷新文件系统信息)
 RT_UPDATE_FILESIZE : Realtime update the file size (实时更新文件大小)
 RT_UPDATE_CLUSTER_CHAIN : Realtime update the cluster chain (实时更新簇链)
                           if not define it ,znFAT will use CCCB algorithm
						   to store the cluster chain in CCCB buffer 
						   (如果没有定义这个宏，则znFAT在数据读写过程中，使用
						    CCCB算法来将簇链临时性的存储在RAM的缓冲区中，这样
							是为了提高数据读写速率，弊端在于缓冲中的簇链不回
							写到物理扇区中，会造成数据丢失，因此要及时回写。)
  CCCB_LEN (XXX) : Define the size of CCCB buffer (定义CCCB 缓冲区大小)
  USE_ALONE_CCCB : Use the alone CCCB buffer (使用独立CCCB 缓冲区，即每一个文件
                                              都会分配一个独立的CCCB缓冲区，数据
											  读写时，各文件各用各自的CCCB缓冲，
											  互不干涉。)
		           if not define it,use the Shared CCCB buffer
				                             (如果此宏没有定义，则znFAT会使用共
											  CCCB 缓冲区，主要是为了节省RAM资
											  源，但这样必然招致多个文件对共享
											  CCCB缓冲的抢夺，在同时操作文件
											  较多的时候，数据读写效率并不高。)
 USE_EXCHANGE_BUFFER : Use the exchange buffer and relevant algorithm (EXB)
                                             (使用EXB扇区交换缓冲及其算法，EXB
											  缓冲是为了减少扇区数据的读-改-写
											  操作次数)
  USE_ALONE_EXB	: Use alone exchange buffer (使用独立的EXB扇区交换缓冲，即每个
                                             文件均有各自独立的EXB缓冲，这样作
											 会极大的增大RAM开销)
	              if no define it,use the Shared exchange buffer
				                            (如果此宏没有定义，则使用共享EXB
											 缓冲，多个文件分时分享一个扇区交换
											 缓冲，会造成争抢问题)
 Data_Redirect : Redirect function name macro defination for read_dataX
                                            (为znFAT_ReadDataX函数所定义的数据
											 重定向单位字节处理函数)

/---------------------------------------------------------------------------*/


//#pragma udata directive
//#pragma udata BUFFER
UINT8 tmpBuf[ZNFAT_BUF_SIZE];
//#pragma udata

UINT8 *znFAT_Buffer=tmpBuf; //znFAT的内部缓冲区，使用者不可私自使用
                            //先定义tmpBuf，再用znFAT_Buffer指向它，是因为在一些架构的CPU中
                            //受限于RAM的特殊结构，只能用指针来访问大数组，比如PIC

//--------------------------------------------------------------------------------------------------
struct znFAT_Init_Args *pInit_Args; //初始化参数结构体指针，用以指向某一存储设备的初始化参数集合
                                    //使用之前*必须*先指向结构化变量
extern struct znFAT_IO_Ctl ioctl; 

UINT8 Dev_No=0; //设备号，用于实现多设备

//--------------------------------------------------------------------------------------------------

struct FileInfo *just_file=(struct FileInfo *)0; //用于记录最近操作的文件

//-------------------SCCCB相关变量定义----------------------
#ifndef RT_UPDATE_CLUSTER_CHAIN //用于定义共享CCCB的变量及缓冲实体
#ifndef USE_ALONE_CCCB
UINT32 scccb_buf[CCCB_LEN]; //CCCB的缓冲区，以连续簇段的方式来记录簇链
UINT8  scccb_counter=0; 
UINT32 scccb_curval=0;

UINT8  scccb_curdev=(UINT8)(-1);
#endif
#endif

#ifndef RT_UPDATE_CLUSTER_CHAIN

#ifndef USE_ALONE_CCCB //不使用独立簇链缓冲，而是使用共享簇链缓冲，以下变量用于完成共享CCCB使用过程中的争抢
UINT32 *pcccb_buf=scccb_buf;
UINT32 *pcccb_curval=&scccb_curval;
UINT8  *pcccb_counter=&scccb_counter;

UINT8  *pcccb_curdev=&scccb_curdev;
struct FileInfo *pcccb_cur_oc=(struct FileInfo *)0;
struct znFAT_Init_Args *pcccb_cur_initargs=(struct znFAT_Init_Args *)0;
#else
UINT32 *pcccb_buf=(UINT32 *)0;
UINT32 *pcccb_curval=(UINT32 *)0;
UINT8  *pcccb_counter=(UINT8 *)0;
#endif
 
UINT8 get_next_cluster_in_cccb=0; //用以标志是否在CCCB中查找下一簇
#endif
//----------------------------------------------------------

//------------------EXB相关变量定义-------------------------
#ifdef USE_EXCHANGE_BUFFER
#ifndef USE_ALONE_EXB
//#pragma udata directive
//#pragma udata SEXB_BUF
UINT8  sexb_buf[ZNFAT_BUF_SIZE];
//#pragma udata

UINT8  sexb_cur_dev=(UINT8)(-1);
UINT32 sexb_cur_sec=0;
struct FileInfo *psexb_cur_oc=(struct FileInfo *)0; //指示当前EXB被哪个文件占用
#endif
#endif

#ifdef USE_EXCHANGE_BUFFER

#ifndef USE_ALONE_EXB
UINT8 *pexb_buf=sexb_buf;
#else
UINT8 *pexb_buf=(UINT8 *)0;
#endif

#endif
//----------------------------------------------------------
//====================一些常用函数================================================================

//znFAT中使用到的公共函数，其中包含了对ROM类型 字节、字、双字的读取以及ROM到RAM之间拷贝操作
//如果要使长文件名或格式化功能，则这些ROM相关的函数是必须正确予以实现的

UINT8 Memory_Set(UINT8 *pmem,UINT32 len,UINT8 value)
{
 UINT32 i=0;
 for(i=0;i<len;i++)
 { 
  pmem[i]=value;
 }

 return 0;
}

UINT8 Memory_Compare(UINT8 *psmem,UINT8 *pdmem,UINT32 len)
{
 UINT32 i=0;

 for(i=0;i<len;i++)
 {
  if(psmem[i]!=pdmem[i])
  {
   return 0;
  }
 }
 return 1;
}

UINT8 * Memory_Copy(UINT8 *pdmem,UINT8 *psmem,UINT32 len)
{
 UINT32 i=0;

 for(i=0;i<len;i++)
 {
  pdmem[i]=psmem[i];
 }

 return pdmem;
}

INT8 * StringCopy(INT8 *dest_str,INT8 *src_str)
{
 UINT8 i=0;

 while('\0'!=src_str[i])
 {
  dest_str[i]=src_str[i];
  i++;
 }

 dest_str[i]='\0';

 return dest_str;
}

UINT32 StringLen(INT8 *pstr)
{
 UINT32 len=0;
 while('\0'!=pstr[len]) 
 {
  len++;
 }
 return len;
}

UINT32 WStringLen(UINT16 *str)
{
 UINT32 i=0;
 while(0!=str[i])
 {
  i++;
 }

 return i;
}

//=============================FLASHROM 操作相关函数=====================

UINT8 PGM_BYTE_FUN(ROM_TYPE_UINT8 *ptr)
{
 return *(ptr); 
}

UINT16 PGM_WORD_FUN(ROM_TYPE_UINT16 *ptr)
{
 return *(ptr);
}

UINT32 PGM_DWORD_FUN(ROM_TYPE_UINT32 *ptr)
{
 return *(ptr);
}

UINT8 * PGM2RAM(UINT8 *pdmem,ROM_TYPE_UINT8 *psmem,UINT32 len)
{
 UINT32 i=0;

 for(i=0;i<len;i++)
 {
  pdmem[i]=PGM_BYTE_FUN((psmem+i));
 }

 return pdmem;
}

//================================================================================================

/*****************************************************************************
 功能：选定一个存储设备
 形参：devno:设备号 pinitargs:指向存储设备所对应的文件系统初始化参数集合的指针
 返回：0
 详解：znFAT是支持多设备的，因此在对设备进行文件系统相关操作之前，必须要先选定
       一个存储设备。此函数对Dev_No这个全局的用于区分存储设备驱动的变量进行设
       定，同时将pInit_Args指向存储设备相应的文件系统初始化参数集合
*****************************************************************************/
UINT8 znFAT_Select_Device(UINT8 devno,struct znFAT_Init_Args *pinitargs) //选择设备
{
 pInit_Args=pinitargs; //将znFAT的初始化参数集合指针指向设备的初始化参数集合

 Dev_No=devno; //设置设备号

 return 0;
}

/***********************************************************************************
 功能：由一个小端排列的字节序列，计算得到其在某一字符长度下所表达的整型值
 形参：dat:指向字节序列的指针 len:将字节序列的前len个字节计算整型值
 返回：计算得到的整型值
 详解：这一函数是屏蔽不同CPU在大小端上的差异的主要手段。比如对于一个小端的4字节序列
       unsigned char *p={0X12,0X34,0X56,0X78} 如果我们想把它合成为一个4字节整型，如
       unsigned long，那么可以这样来作 *((unsigned long *)p)，但对于不同的CPU，因变
       量在RAM中的字节排列顺序不同，即大小端问题，则其所表达的整型值可能为0X12345678
       或 0X78563412，这将会出现错误。为了屏蔽这种差异，引入了此函数，通过对字节序列
       进行计算，最后将可以得到正确的值，对于上例中的字节序列，通过此函数的计算Bytes
       2Value(p,4)值一定为0X12345678。
***********************************************************************************/
UINT32 Bytes2Value(UINT8 *dat,UINT8 len)
{
 UINT32 temp=0;

 if(len>=1) temp|=((UINT32)(dat[0]))    ;
 if(len>=2) temp|=((UINT32)(dat[1]))<<8 ;
 if(len>=3) temp|=((UINT32)(dat[2]))<<16;
 if(len>=4) temp|=((UINT32)(dat[3]))<<24;

 return temp;
}

/***********************************************************************************
 功能：查找FSINFO扇区的物理地址
 形参：fsinfosec:一个指向用于记录FSINFO扇区物理地址的变量的指针
 返回：运行结果，成功或失败
 详解：一般来说FSINFO扇区在DBR扇区的后一个扇区，但不乏有特殊情况，因此这里对DBR+1到
       FAT1的前一个扇区进行遍历，以FSINFO扇区的标志字作为认定是FSINFO扇区的条件。
***********************************************************************************/
UINT8 Find_FSINFO_Sec(UINT32 *fsinfosec) //寻找FSINFO扇区，里面有剩余簇数与可用的空簇
{
 UINT32 iSec=0;
 struct FSInfo *pfsinfo;

 UINT8 head[4]={'R','R','a','A'};
 UINT8 sign[4]={'r','r','A','a'}; //FSINFO扇区的标志

 for(iSec=(pInit_Args->BPB_Sector_No+1);iSec<(pInit_Args->FirstFATSector);iSec++)
 {
  znFAT_Device_Read_Sector(iSec,znFAT_Buffer);
  pfsinfo=((struct FSInfo *)znFAT_Buffer);
  if(Memory_Compare(pfsinfo->Head,head,4) //判断扇区是否是FSINFO扇区
	 && Memory_Compare(pfsinfo->Sign,sign,4))
  {
   *fsinfosec=iSec;
   return ERR_SUCC;
  }
 }

 return ERR_FAIL;
}

/***********************************************************************************
 功能：从FAT表的最头上开始查找可用的空簇，即找到第一个可用空簇
 形参：nFreeCluster:指向用于记录有用的空簇的变量的指针
 返回：运行结果，成功或失败
 详解：从FAT表的最开始位置开始顺次查找，直到第一个可用的空簇出现，此空簇值将会被记录
       在文件系统初始化参数集合中的pInit_Args->Next_Free_Cluster，为文件操作过程中需
       使用空簇时提供可用空簇的参考值。
***********************************************************************************/
UINT8 Search_Free_Cluster_From_Start(UINT32 *nFreeCluster)
{
 UINT32 iSec=0;
 UINT8  iItem=0;

 struct FAT_Sec *pFAT_Sec;

 for(iSec=0;iSec<pInit_Args->FATsectors;iSec++)
 {
  znFAT_Device_Read_Sector(pInit_Args->FirstFATSector+iSec,znFAT_Buffer); //读取FAT扇区
  pFAT_Sec=(struct FAT_Sec *)znFAT_Buffer;

  for(iItem=0;iItem<NITEMSINFATSEC;iItem++) //遍历所有簇项，寻找空闲簇
  {
   if((0==(((pFAT_Sec->items[iItem]).Item)[0])) && (0==(((pFAT_Sec->items[iItem]).Item)[1])) &&
	  (0==(((pFAT_Sec->items[iItem]).Item)[2])) && (0==(((pFAT_Sec->items[iItem]).Item)[3])))
   {
    *nFreeCluster=((iSec*NITEMSINFATSEC)+(UINT32)iItem);
	return ERR_SUCC;
   }
  }
 }
 
 return	ERR_FAIL;
}

/***********************************************************************************
 功能：更新FSINFO扇区参数
 形参：无
 返回：0
 详解：更新FSINFO扇区，其实主要是为了维护剩余空簇数与下一个可用空簇参考值这两个参数。
***********************************************************************************/
#ifdef UPDATE_FSINFO
UINT8 Update_FSINFO(void) //更新FSINFO扇区
{
 struct FSInfo *pfsinfo;
 znFAT_Device_Read_Sector(pInit_Args->FSINFO_Sec,znFAT_Buffer);
 
 pfsinfo=((struct FSInfo *)znFAT_Buffer);
 
 //写入剩余空簇数
 pfsinfo->Free_Cluster[0]=(UINT8)( pInit_Args->Free_nCluster&0X000000FF)    ;
 pfsinfo->Free_Cluster[1]=(UINT8)((pInit_Args->Free_nCluster&0X0000FF00)>>8);
 pfsinfo->Free_Cluster[2]=(UINT8)((pInit_Args->Free_nCluster&0X00FF0000)>>16);
 pfsinfo->Free_Cluster[3]=(UINT8)((pInit_Args->Free_nCluster&0XFF000000)>>24);

 //写入下一空闲簇参考值，并无多大意义
 pfsinfo->Next_Free_Cluster[0]=(UINT8)( pInit_Args->Next_Free_Cluster&0X000000FF)    ;
 pfsinfo->Next_Free_Cluster[1]=(UINT8)((pInit_Args->Next_Free_Cluster&0X0000FF00)>>8);
 pfsinfo->Next_Free_Cluster[2]=(UINT8)((pInit_Args->Next_Free_Cluster&0X00FF0000)>>16);
 pfsinfo->Next_Free_Cluster[3]=(UINT8)((pInit_Args->Next_Free_Cluster&0XFF000000)>>24);

 znFAT_Device_Write_Sector(pInit_Args->FSINFO_Sec,znFAT_Buffer);

 return 0;
}
#endif

/***********************************************************************************
 功能：znFAT中的文件系统初始化函数
 形参：无
 返回：运行结果，成功或失败，如果它返回2，则说明发生FAT32文件系统类型校验错误，即存
       存储上的文件系统非FAT32。
 详解：文件系统初始化函数，将完成文件系统初始化参数集合的装入，为以后的文件操作作好
       准备。
***********************************************************************************/
UINT8 znFAT_Init(void)
{
 struct DBR *pdbr;

 UINT8 dm[3]=DBR_MARK;

 znFAT_Device_Read_Sector(MBR_SECTOR,znFAT_Buffer); 
   
 if(!(znFAT_Buffer[0]==dm[0] && znFAT_Buffer[1]==dm[1] && znFAT_Buffer[2]==dm[2])) //检测0扇区是否为DBR扇区
 {
  pInit_Args->BPB_Sector_No=Bytes2Value(((((struct MBR *)(znFAT_Buffer))->Part[0]).StartLBA),4);
 }
 else
 {
  pInit_Args->BPB_Sector_No=0;
 }
 
 znFAT_Device_Read_Sector((pInit_Args->BPB_Sector_No),znFAT_Buffer); //读取DBR扇区
 pdbr=(struct DBR *)znFAT_Buffer;

 if(!IS_FAT32_TYPE((pdbr->BS_FilSysType1))) return FSTYPE_NOT_FAT32; //FAT32文件系统类型检验

 pInit_Args->BytesPerSector  =Bytes2Value((pdbr->BPB_BytesPerSec),2);//装入每扇区字节数到BytesPerSector中

 pInit_Args->FATsectors      =Bytes2Value((pdbr->BPB_FATSz32)    ,4);//装入FAT表占用的扇区数到FATsectors中

 pInit_Args->SectorsPerClust =pdbr->BPB_SecPerClus;//装入每簇扇区数到SectorsPerClust 中
 pInit_Args->FirstFATSector  =Bytes2Value((pdbr->BPB_RsvdSecCnt) ,2)+pInit_Args->BPB_Sector_No;//装入第一个FAT表扇区号到FirstFATSector 中
 pInit_Args->FirstDirSector  =(pInit_Args->FirstFATSector)+(pdbr->BPB_NumFATs)*(pInit_Args->FATsectors); //装入第一个目录扇区到FirstDirSector中
 pInit_Args->Total_SizeKB    =Bytes2Value((pdbr->BPB_TotSec32),4)/2;  //磁盘的总容量，单位是KB

 if(Find_FSINFO_Sec(&(pInit_Args->FSINFO_Sec))) //查找FSINFO信息扇区
 {
  return ERR_FAIL;
 }

 znFAT_Device_Read_Sector((pInit_Args->FSINFO_Sec),znFAT_Buffer);
 pInit_Args->Free_nCluster=Bytes2Value(((struct FSInfo *)znFAT_Buffer)->Free_Cluster,4); //获取剩余簇数

 if(0XFFFFFFFF==pInit_Args->Free_nCluster) //如果一个磁盘格式化后没有卷标，则其存储空间就没有一点占用，此时FSINFO记录的剩余空簇数为0XFFFFFFFF
 {
  pInit_Args->Free_nCluster=(((pInit_Args->Total_SizeKB*2)-(pInit_Args->FirstDirSector))/(pInit_Args->SectorsPerClust))-1;
 }

 if(Search_Free_Cluster_From_Start(&(pInit_Args->Next_Free_Cluster)))//遍历整个FAT表，搜索可用的空闲簇
 {                                                                   //此操作可能会耗费较多时间，但没有办法
  return ERR_FAIL;                                                   //FSINFO中的空闲簇参考值并不保证正确
 }                                                                   //修正和维护它反而会更加麻烦
 
 #ifdef RT_UPDATE_FSINFO //及时将FSINFO扇区进行更新
 Update_FSINFO();
 #endif

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 #ifndef USE_ALONE_CCCB
 Memory_Set((UINT8 *)pcccb_buf,sizeof(UINT32)*CCCB_LEN,0);
 #endif
 #endif

 #ifdef USE_EXCHANGE_BUFFER
 #ifndef USE_ALONE_EXB
 Memory_Set(pexb_buf,512,0);
 #endif
 #endif

 return ERR_SUCC;
}

#ifndef RT_UPDATE_CLUSTER_CHAIN

#ifdef USE_ALONE_CCCB
UINT8 CCCB_To_Alone(void) //定向到当前文件的CCCB	 
{
 pcccb_buf=(just_file->acccb_buf);
 pcccb_curval=&(just_file->acccb_curval);
 pcccb_counter=&(just_file->acccb_counter);

 return 0;
}
#endif

UINT32 CCCB_Get_Next_Cluster(UINT32 cluster)
{
 UINT32 pos=CCCB_LEN-1;
 UINT32 i=0,temp=0;

 if(pcccb_buf==(UINT32 *)0) return 0;
 
 if(0==pcccb_buf[0]) return 0; //如果CCCB未被占用，则直接返回0

 #ifndef USE_ALONE_CCCB
 if(Dev_No!=(*pcccb_curdev)) return 0; //如果当前占用SCCCB的设备不是现在选定的设备，则直接返回0
 if(just_file!=pcccb_cur_oc) return 0; //如果当前占用SCCCB的文件不是现在正在操作的文件，则直接返回0
 #endif

 while(0==pcccb_buf[pos]) pos--;

 if(cluster>=pcccb_buf[pos] && cluster<=(*pcccb_curval))
 {
  if(cluster==(*pcccb_curval)) return 0X0FFFFFFF;
  if(cluster==pcccb_buf[pos])
  {
   if(pcccb_buf[pos]==(*pcccb_curval)) return 0X0FFFFFFF;
  }
  return (cluster+1);
 }

 temp=pos/2;
 for(i=0;i<temp;i++)
 {
  if(cluster>=pcccb_buf[2*i] && cluster<=pcccb_buf[2*i+1])
  {
   if(cluster==pcccb_buf[2*i+1]) return pcccb_buf[2*i+2];
   if(cluster==pcccb_buf[2*i])
   {
    if(pcccb_buf[2*i]==pcccb_buf[2*i+1]) return pcccb_buf[2*i+2];
   }
   return (cluster+1);
  }  
 }

 return 0;
}

UINT8 CCCB_Update_FAT(void)
{
 UINT32 i=0,j=0,temp=0,temp1=0;
 UINT32 old_clu=0,cur_clu=0,clu_sec=0;

 #ifndef USE_ALONE_CCCB
 UINT8 old_devno=Dev_No;
 struct znFAT_Init_Args *old_pinit_args=pInit_Args;
 #endif

 struct FAT_Sec *pFAT_Sec=(struct FAT_Sec *)znFAT_Buffer; //将数据缓冲区首地址强转为FAT_Sec结构体的指针类型

 if(pcccb_buf==(UINT32 *)0) return 0;

 if(0==pcccb_buf[0]) return 0; //CCCB尚未被占用，无簇链可更新

 #ifndef USE_ALONE_CCCB
 Dev_No=(*pcccb_curdev); pInit_Args=pcccb_cur_initargs;
 #endif

 old_clu=cur_clu=pcccb_buf[0];
 clu_sec=(old_clu/NITEMSINFATSEC); //计算前一簇的簇项所在的FAT扇区
 znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
 
 pcccb_buf[(*pcccb_counter)]=(*pcccb_curval); //将最后一个区间补齐

 temp1=((*pcccb_counter)+1)/2;
 for(i=0;i<temp1;)
 {
  for(j=pcccb_buf[2*i]+1;j<=pcccb_buf[2*i+1];j++)
  {
   cur_clu++;

   if(clu_sec!=(old_clu/NITEMSINFATSEC))
   {
    znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
    znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
     
	clu_sec=(old_clu/NITEMSINFATSEC);
    znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
   }

   temp=(UINT8)(old_clu%NITEMSINFATSEC);
   (((pFAT_Sec->items)[temp]).Item)[0]=(UINT8)(cur_clu&0X000000FF)      ;  //将其链在前面的簇项上   
   (((pFAT_Sec->items)[temp]).Item)[1]=(UINT8)((cur_clu&0X0000FF00)>>8) ;
   (((pFAT_Sec->items)[temp]).Item)[2]=(UINT8)((cur_clu&0X00FF0000)>>16);
   (((pFAT_Sec->items)[temp]).Item)[3]=(UINT8)((cur_clu&0XFF000000)>>24);
	
   old_clu=cur_clu;
  }

  cur_clu=((i==(temp1-1))?(0X0FFFFFFF):(pcccb_buf[2*i+2])); //目标簇取下一簇段的开始簇;

  if(clu_sec!=(old_clu/NITEMSINFATSEC))
  {
   znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
   znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
     
   clu_sec=(old_clu/NITEMSINFATSEC);
   znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
  }

  temp=(UINT8)(old_clu%NITEMSINFATSEC);
  (((pFAT_Sec->items)[temp]).Item)[0]=(UINT8)(cur_clu&0X000000FF)      ;  //将其链在前面的簇项上   
  (((pFAT_Sec->items)[temp]).Item)[1]=(UINT8)((cur_clu&0X0000FF00)>>8) ;
  (((pFAT_Sec->items)[temp]).Item)[2]=(UINT8)((cur_clu&0X00FF0000)>>16);
  (((pFAT_Sec->items)[temp]).Item)[3]=(UINT8)((cur_clu&0XFF000000)>>24);

  old_clu=cur_clu;
  i++;
 }
 znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
 znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);

 //============================================================================================
 Memory_Set((UINT8 *)pcccb_buf,sizeof(UINT32)*CCCB_LEN,0); //清空CCCB
 (*pcccb_counter)=0;

 #ifndef USE_ALONE_CCCB
 pcccb_cur_oc=(struct FileInfo *)0;
 *pcccb_curdev=(UINT8)(-1);
 pcccb_cur_initargs=(struct znFAT_Init_Args *)0;

 Dev_No=old_devno; pInit_Args=old_pinit_args; //恢复设备号与 与设备相关的文件系统参数集合
 #endif

 return 0;
}
#endif

/***********************************************************************************
 功能：获取下一簇
 形参：当前簇
 返回：下一簇的簇号
 详解：此函数在znFAT被频繁调用
***********************************************************************************/
#ifdef GET_NEXT_CLUSTER 
UINT32 Get_Next_Cluster(UINT32 cluster)
{
 UINT32 clu_sec=0;
 struct FAT_Sec *pFAT_Sec;
 struct FAT_Item *pFAT_Item;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 UINT32 next_clu=0;
 #endif

 #ifndef RT_UPDATE_CLUSTER_CHAIN //如果使用了CCCB，则获取下一簇时，先到CCCB中寻找，然后再到FAT中去找
 if(0!=get_next_cluster_in_cccb)
 {
  next_clu=CCCB_Get_Next_Cluster(cluster);
  if(0!=next_clu) return next_clu;
 }
 #endif

 clu_sec=(cluster/NITEMSINFATSEC)+(pInit_Args->FirstFATSector); //指定簇的簇项所在的扇区为其FAT区内的偏移量加上
                                                          
 znFAT_Device_Read_Sector(clu_sec,znFAT_Buffer); //将簇项所在的扇区数据读入缓冲区

 pFAT_Sec=(struct FAT_Sec *)znFAT_Buffer; //将数据缓冲区首地址强转为FAT_Sec结构体的指针类型

 pFAT_Item=&((pFAT_Sec->items)[cluster%NITEMSINFATSEC]); //获取指定簇的簇项在所在扇区中的地址

 return Bytes2Value((UINT8 *)pFAT_Item,NFATITEMBYTES); //返回簇项的值，即指定簇下一簇的簇号
}
#endif

/***********************************************************************************
 功能：文件数据定位
 形参：pfi:指向文件信息集合的指针 offset:目标偏移量
 返回：0
 详解：如果offset大于文件大小，则定位到文件末尾。此函数已经被数据读写函数集成，使用
       者一般不需要调用此函数。
***********************************************************************************/
#ifdef ZNFAT_SEEK
UINT8 znFAT_Seek(struct FileInfo *pfi,UINT32 offset)
{
 UINT32 Cluster_Size=((pInit_Args->SectorsPerClust)*(pInit_Args->BytesPerSector)); //计算簇的总字节数据，以免后面重复计算
 UINT32 temp=0,temp1=0,temp2=0,len=0,k=0,ncluster=0,have_read=0;

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=1;
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 if(offset<(pfi->File_Size)) //如果要定位到的偏移量小于文件大小，则必定不在文件末尾
 {
  pfi->File_IsEOF=BOOL_FALSE;
 }

 if(offset==(pfi->File_CurOffset)) return 0; //如果要定位的位置正好是当前偏移量，则直接返回

 if(offset<(pfi->File_CurOffset)) //如果要定位的位置在当前偏移量之前，则先回到文件起点，因为簇链是单向的
 {
  pfi->File_CurClust=pfi->File_StartClust;
  pfi->File_CurSec=SOC(pfi->File_CurClust);
  pfi->File_CurPos=0;
  pfi->File_CurOffset=0; 
  pfi->File_IsEOF=BOOL_FALSE;
 }
 
 len=offset-(pfi->File_CurOffset); //计算目标偏移量到当前偏移量之间的数据长度
 
 if(offset>=(pfi->File_Size)) //如果从当前位置开始要读取的数据长度len不小于文件大小
 {
  len=(pfi->File_Size-pfi->File_CurOffset);    //对len进行修正，置为文件剩余可读数据量。
  pfi->File_IsEOF=BOOL_TRUE;    //这种情况下，文件必然会读到末尾。                    
 }
 
 //=================================================================== 
 if((pfi->File_CurOffset%Cluster_Size)!=0) //如果当前偏移量是簇大小整数倍，说明此位置即为整簇开始
 {                                         //不要再进行当前簇内数据处理，直接进入簇-扇区-字节阶段
  if(len<=(pInit_Args->BytesPerSector-pfi->File_CurPos))
  {
   //更新当前位置参数
   if((pInit_Args->BytesPerSector-pfi->File_CurPos)==len) //如果正好读到当前扇区的末尾
   {
    if(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust))//如果当前扇区是当前簇的最后一个扇区                     
    {
     if(!pfi->File_IsEOF) 
	 {
	  pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); 
	 }
     pfi->File_CurSec=SOC(pfi->File_CurClust);
    }
    else
    {
     pfi->File_CurSec++;	
    }
    pfi->File_CurPos=0; 
   }
   else
   {
    pfi->File_CurPos+=((UINT16)len); 
   }	
   pfi->File_CurOffset+=len;

   return NUL_RET;
  }
  //===================================================================
  else
  {
   temp=(pInit_Args->BytesPerSector-pfi->File_CurPos); //将当前扇区的剩余数据量赋给中间变量temp
   have_read+=temp;
 	
   if(!(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust))) //如果当前扇区不是当前簇的最后一个扇区
   {
    pfi->File_CurSec++;
    pfi->File_CurPos=0; 

    temp2=(len-have_read); //计算剩余数据量
    temp1=((LAST_SEC_OF_CLU(pfi->File_CurClust)-(pfi->File_CurSec-1))*(pInit_Args->BytesPerSector)); //剩余所有扇区数据量
    if(temp2<=temp1) //如果剩余数据量xxx
    {
	 //这说明要读的数据在当前簇内，没有跨到下一簇	   
     temp=temp2/(pInit_Args->BytesPerSector); //计算当前簇内整扇区读取的结束扇区
   	 have_read+=((pInit_Args->BytesPerSector)*temp);

     if(temp2==temp1)
     {
      if(!pfi->File_IsEOF) 
	  {
	   pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); 
	  }
      pfi->File_CurSec=SOC(pfi->File_CurClust); 
      pfi->File_CurPos=0;
     }
     else
     {
      pfi->File_CurSec+=temp; 
      //更新当前位置参数
      pfi->File_CurPos=(UINT16)(len-have_read);
     }
     pfi->File_CurOffset+=len; 
    
     return NUL_RET;
    }
    else //如果剩余数据的整扇区数不小于当前簇的剩余扇区数，即要读的数据不光在当前簇内，已经跨簇了
    {
   	 temp=LAST_SEC_OF_CLU(pfi->File_CurClust)-(pfi->File_CurSec)+1; //计算当前簇的剩余整扇区数
   	 have_read+=((pInit_Args->BytesPerSector)*temp);
    }
   }
  
   //更新当前位置参数，此时已经读完当前簇的所有剩余数据，跨到下一簇
   pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust);
   pfi->File_CurSec=SOC(pfi->File_CurClust); 
   pfi->File_CurPos=0;    
  }
 }
 //----------------------------以上是处理当前簇内的数据-------------------------------------
 if(len-have_read>0) 
 {
  ncluster=(len-have_read)/Cluster_Size; //计算剩余数据的整簇数

  //更新当前位置参数，此时已经读完所有的整簇数据

  for(k=0;k<ncluster;k++) //读取整簇数据
  {
   have_read+=(Cluster_Size);
   if(!((len-have_read)==0 && pfi->File_IsEOF))  
   {
	pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust);
   }
  }

  pfi->File_CurSec=SOC(pfi->File_CurClust);
 
  //----------------------------以上是处理整簇数据------------------------------------------  
  if(len-have_read>0)
  {
   temp=(len-have_read)/(pInit_Args->BytesPerSector); //计算最终剩余数据的整扇区数
   have_read+=((pInit_Args->BytesPerSector)*temp);   

   pfi->File_CurSec+=temp;
   //----------------------------以上是处理整扇区数据----------------------------------------
   if(len-have_read>0)
   {  
    //更新当前位置参数，此时数据读取操作已经结束
    pfi->File_CurPos=(UINT16)(len-have_read);    
   }
   //----------------------------以上是处理最后扇区内的剩余字节----------------------------------------
  }
 }

 pfi->File_CurOffset+=len;

 return 0;
}
#endif

/******************************************************************************************
 功能：文件数据读取
 形参：pfi:指向文件信息集合的指针 offset:数据读取的开始偏移量 len:要读取的字节数
       app_Buffer:应用数据缓冲区指针
 返回：实际读取到的数据长度 
 详解：如果从offset位置读取的数据长度已经超越了文件大小，则仅读取offset位置到文件末尾的数据
       因此，如果文件的偏移位置已经在文件末尾，再对其进行读取，则必然读不到数据，而返回0，
       这可以作为文件数据已全部读完的标志；当然也可以看File_IsEOF这个标志，如果为1则说明文件
       已经在末尾了；再或者可以比较文件当前偏移量File_CurOffset与文件大小的差值，如果为0，则
       文件已到末尾。读到的数据将被放在app_Buffer指向的应用数据缓冲区中，要注意缓冲区大小，
       防止内存溢出。
******************************************************************************************/
#ifdef ZNFAT_READDATA 
UINT32 znFAT_ReadData(struct FileInfo *pfi,UINT32 offset,UINT32 len,UINT8 *app_Buffer)
{
 UINT32 Cluster_Size=0,iClu=0,next_clu=0,start_clu=0,end_clu=0;
 UINT32 temp=0,temp1=0,temp2=0,ncluster=0,have_read=0;

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=1;
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 znFAT_Seek(pfi,offset); //文件定位

 if(0==len) return 0; //如果要读取的数据长度为0，则直接返回
 
 Cluster_Size=(pInit_Args->SectorsPerClust*pInit_Args->BytesPerSector); //计算簇的总字节数据，以免后面重复计算
 
 if((pfi->File_CurOffset+len)>=(pfi->File_Size)) //如果从当前位置开始要读取的数据长度len不小于文件大小
 {
  len=(pfi->File_Size-pfi->File_CurOffset);    //对len进行修正，置为文件剩余可读数据量。
  pfi->File_IsEOF=BOOL_TRUE;    //这种情况下，文件必然会读到末尾。                    
 }
 
 //============================================================================================================ 
 if((pfi->File_CurOffset%Cluster_Size)!=0) //如果当前偏移量是簇大小整数倍，说明此位置即为整簇开始
 {                                         //不要再进行当前簇内数据处理，直接进入簇-扇区-字节阶段
  znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //将当前扇区读入内部缓冲区

  temp=pInit_Args->BytesPerSector-pfi->File_CurPos; //计算当前扇区中的剩余数据量

  if(len<=temp)
  {
   Memory_Copy(app_Buffer,znFAT_Buffer+(pfi->File_CurPos),len);//将内部缓冲区中要读的数据拷入应用缓冲区
   
   //更新当前位置参数
   if(temp==len) //如果正好读到当前扇区的末尾
   {
    if(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust))//如果当前扇区是当前簇的最后一个扇区                     
    {
     if(!pfi->File_IsEOF) //如果不是文件末尾
	 {
	  pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); //可能有“窘簇” 
	 }
     pfi->File_CurSec=SOC(pfi->File_CurClust);
    }
    else
    {
     pfi->File_CurSec++;	
    }
    pfi->File_CurPos=0; 
   }
   else
   {
    pfi->File_CurPos+=(UINT16)len; 
   }	
   pfi->File_CurOffset+=len;

   return len;
  }
  //===========================================================================================================
  else
  {
   temp=(pInit_Args->BytesPerSector-pfi->File_CurPos); //将当前扇区的剩余数据量赋给中间变量temp

   Memory_Copy(app_Buffer,znFAT_Buffer+(pfi->File_CurPos),temp); //将当前扇区剩余数据誊到应用缓冲区
   have_read+=temp;
 	
   if(!(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust))) //如果当前扇区不是当前簇的最后一个扇区
   {
    pfi->File_CurSec++;
    pfi->File_CurPos=0; 

    temp2=(len-have_read); //计算剩余数据量
    temp1=((LAST_SEC_OF_CLU(pfi->File_CurClust)-(pfi->File_CurSec-1))*(pInit_Args->BytesPerSector)); //剩余所有扇区数据量
    if(temp2<=temp1) //如果要读的剩余数据量小于等于当前簇剩余数据量 
    {
	 //这说明要读的数据在当前簇内，没有跨到下一簇
	   
     temp=temp2/(pInit_Args->BytesPerSector); //计算要读取的剩余数据足够几个整扇区 
     
	 znFAT_Device_Read_nSector(temp,pfi->File_CurSec,app_Buffer+have_read);
	 have_read+=((pInit_Args->BytesPerSector)*temp);

     if(temp2==temp1)
     {
      if(!pfi->File_IsEOF) //如果不是文件末尾
	  {
	   pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); //可能有“窘簇” 
	  }
      pfi->File_CurSec=SOC(pfi->File_CurClust); 
      pfi->File_CurPos=0;
     }
     else
     {
      pfi->File_CurSec+=temp; 
	  temp=len-have_read;

      znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //当前扇区中可能还有部分剩余数据要读
      Memory_Copy(app_Buffer+have_read,znFAT_Buffer,temp); //将最后不足扇区的数据誊入应用缓冲区
      //更新当前位置参数
      pfi->File_CurPos=(UINT16)temp;
      
     }
     pfi->File_CurOffset+=len; 
    
     return len;
    }
    else //如果剩余数据的整扇区数不小于当前簇的剩余扇区数，即要读的数据不光在当前簇内，已经跨簇了
    {
   	 temp=(LAST_SEC_OF_CLU(pfi->File_CurClust))-(pfi->File_CurSec)+1; //计算当前簇还有几个整扇区 

     znFAT_Device_Read_nSector(temp,(pfi->File_CurSec),app_Buffer+have_read);
     have_read+=((pInit_Args->BytesPerSector)*temp);
    }
   }
  
   //更新当前位置参数，此时已经读完当前簇的所有剩余数据，跨到下一簇
   pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); //这里不会产生“窘簇” 
   pfi->File_CurSec=SOC(pfi->File_CurClust); 
   pfi->File_CurPos=0;    
  }
 }
 //----------------------------以上是处理当前簇内的数据-------------------------------------

 temp1=len-have_read;
 ncluster=temp1/Cluster_Size; //计算剩余数据的整簇数
 if(ncluster>0) //剩余数据起码足够一个簇
 {
  //以下计算连续簇段，以尽量使用多扇区读取驱动
  start_clu=end_clu=pfi->File_CurClust;

  for(iClu=1;iClu<ncluster;iClu++)
  {
   next_clu=Get_Next_Cluster(end_clu);
   if((next_clu-1)==end_clu)
   {
    end_clu=next_clu;
   }
   else
   {
    znFAT_Device_Read_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)),SOC(start_clu),app_Buffer+have_read);
	have_read+=((end_clu-start_clu+1)*Cluster_Size);
	start_clu=end_clu=next_clu;
   }
  }

  //----------------------------以上是处理整簇数据------------------------------------------  
  temp=temp1%Cluster_Size; //计算整簇读完之后，是否还有数据要读
  if(temp>0) //整簇数据后面还有数据要读
  {
   temp=temp/(pInit_Args->BytesPerSector); //计算最终不足整簇的剩余数据的整扇区数

   

   next_clu=Get_Next_Cluster(end_clu);
   if((next_clu-1)==end_clu) //如果最后一个簇仍然与前面的连续簇段连续
   {
    znFAT_Device_Read_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)+temp),SOC(start_clu),app_Buffer+have_read);
	have_read+=((pInit_Args->BytesPerSector)*((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)+temp));
   }
   else //如果最后一个簇与前面的连续簇段并不连续
   {
    znFAT_Device_Read_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)),SOC(start_clu),app_Buffer+have_read);
	have_read+=(Cluster_Size*(end_clu-start_clu+1));
    znFAT_Device_Read_nSector(temp,SOC(next_clu),app_Buffer+have_read);
	have_read+=(temp*(pInit_Args->BytesPerSector));
   }

   pfi->File_CurClust=next_clu;
   pfi->File_CurSec=(SOC(next_clu)+temp); 

   //----------------------------以上是处理整扇区数据----------------------------------------
   temp=len-have_read;
   if(temp>0)
   {
    znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //将最后的可能包含一部分要读的数据的扇区读到内部缓冲区
    Memory_Copy(app_Buffer+have_read,znFAT_Buffer,temp); //将最后不足扇区的数据余量誊入应用缓冲区
  
    //更新当前位置参数，此时数据读取操作已经结束
    pfi->File_CurPos=(UINT16)temp;    
   }
   //----------------------------以上是处理最后扇区内的剩余字节----------------------------------------
  }
  else //整簇数据读完之后再无数据要读
  {
   znFAT_Device_Read_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)),SOC(start_clu),app_Buffer+have_read);

   pfi->File_CurClust=end_clu;
   if(!pfi->File_IsEOF) 
   {
    pfi->File_CurClust=Get_Next_Cluster(end_clu);
   }
   pfi->File_CurSec=SOC(pfi->File_CurClust); 
  }
 }
 else //剩余的数据不足一个簇
 {
  temp=temp1/(pInit_Args->BytesPerSector); //计算剩余的数据足够几个扇区
  znFAT_Device_Read_nSector(temp,SOC(pfi->File_CurClust),app_Buffer+have_read);
  have_read+=temp*(pInit_Args->BytesPerSector);

  pfi->File_CurSec+=temp;
  
  temp=temp1%(pInit_Args->BytesPerSector); //计算最终的不足一扇区的数据量
  if(temp>0) //如果最后还有数据
  {
   znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer);
   Memory_Copy(app_Buffer+have_read,znFAT_Buffer,temp);

   pfi->File_CurPos=(UINT16)temp;
  }
 }

 //----------------------------------------------------------------------------------------
 pfi->File_CurOffset+=len;

 return len;
}
#endif

/******************************************************************************************
 功能：文件数据读取+数据重定向
 形参：pfi:指向文件信息集合的指针 offset:数据读取的开始偏移量 len:要读取的字节数
       app_Buffer:应用数据缓冲区指针
 返回：实际读取到的数据长度 
 详解：此函数的实现过程与上面的数据读取函数相似，不同在于可以对读到的数据进行重新定向。
       上面的数据读取函数是将数据直接放到了应用数据缓冲区中，但是在某些硬件平台上RAM资源
       比较紧张，开辟出较大的应用数据缓冲区不太现实，因此在这里引入重定向的概念，可以将
       读到的每一个字节使用某个函数直接进行处理，而不放在缓冲区中。这个用于处理字节数据的
       函数由使用者提供，这个函数的函数名在znfat.h中被宏定义为Data_Redirect。比如如果想把
       读到的数据通过串口发出，则在znfat.h中将宏定义改为 #define Data_Redirect Send_Byte
       Send_Byte就是串口发送字节的函数的函数名。
******************************************************************************************/
#ifdef ZNFAT_READDATAX 
UINT32 znFAT_ReadDataX(struct FileInfo *pfi,UINT32 offset,UINT32 len) //数据重定向读取套用了数据读取函数，但并不使用多扇区驱动
{                                                                     //加之没有足够的RAM资源，且每次处理一个字节，因此效率不高
 UINT32 Cluster_Size=0,iClu=0,iSec=0,next_clu=0,start_clu=0,end_clu=0;
 UINT32 temp=0,temp1=0,temp2=0,k=0,ncluster=0,have_read=0;
 UINT32 i=0;

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=1;
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 znFAT_Seek(pfi,offset); //文件定位
 
 Cluster_Size=(pInit_Args->SectorsPerClust*pInit_Args->BytesPerSector); //计算簇的总字节数据，以免后面重复计算
 
 if((pfi->File_CurOffset+len)>=(pfi->File_Size)) //如果从当前位置开始要读取的数据长度len不小于文件大小
 {
  len=(pfi->File_Size-pfi->File_CurOffset);    //对len进行修正，置为文件剩余可读数据量。
  pfi->File_IsEOF=BOOL_TRUE;    //这种情况下，文件必然会读到末尾。                    
 }
 
 //=================================================================== 
 if((pfi->File_CurOffset%Cluster_Size)!=0) //如果当前偏移量是簇大小整数倍，说明此位置即为整簇开始
 {                                         //不要再进行当前簇内数据处理，直接进入簇-扇区-字节阶段
  znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //将当前扇区读入内部缓冲区

  temp=pInit_Args->BytesPerSector-pfi->File_CurPos; //计算当前扇区中的剩余数据量
  if(len<=temp)
  {
   for(i=0;i<len;i++)
   {
	Data_Redirect(znFAT_Buffer[i+(pfi->File_CurPos)]);
   }
   
   //更新当前位置参数
   if(temp==len) //如果正好读到当前扇区的末尾
   {
    if(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust))//如果当前扇区是当前簇的最后一个扇区                     
    {
     if(!pfi->File_IsEOF) //如果不是文件末尾
	 {
	  pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); //可能有“窘簇” 
	 }
     pfi->File_CurSec=SOC(pfi->File_CurClust);
    }
    else
    {
     pfi->File_CurSec++;	
    }
    pfi->File_CurPos=0; 
   }
   else
   {
    pfi->File_CurPos+=(UINT16)len; 
   }	
   pfi->File_CurOffset+=len;

   return len;
  }
  //===================================================================
  else
  {
   temp=(pInit_Args->BytesPerSector-pfi->File_CurPos); //将当前扇区的剩余数据量赋给中间变量temp

   for(i=0;i<temp;i++)
   {
	Data_Redirect(znFAT_Buffer[i+(pfi->File_CurPos)]);
   }
   
   have_read+=temp;
 	
   if(!(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust))) //如果当前扇区不是当前簇的最后一个扇区
   {
    pfi->File_CurSec++;
    pfi->File_CurPos=0; 

    temp2=(len-have_read); //计算剩余数据量
    temp1=((LAST_SEC_OF_CLU(pfi->File_CurClust)-(pfi->File_CurSec-1))*(pInit_Args->BytesPerSector)); //剩余所有扇区数据量
    if(temp2<=temp1) //如果要读的剩余数据量小于等于当前簇剩余数据量 
    {
	 //这说明要读的数据在当前簇内，没有跨到下一簇
	   
     temp=temp2/(pInit_Args->BytesPerSector); //计算要读取的剩余数据足够几个整扇区 

	 for(iSec=0;iSec<temp;iSec++)
	 {
	  znFAT_Device_Read_Sector(pfi->File_CurSec+iSec,znFAT_Buffer);
	  for(i=0;i<(pInit_Args->BytesPerSector);i++)
	  {
	   Data_Redirect(znFAT_Buffer[i]);
	  }
	 }
	 
	 have_read+=temp*(pInit_Args->BytesPerSector);

     if(temp2==temp1)
     {
      if(!pfi->File_IsEOF) //如果不是文件末尾
	  {
	   pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); //可能有“窘簇” 
	  }
      pfi->File_CurSec=SOC(pfi->File_CurClust); 
      pfi->File_CurPos=0;
     }
     else
     {
      pfi->File_CurSec+=temp; 
	  temp=len-have_read;

      znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //当前扇区中可能还有部分剩余数据要读
      for(i=0;i<temp;i++)
	  {
	   Data_Redirect(znFAT_Buffer[i]);
	  }
      //更新当前位置参数
      pfi->File_CurPos=(UINT16)temp;
      
     }
     pfi->File_CurOffset+=len; 
    
     return len;
    }
    else //如果剩余数据的整扇区数不小于当前簇的剩余扇区数，即要读的数据不光在当前簇内，已经跨簇了
    {
   	 temp=(LAST_SEC_OF_CLU(pfi->File_CurClust))-(pfi->File_CurSec)+1; //计算当前簇还有几个整扇区 

	 for(iSec=0;iSec<temp;iSec++)
	 {
	  znFAT_Device_Read_Sector(pfi->File_CurSec+iSec,znFAT_Buffer);
	  for(i=0;i<(pInit_Args->BytesPerSector);i++)
	  {
	   Data_Redirect(znFAT_Buffer[i]);
	  }
	 }
	 have_read+=temp*(pInit_Args->BytesPerSector);
    }
   }
  
   //更新当前位置参数，此时已经读完当前簇的所有剩余数据，跨到下一簇
   pfi->File_CurClust=Get_Next_Cluster(pfi->File_CurClust); //这里不会产生“窘簇” 
   pfi->File_CurSec=SOC(pfi->File_CurClust); 
   pfi->File_CurPos=0;    
  }
 }
 //----------------------------以上是处理当前簇内的数据-------------------------------------
 temp1=len-have_read;
 ncluster=temp1/Cluster_Size; //计算剩余数据的整簇数

 if(ncluster>0) //剩余数据起码足够一个簇
 {
  //以下计算连续簇段，以尽量使用多扇区读取驱动
  start_clu=end_clu=pfi->File_CurClust;

  for(iClu=1;iClu<ncluster;iClu++)
  {
   next_clu=Get_Next_Cluster(end_clu);
   if((next_clu-1)==end_clu)
   {
    end_clu=next_clu;
   }
   else
   {
    temp=((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust));
    for(iSec=0;iSec<temp;iSec++)
	{
	 znFAT_Device_Read_Sector(SOC(start_clu)+iSec,znFAT_Buffer);
	 for(i=0;i<(pInit_Args->BytesPerSector);i++)
	 {
	  Data_Redirect(znFAT_Buffer[i]);
	 }
	}
    have_read+=temp*(pInit_Args->BytesPerSector);
	start_clu=end_clu=next_clu;
   }
  }

  //----------------------------以上是处理整簇数据------------------------------------------  
  temp=temp1%Cluster_Size; //计算整簇读完之后，是否还有数据要读
  if(temp>0) //整簇数据后面还有数据要读
  { 
   temp=temp/(pInit_Args->BytesPerSector); //计算最终不足整簇的剩余数据的整扇区数
   next_clu=Get_Next_Cluster(end_clu);
   if((next_clu-1)==end_clu) //如果最后一个簇仍然与前面的连续簇段连续
   {
    temp2=((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)+temp);
    for(iSec=0;iSec<temp2;iSec++)
	{
	 znFAT_Device_Read_Sector(SOC(start_clu)+iSec,znFAT_Buffer);

	 for(i=0;i<(pInit_Args->BytesPerSector);i++)
	 {
	  Data_Redirect(znFAT_Buffer[i]);
	 }
	}
   }
   else //如果最后一个簇与前面的连续簇段并不连续
   {
	temp2=((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust));
    for(iSec=0;iSec<temp2;iSec++)
	{
	 znFAT_Device_Read_Sector(SOC(start_clu)+iSec,znFAT_Buffer);

	 for(i=0;i<(pInit_Args->BytesPerSector);i++)
	 {
	  Data_Redirect(znFAT_Buffer[i]);
	 }
	}

    for(iSec=0;iSec<temp;iSec++)
	{
	 znFAT_Device_Read_Sector(SOC(next_clu)+iSec,znFAT_Buffer);

	 for(i=0;i<(pInit_Args->BytesPerSector);i++)
	 {
	  Data_Redirect(znFAT_Buffer[i]);
	 }
	}
   }
   
   have_read+=((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)+temp)*(pInit_Args->BytesPerSector);

   pfi->File_CurClust=next_clu;
   pfi->File_CurSec=(SOC(next_clu)+temp); 

   //----------------------------以上是处理整扇区数据----------------------------------------
   temp=len-have_read;
   if(temp>0)
   {
    znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //将最后的可能包含一部分要读的数据的扇区读到内部缓冲区
  
	for(i=0;i<temp;i++)
	{
	 Data_Redirect(znFAT_Buffer[i]);
	}

    //更新当前位置参数，此时数据读取操作已经结束
    pfi->File_CurPos=(UINT16)temp;    
   }
   //----------------------------以上是处理最后扇区内的剩余字节----------------------------------------
  }
  else //整簇数据读完之后再无数据要读
  {
   temp2=((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust));
   for(iSec=0;iSec<temp2;iSec++)
   {
	znFAT_Device_Read_Sector(SOC(start_clu)+iSec,znFAT_Buffer);
	for(i=0;i<(pInit_Args->BytesPerSector);i++)
	{
	 Data_Redirect(znFAT_Buffer[i]);
	}
   }

   pfi->File_CurClust=end_clu;
   if(!pfi->File_IsEOF) 
   {
    pfi->File_CurClust=Get_Next_Cluster(end_clu);
   }
   pfi->File_CurSec=SOC(pfi->File_CurClust); 
  }
 }
 else //剩余的数据不足一个簇
 {
  temp=temp1/(pInit_Args->BytesPerSector); //计算剩余的数据足够几个扇区
  for(iSec=0;iSec<temp;iSec++)
  {
   znFAT_Device_Read_Sector(SOC(pfi->File_CurClust)+iSec,znFAT_Buffer);
   for(i=0;i<(pInit_Args->BytesPerSector);i++)
   {
	Data_Redirect(znFAT_Buffer[i]);
   }
  }

  pfi->File_CurSec+=temp;
  
  temp=temp1%(pInit_Args->BytesPerSector); //计算最终的不足一扇区的数据量
  if(temp>0) //如果最后还有数据
  {
   znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer);
   for(i=0;i<temp;i++)
   {
	Data_Redirect(znFAT_Buffer[i]);
   }

   pfi->File_CurPos=temp;
  }
 }

 //----------------------------------------------------------------------------------------
 pfi->File_CurOffset+=len;

 return len;
}
#endif

//----------------------------以下函数用于检验文件名的合法性----------------------------------

/******************************************************************************************
 功能：检查文件名中是否含有非法字符，非法字符有 \/:"<>| 
 形参：pfn:指针文件名的指针
 返回：运行结果，成功或失败  成功：文件名中无非法字符，反之则有 
 详解：对于文件名中非法字符的检查，其实还有两个字符*?，但是znFAT是支持文件名的通配的，因此
       这里不限制*与?，在打开文件和目录的时候所使用的文件名中是带有*与?，但创建文件和目录时
       文件名中则不可有*与?，请使用者注意！
******************************************************************************************/
#ifdef CHECK_ILLEGAL_CHAR
UINT8 Check_Illegal_Char(INT8 *pfn) 
{
 UINT32 i=0;
 while(pfn[i])
 {
  if(('\\'==pfn[i]) || ('/'==pfn[i]) || (':'==pfn[i])  
   || /*('*'==pfn[i]) || ('?'==pfn[i]) ||*/ ('"'==pfn[i])  
   || ('<'==pfn[i]) || ('>'==pfn[i]) || ('|'==pfn[i]))
  return ERR_FAIL;
  i++;
 }
 return ERR_SUCC;
}
#endif

/******************************************************************************************
 功能：检查文件名中是否含有特殊字符，+[],;=space
 形参：pfn:指针文件名的指针
 返回：运行结果，成功或失败  成功：文件名中无特殊字符，反之则有 
 详解：对于8.3格式的文件名，如果其中包含了特殊字符，则其被视为长文件名，而非短名。因为短名
       与长名在处理方法是不同的，因此对它们区分开来。SFN是Short FileName的缩写，带有SFN的函
       是针对短名的检查
******************************************************************************************/
#ifdef CHECK_SFN_SPECIAL_CHAR
UINT8 Check_SFN_Special_Char(INT8 *pfn) 
{
 UINT32 i=0;
 UINT32 pos=(StringLen(pfn)-1);

 while(' '==pfn[pos]) pos--;
 
 while(i<=pos)
 {
  if( ('+'==pfn[i]) || ('['==pfn[i]) 
   || (']'==pfn[i]) || (','==pfn[i])
   || (' '==pfn[i]) || (';'==pfn[i])
   || ('='==pfn[i]))
  return ERR_FAIL;
  i++;
 }

 return ERR_SUCC;
}
#endif

/******************************************************************************************
 功能：检查文件名中的.是否符合SFN要求
 形参：pfn:指针文件名的指针
 返回：运行结果，成功或失败
 详解：SFN中的.不得多于1个，如A..A.TXT、A.A.A、AA.A..A等均非SFN，文件名不得以.开始
       若干个.如果在末尾则均被忽略
******************************************************************************************/
#ifdef CHECK_SFN_DOT
UINT8 Check_SFN_Dot(INT8 *pfn) 
{
 UINT32 pos=(StringLen(pfn)-1);
 UINT32 dot_counter=0;

 if('.'==pfn[0]) return ERR_FAIL;

 while(pos>0) 
 {
  if('.'==pfn[pos])
  {
   dot_counter++;
  }
  pos--;
 }

 if(dot_counter>1) return ERR_FAIL;
 else return ERR_SUCC;
}
#endif

/******************************************************************************************
 功能：检查文件名的大小写是否符合SFN要求
 形参：pfn:指针文件名的指针
 返回：运行结果，成功或失败
 详解：SFN中是不可能有大小写混排的，比如AabcC.txt ABC.Txt AbCdef.TXT都不是SFN，而是长名
       简言之，SFN的主文件名和扩展名必须均是大写或小写，比如aaa.TXT AAA.txt aaa.txt AA.TXT
       均是SFN
******************************************************************************************/
#ifdef CHECK_SFN_ILLEGAL_LOWER
UINT8 Check_SFN_Illegal_Lower(INT8 *pfn)
{
 UINT32 i=(StringLen(pfn)-1);
 UINT8 flag1=2,flag2=2; 

 while('.'!=pfn[i] && i>0)
 {
  if(pfn[i]>='a' && pfn[i]<='z') 
  {
   if(0==flag1)
   {
	return (UINT8)-1;
   }
   flag1=1;
  }

  if(pfn[i]>='A' && pfn[i]<='Z') 
  {
   if(1==flag1)
   {
	return (UINT8)-1;
   }
   flag1=0;
  }

  i--;
 }

 if(0==i) //如果没有.说明没有扩展名
 { 
  if(pfn[0]>='a' && pfn[0]<='z') 
  {
   if(0==flag1)
   {
	return (UINT8)-1;
   }
   flag1=1;
  }

  flag2=flag1; 
  flag1=0;
 }

 if('.'==pfn[i])
 {
  i--;
 }

 while(i>0)
 {
  if(pfn[i]>='a' && pfn[i]<='z') 
  {
   if(0==flag2)
   {
	return (UINT8)-1;
   }
   flag2=1;
  }

  if(pfn[i]>='A' && pfn[i]<='Z') 
  {
   if(1==flag2)
   {
	return (UINT8)-1;
   }
   flag2=0;
  }

  i--;  
 }

 if(2==flag2)
 {
  if(pfn[0]>='a' && pfn[0]<='z') 
  {
   flag2=1;
  }
  if(pfn[0]>='A' && pfn[0]<='Z')
  {
   flag2=0;
  }
 }
 
 return (UINT8)((flag1<<4)|(flag2));
}
#endif

/******************************************************************************************
 功能：检查文件名的长度是否符合SFN要求
 形参：pfn:指针文件名的指针
 返回：运行结果，成功或失败
 详解：8.3格式的SFN，要求主文件名的长度0<len<=8 扩展名长度0<=len<=3，不符合这一要求的均为长
       名
******************************************************************************************/
#ifdef CHECK_SFN_ILLEGAL_LENGTH
UINT8 Check_SFN_Illegal_Length(INT8 *pfn)
{
 UINT8 dot_pos=0,have_dot=0,i=0;
 UINT8 mainfn_len=0,extfn_len=0;
 UINT32 fn_len=StringLen(pfn);

 while(' '==pfn[fn_len-1]) fn_len--;

 if(fn_len>12) return ERR_FAIL; //fn is longer than 8.3

 for(i=(UINT8)(fn_len-1);i>0;i--) //反向寻找. 第一个.是主文件与扩展名的分界
 {
  if('.'==pfn[i]) 
  {
   dot_pos=i;
   have_dot=1;
   break;
  }
 }
 
 if(0==have_dot) //没有点
 {
  mainfn_len=(UINT8)fn_len;
  extfn_len=0;

  if((mainfn_len>0 && mainfn_len<=8))
  {
   return ERR_SUCC;
  }
  else
  {
   return ERR_FAIL;
  }
 }
 else //有点
 {
  mainfn_len=dot_pos;
  extfn_len=(UINT8)(((UINT8)fn_len)-(UINT8)(dot_pos+1));

  if(( mainfn_len>0  && mainfn_len<=8) 
   && (/*extfn_len>=0  && */extfn_len <=3))
  {
   return ERR_SUCC;
  }
  else
  {
   return ERR_FAIL;
  }
 }	
}
#endif
//--------------------------------以上函数用于检验SFN的合法性----------------------------------

/******************************************************************************************
 功能：将一个文件目录项中的前11个字节（用于记录8.3短文件名）转为文件名
 形参：name_in_fdi:指向文件目录项中用于记录文件名的字节序列的指针 pfilename:用于记录转换后的
       文件名的数组的指针
 返回：运行结果，成功或失败
 详解：在打开文件和目录等函数中，把文件目录项中的文件名字段转为文件名，以方便与目标文件进行
       匹配和比对。比如要打开ABC.TXT 它所对应的文件目录项记录的形式为ABC     TXT，二者是不能
       直接进行比较的，要先将ABC     TXT转为ABC.TXT，然后再比较。
******************************************************************************************/
#ifdef TO_FILE_NAME
UINT8 To_File_Name(INT8 *name_in_fdi,INT8 *pfileName)
{
 UINT8 i=0,n=7,m=10;

 while(' '==name_in_fdi[n])
 {
  n--;
 }
 n++;

 while(' '==name_in_fdi[m] && m>=8)
 {
  m--;
 }
 m-=7;

 for(i=0;i<n;i++)
 {
  pfileName[i]=name_in_fdi[i];
 }
 pfileName[i]='.';

 for(i=0;i<m;i++)
 {
  pfileName[n+i+1]=name_in_fdi[8+i];
 }

 if('.'==pfileName[n+m]) pfileName[n+m]=0;
 else pfileName[n+m+1]=0;

 return 0;
}
#endif

/******************************************************************************************
 功能：解析一个文件目录项，将解析得到的参数装入到文件信息集合中
 形参：pfi:指向文件信息集合的指针 pFDI:指向文件目录项的指针
 返回：0
 详解：此函数被打开文件的函数调用，对符合匹配条件的文件目录项进行解析，并将参数装入到文件信
       息集合中，以便对文件进行进一步的操作时使用。
******************************************************************************************/
#ifdef ANALYSE_FDI
UINT8 Analyse_FDI(struct FileInfo *pfi,struct FDI *pFDI)
{
 UINT32 temp=0,i=0;

 //==========================================================

 just_file=pfi;

 To_File_Name((INT8 *)pFDI,pfi->File_Name);
 
 temp=(StringLen(pfi->File_Name)-1);
 while('.'!=(pfi->File_Name)[temp] && temp>0)
 {
  if(((pFDI->LowerCase)&0x10)!=0) 
  {
   (pfi->File_Name)[temp]=(INT8)Upper2Low((pfi->File_Name)[temp]);
  }
  temp--;
 }
 if(((pFDI->LowerCase)&0x08)!=0) 
 {
  for(i=0;i<temp;i++)
  {
   (pfi->File_Name)[i]=(INT8)Upper2Low((pfi->File_Name)[i]);   
  }
 }

 temp=(StringLen(pfi->File_Name)-1); 
 if(CHK_ATTR_DIR(pFDI->Attributes)) //如果是目录则将最后的.去掉
 {
  (pfi->File_Name)[temp+1]='\0';
 }
 //==以上是按照LowerCase字节对主文件名与扩展文件名进行小写化

 pfi->File_Attr=pFDI->Attributes; //文件属性
 pfi->File_StartClust=Bytes2Value(pFDI->LowClust,2)+Bytes2Value(pFDI->HighClust,2)*65536;
 pfi->File_Size=Bytes2Value(pFDI->FileSize,4);
  
 //解析文件创建时间与日期
 temp=Bytes2Value(pFDI->CTime,2);
 pfi->File_CTime.sec=(UINT8)((temp&TIME_SEC_MARK)*2); temp>>=TIME_SEC_NBITS;  //创建时间的2秒位
 pfi->File_CTime.min=(UINT8)(temp&TIME_MIN_MARK);   temp>>=TIME_MIN_NBITS; //创建时间的分位
 pfi->File_CTime.hour=(UINT8)(temp&TIME_HOUR_MARK); //创建时间的时位
 pfi->File_CTime.sec+=(UINT8)((UINT16)(pFDI->CTime10ms)/100); //在秒上加上10毫秒位

 temp=Bytes2Value(pFDI->CDate,2);
 pfi->File_CDate.day=(UINT8)(temp&DATE_DAY_MARK);     temp>>=DATE_DAY_NBITS;   //创建日期的日位
 pfi->File_CDate.month=(UINT8)(temp&DATE_MONTH_MARK); temp>>=DATE_MONTH_NBITS; //创建日期的月位
 pfi->File_CDate.year=(UINT16)((temp&DATE_YEAR_MARK)+DATE_YEAR_BASE); //创建日期的年位（加上年份基数）

 //解析文件修改时间与日期
 //temp=Bytes2Value(pFDI->MTime,2);
 //pfi->File_MTime.sec=(UINT8)((temp&TIME_SEC_MARK)*2); temp>>=TIME_SEC_NBITS;  //创建时间的2秒位
 //pfi->File_MTime.min=(UINT8)(temp&TIME_MIN_MARK);   temp>>=TIME_MIN_NBITS; //创建时间的分位
 //pfi->File_MTime.hour=(UINT8)(temp&TIME_HOUR_MARK); //创建时间的时位
 //文件的修改时间没有10毫秒位，所以它只能表达偶数秒

 //temp=Bytes2Value(pFDI->MDate,2);
 //pfi->File_MDate.day=(UINT8)(temp&DATE_DAY_MARK);     temp>>=DATE_DAY_NBITS;   //创建日期的日位
 //pfi->File_MDate.month=(UINT8)(temp&DATE_MONTH_MARK); temp>>=DATE_MONTH_NBITS; //创建日期的月位
 //pfi->File_MDate.year=(UINT8)((temp&DATE_YEAR_MARK)+DATE_YEAR_BASE); //创建日期的年位

 //解析文件访问日期
 //temp=Bytes2Value(pFDI->ADate,2);
 //pfi->File_ADate.day=(UINT8)(temp&DATE_DAY_MARK);     temp>>=DATE_DAY_NBITS;   //创建日期的日位
 //pfi->File_ADate.month=(UINT8)(temp&DATE_MONTH_MARK); temp>>=DATE_MONTH_NBITS; //创建日期的月位
 //pfi->File_ADate.year=(UINT8)((temp&DATE_YEAR_MARK)+DATE_YEAR_BASE); //创建日期的年位

 pfi->File_CurClust=pfi->File_StartClust;
 pfi->File_CurSec=(pfi->File_CurClust)?SOC(pfi->File_CurClust):0;
 pfi->File_CurPos=0;
 pfi->File_CurOffset=0;
 pfi->File_IsEOF=(UINT8)((pfi->File_StartClust)?BOOL_FALSE:BOOL_TRUE);

 return 0;
}
#endif

/******************************************************************************************
 功能：检查文件名是否是通配名，通配名中含有*与?
 形参：pfn:指向文件名的指针
 返回：运行结果 是或否
 详解：znFAT是支持通配名的，对于通配名znFAT中是不对其进行文件名合法性检查的。请使用者自行
       注意。
******************************************************************************************/
#ifdef IS_WILDFILENAME
UINT8 Is_WildFileName(INT8 *pfn)
{ 
 UINT8 i=0;
 while('\0'!=pfn[i])
 {
  if('*'==pfn[i] || '?'==pfn[i])
  {
   return 1;
  }
  i++;
 }
 return 0;
}
#endif

/******************************************************************************************
 功能：在一个字符串中查找一个子串
 形参：str:字符串 substr:子串 pos:从字符串的pos位置开始查找子串
 返回：子串在字符串中的开始位置
 详解：
******************************************************************************************/
#ifdef FINDSUBSTR
UINT8 FindSubStr(INT8 *str,INT8 *substr,UINT8 pos)
{
 UINT8 i=pos,j=0,lens=(UINT8)StringLen(str),lent=(UINT8)StringLen(substr);

 while(i<lens && j<lent)
 {
  if(str[i]==substr[j] || '?'==substr[j])
  {
   i++;
   j++;
  }
  else
  {
   i=(UINT8)(i-j+1);
   j=0;
  }
 }
 if(j==lent) return (UINT8)(i-lent); 
 else return (UINT8)0XFF;
}
#endif

/*********************************************************************************************
 功能：文件名匹配，支持*与?通配
 形参：t:模板文件名，即使用者输入的文件名 s:目标文件名  例如t为a*b.txt s为axxb.txt，它们是匹配的
       再如t为a???x.t* s为axyzx.txt，它们是匹配的。
 返回：运行结果 是或否
 详解：这一匹配算法基于子串匹配，并不保证满足一切情况。其算法强度不可与DOS上的通配相提并论。
**********************************************************************************************/
#ifdef SFN_MATCH
UINT8 SFN_Match(INT8 *t,INT8 *s)
{
 UINT8 i=0,j=0,lens=(UINT8)StringLen(s);
 UINT8 lent=(UINT8)StringLen(t);
 INT8 buf[20];
 UINT8 bufp=0;

 //======================================================
 
 while(j<lent && '*'!=t[j])
 {
  buf[bufp]=(INT8)Lower2Up(t[j]);
  bufp++;
  j++;
 }

 if('\0'==t[j] && (lent!=lens)) return ERR_FAIL;

 buf[bufp]='\0';
 
 if(FindSubStr(s,buf,0)!=0) return ERR_FAIL;
 i=bufp;
 
 while(1)
 {
  while(j<lent && '*'==t[j]) j++;
  if(j==lent) return ERR_SUCC;
  bufp=0;

  while(j<lent && '*'!=t[j])
  {
   buf[bufp]=(INT8)Lower2Up(t[j]);
   bufp++;
   j++;
  }
  buf[bufp]='\0';
  
  if(j==lent)
  {
   if(FindSubStr(s,buf,i)!=(lens-bufp)) return ERR_FAIL;
   return ERR_SUCC;
  }

  i=FindSubStr(s,buf,i);
  if(0XFF==i) return ERR_FAIL;
  i+=bufp;
 }
}
#endif

/*********************************************************************************************
 功能：修改FAT表的某个表项
 形参：cluster:要修改的簇项号 next_cluster:要向簇项中写入的值 
 返回：运行结果 成功或失败
 详解：此函数使cluster链向next_cluster 如果cluster为0或1则放弃操作，因为有效的簇号是从2开始的
**********************************************************************************************/
#ifdef MODIFY_FAT
UINT8 Modify_FAT(UINT32 cluster,UINT32 next_cluster) //构造FAT簇链
{
 UINT32 temp1=0,temp2=0;

 if(0==cluster || 1==cluster) return ERR_FAIL; //簇项0与1是不能更改的

 temp1=pInit_Args->FirstFATSector+(cluster*4/pInit_Args->BytesPerSector);
 temp2=((cluster*4)%pInit_Args->BytesPerSector);

 znFAT_Device_Read_Sector(temp1,znFAT_Buffer);
 znFAT_Buffer[temp2+0]=(UINT8)( next_cluster&0x000000ff)     ;
 znFAT_Buffer[temp2+1]=(UINT8)((next_cluster&0x0000ff00)>>8 );
 znFAT_Buffer[temp2+2]=(UINT8)((next_cluster&0x00ff0000)>>16);
 znFAT_Buffer[temp2+3]=(UINT8)((next_cluster&0xff000000)>>24);
 znFAT_Device_Write_Sector(temp1,znFAT_Buffer);
 
 znFAT_Device_Read_Sector(temp1+pInit_Args->FATsectors,znFAT_Buffer);
 znFAT_Buffer[temp2+0]=(UINT8)( next_cluster&0x000000ff)     ;
 znFAT_Buffer[temp2+1]=(UINT8)((next_cluster&0x0000ff00)>>8 );
 znFAT_Buffer[temp2+2]=(UINT8)((next_cluster&0x00ff0000)>>16);
 znFAT_Buffer[temp2+3]=(UINT8)((next_cluster&0xff000000)>>24);
 znFAT_Device_Write_Sector(temp1+pInit_Args->FATsectors,znFAT_Buffer);

 return ERR_SUCC;
}
#endif

/********************************************************************************************
 功能：对cluster簇进行清0
 形参：cluster:要进行清0操作的簇
 返回：0
 详解：在某个目录下创建文件或目录时，此目录的现有簇已无空闲空间来写入文件目录项时，需要扩展空
       簇，即将空簇链到目录的簇链上，进而继续写入文件目录项，来完成文件或目录的创建。在向空簇
       写入文件目录项以前必须把这个空簇清空为0。因为向目录的簇中写入文件目录项，是以0来判断是
       否有空闲的位置。
********************************************************************************************/
#ifdef CLEAR_CLUSTER
UINT8 Clear_Cluster(UINT32 cluster)
{
 znFAT_Device_Clear_nSector((pInit_Args->SectorsPerClust),SOC(cluster));

 return 0;
}
#endif

/********************************************************************************************
 功能：更新下一空簇参考值 即znFAT中文件系统初始化参数集合中的Next_Free_Cluster
 形参：无
 返回：运行结果 
 详解：在每次需要使用空簇的时候，我们都可以直接去取pInit_Args->Next_Free_Cluster，但是用完之
       要及时更新其值，以为下一次使用空簇作好准备。
********************************************************************************************/
#ifdef UPDATE_NEXT_FREE_CLUSTER
UINT8 Update_Next_Free_Cluster(void)
{
 UINT32 clu_sec,iItem,iSec;
 struct FAT_Sec *pFAT_Sec;

 if(0!=pInit_Args->Free_nCluster) //磁盘仍有空间
 {
  pInit_Args->Free_nCluster--; //空闲簇数减1

  clu_sec=(pInit_Args->Next_Free_Cluster/NITEMSINFATSEC); //指定簇的簇项所在的扇区为其FAT区内的偏移量                                                                                 //FAT的首扇区                                                         
  znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer); //将簇项所在的扇区数据读入缓冲区
  pFAT_Sec=(struct FAT_Sec *)znFAT_Buffer; //将数据缓冲区首地址强转为FAT_Sec结构体的指针类型

  for(iItem=((pInit_Args->Next_Free_Cluster)%NITEMSINFATSEC)+1;iItem<NITEMSINFATSEC;iItem++) //检测在当前FAT扇区内当前簇项之后是否有空簇
  {
   if(0==(((pFAT_Sec->items)[iItem]).Item)[0]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[1]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[2]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[3])
   {
    pInit_Args->Next_Free_Cluster=(clu_sec*NITEMSINFATSEC)+iItem;
    #ifdef RT_UPDATE_FSINFO
    Update_FSINFO();
    #endif
	return ERR_SUCC;
   }
  }

  for(iSec=(clu_sec+1);iSec<(pInit_Args->FATsectors);iSec++) //在后面的FAT扇区中继续查找
  {
   znFAT_Device_Read_Sector(iSec+(pInit_Args->FirstFATSector),znFAT_Buffer);
   pFAT_Sec=(struct FAT_Sec *)znFAT_Buffer;
   for(iItem=0;iItem<NITEMSINFATSEC;iItem++) //检测在当前FAT扇区内当前簇项之后是否有空簇
   {
    if(0==(((pFAT_Sec->items)[iItem]).Item)[0]
    && 0==(((pFAT_Sec->items)[iItem]).Item)[1]
    && 0==(((pFAT_Sec->items)[iItem]).Item)[2]
    && 0==(((pFAT_Sec->items)[iItem]).Item)[3])
    {
     pInit_Args->Next_Free_Cluster=(iSec*NITEMSINFATSEC)+iItem;
     #ifdef RT_UPDATE_FSINFO
     Update_FSINFO();
     #endif
	 return ERR_SUCC;
    }
   }
  }
 }
 pInit_Args->Next_Free_Cluster=2; //如果已无空间，即nFreeCluster为0，则将下一空簇设为2
                                  //WINDOWS就是这样作的
 return ERR_NO_SPACE;
}
#endif

//=================for LFN=====以下代码用于实现长文件名===要使用长名功能 需要对USE_LFN这个宏进行定义===

/********************************************************************************************
 功能：判断一个文件名是否是长文件名
 形参：filename:指向文件名的指针
 返回：运行结果，是或否
 详解：判断一个文件名是否长文件名，非常重要。FAT32中对长名的定义比较零碎而复杂，一些看似短名
       的文件名实为长名。这里只使用长名判断的主要条件，建议在使用长名时，使用者以明确的形式
       给出长名。
********************************************************************************************/
#ifdef IS_LFN
UINT8 Is_LFN(INT8 *filename)
{
 UINT8 is_lfn=BOOL_FALSE;

 if(Check_SFN_Illegal_Length(filename)) is_lfn=BOOL_TRUE;
 if(Check_SFN_Dot(filename))            is_lfn=BOOL_TRUE;
 if(Check_SFN_Special_Char(filename))   is_lfn=BOOL_TRUE;
 if(((UINT8)(-1))==Check_SFN_Illegal_Lower(filename)) is_lfn=BOOL_TRUE;

 return is_lfn;
}
#endif

/***********************************************************************************************
 功能：从长名项中提取其中所含的部分长名的UNI码，此函数通过多次调用，实现了对整个长名UNI码的拼接
 形参：lfn_buf:指向用于装载长名UNI码的缓冲区 plfndi:指向长名项的指针 n:说明这是长名的第n部分
 返回：运行结果
 详解：FAT32中的长名是由多个长名项共同表达的，每个长名项记录着长名的某一段，此函数就是将长名项中
       的长名UNI码提取出来，放到缓冲区的相应位置上。经过多次调用此函数，最终可以拼接得到长名。
************************************************************************************************/
#ifdef GET_PART_NAME
UINT8 Get_Part_Name(UINT16 *lfn_buf,struct LFN_FDI *plfndi,UINT8 n)
{
 UINT8 i=0;
 UINT16 temp=0;

 if((plfndi->AttrByte[0])&0X40) Memory_Set(((UINT8 *)lfn_buf),2*(MAX_LFN_LEN+1),0); //如果是最后一个长名项，则清空lfn_buf

 for(i=0;i<5;i++) //第一部分长名
 {
  if(n>=MAX_LFN_LEN) return ERR_LFN_BUF_OUT;
  temp=(((UINT16)(((plfndi->Name1)+i*2)[0]))&0X00FF)|((((UINT16)(((plfndi->Name1)+i*2)[1]))&0X00FF)<<8);
  if(0==temp)
  {
   lfn_buf[n]=0;
   return 0;
  }
  else
  {
   lfn_buf[n]=temp;n++;
  }
 }

 for(i=0;i<6;i++) //第二部分长名
 {
  if(n>=MAX_LFN_LEN) return ERR_LFN_BUF_OUT;  
  temp=(((UINT16)(((plfndi->Name2)+i*2)[0]))&0X00FF)|((((UINT16)(((plfndi->Name2)+i*2)[1]))&0X00FF)<<8);
  if(0==temp)
  {
   lfn_buf[n]=0;
   return 0;
  }
  else
  {
   lfn_buf[n]=temp;n++;
  }
 }

 for(i=0;i<2;i++) //第三部分长名
 {
  if(n>=MAX_LFN_LEN) return ERR_LFN_BUF_OUT;  
  temp=(((UINT16)(((plfndi->Name3)+i*2)[0]))&0X00FF)|((((UINT16)(((plfndi->Name3)+i*2)[1]))&0X00FF)<<8);
  if(0==temp)
  {
   lfn_buf[n]=0;
   return 0;
  }
  else
  {
   lfn_buf[n]=temp;n++;
  }
 }

 n--;
 if((plfndi->AttrByte[0])&0X40)
 {
  while(((UINT16)(0XFFFF))==lfn_buf[n]) n--;
  lfn_buf[n+1]=0;
 }

 return 0;
}
#endif

/***********************************************************************************************
 功能：将OEM码（对于汉字来说，OEM码就是GB2312）转为UNICODE码
 形参：oem_code:OEM码 uni_code:指向用于记录oem_code所对应的UNICODE编码的变量的指针
 返回：运行结果  成功或失败
 详解：OEM码是各地区的计算机生产商在计算机固化的本地文字编码，比如在中国OEM码就是GB2312码，即区
       位码，但是FAT32的长文件名是以UNICODE来编码的，因此需要一个编码转换的过程。本函数的实现基于
       二分搜索，可快速查找到OEM码对应的UNICODE编码。
************************************************************************************************/
#ifdef OEMTOUNI
UINT8 OEM2UNI(UINT16 oem_code,UINT16 *uni_code) //通过二分法查表将OEM码转为UNI码
{  
 UINT32 low=0,high=MAX_UNI_INDEX-1,mid;//置当前查找区间上下界的初值 
 
 if(oem_code<GET_PGM_WORD(&(oem_uni[0][1]))) return ERR_FAIL;
 if(oem_code>GET_PGM_WORD(&(oem_uni[MAX_UNI_INDEX-1][1]))) return ERR_FAIL; //如果输入的oem_code不是表的范围内，则直接返回

 while(low<=high) //当前查找区间[low..high]非空
 {
  mid=low+(high-low)/2;

  if(oem_code==GET_PGM_WORD(&(oem_uni[mid][1])))
  {
   *uni_code=GET_PGM_WORD(&(oem_uni[mid][0]));

   return ERR_SUCC; //查找成功返回
  }

  if(GET_PGM_WORD(&(oem_uni[mid][1]))>oem_code)
  {
   high=mid-1;  //继续在[low..mid-1]中查找
  }
  else
  {
   low=mid+1; //继续在[mid+1..high]中查找
  }
 }											

 return ERR_FAIL; //当low>high时表示查找区间为空，查找失败
}
#endif

/***********************************************************************************************
 功能：将OEM编码的字符串转为由UNICODE编码的字符串
 形参：oemstr:指向oem编码的字符串  unistr:指向UNICODE编码的字符串
 返回：运行结果  成功 字符集不完整 或 长名缓冲溢出
 详解：引入长文件名之后，涉及到长文件名的比对和匹配，因此必然要将oem编码的字符串转为UNICODE编码
       的字符串，以便与由长名项中提取拼接而成的UNICODE字符串进行比对。
************************************************************************************************/
#ifdef OEMSTRTOUNISTR
UINT8 oemstr2unistr(INT8 *oemstr,UINT16 *unistr)
{
 UINT32 len=StringLen(oemstr);
 UINT32 i=0,pos=0;
 UINT8 res=0;
 UINT16 temp=0;

 for(i=0;i<len;i++)
 {
  if(IS_ASC(oemstr[i])) //检查是否是ASCII码，ASCII码的数值范围为0X00~0X7F，OEM编码值不在此范围，以此区分是ASCII还是OEM
  {
   unistr[pos]=(UINT16)(((UINT16)oemstr[i])&0X00FF);
   pos++;
  }
  #ifdef USE_OEM_CHAR
  else //不是ASCII码，而是OEM编码
  {
   temp=((((UINT16)oemstr[i])<<8)&0xff00);
   temp|=(((UINT16)oemstr[i+1])&0x00ff);
   res=OEM2UNI(temp,unistr+pos);
   if(res) 
   {
	  unistr[0]=0;
	  return ERR_OEM_CHAR_NOT_COMPLETE;
   }
   pos++;i++;
  }
  #endif

  if(pos>MAX_LFN_LEN) 
  {
   unistr[0]=0;
   return ERR_LFN_BUF_OUT; 
  }
 }

 unistr[pos]=0;

 return ERR_SUCC;
}
#endif

/***********************************************************************************************
 功能：对宽字符串（即每个字符均占用两个字节，如UNICODE）进行子串查找
 形参：str:指向宽字符串的指针  substr:指向子串的指针  pos:要查找的起始位置
 返回：子串在宽字符串中的起始位置，如果未找到，则返回-1
 详解：此函数基本与前面SFN的匹配中的查找子串的函数同理，只不过字符串被扩展为了宽字符串
************************************************************************************************/
#ifdef WFINDSUBSTR
UINT32 WFindSubStr(UINT16 *str,UINT16 *substr,UINT32 pos)
{
 UINT32 i=pos,j=0,lens=WStringLen(str),lent=WStringLen(substr);

 while(i<lens && j<lent)
 {
  if(WLower2Up(str[i])==substr[j] || (UINT16)'?'==substr[j])
  {
   i++;
   j++;
  }
  else
  {
   i=i-j+1;
   j=0;
  }
 }

 if(j==lent) return i-lent; 
 else return (UINT32)-1;
}
#endif

/***********************************************************************************************
 功能：对两个宽字符串进行匹配，实为长文件名的通配比对
 形参：t:指向模版宽字符串的指针  s:指目标字符串的指针  例t:振南*无限.txt s:振南电子原创无限.txt
       它们是匹配的。
 返回：运行结果 成功或失败
 详解：此函数基本与前面SFN的匹配函数SFN_Match同理。
************************************************************************************************/
#ifdef LFN_MATCH
UINT8 LFN_Match(UINT16 *t,UINT16 *s)
{
 UINT32 i=0,j=0,lens=WStringLen(s),lent=WStringLen(t);
 UINT16 bufp=0;

 UINT16 buf[MAX_LFN_LEN+1];

 //======================================================
 
 while(j<lent && (UINT16)'*'!=t[j])
 {
  buf[bufp]=WLower2Up(t[j]);
  bufp++;
  j++;
 }

 if(0==t[j] && (lent!=lens)) return ERR_FAIL;

 buf[bufp]=0;
 
 if(WFindSubStr(s,buf,0)!=0) return ERR_FAIL;
 i=bufp;
 
 while(1)
 {
  while(j<lent && (UINT16)'*'==t[j]) j++;
  if(j==lent) return ERR_SUCC;
  bufp=0;

  while(j<lent && (UINT16)'*'!=t[j])
  {
   buf[bufp]=(UINT16)WLower2Up(t[j]);
   bufp++;
   j++;
  }
  buf[bufp]=0;
  
  if(j==lent)
  {
   if(WFindSubStr(s,buf,i)!=(lens-bufp)) return ERR_FAIL;
   return 0;
  }

  i=WFindSubStr(s,buf,i);
  if(((UINT32)(-1))==i) return ERR_FAIL;
  i+=bufp;
 }
}
#endif

/***********************************************************************************************
 功能：从短名文件目录项中获取绑定校验和
 形参：pdi:指向短名文件目录项的指针 
 返回：绑定校验和的值
 详解：此函数所使用的校验算法来自于微软的FAT32相关文档。一个文件的文件名如果为长文件名，则它将有
       若干个长名项来共同表达长名，同时还有与之对应的短名项。比如 ABCDEFGHI.RMVB 有2个长名项（每
       个长名项可记录13个UNICODE码），与之对应的短名可能为ABCDEF~1.RMV。通常情况下，短名紧紧跟在
       长名项后面，但它们之间并不光靠位置关系来保证其联系，还有一种绑定校验和算法：通过短名可以
       计算得到一个值，长名项中有专门的字段也记录了这个值，如果这两个值相等，则认为长名项与短名
       项是有关联的，配对成功。
************************************************************************************************/
#ifdef GET_BINDING_SUMCHK
UINT8 Get_Binding_SumChk(struct FDI *pdi)
{
 UINT8 i=0,c=0;

 for(i=0;i<11;i++)
 {
  c=(UINT8)(((c&0X01)?0X80:0)+(c>>1)+((pdi->Name)[i]));
 }

 return c;
}
#endif

/***********************************************************************************************
 功能：ELF哈希算法，用于由一个字符串计算得到一个值，这个值与字符串是唯一对应的
 形参：str:指向字符串的指针
 返回：计算得到的哈希值
 详解：在创建文件和目录时，如果是长名，在向目录簇中写入长名项的同时，也要写入其对应的短名项，而
       短名如何确定（FAT32中使用加数字后缀的方法，如XXXXXX~n.YYY，但n<=5，n>5的时候就需要一种算
       法由长名直接计算得到一个唯一与之对应的短文件名，此算法即可由HASH算法来实现）。ELF哈希是
       众多哈希算法中用得比较广泛的，它高效而且保证了哈希值与字符串的唯一对应（不同的字符串拥有
       相同的哈希值的概率是极小的）。基于这个唯一的哈希值我们可以快速构造出长名对应的短名。
************************************************************************************************/
#ifdef	ELFHASH
UINT32 ELFHash(INT8 *str)
{
 UINT32 hash=0;
 UINT32 x=0;

 while(*str)
 {
  hash=(hash<<4)+(*str++); //hash左移4位，当前字符ASCII存入hash低四位。 
  x=hash&0xF0000000;
  if(0!=x)
  { //如果最高的四位不为0，则说明字符多余7个，如果不处理，再加第九个字符时，第一个字符会被移出，因此要有如下处理。
    //该处理，如果对于字符串(a-z 或者A-Z)就会仅仅影响5-8位，否则会影响5-31位，因为C语言使用的算数移位
   hash ^= (x >> 24);
   //清空28-31位。
   hash &= ~x;
  }
 }

 //返回一个符号位为0的数，即丢弃最高位，以免函数外产生影响。(我们可以考虑，如果只有字符，符号位不可能为负)
 return (hash&0X7FFFFFFF);
}
#endif

/***********************************************************************************************
 功能：将一个32位的整形值转为16进制字符串 例：0X12345678转为"12345678"
 形参：hex:一个32位的整形数 str:指向字符串的指针
 返回：0
 详解：此函数用于将上面的哈希函数计算得到的32位的哈希值转为字符串，以便进行短文件名的构造。
************************************************************************************************/
#ifdef HEX2STR_32B
UINT8 Hex2Str_32b(UINT32 hex,INT8 *str)
{
 UINT8 i=0,temp=0;
 for(i=0;i<8;i++)
 {
  temp=((unsigned char)((hex&(0X0000000F<<(i*4)))>>(i*4)));
  str[7-i]=(INT8)((temp>=10)?('A'+temp-10):(temp+0X30));	
 }
 str[i]=0;
 
 return 0;
}
#endif

/***********************************************
 功能：由长名构造其对应的短名
 形参：pfn:指针向长名的指针 psfn:指向短名的指针
 返回：0
 详解：此函数还需要进一步修改，但暂可用。
************************************************/
#ifdef MAKE_SHORT_NAME
UINT8 Make_Short_Name(char *pfn,char *psfn) //此函数用于生成长名对应的短名，使用了ELFHASH算法用于生成唯一的短名
{
 //psfn[0]=Lower2Up(pfn[0]);
 //psfn[1]='~';
 UINT32 temp=ELFHash(pfn);
 Hex2Str_32b(temp,psfn);
 //psfn[0]='*';
 psfn[8]='.';
 psfn[9]=psfn[10]=psfn[11]='A';
 psfn[12]=0;

 return 0;
}
#endif

/******************************************************************************
 功能：构造第n个长名项
 形参：unifn:指向UNICODE编码的长文件名 plfni:指向用于存储构造的长名项的变量指针 
       ChkSum:由短名项计算得到的绑定校验和 n:指示是第几个长名项
 返回：0
 详解：构造长名项是znFAT中实现长文件名的一个关键功能函数
*******************************************************************************/
#ifdef FILL_LFN_FDI
UINT8 Fill_LFN_FDI(UINT16 *unifn,struct LFN_FDI *plfni,UINT8 ChkSum,UINT8 n)
{
 UINT8 temp=(UINT8)(n*13),i=0,j=0;
 UINT16 len=(UINT16)WStringLen(unifn+temp);

 (plfni->AttrByte[0])=(UINT8)(n+1); //长名项的序号字节
 if(len<=13) (plfni->AttrByte[0])|=0X40; //如果发现从当前位置开始的UNI串长度小于等于13，则说明书这是最后一个长名项

 (plfni->ChkVal[0])=ChkSum; //写入与SFN的绑定校验值
 (plfni->LFNSign[0])=0X0F; //长名项的标志

 (plfni->Resv[0])=(plfni->StartClu[0])=(plfni->StartClu[1])=0;

 for(i=0;i<10;i++) (plfni->Name1)[i]=0XFF; 
 for(i=0;i<12;i++) (plfni->Name2)[i]=0XFF;
 for(i=0;i<4;i++)  (plfni->Name3)[i]=0XFF; //先把长名UNI的字段用0XFF填充
 
 for(i=0;i<5;i++)
 {
  (plfni->Name1)[2*i]=(UINT8)(unifn[j+temp]&0X00FF);
  (plfni->Name1)[2*i+1]=(UINT8)((unifn[j+temp]&0XFF00)>>8);
  j++;
  if(j>=len) 
  {
   if(4==i) 
   {
    (plfni->Name2)[0]=(plfni->Name2)[1]=0;
   }
   else
   {
	i++;
    (plfni->Name1)[2*i]=(plfni->Name1)[2*i+1]=0;
   }
   return ERR_SUCC; 
  }
 }

 for(i=0;i<6;i++)
 {
  (plfni->Name2)[2*i]=(UINT8)(unifn[j+temp]&0X00FF);
  (plfni->Name2)[2*i+1]=(UINT8)((unifn[j+temp]&0XFF00)>>8);
  j++;
  if(j>=len)
  {
   if(5==i) 
   {
    (plfni->Name3)[0]=(plfni->Name3)[1]=0;
   }
   else
   {
	i++;
    (plfni->Name2)[2*i]=(plfni->Name2)[2*i+1]=0;
   }
   return ERR_SUCC; 
  }
 }

 for(i=0;i<2;i++)
 {
  (plfni->Name3)[2*i]=(UINT8)(unifn[j+temp]&0X00FF);
  (plfni->Name3)[2*i+1]=(UINT8)((unifn[j+temp]&0XFF00)>>8);
  j++;
  if(j>=len)
  {
   if(1!=i) 
   {
	i++;
    (plfni->Name3)[2*i]=(plfni->Name3)[2*i+1]=0;
   }
   return ERR_SUCC; 
  }
 }
 
 return ERR_SUCC;
}
#endif

/********************************************************************************
 功能：向目录簇中注册长名项及其对应的短名项，并返回短名项所在的扇区与扇区内的位置
 形参：cluster:要创建的文件或目录所在的目录的首簇 pfdi:指向短名项的指针 
       unifn:指向UNICODE编码的长名的指针 psec:指向用于存储短名项所在扇区的变量指针
       pn:指向用于记录短名项在其所在扇区中的位置的变量的指针
 返回：运行结果，成功或错误号
 详解：因为长名是由多个长名项和一个短名项所共同来进行表达的，因此创建长名的文件或
       目录，其核心操作就是可以一次性向目录簇中注册多个文件目录项。此函数基本上是
       对Refigster_FDI（它用于实现为短名注册文件目录项）的扩展。
*********************************************************************************/
#ifdef REGISTER_LFN_FDI
UINT8 Register_LFN_FDI(UINT32 cluster,struct FDI *pfdi,UINT16 *unifn,UINT32 *psec,UINT8 *pn)
{
 UINT32 temp_sec=0,old_cluster=0;
 UINT8 iClu=0,iSec=0,iFDI=0,res=0;
 struct FDIesInSEC *pitems;
 struct FDI *pitem;

 struct LFN_FDI *plfni;

 UINT8 have_lfn=0; //在文件目录项扫描过程种，用于标记是否有长名
 UINT8 is_lfn_buf_err=0,cur_binding_sumchk=0,flag=0,chksum=Get_Binding_SumChk(pfdi);
 UINT16 lfn_buf[MAX_LFN_LEN+1]; //用于装载长名UNI码的缓冲
 struct LFN_FDI temp_lfni; //用于构造长名项的临时结构体变量

 UINT16 len=(UINT16)WStringLen(unifn),xlen=0; //计算UNI串长度
 UINT8 nclu=0,nsec=0,nlfni=(UINT8)(len/13);
 if(len%13) nlfni++; //计算UNI串会分为多少个长名项

 //===========================================================================================

 if(0==pInit_Args->Free_nCluster) return ERR_NO_SPACE; //如果没有空间，则直接返回
 
 //以下代码检查是否已经存在重复的文件或目录，以及查询空位并填入目录项
 do
 {
  temp_sec=SOC(cluster);
  for(iSec=0;iSec<(pInit_Args->SectorsPerClust);iSec++)
  {
   znFAT_Device_Read_Sector(temp_sec+(UINT32)iSec,znFAT_Buffer);
   pitems=((struct FDIesInSEC *)znFAT_Buffer);

   for(iFDI=0;iFDI<NFDI_PER_SEC;iFDI++)
   {
	pitem=&(pitems->FDIes[iFDI]); //指向一个文件目录项数据

	if((0X08!=pitem->Attributes) && (0X0F!=pitem->Attributes)  //不是卷标、不是长名项、没有删除、不是.与..
       && (0XE5!=pitem->Name[0]) && ('.'!=pitem->Name[0])) //即遇到一个合法的SFN项
	{
	 if(have_lfn && !is_lfn_buf_err) //遇到SFN项后，它前面可能有长名项，如果有则与输入的UNI串比对，以确定是否有同名文件或目录
	 {                               //而且没有发生lfn_buf的溢出错误
      if(cur_binding_sumchk==Get_Binding_SumChk(pitem)) //如果LFN与SFN目录项绑定校验和相等，则认为长名有效
	  { 
	   xlen=(UINT16)WStringLen(lfn_buf); //计算从各长名项合成后的UNI串的长度
	   if(xlen==len && Memory_Compare(((UINT8 *)unifn),((UINT8 *)lfn_buf),2*((UINT32)len))) //对UNI串进行比对，长度和内容要相等
	   {
            *psec=temp_sec+(UINT32)iSec;
            *pn=iFDI; //记录重名文件目录项的位置，以便对其进行解析
	    return ERR_FDI_ALREADY_EXISTING; //已有同名文件或目录
	   }
	  }
	 }
         if(is_lfn_buf_err) is_lfn_buf_err=0; //恢复长名项UNI提取的错误标记
	}

    if(CHK_ATTR_LFN(pitem->Attributes) && (0XE5!=pitem->Name[0])) //是长名项，而且没有被删除
	{
	 have_lfn=1;
	 
	 plfni=(struct LFN_FDI *)pitem;

     cur_binding_sumchk=(plfni->ChkVal)[0]; //获取长名项的绑定校验和

     res=Get_Part_Name(lfn_buf,plfni,(UINT8)((((plfni->AttrByte[0])&0XBF)-1)*13)); //将当前LFN项中的文件名UNICODE码拼入临时缓冲
                                                                      //此临时缓冲长度为MAX_LFN_LEN，如果越界，则
	                                                                  //不再装入，最终造成LFN的截断。
	 if(res) is_lfn_buf_err=1; //发生LFN_BUF溢出错误
	}
	else
	{
	 have_lfn=0;
	}

	if(0==(pitem->Name)[0])
	{
     flag=1; //标记此扇区已经被修改

	 if(nlfni>0)
	 {
	  Fill_LFN_FDI(unifn,&temp_lfni,chksum,(UINT8)(nlfni-1));
      Memory_Copy(((UINT8 *)pitem),((UINT8 *)(&temp_lfni)),FDI_NBYTES); //将合成的长名项拷贝到扇区数据中
	  nlfni--;
	 }
	 else
	 {
	  *psec=temp_sec+(UINT32)iSec;
	  *pn=iFDI; //记录空位的位置
	  Memory_Copy(((UINT8 *)pitem),((UINT8 *)pfdi),FDI_NBYTES); //将短名项拷贝到扇区数据中
	  znFAT_Device_Write_Sector(temp_sec+(UINT32)iSec,znFAT_Buffer);
	  return ERR_SUCC;
	 }	 
	}
   }

   if(flag) 
   {
	znFAT_Device_Write_Sector(temp_sec+(UINT32)iSec,znFAT_Buffer);
	flag=0;
   }
  }
  old_cluster=cluster;
  cluster=Get_Next_Cluster(cluster);
 }while(!IS_END_CLU(cluster)); //如果不是最后一个簇，则继续循环
 //===========================================================
 //如果运行到这里，则说明当前簇中已无空位
 if(0!=pInit_Args->Free_nCluster) //如果剩余空簇数为0，则说明磁盘已无空间
 {
  nsec=(UINT8)((nlfni+1)/NFDI_PER_SEC); //剩余的长名项还需要多少个扇区,+1是因为最后还有一个短名项
  if((nlfni+1)%NFDI_PER_SEC) nsec++; 

  nclu=(UINT8)(nsec/(pInit_Args->SectorsPerClust));
  if(nsec%(pInit_Args->SectorsPerClust)) nclu++;

  for(iClu=0;iClu<nclu;iClu++)
  {
   Modify_FAT(old_cluster,pInit_Args->Next_Free_Cluster);
   Modify_FAT(pInit_Args->Next_Free_Cluster,0X0FFFFFFF); //构造FAT簇链
   Clear_Cluster(pInit_Args->Next_Free_Cluster); //清空空闲簇

   temp_sec=SOC(pInit_Args->Next_Free_Cluster);

   old_cluster=pInit_Args->Next_Free_Cluster;
   Update_Next_Free_Cluster();

   for(iSec=0;iSec<(pInit_Args->SectorsPerClust);iSec++)
   {
    znFAT_Device_Read_Sector(temp_sec+(UINT32)iSec,znFAT_Buffer);
	pitems=((struct FDIesInSEC *)znFAT_Buffer);
    
	for(iFDI=0;iFDI<NFDI_PER_SEC;iFDI++)
	{
	 pitem=&(pitems->FDIes[iFDI]); //指向一个文件目录项数据

	 if(nlfni>0)
	 {
	  Fill_LFN_FDI(unifn,&temp_lfni,chksum,(UINT8)(nlfni-1));
      Memory_Copy(((UINT8 *)pitem),((UINT8 *)(&temp_lfni)),FDI_NBYTES); //将合成的长名项拷贝到扇区数据中
	  nlfni--;
	 }
	 else
	 {
	  *psec=temp_sec+(UINT32)iSec;
	  *pn=iFDI; //记录空位的位置
	  Memory_Copy(((UINT8 *)pitem),((UINT8 *)pfdi),FDI_NBYTES); //将短名项拷贝到扇区数据中
	  znFAT_Device_Write_Sector(temp_sec+(UINT32)iSec,znFAT_Buffer);
	  return ERR_SUCC;
	 }
	}
	znFAT_Device_Write_Sector(temp_sec+(UINT32)iSec,znFAT_Buffer);
   }   
  }
  return ERR_SUCC;  
 }
 else
 {
  return ERR_NO_SPACE;
 } 
}
#endif

//===================以上代码用于实现长文件名功能===for LFN=======================================

/************************************************************************************
 功能：打开文件
 形参：pfi:指向文件信息集合的指针 filepath:文件的路径 n:序号 is_file:要打开是否是文件
 返回：运行结果，成功 或 错误码
 详解：此函数的功能比较多样性，文件的路径支持无限深层目录 如 /a/b/c/d/e/f/g/..../a.txt
       路径中的目录分隔符可以为/或\（在C中字符串中表达\要这样写\\），文件名中支持能配
       符，如/a/b/c/d/.../a*.txt /??abc.txt ，文件名支持长文件名，而且无论长名短名均支
       持通配，在使用通配文件名时，与之匹配的文件可能会有多个，n用以指定要打开的是第几
       个文件，此函数亦可打开目录，文件或目录打开后，其信息将被装入到pfi所指向的文件
       信息集合中。注：对于目录虽然存储形式上与文件相似，都是以文件目录项来存储，但它
       没有文件大小等字段（恒定为0）。
*************************************************************************************/
#ifdef ZNFAT_OPEN_FILE 
UINT8 znFAT_Open_File(struct FileInfo *pfi,INT8 *filepath,UINT32 n,UINT8 is_file)
{
 UINT8 result=0,flag=0;
 UINT32 sec_temp=0,Cur_Cluster=0,fn_pos=0,item=0;
 UINT32 iSec=0,iFDI=0;

 //===========for LFN=======
 #ifdef USE_LFN
 struct LFN_FDI *plfni;

 UINT8 cur_binding_sumchk=0;
 UINT8 is_wfn=0;
 UINT16 temp_lfn[MAX_LFN_LEN+1]; //用于装载长名UNICODE码的临时缓冲
 UINT8 is_lfn=0; //输入的文件名是否是长名
 UINT8 is_lfn_buf_err=0; //产生长名缓冲溢出错误，此标记为1则直接跳过长名比对
 #endif
 //===========for LFN=======

 INT8 temp_filename[13];
 INT8 *filename;

 struct FDIesInSEC *pitems; //指向文件目录项扇区数据的指针
 struct FDI *pitem; //指向文件目录项数据的指针

 #ifdef USE_LFN
 pfi->have_lfn=BOOL_FALSE; //先设定文件无长名
 #endif

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=0;
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 Memory_Set((UINT8 *)pcccb_buf,sizeof(UINT32)*CCCB_LEN,0);
 #endif
 #endif

 #ifdef USE_EXCHANGE_BUFFER
 #ifdef USE_ALONE_EXB
 //Memory_Set(just_file->exb_buf,512,0);
 just_file->exb_cursec=0;
 #endif
 #endif

 result=znFAT_Enter_Dir(filepath,&Cur_Cluster,&fn_pos); //获取路径最后一级目录的开始簇
 if(result) return result;

 filename=filepath+fn_pos; //filename指向filepath中的文件名
                           //注：如果打开的是目录，则路径filepath不以/或\结束，如打开目录\a\b\c 
                           //不要写成\a\b\c\，这样会造成打开失败
                           //znFAT认为你要打开c目录下的一个名字为空的目录
                           //打开目录与进入目录不同，进入目录仅获取目录首簇，而打开目录
                           //则对目录进行解析，将文件相关参数填入文件信息集合，如目录创建时间等

 if(Check_Illegal_Char(filename)) return ERR_ILL_CHAR; //检查文件名中是否有非法字符，无论长名还是短名，或是通配名 

 //这里主要针对于LFN、SFN及通配名进行相关检测
 if(!Is_WildFileName(filename)) //如果不是通配文件名，即确定名，则需要进行文件名合法性检测
 {
  #ifdef USE_LFN
  if(!Is_LFN(filename)) //如果不是长名,即是短名
  {
  #endif
   //检查SFN合法性，若非法则直接返回，不再进行后面的处理(此处对SFN的合法性检查比较严格)
   //事先检查SFN的合法性，减少后面处理上的麻烦

   if(Check_SFN_Illegal_Length(filename)) return ERR_SFN_ILL_LEN; //检查SFN是否符合8.3长度
   if(Check_SFN_Dot(filename)) return ERR_SFN_DOT; //检查SFN中.是否合法 
   if(Check_SFN_Special_Char(filename)) return ERR_SFN_SPEC_CHAR; //检查SFN中是否有特殊字符
   if(((UINT8)(-1))==Check_SFN_Illegal_Lower(filename)) return ERR_SFN_ILL_LOWER; //检查SFN中是否有非法的大小写

  #ifdef USE_LFN
  }
  else //如果是长名
  {
   is_lfn=1; //标记输入的文件名为长名
   result=oemstr2unistr(filename,temp_lfn);//把filename转为UNICODE码，存在temp_lfn里，以便后面进行文件名比对
   if(result) return result;
  }
  #endif
 }
 else //如果是通配名，即含有*或?
 {
  #ifdef USE_LFN
  is_wfn=1; //标志输入的文件名为通配名
  is_lfn=1; //在这种情况下，也认为是长名，通配名也要与LFN相比对
  result=oemstr2unistr(filename,temp_lfn); //转为带通配符的UNI串
  if(result) return result; //OEM字符集不完全，OEM->UNI转换中有字符找不到 或 LFN缓冲溢出
  #endif
 }

 //================================================
 do
 {
  sec_temp=SOC(Cur_Cluster); //当前簇首扇区
  for(iSec=0;iSec<(pInit_Args->SectorsPerClust);iSec++) 
  {
   znFAT_Device_Read_Sector(sec_temp+(UINT32)iSec,znFAT_Buffer);
   pitems=(struct FDIesInSEC *)znFAT_Buffer; 

   for(iFDI=0;iFDI<NFDI_PER_SEC;iFDI++) //访问扇区中各文件目录项
   {
    pitem=&(pitems->FDIes[iFDI]); //指向一个文件目录项数据
     
    if((is_file?CHK_ATTR_FILE(pitem->Attributes):CHK_ATTR_DIR(pitem->Attributes)) 
		&& (0XE5!=pitem->Name[0]) && ('.'!=pitem->Name[0])) //依is_file检查属性，且没有被删除
		                                                    //不是.与..
    {
     To_File_Name((INT8 *)(pitem->Name),temp_filename); //将FDI中的文件名字段转为8.3文件名

	 #ifdef USE_LFN	
     if(!is_lfn || is_wfn) //如果输入的文件名不是长文件名，即为短文件名，或者是配通名，则要进行SFN比对
	 #endif                //主要是为了防止多余的操作，如果输入文件名为长文件名，则根本不用SFN比对，就
		                   //算比对，结果也一定是不匹配的
     {
      if(!SFN_Match(filename,temp_filename)) //短文件名通配
	  {
	   if(n==item)
	   {
        Analyse_FDI(pfi,pitem); //解析匹配的文件目录项
	    pfi->FDI_Sec=sec_temp+iSec; //文件目录项所在的扇区
	    pfi->nFDI=(UINT8)iFDI; //文件目录项在扇区中的索引

        #ifdef USE_LFN
	    if(!pfi->have_lfn) (pfi->longname)[0]=0;
        #endif

	    return ERR_SUCC;
	   } 
	   flag=1;
	  }
	 }

     #ifdef USE_LFN
     if(is_lfn && (pfi->have_lfn) && !is_lfn_buf_err) //如果输入的文件名为长名，而且也发现了长名
	 {
	  if(cur_binding_sumchk==Get_Binding_SumChk(pitem)) //如果LFN与SFN目录项绑定校验和相等，则认为
	  {	                                                //长名有效
       if(!LFN_Match(temp_lfn,(pfi->longname))) //长文件名通配
	   {
	    if(n==item)
		{
         Analyse_FDI(pfi,pitem); //解析匹配的文件目录项
	     pfi->FDI_Sec=sec_temp+iSec; //文件目录项所在的扇区
	     pfi->nFDI=(UINT8)iFDI; //文件目录项在扇区中的索引
	     return ERR_SUCC;
		}
	    flag=1;
	   }	
	  }
	 }

	 if(is_lfn_buf_err) is_lfn_buf_err=0;
     #endif

	 if(flag) {item++;flag=0;} //如果LFN（如果用了LFN的话）与SFN有一项比对成功，但item不匹配，则item++
	}

    #ifdef USE_LFN
		if((CHK_ATTR_DIR(pitem->Attributes))||(CHK_ATTR_FILE(pitem->Attributes))) //如果遇到短名项恢复长度错误标记
			if(is_lfn_buf_err) is_lfn_buf_err=0;

    if(CHK_ATTR_LFN(pitem->Attributes) && (0XE5!=pitem->Name[0]) && is_lfn) //是长名项，而且没有被删除，并且输入的文件名是长名
	{                                                                       //因为如果我们输入的文件名本身就不是长名，那我们就没必要去关心长名项
     pfi->have_lfn=1;
	 
	 plfni=(struct LFN_FDI *)pitem;

	 cur_binding_sumchk=(plfni->ChkVal)[0]; //获取长名项的绑定校验和
    
     result=Get_Part_Name(pfi->longname,plfni,(UINT8)((((plfni->AttrByte[0])&0XBF)-1)*13)); //将当前LFN项中的文件名UNICODE码拼入临时缓冲
                                                                      //此临时缓冲长度为MAX_LFN_LEN，如果越界，则
	                                                                  //不再装入，最终造成LFN的截断。
	 if(result) is_lfn_buf_err=1; //发生LFN_BUF溢出错误
	}
	else
	{
	 pfi->have_lfn=0;
	}
    #endif

   }
  }

  Cur_Cluster=Get_Next_Cluster(Cur_Cluster); //获取下一簇
 }while(!IS_END_CLU(Cur_Cluster)); //如果不是最后一个簇，则继续循环

 return ERR_NO_FILE;
}
#endif

/************************************************************************************
 功能：获取一个目录的开始簇
 形参：dir_name:目录名 pCluster:指向用于存储簇号的变量的指针 注：此指针不光用于将目录
       所在的目录簇号传入，同时又接收目录的开始簇。比如：我们要获取 /a/b/c c目录的开
       簇，pCluster首先传入b的开始簇，然后程序会在这个开始簇及其后继簇中寻找为名dir_n
       ame的目录项，找到后，将其开始通过pCluster传回，即pCluster所指向的变量发生了改
       变。
 返回：运行结果   成功或错误号
 详解：此函数的实现过程与打开文件差不多，而且比它还简单些。dir_name同样支持长文件名及
       通配。
*************************************************************************************/
#ifdef GET_DIR_START_CLUSTER
UINT8 Get_Dir_Start_Cluster(INT8 *dir_name,UINT32 *pCluster) //获取目录下名为dir_name的子目录的开始簇
{                                                            //目录的首簇存入pCluster所指入的变量
 UINT32 sec_temp=0;
 UINT8 iSec=0,iFDI=0;
 UINT32 Cur_Clust=*pCluster;

 //===========for LFN=======
 #ifdef USE_LFN
 struct LFN_FDI *plfni;

 UINT8 cur_binding_sumchk=0,result=0;
 UINT16 temp_lfn[MAX_LFN_LEN+1]; //用于装载长名UNICODE码的临时缓冲
 UINT16 lfn_buf[MAX_LFN_LEN+1]; //从长名项中提取的UNI码装到这个缓冲中
 UINT8 is_lfn=0; //输入的文件名是否是长名
 UINT8 have_lfn=0; //在搜索目录项时发现了长名项
 UINT8 is_lfn_buf_err=0; //产生长名缓冲溢出错误，此标记为1则直接跳过长名比对
 #endif

 //===========for LFN=======

 INT8 temp_dirname[13];

 struct FDIesInSEC *pitems; //指向文件目录项扇区数据的指针
 struct FDI *pitem; //指向文件目录项数据的指针

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=0;
 #endif

 if(Check_Illegal_Char(dir_name)) return ERR_ILL_CHAR; //检查文件名中是否有非法字符

 #ifdef USE_LFN
 if(!Is_LFN(dir_name)) //如果不是长名,即是短名
 {
 #endif
  //检查短文件名合法性，若非法则直接返回，不再进行后面的处理(此处对SFN的合法性检查非常严格)
  //事先检查SFN的合法性，减少后面处理上的麻烦
  if(Check_SFN_Illegal_Length(dir_name)) return ERR_SFN_ILL_LEN; //检查SFN是否符合8.3长度
  if(Check_SFN_Dot(dir_name)) return ERR_SFN_DOT; //检查SFN中.是否合法  
  if(Check_SFN_Special_Char(dir_name)) return ERR_SFN_SPEC_CHAR; //检查SFN中是否有特殊字符
  if(((UINT8)(-1))==Check_SFN_Illegal_Lower(dir_name)) return ERR_SFN_ILL_LOWER; //检查SFN中是否有非法的大小写
 #ifdef USE_LFN
 }
 else //如果是长名
 {
  is_lfn=1; //标记输入的文件名为长名
  result=oemstr2unistr(dir_name,temp_lfn);//把filename转为UNICODE码，存在temp_lfn里，以便后面进行文件名比对
  if(result) return result;
 }
 #endif

 //================================================
 do
 {
  sec_temp=SOC(Cur_Clust); //当前簇首扇区
  for(iSec=0;iSec<(pInit_Args->SectorsPerClust);iSec++) 
  {
   znFAT_Device_Read_Sector(sec_temp+(UINT32)iSec,znFAT_Buffer);
   pitems=(struct FDIesInSEC *)znFAT_Buffer; 

   for(iFDI=0;iFDI<NFDI_PER_SEC;iFDI++) //访问扇区中各文件目录项
   {
    pitem=&(pitems->FDIes[iFDI]); //指向一个文件目录项数据
     
    if((CHK_ATTR_DIR(pitem->Attributes)) && (0XE5!=pitem->Name[0])) //文件属性为目录，且没有被删除
    {
     To_File_Name((INT8 *)(pitem->Name),temp_dirname); //将FDI中的目录名字段转为8.3文件名
     #ifdef USE_LFN
     if(!is_lfn)
     #endif
	 {
      if(!SFN_Match(dir_name,temp_dirname)) //目录名匹配
	  {
	   //获取目录的开始簇 
	   *pCluster=(Bytes2Value(pitem->LowClust,2))|(Bytes2Value(pitem->HighClust,2)<<16); 
	   return ERR_SUCC;
	  }
	 }

     #ifdef USE_LFN
     if(is_lfn && have_lfn && !is_lfn_buf_err) //如果输入的文件名为长名，而且也发现了长名
	 {                                         //同时又没有发生长名缓冲溢出错误
	  if(cur_binding_sumchk==Get_Binding_SumChk(pitem)) //如果LFN与SFN目录项绑定校验和相等，则认为
	  {	                                                //长名有效
       if(!LFN_Match(temp_lfn,lfn_buf)) //长文件名通配
	   {
	    //获取目录的开始簇 
	    *pCluster=(Bytes2Value(pitem->LowClust,2))|(Bytes2Value(pitem->HighClust,2)<<16); 
		return ERR_SUCC;
	   }	
	  }
	 }
     #endif

    }

    #ifdef USE_LFN
	if((CHK_ATTR_DIR(pitem->Attributes))||(CHK_ATTR_FILE(pitem->Attributes))) //如果遇到短名项恢复长度错误标记
			if(is_lfn_buf_err) is_lfn_buf_err=0;

    if(CHK_ATTR_LFN(pitem->Attributes) && (0XE5!=pitem->Name[0]) && is_lfn) //是长名项，而且没有被删除,输入的名子为长名
	{
     have_lfn=1;
	 
	 plfni=(struct LFN_FDI *)pitem;

	 cur_binding_sumchk=(plfni->ChkVal)[0]; //获取长名项的绑定校验和
    
     result=Get_Part_Name(lfn_buf,plfni,(UINT8)((((plfni->AttrByte[0])&0XBF)-1)*13)); //将当前LFN项中的文件名UNICODE码拼入临时缓冲
                                                                      //此临时缓冲长度为MAX_LFN_LEN，如果越界，则
	                                                                  //不再装入，最终造成LFN的截断。
	 if(result) is_lfn_buf_err=1; //如果发生LFN_BUF溢出错误
	}
    #endif
   }
  }

  Cur_Clust=Get_Next_Cluster(Cur_Clust); //获取下一簇
 }while(!IS_END_CLU(Cur_Clust)); //如果不是最后一个簇，则继续循环

 return ERR_NO_DIR; 
}
#endif

/************************************************************************************
 功能：进入一个目录，实为获取目录的开始簇
 形参：dirpath:目录的路径 pCluster:指向用于记录簇号的变量的指针 pos:路径中文件名的位置
 返回：运行结果 成功或错误码
 详解：此函数虽然使用者可见，但用处不大。它返回目录的开始簇。dirpath可以带有文件名，如
       /a/b/c/d/e/test.txt 函数运行之后pos所指向的变量的值为test.txt在路径中的位置。主
       要是考虑到此函数与打开文件或目录的函数相配合，可以很方便的从路径中提取中文件名
*************************************************************************************/
#ifdef ZNFAT_ENTER_DIR
UINT8 znFAT_Enter_Dir(INT8 *dirpath,UINT32 *pCluster,UINT32 *pos) 
{
 UINT8 index=0,res=0;
 UINT32 i=1;

 #ifndef USE_LFN
 INT8 dirname[13];
 #else
 INT8 dirname[MAX_LFN_LEN+1];
 #endif

 *pos=1;
 *pCluster=2;

 if(('\\'==dirpath[0] || '/'==dirpath[0]) && '\0'==dirpath[1]) //如果是"\\"，则直接取首目录簇，即第2簇
 {
  return ERR_SUCC;
 }

 while('\0'!=dirpath[i])
 {
  if('\\'==dirpath[i] || '/'==dirpath[i])
  {
   dirname[index]='\0';
   index=0;
   
   res=Get_Dir_Start_Cluster(dirname,pCluster);
   if(res) 
   { 
	return res;  //返回错误码  
   }
   *pos=i+1;
  } 
  else
  {
   dirname[index]=dirpath[i];
   index++;
   #ifndef USE_LFN
   if(index>12) //如果不使用长名，则目录名以及文件名最长不能超过8+1+3
   {
	return ERR_SFN_ILL_LEN; //目录名长于8.3，亦防止dirname溢出
   }
   #else
   if(index>MAX_LFN_LEN) //如果使用长名，则目录名以及文件名最长不能超过设定的长名最长长度
   {
	return ERR_LFN_BUF_OUT; //目录名长于MAX_LFN_LEN，亦防止dirname溢出
   }   
   #endif
  }
  i++;
 }
   
 return ERR_SUCC; //成功
}
#endif

/************************************************************************************
 功能：由给定的短文件名及时间信息，构造出一个文件目录项
 形参：pfdi:指向用于装载文件目录项的变量的指针 pfn:指向文件名 pdt:指向时间信息 
       is_file:构造的文件目录项用于表达文件还是目录
 返回：0
 详解：在构造目录的文件目录项时，要事先为其分配好空簇，因此引入了is_file用以区分。
*************************************************************************************/
#ifdef FILL_FDI
UINT8 Fill_FDI(struct FDI *pfdi,INT8 *pfn,struct DateTime *pdt,UINT8 is_file)
{
 UINT8 dot_pos=0,have_dot=0,lowcase=0;
 UINT8 i=0,j=0,fn_len=(UINT8)StringLen(pfn);
 UINT16 time=0,date=0;

 Memory_Set(((UINT8 *)pfdi),FDI_NBYTES,0); //对文件目录项清0

 for(i=(UINT8)(fn_len-1);i>0;i--) //反向寻找. 第一个.是主文件与扩展名的分界
 {
  if('.'==pfn[i]) 
  {
   dot_pos=i;
   have_dot=1;
   break;
  }
 } 

 if(have_dot) //如果有点
 {
  //填入主文件名
  for(i=0;i<dot_pos;i++)
  {
   (pfdi->Name)[i]=(INT8)Lower2Up(pfn[i]); //转为大写
  }
  for(;i<8;i++)
  {
   (pfdi->Name)[i]=' '; //不足8字节部分填入空格
  }

  //填入扩展名
  for(i=(UINT8)(dot_pos+1);i<fn_len;i++)
  {
   (pfdi->Extension)[j]=(UINT8)Lower2Up(pfn[i]); //转为大写
   j++;
  }
  for(;j<3;j++)
  {
   (pfdi->Extension)[j]=' '; //不足8字节部分填入空格
  }
 }
 else //如果没有点
 {
  //填入主文件名
  for(i=0;i<fn_len;i++)
  {
   (pfdi->Name)[i]=(UINT8)Lower2Up(pfn[i]); //转为大写
  }
  for(;i<8;i++)
  {
   (pfdi->Name)[i]=' '; //不足8字节部分填入空格
  } 
  
  //填入扩展名
  for(j=0;j<3;j++)
  {
   (pfdi->Extension)[j]=' '; //扩展名填入空格
  }
 }
 //=======================填入主文件名与扩展名========

 pfdi->Attributes=(UINT8)(is_file?0X20:0X30); //设置属性
 //=======================填入属性========

 lowcase=Check_SFN_Illegal_Lower(pfn); //获取主文件名与扩展名的大小写状态
 if((lowcase&0X0F)==0X01) //如果主文件名为小写
 {
  pfdi->LowerCase|=0X08;
 }
 if((lowcase&0XF0)==0X10) //如果扩展名为小写
 {
  pfdi->LowerCase|=0X10;
 }
 //=======================填入大小写控制字============

 pfdi->CTime10ms=(UINT8)((((pdt->time).sec)%2)?0X78:0X00);
 //=======================填入创建时间10MS位==========

 time=(UINT16)(MAKE_TIME((pdt->time).hour,(pdt->time).min,(pdt->time).sec));
 (pfdi->CTime)[0]=(UINT8)time;
 (pfdi->CTime)[1]=(UINT8)(time>>8);
 //=======================填入创建时间================

 date=(UINT16)(MAKE_DATE((pdt->date).year,(pdt->date).month,(pdt->date).day));
 (pfdi->CDate)[0]=(UINT8)date;
 (pfdi->CDate)[1]=(UINT8)(date>>8);
 //=======================填入创建日期================

 (pfdi->ADate)[0]=(UINT8)date;
 (pfdi->ADate)[1]=(UINT8)(date>>8);
 //=======================填入访问日期================
 
 (pfdi->MTime)[0]=(UINT8)time;
 (pfdi->MTime)[1]=(UINT8)(time>>8);
 //=======================填入修改时间================ 

 (pfdi->MDate)[0]=(UINT8)date;
 (pfdi->MDate)[1]=(UINT8)(date>>8);
 //=======================填入修改日期================

 //填充开始簇
 if(!is_file) //如果是目录，则在fdi中填充空簇
 {
  pfdi->HighClust[0]=(UINT8)(((pInit_Args->Next_Free_Cluster)>>16)&0X000000FF);//目录与文件不同，目录在创建之初
  pfdi->HighClust[1]=(UINT8)(((pInit_Args->Next_Free_Cluster)>>24)&0X000000FF);//就要为其分配空簇，为的是写入.与..
  pfdi->LowClust [0]=(UINT8)(((pInit_Args->Next_Free_Cluster)    )&0X000000FF);//这两个特殊的文件目录项
  pfdi->LowClust [1]=(UINT8)(((pInit_Args->Next_Free_Cluster)>>8 )&0X000000FF);                                                                                                                                                                  
 }

 //文件大小字段暂置为0
 return 0;
}
#endif

/************************************************************************************
 功能：向构造好的文件目录项注册到目录簇中去
 形参：cluster:目录簇 pfdi:指向文件目录项的指针 psec:文件目录项被写到的扇区地址 
       pn:文件目录项在其扇区中的位置 
 返回：运行结果，成功或错误误
 详解：此函数是创建文件和目录的重要核心操作，它是针对短名的，与Register_LFN_FDI相对应
*************************************************************************************/
#ifdef REGISTER_FDI
UINT8 Register_FDI(UINT32 cluster,struct FDI *pfdi,UINT32 *psec,UINT8 *pn)
{
 UINT32 temp_sec=0,old_cluster=0;
 UINT8 iSec=0,iFDI=0;
 struct FDIesInSEC *pfdis;

 if(0==pInit_Args->Free_nCluster) return ERR_NO_SPACE; //如果没有空间，则直接返回
 
 //以下代码检查是否已经存在重复的文件目录项，以及查询空位
 do
 {
  temp_sec=SOC(cluster);
  for(iSec=0;iSec<(pInit_Args->SectorsPerClust);iSec++)
  {
   znFAT_Device_Read_Sector(temp_sec+(UINT32)iSec,znFAT_Buffer);
   pfdis=((struct FDIesInSEC *)znFAT_Buffer);
   for(iFDI=0;iFDI<NFDI_PER_SEC;iFDI++)
   {
    if(Memory_Compare((UINT8*)((pfdis->FDIes)+iFDI),(UINT8*)pfdi,11))  //比较文件名
    {
     *psec=temp_sec+(UINT32)iSec;
     *pn=iFDI; //记录重名文件目录项的位置，以便对其进行解析
     return ERR_FDI_ALREADY_EXISTING;
    }
    else
    {
	 if(0==((((pfdis->FDIes)[iFDI]).Name)[0]))
	 {
	  *psec=temp_sec+(UINT32)iSec;
	  *pn=iFDI; //记录空位的位置

      znFAT_Device_Read_Sector(*psec,znFAT_Buffer);
      Memory_Copy((UINT8*)((((struct FDIesInSEC *)znFAT_Buffer)->FDIes)+*pn),(UINT8 *)pfdi,FDI_NBYTES);
      znFAT_Device_Write_Sector(*psec,znFAT_Buffer);

	  return ERR_SUCC;
	 }
    }
   }
  }
  old_cluster=cluster;
  cluster=Get_Next_Cluster(cluster);
 }while(!IS_END_CLU(cluster)); //如果不是最后一个簇，则继续循环
 //===========================================================
 //如果运行到这里，则说明当前簇中已无空位
 if(0!=pInit_Args->Free_nCluster) //如果剩余空簇数为0，则说明磁盘已无空间
 {
  Modify_FAT(old_cluster,pInit_Args->Next_Free_Cluster);
  Modify_FAT(pInit_Args->Next_Free_Cluster,0X0FFFFFFF); //构造FAT簇链
  Clear_Cluster(pInit_Args->Next_Free_Cluster); //清空空闲簇

  *psec=SOC(pInit_Args->Next_Free_Cluster);
  *pn=0; //记录空位的位置

  znFAT_Device_Read_Sector(*psec,znFAT_Buffer);
  Memory_Copy((UINT8*)((((struct FDIesInSEC *)znFAT_Buffer)->FDIes)),(UINT8 *)pfdi,FDI_NBYTES);
  znFAT_Device_Write_Sector(*psec,znFAT_Buffer);

  Update_Next_Free_Cluster();

  return ERR_SUCC;  
 }
 else
 {
  return ERR_NO_SPACE;
 } 
}
#endif

/************************************************************************************
 功能：文件创建
 形参：pfi:指向文件信息集合指针 pfn:指向文件路径 pdt:指向时间信息
 返回：运行结果 成功或错误码
 详解：此函数可以支持短名与长名，无限深层目录。文件创建完成之后，新建文件的信息将被
       装入文件信息集合，随即可以对此文件进行操作，而无需打次调open_file打开它。如果
       要创建的文件已经有重名，则返回重名错误，并将已经存在的文件的信息装入文件信息
       集合。因此它某些情况下也可以用来充当open_file函数：没有重名文件就创建一个新的，
       如果有重名文件则打开。
*************************************************************************************/
#ifdef ZNFAT_CREATE_FILE
UINT8 znFAT_Create_File(struct FileInfo *pfi,INT8 *pfn,struct DateTime *pdt)
{
 UINT32 Cur_Cluster=0,pos=0,sec=0;
 UINT8 res=0,n=0;
 struct FDI fdi;
 INT8 *filename;

 #ifdef USE_LFN
 UINT8 is_lfn=0;
 INT8 temp_filename[13];
 #endif

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=0;
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 Memory_Set((UINT8 *)pcccb_buf,sizeof(UINT32)*CCCB_LEN,0);
 #endif
 #endif

 #ifdef USE_EXCHANGE_BUFFER
 #ifdef USE_ALONE_EXB
 //Memory_Set(just_file->exb_buf,512,0);
 just_file->exb_cursec=0;
 #endif
 #endif

 res=znFAT_Enter_Dir(pfn,&Cur_Cluster,&pos); //进入目录
 if(res)
 {
  return res;
 }

 filename=pfn+pos;

 if(Check_Illegal_Char(filename)) return ERR_ILL_CHAR; //检查文件名中是否有非法字符
 
 #ifdef USE_LFN
 if(!Is_LFN(filename))
 {
 #endif
  //检查短文件名合法性，若非法则直接返回，不再进行后面的处理(此处对SFN的合法性检查非常严格)
  //事先检查SFN的合法性，减少后面处理上的麻烦
  if(Check_SFN_Illegal_Length(filename)) return ERR_SFN_ILL_LEN; //检查SFN是否符合8.3长度
  if(Check_SFN_Dot(filename)) return ERR_SFN_DOT; //检查SFN中.是否合法 
  if(Check_SFN_Special_Char(filename)) return ERR_SFN_SPEC_CHAR; //检查SFN中是否有特殊字符
  if(((UINT8)(-1))==Check_SFN_Illegal_Lower(filename)) return ERR_SFN_ILL_LOWER; //检查SFN中是否有非法的大小写
 #ifdef USE_LFN
 }
 else
 {
  is_lfn=1;
  res=oemstr2unistr(filename,pfi->longname); //如果是长名，则将filename转为UNI串
  if(res) return res;
 }

 if(!is_lfn) //如果不是长名
 {
 #endif
  Fill_FDI(&fdi,filename,pdt,BOOL_TRUE); //构造文件目录项
  res=Register_FDI(Cur_Cluster,&fdi,&sec,&n);//在当前簇中进行文件目录项的"注册"

 #ifdef USE_LFN
 }
 else //如果是长名
 {
  Make_Short_Name(filename,temp_filename);
  Fill_FDI(&fdi,temp_filename,pdt,BOOL_TRUE); //构造文件目录项
  res=Register_LFN_FDI(Cur_Cluster,&fdi,(pfi->longname),&sec,&n); //在当前簇中进行长名项及相应短名项“注册”
 }
 #endif
 
 if(!res)
 {
  //将新建文件的信息装入文件信息集合
  #ifdef USE_LFN
  if(is_lfn)
  {
   StringCopy(pfi->File_Name,temp_filename);
   pfi->have_lfn=1;
  }
  else
  #endif
  {
   StringCopy(pfi->File_Name,filename);
  }
  pfi->File_Attr=0X20;
  (pfi->File_CTime).hour=(pdt->time).hour;
  (pfi->File_CTime).min=(pdt->time).min;
  (pfi->File_CTime).sec=(pdt->time).sec;
  (pfi->File_CDate).year=(pdt->date).year;
  (pfi->File_CDate).month=(pdt->date).month;
  (pfi->File_CDate).day=(pdt->date).day;
  //(pfi->File_ADate).year=(pdt->date).year;
  //(pfi->File_ADate).month=(pdt->date).month;
  //(pfi->File_ADate).day=(pdt->date).day;
  //(pfi->File_MTime).hour=(pdt->time).hour;
  //(pfi->File_MTime).min=(pdt->time).min;
  //(pfi->File_MTime).sec=(pdt->time).sec;
  //(pfi->File_MDate).year=(pdt->date).year;
  //(pfi->File_MDate).month=(pdt->date).month;
  //(pfi->File_MDate).day=(pdt->date).day;

  pfi->File_StartClust=0;
  pfi->File_Size=0;

  pfi->File_CurClust=0;
  pfi->File_CurSec=0;
  pfi->File_CurPos=0;

  pfi->File_CurOffset=0;
  pfi->File_IsEOF=BOOL_TRUE;

  pfi->FDI_Sec=sec;
  pfi->nFDI=n;

  return ERR_SUCC;
 }
 else
 {
  if(res==ERR_FDI_ALREADY_EXISTING) //如果文件已经存在，则直接解析它
  {
   znFAT_Device_Read_Sector(sec,znFAT_Buffer); //如果重名文件目录项所在的扇区
   Analyse_FDI(pfi,(((struct FDIesInSEC *)znFAT_Buffer)->FDIes)+n); //解析匹配的文件目录项
   pfi->FDI_Sec=sec;
   pfi->nFDI=n;
   
   #ifdef USE_LFN
   if(is_lfn)
   {
    pfi->have_lfn=1;
   }
   else
   {
    pfi->have_lfn=0;
   }
   #endif 
  }
  return res;
 }
}
#endif

/************************************************************************************
 功能：在一个目录中创建一个目录
 形参：cluster:目录簇 pdn:指向目录名 pdt:指向时间信息
 返回：运行结果 成功或错误码
 详解：支持长目录名创建 cluster最终会返回新创建的目录的开始簇
       此函数是znFAT实现多级目录创建的重要函数
*************************************************************************************/
#ifdef CREATE_DIR_IN_CLUSTER
UINT8 Create_Dir_In_Cluster(UINT32 *cluster,INT8 *pdn,struct DateTime *pdt)
{
 UINT8 res=0,i=0;
 UINT32 dummy=0;
 struct FDI fdi;

 #ifdef USE_LFN
 UINT8 is_lfn=0; //标记输入的目录名是否长名
 UINT16 temp_lfn_buf[MAX_LFN_LEN+1]; //用于装载长名UNI串的临时缓冲
 INT8 temp_dirname[13];
 #endif

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=0;
 #endif

 if(Check_Illegal_Char(pdn)) return ERR_ILL_CHAR; //检查目录名中是否有非法字符
 
 #ifdef USE_LFN
 if(!Is_LFN(pdn))
 {
 #endif
  //检查短文件名合法性，若非法则直接返回，不再进行后面的处理(此处对SFN的合法性检查非常严格)
  //事先检查SFN的合法性，减少后面处理上的麻烦
  if(Check_SFN_Illegal_Length(pdn)) return ERR_SFN_ILL_LEN; //检查SFN是否符合8.3长度
  if(Check_SFN_Dot(pdn)) return ERR_SFN_DOT; //检查SFN中.是否合法 
  if(Check_SFN_Special_Char(pdn)) return ERR_SFN_SPEC_CHAR; //检查SFN中是否有特殊字符
  if(((UINT8)(-1))==Check_SFN_Illegal_Lower(pdn)) return ERR_SFN_ILL_LOWER; //检查SFN中是否有非法的大小写
 #ifdef USE_LFN
 }
 else
 {
  is_lfn=1;
  res=oemstr2unistr(pdn,temp_lfn_buf); //如果是长名，则将pdn转为UNI串
  if(res) return res;
 }

 if(!is_lfn) //如果不是长名
 {
 #endif
  Fill_FDI(&fdi,pdn,pdt,BOOL_FALSE); //构造目录项
  res=Register_FDI(*cluster,&fdi,&dummy,&i);//在当前簇中进行文件目录项的"注册"

 #ifdef USE_LFN
 }
 else //如果是长名
 {
  Make_Short_Name(pdn,temp_dirname);
  Fill_FDI(&fdi,temp_dirname,pdt,BOOL_FALSE); //构造文件目录项
  res=Register_LFN_FDI(*cluster,&fdi,temp_lfn_buf,&dummy,&i); //在当前簇中进行长名项及相应短名项“注册”
 }
 #endif

 if(res)
 {
  return res;
 }

 //====================================================================================

 //向目录簇中写入.与..
 Modify_FAT(pInit_Args->Next_Free_Cluster,0X0FFFFFFF); //构造FAT簇链
 Clear_Cluster(pInit_Args->Next_Free_Cluster); //清空空闲簇 

 //把fdi中的名字替换为. 名为.的目录指向了相前簇
 fdi.Name[0]='.';
 for(i=1;i<11;i++)
 {
  fdi.Name[i]=' '; //.后面全部填充空格
 }

 Memory_Copy(znFAT_Buffer,((UINT8 *)(&fdi)),FDI_NBYTES); //将文件目录项.装入到内部缓冲区中

 //把fdi中的名字替换为.. 名为..的目录指向了上一簇(DOS中使用CD..可以回到上一级目录就是这个道理)
 fdi.Name[1]='.';
 
 //把fdi中的簇值改为上一级目录的首簇 
 if(2!=(*cluster)) //如果上一级目录首簇不是"根目录"(首目录)
 {
  fdi.HighClust[0]=(UINT8)(((*cluster)>>16)&0X000000FF);
  fdi.HighClust[1]=(UINT8)(((*cluster)>>24)&0X000000FF);
  fdi.LowClust [0]=(UINT8)(((*cluster)    )&0X000000FF);
  fdi.LowClust [1]=(UINT8)(((*cluster)>>8 )&0X000000FF); 
 }
 else
 {
  fdi.HighClust[0]=fdi.HighClust[1]=fdi.LowClust[0]=fdi.LowClust[1]=0;
 }

 Memory_Copy(znFAT_Buffer+FDI_NBYTES,((UINT8 *)(&fdi)),FDI_NBYTES); //将文件目录项..装入到内部缓冲区中

 znFAT_Device_Write_Sector(SOC(pInit_Args->Next_Free_Cluster),znFAT_Buffer);

 *cluster=(pInit_Args->Next_Free_Cluster); //返回新创建的目录首簇

 Update_Next_Free_Cluster();

 return ERR_SUCC;
}
#endif

/************************************************************************************
 功能：创建目录
 形参：pdp:指向目录路径 pdt:指向时间信息 
 返回：运行结果  成功或错误码
 详解：支持无限深目录创建 如 目录路径可以是 /a/b/c/d/e/f/g/h/ 注意最终一定要以/或"\\"
       结束。否则znFAT会认为最后一级目录名是文件名，而不予以创建。支持长目录名
*************************************************************************************/
#ifdef ZNFAT_CREATE_DIR
UINT8 znFAT_Create_Dir(INT8 *pdp,struct DateTime *pdt)
{
 UINT32 Cur_Cluster=0,i=0;
 UINT8 index=0,res=0;

 #ifndef USE_LFN
 UINT8 dirname[13];
 #else
 UINT8 dirname[MAX_LFN_LEN+1];
 #endif

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=0;
 #endif

 if(znFAT_Enter_Dir(pdp,&Cur_Cluster,&i)) //进入目录，如果中途发生错误
 {                                        //比如某一级目录名不合法，或
  while('\0'!=pdp[i])                     //目录不存在，则将最近的一层
  {                                       //目录的首簇记录在Cur_Cluster里
   if('\\'==pdp[i] || '/'==pdp[i])                       //而将失败的那一级目录名的第一
   {                                      //字符的位置记录在i中
    dirname[index]='\0';                  //比如\a1\b2\c3\e4\f5\，目录c3不存在
    index=0;                              //则Cur_Cluster为目录b2的首簇，i记录的是'c'的位置即7
   
    res=Create_Dir_In_Cluster(&Cur_Cluster,(INT8 *)dirname,pdt); //在簇Cur_Cluster中创建目录
    if(res) 
    { 
	 return res;  //返回错误码  
    }
   }
   else
   {
    dirname[index]=pdp[i];
    index++;
    #ifndef USE_LFN
    if(index>12) //如果不使用长名，则目录名以及文件名最长不能超过8+1+3
    {
	 return ERR_SFN_ILL_LEN; //目录名长于8.3，亦防止dirname溢出
    }
    #else
    if(index>MAX_LFN_LEN) //如果使用长名，则目录名以及文件名最长不能超过设定的长名最长长度
    {
	 return ERR_LFN_BUF_OUT; //目录名长于MAX_LFN_LEN，亦防止dirname溢出
    }   
    #endif
   }
   i++;
  }
  
  return ERR_SUCC; //成功  
 }
 else
 {
  return ERR_DIR_ALREADY_EXISTING; //要创建的目录已经存在了
 }
}
#endif

//========================以下代码尝试实现目录删除功能=====================

/************************************************************************************
 功能：销毁一个FAT簇链
 形参：cluster:簇链的开始簇号
 返回：0
 详解：振南原来使用Modify_FAT+循环来实现此函数，效率极低，速度很慢，现在进行了改进，
       效率较高。
*************************************************************************************/
#ifdef DESTROY_FAT_CHAIN
UINT8 Destroy_FAT_Chain(UINT32 cluster)
{
 UINT32 clu_sec=0,temp1=0,temp2=0,old_clu=0,nclu=1;

 struct FAT_Sec *pFAT_Sec;

 if(cluster<(pInit_Args->Next_Free_Cluster)) //如果要销毁的簇链头簇比下一空簇值小，则将下一空簇值置为它
 {
  pInit_Args->Next_Free_Cluster=cluster;
 }

 old_clu=cluster;

 znFAT_Device_Read_Sector((old_clu/NITEMSINFATSEC)+(pInit_Args->FirstFATSector),znFAT_Buffer);

 pFAT_Sec=(struct FAT_Sec *)znFAT_Buffer;

 cluster=Bytes2Value(((pFAT_Sec->items)[cluster%NITEMSINFATSEC]).Item,4);

 while(!IS_END_CLU(cluster))
 {
  nclu++;

  clu_sec=cluster/NITEMSINFATSEC;

  temp1=old_clu%NITEMSINFATSEC;
  temp2=old_clu/NITEMSINFATSEC;

  ((pFAT_Sec->items)[temp1]).Item[0]=0;
  ((pFAT_Sec->items)[temp1]).Item[1]=0;
  ((pFAT_Sec->items)[temp1]).Item[2]=0;
  ((pFAT_Sec->items)[temp1]).Item[3]=0;

  if(temp2!=clu_sec)
  {     
   znFAT_Device_Write_Sector(temp2+(pInit_Args->FirstFATSector),znFAT_Buffer);
   znFAT_Device_Write_Sector(temp2+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);

   znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
  }

  old_clu=cluster;
  cluster=Bytes2Value(((pFAT_Sec->items)[cluster%NITEMSINFATSEC]).Item,4);
 }

 temp1=old_clu%NITEMSINFATSEC;
 temp2=old_clu/NITEMSINFATSEC;

 ((pFAT_Sec->items)[temp1]).Item[0]=0;
 ((pFAT_Sec->items)[temp1]).Item[1]=0;
 ((pFAT_Sec->items)[temp1]).Item[2]=0;
 ((pFAT_Sec->items)[temp1]).Item[3]=0;

 znFAT_Device_Write_Sector(temp2+(pInit_Args->FirstFATSector),znFAT_Buffer);
 znFAT_Device_Write_Sector(temp2+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);

 pInit_Args->Free_nCluster+=nclu; //更新剩余空簇数

 return ERR_SUCC;
}
#endif

/************************************************************************************
 功能：销毁一个文件目录项所对应的整条簇链
 形参：pitem:指向文件目录项的指针
 返回：0
 详解：主要是用于文件和目录的删除
*************************************************************************************/
#ifdef DESTROY_FDI
UINT8 Destroy_FDI(struct FDI *pitem)
{
 UINT32 start_cluster=Bytes2Value(pitem->LowClust,2)+Bytes2Value(pitem->HighClust,2)*65536;

 if(0==start_cluster) return ERR_SUCC;

 Destroy_FAT_Chain(start_cluster); //销毁簇链

 return ERR_SUCC;
} 
#endif

/************************************************************************************
 功能：对某个目录簇进行搜索，看其中是否有目录项，即该目录下有子目录，搜索过程中遇到的
       文件项将全部销毁。
 形参：cluster:要进行搜索的目录开始簇号 
       for_del_dir:在遇到目录项之后，是返回其开始簇，还是将其直接销毁
 返回：运行结果 找到子目录或未找到 注：如果在目录中未找到子目录，则此目录中的所有文件
       将被全部销毁，在这种情况下，这个目录随即也会被销毁，然后进行目录回溯.....
 详解：目录删除的相关算法均较为复杂，但如果领悟，则会觉得比较简单。目录因为有子目录
       而且是一种树形的结构，因此删除目录的过程其实是一个“递归回溯”的过程，直到回到
       顶层目录。
*************************************************************************************/
#ifdef HAVE_ANY_SUBDIR_WITH_DEL_FOREFILE
UINT8 Have_Any_SubDir_With_Del_ForeFile(UINT32 *cluster,UINT8 for_del_dir)
{
 UINT8 iSec=0,iFDI=0;
 UINT32 sec_temp=0;
 UINT32 temp=*cluster;

 struct FDIesInSEC *pitems; //指向文件目录项扇区数据的指针
 struct FDI *pitem; //指向文件目录项数据的指针

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=0;
 #endif

 do
 {
  sec_temp=SOC(temp); //当前簇首扇区
  for(iSec=0;iSec<(pInit_Args->SectorsPerClust);iSec++) 
  {
   znFAT_Device_Read_Sector(sec_temp+(UINT32)iSec,znFAT_Buffer);
   pitems=(struct FDIesInSEC *)znFAT_Buffer; 

   for(iFDI=0;iFDI<NFDI_PER_SEC;iFDI++) //访问扇区中各文件目录项
   {
    pitem=&(pitems->FDIes[iFDI]); //指向一个文件目录项数据

	if((CHK_ATTR_FILE(pitem->Attributes)) && (0XE5!=pitem->Name[0])) //文件属性为文件，且没有被删除
    {
     Destroy_FDI(pitem); //销毁文件目录项及其对应的簇链 注：内部缓冲区数据被变更
	 znFAT_Device_Read_Sector(sec_temp+(UINT32)iSec,znFAT_Buffer); //重新读取扇区数据，恢复内部缓冲区的数据

	 pitem->Name[0]=0XE5; //给文件目录项打上"已删除"的标记
     pitem->HighClust[0]=pitem->HighClust[1]=0; //开始簇的高字清0

     znFAT_Device_Write_Sector(sec_temp+(UINT32)iSec,znFAT_Buffer); //如果有销毁操作，则回写扇区
	}

    if((CHK_ATTR_DIR(pitem->Attributes)) && (0XE5!=pitem->Name[0]) //文件属性为目录，且没有被删除
	   && ('.'!=pitem->Name[0]))                          //不是.与..
    { 
	 if(!for_del_dir) //不是为了把子目录本身删除，而是为了获取其首簇，进而进入更深子目录
	 {
	  *cluster=Bytes2Value(pitem->LowClust,2)+Bytes2Value(pitem->HighClust,2)*65536;
	 }
     else
	 {
      Destroy_FDI(pitem); //销毁文件目录项及其对应的簇链 注：内部缓冲区数据被变更
	  znFAT_Device_Read_Sector(sec_temp+(UINT32)iSec,znFAT_Buffer); //重新读取扇区数据，恢复内部缓冲区的数据

	  pitem->Name[0]=0XE5; //给文件目录项打上"已删除"的标记
      pitem->HighClust[0]=pitem->HighClust[1]=0; //开始簇的高字清0

      znFAT_Device_Write_Sector(sec_temp+(UINT32)iSec,znFAT_Buffer); //如果有销毁操作，则回写扇区	
	 }
     return BOOL_TRUE;
    }
   }
  }

  temp=Get_Next_Cluster(temp); //获取下一簇
 }while(!IS_END_CLU(temp)); //如果不是最后一个簇，则继续循环
  
 return BOOL_FALSE;
}
#endif

/************************************************************************************
 功能：进入从某个目录开始的最深最“左”的目录
 形参：cluster:指向目录簇的指针
 返回：0
 详解：此函数是目录删除功能的一个基本而关键的操作。它用来进入到“最深最左”的目录，
       同时在此过程中，删除“最左”目录“左边”的所有文件。（此函数的思想较为抽象）
*************************************************************************************/
#ifdef ENTER_DEEP_AHEAD_DIR
UINT8 Enter_Deep_Ahead_Dir(UINT32 *cluster)
{
 UINT32 dir_cluster=*cluster; 

 while(Have_Any_SubDir_With_Del_ForeFile(&dir_cluster,BOOL_FALSE));

 *cluster=dir_cluster;

 return ERR_SUCC;
}
#endif

/************************************************************************************
 功能：获取一个目录的上一级目录开始簇
 形参：cluster:目录的开始簇，同时又用于接收计算得到的上一级目录的开始簇
 返回：运行结果 成功或错误码 
 详解：在目录的最头上，有.与..，在..中记录了上一层目录的开始簇，此函数就是基于这一点
       来获取目录上一级目录开始簇的。
*************************************************************************************/
#ifdef GET_UPPER_DIR
UINT8 Get_Upper_Dir(UINT32 *cluster)
{
 struct FDIesInSEC *pitems; //指向文件目录项扇区数据的指针
 struct FDI *pitem; //指向文件目录项数据的指针

 znFAT_Device_Read_Sector(SOC(*cluster),znFAT_Buffer);
 pitems=(struct FDIesInSEC *)znFAT_Buffer; 
   
 pitem=&(pitems->FDIes[1]); //指向..文件目录项，用以获取上一级目录的首簇

 if('.'==pitem->Name[0] && '.'==pitem->Name[1])
  *cluster=Bytes2Value(pitem->LowClust,2)+Bytes2Value(pitem->HighClust,2)*65536;
 else
  return ERR_FS_DIR; //如果在目录的最头上不是.与..，则说明文件系统已被损坏
 
 if(0==(*cluster)) (*cluster)=2; //如果上一级目录为根目录(首目录)，将簇值置为2

 return ERR_SUCC;
}
#endif

/************************************************************************************
 功能：删除目录
 形参：dirpath:指针向目录路径 目录名支持通配
 返回：运行结果 成功或错误码
 详解：dirpath如\a\b\c，删除目录c，最后不要以\结束。删除目录将会删除目录下的所有子目录
       与文件，子目录下可以再有子目录与文件，依次递归。dirpath也可以为/a/b/c* ，它将
       删除b目录下所有以c打头的目录。
*************************************************************************************/
#ifdef ZNFAT_DELETE_DIR
UINT8 znFAT_Delete_Dir(INT8 *dirpath) 
{
 UINT32 top_dir_first_cluster=0,sub_dir_first_cluster=0;
 UINT8 res=0;

 struct FDIesInSEC *pitems; //指向文件目录项扇区数据的指针

 struct FileInfo fi;

 res=znFAT_Open_File(&fi,dirpath,0,BOOL_FALSE); //尝试打开目录

 if(res) return res; //如果打开目录失败，则直接返回错误码

 while(!res) //目录打开成功
 {
  top_dir_first_cluster=fi.File_StartClust; //顶层目录的首簇
  sub_dir_first_cluster=top_dir_first_cluster;

  //以下代码将顶级目录下所有内容(文件、子目录及子目录中的内容，含递归)销毁

  Enter_Deep_Ahead_Dir(&sub_dir_first_cluster); //获取最深最靠前的目录首簇

  while(sub_dir_first_cluster!=top_dir_first_cluster) //如果最深最靠前目录首簇就是要删除的顶级目录
  {                                                   //则说明顶级目录下的所有内容都已经清空
   Get_Upper_Dir(&sub_dir_first_cluster); //获取上一层目录首簇 
   
   Have_Any_SubDir_With_Del_ForeFile(&sub_dir_first_cluster,BOOL_TRUE); //把已清空其内容的子目录销毁 

   Enter_Deep_Ahead_Dir(&sub_dir_first_cluster); //获取最深最靠前的目录首簇
  }  

  //销毁顶级目录对应的文件目录项及其簇链

  znFAT_Device_Read_Sector(fi.FDI_Sec,znFAT_Buffer); //读取文件目录项所在的扇区
  pitems=(struct FDIesInSEC *)znFAT_Buffer;

  Destroy_FDI((pitems->FDIes)+fi.nFDI); //销毁顶级目录

  znFAT_Device_Read_Sector(fi.FDI_Sec,znFAT_Buffer); //读取文件目录项所在的扇区
  (pitems->FDIes)[fi.nFDI].Name[0]=0XE5;
  (pitems->FDIes)[fi.nFDI].HighClust[0]=(pitems->FDIes)[fi.nFDI].HighClust[1]=0;

  znFAT_Device_Write_Sector(fi.FDI_Sec,znFAT_Buffer); //回写扇区

  res=znFAT_Open_File(&fi,dirpath,0,BOOL_FALSE); //尝试打开目录
 }
 
 znFAT_Close_File(&fi);

 #ifdef RT_UPDATE_FSINFO
 Update_FSINFO();
 #endif

 return ERR_SUCC; 
}
#endif

//========================以上代码用于实现目录删除功能====================

/************************************************************************************
 功能：文件删除
 形参：filepath:文件路径 
 返回：运行结果 成功或错误码
 详解：删除文件比删除目录要简单的多。filepath如\a\b\test.txt，删除文件test.txt
       运行通配，filepath可以为/a/b/c/d/t*.txt 删除d目录下所有的t打头的txt文件
*************************************************************************************/
#ifdef ZNFAT_DELETE_FILE
UINT8 znFAT_Delete_File(INT8 *filepath) 
{
 UINT8 res=0;
 struct FileInfo fi; 

 struct FDIesInSEC *pitems; //指向文件目录项扇区数据的指针
 struct FDI *pitem; //指向文件目录项数据的指针

 res=znFAT_Open_File(&fi,filepath,0,BOOL_TRUE);
 if(res) return res;

 while(!res) //打开文件成功
 {
  znFAT_Device_Read_Sector(fi.FDI_Sec,znFAT_Buffer); //读取文件的文件目录项所在扇区
  pitems=(struct FDIesInSEC *)znFAT_Buffer;
  pitem=(pitems->FDIes)+fi.nFDI;

  if(0!=fi.File_StartClust) Destroy_FAT_Chain(fi.File_StartClust); //销毁整条簇链

  znFAT_Device_Read_Sector(fi.FDI_Sec,znFAT_Buffer); //读取文件目录项所在的扇区

  pitem->Name[0]=0XE5; //给文件目录项打上"已删除"的标记
  pitem->HighClust[0]=pitem->HighClust[1]=0; //开始簇的高字清0

  znFAT_Device_Write_Sector(fi.FDI_Sec,znFAT_Buffer); //回写扇区

  res=znFAT_Open_File(&fi,filepath,0,BOOL_TRUE);
 }

 znFAT_Close_File(&fi);

 #ifdef RT_UPDATE_FSINFO
 Update_FSINFO();
 #endif

 return ERR_SUCC; 
}
#endif

//==========================以下代码用于实现格式化功能============================

/************************************************************************************
 功能：获取在某种磁盘容量下簇大小的推荐值
 形参：nsec:总扇区数
 返回：0
 详解：如果返回0，则说明磁盘容量对于FAT32文件系统来说太小了，无法用FAT32来格式化。
*************************************************************************************/
#ifdef GET_RECMD_SZCLU
UINT8 Get_Recmd_szClu(UINT32 nsec)
{
 if(nsec<(14336)) return 0;

 if((nsec>=(14336)) && (nsec<=(32767))) return 0;
 if((nsec>=(32768)) && (nsec<=(65535))) return 1;
 if((nsec>=(65536)) && (nsec<=(131071))) return 1;
 if((nsec>=(131072)) && (nsec<=(262143))) return 2;
 if((nsec>=(262144)) && (nsec<=(524287))) return 4;
 if((nsec>=(524288)) && (nsec<=(16777215))) return 8;
 if((nsec>=(16777216)) && (nsec<=(33554431))) return 16;
 if((nsec>=(33554432)) && (nsec<=(67108863))) return 32;
 if((nsec>=(67108864)) && (nsec<=(4294967295UL))) return 64;

 return 0;
}
#endif

/************************************************************************************
 功能：在磁盘上创建一个FAT32的文件系统，即格式化
 形参：tt_sec:总扇区数 clu_sz:使用者指定的簇大小，如果为0，则取推荐值
 返回：0
 详解：FAT32的格式化分为两种策略：FDISK与SFD。FDISK是支持多分区的，因此需要构造MBR
       构造MBR会涉及到一些比较底层的内容，较有难度，因此为了简单而高效，znFAT中使用了
       SFD策略，它将整个磁盘就当作一个默认的大分区，因此它没有MBR。格式化后磁盘的卷
       标为ZN'ZNFATOK!
*************************************************************************************/
#ifdef ZNFAT_MAKE_FS
UINT8 znFAT_Make_FS(UINT32 tt_sec,UINT16 clu_sz) //格式化 tt_sec 总扇区数 clu_sz 指定的簇大小
{                                          //若为0则按磁盘大小选定默认值 格式化策略采用比较简单的SFD策略
	                                       //无MBR，不支持分区，直接从DBR开始
 struct DBR      *pdbr;
 struct FSInfo   *pfsinfo;

 UINT32 temp=0,temp1=0,temp2=0;

 tt_sec/=(UINT32)(NSECPERCYLINDER); 
 tt_sec*=(UINT32)(NSECPERCYLINDER);//舍去“剩余扇区”，剩余扇区是指不足一个柱面的扇区数

 //=================合成DBR扇区数据=============================================================
 PGM2RAM(znFAT_Buffer,_dbr,512); //从模版数组中把数据搬到内部缓冲区
 pdbr=(struct DBR *)znFAT_Buffer;

 pdbr->BPB_SecPerClus=(UINT8)(clu_sz/512); //每簇扇区数
 if(0==pdbr->BPB_SecPerClus) pdbr->BPB_SecPerClus=Get_Recmd_szClu(tt_sec);
 if(0==pdbr->BPB_SecPerClus) return ERR_FMT_TOO_LOW_VOLUME; //容量太小，不能用FAT32进行格式化

 temp1=pdbr->BPB_SecPerClus;

 pdbr->BPB_TotSec32[0]=(UINT8)((tt_sec)    &0X000000FF);
 pdbr->BPB_TotSec32[1]=(UINT8)((tt_sec>>8) &0X000000FF);
 pdbr->BPB_TotSec32[2]=(UINT8)((tt_sec>>16)&0X000000FF);
 pdbr->BPB_TotSec32[3]=(UINT8)((tt_sec>>24)&0X000000FF); //该分区的总扇区数

 temp=(tt_sec-32)/(((UINT32)NITEMSINFATSEC)*((UINT32)(pdbr->BPB_SecPerClus)));
 if((tt_sec-32)%((UINT32)NITEMSINFATSEC)*((UINT32)pdbr->BPB_SecPerClus)) temp++; //((tt_sec-32)-2*FATsz)/(SecPerClus*128)=FATsz 解这个等式
 temp2=temp;
 
 pdbr->BPB_FATSz32[0]=(UINT8)((temp)    &0X000000FF);
 pdbr->BPB_FATSz32[1]=(UINT8)((temp>>8) &0X000000FF);
 pdbr->BPB_FATSz32[2]=(UINT8)((temp>>16)&0X000000FF);
 pdbr->BPB_FATSz32[3]=(UINT8)((temp>>24)&0X000000FF); //FAT表的扇区数

 znFAT_Device_Write_Sector(0,znFAT_Buffer); //将合成好的DBR数据写入到0扇区中去

 //===============================以上代码完成对DBR扇区数据的合成===============================

 //===================================以下代码完成对FSINFO扇区数据的合成========================

 Memory_Set(znFAT_Buffer,ZNFAT_BUF_SIZE,0); //将内部缓冲区清0
 PGM2RAM(znFAT_Buffer,_fsinfo_1,4); //将FSINFO模板数据的第一部分搬过来
 PGM2RAM(znFAT_Buffer+484,_fsinfo_2,28); //将FSINFO模板数据的第二部分搬过来
                                             //注：FSINFO模板数据分为两部分，主要是因为其中有绝大
                                             //部分是0，为了节省固化数据量，减少FLASHROM空间的使用量
 pfsinfo=(struct FSInfo *)znFAT_Buffer;
 
 temp=(tt_sec-32-2*temp)/((UINT32)(temp1))-1; //总簇数-1，因为第2簇为首目录，已经被卷标占用
 pfsinfo->Free_Cluster[0]=(UINT8)((temp)    &0X000000FF);
 pfsinfo->Free_Cluster[1]=(UINT8)((temp>>8) &0X000000FF);
 pfsinfo->Free_Cluster[2]=(UINT8)((temp>>16)&0X000000FF);
 pfsinfo->Free_Cluster[3]=(UINT8)((temp>>24)&0X000000FF); //剩余空簇数

 znFAT_Device_Write_Sector(1,znFAT_Buffer); //将合成好的DBR数据写入到64扇区中去，即DBR扇区的后一个扇区

 //=====================================以上代码完成对FSINFO扇区数据的合成=====================

 //=====================================以下代码完成对FAT表的初始化============================

 znFAT_Device_Clear_nSector(temp1,32+2*temp2);
 znFAT_Device_Clear_nSector(temp2-1,33);
 znFAT_Device_Clear_nSector(temp2-1,33+temp2);

 PGM2RAM(znFAT_Buffer,_fatsec,12); //将FAT表模版数据搬到内部缓冲区
 znFAT_Device_Write_Sector(32,znFAT_Buffer); //向FAT表1中写入0
 znFAT_Device_Write_Sector(32+temp2,znFAT_Buffer); //向FAT表2中写入0

 //=====================================以上代码完成对FAT表的初始化============================

 //=====================================以下代码对数据区首扇区进行初始化，写入卷标=============

 PGM2RAM(znFAT_Buffer,_1stsec,26); 
 znFAT_Device_Write_Sector(32+2*temp2,znFAT_Buffer); //向数据区第一个扇区写入数据

 //=====================================以上.....==============================================
 return ERR_SUCC;
}  
#endif

//===================以下代码用于实现文件数据写入=====================

/************************************************************************************
 功能：更新文件的大小，将当前文件信息集合中的File_Size值写入到文件目录项中去
 形参：pfi:指针文件目录项的指针
 返回：0
 详解：在向文件写入数据，或删除数据之后，如果不调用此函数将文件大小更新到目录项中去
       则我们在电脑上是看不到写入的数据的。此函数在znFAT中受到RT_UPDATE_FILESIZE这个
       宏的控制，以决定是否实时更新文件大小。
*************************************************************************************/
#ifdef UPDATE_FILE_SIZE
UINT8 Update_File_Size(struct FileInfo *pfi) //更新文件大小
{
 struct FDI *pfdi;

 just_file=pfi;

 znFAT_Device_Read_Sector(pfi->FDI_Sec,znFAT_Buffer);

 pfdi=(((struct FDIesInSEC *)znFAT_Buffer)->FDIes)+(pfi->nFDI); //文件的文件目录项

 (pfdi->FileSize)[0]=(UINT8)((pfi->File_Size)&0X000000FF)      ;
 (pfdi->FileSize)[1]=(UINT8)(((pfi->File_Size)&0X0000FF00)>>8) ;
 (pfdi->FileSize)[2]=(UINT8)(((pfi->File_Size)&0X00FF0000)>>16);
 (pfdi->FileSize)[3]=(UINT8)(((pfi->File_Size)&0XFF000000)>>24);

 znFAT_Device_Write_Sector(pfi->FDI_Sec,znFAT_Buffer);

 return 0;
}
#endif

/************************************************************************************
 功能：更新文件的开始簇
 形参：pfi:指向文件信息集合的指针 clu:文件开始簇号
 返回：0
 详解：对于一个刚刚创建的文件（空文件），它的开始簇为0，因此向其写入数据时，不光要更
       新簇链，还要更新文件的开始簇，它是文件整条簇链的开始。
*************************************************************************************/
#ifdef UPDATE_FILE_SCLUST
UINT8 Update_File_sClust(struct FileInfo *pfi,UINT32 clu) //更新文件开始簇
{
 struct FDI *pfdi;

 just_file=pfi;

 znFAT_Device_Read_Sector(pfi->FDI_Sec,znFAT_Buffer);

 pfdi=(((struct FDIesInSEC *)znFAT_Buffer)->FDIes)+(pfi->nFDI); //文件的文件目录项

 pfi->File_StartClust=clu;

 (pfdi->HighClust)[0]=(UINT8)((clu&0X00FF0000)>>16);
 (pfdi->HighClust)[1]=(UINT8)((clu&0XFF000000)>>24);
 (pfdi->LowClust )[0]=(UINT8)((clu&0X000000FF))    ;
 (pfdi->LowClust )[1]=(UINT8)((clu&0X0000FF00)>>8) ;

 znFAT_Device_Write_Sector(pfi->FDI_Sec,znFAT_Buffer);

 return 0; 
}
#endif

/************************************************************************************
 功能：创建一条簇链
 形参：pfi:指向文件信息集合的指针 cluster:簇链的开始簇 datalen:数据长度(字节)
 返回：运行结果 成功或错误码
 详解：此函数的功能是为了向文件中写入数据而预先建立簇链，从而可以提供数据写入的效率
       （使数据写入的过程中，不再牵扯簇链构造的操作）加之对簇链的连续段的分析，可以很
       大程序上提高多扇区连续读写驱动的使用率。这一函数是znFAT“大模式”的基础（大模
       式就是先为文件预先建立好簇链，然后就是单纯的数据写入了，这样极大的提升了数据写
       入的速度和效率，是数据高速存储的解决方案）。
*************************************************************************************/
#ifdef CREATE_CLUSTER_CHAIN
UINT8 Create_Cluster_Chain(struct FileInfo *pfi,UINT32 cluster,UINT32 datalen)
{
 UINT32 iSec=0,clu_sec=0,old_clu=cluster,ncluster=0,temp_ncluster=0;
 UINT8 iItem=0,temp=0;
 struct FAT_Sec *pFAT_Sec=(struct FAT_Sec *)znFAT_Buffer; //将数据缓冲区首地址强转为FAT_Sec结构体的指针类型

 UINT32 Clu_Size=(pInit_Args->SectorsPerClust*pInit_Args->BytesPerSector); //簇大小，以免后面重复计算

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 ncluster=datalen/Clu_Size;
 if(0!=datalen%Clu_Size) ncluster++; //如果有数据余量（整扇区），簇数加1

 temp_ncluster=ncluster; //记录下簇链的簇数

 if((pInit_Args->Free_nCluster)<ncluster) return ERR_NO_SPACE; //无足够的存储空间

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 #ifndef USE_ALONE_CCCB
 if(pfi!=pcccb_cur_oc) //如果当前占用CCCB的不是现在要操作的文件
 {
  CCCB_Update_FAT();
  pcccb_cur_oc=pfi;
  (*pcccb_curdev)=Dev_No;
  pcccb_cur_initargs=pInit_Args;
 }
 #endif
 (*pcccb_curval)=cluster;
 #endif

 //===================================这里可能产生返链==========================================

 cluster=(pInit_Args->Next_Free_Cluster);

 pfi->File_CurClust=pInit_Args->Next_Free_Cluster; //当前簇为下一空簇
 pfi->File_CurSec=SOC(pfi->File_CurClust); //当前簇的首扇区

 ncluster--;
 //============================

 if(0!=old_clu)
 {
  clu_sec=(old_clu/NITEMSINFATSEC); //计算前一簇的簇项所在的FAT扇区
  znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);

  #ifdef RT_UPDATE_CLUSTER_CHAIN
  temp=(UINT8)(old_clu%NITEMSINFATSEC);
  (((pFAT_Sec->items)[temp]).Item)[0]=(UINT8)(cluster&0X000000FF)      ;  //将其链在前面的簇项上   
  (((pFAT_Sec->items)[temp]).Item)[1]=(UINT8)((cluster&0X0000FF00)>>8) ;
  (((pFAT_Sec->items)[temp]).Item)[2]=(UINT8)((cluster&0X00FF0000)>>16);
  (((pFAT_Sec->items)[temp]).Item)[3]=(UINT8)((cluster&0XFF000000)>>24);
  #else
  //---------------------------------CCCB的处理--------------------------------------
  if(0==(*pcccb_counter)) 
  {
   pcccb_buf[(*pcccb_counter)]=(*pcccb_curval);
   (*pcccb_counter)++;
  }

  if(cluster==((*pcccb_curval)+1))
  {
   (*pcccb_curval)++;
  }
  else
  {
   if(((*pcccb_counter)+1)!=CCCB_LEN) //CCCB没有溢出
   {
    pcccb_buf[(*pcccb_counter)]=(*pcccb_curval);

    (*pcccb_counter)++;
    pcccb_buf[(*pcccb_counter)]=cluster;
    (*pcccb_curval)=cluster;
    (*pcccb_counter)++;
   }
   else //CCCB溢出，此时需要将CCCB更新到FAT
   {
	CCCB_Update_FAT();
	#ifndef USE_ALONE_CCCB
	pcccb_cur_oc=pfi;
    (*pcccb_curdev)=Dev_No;
    pcccb_cur_initargs=pInit_Args;
    #endif

    (*pcccb_counter)=0;
    pcccb_buf[(*pcccb_counter)]=pcccb_buf[(*pcccb_counter)+1]=(*pcccb_curval);
    pcccb_buf[(*pcccb_counter)+2]=cluster;
    (*pcccb_curval)=cluster;
    (*pcccb_counter)+=3;
   }
  }
  //---------------------------------CCCB的处理--------------------------------------
  #endif
 }
 else
 {
  clu_sec=(cluster/NITEMSINFATSEC); //计算前一簇的簇项所在的FAT扇区
  znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
  
  #ifndef RT_UPDATE_CLUSTER_CHAIN
  //---------------------------------CCCB的处理--------------------------------------
  (*pcccb_counter)=0;
  pcccb_buf[(*pcccb_counter)]=cluster;
  (*pcccb_curval)=cluster;
  (*pcccb_counter)++;
  //---------------------------------CCCB的处理--------------------------------------
  #endif
 }

 #ifdef RT_UPDATE_CLUSTER_CHAIN
 if(clu_sec==(cluster/NITEMSINFATSEC)) //如果目标簇项与前一簇项在同一扇区
 {
  if(0==ncluster) 
  {
   temp=(UINT8)(cluster%NITEMSINFATSEC);
   (((pFAT_Sec->items)[temp]).Item)[0]=0XFF;  //簇链封口  
   (((pFAT_Sec->items)[temp]).Item)[1]=0XFF;
   (((pFAT_Sec->items)[temp]).Item)[2]=0XFF;
   (((pFAT_Sec->items)[temp]).Item)[3]=0X0F;   

   znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
   znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
  }
 }
 else //如果目标簇项与前一簇项不在同一扇区
 {
  znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
  znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);

  clu_sec=(cluster/NITEMSINFATSEC); //计算前一簇的簇项所在的FAT扇区
  znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);

  if(0==ncluster) 
  {
   temp=(UINT8)(cluster%NITEMSINFATSEC);

   (((pFAT_Sec->items)[temp]).Item)[0]=0XFF;  //簇链封口  
   (((pFAT_Sec->items)[temp]).Item)[1]=0XFF;
   (((pFAT_Sec->items)[temp]).Item)[2]=0XFF;
   (((pFAT_Sec->items)[temp]).Item)[3]=0X0F;   

   znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
   znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
  }
 }
 #endif
 
 if(0==ncluster) //如果簇链构造完成
 {
  pInit_Args->Free_nCluster-=temp_ncluster; //更新剩余空簇数

  Update_Next_Free_Cluster();

  #ifdef RT_UPDATE_FSINFO
  Update_FSINFO();
  #endif

  return ERR_SUCC;
 }

 old_clu=cluster; 

 //=============================================================================================

 clu_sec=(old_clu/NITEMSINFATSEC);

 if(((UINT8)((cluster%NITEMSINFATSEC)+1))!=((UINT8)NITEMSINFATSEC)) //如果当前的簇项不是其所在FAT扇区中的最后一个簇项
 {
  znFAT_Device_Read_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);

  for(iItem=(UINT8)((cluster%NITEMSINFATSEC)+1);iItem<NITEMSINFATSEC;iItem++) //检测在当前FAT扇区内当前簇项之后是否有空簇
  {
   cluster++; //簇号自增

   if(0==(((pFAT_Sec->items)[iItem]).Item)[0]  //如果发现空簇
   && 0==(((pFAT_Sec->items)[iItem]).Item)[1]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[2]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[3])
   { 
	#ifdef RT_UPDATE_CLUSTER_CHAIN
    temp=(UINT8)(old_clu%NITEMSINFATSEC);

    (((pFAT_Sec->items)[temp]).Item)[0]=(UINT8)(cluster&0X000000FF)      ;  //将其链在前面的簇项上   
    (((pFAT_Sec->items)[temp]).Item)[1]=(UINT8)((cluster&0X0000FF00)>>8) ;
    (((pFAT_Sec->items)[temp]).Item)[2]=(UINT8)((cluster&0X00FF0000)>>16);
    (((pFAT_Sec->items)[temp]).Item)[3]=(UINT8)((cluster&0XFF000000)>>24);
    #else
	//---------------------------------CCCB的处理--------------------------------------
    if(cluster==((*pcccb_curval)+1))
	{
     (*pcccb_curval)++;
	}
    else
	{
     if(((*pcccb_counter)+1)!=CCCB_LEN) //CCCB没有溢出
     {
      pcccb_buf[(*pcccb_counter)]=(*pcccb_curval);

      (*pcccb_counter)++;
      pcccb_buf[(*pcccb_counter)]=cluster;
      (*pcccb_curval)=cluster;
      (*pcccb_counter)++;
     }
     else //CCCB溢出，此时需要将CCCB更新到FAT，并清空CCCB，以便再次利用
     {
	  CCCB_Update_FAT();
	  #ifndef USE_ALONE_CCCB
	  pcccb_cur_oc=pfi;
      (*pcccb_curdev)=Dev_No;
      pcccb_cur_initargs=pInit_Args;
      #endif

	  (*pcccb_counter)=0;
	  pcccb_buf[(*pcccb_counter)]=pcccb_buf[(*pcccb_counter)+1]=(*pcccb_curval);
	  pcccb_buf[(*pcccb_counter)+2]=cluster;
      (*pcccb_curval)=cluster;
	  (*pcccb_counter)+=3;
     }
	}
	//---------------------------------CCCB的处理--------------------------------------
    #endif
    ncluster--;
    old_clu=cluster;
   }

   if(0==ncluster) 
   {
    #ifdef RT_UPDATE_CLUSTER_CHAIN
	(((pFAT_Sec->items)[iItem]).Item)[0]=0XFF;
	(((pFAT_Sec->items)[iItem]).Item)[1]=0XFF;
	(((pFAT_Sec->items)[iItem]).Item)[2]=0XFF;
	(((pFAT_Sec->items)[iItem]).Item)[3]=0X0F; //FAT链封口
    
    znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
    znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
    #endif
    pInit_Args->Free_nCluster-=temp_ncluster; //更新剩余空簇数
    pInit_Args->Next_Free_Cluster=cluster; 

    Update_Next_Free_Cluster();

    #ifdef RT_UPDATE_FSINFO
    Update_FSINFO();
    #endif

    return ERR_SUCC;
   }
  }
 }
 #ifdef RT_UPDATE_CLUSTER_CHAIN
 znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
 znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
 #endif
 //=============================================================================================

 for(iSec=(clu_sec+1);iSec<(pInit_Args->FATsectors);iSec++) //在后面的FAT扇区中继续查找
 {
  znFAT_Device_Read_Sector(iSec+(pInit_Args->FirstFATSector),znFAT_Buffer);

  for(iItem=0;iItem<NITEMSINFATSEC;iItem++) //检测在当前FAT扇区内当前簇项之后是否有空簇
  {
   cluster++;

   if(0==(((pFAT_Sec->items)[iItem]).Item)[0]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[1]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[2]
   && 0==(((pFAT_Sec->items)[iItem]).Item)[3])
   {
    #ifdef RT_UPDATE_CLUSTER_CHAIN
	clu_sec=(old_clu/NITEMSINFATSEC);
    temp=(UINT8)(old_clu%NITEMSINFATSEC);

    if(iSec!=clu_sec) //如果要更新的簇项所在的扇区与当前扇区不是同一扇区
	{                 //则需要在更新簇项后，恢复内部级冲的数据为当前扇区
     Modify_FAT(old_clu,cluster);

     znFAT_Device_Read_Sector(iSec+(pInit_Args->FirstFATSector),znFAT_Buffer);	 
	}
	else //而如果要更新的簇项所在扇区与当前扇区为同一扇区，则只需要在内部缓冲中进行更新
	{    //而无需向扇区中回写，这是提供簇链创建速度的核心思想
     (((pFAT_Sec->items)[temp]).Item)[0]=(UINT8)(cluster&0X000000FF)      ;  //将其链在前面的簇项上   
     (((pFAT_Sec->items)[temp]).Item)[1]=(UINT8)((cluster&0X0000FF00)>>8) ;
     (((pFAT_Sec->items)[temp]).Item)[2]=(UINT8)((cluster&0X00FF0000)>>16);
     (((pFAT_Sec->items)[temp]).Item)[3]=(UINT8)((cluster&0XFF000000)>>24);
	}
    #else
	//---------------------------------CCCB的处理--------------------------------------
    if(cluster==((*pcccb_curval)+1))
	{
     (*pcccb_curval)++;
	}
    else
	{
     if(((*pcccb_counter)+1)!=CCCB_LEN) //CCCB没有溢出
     {
      pcccb_buf[(*pcccb_counter)]=(*pcccb_curval);

      (*pcccb_counter)++;
      pcccb_buf[(*pcccb_counter)]=cluster;
      (*pcccb_curval)=cluster;
      (*pcccb_counter)++;
     }
     else //CCCB没有溢出，此时需要将CCCB更新到FAT
     {
	  CCCB_Update_FAT();
	  #ifndef USE_ALONE_CCCB
	  pcccb_cur_oc=pfi;
      (*pcccb_curdev)=Dev_No;
      pcccb_cur_initargs=pInit_Args;
      #endif

	  (*pcccb_counter)=0;
	  pcccb_buf[(*pcccb_counter)]=pcccb_buf[(*pcccb_counter)+1]=(*pcccb_curval);
	  pcccb_buf[(*pcccb_counter)+2]=cluster;
      (*pcccb_curval)=cluster;
	  (*pcccb_counter)+=3;
     }
	}
	//---------------------------------CCCB的处理--------------------------------------
    #endif
	ncluster--;
	old_clu=cluster;
   }

   if(0==ncluster) 
   {
	#ifdef RT_UPDATE_CLUSTER_CHAIN
	clu_sec=(old_clu/NITEMSINFATSEC);

	(((pFAT_Sec->items)[iItem]).Item)[0]=0XFF;
	(((pFAT_Sec->items)[iItem]).Item)[1]=0XFF;
	(((pFAT_Sec->items)[iItem]).Item)[2]=0XFF;
	(((pFAT_Sec->items)[iItem]).Item)[3]=0X0F; //FAT链封口

    znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector),znFAT_Buffer);
    znFAT_Device_Write_Sector(clu_sec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
    #endif
    pInit_Args->Free_nCluster-=temp_ncluster; //更新剩余空簇数
    pInit_Args->Next_Free_Cluster=cluster; 

    Update_Next_Free_Cluster();

    #ifdef RT_UPDATE_FSINFO
    Update_FSINFO();
    #endif

    return ERR_SUCC;
   }
  }
  #ifdef RT_UPDATE_CLUSTER_CHAIN
  znFAT_Device_Write_Sector(iSec+(pInit_Args->FirstFATSector),znFAT_Buffer);
  znFAT_Device_Write_Sector(iSec+(pInit_Args->FirstFATSector+pInit_Args->FATsectors),znFAT_Buffer);
  #endif
 } 

 return ERR_FAIL;
}
#endif

/************************************************************************************
 功能：从文件整簇位置开始写入数据
 形参：pfi:指向文件信息集合的指针 len:要写入的数据长度 pbuf:指向数据缓冲区
 返回：实际写入的数据量
 详解：znFAT中向文件写入数据是以追加方式写入的，即总是从文件的末尾向后写入数据。写入
       数据的依据是文件当前的位置参数，但是当文件为空文件，即其数据为0，或是数据为簇
       大小的整数倍，此时文件的位置参数将不能如实表达文件数据的当前位置（文件为空时
       当前簇为0，整簇时当前簇为最后一个有效簇），这种情况就是znFAT中的“囧簇”，此函
       数专门用以处理这种情况。
*************************************************************************************/
#ifdef WRITEDATA_FROM_NCLUSTER
UINT32 WriteData_From_nCluster(struct FileInfo *pfi,UINT32 len,UINT8 *pbuf)
{
 UINT32 CluSize=((pInit_Args->BytesPerSector)*(pInit_Args->SectorsPerClust)); //计算簇大小，以免后面重复计算
 UINT32 temp=len/CluSize;//计算要写入的数据量够几个整簇
 UINT32 iClu=0,start_clu=0,end_clu=0,next_clu=0; 
 UINT32 temp1=0,temp2=0;
 UINT8 res=0;

 #ifdef USE_EXCHANGE_BUFFER
 #ifndef USE_ALONE_EXB
 UINT8 old_devno=Dev_No;
 #else
 pexb_buf=(pfi->exb_buf);
 #endif
 #endif

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=1;
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 if(0==len) return 0; //如果要写入的数据长度为0，则直接返回

 if(0==pfi->File_CurClust) //如果是空文件，则当前簇为0，即尚未为其分配簇
 {
  pfi->File_StartClust=pInit_Args->Next_Free_Cluster;
  Update_File_sClust(pfi,pInit_Args->Next_Free_Cluster);
 }

 res=Create_Cluster_Chain(pfi,pfi->File_CurClust,len); //为整簇数据预建簇链
 if(res) return res;

 if(0==temp) //如果要写入的数据少于一个簇
 {
  temp=len/(pInit_Args->BytesPerSector); //要写入的数据够几个整扇区
  znFAT_Device_Write_nSector(temp,SOC(pfi->File_CurClust),pbuf);
  pfi->File_CurSec+=temp;
  pbuf+=(temp*(pInit_Args->BytesPerSector));

  temp=len%(pInit_Args->BytesPerSector);
  if(0!=temp) //还有数据要写入，不足扇区的最后一点数据
  {
   #ifndef USE_EXCHANGE_BUFFER
   Memory_Copy(znFAT_Buffer,pbuf,temp); //把不足一扇区的数据先放入内部缓冲区中
   znFAT_Device_Write_Sector(pfi->File_CurSec,znFAT_Buffer); //将内部缓冲区中的数据写入扇区中
                                                         //如果直接使用pbuf作数据源来写入，因不足一个扇区
                                                         //从而会造成内存溢出,程序崩溃
   #else
   #ifndef USE_ALONE_EXB
   if(Dev_No!=sexb_cur_dev) //如果现在操作的设备不是当前占用EXB的设备
   {
	if(0!=sexb_cur_sec) //如果EXB正被占用
	{
	 Dev_No=sexb_cur_dev;
	 znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

     Dev_No=old_devno;
	}
   }
   else //如果现在操作的设备正是当前占用EXB的设备
   {
	if(sexb_cur_sec!=(pfi->File_CurSec)) //占用EXB的扇区不是当前要操作的扇区
	{
	 if(0!=sexb_cur_sec) //如果EXB正被占用
	 {
	  znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中
	 }	   
	}
   }
   #endif 

   Memory_Copy(pexb_buf,pbuf,temp);

   #ifndef USE_ALONE_EXB
   sexb_cur_sec=pfi->File_CurSec;   //记录当前交换缓冲中所反映的扇区地址
   sexb_cur_dev=Dev_No; //记录当前交换缓冲中数据所在的设备号
   psexb_cur_oc=pfi; //记录EXB中缓冲的数据属于哪个文件
   #else
   just_file->exb_cursec=pfi->File_CurSec;
   #endif
   #endif
	  
   pfi->File_CurPos=(UINT16)temp;   
  }
 }
 else
 {
  //计算各连续簇段，以尽可能的使用多扇区写驱动，以提高数据写入速度
  //start_clu与end_clu用于记录连续簇段的始末
  start_clu=end_clu=pfi->File_CurClust; 

  for(iClu=1;iClu<temp;iClu++)
  {
   next_clu=Get_Next_Cluster(end_clu);

   if((next_clu-1)==end_clu) //如果两个簇相临，即连续
   {
    end_clu=next_clu;
   }
   else //如果两个簇不相临，即遇到簇链断点
   {
    znFAT_Device_Write_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)),SOC(start_clu),pbuf);
	pbuf+=((end_clu-start_clu+1)*CluSize);
    start_clu=next_clu;
    end_clu=next_clu;
   }
  }

  temp1=(len%CluSize)/(pInit_Args->BytesPerSector); //剩余数据够几个整扇区
  temp2=Get_Next_Cluster(end_clu);
  temp=(end_clu-start_clu+1)*(pInit_Args->SectorsPerClust);
 
  if(!IS_END_CLU(temp2)) //如果下一簇不是结束簇，即后面还有数据要写入
  {
   if((temp2-1)==end_clu) //如果最后一个簇中的剩余扇区与前面的最后的连续簇段是连续的
   {
    znFAT_Device_Write_nSector((temp+temp1),SOC(start_clu),pbuf);
    pbuf+=((temp+temp1)*(pInit_Args->BytesPerSector));
   }
   else
   {
    znFAT_Device_Write_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)),SOC(start_clu),pbuf); 
    pbuf+=(temp*(pInit_Args->BytesPerSector));
    znFAT_Device_Write_nSector(temp1,SOC(temp2),pbuf);
    pbuf+=(temp1*(pInit_Args->BytesPerSector));
   }

   pfi->File_CurClust=temp2;
   pfi->File_CurSec=(SOC(temp2)+temp1);
   //=======================================================================================
   temp=len%(pInit_Args->BytesPerSector);
   if(0!=temp) //还有数据要写入
   {
	#ifndef USE_EXCHANGE_BUFFER
    Memory_Copy(znFAT_Buffer,pbuf,temp); //把不足一扇区的数据先放入内部缓冲区中
    znFAT_Device_Write_Sector(pfi->File_CurSec,znFAT_Buffer); //将内部缓冲区中的数据写入扇区中
                                                         //如果直接使用pbuf作数据源来写入，因不足一个扇区
                                                         //从而会造成内存溢出,程序崩溃
    #else
    #ifndef USE_ALONE_EXB
    if(Dev_No!=sexb_cur_dev) //如果现在操作的设备不是当前占用EXB的设备
    {
	 if(0!=sexb_cur_sec) //如果EXB正被占用
	 {
	  Dev_No=sexb_cur_dev;
	  znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

      Dev_No=old_devno;
	 }
    }
    else //如果现在操作的设备正是当前占用EXB的设备
    {
	 if(sexb_cur_sec!=(pfi->File_CurSec)) //占用EXB的扇区不是当前要操作的扇区
	 {
	  if(0!=sexb_cur_sec) //如果EXB正被占用
	  {
	   znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中
	  }	   
	 }
    }
    #endif

    Memory_Copy(pexb_buf,pbuf,temp);

    #ifndef USE_ALONE_EXB
    sexb_cur_sec=pfi->File_CurSec;
	sexb_cur_dev=Dev_No; //记录当前交换缓冲中数据所在的设备号
	psexb_cur_oc=pfi; //记录EXB中缓冲的数据属于哪个文件
    #else
    just_file->exb_cursec=pfi->File_CurSec;
    #endif
    #endif   
	
	pfi->File_CurPos=(UINT16)temp;
   }
  }
  else //如果下一簇已为结束簇，即后面已无数据再要写入
  {
   znFAT_Device_Write_nSector(temp,SOC(start_clu),pbuf);
   pbuf+=((temp+temp1)*(pInit_Args->BytesPerSector)); 

   pfi->File_CurClust=end_clu;
   pfi->File_CurSec=SOC(end_clu);
  }
 }

 //========================以上代码用于处理整簇与整扇区数据的写入，尽可能利用了扇区的连续性========================
 #ifdef RT_UPDATE_FSINFO
 Update_FSINFO();
 #endif

 pfi->File_CurOffset+=len;

 return len;
} 
#endif

/************************************************************************************
 功能：向文件中写入数据
 形参：pfi:指针文件信息集合的指针 len:要写入的数据量 pbuf:指向数据缓冲区的指针
 返回：实际向文件中写入的数据量  如果文件大小已经达到FAT32中所限制的极限，返回-2
 详解：这是最终供使用者调用的文件数据写入函数，它总是从文件的末尾向后写入数据，即数据
       是以追加的方式被写入的。在数据写入之后，要及时的更新文件大小，否则写入的数据将
       不可见。可以打开RT_UPDATE_FILESIZE宏来开启实时文件大小更新功能，即每次写入数据
       znFAT都会去更新文件大小，这种工作方式下哪怕中间突然断电或死机都没有关系，可以
       保证文件大小如实反映文件的有效数据量，但这种方式使数据写入的速度和效率变慢。若
       不使用此宏，则需要使用者在所有写入数据的操作完成之后，调用更新文件大小的函数。
*************************************************************************************/
#ifdef ZNFAT_WRITEDATA
UINT32 znFAT_WriteData(struct FileInfo *pfi,UINT32 len,UINT8 *pbuf)
{
 UINT32 temp=0,temp1=0,len_temp=len;
 UINT32 Cluster_Size=((pInit_Args->BytesPerSector)*(pInit_Args->SectorsPerClust));

 #ifdef USE_EXCHANGE_BUFFER
 #ifndef USE_ALONE_EXB
 UINT8 old_devno=Dev_No;
 #else
 pexb_buf=(pfi->exb_buf);
 #endif
 #endif

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 get_next_cluster_in_cccb=1;
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 if(0==len) return 0; //如果要写入的数据长度为0，则直接返回0

 if(len>(0XFFFFFFFF-pfi->File_Size)) return (UINT32)-2; //文件大小在写入数据后将发生溢出错误
 
 znFAT_Seek(pfi,pfi->File_Size); //文件数据定位到文件末尾，文件位置相关信息随即改变

 //检查磁盘剩余空间是否够用
 if((pfi->File_CurOffset%Cluster_Size)!=0)
 {
  temp=((pInit_Args->BytesPerSector)-(pfi->File_CurPos))+((LAST_SEC_OF_CLU(pfi->File_CurClust))-(pfi->File_CurSec))*(Cluster_Size);
  //当前簇剩余数据量

  if(len>temp) //如果要写入的数据量大于temp，则说明必然会超出当前簇，为其扩展空簇
  {
   temp1=(len-temp)/(Cluster_Size);
   if((len-temp)%(Cluster_Size)) temp1++; //计算需要多少个空簇

   if(temp1>(pInit_Args->Free_nCluster)) return ((UINT32)-1); //空间不足
  }

 }
 else
 {
  temp1=len/(Cluster_Size);
  if(len%(Cluster_Size)) temp1++; //计算需要多少个空簇  
  if(temp1>(pInit_Args->Free_nCluster)) return ((UINT32)-1); //空间不足
 }

 //===========================================================================================================

 temp=((pInit_Args->BytesPerSector)-(pfi->File_CurPos)); //计算赋给临时变量，以免后面重复计算

 if((pfi->File_CurOffset%Cluster_Size)!=0)
 {
  if(len<=temp) //要写入的数据小于等于当前扇区剩余数据量
  {
   #ifndef USE_EXCHANGE_BUFFER
   znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //读取当前扇区数据，以便作扇区内数据拼接
   Memory_Copy(znFAT_Buffer+pfi->File_CurPos,pbuf,len); //扇区数据拼接
   znFAT_Device_Write_Sector(pfi->File_CurSec,znFAT_Buffer); //回写扇区数据
   #endif

   if(len==temp) //如果要写入的数据正好占满当前扇区
   {
	#ifdef USE_EXCHANGE_BUFFER
	if(0!=pfi->File_CurPos) 
	{
     #ifndef USE_ALONE_EXB
	 if(Dev_No!=sexb_cur_dev) //如果现在操作的设备不是当前占用EXB的设备
	 {
	  if(0!=sexb_cur_sec) //如果EXB正被占用
	  {
	   Dev_No=sexb_cur_dev;
	   znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

       Dev_No=old_devno;
	  }
	  znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf); 
	 }
	 else //如果现在操作的设备正是当前占用EXB的设备
	 {
	  if(sexb_cur_sec!=(pfi->File_CurSec)) //占用EXB的扇区不是当前要操作的扇区
	  {
	   if(0!=sexb_cur_sec) //如果EXB正被占用
	   {
	    znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

	    znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf); 
	   }	   
	  }
	 }

     #else
	 if(0==(just_file->exb_cursec)) 
	 {
	  znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf);
	 }
     #endif

	 Memory_Copy(pexb_buf+pfi->File_CurPos,pbuf,len); //扇区数据拼接

     znFAT_Device_Write_Sector(pfi->File_CurSec,pexb_buf); //回写扇区数据
     
     #ifndef USE_ALONE_EXB
	 sexb_cur_sec=0; //每次EXB中的数据以整扇区数据回写之后，我们便认为它不再被占用了
	 sexb_cur_dev=(UINT8)(-1); //EXB的当前设备号置为空，这里取-1认定其为空，为了与有效设备号0相区分
	 psexb_cur_oc=(struct FileInfo *)0; //此时EXB不归任何文件所有
     #else
	 (just_file->exb_cursec)=0; //文件的独立EXB未占用
     #endif
	}
	else
	{
	 znFAT_Device_Write_Sector(pfi->File_CurSec,pbuf); //回写扇区数据
	}
    #endif

	if(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust)) //如果当前扇区是当前簇的最后一个扇区
	{
	 pfi->File_CurSec=SOC(pfi->File_CurClust); //更新当前扇区，其实无效，为了规整
	}
	else //当前扇区不是当前簇的最后扇区
	{
	 pfi->File_CurSec++;
	}

	pfi->File_CurPos=0;
	pfi->File_CurOffset+=len; //更新当前偏移量
	pfi->File_Size+=len; //更新文件大小
     
    #ifdef RT_UPDATE_FILESIZE
	Update_File_Size(pfi); //更文件目录项中的文件大小字段
    #endif

	return len;
   }
   else//len小于当前扇区剩余数据量
   {  
	#ifdef USE_EXCHANGE_BUFFER
    #ifndef USE_ALONE_EXB
	if(Dev_No!=sexb_cur_dev) //如果现在操作的设备不是当前占用EXB的设备
	{
	 if(0!=sexb_cur_sec) //如果EXB正被占用
	 {
	  Dev_No=sexb_cur_dev;
	  znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

      Dev_No=old_devno; 
	 }
	 znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf);
	}
	else //如果现在操作的设备正是当前占用EXB的设备
	{
	 if(sexb_cur_sec!=(pfi->File_CurSec)) //占用EXB的扇区不是当前要操作的扇区
	 {
	  if(0!=sexb_cur_sec) //如果EXB正被占用
	  {
	   znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

	   znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf); 
	  }	   
	 }
	}
    #else
	if((0==(just_file->exb_cursec)) && (0!=(pfi->File_CurPos))) //当前文件的独立EXB未被占用，且文件当前扇区内偏移不为0，如果为0则没必要读扇区
	{
	 znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf);
	}
    #endif

	Memory_Copy(pexb_buf+pfi->File_CurPos,pbuf,len); //扇区数据拼接

    #ifndef USE_ALONE_EXB
	sexb_cur_dev=Dev_No;
	sexb_cur_sec=pfi->File_CurSec;
	psexb_cur_oc=pfi; //记录EXB中缓冲的数据属于哪个文件
    #else
	(just_file->exb_cursec)=pfi->File_CurSec;
    #endif
    #endif
    //znFAT_Device_Write_Sector(pfi->File_CurSec,ex_buf); //回写扇区数据

    pfi->File_CurPos+=(UINT16)len;
    pfi->File_CurOffset+=len; //更新当前偏移量
    pfi->File_Size+=len; //更新文件大小 
	
    #ifdef RT_UPDATE_FILESIZE
	Update_File_Size(pfi); //更文件目录项中的文件大小字段
    #endif

	return len;	 
   }
  }
  else 
  {
   #ifndef USE_EXCHANGE_BUFFER
   znFAT_Device_Read_Sector(pfi->File_CurSec,znFAT_Buffer); //读取当前扇区
   Memory_Copy(znFAT_Buffer+pfi->File_CurPos,pbuf,temp); //扇区数据拼接
   znFAT_Device_Write_Sector(pfi->File_CurSec,znFAT_Buffer); //回写扇区
   #else

   if(0!=pfi->File_CurPos) 
   {
    #ifndef USE_ALONE_EXB
	if(Dev_No!=sexb_cur_dev) //如果现在操作的设备不是当前占用EXB的设备
	{
	 if(0!=sexb_cur_sec) //如果EXB正被占用
	 {
	  Dev_No=sexb_cur_dev;
	  znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

      Dev_No=old_devno;
	 }
	 znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf); 
	}
	else //如果现在操作的设备正是当前占用EXB的设备
	{
	 if(sexb_cur_sec!=(pfi->File_CurSec)) //占用EXB的扇区不是当前要操作的扇区
	 {
	  if(0!=sexb_cur_sec) //如果EXB正被占用
	  {
	   znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

	   znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf); 
	  }	   
	 }
	}
    #else
	if(0==(just_file->exb_cursec)) 
	{
	 znFAT_Device_Read_Sector(pfi->File_CurSec,pexb_buf);
	}
    #endif

	Memory_Copy(pexb_buf+pfi->File_CurPos,pbuf,temp); //扇区数据拼接

    znFAT_Device_Write_Sector(pfi->File_CurSec,pexb_buf); //回写扇区数据

    #ifndef USE_ALONE_EXB
	sexb_cur_sec=0; //每次EXB中的数据以整扇区数据回写之后，我们便认为它不再被占用了
	sexb_cur_dev=(UINT8)(-1); //EXB的当前设备号置为空，这里取-1认定其为空，为了与有效设备号0相区分
	psexb_cur_oc=(struct FileInfo *)0;
    #else
	(just_file->exb_cursec)=0; //当前文件独立EXB未被占用
    #endif
   }
   else //如果当前位置在0位置，则直接写扇区
   {
	znFAT_Device_Write_Sector(pfi->File_CurSec,pbuf); //回写扇区数据
   }
   #endif

   len_temp-=temp;
   pbuf+=temp;

   if(!(IS_END_SEC_OF_CLU(pfi->File_CurSec,pfi->File_CurClust))) //如果当前扇区不是当前簇的最后一个扇区
   {
	pfi->File_CurSec++;
	pfi->File_CurPos=0;

    pfi->File_CurOffset+=temp;

    temp=(LAST_SEC_OF_CLU(pfi->File_CurClust)-(pfi->File_CurSec)+1)*(pInit_Args->BytesPerSector);//当前簇中的剩余整整扇区数据量

    if(len_temp<=temp) //如果要写入的数据量小于等于当前簇中的剩余整扇区数据量
	{
	 temp1=len_temp/(pInit_Args->BytesPerSector); //计算要写入的数据量够几个整扇区

	 znFAT_Device_Write_nSector(temp1,pfi->File_CurSec,pbuf);
	 pbuf+=((pInit_Args->BytesPerSector)*temp1);

	 if(len_temp==temp) //如果正好写满当前簇
	 {
	  pfi->File_CurSec=SOC(pfi->File_CurClust); //囧簇
	  pfi->File_CurPos=0;

	  pfi->File_CurOffset+=len_temp;
	  pfi->File_Size+=len;

      #ifdef RT_UPDATE_FILESIZE
   	  Update_File_Size(pfi); //更文件目录项中的文件大小字段
      #endif

	  return len;
	 }
	 else
	 {
	  pfi->File_CurSec+=temp1;
	  pfi->File_CurPos=(UINT16)(len_temp%(pInit_Args->BytesPerSector));

	  if(pfi->File_CurPos) //还有要写的数据,处理最后的字节数据
	  {
       #ifndef USE_EXCHANGE_BUFFER
	   Memory_Copy(znFAT_Buffer,pbuf,pfi->File_CurPos);
	   znFAT_Device_Write_Sector(pfi->File_CurSec,znFAT_Buffer);
       #else
       #ifndef USE_ALONE_EXB
       if(Dev_No!=sexb_cur_dev) //如果现在操作的设备不是当前占用EXB的设备
	   {
	    if(0!=sexb_cur_sec) //如果EXB正被占用
		{
	     Dev_No=sexb_cur_dev;
	     znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中

         Dev_No=old_devno;
		}
	   }
       else //如果现在操作的设备正是当前占用EXB的设备
	   {
	    if(sexb_cur_sec!=(pfi->File_CurSec)) //占用EXB的扇区不是当前要操作的扇区
		{
	     if(0!=sexb_cur_sec) //如果EXB正被占用
		 {
	      znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //如果EXB中还有数据，则先将这些数据回写到其相应扇区中
		 }	   
		}
	   }
       #endif

	   Memory_Copy(pexb_buf,pbuf,pfi->File_CurPos);

       #ifndef USE_ALONE_EXB
	   sexb_cur_sec=pfi->File_CurSec;
	   sexb_cur_dev=Dev_No;
	   psexb_cur_oc=pfi; //记录EXB中缓冲的数据属于哪个文件
       #else
	   just_file->exb_cursec=pfi->File_CurSec;
       #endif
       #endif
	  }

      pfi->File_CurOffset+=len_temp;
	  pfi->File_Size+=len;

      #ifdef RT_UPDATE_FILESIZE
  	  Update_File_Size(pfi); //更文件目录项中的文件大小字段
      #endif

	  return len;
	 }
	}
	else
	{
	 temp1=temp/(pInit_Args->BytesPerSector);

	 znFAT_Device_Write_nSector(temp1,pfi->File_CurSec,pbuf);
	 pbuf+=temp;

	 len_temp-=temp;

	 pfi->File_CurSec=SOC(pfi->File_CurClust);
	 pfi->File_CurPos=0;

	 pfi->File_CurOffset+=temp;	 
	}
   }
   else //当前扇区是当前簇最后一个扇区
   {
	pfi->File_CurSec=SOC(pfi->File_CurClust);
	pfi->File_CurPos=0;

	pfi->File_CurOffset+=temp;	   
   }   
  }
 }

 //如果文件的当前偏移量是簇大小的整数倍，则
 //直接进入空文件开始位置或整簇位置写数据的阶段
 WriteData_From_nCluster(pfi,len_temp,pbuf); //从空文件开始位置或整簇位置写数据，此时都是一种“窘境簇”的情况
                                              //这种情况下，数据正好停止于末簇的末尾或是空文件而没有数据，通
                                              //常文件信息集合中记录的文件位置信息应该是下一簇的最开始位置，
                                              //但是这个时候下一簇尚没有被分配，即文件末簇的FAT簇项记录的是
                                              //0XFFFFFF0F（簇链结束标记），因此此时的文件位置信息是无效的
                                              //znFAT中作出约定:“窘境簇”情况下，空文件当前簇为0，在末簇末尾
                                              //时，当前簇为末簇。这种约定便于znFAT的数据写入功能在“窘境簇”
                                              //情况下的正确性
 pfi->File_Size+=len;
 
 #ifdef RT_UPDATE_FILESIZE
 Update_File_Size(pfi); //更文件目录项中的文件大小字段
 #endif

 return len;
}
#endif

/************************************************************************************
 功能：文件数据截断
 形参：pfi:指向文件信息集合的指针 offset:要进行数据截断的起始位置
 返回：运行结果 成功或失败 
 详解：此函数用于将文件的数据从offset位置进行截断，即从此位置后面的数据均被删除。如果
       指定的offset如果大于等于文件的大小，则直接返回错误。
*************************************************************************************/
#ifdef ZNFAT_DUMP_DATA
UINT8 znFAT_Dump_Data(struct FileInfo *pfi,UINT32 offset)
{
 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 if(offset>=(pfi->File_Size)) //要截断的位置超出文件范围
 {
  return ERR_FAIL;
 }

 znFAT_Seek(pfi,offset); //定位到要截断的位置

 #ifndef RT_UPDATE_CLUSTER_CHAIN //销毁之前，先把CCCB中的簇链段更新到FAT
 #ifndef USE_ALONE_CCCB
 if(pfi==pcccb_cur_oc) 
 #endif
 {
  CCCB_Update_FAT();
 }
 #endif

 Destroy_FAT_Chain(pfi->File_CurClust); //销毁簇链

 if(offset>0)
 {
  Modify_FAT(pfi->File_CurClust,0X0FFFFFFF); //簇链封口
 }

 pfi->File_Size=offset; //更新文件大小

 #ifdef RT_UPDATE_FILESIZE
 Update_File_Size(pfi); //更文件目录项中的文件大小字段
 #endif

 if(0==pfi->File_Size) Update_File_sClust(pfi,0); //如果文件大小为0，更新文件开始簇为0

 #ifdef RT_UPDATE_FSINFO
 Update_FSINFO();
 #endif

 return ERR_SUCC;
}
#endif

#ifdef ZNFAT_MODIFY_DATA
UINT32 znFAT_Modify_Data(struct FileInfo *pfi,UINT32 offset,UINT32 len,UINT8 *app_Buffer)
{
 UINT32 temp_len=0,temp=0,nsec=0,iClu=0,start_clu=0,end_clu=0,next_clu=0,temp2=0,temp1=0;
 UINT32 Cluster_Size=((pInit_Args->BytesPerSector)*(pInit_Args->SectorsPerClust));

 if(offset>=(pfi->File_Size)) return ERR_MD_POS_OVER_FSIZE; //如果要修改的数据位置超出文件大小

 if((offset+len)>=(pfi->File_Size))  //从offset位置要改写的数据长度长于文件长度，则取实际长度
 {
  len=(pfi->File_Size)-offset;
  (pfi->File_IsEOF)=1; //此种情况一定会修改到文件最末尾，文件将达到结束位置
 }

 temp_len=len; 

 znFAT_Seek(pfi,offset); //定位文件offset位置，定位之后文件信息体相关变量即得到更新

 //======================================下面开始对数据进行改写==================================
 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN //如果使用了CCCB缓冲，则需要打开在CCCB中寻簇的标记
 get_next_cluster_in_cccb=1;
 #ifdef USE_ALONE_CCCB //如果使用了独立CCCB缓冲，则把CCCB打到当前所操作文件的CCCB
 CCCB_To_Alone();
 #endif
 #endif 
 //==============================================================================================
 if(((pfi->File_CurOffset)%Cluster_Size)!=0) //如果当前位置非簇大小整数倍，即不在簇开始位置
 {
  if(((pfi->File_CurOffset)%(pInit_Args->BytesPerSector))!=0) //如果当前位置非整扇区大小整数倍，即不在扇区开始位置
  {
   if(len<=((pInit_Args->BytesPerSector)-(pfi->File_CurPos))) //如果要修改的数据长度小于等于文件当前扇区剩余数据量
   {
	//如果启用了EXB缓冲，则此不足扇区的数据，可能并不在物理扇区中，而在EXB缓冲中
    #ifdef USE_EXCHANGE_BUFFER //如果使用了EXB缓冲
    #ifndef USE_ALONE_EXB //如果没有启用独立EXB缓冲，即启用了共享EXB缓冲
    if((psexb_cur_oc==pfi)&&(sexb_cur_dev==Dev_No)&&(sexb_cur_sec==(pfi->File_CurSec))) //如果此时文件正占用共享EXB，同时当前文件所在的设备与扇区和共享EXB相符
	{
	 Memory_Copy(pexb_buf+(pfi->File_CurPos),app_Buffer,len); //修改在共享EXB中的数据
	}
	else //否则要修改的数据不在EXB中，而在物理扇区中
	{
	 znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
     Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
     znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中
	}
    #else //如果启动独立EXB缓冲
	if((pfi->exb_cursec)==(pfi->File_CurSec)) //如果此文件的EXB对应扇区与文件当前扇区一致，则说明要修改的数据在此文件的EXB中
	{
	 Memory_Copy((pfi->exb_buf)+(pfi->File_CurPos),app_Buffer,len); //修改在文件独立EXB中的数据
	}
	else //如果此文件EXB对应扇区与文件当前扇区不一致，要修改的数据在物理扇区中
	{
	 znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
     Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
     znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中	 
	}
    
    #endif
    #else //如果没有使用EXB缓冲，则直接修改物理扇区数据
	znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
    Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
    znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中
    
    #endif

    //以下更新文件位置信息
	if(len<((pInit_Args->BytesPerSector)-(pfi->File_CurPos))) //要修改的数据量小于当前扇区剩余数据量
	{
	 (pfi->File_CurOffset)+=len;
	 (pfi->File_CurPos)+=len;
	}
    else
	if(len==((pInit_Args->BytesPerSector)-(pfi->File_CurPos))) //要修改的数据量等于当前扇区剩余数据量
	{
     if((len+(pfi->File_CurOffset))==(pfi->File_Size)) //正好写到文件末尾
	 {
	  if(((pfi->File_Size)%Cluster_Size)==0) //如果文件大小为簇大小整数倍，即此时产生了“囧簇”
	  {
	   (pfi->File_CurOffset)+=len;
	   (pfi->File_CurPos)=0;
	   (pfi->File_CurSec)=SOC((pfi->File_CurClust));
	  }
	  else //如果文件大小非簇大小整数倍，即此时数据写到了非簇末扇区末尾
	  {
	   (pfi->File_CurOffset)+=len;
	   (pfi->File_CurPos)=0;
	   (pfi->File_CurSec)++;
	  }
	 }
	 else
	 {
	  if(((len+(pfi->File_CurOffset))%Cluster_Size)==0) //如果写到的位置正好是簇大小整数倍，而此时并没有到文件末尾
	  {
	   (pfi->File_CurOffset)+=len;
	   (pfi->File_CurPos)=0;
	   (pfi->File_CurClust)=Get_Next_Cluster((pfi->File_CurClust));
	   (pfi->File_CurSec)=SOC((pfi->File_CurClust));	   
	  }
	  else //如果写到的位置非簇大小整数倍，即此时数据写到了非簇末扇区末尾
	  {
	   (pfi->File_CurOffset)+=len;
	   (pfi->File_CurPos)=0;
	   (pfi->File_CurSec)++;
	  }
	 }
	}
	return temp_len;
   }
   else //要修改的数据长度大于当前扇区剩余数据量
   {
	temp=(pInit_Args->BytesPerSector)-(pfi->File_CurPos);
	znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer);
    Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,temp);
	znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer);
	len-=temp;app_Buffer+=temp;
    (pfi->File_CurOffset)+=temp;

	(pfi->File_CurPos)=0;

    if(!IS_END_SEC_OF_CLU((pfi->File_CurSec),(pfi->File_CurClust))) //如果当前扇区不是当前簇的结束扇区
	{
	 (pfi->File_CurSec)++;
	}
	else
	{
	 (pfi->File_CurClust)=Get_Next_Cluster((pfi->File_CurClust));
	 (pfi->File_CurSec)=SOC((pfi->File_CurClust));
	}
   }
  }

  if(((pfi->File_CurOffset)%(Cluster_Size))!=0) //这里再次判断此时的位置是否在整簇开始，为了后面的统一化处理
  {
   temp=(((SOC(pfi->File_CurClust))+(pInit_Args->SectorsPerClust)-1)-(pfi->File_CurSec)+1)*(pInit_Args->BytesPerSector);

   if(len<=temp) //要修改的剩余数据量小于等于当前整扇区的剩余数据量
   {
    nsec=len/(pInit_Args->BytesPerSector); //计算要修改的剩余数据量中的整扇区数
    znFAT_Device_Write_nSector(nsec,(pfi->File_CurSec),app_Buffer); //向当前簇内要修改的整扇区数据部分写入数据
    temp=(nsec*(pInit_Args->BytesPerSector));
    len-=temp;app_Buffer+=temp;

    (pfi->File_CurOffset)+=temp;

    if(len==0) //要修改的数据正好写满当前簇
	{
     if((pfi->File_CurOffset)==(pfi->File_Size)) //如果正好写到了文件末尾，则此时产生“囧簇”
	 {
	  (pfi->File_CurPos)=0;
	  (pfi->File_CurSec)=SOC((pfi->File_CurClust));
	 }
     else //写满了当前簇，但并不是文件末尾
	 {
	  (pfi->File_CurClust)=Get_Next_Cluster((pfi->File_CurClust));
	  (pfi->File_CurPos)=0;
	  (pfi->File_CurSec)=SOC((pfi->File_CurClust));
	 }
	}
    else //没有写满当前簇
	{
     (pfi->File_CurPos)=0;
     (pfi->File_CurSec)+=nsec;

     //如果启用了EXB缓冲，则此不足扇区的数据，可能并不在物理扇区中，而在EXB缓冲中
     #ifdef USE_EXCHANGE_BUFFER //如果使用了EXB缓冲
     #ifndef USE_ALONE_EXB //如果没有启用独立EXB缓冲，即启用了共享EXB缓冲
     if((psexb_cur_oc==pfi)&&(sexb_cur_dev==Dev_No)&&(sexb_cur_sec==(pfi->File_CurSec))) //如果此时文件正占用共享EXB，同时当前文件所在的设备与扇区和共享EXB相符
	 {
	  Memory_Copy(pexb_buf+(pfi->File_CurPos),app_Buffer,len); //修改在共享EXB中的数据
	 }
     else //否则要修改的数据不在EXB中，而在物理扇区中
	 {
	  znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
      Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
      znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中
	 }
     #else //如果启动独立EXB缓冲
     if((pfi->exb_cursec)==(pfi->File_CurSec)) //如果此文件的EXB对应扇区与文件当前扇区一致，则说明要修改的数据在此文件的EXB中
	 {
	  Memory_Copy((pfi->exb_buf)+(pfi->File_CurPos),app_Buffer,len); //修改在文件独立EXB中的数据
	 }
     else //如果此文件EXB对应扇区与文件当前扇区不一致，要修改的数据在物理扇区中
	 {
	  znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
      Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
      znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中	 
	 }
   
     #endif
     #else //如果没有使用EXB缓冲，则直接修改物理扇区数据
     znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
     Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
     znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中
   
     #endif

     (pfi->File_CurOffset)+=len;
     (pfi->File_CurPos)=len;
	}

	return temp_len;
   }
   else //要修改的剩余数据量大于当前簇内剩余数据量
   {
    //temp=(pInit_Args->BytesPerSector)-(pfi->File_CurPos);
    //znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer);
    //Memory_Copy(znFAT_Buffer,app_Buffer,temp);
    //znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer);
    //len-=temp;app_Buffer+=temp;
    //(pfi->File_CurOffset)+=temp;
    //(pfi->File_CurSec)++;

    temp=(((SOC(pfi->File_CurClust))+(pInit_Args->SectorsPerClust)-1)-(pfi->File_CurSec)+1);
    znFAT_Device_Write_nSector(temp,(pfi->File_CurSec),app_Buffer); //向当前簇内要修改的整扇区数据部分写入数据
	temp*=(pInit_Args->BytesPerSector);
	len-=temp;app_Buffer+=temp;

	(pfi->File_CurOffset)+=temp;
	(pfi->File_CurClust)=Get_Next_Cluster((pfi->File_CurClust));
	(pfi->File_CurPos)=0;
	(pfi->File_CurSec)=SOC((pfi->File_CurClust));
   }
  }
 }

 //计算各连续簇段，以尽可能的使用多扇区写驱动，以提高数据写入速度
 //start_clu与end_clu用于记录连续簇段的始末
 temp=(len/Cluster_Size); 

 if(temp>0) //如果整簇数大于0，则涉及到多簇写
 {
  start_clu=end_clu=(pfi->File_CurClust);

  for(iClu=1;iClu<temp;iClu++)
  {
   next_clu=Get_Next_Cluster(end_clu);

   if((next_clu-1)==end_clu) //如果两个簇相临，即连续
   {
    end_clu=next_clu;
   }
   else //如果两个簇不相临，即遇到簇链断点
   {
    znFAT_Device_Write_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)),SOC(start_clu),app_Buffer);
    app_Buffer+=((end_clu-start_clu+1)*Cluster_Size);
    start_clu=next_clu;
    end_clu=next_clu;
   }
  } 
	 
  temp2=Get_Next_Cluster(end_clu);
  temp1=(len-temp*Cluster_Size); //要修改的剩余的非整簇数据
     
  if(temp1==0) //已无要修改的数据，即要修改的数据正好写到整簇
  {
   znFAT_Device_Write_nSector(((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)),SOC(start_clu),app_Buffer);

   if(!IS_END_CLU(temp2)) //如果已到了结束簇，即文件已到末尾，即“囧簇”
   {
    (pfi->File_CurClust)=end_clu;
   }
   else
   {
    (pfi->File_CurClust)=temp2;	   
   }

   (pfi->File_CurOffset)+=(temp*Cluster_Size);
   (pfi->File_CurPos)=0;
   (pfi->File_CurSec)=SOC((pfi->File_CurClust));

   return temp_len;
  }
  else
  {
   if((temp2-1)==end_clu) //下一簇如果与前面整簇最后一簇仍然连续
   {
    temp=((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust)+(temp1/(pInit_Args->BytesPerSector))); //连续扇区数
    znFAT_Device_Write_nSector(temp,SOC(start_clu),app_Buffer);

    (pfi->File_CurOffset)+=(temp*(pInit_Args->BytesPerSector));

    app_Buffer+=(temp*(pInit_Args->BytesPerSector));
    len-=(temp*(pInit_Args->BytesPerSector));
   }
   else //下一簇与前面簇不再连续
   {
    temp=((end_clu-start_clu+1)*(pInit_Args->SectorsPerClust));
    znFAT_Device_Write_nSector(temp,SOC(start_clu),app_Buffer);
    iClu=(temp*(pInit_Args->BytesPerSector)); //借用临时变量，减少计算量
    app_Buffer+=iClu;len-=iClu;
    (pfi->File_CurOffset)+=iClu;

    znFAT_Device_Write_nSector((temp1/(pInit_Args->BytesPerSector)),SOC(temp2),app_Buffer);
    iClu=((temp1/(pInit_Args->BytesPerSector))*(pInit_Args->BytesPerSector)); //借用临时变量，减少计算量
    app_Buffer+=iClu;len-=iClu;
    (pfi->File_CurOffset)+=iClu;
   }

   (pfi->File_CurClust)=temp2;
   (pfi->File_CurPos)=0;
   (pfi->File_CurSec)=SOC(temp2)+(temp1/(pInit_Args->BytesPerSector));
  }
 }
 else
 {
	temp=len/(pInit_Args->BytesPerSector);
	znFAT_Device_Write_nSector(temp,(pfi->File_CurSec),app_Buffer);
	app_Buffer+=(temp*(pInit_Args->BytesPerSector));
	len-=(temp*(pInit_Args->BytesPerSector));
	(pfi->File_CurOffset)+=(temp*(pInit_Args->BytesPerSector));
	 
	(pfi->File_CurSec)+=temp;
 }

 if(len==0) //如果要修改的数据已无，即数据正好写到了扇区末尾
 {
  return temp_len;
 }
 else //如果还有数据要修改，最后的不足扇区部分
 {
  //如果启用了EXB缓冲，则此不足扇区的数据，可能并不在物理扇区中，而在EXB缓冲中
  #ifdef USE_EXCHANGE_BUFFER //如果使用了EXB缓冲
  #ifndef USE_ALONE_EXB //如果没有启用独立EXB缓冲，即启用了共享EXB缓冲
  if((psexb_cur_oc==pfi)&&(sexb_cur_dev==Dev_No)&&(sexb_cur_sec==(pfi->File_CurSec))) //如果此时文件正占用共享EXB，同时当前文件所在的设备与扇区和共享EXB相符
  {
   Memory_Copy(pexb_buf+(pfi->File_CurPos),app_Buffer,len); //修改在共享EXB中的数据
  }
  else //否则要修改的数据不在EXB中，而在物理扇区中
  {
   znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
   Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
   znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中
  }
  #else //如果启动独立EXB缓冲
  if((pfi->exb_cursec)==(pfi->File_CurSec)) //如果此文件的EXB对应扇区与文件当前扇区一致，则说明要修改的数据在此文件的EXB中
  {
   Memory_Copy((pfi->exb_buf)+(pfi->File_CurPos),app_Buffer,len); //修改在文件独立EXB中的数据
  }
  else //如果此文件EXB对应扇区与文件当前扇区不一致，要修改的数据在物理扇区中
  {
   znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
   Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
   znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中	 
  }
   
  #endif
  #else //如果没有使用EXB缓冲，则直接修改物理扇区数据
	
  znFAT_Device_Read_Sector((pfi->File_CurSec),znFAT_Buffer); //将文件当前扇区读出到内部缓冲，以备修改
  Memory_Copy(znFAT_Buffer+(pfi->File_CurPos),app_Buffer,len); //修改在内部缓冲中的数据
  znFAT_Device_Write_Sector((pfi->File_CurSec),znFAT_Buffer); //将数据回写到扇区中
   
  #endif

  (pfi->File_CurOffset)+=len;
  (pfi->File_CurPos)=len;	
 
  return temp_len;
 }
}
#endif

/************************************************************************************
 功能：关闭文件
 形参：pfi:指向文件信息集合的指针
 返回：0
 详解：此函数用于关闭文件，即对文件的信息集合进行清0，使文件的相关信息得以销毁，从而
       无法再对其进行操作，除非重新打开此文件。与此同时，此函数还依照RT_UPDATE_FILESIZE
       的定义对文件大小进行更新。如果没有开启文件大小的实时更新功能，则在调用此函数时
       将对其进行更新。因此如果没有使用实时文件大小更新，那么在文件操作，尤其是数据写入
       或修改操作之后，一定要调用close_file函数。
*************************************************************************************/
#ifdef ZNFAT_CLOSE_FILE
UINT8 znFAT_Close_File(struct FileInfo *pfi)
{
 #ifndef RT_UPDATE_FILESIZE
 Update_File_Size(pfi); //更文件目录项中的文件大小字段
 #endif

 just_file=pfi;

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 #ifdef USE_ALONE_CCCB
 CCCB_To_Alone();
 #endif
 #endif

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 #ifndef USE_ALONE_CCCB
 if(pfi==pcccb_cur_oc) //如果现在操作的这个文件正占用CCCB，则在关闭文件以前，需要将CCCB更新到FAT
 #endif
  CCCB_Update_FAT();
 #endif

 #ifndef RT_UPDATE_CLUSTER_CHAIN
 #ifdef USE_ALONE_CCCB
 pcccb_buf=(UINT32 *)0;
 pcccb_curval=(UINT32 *)0;
 pcccb_counter=(UINT8 *)0;
 #endif
 #endif

 #ifdef USE_EXCHANGE_BUFFER
 #ifndef USE_ALONE_EXB
 if(pfi==psexb_cur_oc && 0!=(sexb_cur_sec)) //如果当前EXB被占用，而且是现在要关闭的文件
 {
  znFAT_Device_Write_Sector(sexb_cur_sec,pexb_buf); //EXB缓冲数据回写
  sexb_cur_sec=0;
  psexb_cur_oc=(struct FileInfo *)0;
  sexb_cur_dev=(UINT8)(-1);
 }
 #else
 pexb_buf=pfi->exb_buf;
 if(0!=pfi->exb_cursec) //如果要关闭文件的EXB被占用
 {
  znFAT_Device_Write_Sector(pfi->File_CurSec,pexb_buf); //EXB缓冲数据回写
 }
 pexb_buf=(UINT8 *)0;
 #endif
 #endif

 Memory_Set((UINT8 *)pfi,(UINT32)sizeof(struct FileInfo),0);

 just_file=(struct FileInfo *)0;

 return ERR_SUCC;
}
#endif

/************************************************************************************
 功能：刷新文件系统
 形参：无
 返回：0
 详解：在我们进行了文件操作之后，比如创建文件或目录，向文件中写入数据，或是删除操作，
       都会影响到文件系统本身的一些参数，比如磁盘的剩余容量等。znFAT中可以通过打开
       RT_UPDATE_FSINFO这个宏来启动文件系统实时更新功能，任何的增加删改操作znFAT都会
       立即更新文件系统。但这样会被数据写入等操作效率降低，因此可以屏蔽掉这一实时更新
       功能，但是必须在所有文件操作结束之后，调用此函数来刷新文件系统。
*************************************************************************************/
#ifdef ZNFAT_FLUSH_FS
UINT8 znFAT_Flush_FS(void)
{
 #ifndef RT_UPDATE_FSINFO
 Update_FSINFO(); //更新文件系统信息
 #endif

 return ERR_SUCC; 
}
#endif
                                       
