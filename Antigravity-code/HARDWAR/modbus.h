//modbus.h
#ifndef __modbus_H
#define __modbus_H
#include "stdio.h"	
#include "sys.h" 
#include "timer.h"
#include "usart.h"
extern u16 modbus_virtual_register[10]; //ÐéÄâ¼Ä´æÆ÷
extern u8 modbus_time_ms;
void modbus_service(void);
void modbus_03_function(void);
void modbus_06_function(void);
void modbus_16_function(void);
void modbus_send_data(u8 *buff,u8 len);
unsigned int CRC16(unsigned char *puchMsg,  unsigned char usDataLen);

#endif

