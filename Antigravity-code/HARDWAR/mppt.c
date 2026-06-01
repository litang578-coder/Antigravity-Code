#include "mppt.h"
#include "ina3221.h"
#include "timer.h"
#include "pwm.h"
#include <math.h>

/* 引用外部环境温度 (由 main.c 中的 DS18B20 传感器实时读取) */
extern float temp;

/* 全局 MPPT 算法控制句柄 */
MPPT_INCC_F g_mppt;

/* 内部宏定义：快速计算浮点倒数 (1.0 / x) */
#define __einvf32(x) (1.0f / (x))

/**
 * @brief  MPPT 算法及面板参数初始化
 * @param  无
 * @retval 无
 */
void MPPT_Init(void)
{
    // 1. MPPT 追踪核心参数初始化
    g_mppt.mppt_enable = 1;     /* 使能 MPPT 算法追踪 */
    g_mppt.mppt_first = 1;      /* 标记首次运行，用于执行初始扰动 */
    g_mppt.Stepsize = 2.0f;     /* 扰动步长 (对应占空比千分比的增减步长，即 0.2%) */
    g_mppt.VmppOut = 500.0f;    /* 初始占空比设定值为 500 (对应 50.0% 占空比) */
    g_mppt.pwm_duty = 500;      /* 初始化硬件 PWM 占空比寄存器缓存值 */
    
    // 2. 光伏面板标称参数 (标准测试条件 STC: 1000W/m2, 25℃ 下的物理特性参数)
    g_mppt.Uoc_nom = 11.2f;     /* 标称开路电压 (V) */
    g_mppt.Um_nom  = 9.0f;      /* 标称最大功率点电压 (V) */
    g_mppt.Isc_nom = 0.24f;     /* 标称短路电流 (A) */
    g_mppt.Im_nom  = 0.22f;     /* 标称最大功率点电流 (A) */
    
    // 3. 环境变量及功率缓存初始化
    g_mppt.Irradiance = 1000.0f; /* 默认实时光照强度设定为 1000 W/m² */
    g_mppt.Temp_Air = 25.0f;     /* 默认环境温度设定为 25 ℃ */
    g_mppt.P_mpp_nom = g_mppt.Um_nom * g_mppt.Im_nom; /* 计算面板标称最大功率 (W) */
    g_mppt.P_mpp_comp = 0.0f;    /* 经环境温度与光照补偿后的理论最大功率 */
    g_mppt.efficiency = 0.0f;    /* 追踪效率 (%) */
}

/**
 * @brief  光伏特性补偿计算 (MATLAB 算法移植)
 * @details 根据当前的实时环境温度与光照强度，修正光伏面板的理论极限参数
 * @param  v: MPPT 结构体指针
 * @retval 无
 */
void MPPT_Compensate_Calc(MPPT_INCC_F *v)
{
    const float Sref = 1000.0f;  /* 标准参考光照强度 1000 W/m² */
    const float Tref = 25.0f;    /* 标准参考温度 25 ℃ */
    const float a = 0.00255f;    /* 电流温度补偿系数 (正温度系数，温度升高短路电流微增) */
    const float b = 0.55f;       /* 光照强度修正系数 */
    const float c = 0.00285f;    /* 电压温度补偿系数 (负温度系数，温度升高开路电压显著下降) */
    
    // 从外部 DS18B20 传感器获取最新环境温度
    v->Temp_Air = temp; 
    
    // 1. 计算电池板实际工作温度 T_1
    // 经验公式: T_cell = T_air + 0.028 * S (光照会使面板温度高于空气温度)
    float T_1 = v->Temp_Air + 0.028f * v->Irradiance;
    float T_delta = T_1 - Tref;                  /* 温度偏离量 */
    float S_delta = (v->Irradiance / Sref) - 1.0f; /* 光照强度相对变化量 */
    
    // 2. 计算温度与光照修正后的短路电流 Isc_comp 和开路电压 Uoc_comp
    // 修正公式: Isc_comp = Isc_nom * (S/Sref) * [1 + a * (T_cell - Tref)]
    v->Isc_comp = v->Isc_nom * (v->Irradiance / Sref) * (1.0f + a * T_delta);
    
    // 修正公式: Uoc_comp = Uoc_nom * [1 - c * (T_cell - Tref)] * ln(e + b * S_delta)
    // 引入自然常数 e = exp(1.0f)，保证在标准光照下 (S_delta=0)，对数项 ln(e) = 1
    v->Uoc_comp = v->Uoc_nom * (1.0f - c * T_delta) * logf(expf(1.0f) + b * S_delta);
    
    // 3. 计算修正后的理论最大功率点电流 Im_comp 和最大功率点电压 Um_comp
    v->Im_comp = v->Im_nom * (v->Irradiance / Sref) * (1.0f + a * T_delta);
    v->Um_comp = v->Um_nom * (1.0f - c * T_delta) * logf(expf(1.0f) + b * S_delta);
    
    // 4. 计算当前环境下的理论最大输出功率 (用作追踪效率评估的基准)
    v->P_mpp_comp = v->Um_comp * v->Im_comp;
}

/**
 * @brief  MPPT 核心算法：增量电导法 (Incremental Conductance, INC)
 * @details 通过比较增量电导 (dI/dV) 与瞬时电导 (-I/V) 的关系，决定 PWM 占空比的调节方向
 *          算法数学原理：
 *          - 当 dI/dV = -I/V 时，处于最大功率点 (MPP)
 *          - 当 dI/dV > -I/V 时，工作点在 MPP 左侧，电压偏低，应减小占空比 (提升电压)
 *          - 当 dI/dV < -I/V 时，工作点在 MPP 右侧，电压偏高，应增大占空比 (降低电压)
 * @param  v: MPPT 结构体指针
 * @retval 无
 */
void MPPT_INCC_F_FUNC(MPPT_INCC_F *v)
{
    if (v->mppt_enable == 1)
    {
        // 首次运行：执行一次初始主动扰动，并建立采样基准值
        if (v->mppt_first == 1)
        {
            v->VmppOut = v->VmppOut - 5.0f; /* 初始微弱扰动，占空比减小 0.5% */
            v->VpvOld = v->Vpv;             /* 缓存当前电压值 */
            v->IpvOld = v->Ipv;             /* 缓存当前电流值 */
            v->mppt_first = 0;              /* 清除首次运行标志 */
        }
        else
        {
            // 计算相邻两次采样周期内的电压与电流变化量 (差分近似导数)
            float deltaV = v->Vpv - v->VpvOld;
            float deltaI = v->Ipv - v->IpvOld;

            // 1. 判断电压是否发生改变
            if (fabs(deltaV) < 0.01f) 
            {
                // 若电压基本未变，则根据电流变动量进行调整 (主要应对快速光照突变)
                if (fabs(deltaI) > 0.001f) 
                {
                    if (deltaI > 0) 
                    {
                        // 电流增加，说明光照变强，最大功率点电压右移，应降低占空比以提升电压
                        v->VmppOut -= v->Stepsize; 
                    }
                    else 
                    {
                        // 电流减少，说明光照变弱，最大功率点电压左移，应增加占空比以降低电压
                        v->VmppOut += v->Stepsize; 
                    }
                }
            }
            else 
            {
                // 若电压发生显著改变，利用电导关系进行判决
                float cond = v->Ipv * __einvf32(v->Vpv);      /* 瞬时电导 G = I / V */
                float incCond = deltaI * __einvf32(deltaV);   /* 增量电导 dG = dI / dV */
                
                // 判断当前工作点是否偏离 MPP (引入 0.01 容差，防止在 MPP 点来回震荡)
                if (fabs(incCond + cond) > 0.01f) 
                {
                    // dI/dV > -I/V: 处于 MPP 左侧，电压偏低，应减小占空比以提高电压
                    if (incCond > (-cond)) 
                    {
                        v->VmppOut -= v->Stepsize; 
                    }
                    // dI/dV < -I/V: 处于 MPP 右侧，电压偏高，应增加占空比以降低电压
                    else 
                    {
                        v->VmppOut += v->Stepsize; 
                    }
                }
            }
            
            // 2. 占空比输出限幅保护 (防止异常控制导致占空比溢出或驱动电路异常)
            if(v->VmppOut > 950) v->VmppOut = 950;  /* 最高占空比限制在 95.0% */
            if(v->VmppOut < 50)  v->VmppOut = 50;   /* 最低占空比限制在 5.0% */
            
            // 3. 缓存历史数据，并更新输出占空比
            v->VpvOld = v->Vpv;
            v->IpvOld = v->Ipv;
            v->pwm_duty = (uint16_t)v->VmppOut;     /* 存入无符号整型，便于直接写入定时器寄存器 */
        }
    }
}

/**
 * @brief  MPPT 周期性处理函数 (由 main.c 或定时器中断周期调用)
 * @details 负责读取传感器、执行参数补偿、运行追踪算法并最终更新 PWM 输出
 * @param  无
 * @retval 无
 */
void MPPT_Process(void)
{
    /* 1. 采样获取实时数据 (当前系统使用 INA3221 通道 2 采集光伏面板电压/电流) */
    g_mppt.Vpv = g_ina3221.ch2_voltage; 
    g_mppt.Ipv = g_ina3221.ch2_current;
    
    /* 2. 执行光伏特性补偿计算 (根据温度和光照刷新当前面板的极限参数与理论 MPP) */
    MPPT_Compensate_Calc(&g_mppt);
    
    /* 3. 执行 MPPT 追踪状态机 */
    if (g_mppt.Vpv > 3.0f) 
    { 
        // 只有当面板输出电压大于 3.0V (有基本光照) 时才运行 MPPT 追踪
        MPPT_INCC_F_FUNC(&g_mppt);
    } 
    else 
    {
        // 若光强极弱、阴天或黑夜导致面板电压低于 3.0V，关闭 PWM 输出以防止逆向放电或空耗
        g_mppt.pwm_duty = 0;
        g_mppt.mppt_first = 1; /* 重置首次运行标志，待光照恢复时重新执行初始扰动 */
    }
    
    /* 4. 更新硬件定时器输出 PWM */
    // 更新 TIM4 捕获比较寄存器 4，动态控制 Buck/Boost 驱动电路的占空比
    TIM_SetCompare4(TIM4, g_mppt.pwm_duty);
}
