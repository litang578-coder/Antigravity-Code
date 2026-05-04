
#include "DS1302.h"
#include "delay.h"	
#include "usart.h"
//*****************DS1302控制命令*******************
#define WRITE_SECOND              0x80
#define WRITE_MINUTE              0x82
#define WRITE_HOUR                0x84
#define WRITE_DAY                 0x86
#define WRITE_MONTH               0x88
#define WRITE_WEEK                0x8A
#define WRITE_YEAR                0x8C
#define WRITE_TIMER_FLAG          0xC0

#define READ_SECOND               0x81
#define READ_MINUTE               0x83
#define READ_HOUR                 0x85
#define READ_DAY                  0x87
#define READ_MONTH                0x89
#define READ_WEEK                 0x8B
#define READ_YEAR                 0x8D
#define READ_TIMER_FLAG           0xC1
#define WRITE_PROTECT             0x8E
_calendar_obj calendar;							//时钟结构体 
_next_obj next;                               		
//月份数据表											 
u8 const table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5}; //月修正数据表	  
extern u16 rtctime;			//时间
u8 BCD2HEX(u8 bcd_data)    //BCDtoHEX   
{   
    u8 temp;   
    temp=(bcd_data/16*10 + bcd_data%16);   
    return temp;   
}   
u8 HEX2BCD(u8 hex_data)    //HEXtoBCD    
{   
    u8 temp;   
    temp=(hex_data/10*16 + hex_data%10);   
    return temp;   
}   
//============================================
//函数名称：void Ds1302_Write_Byte (byte addr, byte dat)  
//功能：    串行发送地址、数据，先发低位，且在上升沿发送
//参数传递：有，地址和数据
//返回值：  无
//===========================================
void Ds1302_Write_Byte(u8 addr, u8 dat)     
{
	u8 i;
  DS1302_IO_OUT();                //数据端口定义为输出
  CE = 1;
	delay_us(10);
	SCLK = 0;
	delay_us(10);
	for(i=0;i<8;i++) 
	{ 
		if(addr&0x01) 
		{
			DIO = 1;
		}
		else 
		{
			DIO = 0;
		}
		addr = addr>>1;
		SCLK = 1;
		delay_us(10);
		SCLK = 0;
		delay_us(10);

	} 
	for(i=0;i<8;i++)  			//写入数据：dat
  {
		if(dat&0x01) 
		{
			DIO = 1;
		}
		else 
		{
			DIO = 0;
		}
		dat = dat>>1;
		SCLK = 1;
		delay_us(10);
		SCLK = 0;
		delay_us(10);
  }
  CE = 0;;                     //停止DS1302总线
	delay_us(10);
}
//===============================================
//函数名称：byte Ds1302_Read_Byte ( byte addr )
//功能：    串行读取数据，先发低位，且在下降沿发送
//参数传递：有，地址
//返回值：  有，读取的数据
//===============================================
u8 Ds1302_Read_Byte(u8 addr)
{
  u8  i;
  u8  temp = 1;
	CE = 1;
	delay_us(10);
  for(i=0;i<8;i++) 
  {      
    SCLK = 0;
		delay_us(10);
		if(addr&0x01) 
    {
       DIO = 1;
    }
    else 
    {
       DIO = 0;
    }
		addr = addr>>1;
		SCLK = 1;
		delay_us(10);
  }
  DS1302_IO_IN();                  //数据端口定义为输入
  for(i=0;i<8;i++) 
  {
    temp = temp >> 1;							 //输出数据：temp
    SCLK = 0;
		delay_us(10);
		if(DIO_IN) 
    {
       temp |= 0x80;
    }
		SCLK = 1;
		delay_us(10);
  }
  DS1302_IO_OUT();                //数据端口定义为输出                       
  SCLK = 0;
	delay_us(10);
	CE = 0;                         //停止DS1302总线
	delay_us(10);
	return temp;                    
}
//获得现在是星期几
//功能描述:输入公历日期得到星期(只允许1901-2099年)
//输入参数：公历年月日 
//返回值：星期号																						 
u8 RTC_Get_Week(u16 year,u8 month,u8 day)
{	
	u16 temp2;
	u8 yearH,yearL;	
	yearH=year/100;	
	yearL=year%100; 									//如果为21世纪,年份数加100  
	if (yearH>19)yearL+=100;					//所过闰年数只算1900年之后的  
	temp2=yearL+yearL/4;
	temp2=temp2%7; 
	temp2=temp2+day+table_week[month-1];
	if (yearL%4==0&&month<3)temp2--;
	return(temp2%7);
}			  
//===============================================
//           向DS1302写入时钟数据
//===============================================
void Ds1302_Time_Set(u16 year,u8 mon,u8 day,u8 hour,u8 min,u8 sec) 
{    
	u8 WR_week;
	u8 WR_yearL = 0;
	if(year>=2000)
	{
		WR_yearL = year - 2000;
	}
	else
		WR_yearL = year;
	WR_week = RTC_Get_Week(year,mon,day);						 //根据写入的日期算星期几
	Ds1302_Write_Byte(WRITE_PROTECT,0x00);           //关闭写保护 
	Ds1302_Write_Byte(WRITE_SECOND,0x80);            //暂停 
//	Ds1302_Write_Byte(ds1302_charger_add,0xa9);    //涓流充电 
	Ds1302_Write_Byte(WRITE_YEAR,HEX2BCD(WR_yearL)); //年 
	Ds1302_Write_Byte(WRITE_MONTH,HEX2BCD(mon));     //月 
	Ds1302_Write_Byte(WRITE_DAY,HEX2BCD(day));       //日 
	Ds1302_Write_Byte(WRITE_HOUR,HEX2BCD(hour));     //时 
	Ds1302_Write_Byte(WRITE_MINUTE,HEX2BCD(min));    //分
	Ds1302_Write_Byte(WRITE_SECOND,HEX2BCD(sec));    //秒
	Ds1302_Write_Byte(WRITE_WEEK,HEX2BCD(WR_week));  //周 
	Ds1302_Write_Byte(WRITE_PROTECT,0x80);           //打开写保护 
}

//========================================
//           从DS1302读出时钟数据
//========================================
void Ds1302_Time_Get(void)  
{ 
//	u8  i,tmp;
	calendar.w_year  = BCD2HEX(Ds1302_Read_Byte(READ_YEAR)); 					//年 
	calendar.w_month = BCD2HEX(Ds1302_Read_Byte(READ_MONTH));         //月 
	calendar.w_date  = BCD2HEX(Ds1302_Read_Byte(READ_DAY));           //日 
	calendar.hour    = BCD2HEX(Ds1302_Read_Byte(READ_HOUR));          //时 
	calendar.min     = BCD2HEX(Ds1302_Read_Byte(READ_MINUTE));        //分 
	calendar.sec     = BCD2HEX(Ds1302_Read_Byte(READ_SECOND)&0x7F); 	//秒 
//	calendar.week    = BCD2HEX(Ds1302_Read_Byte(READ_WEEK));          //周 
//	calendar.w_year  = calendar.w_year+2000;
}

//==========================================
//              DS1302初始化
//==========================================
void Ds1302_Init(void)
{ 
	DS1302_IO_Init();					   							//GPIO初始化	
  CE = 0;                                    		//RST脚置低
  SCLK = 0;                                  		//SCK脚置低
	Ds1302_Write_Byte(WRITE_SECOND,0x00);      		//开始  
  //Ds1302_Time_Set(2023,12,15,19,22,0) ;  //将注释取消，更改成功之后重新注销
	      //年月日时分秒
	Ds1302_Write_Byte(WRITE_PROTECT,0x80);           //打开写保护 
}
//*******************以下UTC时间计算部分函数*****************
//判断是否是闰年函数
//月份   1  2  3  4  5  6  7  8  9  10 11 12
//闰年   31 29 31 30 31 30 31 31 30 31 30 31
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
u8 Is_Leap_Year(u16 year)
{			  
	if(year%4==0) 																//必须能被4整除
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1;									//如果以00结尾,还要能被400整除 	   
			else return 0;   
		}else return 1;   
	}else return 0;	
}	 			   

 void DS1302_IO_Init(void)
{	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  

	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//使能PA,PB,PC端口时钟

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 													//IO口速度为50MHz	
	//PC端口初始化	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;						//设置PC10~PC12端口推挽输出
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP; 		 												//推挽输出	
	GPIO_Init(GPIOB, &GPIO_InitStructure);																							
}
// void DS1302_IO_Init(void)
//{	
//	GPIO_InitTypeDef  GPIO_InitStructure;
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能PA,PB,PC端口时钟
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;		 													//IO口速度为50MHz	
//	//PC端口初始化	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6;						//设置PC10~PC12端口推挽输出
//	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP; 		 												//推挽输出	
//	GPIO_Init(GPIOA, &GPIO_InitStructure);																							
//}



   				


