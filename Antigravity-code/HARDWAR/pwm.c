/*********************************************************
 * TIM4 输出 4 路 PWM, 可调占空比
 **********************************************************/

#include "pwm.h"
#include "delay.h"

/**
 * @brief  TIM4 PWM 初始化
 * @param  arr: 自动重装载值 (决定 PWM 周期)
 * @param  psc: 预分频值
 */
void TIM4_PWM_Init(u16 arr,u16 psc)
{  

	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);       /* 使能 TIM4 时钟 */
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);     /* 使能 GPIOB 时钟 */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;             /* 复用推挽输出 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* 自动重装载寄存器值, 累计 TIM_Period+1 个频率后产生一次更新中断 */
	TIM_TimeBaseStructure.TIM_Period=arr;	
	/* 设置 CNT 计数时钟 = Fck_int / (psc+1) */
	TIM_TimeBaseStructure.TIM_Prescaler=psc;	
	/* 时钟分频因子, 配置死区时间时使用, 此处未用到 */
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;		
	/* 计数模式设为向上计数 */
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;		
	/* 重复计数器, 此处未用到 */
	TIM_TimeBaseStructure.TIM_RepetitionCounter=0;	
	/* 初始化定时器 */
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
 
	/* 配置为 PWM 模式 1: 当计时器值小于比较设定值时, 通道输出有效高电平 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	/* 输出使能 */
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	/* 互补输出使能 */
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable; 
	/* 占空比大小 (初始值) */
	TIM_OCInitStructure.TIM_Pulse = 0;
	/* 输出通道电平极性: 高电平有效 */
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	/* 互补通道电平极性: 高电平有效 */
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	/* 输出通道空闲电平: 高 */
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	/* 互补通道空闲电平: 低 */
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
	
	/* 使能定时器 */
	TIM_Cmd(TIM4, ENABLE);
}

/**
 * @brief  舵机控制函数
 * @param  angle1: 初始角度对应的比较值
 * @param  angle2: 平时保持角度对应的比较值
 * @note   示例: servo(50, 150);
 */
void servo(u16 angle1,u16 angle2)
{
	TIM_Cmd(TIM4, ENABLE);
	TIM_SetCompare4(TIM4,angle1);
	delay_ms(1000);
	delay_ms(1000);
	TIM_SetCompare4(TIM4,angle2);
	delay_ms(1000);
	TIM_SetCompare4(TIM4,0);
}
