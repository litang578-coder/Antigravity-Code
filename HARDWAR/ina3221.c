#include "ina3221.h"
#include "delay.h"

/*
 * ============================================================
 * 全局变量
 * ============================================================
 */
INA3221_Data g_ina3221 = {0};
static uint8_t g_sensor_addr = 0x80;

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
  INA3221_IIC_Wait_Ack();
  INA3221_IIC_Send_Byte((uint8_t)(data >> 8));
  INA3221_IIC_Wait_Ack();
  INA3221_IIC_Send_Byte((uint8_t)(data & 0x00FF));
  INA3221_IIC_Wait_Ack();
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
  INA3221_IIC_Wait_Ack();
  INA3221_IIC_Stop();
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

/*
 * ============================================================
 * 初始化与应用
 * ============================================================
 */

void INA3221_Init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  uint8_t addrs[] = {0x80, 0x82, 0x84, 0x86};
  uint8_t i;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  INA3221_SCL = 1;
  INA3221_SDA = 1;
  delay_ms(100);

  for (i = 0; i < 4; i++) {
    uint16_t id = INA3221_ReadData(addrs[i], INA3221_REG_MANUF_ID);
    if (id == 0x5449) {
      g_sensor_addr = addrs[i];
      break;
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
  raw = INA3221_ReadData(g_sensor_addr, INA3221_REG_CH1_BUS);
  if (raw != 0xFFFF)
    g_ina3221.ch1_voltage = (float)((int16_t)raw >> 3) * 0.008f;
  raw = INA3221_ReadData(g_sensor_addr, INA3221_REG_CH1_SHUNT);
  if (raw != 0xFFFF) {
    new_val = (float)((int16_t)raw >> 3) * 0.00004f / 0.1f;
    // 软件一阶滤波，让显示更平滑
    g_ina3221.ch1_current = g_ina3221.ch1_current * 0.7f + new_val * 0.3f;
  }

  // CH2 (MPPT)
  raw = INA3221_ReadData(g_sensor_addr, INA3221_REG_CH2_BUS);
  if (raw != 0xFFFF)
    g_ina3221.ch2_voltage = (float)((int16_t)raw >> 3) * 0.008f;
  raw = INA3221_ReadData(g_sensor_addr, INA3221_REG_CH2_SHUNT);
  if (raw != 0xFFFF) {
    new_val = (float)((int16_t)raw >> 3) * 0.00004f / 0.1f;
    g_ina3221.ch2_current = g_ina3221.ch2_current * 0.7f + new_val * 0.3f;
  }

  // CH3 (输出)
  raw = INA3221_ReadData(g_sensor_addr, INA3221_REG_CH3_BUS);
  if (raw != 0xFFFF)
    g_ina3221.ch3_voltage = (float)((int16_t)raw >> 3) * 0.008f;
  raw = INA3221_ReadData(g_sensor_addr, INA3221_REG_CH3_SHUNT);
  if (raw != 0xFFFF) {
    new_val = (float)((int16_t)raw >> 3) * 0.00004f / 0.1f;
    g_ina3221.ch3_current = g_ina3221.ch3_current * 0.7f + new_val * 0.3f;
  }

  // SOC 计算与平滑
  uint8_t new_soc = Calculate_Battery_SOC(g_ina3221.ch1_voltage);
  if (g_ina3221.battery_soc == 0)
    g_ina3221.battery_soc = new_soc; // 首次赋值
  else {
    // 防止SOC频繁跳变，只有当电压稳定变化时才更新
    static uint8_t soc_filter_cnt = 0;
    if (new_soc != g_ina3221.battery_soc) {
      if (++soc_filter_cnt >= 5) { // 连续5次读数一致才更新
        g_ina3221.battery_soc = new_soc;
        soc_filter_cnt = 0;
      }
    } else {
      soc_filter_cnt = 0;
    }
  }
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