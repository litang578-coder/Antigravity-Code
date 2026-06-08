#include "ina226.h"

/* 纳秒级延时 (约 30ns/次, IIC 标准模式无需 HS 模式) */
static void delay_nns(uint16_t D)
{
    while(--D);
}

/* 毫秒级延时 */
void delay_nms(uint16_t ms)
{
    uint16_t i;
    uint32_t M = 0;
    for(i = 0;i < ms; i++)
    for(M=12000;M > 0;M--);
}

/* 微秒级延时 */
void delay_nus(uint16_t us)
{
    uint16_t i;
    uint16_t M = 0;
    for(i = 0;i < us; i++)
    for(M=72;M > 0;M--);
}

/**************** IIC 初始化 ***************************/
void INA226_IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(INA_SCL_RCC|INA_SDA_RCC, ENABLE);	/* 开启 GPIO 时钟 */
    
    GPIO_InitStructure.GPIO_Pin=INA_SCL_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  
		//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  /* 开漏模式需外接 SCL,SDA 10k 上拉至 VCC */
    GPIO_Init(INA_SCL_GPIO_PORT, &GPIO_InitStructure);
	
	  GPIO_InitStructure.GPIO_Pin=INA_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  
		//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  /* 开漏模式需外接 SCL,SDA 10k 上拉至 VCC */
    GPIO_Init(INA_SDA_GPIO_PORT, &GPIO_InitStructure);
    
    INA_SCL=1;
    INA_SDA=1;  
    delay_nms(5);
}

/**********************IIC_END*************************/

/**
 * @brief INA226 初始化 (地址 0x80, A0=GND, A1=GND)
 */
void INA226_Init(void)
{
    uint16_t Cal;

    INA226_IIC_Init();
    delay_nms(10);

    INA226_SendData(WRITE_ADDR, 0x00, 0x4807);

    /*
     * INA226 shunt voltage LSB is 2.5uV, full scale is 81.92mV.
     * With a 0.1R shunt, hardware full scale is about 819mA.
     * Current_LSB = 0.02mA keeps expected current within 655.36mA.
     * Cal = 0.00512 / (0.00002 * 0.1) = 2560 = 0x0A00.
     */
    Current_LSB = 0.00002f;
    Cal = (uint16_t)(0.00512f / (Current_LSB * SHUNT_RESISTOR) + 0.5f);
    Power_LSB = Current_LSB * 25.0f;

    INA226_SendData(WRITE_ADDR, 0x05, Cal);
}

/**
 * @brief INA226 初始化 (地址 0x8A, A0=VCC, A1=VCC)
 */
void INA226_Init1(void)
{
		uint16_t Cal ;
    INA226_IIC_Init();
    delay_nms(10);
 
	  INA226_SendData(WRITE_ADDR1,0x00,0x4807); /* 配置: 128 次平均, 电压转换时间 0.14ms, 连续模式 */
	 /* 校准值计算 */
	 Current_LSB=(float)MAX_CURRENT/32768;
	 //Current_LSB = 0.000305f;		/* 即 MAX_CURRENT / 2^15 */
   Cal = (uint16_t)(0.00512f / (Current_LSB * SHUNT_RESISTOR) + 0.5f);
   INA226_SendData(WRITE_ADDR1,0x05,Cal);
	
	Power_LSB = Current_LSB * 25.0f;		/* 功率 LSB = 电流 LSB * 25 */
 
}
    
/* IIC 起始信号 */
void INA226_IIC_Start(void)
{
    INA_SCL=1;
    INA_SDA=1;
    delay_nns(5);
     INA_SDA=0; /* START: 当 CLK 为高时, SDA 从高变低 */
    delay_nns(5);
    INA_SCL=0; /* 钳住 I2C 总线, 准备发送或接收 */
    delay_nns(5);
}    

/* IIC 停止信号 */
void INA226_IIC_Stop(void)
{
    INA_SDA=0; /* STOP: 当 CLK 为高时, SDA 从低变高 */
     delay_nns(5);
    INA_SCL=1; 
     delay_nns(5);
    INA_SDA=1; /* 发送 I2C 总线结束信号 */
    delay_nns(5);                                   
}

/* IIC 应答信号 */
void INA226_IIC_Ack(void)
{
    INA_SDA=0;
    delay_nns(5);
    INA_SCL=1;
    delay_nns(5);
    INA_SCL=0;
    delay_nns(5);
    INA_SDA=1;
}

/* IIC 非应答信号 */
void INA226_IIC_NAck(void)
{
    INA_SDA=1;
    delay_nns(5);
    INA_SCL=1;
    delay_nns(5);
    INA_SCL=0;;
    delay_nns(5);
    INA_SDA=0;
}        

/* 等待应答信号, 返回 0 成功, 1 超时失败 */
uint8_t INA226_IIC_Wait_Ack(void)
{
    uint8_t ucErrTime=0;
   
    INA_SDA=1;
    delay_nns(5);       
    INA_SCL=1;
    delay_nns(5);          
    
    while(READ_SDA)
    {
        ucErrTime++;
        if(ucErrTime>250)
        {
            INA226_IIC_Stop();
            return 1;
        }
    }
    INA_SCL=0; /* 时钟输出 0 */
    return 0;  
}

/* IIC 发送一个字节 */
void INA226_IIC_Send_Byte(uint8_t txd)
{  int i;                           
   INA_SCL=0;; /* 拉低时钟开始数据传输 */
   for( i = 0;i < 8;i++)
    {              
        if(txd&0x80)
                    INA_SDA=1;
                else
                    INA_SDA=0;                
        txd<<=1;       
                
        INA_SCL=1;
        delay_nns(5);
        INA_SCL=0;
        delay_nns(5);
    }            
} 

/* IIC 读取一个字节, ack=1 发送应答, ack=0 发送非应答 */
uint8_t INA226_IIC_Read_Byte(unsigned char ack)
{
      uint8_t TData=0,i;
    for(i=0;i<8;i++)
    {
        INA_SCL=1;
        delay_nns(5);
        TData=TData<<1;
            if(READ_SDA)
        {
            TData|=0x01;
        }
        INA_SCL=0;;
        delay_nns(5);
    }
        if(!ack)
            INA226_IIC_NAck();
        else
            INA226_IIC_Ack();
    return TData;
}

/* 向指定寄存器写入 16 位数据 */
void INA226_SendData(uint8_t addr,uint8_t reg,uint16_t data)
{
    uint8_t temp = 0;
    INA226_IIC_Start();
    INA226_IIC_Send_Byte(addr);
    INA226_IIC_Wait_Ack();
 
    INA226_IIC_Send_Byte(reg);
    INA226_IIC_Wait_Ack();
    
    temp = (uint8_t)(data>>8);
    INA226_IIC_Send_Byte(temp);
    INA226_IIC_Wait_Ack();
 
    temp = (uint8_t)(data&0x00FF);
    INA226_IIC_Send_Byte(temp);
    INA226_IIC_Wait_Ack();
    
    INA226_IIC_Stop();
}

/* 设置寄存器指针 */
void INA226_SetRegPointer(uint8_t addr,uint8_t reg)
{
    INA226_IIC_Start();
 
    INA226_IIC_Send_Byte(addr);
    INA226_IIC_Wait_Ack();
 
    INA226_IIC_Send_Byte(reg);
    INA226_IIC_Wait_Ack();
 
    INA226_IIC_Stop();
}

/* 从指定地址读取 16 位数据 */
uint16_t INA226_ReadData(uint8_t addr)
{
    uint16_t temp=0;
    INA226_IIC_Start();
 
    INA226_IIC_Send_Byte(addr+1);
    INA226_IIC_Wait_Ack();
    
    temp = INA226_IIC_Read_Byte(1);
    temp<<=8;    
    temp |= INA226_IIC_Read_Byte(0);
    
    INA226_IIC_Stop();
    return temp;
}

/**
 * @brief 读取电流, 单位: A
 */
float INA226_ReadCurrent_A(uint8_t addr)
{
  uint16_t raw = INA226_GetShuntCurrent(addr);
  return raw * Current_LSB;  /* 乘以实际 LSB 值 */
}

/**
 * @brief 读取电流, 单位: mA
 */
float INA226_ReadCurrent_mA(uint8_t addr)
{
	float  Curren;
  uint16_t raw = INA226_GetShuntCurrent(addr);
	Curren=raw * Current_LSB;
	Curren*=1000;
  return Curren;  /* 乘以实际 LSB 值并转换为 mA */
}

/**
 * @brief 读取功率, 单位: W
 */
float INA226_ReadPower(uint8_t addr)
{
    uint16_t raw =  INA226_Get_Power( addr);
    return raw * Power_LSB;  /* 25 * 电流最小分辨率 */
}

/* 读取电流寄存器原始值 */
uint16_t INA226_GetShuntCurrent(uint8_t addr)
{
    uint16_t temp=0;    
    INA226_SetRegPointer(addr,Current_Reg);
    temp = INA226_ReadData(addr);
   if(temp&0x8000)    temp = ~(temp - 1); /* 负值取绝对值 */
      return temp;
}

/* 读取芯片 ID */
uint16_t  INA226_Get_ID(uint8_t addr)
{
    uint16_t temp=0;
    INA226_SetRegPointer(addr,ID_Reg);
    temp = INA226_ReadData(addr);
    return (uint16_t)temp;
}

/* 读取校准寄存器值 */
uint16_t INA226_GET_CAL_REG(uint8_t addr)
{    
    uint16_t temp=0;
    INA226_SetRegPointer(addr,Calib_Reg);
    temp = INA226_ReadData(addr);
    return temp;
}

/* 读取总线电压, 1.25mV/bit */
uint16_t INA226_GetVoltage(uint8_t addr)
{
    uint16_t temp=0;
    INA226_SetRegPointer(addr,Bus_V_Reg);
    temp = INA226_ReadData(addr);
    return temp;    
}

/* 读取 Shunt 电压, 2.5uV/bit */
uint16_t INA226_GetShuntVoltage(uint8_t addr)
{
	  uint16_t temp=0;
    INA226_SetRegPointer(addr,Shunt_V_Reg);
    temp = INA226_ReadData(addr);
    if(temp&0x8000)    temp = ~(temp - 1);    
    return temp;
}

/* 读取功率寄存器原始值, 2.5mW/bit */
uint16_t INA226_Get_Power(uint8_t addr)
{
    uint16_t temp=0;
    INA226_SetRegPointer(addr,Power_Reg);
    temp = INA226_ReadData(addr);
    return temp;
}
