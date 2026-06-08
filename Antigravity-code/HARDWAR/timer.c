#include "timer.h"

extern vu16 USART2_RX_STA;
u32 time_1ms=0;

// void TIM2_IRQHandler(void)   /* TIM2 中断 */
//{
//	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  /* 检查 TIM2 中断是否发生 */
//	{
//		TIM_ClearITPendingBit(TIM2, TIM_IT_Update  );  /* 清除 TIMx 中断标志 */
//		time_s++;
//	}
//}

/**
 * @brief  定时器 3 中断服务函数
 */
void TIM3_IRQHandler(void)
{ 	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) /* 检查是否为更新中断 */
	{	 			   
		USART2_RX_STA|=1<<15;	/* 标记接收完成 */
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  /* 清除 TIM3 中断标志 */
		TIM_Cmd(TIM3, DISABLE);  /* 关闭 TIM3 */
	}    
}

/**
 * @brief  通用定时器 3 中断初始化
 * @note   定时器时钟选择为 APB1 的 2 倍, APB1 为 36MHz
 *         定时器溢出时间计算方法: Tout = ((arr+1) * (psc+1)) / Ft (us)
 *         Ft = 定时器工作频率, 单位: MHz
 * @param  arr: 自动重装载值
 * @param  psc: 定时器预分频值
 */
void TIM3_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); /* TIM3 时钟使能 */
	
	/* 定时器 TIM3 初始化 */
	TIM_TimeBaseStructure.TIM_Period = arr;                /* 设定自动重装载寄存器的值 */
	TIM_TimeBaseStructure.TIM_Prescaler =psc;              /* 设为 TIMx 时钟频率除以预分频值 */
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;/* 设置时钟分割: TDTS = Tck_tim */
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; /* TIM 向上计数模式 */
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);        /* 根据指定参数初始化 TIMx 时基单元 */
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE );              /* 使能 TIM3 更新中断 */
	
	TIM_Cmd(TIM3,ENABLE); /* 启动定时器 3 */
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;  /* 抢占优先级 0 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		  /* 子优先级 2 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  /* IRQ 通道使能 */
	NVIC_Init(&NVIC_InitStructure);	/* 根据指定参数初始化 NVIC 寄存器 */
	
}

/**
 * @brief  通用定时器 2 中断初始化
 * @param  arr: 自动重装载值
 * @param  psc: 定时器预分频值
 */
void TIM2_Int_Init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);  /* 时钟使能 */
	
	/* 定时器 TIM2 初始化 */
	TIM_TimeBaseStructure.TIM_Period = arr;                /* 设定自动重装载寄存器的值 */
	TIM_TimeBaseStructure.TIM_Prescaler =psc;              /* 设为 TIMx 时钟频率除以预分频值 */
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;/* 设置时钟分割: TDTS = Tck_tim */
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; /* TIM 向上计数模式 */
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);        /* 根据指定参数初始化 TIMx 时基单元 */
 
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE );              /* 使能 TIM2 更新中断 */

	/* 中断优先级 NVIC 设置 */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;            /* TIM2 中断 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  /* 抢占优先级 0 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;         /* 子优先级 3 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            /* IRQ 通道使能 */
	NVIC_Init(&NVIC_InitStructure);  /* 初始化 NVIC 寄存器 */

	TIM_Cmd(TIM2, ENABLE);  /* 使能 TIM2 */
}
