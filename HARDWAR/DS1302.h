#ifndef __DS1302_H
#define __DS1302_H
#include "sys.h"

//-------------------------------------------------------------------------*
//文件名:  DS1302.h (实时时钟头文件)                                          *
//-------------------------------------------------------------------------*

//IO方向设置
//#define DS1302_IO_IN()  {GPIOA->CRL&=0xFF0FFFFF;GPIOA->CRL|=0x00800000;}				//低八位引脚的PB14脚定义为输入
//#define DS1302_IO_OUT() {GPIOA->CRL&=0xFF0FFFFF;GPIOA->CRL|=0x00300000;}  	   	//低八位引脚的PB14脚定义为输出
////IO操作函数											   
//#define	DIO_OUT PBout(5) 			//数据端口	
//#define	DIO_IN  PBin(5)  			//数据端口	
//#define CE 					PBout(4)			//RST
//#define DIO					PBout(5)			//DATA
//#define SCLK 				PBout(6)			//CLK

//IO方向设置
#define DS1302_IO_IN()  {GPIOB->CRL&=0xFFF0FFFF;GPIOB->CRL|=0x00080000;}				//低八位引脚的PB14脚定义为输入
#define DS1302_IO_OUT() {GPIOB->CRL&=0xFFF0FFFF;GPIOB->CRL|=0x00030000;}  	   	//低八位引脚的PB14脚定义为输出
//IO操作函数											   
#define	DIO_OUT PBout(4) 			//数据端口	
#define	DIO_IN  PBin(4)  			//数据端口	
#define SCLK 				PBout(3)			//CLK
#define DIO					PBout(4)			//DATA
#define CE 					PBout(5)			//RST



typedef struct 
{
	u8  sec;
	u8  min;
	u8  hour;
	u8  day;
	u8  mon;
  u16 year;
	u8  week;
}_next_obj;	
extern _next_obj next;
typedef struct 
{
	vu8 hour;
	vu8 min;
	vu8 sec;			
	//公历日月年周
	vu16 w_year;
	vu8  w_month;
	vu8  w_date;
	vu8  week;		 
}_calendar_obj;	
extern _calendar_obj calendar;	//日历结构体

extern u32 RTC_sec_sum;														//当前时间的总秒值
extern u32 Program_sec_sum;												//当前编程任务的总秒值,与RTC_sec_sum进行比较
void Ds1302_Time_Set(u16 year,u8 mon,u8 day,u8 hour,u8 min,u8 sec);
void Ds1302_Time_Get(void);
void NEXT_Date(u8 day);
void DS1302_IO_Init(void);

u8 RTC_Pro_count(u16 syear,u8 smon,u8 sday,u8 hour,u8 min,u8 sec,u8 mode);//编程任务时间计算
u8 Pro_Get_time(u32 ttt);								//编程模式无效时间时计算下次开始的日期
void Ds1302_Init(void);

#endif	
