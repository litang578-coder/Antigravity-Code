#include "step_motor.h"

/* 步进电机1参数宏 */
#define LA PAout(4)     /* A相 */
#define LB PAout(5)     /* B相 */
#define LC PAout(6)     /* C相 */
#define LD PAout(7)     /* D相 */

/* A相 */
#define LA_GPIO_PORT    GPIOA
#define LA_GPIO_PIN     GPIO_Pin_4
#define LA_GPIO_CLK     RCC_APB2Periph_GPIOA
/* B相 */
#define LB_GPIO_PORT    GPIOA
#define LB_GPIO_PIN     GPIO_Pin_5
#define LB_GPIO_CLK     RCC_APB2Periph_GPIOA
/* C相 */
#define LC_GPIO_PORT    GPIOA
#define LC_GPIO_PIN     GPIO_Pin_6
#define LC_GPIO_CLK     RCC_APB2Periph_GPIOA
/* D相 */
#define LD_GPIO_PORT    GPIOA
#define LD_GPIO_PIN     GPIO_Pin_7
#define LD_GPIO_CLK     RCC_APB2Periph_GPIOA

int stepmotor_set=0;
int stepmotor_now=0;
int stepmotor_set_max=2048;   //4096为1圈
int stepmotor_set_min=0;
void stepmotor_run()
{
	static u8 step_cnt=1;
	
	if(stepmotor_set>stepmotor_now)
	{
		stepmotor_now++;
		if(++step_cnt>8) step_cnt = 1;
	}
	else if(stepmotor_set<stepmotor_now)
	{
		stepmotor_now--;
		if(--step_cnt<1) step_cnt = 8;
	}
	else
	{
		Step_Motor_Stop();
		return ;
	}
	switch(step_cnt)
	{
			case 1:LA=0;LB=1;LC=1;LD=0;//   0110
			break;
			case 2:LA=0;LB=1;LC=1;LD=1;//   0111
			break;
			case 3:LA=0;LB=0;LC=1;LD=1;//   0011
			break;
			case 4:LA=1;LB=0;LC=1;LD=1;//   1011
			break;
			case 5:LA=1;LB=0;LC=0;LD=1;//   1001
			break;
			case 6:LA=1;LB=1;LC=0;LD=1;//   1101
			break;
			case 7:LA=1;LB=1;LC=0;LD=0;//   1100
			break;
			case 8:LA=1;LB=1;LC=1;LD=0;//   1110
			break;
			default:
			break;
	}
}
/**
 * @name: Step_Motor_Stop
 * @description: 电机停止
 * @param {*}
 * @return {*}
 */
void Step_Motor_Stop(void)
{
   GPIO_ResetBits(LA_GPIO_PORT, LA_GPIO_PIN);
   GPIO_ResetBits(LB_GPIO_PORT, LB_GPIO_PIN);
   GPIO_ResetBits(LC_GPIO_PORT, LC_GPIO_PIN);
   GPIO_ResetBits(LD_GPIO_PORT, LD_GPIO_PIN);
}

/**
 * @name: Step_Motor_Init
 * @description: 步进电机初始化端口
 * @param {*}
 * @return {*}
 */
void Step_Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(LA_GPIO_CLK | LB_GPIO_CLK | LC_GPIO_CLK | LD_GPIO_CLK, ENABLE);

    /* A相端口初始化 */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = LA_GPIO_PIN;
    GPIO_Init(LA_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = LB_GPIO_PIN;
    GPIO_Init(LB_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = LC_GPIO_PIN;
    GPIO_Init(LC_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = LD_GPIO_PIN;
    GPIO_Init(LD_GPIO_PORT, &GPIO_InitStruct);

    GPIO_ResetBits(LA_GPIO_PORT, LA_GPIO_PIN);
    GPIO_ResetBits(LB_GPIO_PORT, LB_GPIO_PIN);
    GPIO_ResetBits(LC_GPIO_PORT, LC_GPIO_PIN);
    GPIO_ResetBits(LD_GPIO_PORT, LD_GPIO_PIN);
}

