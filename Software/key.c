#include "key.h" 

 /*
 * 函数名：Key_GPIO_Config
 * 描述  ：配置按键用到的I/O口
 * 输入  ：无
 * 输出  ：无
 */
void Key_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_InitStructure.GPIO_PuPd =GPIO_PuPd_UP; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

 /*
 * 函数名：Key_Scan
 * 描述  ：检测是否有按键按下
 * 输入  ：GPIOx：x 可以是 A，B，C，D或者 E
 *		     GPIO_Pin：待读取的端口位 	
 * 输出  ：KEY_OFF(没按下按键)、KEY_ON（按下按键）
 */
u8 Key_ScanL(GPIO_TypeDef* GPIOx,u16 GPIO_Pin)
{			
		/*检测是否有按键按下 */
   if(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) == 0 ) 
  {	   
	 	 /*延时消抖*/
    DelayCycle(5000);		//CPU占用高，5000 is enough
    if(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) == 0 )  
    {	 
	/*等待按键释放 */
	while(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) == 0);  
	return 	1;	 
    }
    else
	return 0;
  }
  else
    return 0;
}

unsigned char Key_Scan(unsigned char id)
{
  switch (id)
  {
  case 0: return Key_ScanL(GPIOB,GPIO_Pin_12);
  case 1: return Key_ScanL(GPIOB,GPIO_Pin_13);
  case 2: return Key_ScanL(GPIOB,GPIO_Pin_14);
  case 3: return Key_ScanL(GPIOB,GPIO_Pin_15);
  }
}
