/*******************************************************************************
  EPD DRIVER FOR STM32F2/4 w/ FSMC
  By ZephRay(zephray@outlook.com)
*******************************************************************************/
#ifndef __EPD_H__
#define __EPD_H__

#include "main.h"

extern unsigned char EPD_BG[212000] @ ".extram";

#define PulseDelay()    {}

//SOURCE DRIVER
#define EPD_CL_L()        {GPIOA->BSRRH = GPIO_Pin_1; PulseDelay();}
#define EPD_CL_H()        {GPIOA->BSRRL = GPIO_Pin_1; PulseDelay();}
#define EPD_LE_L()        {GPIOA->BSRRH = GPIO_Pin_2; PulseDelay();}
#define EPD_LE_H()        {GPIOA->BSRRL = GPIO_Pin_2; PulseDelay();}
#define EPD_OE_L()        {GPIOA->BSRRH = GPIO_Pin_3; PulseDelay();}
#define EPD_OE_H()        {GPIOA->BSRRL = GPIO_Pin_3; PulseDelay();}
#define EPD_SHR_L()       {GPIOA->BSRRH = GPIO_Pin_4; PulseDelay();}
#define EPD_SHR_H()       {GPIOA->BSRRL = GPIO_Pin_4; PulseDelay();}
#define EPD_SPH_L()       {GPIOA->BSRRH = GPIO_Pin_5; PulseDelay();}
#define EPD_SPH_H()       {GPIOA->BSRRL = GPIO_Pin_5; PulseDelay();}

//GATE DRIVER
#define EPD_GMODE1_L()    {GPIOE->BSRRH = GPIO_Pin_3; PulseDelay();}
#define EPD_GMODE1_H()    {GPIOE->BSRRL = GPIO_Pin_3; PulseDelay();}
#define EPD_GMODE2_L()    {GPIOE->BSRRH = GPIO_Pin_2; PulseDelay();}
#define EPD_GMODE2_H()    {GPIOE->BSRRL = GPIO_Pin_2; PulseDelay();}
#define EPD_XRL_L()       {GPIOE->BSRRH = GPIO_Pin_4; PulseDelay();}
#define EPD_XRL_H()       {GPIOE->BSRRL = GPIO_Pin_4; PulseDelay();}
#define EPD_SPV_L()       {GPIOA->BSRRH = GPIO_Pin_6; PulseDelay();}
#define EPD_SPV_H()       {GPIOA->BSRRL = GPIO_Pin_6; PulseDelay();}
#define EPD_CKV_L()       {GPIOA->BSRRH = GPIO_Pin_7; PulseDelay();}
#define EPD_CKV_H()       {GPIOA->BSRRL = GPIO_Pin_7; PulseDelay();}


void EPD_Init(void);
void EPD_Power_Off(void);
void EPD_Power_On(void);
void EPD_Clear(void);
void EPD_PrepareWaveform(void);
void EPD_DispPic(unsigned char *pic);
void EPD_DispInfo(void);
void EPD_DispFull(void);
void EPD_ClearInfo(unsigned char c);
void EPD_ClearBack(void);
void EPD_SetPixel(unsigned short x,unsigned short y,unsigned char color);
void EPD_Line(unsigned short x0,unsigned short y0,unsigned short x1,unsigned short y1,unsigned short color);
void EPD_PutChar_16(unsigned short x, unsigned short y, unsigned short chr, unsigned char color);
void EPD_PutChar_Legacy(unsigned short x, unsigned short y, unsigned short chr, unsigned char color);
void EPD_String_16(unsigned short x,unsigned short y,unsigned char *s,unsigned char color);
void EPD_String_24(unsigned short x,unsigned short y,unsigned char *s,unsigned char color);
void EPD_FillRect(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2,unsigned char color);

#endif
