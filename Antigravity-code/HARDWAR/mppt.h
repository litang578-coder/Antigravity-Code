#ifndef __MPPT_H
#define __MPPT_H

#include "sys.h"

/* 数据类型定义 */
typedef float   float32;
typedef int16_t int16;

/* MPPT 增量电导与补偿算法结构体 */
typedef struct {
    // 实时测量值
    float32  Ipv;          /* 当前 PV 电流 */
    float32  Vpv;          /* 当前 PV 电压 */
    
    // MPPT 算法参数
    float32  Stepsize;     /* 步进大小 (占空比调节量) */
    float32  VmppOut;      /* 当前占空比控制量 */
    float32  VpvOld;       /* 上次电压 */
    float32  IpvOld;       /* 上次电流 */
    float32  StepFirst;    /* 初始步进 */
    int16    mppt_enable;  /* MPPT 使能 */
    int16    mppt_first;   /* 首次运行标志 */
    uint16_t pwm_duty;     /* 硬件 PWM 占空比 (0-1000) */

    // 光伏特性补偿参数 (用户手动填写的标称值)
    float32  Uoc_nom;      /* 标称开路电压 */
    float32  Um_nom;       /* 标称最大功率点电压 */
    float32  Isc_nom;      /* 标称短路电流 */
    float32  Im_nom;       /* 标称最大功率点电流 */
    
    // 补偿计算结果 (实时修正后的参数)
    float32  Uoc_comp;     /* 修正后的开路电压 */
    float32  Isc_comp;     /* 修正后的短路电流 */
    float32  Um_comp;      /* 修正后的最大功率点电压 */
    float32  Im_comp;      /* 修正后的最大功率点电流 */
    float32  efficiency;   /* 补偿后MPPT参考值 / 标称最大功率点 (%) */
    
    // 环境变量
    float32  Irradiance;   /* 实时光照强度 (W/m2) */
    float32  Temp_Air;     /* 环境温度 (来自 DS18B20) */

} MPPT_INCC_F;

/* 全局句柄 */
extern MPPT_INCC_F g_mppt;

/* 函数声明 */
void MPPT_Init(void);
void MPPT_Process(void);
void MPPT_Compensate_Calc(MPPT_INCC_F *v);
void MPPT_INCC_F_FUNC(MPPT_INCC_F *v);

#endif
