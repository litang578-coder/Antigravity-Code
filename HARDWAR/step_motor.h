#ifndef __STEP_MOTOR_H
#define __STEP_MOTOR_H
#include "sys.h"
#include "delay.h"
/* ∫Í∂®“Â ------------------------------------------ */

extern int stepmotor_set;
extern int stepmotor_now;
extern int stepmotor_set_max;
extern int stepmotor_set_min;
//void Step_Motor_CW(uint32_t nus);
//void Step_Motor_CCW(uint32_t nus);
void Step_Motor_Init(void);
void stepmotor_run(void);
void Step_Motor_Stop(void);
#endif
