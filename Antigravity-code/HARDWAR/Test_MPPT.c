#include "Test_MPPT.h"
#include "mppt.h"
#include "ina226.h"
#include "pwm.h"
#include "OLED.h"
#include "timer.h"
#include <math.h>

/* 引用 main.c 中 INA226 读取的全局变量 */
extern float volt;
extern float current;

/*
 * ============================================================
 * Test_MPPT_Init - 初始化 MPPT 测试环境
 * ============================================================
 */
void Test_MPPT_Init(void)
{
    TIM4_PWM_Init(1000, 71);
    MPPT_Init();
}

/*
 * ============================================================
 * Test_MPPT_Process - 执行一次 MPPT 模拟算法
 *
 * 同时计算补偿后 MPPT 参考值:
 *   efficiency = P_mpp_comp / P_mpp_nom * 100%
 *   P_mpp_comp = Um_comp * Im_comp       (补偿后理论最大功率)
 *   P_mpp_nom  = Um_nom  * Im_nom        (标称最大功率点, 默认1.98W)
 * ============================================================
 */
void Test_MPPT_Process(void)
{
    float P_mpp_comp;
    float P_mpp_nom;

    /* 1. 使用 INA226 数据作为 PV 输入 */
    g_mppt.Vpv = volt;
    g_mppt.Ipv = current;

    /* 2. 光伏特性补偿计算 (更新 Um_comp / Im_comp) */
    MPPT_Compensate_Calc(&g_mppt);

    /* 3. 执行 MPPT 追踪算法 */
    if (g_mppt.Vpv > 3.0f) {
        MPPT_INCC_F_FUNC(&g_mppt);
    } else {
        g_mppt.pwm_duty = 0;
        g_mppt.mppt_first = 1;
    }

    /* 4. 输出 PWM */
    TIM_SetCompare4(TIM4, g_mppt.pwm_duty);

    /* 5. 计算补偿后 MPPT 参考值相对标称最大功率点的比例 */
    P_mpp_comp = g_mppt.Um_comp * g_mppt.Im_comp;
    P_mpp_nom  = g_mppt.Um_nom  * g_mppt.Im_nom;

    if (P_mpp_nom > 0.001f) {
        g_mppt.efficiency = (P_mpp_comp / P_mpp_nom) * 100.0f;
        /* 参考值上限 100%, 下限钳位 0 */
        if (g_mppt.efficiency > 100.0f) g_mppt.efficiency = 100.0f;
        if (g_mppt.efficiency <   0.0f) g_mppt.efficiency =   0.0f;
    } else {
        g_mppt.efficiency = 0.0f;
    }
}

/*
 * ============================================================
 * Test_MPPT_Display - OLED Page 2 显示 MPPT 测试数据
 *
 * 布局 (128x64, 4行):
 *   行0: PVin:xx.xV x.xxA   (INA226 实测)
 *   行2: Duty:xx.x%  Pwr:x.xxW
 *   行4: Eff :xx.x%          (补偿后MPPT参考值 / 标称最大功率点)
 *   行6: MPP :xx.xV x.xxA   (补偿后理论MPP)
 * ============================================================
 */


void Test_MPPT_Display(void)
{
    float duty_percent;
    float power;

    duty_percent = g_mppt.pwm_duty / 10.0f;
    power        = volt * current;

    /* 行0: PV 实测输入 */
//    OLED_printf(0, 0, "PVin:%.1fV %.2fA", volt = 8.5, current = 0.100);

    /* 行2: 占空比 + 功率 */
//    OLED_printf(0, 2, "D:%.1f%% P:%.3fW  ", duty_percent = 32, power = 1.9323);

    /* 行4: 补偿后 MPPT 参考值比例 */
 //   OLED_printf(0, 4, "Eff :%.1f%%      ", g_mppt.efficiency);

    /* 行6: 补偿后理论 MPP 参考值 */
 //   OLED_printf(0, 6, "MPP:%.1fV %.2fA", g_mppt.Um_comp = 9.29, g_mppt.Im_comp = 0.208);



    // /* 行0: PV 实测输入 */
    // OLED_printf(0, 0, "PVin:%.1fV %.2fA", volt = 9.6, current = 0.119);

    // /* 行2: 占空比 + 功率 */
    // OLED_printf(0, 2, "D:%.1f%% P:%.3fW  ", duty_percent = 28.5, power = 1.9323);

    // /* 行4: 补偿后 MPPT 参考值比例 */
    // OLED_printf(0, 4, "Eff :%.1f%%      ", g_mppt.efficiency = 95.3);

    // /* 行6: 补偿后理论 MPP 参考值 */
    // OLED_printf(0, 6, "MPP:%.1fV %.2fA", g_mppt.Um_comp = 9.12, g_mppt.Im_comp = 0.208);



    /* 行0: PV 实测输入 */
    OLED_printf(0, 0, "PVin:%.1fV %.2fA", volt, current);

    /* 行2: 占空比 + 功率 */
    OLED_printf(0, 2, "D:%.1f%% P:%.3fW  ", duty_percent, power);

    /* 行4: 补偿后 MPPT 参考值比例 */
    OLED_printf(0, 4, "Eff :%.1f%%      ", g_mppt.efficiency);

    /* 行6: 补偿后理论 MPP 参考值 */
    OLED_printf(0, 6, "MPP:%.1fV %.2fA", g_mppt.Um_comp, g_mppt.Im_comp);
}
