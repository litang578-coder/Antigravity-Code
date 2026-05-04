#include "IO_Init.h"
int key_num=0; //按键全局变量

void Led_Init() //LED初始化
{
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(GPIO_RCC_Led,ENABLE);
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_Led;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 	GPIO_Init(GPIO_Led, &GPIO_InitStructure);
	Led=0;
}
void IO_init()  //其他IO初始化
{
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1|GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
void Beep_Init() //蜂鸣器初始化
{
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(GPIO_RCC_Beep,ENABLE);
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_Beep;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 	GPIO_Init(GPIO_Beep, &GPIO_InitStructure);
	Beep=1;
}

void Relay_Init() //继电器初始化
{
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(GPIO_RCC_Relay | GPIO_RCC_Relay_BAT,ENABLE);
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_Relay | GPIO_Pin_Relay_BAT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 	GPIO_Init(GPIO_Relay, &GPIO_InitStructure);
}

/* ============================================================
 * Key_Init - 按键初始化（K1/K2/K3 原有 + K4 新增）
 * ============================================================ */
void Key_Init(void)
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能K1/K2/K3/K4所在GPIO时钟（均在GPIOB，一次使能即可） */
 	RCC_APB2PeriphClockCmd(GPIO_RCC_K1|GPIO_RCC_K2|GPIO_RCC_K3|GPIO_RCC_K4|GPIO_RCC_K5, ENABLE);

	/* K1 -> PB13 上拉输入 */
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_K1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
 	GPIO_Init(GPIO_K1, &GPIO_InitStructure);
	
	/* K2 -> PB14 上拉输入 */
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_K2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
 	GPIO_Init(GPIO_K2, &GPIO_InitStructure);
	
	/* K3 -> PB15 上拉输入 */
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_K3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIO_K3, &GPIO_InitStructure);

	/* K4 -> PB12 上拉输入（新增，用于OLED菜单切换） */
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_K4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIO_K4, &GPIO_InitStructure);

	/* K5 -> PB11 */
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_K5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIO_K5, &GPIO_InitStructure);
}

/* ============================================================
 * Key_Scan - K1/K2/K3 按键扫描（原有，不修改逻辑）
 * ============================================================ */
u8 Key_Scan()
{
	static u8 key_up,key_num;	//按键松手标志
	key_num=0;
	if(((K1==0)||(K2==0)||(K3==0))&&(key_up==1) )
	{
		key_up=0;
		delay_ms(10);		//软件消抖
		if(K1==0)
			key_num = 1;
		else if(K2==0)
			key_num = 2;
		else if(K3==0)
			key_num = 3;
	}
	else if((K1==1)&&(K2==1)&&(K3==1))
	{
		key_up=1;
	}
	return key_num;
}

/* ============================================================
 * Key_long_Scan - 长按扫描（原有，不修改逻辑）
 * ============================================================ */
u8 Key_long_Scan()				//长按键扫描
{
	static u16 key_up,key_num,key_long_time,key_return;	//按键松手标志
	key_return = 0;
	if(((K1==0)||(K2==0)||(K3==0))&&(key_up==1) )  //首次按下，记录是哪个键
	{
		key_up=0;
		delay_ms(10);
		if(K1==0)
			key_num = 1;
		else if(K2==0)
			key_num = 2;
		else if(K3==0)
			key_num = 3;
	}
	else if((K1==0)||(K2==0)||(K3==0))  //持续按下计时
	{
		delay_ms(10);	//消抖延时
		if(key_long_time++>30)
		{
			if(K1==0)
				key_num=4;
			if(K2==0)
				key_num=5;
			if(K3==0)
				key_num=6;
			key_long_time=0;
			key_return = key_num;
			key_num = 0;
		}
	}
	else if((K1==1)&&(K2==1)&&(K3==1))
	{
		key_long_time = 0;
		key_return = key_num;
		key_num = 0;
		key_up=1;
	}
	return key_return;
}

/* ============================================================
 * Key4_Scan - K4按键扫描（新增，含软件消抖）
 *
 * 功能：检测K4(PB12)的下降沿（按下事件），返回1表示有效按键事件
 * 消抖方式：静态变量记录松手状态，按下后延时10ms再确认
 * 返回值：1=K4被按下（单次触发），0=未按下
 * ============================================================ */
u8 Key4_Scan(void)
{
	static u8 k4_up = 1;   /* 松手标志：1=已松手可响应 */

	if ((K4 == 0) && (k4_up == 1))   /* K4按下且之前已松手 */
	{
		k4_up = 0;
		delay_ms(10);                /* 软件消抖等待10ms */
		if (K4 == 0)                 /* 再次确认仍处于按下状态 */
		{
			return 1;                /* 有效按键事件，返回1 */
		}
	}
	else if (K4 == 1)               /* K4松手，重置标志 */
	{
		k4_up = 1;
	}
	return 0;                        /* 无按键事件，返回0 */
}
u8 Key5_Scan(void)
{
	static u8 k5_up = 1;
	if ((K5 == 0) && (k5_up == 1))
	{
		k5_up = 0;
		delay_ms(10);
		if (K5 == 0)
		{
			return 1;
		}
	}
	else if (K5 == 1)
	{
		k5_up = 1;
	}
	return 0;
}
