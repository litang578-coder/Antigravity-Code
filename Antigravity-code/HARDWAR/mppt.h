#ifndef __MPPT_H
#define __MPPT_H

#include "sys.h"

typedef float   float32;
typedef int16_t int16;

typedef struct {
    /* 实时测量值 */
    float32  Ipv;
    float32  Vpv;
    
    /* MPPT 算法参数 */
    float32  Stepsize;
    float32  VmppOut;
    float32  VpvOld;
    float32  IpvOld;
    float32  StepFirst;
    int16    mppt_enable;
    int16    mppt_first;
    uint16_t pwm_duty;

    /* 光伏面板标称参数 */
    float32  Uoc_nom;
    float32  Um_nom;
    float32  Isc_nom;
    float32  Im_nom;
    
    /* 补偿计算结果 */
    float32  Uoc_comp;
    float32  Isc_comp;
    float32  Um_comp;
    float32  Im_comp;
    
    /* 环境变量 */
    float32  Irradiance;
    float32  Temp_Air;

    /* 效率 (%) : 补偿后MPPT参考值 / 标称最大功率点 */
    float32  efficiency;

} MPPT_INCC_F;

extern MPPT_INCC_F g_mppt;

void MPPT_Init(void);
void MPPT_Process(void);
void MPPT_Compensate_Calc(MPPT_INCC_F *v);
void MPPT_INCC_F_FUNC(MPPT_INCC_F *v);

#endif
