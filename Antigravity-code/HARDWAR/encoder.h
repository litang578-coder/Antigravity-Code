#ifndef __ENCODER_H
#define __ENCODER_H
#include <sys.h>

#define ENCODER_TIM_PERIOD (u16)(65535)   /* 最大计数值 65535, 因为 F103 的定时器为 16 位 */

void Encoder_Init_TIM2(void);
void Encoder_Init_TIM4(void);
int Read_Encoder(u8 TIMX);
void TIM4_IRQHandler(void);
void TIM2_IRQHandler(void);

extern int encode_exit_cnt;              /* 编码器中断计数 */
extern unsigned long int encode_cnt;

#endif
