#ifndef _pwm_H
#define _pwm_H

#include "stm32f10x.h"

void TIM4_PWM_Init(u16 arr,u16 psc);
void servo(u16 angle1,u16 angle2);

#endif
