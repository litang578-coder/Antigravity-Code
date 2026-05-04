#ifndef __DS18B20_H__
#define __DS18B20_H__

#include "sys.h"
#include "delay.h"

/* 默认使用 PA0，可按实际接线修改 */
#define DS18B20_GPIO_PORT	GPIOA
#define DS18B20_GPIO_PIN	GPIO_Pin_0
#define DS18B20_GPIO_RCC	RCC_APB2Periph_GPIOA

void DS18B20_Init(void);
u8 DS18B20_Reset(void);
void DS18B20_StartConvert(void);
u8 DS18B20_ReadTemp(float *temperature);

#endif
