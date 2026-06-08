#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "esp8266.h"


#define Usart_SendString Usart_SendStrLen
#define UsartSendData 		Usart_SendData
#define usart1_Init 			Usart1_Init 	
#define usart2_Init				Usart2_Init 	
#define usart3_Init				Usart3_Init
#define UsartPrintf				Usart_Printf

/* 调试串口标识，工程中使用 USART_DEBUG 作为调试输出口 */
#define USART_DEBUG USART3

/*串口1 接收缓存区 */
#define USART1_MAX_RECV_LEN		400					//最大接收缓存字节数
#define USART1_MAX_SEND_LEN		400					//最大发送缓存字节数
extern u16 USART1_RX_STA;	
extern u8 USART1_RX_BUF[USART1_MAX_RECV_LEN]; 				//接收缓冲,最大USART2_MAX_RECV_LEN个字节.
extern u8 USART1_TX_BUF[USART1_MAX_SEND_LEN]; 			  //发送缓冲,最大USART2_MAX_SEND_LEN字节
/*串口2 接收缓存区 */
#define USART2_MAX_RECV_LEN		400					//最大接收缓存字节数
#define USART2_MAX_SEND_LEN		400					//最大发送缓存字节数
extern u16 USART2_RX_STA;	
extern u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; 				//接收缓冲,最大USART2_MAX_RECV_LEN个字节.
extern u8 USART2_TX_BUF[USART2_MAX_SEND_LEN]; 			  //发送缓冲,最大USART2_MAX_SEND_LEN字节
/*串口3 接收缓存区 */
#define USART3_MAX_RECV_LEN		400					//最大接收缓存字节数
#define USART3_MAX_SEND_LEN		400					//最大发送缓存字节数
extern u16 USART3_RX_STA;   						//接收数据状态
extern u8 USART3_RX_BUF[USART3_MAX_RECV_LEN]; 				//接收缓冲,最大USART2_MAX_RECV_LEN个字节.
extern u8 USART3_TX_BUF[USART3_MAX_SEND_LEN]; 			  //发送缓冲,最大USART2_MAX_SEND_LEN字节

void Usart1_Init(u32 bound);
void Usart2_Init(u32 bound);
void Usart3_Init(u32 bound);

void USART2_IRQHandler(void);
void USART1_IRQHandler(void); 

//Usart_Printf 不可重入，不能与printf、sprintf同时调用
//不能与OLED_Printf 同时调用
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);

void Usart_SendData(USART_TypeDef *USARTx, u8 data);
void Usart_SendStr(USART_TypeDef *USARTx, unsigned char *str);
void Usart_SendStrLen(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

#endif


