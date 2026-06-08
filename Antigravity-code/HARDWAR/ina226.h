#ifndef INA226_H
#define INA226_H
#include "sys.h"

/********** 硬件参数 **********/
#define SHUNT_RESISTOR    0.100f           /* 采样电阻 100 毫欧 */
#define MAX_CURRENT       0.65536f         /* 最大量程电流 (A) */

/********** 校准参数 **********/
static float Current_LSB;                  /* 电流最小分辨率 (A/bit) */
static float Power_LSB;                    /* 功率最小分辨率 (W/bit) */

/* IIC 引脚定义 */
#define INA_SCL PAout(4)                   /* SCL 引脚 */
#define INA_SDA PAout(5)                   /* SDA 引脚 */

#define INA_SCL_GPIO_PORT	GPIOA          /* SCL 所在 GPIO 端口 */
#define INA_SCL_RCC 	    RCC_APB2Periph_GPIOA  /* SCL GPIO 端口时钟 */
#define INA_SCL_PIN		    GPIO_Pin_4     /* SCL 对应引脚 */

#define INA_SDA_GPIO_PORT	GPIOA          /* SDA 所在 GPIO 端口 */
#define INA_SDA_RCC 	    RCC_APB2Periph_GPIOA  /* SDA GPIO 端口时钟 */
#define INA_SDA_PIN		    GPIO_Pin_5     /* SDA 对应引脚 */

#define READ_SDA GPIO_ReadInputDataBit(INA_SDA_GPIO_PORT,INA_SDA_PIN)

/* IIC 地址定义 (7位地址左移1位) */
#define READ_ADDR         0x81             /* A0=GND, A1=GND, 读地址 */
#define WRITE_ADDR        0x80             /* A0=GND, A1=GND, 写地址 */

#define READ_ADDR1         0x8B            /* A0=VCC, A1=VCC, 读地址 */
#define WRITE_ADDR1       0x8A             /* A0=VCC, A1=VCC, 写地址 */

/* INA226 寄存器地址 */
#define Config_Reg        0x00             /* 配置寄存器 */
#define Shunt_V_Reg       0x01             /* Shunt 电压寄存器: 2.5uV/bit, 0.1R shunt max ~819mA */
#define Bus_V_Reg         0x02             /* 总线电压寄存器: 1.25mV/bit, 范围 0~40.96V */
#define Power_Reg         0x03             /* 功率寄存器: Current_LSB * 25 */
#define Current_Reg       0x04             /* 电流寄存器: LSB = 0.02mA */
#define Calib_Reg         0x05             /* 校准寄存器, 取决于采样电阻和 Current_LSB */
#define Mask_En_Reg       0x06             /* 告警屏蔽/使能寄存器 */
#define Alert_Reg         0x07             /* 告警限值寄存器 */
#define Man_ID_Reg        0xFE             /* 制造商 ID: 0x5449 */
#define ID_Reg            0xFF             /* 芯片 ID: 0x2260 */

/* 函数声明 */
void INA226_Init(void);
void INA226_Init1(void);
void INA226_SendData(uint8_t addr, uint8_t reg, uint16_t data);
uint16_t INA226_Get_ID(uint8_t addr);
uint16_t INA226_GetShuntVoltage(uint8_t addr);
uint16_t INA226_GetShuntCurrent(uint8_t addr);
uint16_t INA226_GetVoltage(uint8_t addr);
uint16_t INA226_Get_Power(uint8_t addr);

float INA226_ReadCurrent_A(uint8_t addr);
float INA226_ReadCurrent_mA(uint8_t addr);
float INA226_ReadPower(uint8_t addr);

#endif
