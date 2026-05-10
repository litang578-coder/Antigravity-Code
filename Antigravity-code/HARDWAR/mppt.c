#include "mppt.h"
#include "ina3221.h"
#include "timer.h"
#include "pwm.h"
#include <math.h>

/* 引用外部环境温度 (来自 main.c 中的 DS18B20 读取值) */
extern float temp;

/* 全局句柄 */
MPPT_INCC_F g_mppt;

/* 内部宏定义 */
#define __einvf32(x) (1.0f / (x))

/* 
 * ============================================================
 * MPPT 初始化
 * ============================================================ 
 */
void MPPT_Init(void)
{
    // 1. MPPT 算法参数初始化
    g_mppt.mppt_enable = 1;
    g_mppt.mppt_first = 1;
    g_mppt.Stepsize = 2.0f;     /* 占空比步进 */
    g_mppt.VmppOut = 500.0f;    /* 初始占空比 50% */
    g_mppt.pwm_duty = 500;
    
    // 2. 光伏面板标称参数 (请在此处手动修改你的面板参数)
    g_mppt.Uoc_nom = 44.2f;     /* 标称开路电压 */
    g_mppt.Um_nom  = 35.4f;     /* 标称最大功率点电压 */
    g_mppt.Isc_nom = 5.29f;     /* 标称短路电流 */
    g_mppt.Im_nom  = 4.95f;     /* 标称最大功率点电流 */
    
    // 3. 环境变量初始化
    g_mppt.Irradiance = 1000.0f; /* 默认 1000W/m2 */
    g_mppt.Temp_Air = 25.0f;
}

/* 
 * ============================================================
 * 光伏特性补偿计算 (MATLAB 算法移植)
 * ============================================================ 
 */
void MPPT_Compensate_Calc(MPPT_INCC_F *v)
{
    const float Sref = 1000.0f;
    const float Tref = 25.0f;
    const float a = 0.00255f;
    const float b = 0.55f;
    const float c = 0.00285f;
    
    // 获取环境温度
    v->Temp_Air = temp; 
    
    // 1. 计算电池板实际温度 T_1
    // 计算公式: T = Tair + 0.028 * S
    float T_1 = v->Temp_Air + 0.028f * v->Irradiance;
    float T_delta = T_1 - Tref;
    float S_delta = (v->Irradiance / Sref) - 1.0f;
    
    // 2. 计算修正后的短路电流 Isc_comp 和开路电压 Uoc_comp
    // 公式: Isc_comp = Isc * (S/Sref) * (1 + a*T_delta)
    v->Isc_comp = v->Isc_nom * (v->Irradiance / Sref) * (1.0f + a * T_delta);
    
    // 公式: Uoc_comp = Uoc * (1 - c*T_delta) * log(e + b*S_delta)
    // e = exp(1.0)
    v->Uoc_comp = v->Uoc_nom * (1.0f - c * T_delta) * logf(expf(1.0f) + b * S_delta);
    
    // 3. 计算修正后的最大功率点电流 Im_comp 和电压 Um_comp
    v->Im_comp = v->Im_nom * (v->Irradiance / Sref) * (1.0f + a * T_delta);
    v->Um_comp = v->Um_nom * (1.0f - c * T_delta) * logf(expf(1.0f) + b * S_delta);
}

/* 
 * ============================================================
 * MPPT 核心算法 (增量电导法)
 * ============================================================ 
 */
void MPPT_INCC_F_FUNC(MPPT_INCC_F *v)
{
    if (v->mppt_enable == 1)
    {
        if (v->mppt_first == 1)
        {
            v->VmppOut = v->VmppOut - 5.0f; /* 初始扰动 */
            v->VpvOld = v->Vpv;
            v->IpvOld = v->Ipv;
            v->mppt_first = 0;
        }
        else
        {
            float deltaV = v->Vpv - v->VpvOld;
            float deltaI = v->Ipv - v->IpvOld;

            if (fabs(deltaV) < 0.01f) 
            {
                if (fabs(deltaI) > 0.001f) 
                {
                    if (deltaI > 0) v->VmppOut -= v->Stepsize; 
                    else v->VmppOut += v->Stepsize; 
                }
            }
            else 
            {
                float cond = v->Ipv * __einvf32(v->Vpv);
                float incCond = deltaI * __einvf32(deltaV);
                
                if (fabs(incCond + cond) > 0.01f) 
                {
                    if (incCond > (-cond)) v->VmppOut -= v->Stepsize; 
                    else v->VmppOut += v->Stepsize; 
                }
            }
            
            // 占空比限幅
            if(v->VmppOut > 950) v->VmppOut = 950;
            if(v->VmppOut < 50)  v->VmppOut = 50;
            
            v->VpvOld = v->Vpv;
            v->IpvOld = v->Ipv;
            v->pwm_duty = (uint16_t)v->VmppOut;
        }
    }
}

/* 
 * ============================================================
 * MPPT 运行处理 (由 main.c 定时调用)
 * ============================================================ 
 */
void MPPT_Process(void)
{
    /* 1. 更新采样数据 (CH2 为 PV 面板) */
    g_mppt.Vpv = g_ina3221.ch2_voltage; 
    g_mppt.Ipv = g_ina3221.ch2_current;
    
    /* 2. 执行光伏特性补偿计算 (更新 Uoc_comp / Isc_comp) */
    MPPT_Compensate_Calc(&g_mppt);
    
    /* 3. 执行 MPPT 追踪算法 */
    if (g_mppt.Vpv > 3.0f) { 
        MPPT_INCC_F_FUNC(&g_mppt);
    } else {
        g_mppt.pwm_duty = 0;
        g_mppt.mppt_first = 1;
    }
    
    /* 4. 输出 PWM 调节 */
    TIM_SetCompare4(TIM4, g_mppt.pwm_duty);
}
