/******************** (C) COPYRIGHT 2012 WildFire Team **************************
 * 文件名  ：sd_fs_app.c
 * 描述    ：MicroSD卡文件系统应用函数库。
 *              
 * 实验平台：野火STM32开发板
 * 硬件连接：无
 *
 * 库版本  ：ST3.5.0
 *
 * 作者    ：wildfire team 
 * 论坛    ：http://www.amobbs.com/forum-1008-1.html
 * 淘宝    ：http://firestm32.taobao.com
**********************************************************************************/
#include "sd_fs_app.h"
#include <stdio.h>

FATFS myfs[2];                 // Work area (file system object) for logical drive
FIL myfsrc, myfdst;            // file objects
FRESULT myres;                 // FatFs function common result code
BYTE mybuffer[512];            // file copy buffer
BYTE my_latest_buffer[512];
uint8_t mystring[512]="this is a MicroSD demo base on fatfs";
UINT mybr, mybw;               // File R/W count
int mya = 0;
char mypath[512]="0:";         // 一定要初始化为0:

/*
 * 函数名：NVIC_Configuration
 * 描述  ：SDIO 中断通道配置
 * 输入  ：无
 * 输出  ：无
 */
void SDIO_NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    
    NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


void sd_fs_init(void)
{
    /* SD卡中断初始化 */
		SDIO_NVIC_Configuration();
		
		/* SD 卡硬件初始化，初始化盘符为0 */	 
   f_mount(0,&myfs[0]);	       
}

/*******************************************************************************
* Function Name  : Sdfs_new
* Description    : 兴建一个文件并写入数据  
* Input          : new_file_name--兴建文件的名称  
*				   write_buffer--写入文件的数据缓冲区地址  
*				   buffer_size--缓冲区大小
* Output         : None
* Return         : 0(success)  1(file existed )  -1(fail)
* Attention		 : None
*******************************************************************************/           
int Sdfs_new(BYTE *new_file_name, BYTE *write_buffer, BYTE buffer_size)
{
    BYTE name_buffer[50];
    sprintf((char*)name_buffer,"0:%s",new_file_name);
    
    f_mount(0, &myfs[0]);
    myres = f_open( &myfsrc , (char*)name_buffer , FA_CREATE_NEW | FA_WRITE);
    
    if ( myres == FR_OK ) 
    { 
        
        myres = f_write(&myfsrc, write_buffer,buffer_size, &mybr); //写入数据   
        f_close(&myfsrc);
        
        return 0;      
    }
    
    else if ( myres == FR_EXIST )  //如果文件已经存在
    {
        return FR_EXIST;	 
    }
    
    else
    {
        return -1;
    }
    
}


/*******************************************************************************
* Function Name  : Sdfs_write
* Description    : 对文件写入数据  
* Input          : new_file_name--下入数据文件的名称  
*				   write_buffer--写入文件的数据缓冲区地址  
*				   buffer_size--缓冲区大小
* Output         : None
* Return         : 0(success)   -1(fail)
* Attention		 : None
*******************************************************************************/            
int Sdfs_write(BYTE *write_file_name, BYTE *write_buffer, BYTE buffer_size)
{
    BYTE name_buffer[50];
    sprintf((char*)name_buffer,"0:%s",write_file_name);
    
    f_mount(0, &myfs[0]);
    myres = f_open( &myfsrc , (char*)name_buffer ,FA_WRITE);
    
    if ( myres == FR_OK )  
    { 
        /* Write buffer to file */	
        myres = f_write(&myfsrc, write_buffer,buffer_size, &mybr); //写入数据   
        f_close(&myfsrc);
        
        return 0;      
    }
    else 
	if(myres == FR_NO_FILE)	 //如果没有该文件
	{
        return FR_NO_FILE;
    } 
    else	
	return -1;
}



/*******************************************************************************
* Function Name  : Sdfs_read
* Description    : 从一个文件读出数据到缓冲区 
* Input          : read_file_name--文件的名称  
*				   				save_buffer--数据需要保存的地址 
* Output         : None
* Return         : 0(success)  -1(fail)
* Attention		 : None
*******************************************************************************/ 
int Sdfs_read(BYTE *read_file_name, BYTE *save_buffer)
{
    
    int count=0;
    BYTE name_buffer[50];
    sprintf((char*)name_buffer,"0:%s",read_file_name);
    sd_fs_init();
    f_mount(0, &myfs[0]);
    myres = f_open(&myfsrc , (char*)name_buffer , FA_OPEN_EXISTING | FA_READ);
    
    if ( myres == FR_OK ) 
    { 
        for (;;) 
        {
            
            for ( mya=0; mya<512; mya++ ) 	/* 清缓冲区 */
                mybuffer[mya]=0;
            
            myres = f_read( &myfsrc, mybuffer, sizeof(mybuffer), &mybr ); /* 将文件里面的内容以512字节为单位读到本地缓冲区 */
            sprintf((char*)&save_buffer[count*512],"%s",mybuffer);					//打印到用户指定的缓冲区缓冲区
            
            count++;							
            if (myres || mybr == 0) break;   // error or eof        	    	
        }  	
        
        return 0;
    }
    
    else
        return -1;
    
    
}




/*******************************************************************************
* Function Name  : GetGBKCode_from_sd
* Description    : 从SD卡字库中读取自摸数据到指定的缓冲区 
* Input          : pBuffer---数据保存地址  
*				   					c--汉字字符低字节码 
* Output         : None
* Return         : 0(success)  -1(fail)
* Attention		 	 : None
*******************************************************************************/ 
int GetGBKCode_from_sd(unsigned char* pBuffer,const unsigned char * c)
{ 
    unsigned char High8bit,Low8bit;
    unsigned int pos;
    High8bit=*c;     /* 取高8位数据 */
    Low8bit=*(c+1);  /* 取低8位数据 */
    
    pos = ((High8bit-0xb0)*94+Low8bit-0xa1)*24 ;	
    f_mount(0, &myfs[0]);
    myres = f_open(&myfsrc , "0:/HZLIB.bin", FA_OPEN_EXISTING | FA_READ);
    
    if ( myres == FR_OK ) 
    {
        f_lseek (&myfsrc, pos);														 //指针偏移
        myres = f_read( &myfsrc, pBuffer, 24, &mybr );		 //16*16大小的汉字 其字模 占用16*2个字节
        
        f_close(&myfsrc);
        
        return 0;  
    }
    
    else
        return -1;    
}
/******************* (C) COPYRIGHT 2012 WildFire Team *****END OF FILE************/


