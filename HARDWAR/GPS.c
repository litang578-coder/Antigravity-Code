#include "GPS.h"


#define STAGE_SOHE  0x01
#define STAGE_TYPE  0x02
#define STAGE_NONE  0x03
#define STAGE_DATA  0x04

unsigned char	devide_flag;		        //GPS数据逗号分隔符标志
unsigned char	speed_end;			//收到速度数据结束标志
unsigned char	dir_end;			//收到方向数据结束标志
unsigned char  sysmode_GPS=0;                    //gps有无效标志
unsigned char  ew_flag;                        //东西标志
unsigned char  ns_flag;                        //南北标志

unsigned char	gps_infor_weijing[17];    //经纬度暂存 格式是度分秒形式
unsigned char	gps_infor_speed[4];       //暂存速度
unsigned char	gps_infor_time[6];        //暂存时间
unsigned char	gps_infor_date[6];        //暂存日期
unsigned char	gps_infor_dir[3];         //暂存方向

unsigned char recv1_step=STAGE_SOHE;                       //串口接收指令步骤
unsigned char uart1_r_buf;                       //串口缓存
unsigned char rev1_buf_busy;                    //串口判忙
unsigned char temp1_buf[85];                   //串口接收数组
unsigned int record1=0;   
unsigned char rendFlag=0;
  
unsigned long Mid_Du;       //经纬度处理 度
unsigned long Mid_Fen;      //经纬度处理  分
unsigned long Mid_Vale;     //经纬度处理 中间变量

char Lin0_No[16]="N:000.000000";//存储纬度
char Lin1_Ea[16]="E:000.000000";//存储经度
char dis0[16]="2000-00-00   ";
char dis1[16]="00:00:00     ";


unsigned long seco_Beijing;//时间转化变量 秒
unsigned long minu_Beijing;//时间转化变量 分
unsigned long hour_Beijing;//时间转化变量  小时
unsigned long days_Beijing;//时间转化变量  天
unsigned long mont_Beijing;//时间转化变量 月
unsigned long year_Beijing;//时间转化变量 年

unsigned char monthrun_table[13]={0,31,29,31,30,31,30,31,31,30,31,30,31};//月份 天数 闰年
//1  2  3  4   5  6  7  8  9 10 11 12    
unsigned char monthmon_table[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};//月份天数
//1  2  3  4   5  6  7  8  9 10 11 12 

void dealGps(void)
{		
//  seco_Beijing=(gps_infor_time[4]-0x30)*10+(gps_infor_time[5]-0x30);//提取时间 秒   
//  minu_Beijing=(gps_infor_time[2]-0x30)*10+(gps_infor_time[3]-0x30);//提取时间 分
//  hour_Beijing=((gps_infor_time[0]-0x30)*10+(gps_infor_time[1]-0x30))+8;//提取时间 小时   
//  days_Beijing=(gps_infor_date[0]-0x30)*10+(gps_infor_date[1]-0x30);//提取时间 天  
//  mont_Beijing=(gps_infor_date[2]-0x30)*10+(gps_infor_date[3]-0x30);//提取时间 月   
//  year_Beijing=(gps_infor_date[4]-0x30)*10+(gps_infor_date[5]-0x30);//提取时间 年   
//  if(hour_Beijing>=24)	//北京时间转换
//  {
//    hour_Beijing=hour_Beijing%24;
//    days_Beijing=days_Beijing+1;
//    if(year_Beijing%4==0)            
//    {
//      if(days_Beijing>=monthrun_table[mont_Beijing])//天数超过该月份
//      {
//        days_Beijing=1;
//        mont_Beijing++;
//        if(mont_Beijing>=12)
//        {
//          mont_Beijing=1;
//          year_Beijing++;   
//        }
//      }
//    }
//    if(year_Beijing%4!=0)              
//    {
//      if(days_Beijing>=monthmon_table[mont_Beijing])//是否夸年
//      {
//        days_Beijing=1;
//        mont_Beijing++;
//        if(mont_Beijing>=12)
//        {
//          mont_Beijing=1;
//          year_Beijing++;   
//        }
//      }
//    } 
//  }
//	sprintf(dis0,"20%02d-%02d-%02d   ",(u16)year_Beijing,(u16)mont_Beijing,(u16)days_Beijing);//打印日期
//	sprintf(dis1,"%02d:%02d:%02d      ",(u16)hour_Beijing,(u16)minu_Beijing,(u16)seco_Beijing);//打印时间
	
	Mid_Du=(gps_infor_weijing[0]-0x30)*10000000+(gps_infor_weijing[1]-0x30)*1000000;    //处理经度扩大10000000
	
	Mid_Fen=(gps_infor_weijing[2]-0x30)*10000000+(gps_infor_weijing[3]-0x30)*1000000+
		(gps_infor_weijing[4]-0x30)*100000+(gps_infor_weijing[5]-0x30)*10000+
			(gps_infor_weijing[6]-0x30)*1000+(gps_infor_weijing[7]-0x30)*100;          
	Mid_Fen=Mid_Fen/60;                                                      //分秒换算为小数位
	Mid_Vale=Mid_Du+Mid_Fen;         //最终为度格式000.00000000 非度分秒格式
	Lin0_No[0]='N';                  
	Lin0_No[1]=':';                  
	Lin0_No[2]='0';                  
	Lin0_No[3]=Mid_Vale/10000000+0x30;                  //转化为字符
	Lin0_No[4]=(Mid_Vale/1000000)%10+0x30;
	Lin0_No[5]='.';
	Lin0_No[6]=(Mid_Vale/100000)%10+0x30;
	Lin0_No[7]=(Mid_Vale/10000)%10+0x30;
	Lin0_No[8]=(Mid_Vale/1000)%10+0x30;
	Lin0_No[9]=(Mid_Vale/100)%10+0x30;
	Lin0_No[10]=(Mid_Vale/10)%10+0x30;
	Lin0_No[11]=Mid_Vale%10+0x30;

	Mid_Du=(gps_infor_weijing[8]-0x30)*100000000+(gps_infor_weijing[9]-0x30)*10000000+(gps_infor_weijing[10]-0x30)*1000000; //处理经度扩大10000000     

	Mid_Fen=(gps_infor_weijing[11]-0x30)*10000000+(gps_infor_weijing[12]-0x30)*1000000+
		(gps_infor_weijing[13]-0x30)*100000+(gps_infor_weijing[14]-0x30)*10000+
		(gps_infor_weijing[15]-0x30)*1000+(gps_infor_weijing[16]-0x30)*100; 
	Mid_Fen=Mid_Fen/60;                                                //分秒换算为小数位
	Mid_Vale=Mid_Du+Mid_Fen;                                          //最终为度格式000.00000000 非度分秒格式
	Lin1_Ea[0]='E';                  
	Lin1_Ea[1]=':';     
	Lin1_Ea[2]=Mid_Vale/100000000+0x30;                           //转化为字符
	Lin1_Ea[3]=(Mid_Vale/10000000)%10+0x30;
	Lin1_Ea[4]=(Mid_Vale/1000000)%10+0x30;
	Lin1_Ea[5]='.';
	Lin1_Ea[6]=(Mid_Vale/100000)%10+0x30;
	Lin1_Ea[7]=(Mid_Vale/10000)%10+0x30;
	Lin1_Ea[8]=(Mid_Vale/1000)%10+0x30;
	Lin1_Ea[9]=(Mid_Vale/100)%10+0x30;
	Lin1_Ea[10]=(Mid_Vale/10)%10+0x30;
	Lin1_Ea[11]=Mid_Vale%10+0x30;
}

void USART2_IRQHandler(void)                	//串口1中断服务程序
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		uart1_r_buf=USART2->DR;//();	//读取接收到的数据
		rev1_buf_busy=0x00;                         //判断 防止break问题
		switch(recv1_step)
		{
			case STAGE_SOHE: if(uart1_r_buf == '$')     //具体参考GPS标准协议NMEA0183
			{
				rev1_buf_busy=0x01;
				if(uart1_r_buf == '$')              //查看收到$
				{
					recv1_step=STAGE_TYPE;            //跳转到下一步
					record1=0;                        //计数清零
				}
				else
				{
					recv1_step=STAGE_SOHE;        //恢复初始化
					record1=0;
				}
			}
	  break;
			case STAGE_TYPE: if(rev1_buf_busy == 0x00)
			{
				rev1_buf_busy=0x01;
				temp1_buf[record1]=uart1_r_buf;
				record1++;
				if(record1 == 0x05)
				{                                                 //确认开头$GPRMC
					if((temp1_buf[0] == 'G') /*&& (temp1_buf[1] == 'P') */&& (temp1_buf[2] == 'R') && (temp1_buf[3] == 'M') && (temp1_buf[4] == 'C'))
					{
						recv1_step=STAGE_NONE;    //跳转到下一步
						record1=0;
					} 
					else
					{
						recv1_step=STAGE_SOHE;//恢复初始化
						record1=0;
					}
				}
			}
	  break;
			case STAGE_NONE: if(rev1_buf_busy == 0x00)//接收数据格式:$GPRMC,054347.00,A,3202.04770,N,11846.23632,E,0.000,0.00,221013,,,A*67
			{
				rev1_buf_busy=0x01;
				record1++;
				if((record1 > 0x01) && (record1 < 0x08))                                                                                    
				{
					gps_infor_time[record1-2]=uart1_r_buf;			//存储时间					
				}
				if((uart1_r_buf == ',') && (record1 > 0x07) && (record1 < 0x010))   //||((uart1_r_buf == ',') && (record1==0x02))
				{
					record1=0xcc;
				}
				if(record1 ==  0xcd)
				{
					record1=0;
					devide_flag=2;
					speed_end=0x00;
					dir_end=0x00;
					if(uart1_r_buf == 'A')  //gps收到数据 且有效
					{ 
						recv1_step=STAGE_DATA;    //跳转到下一步
					}
					else
					{
						sysmode_GPS=0;
						recv1_step=STAGE_SOHE;    //无效恢复初始化
						record1=0;
					}
				}
			}
	  break;
			case STAGE_DATA:  if(rev1_buf_busy == 0x00)
			{
				rev1_buf_busy=0x01;
				record1++;
				if(uart1_r_buf == ',')    //判断为逗号
				{ 
					devide_flag++;      //逗号 次数记录
					record1=0;
				}
				if(devide_flag == 3)
				{
					if((record1 > 0) && (record1 < 5))
					{
						gps_infor_weijing[record1-1]=uart1_r_buf;	    //存储经纬度 此处为纬度					
					}
					if((record1 > 5) && (record1 < 10))             //跳过小数点的存储
					{
						gps_infor_weijing[record1-2]=uart1_r_buf;	   //存储经纬度 此处为纬度														
					}
				}
				if(devide_flag == 4)
				{
					if(record1 > 0)
					{
						ns_flag=uart1_r_buf;            //接受纬度 NS标志
					}
				}
				if(devide_flag == 5)
				{
					if((record1 > 0) && (record1 < 6))
					{
						gps_infor_weijing[record1+7]=uart1_r_buf;	  //跳过小数点的存储										
					}
					if((record1 > 6) && (record1 < 11))                //
					{
						gps_infor_weijing[record1+6]=uart1_r_buf;       //存储经纬度	 此处为经度																	
					}
				}
				if(devide_flag == 6)
				{
					if(record1 > 0)
					{
						ew_flag=uart1_r_buf;            //经纬度 EW标志
					}
				}
				if(devide_flag == 7)
				{
					if(speed_end == 0x00)
					{
						if((record1 > 0) && (uart1_r_buf != '.'))
						{
							gps_infor_speed[record1-1]=uart1_r_buf;   //接受速率
						}
						else if(uart1_r_buf == '.')
						{
							record1--;
							speed_end=0xff;
						}
					}
					else if(speed_end == 0xff)
					{
						speed_end=0xfe;
						gps_infor_speed[record1-1]=uart1_r_buf;
						gps_infor_speed[3]=gps_infor_speed[record1-1];
						gps_infor_speed[2]=gps_infor_speed[record1-2];
						if(record1 > 2)
						{
							gps_infor_speed[1]=gps_infor_speed[record1-3];
						}
						else
						{
							gps_infor_speed[1]=0x30;
						}
						if(record1 > 3)
						{
							gps_infor_speed[0]=gps_infor_speed[record1-4];
						}
						else
						{
							gps_infor_speed[0]=0x30;
						}
					}
				}
				if(devide_flag == 8)
				{
					if(dir_end == 0x00)
					{
						if((record1 > 0) && (uart1_r_buf != '.'))
						{
							gps_infor_dir[record1-1]=uart1_r_buf;   //存储方向
						}
						else if(uart1_r_buf == '.')
						{
							record1--;
							dir_end=0xff;
						}
					}
					else if(dir_end == 0xff)
					{
						dir_end=0xfe;
						if(record1 == 2)
						{
							gps_infor_dir[2]=gps_infor_dir[record1-2];
							gps_infor_dir[1]=0x30;
							gps_infor_dir[0]=0x30;
						}
						if(record1 == 3)
						{
							gps_infor_dir[2]=gps_infor_dir[record1-2];
							gps_infor_dir[1]=gps_infor_dir[record1-3];
							gps_infor_dir[0]=0x30;
						}
					}
				}
				if(devide_flag == 9)
				{
					if((record1 > 0) && (record1 < 7))
					{
						gps_infor_date[record1-1]=uart1_r_buf;
					}
				}
				if(uart1_r_buf == 0x0d)
				{
					recv1_step=STAGE_SOHE;    //接受完成 信号确定
					record1=0;                //恢复初始化状态 为下一次做准备
					devide_flag=0;
					sysmode_GPS=1;         //标志 GPS信号有效
				}
			}
	  break;
	  }
  } 
} 
