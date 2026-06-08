/* modbus.h */
#ifndef __modbus_H
#define __modbus_H
#include "stdio.h"
#include "sys.h"
#include "timer.h"
#include "usart.h"

extern u16 modbus_virtual_register[10]; /* Modbus 虚拟寄存器 */
extern u8 modbus_time_ms;

void modbus_service(void);              /* Modbus 主服务处理 */
void modbus_03_function(void);          /* 功能码 03: 读保持寄存器 */
void modbus_06_function(void);          /* 功能码 06: 写单个寄存器 */
void modbus_16_function(void);          /* 功能码 16: 写多个寄存器 */
void modbus_send_data(u8 *buff, u8 len);/* 发送数据 */
unsigned int CRC16(unsigned char *puchMsg, unsigned char usDataLen); /* CRC16 校验 */

#endif