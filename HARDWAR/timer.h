#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

extern u32 time_1ms;
void TIM3_Int_Init(u16 arr,u16 psc);
void TIM2_Int_Init(u16 arr,u16 psc);
#endif
