#ifndef __IO_Iint_H
#define __IO_Iint_H
#include "sys.h"
#include "delay.h"
/****************************
* LED定义
****************************/
#define GPIO_RCC_Led 	RCC_APB2Periph_GPIOB
#define GPIO_Led 			GPIOB
#define GPIO_Pin_Led  	GPIO_Pin_9
#define Led  					PBout(9) //
/***************************
* 蜂鸣器定义
****************************/
#define GPIO_RCC_Beep 	RCC_APB2Periph_GPIOA
#define GPIO_Beep 			GPIOA
#define GPIO_Pin_Beep 	GPIO_Pin_8
#define Beep  				PAout(8) //蜂鸣器
/***************************
* 风扇定义
****************************/
#define GPIO_RCC_K5 		RCC_APB2Periph_GPIOB
#define GPIO_K5 				GPIOB
#define GPIO_Pin_K5 		GPIO_Pin_11
#define K5  					PBin(11) //
/***************************
* 继电器IO定义
****************************/
#define GPIO_RCC_Relay 	RCC_APB2Periph_GPIOA
#define GPIO_Relay 			GPIOA
#define GPIO_Pin_Relay 	GPIO_Pin_12
#define Relay  				PAout(12) //

/***************************
* BAT (B8)
****************************/
#define GPIO_RCC_Relay_BAT 	RCC_APB2Periph_GPIOB
#define GPIO_Relay_BAT 			GPIOB
#define GPIO_Pin_Relay_BAT 	GPIO_Pin_8
#define Relay_BAT  				PBout(8) //BAT继电器1
/***************************
* 按键IO定义
****************************/
#define GPIO_RCC_K1 RCC_APB2Periph_GPIOB
#define GPIO_RCC_K2 RCC_APB2Periph_GPIOB
#define GPIO_RCC_K3 RCC_APB2Periph_GPIOB
#define GPIO_K1 		GPIOB
#define GPIO_K2 		GPIOB
#define GPIO_K3 		GPIOB
#define GPIO_Pin_K1 GPIO_Pin_13
#define GPIO_Pin_K2 GPIO_Pin_14
#define GPIO_Pin_K3 GPIO_Pin_15
#define K1  			PBin(13) //按键1读取
#define K2 				PBin(14) //按键2读取
#define K3  			PBin(15) //按键3读取

/***************************
* K4按键IO定义（菜单切换键）
* 硬件：K4接PB12，低电平触发，内部上拉
****************************/
#define GPIO_RCC_K4 RCC_APB2Periph_GPIOB
#define GPIO_K4     GPIOB
#define GPIO_Pin_K4 GPIO_Pin_12
#define K4          PBin(12)   //按键4读取，切换OLED显示菜单

/***************************
* 函数声明
****************************/
void Key_Init(void);      //按键IO初始化（K1/K2/K3/K4）
u8 Key_Scan(void);        //按键扫描（K1/K2/K3）
u8 Key_long_Scan(void);   //长按扫描
u8 Key4_Scan(void);       //K4按键扫描（含软件消抖）
void Relay_Init(void);    //继电器初始化
void Beep_Init(void);     //蜂鸣器初始化
u8 Key5_Scan(void);      //风扇初始化
void Led_Init(void);      //LED初始化
void IO_init(void);       //其他IO初始化
#endif