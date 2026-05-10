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

#define INA3221_CH1_CHARGE_NEGATIVE 1
#define INA3221_CH1_CURRENT_DEADZONE_A 0.02f
#define INA3221_BATTERY_CAPACITY_AH 1.2f
#define INA3221_SOC_UPDATE_PERIOD_H (0.1f / 3600.0f)
#define INA3221_SOC_IDLE_VOLTAGE_BLEND 0.02f
#define INA3221_SOC_IDLE_CALIBRATE_COUNT 50
#define INA3221_SOC_INIT_MIN_VOLTAGE 3.0f
#define INA3221_CHARGE_VOLTAGE_COMPENSATION 0.18f
#define INA3221_FULL_CHARGE_VOLTAGE 4.18f
#define INA3221_FULL_CHARGE_CURRENT_A 0.10f
#define INA3221_BATTERY_SLOT_COUNT 2

/*
 * ============================================================
 * 数据结构
 * ============================================================
 */
typedef enum {
  INA3221_BATTERY_IDLE = 0,
  INA3221_BATTERY_CHARGING,
  INA3221_BATTERY_DISCHARGING
} INA3221_BatteryState;

typedef struct {
  float ch1_voltage;   /* CH1电池端电压（V） */
  float ch1_current;   /* CH1电池端电流（A） */
  float ch2_voltage;   /* CH2 MPPT端电压（V） */
  float ch2_current;   /* CH2 MPPT端电流（A） */
  float ch3_voltage;   /* CH3 降压端电压（V） */
  float ch3_current;   /* CH3 降压端电流（A） */
  float ch1_signed_current;
  float battery_charge_current;
  float battery_discharge_current;
  float battery_soc_estimated;
  INA3221_BatteryState battery_current_state;
  uint8_t battery_soc_initialized;
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
void INA3221_SetActiveBattery(uint8_t battery_id);
uint8_t Calculate_Battery_SOC(float voltage);

/* 寄存器底层读写接口 (供调试或扩展使用) */
void INA3221_SendData(uint8_t addr, uint8_t reg, uint16_t data);
uint16_t INA3221_ReadData(uint8_t addr, uint8_t reg);

#endif
