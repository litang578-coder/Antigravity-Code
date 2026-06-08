#ifndef _pwm_H
#define _pwm_H

#include "stm32f10x.h"

void TIM4_PWM_Init(u16 arr, u16 psc);  /* TIM4 PWM 初始化 */
void servo(u16 angle1, u16 angle2);     /* 舵机控制 */

#endif
