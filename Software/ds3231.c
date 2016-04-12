/******************************************************************************
* @ File name --> ds3231.c
* @ Author    --> By@ Sam Chan
* @ Version   --> V1.0
* @ Date      --> 02 - 01 - 2014
* @ Brief     --> 高精度始终芯片DS3231驱动函数
*
* @ Copyright (C) 20**
* @ All rights reserved
*******************************************************************************
*
*                                  File Update
* @ Version   --> V1.
* @ Author    -->
* @ Date      -->
* @ Revise    -->
*
******************************************************************************/

#include "ds3231.h"

/******************************************************************************
                               定义显示时间格式
                         要改变显示的格式请修改此数组
******************************************************************************/

u8 Display_Time[8] = {0x30,0x30,0x3a,0x30,0x30,0x3a,0x30,0x30};	
					//时间显示缓存   格式  00:00:00

u8 Display_Date[13] = {0x32,0x30,0x31,0x33,0x2f,0x31,0x30,0x2f,0x32,0x30,0x20,0x37,0x57};
					//日期显示缓存   格式  2013/10/20 7W

/******************************************************************************
                               定义相关的变量函数
******************************************************************************/

//Time_Typedef TimeValue;	//定义时间数据指针
u8 TimeValue[7];

u8 Time_Buffer[7];	//时间日历数据缓存

void IIC_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
  
  GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8 | GPIO_Pin_9;   //选择PB6和PB7引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;           //开启PB6和PB7的复用功能
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    //PB6和PB7设置为开漏输出
  GPIO_InitStructure.GPIO_PuPd =GPIO_PuPd_NOPULL;   //PB6和PB7口不带上拉电阻
  GPIO_Init(GPIOB, &GPIO_InitStructure);

}

void SDA_OUT(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  GPIO_InitStructure.GPIO_Pin= GPIO_Pin_9;   
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   
  GPIO_InitStructure.GPIO_PuPd =GPIO_PuPd_NOPULL; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void SDA_IN(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  GPIO_InitStructure.GPIO_Pin= GPIO_Pin_9;   
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_InitStructure.GPIO_PuPd =GPIO_PuPd_UP; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT();     //sda线输出
	IIC_SDA_H();
	Delay_Us(1);	  	  
	IIC_SCL_H();
	Delay_Us(5);
 	IIC_SDA_L();//START:when CLK is high,DATA change form high to low 
	Delay_Us(5);
	IIC_SCL_L();//钳住I2C总线，准备发送或接收数据 
	Delay_Us(2);
}	  
//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT();//sda线输出
	IIC_SCL_L();
	IIC_SDA_L();//STOP:when CLK is high DATA change form low to high
 	Delay_Us(4);
	IIC_SCL_H(); 
	Delay_Us(5);
	IIC_SDA_H();//发送I2C总线结束信号
	Delay_Us(4);							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 IIC_Wait_Ack(void)
{
	u16 ucErrTime=0;
	SDA_IN();      //SDA设置为输入  
	IIC_SDA_H();Delay_Us(1);	   
	IIC_SCL_H();Delay_Us(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>500)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL_L();//时钟输出0 	   
	return 0;  
} 
//产生ACK应答
void IIC_SAck(void)
{
	IIC_SCL_L();
	SDA_OUT();
	IIC_SDA_L();
	Delay_Us(2);
	IIC_SCL_H();
	Delay_Us(2);
	IIC_SCL_L();
}
//不产生ACK应答		    
void IIC_NAck(void)
{
	IIC_SCL_L();
	SDA_OUT();
	IIC_SDA_H();
	Delay_Us(2);
	IIC_SCL_H();
	Delay_Us(2);
	IIC_SCL_L();
}					 				     
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
u8 IIC_Write_Byte(u8 txd)
{                        
    u8 t;   
    
    SDA_OUT(); 	    
    IIC_SCL_L();//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
        //IIC_SDA=(txd&0x80)>>7;
		if((txd&0x80)>>7)
			IIC_SDA_H();
		else
			IIC_SDA_L();
		txd<<=1; 	  
		Delay_Us(2);   //对TEA5767这三个延时都是必须的
		IIC_SCL_H();
		Delay_Us(2); 
		IIC_SCL_L();	
		Delay_Us(2);
    }	 

    t = IIC_Wait_Ack();
    
    Delay_Us(5);
    
    return t;
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
u8 IIC_Read_Byte()
{
	unsigned char i,receive=0;
	SDA_IN();//SDA设置为输入
    for(i=0;i<8;i++ )
	{
        IIC_SCL_L(); 
        Delay_Us(2);
		IIC_SCL_H();
        receive<<=1;
        if(READ_SDA)receive++;   
		Delay_Us(1); 
    }					 
    /*if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK   */
    return receive;
}

void IIC_Ack(unsigned char ack)
{
  if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_SAck(); //发送ACK   
}

/******************************************************************************
* Function Name --> DS3231某寄存器写入一个字节数据
* Description   --> none
* Input         --> REG_ADD：要操作寄存器地址
*                   dat：要写入的数据
* Output        --> none
* Reaturn       --> none 
******************************************************************************/
void DS3231_Write_Byte(u8 REG_ADD,u8 dat)
{
	IIC_Start();
        Delay_Us(5);
	if(!(IIC_Write_Byte(DS3231_Write_ADD)))	//发送写命令并检查应答位
	{
		IIC_Write_Byte(REG_ADD);
		IIC_Write_Byte(dat);	//发送数据
	}
	IIC_Stop();
        
        //Delay_Us(500);
}
/******************************************************************************
* Function Name --> DS3231某寄存器读取一个字节数据
* Description   --> none
* Input         --> REG_ADD：要操作寄存器地址
* Output        --> none
* Reaturn       --> 读取到的寄存器的数值 
******************************************************************************/
u8 DS3231_Read_Byte(u8 REG_ADD)
{
	u8 ReData;
        
	IIC_Start();
	if(!(IIC_Write_Byte(DS3231_Write_ADD)))	//发送写命令并检查应答位
	{
		IIC_Write_Byte(REG_ADD);	//确定要操作的寄存器
		IIC_Start();	//重启总线
		IIC_Write_Byte(DS3231_Read_ADD);	//发送读取命令
		ReData = IIC_Read_Byte();	//读取数据
		IIC_Ack(1);	//发送非应答信号结束数据传送
	}
	IIC_Stop();
        
        //Delay_Us(500);
        
	return ReData;
}
/******************************************************************************
* Function Name --> DS3231对时间日历寄存器操作，写入数据或者读取数据
* Description   --> 连续写入n字节或者连续读取n字节数据
* Input         --> REG_ADD：要操作寄存器起始地址
*                   *WBuff：写入数据缓存
*                   num：写入数据数量
*                   mode：操作模式。0：写入数据操作。1：读取数据操作
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void DS3231_Operate_Register(u8 REG_ADD,u8 *pBuff,u8 num,u8 mode)
{
	u8 i;
	if(mode)	//读取数据
	{
		IIC_Start();
		if(!(IIC_Write_Byte(DS3231_Write_ADD)))	//发送写命令并检查应答位
		{
			IIC_Write_Byte(REG_ADD);	//定位起始寄存器地址
			IIC_Start();	//重启总线
			IIC_Write_Byte(DS3231_Read_ADD);	//发送读取命令
			for(i = 0;i < num;i++)
			{
				*pBuff = IIC_Read_Byte();	//读取数据
				if(i == (num - 1))	IIC_Ack(1);	//发送非应答信号
				else IIC_Ack(0);	//发送应答信号
				pBuff++;
			}
		}
		IIC_Stop();	
	}
	else	//写入数据
	{		 	
		IIC_Start();
		if(!(IIC_Write_Byte(DS3231_Write_ADD)))	//发送写命令并检查应答位
		{
			IIC_Write_Byte(REG_ADD);	//定位起始寄存器地址
			for(i = 0;i < num;i++)
			{
				IIC_Write_Byte(*pBuff);	//写入数据
				pBuff++;
			}
		}
		IIC_Stop();
	}
}
/******************************************************************************
* Function Name --> DS3231读取或者写入时间信息
* Description   --> 连续写入n字节或者连续读取n字节数据
* Input         --> *pBuff：写入数据缓存
*                   mode：操作模式。0：写入数据操作。1：读取数据操作
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void DS3231_ReadWrite_Time(u8 mode)
{
	u8 Time_Register[8];	//定义时间缓存
	
	if(mode)	//读取时间信息
	{
		//DS3231_Operate_Register(Address_second,Time_Register,7,1);	//从秒地址（0x00）开始读取时间日历数据
                Time_Register[6] = DS3231_Read_Byte(0x06);
                Time_Register[5] = DS3231_Read_Byte(0x05);
                Time_Register[4] = DS3231_Read_Byte(0x04);
                //Time_Register[3] = DS3231_Read_Byte(0x03);
                Time_Register[2] = DS3231_Read_Byte(0x02);
                Time_Register[1] = DS3231_Read_Byte(0x01);
                Time_Register[0] = DS3231_Read_Byte(0x00);
          
          
		/******将数据复制到时间结构体中，方便后面程序调用******/
		TimeValue[0] = Time_Register[0] & Shield_secondBit;	//秒数据
		TimeValue[1] = Time_Register[1] & Shield_minuteBit;	//分钟数据
		TimeValue[2] = Time_Register[2] & Shield_hourBit;	//小时数据
		TimeValue[3] = Time_Register[3] & Shield_weekBit;	//星期数据
		TimeValue[4] = Time_Register[4] & Shield_dateBit;	//日数据
		TimeValue[5] = Time_Register[5] & Shield_monthBit;	//月数据
		TimeValue[6] = Time_Register[6];	//年数据
	}
	else
	{
		/******从时间结构体中复制数据进来******/
		Time_Register[0] = TimeValue[0];	//秒
		Time_Register[1] = TimeValue[1];	//分钟
		Time_Register[2] = TimeValue[2] | Hour_Mode24;	//小时
		Time_Register[3] = TimeValue[3];	//星期
		Time_Register[4] = TimeValue[4];	//日
		Time_Register[5] = TimeValue[5];	//月
		Time_Register[6] = TimeValue[6];	//年
		
		//DS3231_Operate_Register(Address_second,Time_Register,7,0);	//从秒地址（0x00）开始写入时间日历数据
                DS3231_Write_Byte(0x06, Time_Register[6]);
                DS3231_Write_Byte(0x05, Time_Register[5]);
                DS3231_Write_Byte(0x04, Time_Register[4]);
                //DS3231_Write_Byte(0x03, Time_Register[3]);
                DS3231_Write_Byte(0x02, Time_Register[2]);
                DS3231_Write_Byte(0x01, Time_Register[1]);
                DS3231_Write_Byte(0x00, Time_Register[0]);
	}
}
/******************************************************************************
* Function Name --> 时间日历初始化
* Description   --> none
* Input         --> *TimeVAL：RTC芯片寄存器值指针
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void DS3231_Init(void)
{	
	//时间日历数据
	/*Time_Buffer[0] = TimeVAL->second;
	Time_Buffer[1] = TimeVAL->minute;
	Time_Buffer[2] = TimeVAL->hour;
	Time_Buffer[3] = TimeVAL->week;
	Time_Buffer[4] = TimeVAL->date;
	Time_Buffer[5] = TimeVAL->month;
	Time_Buffer[6] = TimeVAL->year;
	
	DS3231_Operate_Register(Address_second,Time_Buffer,7,0);	//从秒（0x00）开始写入7组数据*/
	DS3231_Write_Byte(Address_control, OSC_Enable);
	DS3231_Write_Byte(Address_control_status, Clear_OSF_Flag);
}
/******************************************************************************
* Function Name --> DS3231检测函数
* Description   --> 将读取到的时间日期信息转换成ASCII后保存到时间格式数组中
* Input         --> none
* Output        --> none
* Reaturn       --> 0: 正常
*                   1: 不正常或者需要初始化时间信息
******************************************************************************/
u8 DS3231_Check(void)
{
	if(DS3231_Read_Byte(Address_control_status) & 0x80)  //晶振停止工作了
	{
		return 1;  //异常
	}
	else if(DS3231_Read_Byte(Address_control) & 0x80)  //或者 EOSC被禁止了
	{
		return 1;  //异常
	}
	else	return 0;  //正常
}
/******************************************************************************
* Function Name --> 时间日历数据处理函数
* Description   --> 将读取到的时间日期信息转换成ASCII后保存到时间格式数组中
* Input         --> none
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void Time_Handle(void)
{
	/******************************************************
	                   读取时间日期信息
	******************************************************/
	
	//DS3231_ReadWrite_Time(1);	//获取时间日历数据
	
	/******************************************************
	            时间信息转换为ASCII码可视字符
	******************************************************/
	
	Display_Time[6] = (TimeValue[0] >> 4) + 0x30;
	Display_Time[7] = (TimeValue[0] & 0x0f) + 0x30;	//Second

	Display_Time[3] = (TimeValue[1] >> 4) + 0x30;
	Display_Time[4] = (TimeValue[1] & 0x0f) + 0x30;	//Minute

	Display_Time[0] = (TimeValue[2] >> 4) + 0x30;
	Display_Time[1] = (TimeValue[2] & 0x0f) + 0x30;	//Hour 

	Display_Date[8] = (TimeValue[4] >> 4) + 0x30;
	Display_Date[9] = (TimeValue[4] & 0x0f) + 0x30;	//Date

	Display_Date[5] = (TimeValue[5] >> 4) + 0x30;
	Display_Date[6] = (TimeValue[5] & 0x0f) + 0x30;	//Month

	//Display_Date[0] = (u8)(TimeValue.year >> 12) + 0x30;
	//Display_Date[1] = (u8)((TimeValue.year & 0x0f00) >> 8) + 0x30;
	Display_Date[2] = (TimeValue[6] >> 4) + 0x30;
	Display_Date[3] = (TimeValue[6] & 0x0f) + 0x30;	//Year

	Display_Date[11] = (TimeValue[3] & 0x0f) + 0x30;	//week

}
/******************************************************************************
* Function Name --> 读取芯片温度寄存器
* Description   --> 温度寄存器地址为0x11和0x12，这两寄存器为只读
* Input         --> none
* Output        --> *Temp：最终温度显示字符缓存
* Reaturn       --> none
******************************************************************************/
void DS3231_Read_Temp(u8 *Temp)
{
	u8 temph,templ;
	float temp_dec;

	temph = DS3231_Read_Byte(Address_temp_MSB);	//读取温度高8bits
	templ = DS3231_Read_Byte(Address_temp_LSB) >> 6;	//读取温度低2bits

	//温度值转换
	if(temph & 0x80)	//判断温度值的正负
	{	//负温度值
		temph = ~temph;	//高位取反
		templ = ~templ + 0x01;	//低位取反加1
		Temp[0] = 0x2d;	//显示“-”
	}
	else	Temp[0] = 0x20;	//正温度不显示符号，显示正号填0x2b

	//小数部分计算处理
	temp_dec = (float)templ * (float)0.25;	//0.25℃分辨率

	//整数部分计算处理
	temph = temph & 0x70;	//去掉符号位
	Temp[1] = temph % 1000 / 100 + 0x30;	//百位
	Temp[2] = temph % 100 / 10 + 0x30;	//十位
	Temp[3] = temph % 10 + 0x30;	//个位
	Temp[4] = 0x2e;	//.

	//小数部分处理
	Temp[5] = (u8)(temp_dec * 10) + 0x30;	//小数点后一位
	Temp[6] = (u8)(temp_dec * 100) % 10 + 0x30;	//小数点后二位

	if(Temp[1] == 0x30)	Temp[1] = 0x20;	//百位为0时不显示
	if(Temp[2] == 0x30)	Temp[2] = 0x20;	//十位为0时不显示
}

