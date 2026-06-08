#include "encoder.h"
#include "stm32f10x_gpio.h"

int encode_exit_cnt;
unsigned long int encode_cnt;

/**************************************************************************
 * 功能: TIM2 初始化为编码器接口模式
 * 入参: 无
 * 返回值: 无
 **************************************************************************/
void Encoder_Init_TIM2(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
  TIM_ICInitTypeDef TIM_ICInitStructure;  
	NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); /* 使能定时器 2 时钟 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);/* 使能 GPIOA 端口时钟 */
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;	/* 端口配置 PA0, PA1 */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; /* 浮空输入 */
  GPIO_Init(GPIOA, &GPIO_InitStructure);                /* 初始化 GPIOA */
  
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Prescaler = 0x0;             /* 预分频器: 不分频 */
  TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD; /* 设定自动重装载值 */
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;/* 选择时钟分频: 不分频 */
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; /* TIM 向上计数 */
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_TIxExternalClockConfig(TIM2,TIM_TIxExternalCLK1Source_TI1,TIM_ICPolarity_Falling,15);
//  TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising); /* 使用编码器模式 3 */
  TIM_ICStructInit(&TIM_ICInitStructure);
  TIM_ICInitStructure.TIM_ICFilter = 10;
  TIM_ICInit(TIM2, &TIM_ICInitStructure);
  TIM_ClearFlag(TIM2, TIM_FLAG_Update);                  /* 清除 TIM 的更新标志位 */
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel =TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  /* 复位计数器 */
  TIM_SetCounter(TIM2,0);
  TIM_Cmd(TIM2, ENABLE); 
}

/**************************************************************************
 * 功能: TIM4 初始化为编码器接口模式
 * 入参: 无
 * 返回值: 无
 **************************************************************************/
void Encoder_Init_TIM4(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
  TIM_ICInitTypeDef TIM_ICInitStructure;  
	NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); /* 使能定时器 4 时钟 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);/* 使能 GPIOB 端口时钟 */
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;	/* 端口配置 PB6, PB7 */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; /* 浮空输入 */
  GPIO_Init(GPIOB, &GPIO_InitStructure);                /* 初始化 GPIOB */
  
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Prescaler = 0x0;             /* 预分频器: 不分频 */
  TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD; /* 设定自动重装载值 */
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;/* 选择时钟分频: 不分频 */
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; /* TIM 向上计数 */
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	TIM_TIxExternalClockConfig(TIM4,TIM_TIxExternalCLK1Source_TI1,TIM_ICPolarity_Falling,15);
	
//  TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI1, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising); /* 使用编码器模式 3 */
  TIM_ICStructInit(&TIM_ICInitStructure);
  TIM_ICInitStructure.TIM_ICFilter = 10;                 /* 滤波值 */
  TIM_ICInit(TIM4, &TIM_ICInitStructure);
  TIM_ClearFlag(TIM4, TIM_FLAG_Update);                  /* 清除 TIM 的更新标志位 */
  TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);             /* 更新中断 */
	
	NVIC_InitStructure.NVIC_IRQChannel =TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  /* 复位计数器 */
  TIM_SetCounter(TIM4,0);
  TIM_Cmd(TIM4, ENABLE); 
}

/**************************************************************************
 * 功能: 读取编码器计数值
 * 入参: 定时器编号 TIMX (2/3/4)
 * 返回值: 编码器速度值
 **************************************************************************/
int Read_Encoder(u8 TIMX)
{
    int Encoder_TIM;    
   switch(TIMX)
	 {
	   case 2:  Encoder_TIM= (short)TIM2 -> CNT;  TIM2 -> CNT=0;break;
		 case 3:  Encoder_TIM= (short)TIM3 -> CNT;  TIM3 -> CNT=0;break;	
		 case 4:  Encoder_TIM= (short)TIM4 -> CNT;  TIM4 -> CNT=0;break;	
		 default:  Encoder_TIM=0;
	 }
		return Encoder_TIM;
}

/**************************************************************************
 * 功能: TIM4 中断服务函数
 * 入参: 无
 * 返回值: 无
 **************************************************************************/
void TIM4_IRQHandler(void)
{ 		    		  			    
	if(TIM4->SR&0X0001) /* 溢出中断 */
	{    				
		if((TIM4->CR1&0x10) == 0x10) /* 向下计数 */
			encode_exit_cnt--;
    else /* 向上计数 */
			encode_exit_cnt++;
	}
	TIM4->SR&=~(1<<0); /* 清除中断标志位 */
}

/**************************************************************************
 * 功能: TIM2 中断服务函数
 * 入参: 无
 * 返回值: 无
 **************************************************************************/
//void TIM2_IRQHandler(void)
//{ 		    		  			    
//	if(TIM2->SR&0X0001) /* 溢出中断 */
//	{    							     	    	
//	}				   
//	TIM2->SR&=~(1<<0); /* 清除中断标志位 */
//}
