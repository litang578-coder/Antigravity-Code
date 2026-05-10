#include "ina3221.h"
#include "delay.h"

/*
 * ============================================================
 * 全局变量
 * ============================================================
 */
INA3221_Data g_ina3221 = {0};
static uint8_t g_sensor_addr = 0x80;
static uint8_t g_active_battery = 0;
static float g_battery_soc_estimated_slots[INA3221_BATTERY_SLOT_COUNT] = {0};
static uint8_t g_battery_soc_slots[INA3221_BATTERY_SLOT_COUNT] = {0};
static uint8_t g_battery_soc_initialized_slots[INA3221_BATTERY_SLOT_COUNT] = {0};
static uint8_t g_soc_idle_calib_cnt = 0;

/* 底层位带操作 */
#define INA3221_SCL PBout(4)
#define INA3221_SDA PBout(5)
#define INA3221_SDA_IN PBin(5)

/* 寄存器定义 */
#define INA3221_REG_CONFIG 0x00
#define INA3221_REG_CH1_SHUNT 0x01
#define INA3221_REG_CH1_BUS 0x02
#define INA3221_REG_CH2_SHUNT 0x03
#define INA3221_REG_CH2_BUS 0x04
#define INA3221_REG_CH3_SHUNT 0x05
#define INA3221_REG_CH3_BUS 0x06
#define INA3221_REG_MANUF_ID 0xFE
#define INA3221_READ_RETRY_COUNT 3
#define INA3221_MEASURE_PRECISION 1000.0f

static float INA3221_RoundMeasure(float value) {
  long scaled;

  if (value >= 0.0f)
    scaled = (long)(value * INA3221_MEASURE_PRECISION + 0.5f);
  else
    scaled = (long)(value * INA3221_MEASURE_PRECISION - 0.5f);

  return (float)scaled / INA3221_MEASURE_PRECISION;
}

static float INA3221_LimitSoc(float soc) {
  if (soc > 100.0f)
    return 100.0f;
  if (soc < 0.0f)
    return 0.0f;
  return soc;
}

static void INA3221_SaveBatterySocSlot(void) {
  g_battery_soc_estimated_slots[g_active_battery] = g_ina3221.battery_soc_estimated;
  g_battery_soc_slots[g_active_battery] = g_ina3221.battery_soc;
  g_battery_soc_initialized_slots[g_active_battery] =
      g_ina3221.battery_soc_initialized;
}

static void INA3221_LoadBatterySocSlot(void) {
  g_ina3221.battery_soc_estimated =
      g_battery_soc_estimated_slots[g_active_battery];
  g_ina3221.battery_soc = g_battery_soc_slots[g_active_battery];
  g_ina3221.battery_soc_initialized =
      g_battery_soc_initialized_slots[g_active_battery];
}

void INA3221_SetActiveBattery(uint8_t battery_id) {
  if (battery_id >= INA3221_BATTERY_SLOT_COUNT)
    battery_id = INA3221_BATTERY_SLOT_COUNT - 1;

  INA3221_SaveBatterySocSlot();
  if (g_active_battery == battery_id)
    return;

  g_active_battery = battery_id;
  INA3221_LoadBatterySocSlot();

  g_ina3221.ch1_signed_current = 0.0f;
  g_ina3221.ch1_current = 0.0f;
  g_ina3221.battery_charge_current = 0.0f;
  g_ina3221.battery_discharge_current = 0.0f;
  g_ina3221.battery_current_state = INA3221_BATTERY_IDLE;
  g_soc_idle_calib_cnt = 0;
}

static void INA3221_UpdateBatterySoc(void) {
  float voltage_soc;
  float voltage_for_soc;
  float delta_soc;

  voltage_for_soc = g_ina3221.ch1_voltage;
  if (g_ina3221.battery_current_state == INA3221_BATTERY_CHARGING) {
    voltage_for_soc -= INA3221_CHARGE_VOLTAGE_COMPENSATION;
    if (voltage_for_soc < 0.0f)
      voltage_for_soc = 0.0f;
  }

  voltage_soc = (float)Calculate_Battery_SOC(voltage_for_soc);
  if (!g_ina3221.battery_soc_initialized) {
    if (voltage_for_soc < INA3221_SOC_INIT_MIN_VOLTAGE) {
      g_ina3221.battery_soc = 0;
      return;
    }
    g_ina3221.battery_soc_estimated = voltage_soc;
    g_ina3221.battery_soc_initialized = 1;
  }

  if ((g_ina3221.battery_soc == 0) &&
      (g_ina3221.battery_soc_estimated <= 0.5f) &&
      (voltage_for_soc >= INA3221_SOC_INIT_MIN_VOLTAGE) &&
      (voltage_soc > 0.0f)) {
    g_ina3221.battery_soc_estimated = voltage_soc;
    g_ina3221.battery_soc_initialized = 1;
  }

  delta_soc =
      (g_ina3221.battery_charge_current - g_ina3221.battery_discharge_current) *
      INA3221_SOC_UPDATE_PERIOD_H / INA3221_BATTERY_CAPACITY_AH * 100.0f;
  g_ina3221.battery_soc_estimated =
      INA3221_LimitSoc(g_ina3221.battery_soc_estimated + delta_soc);

  if ((g_ina3221.battery_current_state == INA3221_BATTERY_CHARGING) &&
      (g_ina3221.ch1_voltage >= INA3221_FULL_CHARGE_VOLTAGE) &&
      (g_ina3221.battery_charge_current <= INA3221_FULL_CHARGE_CURRENT_A)) {
    g_ina3221.battery_soc_estimated = 100.0f;
    g_soc_idle_calib_cnt = 0;
  } else if (g_ina3221.battery_current_state == INA3221_BATTERY_IDLE) {
    if (g_soc_idle_calib_cnt < INA3221_SOC_IDLE_CALIBRATE_COUNT) {
      g_soc_idle_calib_cnt++;
    } else {
      g_ina3221.battery_soc_estimated =
          g_ina3221.battery_soc_estimated *
              (1.0f - INA3221_SOC_IDLE_VOLTAGE_BLEND) +
          voltage_soc * INA3221_SOC_IDLE_VOLTAGE_BLEND;
    }
  } else {
    g_soc_idle_calib_cnt = 0;
  }

  g_ina3221.battery_soc =
      (uint8_t)(INA3221_LimitSoc(g_ina3221.battery_soc_estimated) + 0.5f);
  INA3221_SaveBatterySocSlot();
}

/*
 * ============================================================
 * IIC 底层 (开漏模式)
 * ============================================================
 */

void INA3221_IIC_Start(void) {
  INA3221_SDA = 1;
  INA3221_SCL = 1;
  delay_us(5);
  INA3221_SDA = 0;
  delay_us(5);
  INA3221_SCL = 0;
  delay_us(5);
}

void INA3221_IIC_Stop(void) {
  INA3221_SDA = 0;
  delay_us(5);
  INA3221_SCL = 1;
  delay_us(5);
  INA3221_SDA = 1;
  delay_us(5);
}

static void INA3221_IIC_BusRecover(void) {
  uint8_t i;

  INA3221_SDA = 1;
  for (i = 0; i < 9; i++) {
    INA3221_SCL = 0;
    delay_us(5);
    INA3221_SCL = 1;
    delay_us(5);
  }
  INA3221_IIC_Stop();
}

void INA3221_IIC_Ack(void) {
  INA3221_SDA = 0;
  delay_us(5);
  INA3221_SCL = 1;
  delay_us(5);
  INA3221_SCL = 0;
  delay_us(5);
  INA3221_SDA = 1;
}

void INA3221_IIC_NAck(void) {
  INA3221_SDA = 1;
  delay_us(5);
  INA3221_SCL = 1;
  delay_us(5);
  INA3221_SCL = 0;
  delay_us(5);
}

uint8_t INA3221_IIC_Wait_Ack(void) {
  uint8_t ucErrTime = 0;
  INA3221_SDA = 1;
  delay_us(5);
  INA3221_SCL = 1;
  delay_us(5);
  while (INA3221_SDA_IN) {
    ucErrTime++;
    if (ucErrTime > 250) {
      INA3221_IIC_Stop();
      return 1;
    }
  }
  INA3221_SCL = 0;
  return 0;
}

void INA3221_IIC_Send_Byte(uint8_t txd) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    if (txd & 0x80)
      INA3221_SDA = 1;
    else
      INA3221_SDA = 0;
    txd <<= 1;
    delay_us(5);
    INA3221_SCL = 1;
    delay_us(5);
    INA3221_SCL = 0;
    delay_us(5);
  }
}

uint8_t INA3221_IIC_Read_Byte(unsigned char ack) {
  uint8_t i, receive = 0;
  INA3221_SDA = 1;
  for (i = 0; i < 8; i++) {
    delay_us(5);
    INA3221_SCL = 1;
    delay_us(5);
    receive <<= 1;
    if (INA3221_SDA_IN)
      receive |= 0x01;
    INA3221_SCL = 0;
  }
  if (!ack)
    INA3221_IIC_NAck();
  else
    INA3221_IIC_Ack();
  return receive;
}

void INA3221_SendData(uint8_t addr, uint8_t reg, uint16_t data) {
  INA3221_IIC_Start();
  INA3221_IIC_Send_Byte(addr);
  if (INA3221_IIC_Wait_Ack())
    return;
  INA3221_IIC_Send_Byte(reg);
  if (INA3221_IIC_Wait_Ack())
    return;
  INA3221_IIC_Send_Byte((uint8_t)(data >> 8));
  if (INA3221_IIC_Wait_Ack())
    return;
  INA3221_IIC_Send_Byte((uint8_t)(data & 0x00FF));
  if (INA3221_IIC_Wait_Ack())
    return;
  INA3221_IIC_Stop();
}

uint16_t INA3221_ReadData(uint8_t addr, uint8_t reg) {
  uint16_t temp = 0;
  INA3221_IIC_Start();
  INA3221_IIC_Send_Byte(addr);
  if (INA3221_IIC_Wait_Ack()) {
    INA3221_IIC_Stop();
    return 0xFFFF;
  }
  INA3221_IIC_Send_Byte(reg);
  if (INA3221_IIC_Wait_Ack()) {
    INA3221_IIC_Stop();
    return 0xFFFF;
  }
  delay_us(5);
  INA3221_IIC_Start();
  INA3221_IIC_Send_Byte(addr | 0x01);
  if (INA3221_IIC_Wait_Ack()) {
    INA3221_IIC_Stop();
    return 0xFFFF;
  }
  temp = INA3221_IIC_Read_Byte(1);
  temp <<= 8;
  temp |= INA3221_IIC_Read_Byte(0);
  INA3221_IIC_Stop();
  return temp;
}

static uint8_t INA3221_ReadRegRetry(uint8_t reg, uint16_t *data) {
  uint8_t i;
  uint16_t raw;

  for (i = 0; i < INA3221_READ_RETRY_COUNT; i++) {
    raw = INA3221_ReadData(g_sensor_addr, reg);
    if (raw != 0xFFFF) {
      *data = raw;
      return 1;
    }

    INA3221_IIC_BusRecover();
    delay_us(50);
  }

  return 0;
}

/*
 * ============================================================
 * 初始化与应用
 * ============================================================
 */

void INA3221_Init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  uint8_t addrs[] = {0x80, 0x82, 0x84, 0x86};
  uint8_t i;
  uint8_t retry;
  uint16_t id;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  INA3221_SCL = 1;
  INA3221_SDA = 1;
  INA3221_IIC_BusRecover();
  delay_ms(100);

  for (i = 0; i < 4; i++) {
    for (retry = 0; retry < INA3221_READ_RETRY_COUNT; retry++) {
      id = INA3221_ReadData(addrs[i], INA3221_REG_MANUF_ID);
      if (id == 0x5449) {
        g_sensor_addr = addrs[i];
        i = 4;
        break;
      }
      INA3221_IIC_BusRecover();
      delay_us(50);
    }
  }

  INA3221_SendData(g_sensor_addr, INA3221_REG_CONFIG, 0x8000);
  delay_ms(10);

  /*
   * 配置寄存器设置：0x7927
   * - 0x7...: 使能 CH1, CH2, CH3
   * - ...9..: AVG位为100 -> 128次硬件平均（显著抑制电流跳变）
   * - ...27: 1.1ms 采样时间，连续模式
   */
  INA3221_SendData(g_sensor_addr, INA3221_REG_CONFIG, 0x7927);
}

void INA3221_ReadAll(void) {
  uint16_t raw;
  float new_val;

  // CH1 (电池)
  if (INA3221_ReadRegRetry(INA3221_REG_CH1_BUS, &raw))
    g_ina3221.ch1_voltage =
        INA3221_RoundMeasure((float)((int16_t)raw >> 3) * 0.008f);
  if (INA3221_ReadRegRetry(INA3221_REG_CH1_SHUNT, &raw)) {
    new_val = (float)((int16_t)raw >> 3) * 0.00004f / 0.1f;
    // 软件一阶滤波，让显示更平滑
    g_ina3221.ch1_signed_current =
        INA3221_RoundMeasure(g_ina3221.ch1_signed_current * 0.7f +
                             new_val * 0.3f);
    g_ina3221.ch1_current = g_ina3221.ch1_signed_current;
  }

#if INA3221_CH1_CHARGE_NEGATIVE
  if (g_ina3221.ch1_signed_current < -INA3221_CH1_CURRENT_DEADZONE_A) {
    g_ina3221.battery_current_state = INA3221_BATTERY_CHARGING;
    g_ina3221.battery_charge_current = -g_ina3221.ch1_signed_current;
    g_ina3221.battery_discharge_current = 0.0f;
  } else if (g_ina3221.ch1_signed_current > INA3221_CH1_CURRENT_DEADZONE_A) {
    g_ina3221.battery_current_state = INA3221_BATTERY_DISCHARGING;
    g_ina3221.battery_charge_current = 0.0f;
    g_ina3221.battery_discharge_current = g_ina3221.ch1_signed_current;
  } else {
    g_ina3221.battery_current_state = INA3221_BATTERY_IDLE;
    g_ina3221.battery_charge_current = 0.0f;
    g_ina3221.battery_discharge_current = 0.0f;
  }
#else
  if (g_ina3221.ch1_signed_current > INA3221_CH1_CURRENT_DEADZONE_A) {
    g_ina3221.battery_current_state = INA3221_BATTERY_CHARGING;
    g_ina3221.battery_charge_current = g_ina3221.ch1_signed_current;
    g_ina3221.battery_discharge_current = 0.0f;
  } else if (g_ina3221.ch1_signed_current < -INA3221_CH1_CURRENT_DEADZONE_A) {
    g_ina3221.battery_current_state = INA3221_BATTERY_DISCHARGING;
    g_ina3221.battery_charge_current = 0.0f;
    g_ina3221.battery_discharge_current = -g_ina3221.ch1_signed_current;
  } else {
    g_ina3221.battery_current_state = INA3221_BATTERY_IDLE;
    g_ina3221.battery_charge_current = 0.0f;
    g_ina3221.battery_discharge_current = 0.0f;
  }
#endif

  // CH2 (MPPT)
  if (INA3221_ReadRegRetry(INA3221_REG_CH2_BUS, &raw))
    g_ina3221.ch2_voltage =
        INA3221_RoundMeasure((float)((int16_t)raw >> 3) * 0.008f);
  if (INA3221_ReadRegRetry(INA3221_REG_CH2_SHUNT, &raw)) {
    new_val = (float)((int16_t)raw >> 3) * 0.00004f / 0.1f;
    g_ina3221.ch2_current =
        INA3221_RoundMeasure(g_ina3221.ch2_current * 0.7f + new_val * 0.3f);
  }

  // CH3 (输出)
  if (INA3221_ReadRegRetry(INA3221_REG_CH3_BUS, &raw))
    g_ina3221.ch3_voltage =
        INA3221_RoundMeasure((float)((int16_t)raw >> 3) * 0.008f);
  if (INA3221_ReadRegRetry(INA3221_REG_CH3_SHUNT, &raw)) {
    new_val = (float)((int16_t)raw >> 3) * 0.00004f / 0.1f;
    g_ina3221.ch3_current =
        INA3221_RoundMeasure(g_ina3221.ch3_current * 0.7f + new_val * 0.3f);
  }

  INA3221_UpdateBatterySoc();

}

uint8_t Calculate_Battery_SOC(float voltage) {
  static const float v_t[] = {4.2f, 4.1f, 4.0f, 3.9f, 3.8f,
                              3.7f, 3.6f, 3.4f, 3.2f, 3.0f};
  static const uint8_t s_t[] = {100, 90, 80, 70, 55, 40, 25, 10, 5, 0};
  if (voltage >= 4.15f)
    return 100;
  if (voltage <= 3.1f)
    return 0;
  for (uint8_t i = 0; i < 9; i++) {
    if (voltage >= v_t[i + 1]) {
      float ratio = (voltage - v_t[i + 1]) / (v_t[i] - v_t[i + 1]);
      return (uint8_t)(s_t[i + 1] + ratio * (s_t[i] - s_t[i + 1]));
    }
  }
  return 0;
}
