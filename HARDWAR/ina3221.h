#ifndef __INA3221_H
#define __INA3221_H

#include "sys.h"

/*
 * ============================================================
 * I2C 地址定义 (8-bit)
 * ============================================================
 */
#define INA3221_ADDR1 0x80 /* A0=GND */
#define INA3221_ADDR2 0x82 /* A0=VS  */
#define INA3221_ADDR3 0x84 /* A0=SDA */
#define INA3221_ADDR4 0x86 /* A0=SCL */

/* 工程默认使用的地址 */
#define INA3221_WRITE_ADDR INA3221_ADDR1

/*
 * ============================================================
 * 数据结构
 * ============================================================
 */
typedef struct {
  float ch1_voltage;   /* CH1电池端电压（V） */
  float ch1_current;   /* CH1电池端电流（A） */
  float ch2_voltage;   /* CH2 MPPT端电压（V） */
  float ch2_current;   /* CH2 MPPT端电流（A） */
  float ch3_voltage;   /* CH3 降压端电压（V） */
  float ch3_current;   /* CH3 降压端电流（A） */
  uint8_t battery_soc; /* 电池电量（0~100%） */
} INA3221_Data;

extern INA3221_Data g_ina3221;

/*
 * ============================================================
 * 函数声明
 * ============================================================
 */
void INA3221_Init(void);
void INA3221_ReadAll(void);
uint8_t Calculate_Battery_SOC(float voltage);

/* 寄存器底层读写接口 (供调试或扩展使用) */
void INA3221_SendData(uint8_t addr, uint8_t reg, uint16_t data);
uint16_t INA3221_ReadData(uint8_t addr, uint8_t reg);

#endif
