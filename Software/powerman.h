#ifndef __POWERMAN_H__
#define __POWERMAN_H__

#include "main.h"

void PM_Config(void);
void PM_EnterStopMode(void);
void PM_EnterStandbyMode(void);
void PM_SetCPUFreq(uint8_t freq);
uint32_t PM_GetVolt();

#endif
