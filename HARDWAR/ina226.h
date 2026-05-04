#ifndef INA226_H
#define INA226_H
#include "sys.h"
 
 
 
/********** 硬件参数配置 **********/
#define SHUNT_RESISTOR    0.010f				// 分流电阻10毫欧
#define MAX_CURRENT				10.0f					// 最大电流
 
///********** 软件参数 **********/
static float Current_LSB;								// 电流分辨率
static float Power_LSB;     						// 功率校准值
 
 
#define INA_SCL PAout(4)  //SCL引脚
#define INA_SDA PAout(5)  //SDA引脚
 
#define INA_SCL_GPIO_PORT	GPIOA			/* GPIO端口 */
#define INA_SCL_RCC 	    RCC_APB2Periph_GPIOA		/* GPIO端口时钟 */
#define INA_SCL_PIN		    GPIO_Pin_4		/* 连接到SCL时钟线的GPIO */
 
#define INA_SDA_GPIO_PORT	GPIOA			/* GPIO端口 */
#define INA_SDA_RCC 	    RCC_APB2Periph_GPIOA		/* GPIO端口时钟 */
#define INA_SDA_PIN		    GPIO_Pin_5		/* 连接到SDA数据线的GPIO */
 
#define READ_SDA GPIO_ReadInputDataBit(INA_SDA_GPIO_PORT,INA_SDA_PIN)
 
#define READ_ADDR         0x81	 	//A0=GND，A1=GND // R=1, W=0
#define WRITE_ADDR        0x80		//芯片IIC地址
 
#define READ_ADDR1         0x8B	 	//A0=VCC，A1=VCC // R=1, W=0
#define WRITE_ADDR1       0x8A		//芯片IIC地址
 
#define Config_Reg        0x00		//模式配置寄存器
#define Shunt_V_Reg       0x01		//采样电阻压差；取值：2.5uV~81.92mV(0~7FFF)(10mΩ采样电阻下最大电流8A)
#define Bus_V_Reg         0x02		//BUS总线电压：1.25mV~40.96V(0~7FFF)
#define Power_Reg         0x03		//功率；Current_LSB * 25，FSR = 0xFFFF
#define Current_Reg       0x04		//电流；LSB = 0.02mA，FSR = 0x7FFF
#define Calib_Reg         0x05		//（当单位为mA时：采样电阻0.002R=0xA00;0.01R=0x200）；写入Calibration Register的数据需要根据实际电路中的采样电阻阻值以及电流分辨率来设置
#define Mask_En_Reg       0x06		//
#define Alert_Reg         0x07		//
#define Man_ID_Reg        0xFE  	//0x5449
#define ID_Reg            0xFF  	//0x2260
 
 
void INA226_Init(void);
void INA226_Init1(void);
void INA226_SendData(uint8_t addr,uint8_t reg,uint16_t data);
uint16_t  INA226_Get_ID(uint8_t addr);
uint16_t INA226_GetShuntVoltage(uint8_t addr);
uint16_t INA226_GetShuntCurrent(uint8_t addr);
uint16_t INA226_GetVoltage(uint8_t addr);
uint16_t INA226_Get_Power(uint8_t addr);
 
float INA226_ReadCurrent_A(uint8_t addr);
float INA226_ReadCurrent_mA(uint8_t addr);
float INA226_ReadPower(uint8_t addr);
 
 
#endif

