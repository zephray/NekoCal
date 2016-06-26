#include "main.h"
#include "usart.h"
//#include "sdio_sd.h"
#include "sdcard.h"
#include "epd.h"
#include "image.h"
#include "sram.h"
//#include "dac.h"
#include "powerman.h"
#include "ds3231.h"

//#include "gui.h"

__IO uint32_t TimingDelay;

void NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 10 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

int main(void)
{
  //RCC_ClocksTypeDef RCC_Clocks;
  unsigned long i;
  
  USART1_Config();
        
  printf("\r\n\r\nSTM32F2 Development Platform\r\nBuild by Zweb.\r\n");
  printf("Ready to turn on the Systick.\r\n");
        
  /* SysTick end of count event each 10ms */
  //RCC_GetClocksFreq(&RCC_Clocks);
  //SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);
  
  //printf("Systick opened successfully.\r\n");
  //printf("Main Clock Frequency: %d MHz\r\n",(RCC_Clocks.HCLK_Frequency/1000000));

  //NVIC_Config();
  //PM_SetCPUFreq(120);
  
  EPD_Init();
  EPD_Power_On();
  EPD_FastClear();
  EPD_Power_Off();
  
  IIC_Config();
  
  DS3231_Init();
  DS3231_ReadWrite_Time(1);
  if (TimeValue[6] < 0x10) 
  {
    TimeValue[0] = 0x00;
    TimeValue[1] = 0x00;
    TimeValue[2] = 0x08;
    TimeValue[3] = 0x01;
    TimeValue[4] = 0x04;
    TimeValue[5] = 0x04;
    TimeValue[6] = 0x16;
    DS3231_ReadWrite_Time(0);
  }
  
  Key_GPIO_Config();
  
  UI_Main();//Should not return*/
  
  while(1)
  {
    
  }
  
}
