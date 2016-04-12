#include "ui.h"
#include "epd.h"
#include "key.h"
#include "ds3231.h"
#include "znfat.h"

extern const unsigned char gImage_bg[212000];

unsigned int fcount,fsel;

const int color_bow = 1;
const int color_wob = 0;

u8 BCD2HEX(u8 val)
{
    u8 i;
    i= val&0x0f;
    val >>= 4;
    val &= 0x0f;
    val *= 10;
    i += val;
    
    return i;
}

u8 HEX2BCD(u8 val)
{
  u8 i,j,k;
  i=val/10;
  j=val%10;
  k=j+(i<<4);
  return k;
}


const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int calc_first(int year, int month, int day) {
        if(month==1||month==2)//判断month是否为1或2　
        {
            year--;
            month+=12;
        }
        int c = year / 100;
        int y = year - c * 100;
        int week = c / 4 - 2 * c + y + y / 4 + 13 * (month + 1) / 5 + day - 1;
        while(week < 0)
            week += 7;
        return week %= 7;
}

void UI_Setting_DispDateTime(uint8_t highlight)
{
  Time_Handle();
  EPD_PutChar_24(270,100,Display_Date[2],((highlight==1)?0:3));
  EPD_PutChar_24(282,100,Display_Date[3],((highlight==1)?0:3));
  EPD_PutChar_24(306,100,Display_Date[4],3);
  EPD_PutChar_24(330,100,Display_Date[5],((highlight==2)?0:3));
  EPD_PutChar_24(342,100,Display_Date[6],((highlight==2)?0:3));
  EPD_PutChar_24(366,100,Display_Date[7],3);
  EPD_PutChar_24(390,100,Display_Date[8],((highlight==3)?0:3));
  EPD_PutChar_24(402,100,Display_Date[9],((highlight==3)?0:3));
  
  EPD_PutChar_24(270,124,Display_Time[0],((highlight==4)?0:3));
  EPD_PutChar_24(282,124,Display_Time[1],((highlight==4)?0:3));
  EPD_PutChar_24(306,124,Display_Time[2],3);
  EPD_PutChar_24(330,124,Display_Time[3],((highlight==5)?0:3));
  EPD_PutChar_24(342,124,Display_Time[4],((highlight==5)?0:3));
  EPD_PutChar_24(366,124,Display_Time[5],3);
  EPD_PutChar_24(390,124,Display_Time[6],((highlight==6)?0:3));
  EPD_PutChar_24(402,124,Display_Time[7],((highlight==6)?0:3));
}

void UI_Setting_Main()
{
  unsigned char i,refresh;
  const unsigned char ValMax[7] = {0,99,12,31,23,59,59};
  const unsigned char ValMin[7] = {0,0,1,1,0,0,0};
  const unsigned char ValMap[7] = {0,6,5,4,2,1,0};
  
  EPD_Power_On();
  EPD_FastClear();
  EPD_Power_Off();
  
  EPD_ClearInfo(0x55);
  EPD_ClearBack();
  
  EPD_FastFillRect(0,570,800,600,0);
  EPD_FastFillRect(30,30,785,555,2);
  EPD_FastFillRect(15,15,770,540,0);
  
  EPD_Line(19,67,19,536,3);
  EPD_Line(20,67,20,536,3);
  EPD_Line(765,67,765,536,3);
  EPD_Line(766,67,766,536,3);
  
  EPD_Line(19,67,766,67,3);
  EPD_Line(19,68,766,68,3);
  EPD_Line(19,535,766,535,3);
  EPD_Line(19,536,766,536,3);
  
  EPD_Line(23,71,23,532,3);
  EPD_Line(24,71,24,532,3);
  EPD_Line(761,71,761,532,3);
  EPD_Line(762,71,762,532,3);
  
  EPD_Line(23,71,762,71,3);
  EPD_Line(23,72,762,72,3);
  EPD_Line(23,531,762,531,3);
  EPD_Line(23,532,762,532,3);
  
  EPD_String_24(30,573,"KEY1 : Back     KEY2 : Up      KEY3 : Down     KEY4 : Enter",3);
  EPD_String_24(236,15,"STANDARD CMOS SETUP UTILITY",3);
  EPD_String_24(151,39,"Copyleft 2016 www.ZephRay.com/EPDCalendar",3);
  
  EPD_String_24(54,100,"Date (yy/mm/dd) :",3);
  EPD_String_24(54,124,"Time (hh:mm:ss) :",3);
  EPD_String_24(54,172,"Background Update Interval : Per day",2);
  EPD_String_24(54,196,"Display layout : Default",2);
  
  EPD_Line(31,232,754,232,3);
  EPD_Line(31,233,754,233,3);
  
  EPD_String_24(54,244,"ST(R) STM32(tm) F407ZET6 ARM Cortex-M4F r1p0",2);
  EPD_String_24(54,268,"Core : 120MHz  AHB : 60MHz  APB : 30MHz",2);
  EPD_String_24(54,316,"    Base Memory :    128 KB",2);
  EPD_String_24(54,340,"     CCM Memory :     64 KB",2);
  EPD_String_24(54,364,"Extended Memory :    512 KB",2);
  EPD_String_24(54,388,"---------------------------",2);
  EPD_String_24(54,412,"   Total Memory :    704 KB",2);
  
  UI_Setting_DispDateTime(1);
  
  EPD_Power_On();
  EPD_DispFull();
  EPD_Power_Off();
  
  i=1;
  refresh = 0;
  
  while (i<7)
  {
    if (Key_Scan(0))  { if (i>1) { i--;  refresh = 1; } else { i=8;}}
    if (Key_Scan(1)&&(BCD2HEX(TimeValue[ValMap[i]])<ValMax[i])) { TimeValue[ValMap[i]] = HEX2BCD(BCD2HEX(TimeValue[ValMap[i]])+1); refresh = 1; }
    if (Key_Scan(2)&&(BCD2HEX(TimeValue[ValMap[i]])>ValMin[i])) { TimeValue[ValMap[i]] = HEX2BCD(BCD2HEX(TimeValue[ValMap[i]])-1); refresh = 1; }
    if (Key_Scan(3))  {   i++; refresh = 1; }
    if (refresh) {
      UI_Setting_DispDateTime(i);
      EPD_Power_On();
      EPD_DispFull();
      EPD_Power_Off();
      refresh = 0;
    }
  }
  if (i == 7)
    DS3231_ReadWrite_Time(0);//Write New Time
  else
    DS3231_ReadWrite_Time(1);//Read Old Time
}

void UI_DispCal()
{
  int DateNow, MonthNow, YearNow;
  char day[32];
  int offist;
  int x, y;
  
  YearNow = BCD2HEX(TimeValue[6]);
  MonthNow= BCD2HEX(TimeValue[5]);
  DateNow = BCD2HEX(TimeValue[4]);
  
  EPD_String_16(130,540,"SUN MON TUE WED THU FRI SAT SUN MON TUE WED THU FRI SAT SUN MON TUE WED THU FRI SAT", color_bow);

  int first = calc_first(YearNow, MonthNow, 1);
  x = first;
  y = 1;
  for (offist = 0; offist != days[MonthNow - 1]; offist++) {
    sprintf(day, "%2d", offist+1);
    if (DateNow - 1 == offist) {
      EPD_String_16(130 + 32 * x, 540 + 16 * y, day, color_wob);
    }
    else {
      EPD_String_16(130 + 32 * x, 540 + 16 * y, day, color_bow);
    }
    x++;
    if (x == 21) {
      x = 0;
      y++;
    }
  }
}

void UI_DispMain()
{
  EPD_PutChar_Legacy(5,540,Display_Time[0],3);
  EPD_PutChar_Legacy(29,540,Display_Time[1],3);
  EPD_PutChar_Legacy(53,540,Display_Time[2],3);
  EPD_PutChar_Legacy(77,540,Display_Time[3],3);
  EPD_PutChar_Legacy(101,540,Display_Time[4],3);
}

void UI_DispMainFull()
{
  DS3231_ReadWrite_Time(1);
  Time_Handle();
  EPD_ClearInfo(0);
  EPD_ClearBack();
  
  UI_DispCal();
  
  UI_DispMain();
  
  EPD_Power_On();
  EPD_FastClear();
  EPD_DispPic(EPD_BG);
  EPD_Power_Off();
  
  Delay_Us(1000000);
  
  EPD_Power_On();
  EPD_DispInfo();
  EPD_Power_Off();

}
               
void UI_PickBg(unsigned char first)
{
  struct znFAT_Init_Args Init_Args; //文件系统参数集合，用于记录文件系统的重要参数
  struct FileInfo FileInfo;	//文件参数集合
  unsigned char res=0;
  unsigned int i;
  
  znFAT_Device_Init(); //存储设备初始化
  Delay_Us(10000);
	
  znFAT_Select_Device(0,&Init_Args); //选择设备
  Delay_Us(10000);
  res=znFAT_Init(); //文件系统初始化
  Delay_Us(100000);
	
  if(!res) //文件系统初始化成功
  {
    fcount = 0;
    while(1) {
      res = znFAT_Open_File(&FileInfo,"\\*.ebm",fcount,1);
      Delay_Us(10000);
      if (res==0) {             
        fcount ++;
      }else
        break;
    }
    if (first)
      fsel = rand()%fcount;
    else
      if (fsel < (fcount-1)) fsel++; else fsel=0;
    res = znFAT_Open_File(&FileInfo,"\\*.ebm",fsel,1);
    Delay_Us(10000);
    res = znFAT_ReadData(&FileInfo,0,212000,EPD_BG); 
    Delay_Us(10000);
    znFAT_Flush_FS();
  }
  
  /*if (res)
    for (i=0;i<212000;i++)
      EPD_BG[i] = gImage_bg[i];*/
}

void TIM3_Init(u32 TIM_scale, u32 TIM_Period)//TIM_Period为16位的数
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  	NVIC_InitTypeDef  NVIC_InitStructure; 
	 
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);  
  	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);					
  	TIM_TimeBaseStructure.TIM_Period = TIM_Period;//计数器重装值
  	TIM_TimeBaseStructure.TIM_Prescaler = 0;
  	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  	TIM_PrescalerConfig(TIM3, (TIM_scale-1), TIM_PSCReloadMode_Immediate);
  	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
  	TIM_Cmd(TIM3, ENABLE);
}

void UI_Main()
{
  unsigned char lst;
  unsigned char nextf;
  unsigned int ct;
  
  DS3231_ReadWrite_Time(1);
  srand(TimeValue[0]);
  UI_PickBg(1);
  UI_DispMainFull();
  lst = 0xff;
  while (1)
  {
    DS3231_ReadWrite_Time(1);
    if (TimeValue[1] != lst)
    {
      Time_Handle();
      if (nextf > 0)
      {
        if (nextf == 1)
          UI_DispMainFull();
        nextf --;
      }
      if (TimeValue[2] == 0)
      {
        UI_PickBg(0);
        UI_DispMainFull();
        nextf = 2;
      }
      UI_DispMain();
      EPD_Power_On();
      EPD_DispInfo();
      EPD_Power_Off();
    }
    lst = TimeValue[1];
    if ((GPIOB->IDR & 0xF000)!=0xF000)
    {
      while ((GPIOB->IDR & 0xF000)!=0xF000);
      UI_Setting_Main();
      UI_PickBg(0);
      UI_DispMainFull();
      nextf = 2;
    }
    
    //LOWER POWER CONSUMPTION
    PM_SetCPUFreq(4);
    while (1) {
      if ((GPIOB->IDR & 0xF000)!=0xF000) break;
      Delay_Us(1000);//1ms->120ms
      if (ct==60) break;
      ct ++;
    }
    PM_SetCPUFreq(120);
    ct = 0;
    
  }
}