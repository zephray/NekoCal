#include "powerman.h"

void PM_Config()
{

}

void PM_SetCPUFreq(uint8_t freq)
{
  //PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
  u8 PLL_M=25;
  u16 PLL_N=240;
  //SYSCLK = PLL_VCO / PLL_P
  u8 PLL_P=2;
  //USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ
  u8 PLL_Q=5;
  RCC_ClocksTypeDef RCC_Clocks;
  
  //选择HSI为时钟源
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= RCC_CFGR_SW_HSI;
  //等待时钟源被选择为HSI
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_HSI);
  if (freq>16)//如果需要使用PLL
  {
    PLL_N=freq*2;
    //关闭主PLL
    RCC->CR &= (uint32_t)((uint32_t)~(RCC_CR_PLLON));
    //配置主PLL
    RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
                 (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);
    //启用HSE
    RCC->CR |= RCC_CR_HSEON;
    //等待HSE准备好
    while((RCC->CR & RCC_CR_HSERDY) == 0);
    //启用主PLL
    RCC->CR |= RCC_CR_PLLON;
    //等待PLL准备好
    while((RCC->CR & RCC_CR_PLLRDY) == 0);
    //启用FLASH预读，I/D-Cache，配置Flash等待
    FLASH->ACR = FLASH_ACR_PRFTEN |FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_4WS;
    //将时钟源选为PLL
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    //等待主时钟源被切换成PLL
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL);
    
    RCC->CFGR &= ~0xF0;
  }
  else//如果时钟为16MHz
  {
    //关闭主PLL
    RCC->CR &= (uint32_t)((uint32_t)~(RCC_CR_PLLON));
    //关闭HSE
    RCC->CR &= (uint32_t)((uint32_t)~(RCC_CR_HSEON));
    //启用FLASH预读，I/D-Cache，配置Flash等待
    FLASH->ACR = FLASH_ACR_PRFTEN |FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_0WS;
    
    RCC->CFGR &= ~0xF0;
    RCC->CFGR |= 0xB0;
  }
  SystemCoreClockUpdate();
  //重新配置SysTick
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);
}

void PM_AdcInit()
{
  ADC_InitTypeDef ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //开ADC1时钟
  ADC_DeInit();
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;  //精度为12位           
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;   //扫描转换模式失能
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;  //连续转换使能
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; //不用外部触发，软件触发转换
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; //数据右对齐，低字节对齐
  ADC_InitStructure.ADC_NbrOfConversion = 1;    //规定了顺序进行规则转换的ADC通道的数目
  ADC_Init(ADC1, &ADC_InitStructure);      
  
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent; //独立模式
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8; //分频为8
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //失能DMA_MODE
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;//两次采样间隔20个周期
  ADC_CommonInit(&ADC_CommonInitStructure);
  
  ADC_Cmd(ADC1, ENABLE);       //使能ADC1
}

void PM_AdcDeInit()
{
  ADC_Cmd(ADC1, DISABLE);
  ADC_DeInit();
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
}

uint32_t PM_GetVolt()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  uint32_t vref,volt;
  uint32_t realvolt;
  uint8_t i;
  
  PM_AdcInit();
  ADC_TempSensorVrefintCmd(ENABLE);
  vref=0;
  for(i=0;i<5;i++)
  {
    ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 1, ADC_SampleTime_480Cycles);
    ADC_SoftwareStartConv(ADC1);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
    vref += ADC_GetConversionValue(ADC1);
  }
  ADC_TempSensorVrefintCmd(DISABLE);
  vref=vref/5;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB,&GPIO_InitStructure);
  
  PM_AdcInit();
  volt=0;
  for(i=0;i<5;i++)
  {
    ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_480Cycles);//规则通道配置，1表示规则组采样顺序
    ADC_SoftwareStartConv(ADC1);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
    volt += ADC_GetConversionValue(ADC1);
  }
  PM_AdcDeInit();
  volt = volt/5;
  
  realvolt = (volt*1200*2)/vref;
  
  return realvolt;
}

void PM_EnterStopMode()
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
  #ifdef MDK_ARM
	WFI();
#else
  asm("WFI");               //执行WFE指令
#endif
}

#ifdef MDK_ARM
__asm void WFI(void)
{
	WFI
}
#endif

void PM_EnterStandbyMode()
{
  //LCD_PowerSave();
  //SPI_Flash_PowerDown();
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
  PWR_WakeUpPinCmd(ENABLE);
  SCB->SCR|=1<<2;//使能SLEEPDEEP位 (SYS->CTRL)
  PWR->CR|=1<<2;           //清除Wake-up 标志
  PWR->CR|=1<<1;           //PDDS置位
#ifdef MDK_ARM
	WFI();
#else
  asm("WFI");               //执行WFE指令
#endif
}

void PM_TIMConfig(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  uint16_t PrescalerValue = 0;
  
  /* TIM2 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
  
  /* Enable the TIM2 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init(&NVIC_InitStructure);
  
  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) 0x400;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 0x800;
  TIM_TimeBaseStructure.TIM_Prescaler = 0x400;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  TIM_ClearFlag(TIM3, TIM_FLAG_Update);
  
  /* TIM IT enable */
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  /* TIM2 enable counter */
  TIM_Cmd(TIM3, ENABLE);
  
  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);
}

