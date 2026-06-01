/**
 ************************************************************
 *	文件名：	esp8266.c
 *	作者：		张继瑞
 *	日期：		2017-05-08
 *	版本：		V1.0
 *	说明：		ESP8266的简单驱动
 *	修改记录：	V1.1 增加透传模式、IDLE中断接收优化
 ************************************************************
**/

#include "stm32f10x.h"
#include "esp8266.h"
#include "delay.h"
#include "usart.h"
#include "OLED.h"
#include <string.h>
#include <stdio.h>

#define ESP8266_WIFI_INFO		"AT+CWJAP=\"K60Ultra\",\"2654527287\"\r\n"

unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0;
unsigned short esp8266_cntPre = 0;
unsigned char esp8266_rxDone = 0;			// USART2 IDLE中断置1
unsigned char esp8266_transparent = 0;		// 1=透传模式

//==========================================================
//	清空接收缓存
void ESP8266_Clear(void)
{
	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;
	esp8266_rxDone = 0;
}

//==========================================================
//	检查是否接收完成（使用IDLE中断标志）
_Bool ESP8266_WaitRecive(void)
{
	if(esp8266_cnt == 0)
		return REV_WAIT;

	// IDLE中断触发 -> 一帧接收完成
	if(esp8266_rxDone)
	{
		esp8266_rxDone = 0;
		return REV_OK;
	}

	return REV_WAIT;
}

//==========================================================
//	发送AT命令并检查响应
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	unsigned char timeOut = 100;		// 500ms超时 (100 x 5ms)

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));

	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)
			{
				ESP8266_Clear();
				return 0;
			}
		}
		delay_ms(5);
	}
	return 1;
}

//==========================================================
//	发送数据（支持透传模式）
void ESP8266_SendData(unsigned char *data, unsigned short len)
{
	if(esp8266_transparent)
	{
		// 透传模式：直接写入字节，无需AT握手
		Usart_SendString(USART2, data, len);
	}
	else
	{
		// 普通AT模式
		char cmdBuf[32];
		ESP8266_Clear();
		sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);
		if(!ESP8266_SendCmd(cmdBuf, ">"))
		{
			Usart_SendString(USART2, data, len);
		}
	}
}

//==========================================================
//	获取接收数据（支持透传模式）
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{
	if(esp8266_transparent)
	{
		// 透传模式：原始MQTT数据，无+IPD头
		do
		{
			if(ESP8266_WaitRecive() == REV_OK)
			{
				if(esp8266_cnt > 0)
					return esp8266_buf;
			}
			delay_ms(2);
		} while(timeOut--);
		return NULL;
	}
	else
	{
		// 普通AT模式：解析+IPD头
		char *ptrIPD = NULL;
		do
		{
			if(ESP8266_WaitRecive() == REV_OK)
			{
				ptrIPD = strstr((char *)esp8266_buf, "IPD,");
				if(ptrIPD != NULL)
				{
					ptrIPD = strchr(ptrIPD, ':');
					if(ptrIPD != NULL)
					{
						ptrIPD++;
						return (unsigned char *)(ptrIPD);
					}
					else
						return NULL;
				}
			}
			delay_ms(2);
		} while(timeOut--);
		return NULL;
	}
}

//==========================================================
//	初始化ESP8266
void ESP8266_Init(void)
{
	ESP8266_Clear();

	OLED_Clear();
	OLED_ShowStr(0, 0, (u8 *)"1.AT", 16);
	while(ESP8266_SendCmd("AT\r\n", "OK"))
		delay_ms(200);

	OLED_ShowStr(0, 2, (u8 *)"2.CWMODE", 16);
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		delay_ms(200);

	OLED_ShowStr(0, 4, (u8 *)"3.AT+CWDHCP", 16);
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		delay_ms(200);

	OLED_ShowStr(0, 6, (u8 *)"4.CWJAP", 16);
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "OK"))
		delay_ms(200);

	OLED_Clear();
	OLED_ShowStr(0, 0, (u8 *)"5.Init OK", 16); 
	delay_ms(500);
}

//==========================================================
//	进入透传模式
_Bool ESP8266_EnterTransparent(void)
{
	if(ESP8266_SendCmd("AT+CIPMODE=1\r\n", "OK"))
		return 1;		// 固件不支持，回退普通模式

	ESP8266_Clear();
	Usart_SendString(USART2, (unsigned char *)"AT+CIPSEND\r\n", 12);
	delay_ms(200);

	if(ESP8266_WaitRecive() == REV_OK)
	{
		if(strstr((const char *)esp8266_buf, ">") != NULL)
		{
			ESP8266_Clear();
			esp8266_transparent = 1;
			return 0;		// 成功
		}
	}

	ESP8266_SendCmd("AT+CIPMODE=0\r\n", "OK");
	return 1;
}

//==========================================================
//	退出透传模式
void ESP8266_ExitTransparent(void)
{
	if(!esp8266_transparent)
		return;
	delay_ms(1000);
	Usart_SendString(USART2, (unsigned char *)"+++", 3);
	delay_ms(1000);
	esp8266_transparent = 0;
	ESP8266_Clear();
}

//==========================================================
//	USART2中断服务函数
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))
			esp8266_cnt = 0;

		esp8266_buf[esp8266_cnt++] = USART2->DR;

		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}

	// IDLE中断：一帧数据接收完成
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		(void)USART2->SR;	// 清除IDLE标志：先读SR再读DR
		(void)USART2->DR;
		esp8266_rxDone = 1;
	}
}