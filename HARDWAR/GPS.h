#ifndef __GPS_H
#define __GPS_H

#include "stdio.h"
#include "sys.h" 

#define	SIZEBUF	(110)  //变量定义

extern unsigned char timebuf[SIZEBUF];//暂存串口数据
extern unsigned char clear;	 //串口buf清空计数
extern unsigned char count; 	 //串口接收计数
extern unsigned char readFlag;	//读取短信标志

extern unsigned char  sysmode_GPS;
extern unsigned char	gps_infor_weijing[17];
extern unsigned char rendFlag;
extern unsigned char	gps_infor_time[6];        //暂存时间
extern unsigned char	gps_infor_date[6];        //暂存日期
  
extern unsigned long Mid_Du;       //经纬度处理 度
extern unsigned long Mid_Fen;      //经纬度处理  分
extern unsigned long Mid_Vale;     //经纬度处理 中间变量

extern char Lin0_No[16];//存储纬度
extern char Lin1_Ea[16];//存储经度
extern char dis0[16];
extern char dis1[16];


extern unsigned long seco_Beijing;//时间转化变量 秒
extern unsigned long minu_Beijing;//时间转化变量 分
extern unsigned long hour_Beijing;//时间转化变量  小时
extern unsigned long days_Beijing;//时间转化变量  天
extern unsigned long mont_Beijing;//时间转化变量 月
extern unsigned long year_Beijing;//时间转化变量 年

extern unsigned char monthrun_table[13];//月份 天数 闰年
//1  2  3  4   5  6  7  8  9 10 11 12    
extern unsigned char monthmon_table[13];//月份天数
//1  2  3  4   5  6  7  8  9 10 11 12 



void dealGps(void);






#endif






