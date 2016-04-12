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

#ifndef _ds3231_h_
#define _ds3231_h_

/******************************************************************************
                                外部函数头文件                        
******************************************************************************/

#include "main.h"

//#include "iic.h"

#define IIC_SCL_L()     GPIOB->BSRRH = GPIO_Pin_8
#define IIC_SCL_H()     GPIOB->BSRRL = GPIO_Pin_8
#define IIC_SDA_L()     GPIOB->BSRRH = GPIO_Pin_9
#define IIC_SDA_H()     GPIOB->BSRRL = GPIO_Pin_9
#define READ_SDA        (GPIOB->IDR & GPIO_Pin_9)

/******************************************************************************
                         DS3231日历时钟寄存器结构定义                          
******************************************************************************/
/*typedef struct
{
	u16 year;	//年
	u8 month;	//月
	u8 week;	//星期
	u8 date;	//日
	u8 hour;	//小时
	u8 minute;	//分钟
	u8 second;	//秒钟
}Time_Typedef;*/

extern u8 TimeValue[7];
//extern Time_Typedef TimeValue;	//定义时间数据缓存
extern u8 Time_Buffer[7];	//时间日历数据缓存

extern u8 Display_Time[8];	//显示时间格式
extern u8 Display_Date[13];	//显示日期格式

/******************************************************************************
                                  参数宏定义                       
******************************************************************************/

//#define DS3231_YEARDATA                  (u16)0x2000	//年结构，如果是19xx年、21xx年的话请修改这个数值即可

#define DS3231_ADD_BASS					0xd0	//器件基地址

#define DS3231_Write_ADD				(DS3231_ADD_BASS | 0x00)	//写DS3231
#define DS3231_Read_ADD					(DS3231_ADD_BASS | 0x01)	//读DS3231

/******************************************************************************
                              参数寄存器地址宏定义                    
******************************************************************************/

#define Address_second					0x00	//秒
#define Address_minute					0x01	//分
#define Address_hour					0x02	//时
#define Address_week					0x03	//星期
#define Address_date					0x04	//日
#define Address_month					0x05	//月
#define Address_year					0x06	//年

#define Address_second_Alarm1			0x07	//秒闹铃
#define Address_minute_Alarm1			0x08	//分闹铃
#define Address_hour_Alarm1				0x09	//时闹铃
#define Address_week_Alarm1				0x0a	//日闹铃、星期闹铃

#define Address_minute_Alarm2			0x0b	//分闹铃
#define Address_hour_Alarm2				0x0c	//时闹铃
#define Address_week_Alarm2				0x0d	//日闹铃、星期闹铃


#define Address_control					0x0e	//控制
#define Address_control_status			0x0f	//控制和状态标志

#define Address_offset					0x10	//Aging Offset

#define Address_temp_MSB				0x11	//温度高8位
#define Address_temp_LSB				0x12	//温度低8位

/******************************************************************************
                             时间参数屏蔽无效位宏定义                 
******************************************************************************/

#define Shield_secondBit			0x7f
#define Shield_minuteBit			0x7f
#define Shield_hourBit				0x3f
#define Shield_weekBit				0x07
#define Shield_dateBit				0x3f
#define Shield_monthBit				0x1f

/******************************************************************************
                                 参数命令定义                      
******************************************************************************/

//小时寄存器
#define Hour_Mode12					(1<<6)	//12小时格式
#define Hour_Mode24					(0<<6)	//24小时格式

//秒闹铃寄存器1
#define Alarm_second_open			(1<<7)	//秒闹铃开
#define Alarm_second_close			(0<<7)	//秒闹铃关

//分闹铃寄存器1&2
#define Alarm_minute_open			(1<<7)	//分闹铃开
#define Alarm_minute_close			(0<<7)	//分闹铃关

//时闹铃寄存器1&2（可设置为12小时模式或者24小时模式）
#define Alarm_hour_open12			(3<<6)	//时闹铃开，12小时格式
#define Alarm_hour_close12			(1<<6)	//时闹铃关，24小时格式

#define Alarm_hour_open24			(2<<6)	//时闹铃开，12小时格式
#define Alarm_hour_close24			(0<<6)	//时闹铃关，24小时格式

//星期、日闹铃寄存器1&2（可选择星期闹铃或则日期闹铃）
#define Alarm_week_open				(3<<6)	//星期闹铃开
#define Alarm_week_close			(1<<6)	//星期闹铃关

#define Alarm_date_open				(2<<6)	//星期闹铃开
#define Alarm_date_close			(0<<6)	//星期闹铃关


//晶振控制寄存器
#define OSC_Enable					(0<<7)	//启动晶振
#define OSC_Disable					(1<<7)	//停止晶振

#define SET_BBSQW					(1<<6)	//Vbat pin
#define RESET_BBSQW					(0<<6)	//int/sqw高阻

#define Temp_CONV_SET				(1<<5)	//强制温度转换位数字码
#define Temp_CONV_Clear				(0<<5)

#define SQW_OUT1Hz					(0<<3)	//1Hz
#define SQW_OUT1024Hz				(1<<3)	//1.024KHz
#define SQW_OUT4096Hz				(2<<3)	//4.096KHz
#define SQW_OUT8192Hz				(3<<3)	//8.192KHz

#define OUTPUT_INTSQW				(0<<2)	//输出方波，上电该位置1（INTCN位）

#define A2IE_Enable					(1<<1)	//enable alarm 2
#define A2IE_Disable				(0<<1)	//disable alarm 2

#define A1IE_Enable					(1<<0)	//enable alarm 1
#define A1IE_Disable				(0<<0)	//disable alarm 1

//控制、状态寄存器
#define Clear_OSF_Flag				(0<<7)	//clear OSF flag

#define Enable_OSC32768				(1<<3)	//EN32KHz EN
#define Disable_OSC32768			(0<<3)	//EN32KHz高阻

#define Clear_A2IE_Flag				(0<<1)	//清除闹铃2中断标志
#define Clear_A1IE_Flag				(0<<0)	//清除闹铃2中断标志

/******************************************************************************
                                外部功能函数                      
******************************************************************************/

u8 DS3231_Check(void);	//DS3231检测函数

void DS3231_Write_Byte(u8 REG_ADD,u8 dat);	//DS3231某寄存器写入一个字节数据

u8 DS3231_Read_Byte(u8 REG_ADD);	//DS3231某寄存器读取一个字节数据

void DS3231_ReadWrite_Time(u8 mode);	//DS3231读取或者写入时间信息

void DS3231_Init(void);	//时间日历初始化

void Time_Handle(void);	//时间日历数据处理函数


void DS3231_Read_Temp(u8 *Temp);	//读取芯片温度寄存器


#endif

