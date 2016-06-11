/*******************************************************************************
  EPD DRIVER FOR STM32F2/4 w/ FSMC
  By ZephRay(zephray@outlook.com)
*******************************************************************************/
#include "epd.h"

unsigned char EPD_InfoBack[120000] @ ".extram";
unsigned char EPD_InfoView[120000] @ ".extram";
unsigned char EPD_BG[212000] @ ".extram";

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
    
extern const unsigned char WQY_ASCII_16[];
extern const unsigned char WQY_ASCII_24[];

uint8_t *Curr_Font;

//黑白黑白刷屏。最终到达白背景
#define FRAME_INIT_LEN 		65
const unsigned char wave_init[FRAME_INIT_LEN]=
{
0x55,0x55,0x55,0x55,0x55,0x55,0x55,
0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
0x55,0x55,0x55,0x55,0x55,0x55,0x55,
0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
0,
0x55,0x55,0x55,0x55,0x55,0x55,0x55,
0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
0x55,0x55,0x55,0x55,0x55,0x55,0x55,
0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
0,0,0,0,0,0,0,0
};

#define FRAME_INIT_RAPID_LEN     10

const unsigned char wave_init_rapid[FRAME_INIT_RAPID_LEN]=
{
  0x55,0x55,0x55,0x55,
  0xaa,0xaa,0xaa,0xaa,
  0,0,
};

#define FRAME_INIT_FAST_LEN     30

const unsigned char wave_init_fast[FRAME_INIT_FAST_LEN]=
{
  0x55,0x55,0x55,0x55,0x55,0x55,0x55,
  0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
  0,
  0x55,0x55,0x55,0x55,0x55,0x55,0x55,
  0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
  0,
};
/*
#define FRAME_END_LEN		8
unsigned char wave_end[4][FRAME_END_LEN]=
{
0,0,0,0,0,0,0,0,				//GC3->GC0
1,1,0,0,0,0,0,0,				//GC3->GC1
1,1,1,1,0,0,0,0,				//GC3->GC2
1,1,1,1,1,1,0,0,				//GC3->GC3
};

#define FRAME_BEGIN_LEN		10
unsigned char wave_begin[4][FRAME_BEGIN_LEN]=
{
0,0,0,0,0,0,0,0,0,0,				//GC3->GC0
2,2,2,0,0,0,0,0,0,0,				//GC3->GC1
2,2,2,2,2,0,0,0,0,0,				//GC3->GC2
2,2,2,2,2,2,2,2,0,0,				//GC3->GC3
};*/

#define FRAME_BEGIN_LEN		10
unsigned char wave_begin[4][FRAME_BEGIN_LEN]=
{
0,1,1,1,1,2,2,2,2,0,						//GC3->GC3
0,0,1,1,1,2,2,2,2,0,						//GC2->GC3
0,0,0,1,1,2,2,2,2,0,						//GC1->GC3
0,0,0,0,1,2,2,2,2,0,						//GC0->GC3
};

//从白刷到新图片
#define FRAME_END_LEN		15
unsigned char wave_end[4][FRAME_END_LEN]=
{
0,1,1,1,1,2,2,2,2,0,0,0,0,0,0,				//GC3->GC3
0,1,1,1,1,2,2,2,2,1,0,0,0,0,0,				//GC3->GC2
0,1,1,1,1,2,2,2,2,1,1,0,0,0,0,				//GC3->GC1
0,1,1,1,1,2,2,2,2,1,1,1,1,0,0,				//GC3->GC0
};

unsigned char wave_begin_table[256][FRAME_BEGIN_LEN] @ ".ccmram";
unsigned char wave_end_table[256][FRAME_END_LEN] @ ".ccmram";

unsigned char g_dest_data[200];				//送到电子纸的一行数据缓存

void DelayCycle(unsigned long x)
{
  while (x--)
  {
    asm("nop");
  }
}

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
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG,ENABLE);
  
  GPIOG->BSRRH = GPIO_Pin_12;//TURN OFF ALL VOLTAGES BEFORE NEXT
  GPIOG->BSRRH = GPIO_Pin_13;
  GPIOG->BSRRH = GPIO_Pin_14;
  GPIOG->BSRRH = GPIO_Pin_15;
  
  //Power controll
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOG,&GPIO_InitStructure);
  
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
  
  GPIOG->BSRRH = GPIO_Pin_15;
  DelayCycle(400);
  GPIOG->BSRRH = GPIO_Pin_14;
  DelayCycle(20);
  GPIOG->BSRRH = GPIO_Pin_13;
  DelayCycle(20);
  GPIOG->BSRRH = GPIO_Pin_12;
  DelayCycle(400);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOG,&GPIO_InitStructure);
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
  
  for (i=0;i<120000;i++)
  {
    EPD_InfoBack[i]=0;
    EPD_InfoView[i]=0;
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
    GPIOC->ODR = a|pArray[i];
    EPD_CL_H();
    //DelayCycle(2);
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
  EPD_CKV_H(); 
  DelayCycle(2);
  EPD_CKV_L();
  Delay_Us(150);
}

void EPD_Send_Row_Data_Slow(u8 *pArray)  
{
  unsigned char i;
  unsigned short a;
  
  a = GPIOC->IDR & 0xFF00;
  
  EPD_OE_H();
  
  EPD_SPH_L();                                          
  
  for (i=0;i<200;i++)
  {
    GPIOC->ODR = a|pArray[i];
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
  
  DelayCycle(90);
  
  EPD_CKV_L();
  
  DelayCycle(20);
  
  EPD_OE_L();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
       
  //
}

void EPD_Send_Row_Data_Medium(u8 *pArray)  
{
  unsigned char i;
  unsigned short a;
  
  a = GPIOC->IDR & 0xFF00;
  
  EPD_OE_H();
  
  EPD_SPH_L();                                          
  
  for (i=0;i<200;i++)
  {
    GPIOC->ODR = a|pArray[i];
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
  
  DelayCycle(60);
  
  EPD_CKV_L();
  
  DelayCycle(20);
  
  EPD_OE_L();
  
  EPD_CL_H();
  EPD_CL_L();
  EPD_CL_H();
  EPD_CL_L();
       
  //
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

//生成于RAM中的波形表，以提高屏的扫描速度
void EPD_PrepareWaveform(void)
{
	int frame, num;
	unsigned char tmp,value;

        //wave_begin_table
	for(frame=0; frame<FRAME_BEGIN_LEN; frame++)
	{		
		for(num=0; num<256; num++)
		{
			tmp = 0;
			tmp = wave_begin[(num>>6)&0x3][frame];
					
			tmp = tmp<< 2;
			tmp &= 0xfffc;
			tmp |= wave_begin[(num>>4)&0x3][frame];

			tmp = tmp<< 2;
			tmp &= 0xfffc;
			tmp |= wave_begin[(num>>2)&0x3][frame];

			tmp = tmp<< 2;
			tmp &= 0xfffc;
			tmp |= wave_begin[(num)&0x3][frame];

			value = 0;
			value = (tmp <<6) & 0xc0;
			value += (tmp<<2) & 0x30;
			value += (tmp>>2) & 0x0c;
			value += (tmp>>6) & 0x03;
			wave_begin_table[num][frame] = value;
		}
	}
        
	//wave_end_table
	for(frame=0; frame<FRAME_END_LEN; frame++)
	{		
		for(num=0; num<256; num++)
		{
			tmp = 0;
			tmp = wave_end[(num>>6)&0x3][frame];
					
			tmp = tmp<< 2;
			tmp &= 0xfffc;
			tmp |= wave_end[(num>>4)&0x3][frame];

			tmp = tmp<< 2;
			tmp &= 0xfffc;
			tmp |= wave_end[(num>>2)&0x3][frame];

			tmp = tmp<< 2;
			tmp &= 0xfffc;
			tmp |= wave_end[(num)&0x3][frame];

			value = 0;
			value = (tmp <<6) & 0xc0;
			value += (tmp<<2) & 0x30;
			value += (tmp>>2) & 0x0c;
			value += (tmp>>6) & 0x03;
			wave_end_table[num][frame] = value;
		}
	}
}

void line_data_init(u8 frame)
{
	int i;
	
	for(i=0; i<200; i++)
	{
		g_dest_data[i] = wave_init[frame];	
	}	
}

void line_begin_pic(u8 *old_pic, u8 frame)
{
	int i;
	
	for(i=0; i<200; i++)
	{
		g_dest_data[i] = wave_begin_table[old_pic[i]][frame];	
	}	
}

void EPD_EncodeLine_Pic(u8 *new_pic, u8 frame)
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

void line_end_pic(u8 *new_pic, u8 frame)
{
	int i;
	
	for(i=0; i<200; i++)
	{
		g_dest_data[i] = wave_end_table[new_pic[i]][frame];	
	}	
}

void EPD_Power_On(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  GPIOG->BSRRL = GPIO_Pin_12;
  DelayCycle(20);
  GPIOG->BSRRL = GPIO_Pin_13;
  DelayCycle(20);
  GPIOG->BSRRL = GPIO_Pin_14;
  DelayCycle(20);
  GPIOG->BSRRL = GPIO_Pin_15;
  DelayCycle(400);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOG,&GPIO_InitStructure);
}

void EPD_Clear(void)
{
  unsigned short line,frame;
  
  for(frame=0; frame<FRAME_INIT_LEN; frame++)			
  {
    EPD_Start_Scan();
    for(line=0; line<600; line++)
    {
      line_data_init(frame);							//14ms
      EPD_Send_Row_Data( g_dest_data );				//40ms
    }
    EPD_Send_Row_Data( g_dest_data );					//最后一行还需GATE CLK,故再传一行没用数据
  }
}

void EPD_FastClear(void)
{
  unsigned short line,frame,i;
  
  for(frame=0; frame<FRAME_INIT_FAST_LEN; frame++)			
  {
    EPD_Start_Scan();
    for(line=0; line<600; line++)
    {
      for(i=0;i<200;i++)  g_dest_data[i]=wave_init_fast[frame];		
      EPD_Send_Row_Data( g_dest_data );				//40ms
    }
    EPD_Send_Row_Data( g_dest_data );					//最后一行还需GATE CLK,故再传一行没用数据
  }
}

void EPD_DispPic(unsigned char *img)
{
  unsigned short frame;
  signed short line;
  unsigned long i;
  unsigned char *ptr;
  
  ptr = img;
  
  //从黑刷到灰度
  for(frame=0; frame<10; frame++)					
  {
    EPD_Start_Scan();
    
    for(line=0; line<70; line++)
    {
      EPD_SkipRow();
    }
    for(line=530-1; line>=0; line--)
    {
      EPD_EncodeLine_Pic(ptr + line*400, frame);		
      EPD_Send_Row_Data_Medium(g_dest_data);		
    }
    EPD_Send_Row_Data( g_dest_data );				
  }
  
  for(frame=10; frame<12; frame++)					
  {
    EPD_Start_Scan();
    
    for(line=0; line<70; line++)
    {
      EPD_SkipRow();
    }
    for(line=530-1; line>=0; line--)
    {
      EPD_EncodeLine_Pic(ptr + line*400, frame);		
      EPD_Send_Row_Data_Slow(g_dest_data);		
    }
    EPD_Send_Row_Data( g_dest_data );				
  }
  
  for(frame=12; frame<16; frame++)					
  {
    EPD_Start_Scan();
    
    for(line=0; line<70; line++)
    {
      EPD_SkipRow();
    }
    for(line=530-1; line>=0; line--)
    {
      EPD_EncodeLine_Pic(ptr + line*400, frame);		
      EPD_Send_Row_Data(g_dest_data);		
    }
    EPD_Send_Row_Data( g_dest_data );				
  }
}

void EPD_DispInfo(void)
{
  unsigned short frame;
  signed short line;
  unsigned long i;
  unsigned char *ptr;
  
  ptr = EPD_InfoBack;
  
  for(frame=0; frame<FRAME_BEGIN_LEN-2; frame++)					
  {
    EPD_Start_Scan();
    for(line=(600-1); line>=(600-70); line--)
    {
      line_begin_pic(ptr + line*200, frame);
      EPD_Send_Row_Data( g_dest_data );
    }
    for(i=0;i<200;i++)  g_dest_data[i]=0xaa;
    EPD_Send_Row_Data( g_dest_data );
    for(line=0;line<(600-70);line++)
      EPD_SkipRow();
    EPD_Send_Row_Data( g_dest_data );					
  }
  
  //WTF
  /*for(frame=0; frame<FRAME_INIT_RAPID_LEN; frame++)					
  {
    EPD_Start_Scan();
    
    for(line=(600-1); line>=(600-70); line--)
    {
      for(i=0;i<200;i++)  g_dest_data[i]=wave_init_rapid[frame];
      EPD_Send_Row_Data( g_dest_data );
    }
    for(line=0;line<(600-70);line++)
      EPD_SkipRow();
    EPD_Send_Row_Data( g_dest_data );					
  }*/
  
  ptr = EPD_InfoView;
  
  for(frame=0; frame<FRAME_END_LEN-2; frame++)					
  {
    EPD_Start_Scan();
    for(line=(600-1); line>=(600-70); line--)
    {
      line_end_pic(ptr + line*200, frame);
      EPD_Send_Row_Data( g_dest_data );
    }
    for(i=0;i<200;i++)  g_dest_data[i]=0xaa;
    EPD_Send_Row_Data( g_dest_data );
    for(line=0;line<(600-70);line++)
      EPD_SkipRow();
    EPD_Send_Row_Data( g_dest_data );					
  }
  
  for (i=0;i<120000;i++)
    EPD_InfoBack[i] = EPD_InfoView[i];
}

void EPD_DispFull(void)
{
  unsigned short frame;
  signed short line;
  unsigned long i;
  unsigned char *ptr;
  
  ptr = EPD_InfoBack;
  
  for(frame=0; frame<FRAME_BEGIN_LEN-2; frame++)					
  {
    EPD_Start_Scan();
    for(line=(600-1); line>=0; line--)
    {
      line_begin_pic(ptr + line*200, frame);
      EPD_Send_Row_Data( g_dest_data );
    }				
  }
  
  ptr = EPD_InfoView;
  
  for(frame=0; frame<FRAME_END_LEN-2; frame++)					
  {
    EPD_Start_Scan();
    for(line=(600-1); line>=0; line--)
    {
      line_end_pic(ptr + line*200, frame);
      EPD_Send_Row_Data( g_dest_data );
    }				
  }
  
  for (i=0;i<120000;i++)
    EPD_InfoBack[i] = EPD_InfoView[i];
}

void EPD_ClearInfo(unsigned char c)
{
  unsigned long i;
  
  for (i=0;i<120000;i++)
    EPD_InfoView[i]=c;
}

void EPD_ClearBack(void)
{
  unsigned long i;
  
  for (i=0;i<120000;i++)
    EPD_InfoBack[i]=0x00;
}

void EPD_SetPixel(unsigned short x,unsigned short y,unsigned char color)
{
  unsigned short x_bit;
  unsigned long x_byte;  
  
  if ((x<800)&&(y<600))
  {
    x_byte=x/4+y*200;
    x_bit=(x%4)<<1;             
  
    EPD_InfoView[x_byte] &= ~(0x03 << x_bit);
    EPD_InfoView[x_byte] |= (color << x_bit);
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

void EPD_SetLegacyFont(unsigned char * font)
{
  Curr_Font = font;
}

void EPD_PutChar_16(unsigned short x, unsigned short y, unsigned short chr, unsigned char color)
{
  unsigned short x1,y1;
  unsigned short ptr;
  
  ptr=(chr-0x20)*64;
  for (y1=0;y1<16;y1++)
  {
    for (x1=0;x1<8;x1+=2)
    {
      if (color)
      {
        EPD_SetPixel(x+x1+1,y+y1,3-((WQY_ASCII_16[ptr] & 0xF)>>2)); 
        EPD_SetPixel(x+x1,y+y1, 3-(WQY_ASCII_16[ptr] >> 6)); 
        ptr++;
      }
      else
      {
        EPD_SetPixel(x+x1+1,y+y1,(WQY_ASCII_16[ptr] & 0xF)>>2); 
        EPD_SetPixel(x+x1,y+y1, WQY_ASCII_16[ptr] >> 6); 
        ptr++;
      }
    }
    
  }
}

void EPD_PutChar_24(unsigned short x, unsigned short y, unsigned short chr, unsigned char color)
{
  unsigned short x1,y1;
  unsigned short ptr;
  
  ptr=(chr-0x20)*144;
  for (y1=0;y1<24;y1++)
  {
    for (x1=0;x1<12;x1+=2)
    {
      if (color)
      {
        EPD_SetPixel(x+x1+1,y+y1,3-((WQY_ASCII_24[ptr] & 0xF)>>2)); 
        EPD_SetPixel(x+x1,y+y1, 3-(WQY_ASCII_24[ptr] >> 6)); 
        ptr++;
      }
      else
      {
        EPD_SetPixel(x+x1+1,y+y1,(WQY_ASCII_24[ptr] & 0xF)>>2); 
        EPD_SetPixel(x+x1,y+y1, WQY_ASCII_24[ptr] >> 6); 
        ptr++;
      }
    }
    
  }
}

void EPD_PutChar_Legacy(unsigned short x, unsigned short y, unsigned short chr, unsigned char color)
{
  unsigned short x1,y1,b;
  unsigned short ptr;
  
  ptr=(chr-0x20)*144;
  for (y1=0;y1<48;y1++)
  {
    for (x1=0;x1<24;x1+=8)
    {
      for (b=0;b<8;b++)
        if ((Curr_Font[ptr]>>b)&0x01)
          EPD_SetPixel(x+x1+b,y+y1,color); 
        else
          EPD_SetPixel(x+x1+b,y+y1,3-color);
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

void EPD_FastFillRect(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2,unsigned char color)
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
}