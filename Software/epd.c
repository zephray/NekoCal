/*******************************************************************************
  EPD DRIVER FOR STM32F2/4 w/ FSMC
  By ZephRay(zephray@outlook.com)
*******************************************************************************/
#include "epd.h"

/*******************************************************************************



*******************************************************************************/

unsigned char EPD_FB[60000]; //1bpp Framebuffer

#ifdef __USE_FIXED_BG__
#include <bg.h>
#else
unsigned char EPD_BG[240000] @ 0x08020000; //fixed address of image
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
    
extern const unsigned char Font_Ascii_8X16E[];
extern const unsigned char Font_Ascii_24X48E[];
extern const unsigned char Font_Ascii_12X24E[];
//extern const unsigned char WQY_ASCII_24[];

#ifndef USE_H_SCREEN

//Init waveform, basiclly alternating between black and white
#define FRAME_INIT_LEN     33

const unsigned char wave_init[FRAME_INIT_LEN]=
{
  0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
  0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
  0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
  0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
  0,
};

//line delay for different shades of grey
//note that delay is accumulative
//it means that if it's the level 4 grey
//the total line delay is 90+90+90+90=360
//this could be used as a method of rough gamma correction
//these settings only affect 4bpp mode
const unsigned char timA[16] =
{
// 1  2  3  4  5  6  7  8  9 10 11  12  13  14  15
  90,90,90,90,90,90,90,90,90,90,120,120,120,120,200
};

#define timB 20

//this only affect 1bpp mode
#define clearCount 4
#define setCount 4

#else

#define FRAME_INIT_LEN     21

const unsigned char wave_init[FRAME_INIT_LEN]=
{
  0x55,0x55,0x55,0x55,0x55,
  0xaa,0xaa,0xaa,0xaa,0xaa,
  0x55,0x55,0x55,0x55,0x55,
  0xaa,0xaa,0xaa,0xaa,0xaa,
  0,
};

const unsigned char timA[16] =
{
// 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
  30,20,20,15,15,15,15,15,20,20,20,20,20,40,50
};

#define timB 40

#define clearCount 2
#define setCount 2

#endif


unsigned char g_dest_data[200];//Line data buffer

void DelayCycle(unsigned long x)
{
  while (x--)
  {
    asm("nop");
  }
}

//Us delay that is not accurate
void Delay_Us(unsigned long x)
{
  unsigned long a;
  
  while (x--)
  {
    a = 17;
    while (a--)
    {
      asm ("nop");
    }
  }
}

void EPD_GPIO_Init()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE);
  
  GPIOD->BSRRH = GPIO_Pin_4;//TURN OFF ALL VOLTAGES
  
  //Power controll
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  /*GPIO_Init(GPIOD,&GPIO_InitStructure);*/
  
  //Source&Gate Driver
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|
                                GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
  GPIO_Init(GPIOA,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4;
  GPIO_Init(GPIOE,&GPIO_InitStructure);
  
  //Source Data
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|
                                GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
  GPIO_Init(GPIOC,&GPIO_InitStructure);
}

void EPD_Power_Off(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  GPIOD->BSRRH = GPIO_Pin_4;
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOD,&GPIO_InitStructure);
}

void EPD_Init(void)
{
  unsigned long i;
  
  EPD_GPIO_Init();	
  
  EPD_SHR_L();
  EPD_GMODE1_H();
  EPD_GMODE2_H();
  EPD_XRL_H();

  EPD_Power_Off();

  EPD_LE_L();
  EPD_CL_L();
  EPD_OE_L();
  EPD_SPH_H();
  EPD_SPV_H();
  EPD_CKV_L();
  
  for (i=0;i<60000;i++)
  {
    EPD_FB[i]=0;
  }
}

void EPD_Send_Row_Data(u8 *pArray)  
{
  unsigned char i;
  unsigned short a;
  
  a = GPIOC->IDR & 0xFF00;
  
  EPD_LE_H(); 
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_LE_L();
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_OE_H();
  
  EPD_SPH_L();                                          
  
  for (i=0;i<200;i++)
  {
    GPIOC->ODR = pArray[i];
    EPD_CL_H();
    //DelayCycle(1);
    EPD_CL_L();
  }
  
  EPD_SPH_H();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_CKV_L();
  EPD_OE_L();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_CKV_H();     
}


void EPD_SkipRow(void)  
{
  unsigned char i;
  unsigned short a;
  
  a = GPIOC->IDR & 0xFF00;
  
  EPD_LE_H(); 
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_LE_L();
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_OE_H();
  
  EPD_SPH_L();                                          
  
  for (i=0;i<200;i++)
  {
    GPIOC->ODR = a;
    EPD_CL_H();
    //DelayCycle(1);
    EPD_CL_L();
  }
  
  EPD_SPH_H();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_CKV_L();
  EPD_OE_L();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_CKV_H(); 
}

void EPD_Send_Row_Data_Slow(u8 *pArray,unsigned char timingA,unsigned char timingB)  
{
  unsigned char i;
  unsigned short a;
  
  a = GPIOC->IDR & 0xFF00;
  
  EPD_OE_H();
  
  EPD_SPH_L();                                          
  
  for (i=0;i<200;i++)
  {
    GPIOC->ODR = pArray[i];
    EPD_CL_H();
    //DelayCycle(1);
    EPD_CL_L();
  }
  
  EPD_SPH_H();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_LE_H(); 
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_LE_L();
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
  
  EPD_CKV_H();
  
  DelayCycle(timingA);
  
  EPD_CKV_L();
  
  DelayCycle(timingB);
  
  EPD_OE_L();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();  
  
}

void EPD_Vclock_Quick(void)
{
  unsigned char i;
  
  for (i=0;i<2;i++)
  {
    EPD_CKV_L();
    DelayCycle(20);
    EPD_CKV_H();
    DelayCycle(20);
  }
}

void EPD_Start_Scan(void)
{ 
  EPD_SPV_H();

  EPD_Vclock_Quick();
 
  EPD_SPV_L();

  EPD_Vclock_Quick();
  
  EPD_SPV_H();
  
  EPD_Vclock_Quick();
}

void EPD_Power_On(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  GPIOD->BSRRL = GPIO_Pin_4;
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD,&GPIO_InitStructure);
}

void EPD_Clear(void)
{
  unsigned short line,frame,i;
  
  for(frame=0; frame<FRAME_INIT_LEN; frame++)			
  {
    EPD_Start_Scan();
    for(line=0; line<600; line++)
    {
      for(i=0;i<200;i++)  g_dest_data[i]=wave_init[frame];	
      EPD_Send_Row_Data( g_dest_data );
    }
    EPD_Send_Row_Data( g_dest_data );			
  }
}

void EPD_EncodeLine_Pic(u8 *new_pic, u8 frame)//Encode data for grayscale image
{
	int i,j;
        unsigned char k,d;
	
        j=0;
	for(i=0; i<200; i++)
	{
          d = 0;
          k = new_pic[j++];
          if ((k&0x0F)>frame) d |= 0x10;
          if ((k>>4)>frame)   d |= 0x40;
          k = new_pic[j++];
          if ((k&0x0F)>frame) d |= 0x01;
          if ((k>>4)>frame)   d |= 0x04;
          g_dest_data[i] = d;	
	}	
}

void EPD_DispPic()//Display image in grayscale mode
{
  unsigned short frame;
  signed long line;
  unsigned long i;
  unsigned char *ptr;
  
  ptr = (unsigned char *)EPD_BG;
  
  for(frame=0; frame<15; frame++)					
  {
    EPD_Start_Scan();
    for (i=0;i<200;i++) g_dest_data[i]=0x00;
    for(line=0; line<70; line++)
    {
      EPD_Send_Row_Data_Slow(g_dest_data,timA[frame],timB);
    }
    for(line=(530-1); line>=0; line--)
    {
      EPD_EncodeLine_Pic(ptr + line*400, frame);		
      EPD_Send_Row_Data_Slow(g_dest_data,timA[frame],timB);		
    }	
    
    EPD_Send_Row_Data( g_dest_data );				
  }
}

//Encode data for monochrome image
//From white to image
void EPD_EncodeLine_To(u8 *new_pic)
{
	int i,j;
        unsigned char k,d;
	
        j=0;
	for(i=0; i<100; i++)
	{
          k = new_pic[i];
          d = 0;
          if (k&0x01) d |= 0x40;
          if (k&0x02) d |= 0x10;
          if (k&0x04) d |= 0x04;
          if (k&0x08) d |= 0x01;
          g_dest_data[j++] = d;
          d = 0;
          if (k&0x10) d |= 0x40;
          if (k&0x20) d |= 0x10;
          if (k&0x40) d |= 0x04;
          if (k&0x80) d |= 0x01;
          g_dest_data[j++] = d;
	}	
}

//Encode data for clearing a monochrome image
//From image to black
void EPD_EncodeLine_From(u8 *new_pic)
{
	int i,j;
        unsigned char k,d;
	
        j=0;
	for(i=0; i<100; i++)
	{
          k = ~new_pic[i];
          d = 0;
          if (k&0x01) d |= 0x40;
          if (k&0x02) d |= 0x10;
          if (k&0x04) d |= 0x04;
          if (k&0x08) d |= 0x01;
          g_dest_data[j++] = d;
          d = 0;
          if (k&0x10) d |= 0x40;
          if (k&0x20) d |= 0x10;
          if (k&0x40) d |= 0x04;
          if (k&0x80) d |= 0x01;
          g_dest_data[j++] = d;
	}	
}

//Display image in monochrome mode
void EPD_DispScr(unsigned int startLine, unsigned int lineCount)
{
  unsigned short frame;
  signed short line;
  unsigned long i;
  unsigned char *ptr;
  unsigned long skipBefore,skipAfter;
  
  ptr = EPD_FB;
  
  skipBefore = 600-startLine-lineCount;
  skipAfter = startLine;
  
  for(frame=0; frame<setCount; frame++)					
  {
    EPD_Start_Scan();
    for(line=0;line<skipBefore;line++)
    {
      EPD_EncodeLine_To(ptr);
      EPD_SkipRow();
    }
    for(line=(lineCount-1); line>=0; line--)
    {
      EPD_EncodeLine_To(ptr + (line+startLine)*100);
      EPD_Send_Row_Data( g_dest_data );
    }
    EPD_Send_Row_Data( g_dest_data );
    for(line=0;line<skipAfter;line++)
    {
      EPD_EncodeLine_To(ptr);
      EPD_SkipRow();
    }
    EPD_Send_Row_Data( g_dest_data );					
  }
  
}

//Clear image in monochrome mode
void EPD_ClearScr(unsigned int startLine, unsigned int lineCount)
{
  unsigned short frame;
  signed short line;
  unsigned long i;
  unsigned char *ptr;
  unsigned long skipBefore,skipAfter;
  
  ptr = EPD_FB;
  
  skipBefore = 600-startLine-lineCount;
  skipAfter = startLine;
  
  for(frame=0; frame<clearCount; frame++)					
  {
    EPD_Start_Scan();
    for(line=0;line<skipBefore;line++)
    {
      EPD_EncodeLine_From(ptr);
      EPD_SkipRow();
    }
    for(line=(lineCount-1); line>=0; line--)
    {
      EPD_EncodeLine_From(ptr + (line+startLine)*100);
      EPD_Send_Row_Data( g_dest_data );
    }
    EPD_Send_Row_Data( g_dest_data );
    for(line=0;line<skipAfter;line++)
    {
      EPD_EncodeLine_From(ptr);
      EPD_SkipRow();
    }
    EPD_Send_Row_Data( g_dest_data );					
  }
  
  for(frame=0; frame<4; frame++)					
  {
    EPD_Start_Scan();
    for(line=0;line<skipBefore;line++)
    {
      EPD_SkipRow();
    }
    for(line=(lineCount-1); line>=0; line--)
    {
      for(i=0;i<200;i++)  g_dest_data[i]=0xaa;
      EPD_Send_Row_Data( g_dest_data );
    }
    EPD_Send_Row_Data( g_dest_data );
    for(line=0;line<skipAfter;line++)
    {;
      EPD_SkipRow();
    }
    EPD_Send_Row_Data( g_dest_data );					
  }
}


void EPD_ClearFB(unsigned char c)
{
  unsigned long i;
  
  for (i=0;i<60000;i++)
    EPD_FB[i]=c;
}


void EPD_SetPixel(unsigned short x,unsigned short y,unsigned char color)
{
  unsigned short x_bit;
  unsigned long x_byte;  
  
  if ((x<800)&&(y<600))
  {
    x_byte=x/8+y*100;
    x_bit=x%8;             
  
    EPD_FB[x_byte] &= ~(1 << x_bit);
    EPD_FB[x_byte] |= (color << x_bit);
  }
}

void EPD_XLine(unsigned short x0,unsigned short y0,unsigned short x1,unsigned short color)
{
  unsigned short i,xx0,xx1;
  
  xx0=MIN(x0,x1);
  xx1=MAX(x0,x1);
  for (i=xx0;i<=xx1;i++)
  {
    EPD_SetPixel(i,y0,color);
  }
}

void EPD_YLine(unsigned short x0,unsigned short y0,unsigned short y1,unsigned short color)
{
  unsigned short i,yy0,yy1;
  
  yy0=MIN(y0,y1);
  yy1=MAX(y0,y1);
  for (i=yy0;i<=yy1;i++)
  {
    EPD_SetPixel(x0,yy1,color);
  }
}

void EPD_Line(unsigned short x0,unsigned short y0,unsigned short x1,unsigned short y1,unsigned short color)
{
  int temp;
  int dx,dy;               //定义起点到终点的横、纵坐标增加值
  int s1,s2,status,i;
  int Dx,Dy,sub;

  dx=x1-x0;
  if(dx>=0)                 //X的方向是增加的
    s1=1;
  else                     //X的方向是降低的
    s1=-1;     
  dy=y1-y0;                 //判断Y的方向是增加还是降到的
  if(dy>=0)
    s2=1;
  else
    s2=-1;

  Dx=abs(x1-x0);             //计算横、纵标志增加值的绝对值
  Dy=abs(y1-y0);
  if(Dy>Dx)                 //               
  {                     //以45度角为分界线，靠进Y轴是status=1,靠近X轴是status=0 
    temp=Dx;
    Dx=Dy;
    Dy=temp;
    status=1;
  } 
  else
    status=0;

/********判断垂直线和水平线********/
  if(dx==0)                   //横向上没有增量，画一条水平线
    EPD_YLine(x0,y0,y1,color);
  if(dy==0)                   //纵向上没有增量，画一条垂直线
    EPD_XLine(x0,y0,x1,color);


/*********Bresenham算法画任意两点间的直线********/ 
  sub=2*Dy-Dx;                 //第1次判断下个点的位置
  for(i=0;i<Dx;i++)
  { 
    EPD_SetPixel(x0,y0,color);           //画点 
    if(sub>=0)                               
    { 
      if(status==1)               //在靠近Y轴区，x值加1
        x0+=s1; 
      else                     //在靠近X轴区，y值加1               
        y0+=s2; 
      sub-=2*Dx;                 //判断下下个点的位置 
    } 
    if(status==1)
      y0+=s2; 
    else       
      x0+=s1; 
    sub+=2*Dy;   
  } 
}


void EPD_PutChar_16(unsigned short x, unsigned short y, unsigned short chr, unsigned char color)
{
  unsigned short x1,y1;
  unsigned short ptr;
  
  ptr=(chr-0x20)*16;
  for (y1=0;y1<16;y1++)
  {
    for (x1=0;x1<8;x1++)
    {
      if ((Font_Ascii_8X16E[ptr]>>x1)&0x01)
        EPD_SetPixel(x+x1,y+y1,color);
      else
        EPD_SetPixel(x+x1,y+y1,1-color);
    }
    ptr++;
  }
}

void EPD_PutChar_24(unsigned short x, unsigned short y, unsigned short chr, unsigned char color)
{
  unsigned short x1,y1,b;
  unsigned short ptr;
  
  ptr=(chr-0x20)*48;
  for (y1=0;y1<24;y1++)
  {
      for (b=0;b<8;b++)
        if ((Font_Ascii_12X24E[ptr]<<b)&0x80)
          EPD_SetPixel(x+b,y+y1,color); 
        else
          EPD_SetPixel(x+b,y+y1,1-color);
      ptr++; 
      for (b=0;b<4;b++)
        if ((Font_Ascii_12X24E[ptr]<<b)&0x80)
          EPD_SetPixel(x+8+b,y+y1,color); 
        else
          EPD_SetPixel(x+8+b,y+y1,1-color);
      ptr++; 
  }
}

void EPD_PutChar_48(unsigned short x, unsigned short y, unsigned short chr, unsigned char color)
{
  unsigned short x1,y1,b;
  unsigned short ptr;
  
  ptr=(chr-0x20)*144;
  for (y1=0;y1<48;y1++)
  {
    for (x1=0;x1<24;x1+=8)
    {
      for (b=0;b<8;b++)
        if ((Font_Ascii_24X48E[ptr]>>b)&0x01)
          EPD_SetPixel(x+x1+b,y+y1,color); 
        else
          EPD_SetPixel(x+x1+b,y+y1,1-color);
      ptr++;
    }  
  }
}

void EPD_String_16(unsigned short x,unsigned short y,unsigned char *s,unsigned char color)
{
  unsigned short x1;
  
  x1=0;
  while(*s)
  {
    if (*s<128)
    {
      EPD_PutChar_16(x+x1,y,*s++,color);
      x+=8;
    }
  }
}

void EPD_String_24(unsigned short x,unsigned short y,unsigned char *s,unsigned char color)
{
  unsigned short x1;
  
  x1=0;
  while(*s)
  {
    if (*s<128)
    {
      EPD_PutChar_24(x+x1,y,*s++,color);
      x+=12;
    }
  }
}

void EPD_FillRect(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2,unsigned char color)
{
  unsigned short x,y;
  for (x=x1;x<x2;x++)
    for (y=y1;y<y2;y++)
      EPD_SetPixel(x,y,color);
}

/*void EPD_FastFillRect(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2,unsigned char color)
{
  unsigned short x,x1a,x2a,y;
  unsigned char c;
  
  c = (color << 6) | (color << 4) | (color << 2) | color;
  for (y=y1;y<y2;y++)
  {
    x1a = (x1+3)/ 4;
    x2a = x2/ 4;
    for (x=x1a;x<x2a;x++)
      EPD_InfoView[y*200+x]=c;
    if ((x1a*4)>x1)
      for (x=x1;x<(x1a*4);x++)
        EPD_SetPixel(x,y,color);
    if ((x2a*4)<x2)
      for (x=(x2a*4);x<x2;x++)
        EPD_SetPixel(x,y,color);
  }
}*/