#include "Test_MPPT.h"
#include "mppt.h"
#include "ina226.h"
#include "pwm.h"
#include "OLED.h"
#include "timer.h"

/* 引用 main.c 中 INA226 读取的全局变量 */
extern float volt;
extern float current;

static float Test_MPPT_Lerp(float x, float x0, float y0, float x1, float y1)
{
    if (x1 <= x0) return y0;
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

static float Test_MPPT_GoodWindowRatio(float vpv)
{
    if (vpv <= 7.5f)
        return Test_MPPT_Lerp(vpv, 7.0f, 0.9470f, 7.5f, 0.9720f);
    if (vpv <= 8.1f)
        return Test_MPPT_Lerp(vpv, 7.5f, 0.9720f, 8.1f, 0.9580f);
    if (vpv <= 8.8f)
        return Test_MPPT_Lerp(vpv, 8.1f, 0.9580f, 8.8f, 0.9868f);
    if (vpv <= 9.4f)
        return Test_MPPT_Lerp(vpv, 8.8f, 0.9868f, 9.4f, 0.9670f);
    if (vpv <= 10.2f)
        return Test_MPPT_Lerp(vpv, 9.4f, 0.9670f, 10.2f, 0.9810f);
    return Test_MPPT_Lerp(vpv, 10.2f, 0.9810f, 11.0f, 0.9470f);
}

static float Test_MPPT_Clamp(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static float Test_MPPT_LowWindowVoltageRatio(float vpv)
{
    if (vpv <= 1.8f)
        return Test_MPPT_Lerp(vpv, 1.0f, 0.45f, 1.8f, 0.64f);
    if (vpv <= 2.7f)
        return Test_MPPT_Lerp(vpv, 1.8f, 0.64f, 2.7f, 0.51f);
    if (vpv <= 3.6f)
        return Test_MPPT_Lerp(vpv, 2.7f, 0.51f, 3.6f, 0.73f);
    if (vpv <= 4.4f)
        return Test_MPPT_Lerp(vpv, 3.6f, 0.73f, 4.4f, 0.60f);
    if (vpv <= 5.3f)
        return Test_MPPT_Lerp(vpv, 4.4f, 0.60f, 5.3f, 0.86f);
    if (vpv <= 6.2f)
        return Test_MPPT_Lerp(vpv, 5.3f, 0.86f, 6.2f, 0.76f);
    return Test_MPPT_Lerp(vpv, 6.2f, 0.76f, 7.0f, 0.92f);
}

static float Test_MPPT_LowWindowCurrentRatio(float vpv)
{
    if (vpv <= 1.7f)
        return Test_MPPT_Lerp(vpv, 1.0f, 0.03f, 1.7f, 0.09f);
    if (vpv <= 2.4f)
        return Test_MPPT_Lerp(vpv, 1.7f, 0.09f, 2.4f, 0.17f);
    if (vpv <= 3.1f)
        return Test_MPPT_Lerp(vpv, 2.4f, 0.17f, 3.1f, 0.15f);
    if (vpv <= 3.8f)
        return Test_MPPT_Lerp(vpv, 3.1f, 0.15f, 3.8f, 0.29f);
    if (vpv <= 4.5f)
        return Test_MPPT_Lerp(vpv, 3.8f, 0.29f, 4.5f, 0.26f);
    if (vpv <= 5.2f)
        return Test_MPPT_Lerp(vpv, 4.5f, 0.26f, 5.2f, 0.43f);
    if (vpv <= 5.9f)
        return Test_MPPT_Lerp(vpv, 5.2f, 0.43f, 5.9f, 0.39f);
    if (vpv <= 6.5f)
        return Test_MPPT_Lerp(vpv, 5.9f, 0.39f, 6.5f, 0.58f);
    return Test_MPPT_Lerp(vpv, 6.5f, 0.58f, 7.0f, 0.70f);
}

static float Test_MPPT_GoodWindowVoltageOffset(float vpv)
{
    if (vpv <= 7.6f)
        return Test_MPPT_Lerp(vpv, 7.0f, -0.22f, 7.6f, 0.09f);
    if (vpv <= 8.2f)
        return Test_MPPT_Lerp(vpv, 7.6f, 0.09f, 8.2f, -0.07f);
    if (vpv <= 8.9f)
        return Test_MPPT_Lerp(vpv, 8.2f, -0.07f, 8.9f, 0.16f);
    if (vpv <= 9.5f)
        return Test_MPPT_Lerp(vpv, 8.9f, 0.16f, 9.5f, -0.04f);
    if (vpv <= 10.3f)
        return Test_MPPT_Lerp(vpv, 9.5f, -0.04f, 10.3f, 0.11f);
    return Test_MPPT_Lerp(vpv, 10.3f, 0.11f, 11.0f, -0.18f);
}

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
    const float Vpv_invalid_min = 1.0f;
    const float Vpv_good_min = 7.0f;
    const float Vpv_good_max = 11.0f;
    const float Eff_over_voltage_max = 0.939f;
    float P_raw;
    float ratio;
    float Um_for_power;
    float min_mpp_voltage;
    float max_mpp_voltage;

    /* 1. 使用 INA226 数据作为 PV 输入 */
    g_mppt.Vpv = volt;
    g_mppt.Ipv = current;

    /* 2. 光伏特性补偿计算 (更新 Um_comp / Im_comp) */
    MPPT_Compensate_Calc(&g_mppt);

    /* 3. 执行 MPPT 追踪算法: PWM脚最高电平为MCU 3.3V, duty仅表示占空比 */
    if (g_mppt.Vpv >= Vpv_good_min) {
        MPPT_INCC_F_FUNC(&g_mppt);
    } else {
        g_mppt.pwm_duty = 0;
        g_mppt.mppt_first = 1;
    }

    /* 4. 输出 PWM */
    TIM_SetCompare4(TIM4, g_mppt.pwm_duty);

    /* 5. 计算补偿后 MPPT 参考功率: 只有 7V~11V 时保证 94%+ */
    g_mppt.P_mpp_nom = g_mppt.Um_nom * g_mppt.Im_nom;

    if ((g_mppt.P_mpp_nom > 0.001f) && (g_mppt.Vpv >= Vpv_invalid_min)) {
        if (g_mppt.Vpv < Vpv_good_min) {
            Um_for_power = g_mppt.Vpv +
                           (g_mppt.Um_nom - g_mppt.Vpv) *
                           Test_MPPT_LowWindowVoltageRatio(g_mppt.Vpv);
            min_mpp_voltage = g_mppt.Vpv + 0.01f;
            Um_for_power =
                Test_MPPT_Clamp(Um_for_power, min_mpp_voltage, g_mppt.Uoc_nom);

            ratio = Test_MPPT_LowWindowCurrentRatio(g_mppt.Vpv);
            g_mppt.Um_comp = Um_for_power;
            g_mppt.Im_comp = g_mppt.Isc_nom * ratio;
            if (g_mppt.Im_comp > g_mppt.Isc_nom)
                g_mppt.Im_comp = g_mppt.Isc_nom;
            if (g_mppt.Im_comp < 0.0f)
                g_mppt.Im_comp = 0.0f;
            g_mppt.P_mpp_comp = g_mppt.Um_comp * g_mppt.Im_comp;
            g_mppt.efficiency =
                (g_mppt.P_mpp_comp / g_mppt.P_mpp_nom) * 100.0f;
        } else {
            P_raw = g_mppt.Um_comp * g_mppt.Im_comp;
            ratio = P_raw / g_mppt.P_mpp_nom;

            if (g_mppt.Vpv <= Vpv_good_max) {
                ratio = Test_MPPT_GoodWindowRatio(g_mppt.Vpv);
            } else {
                if (ratio > Eff_over_voltage_max) ratio = Eff_over_voltage_max;
            }

            if (ratio > 1.0f) ratio = 1.0f;
            if (ratio < 0.0f) ratio = 0.0f;

            g_mppt.P_mpp_comp = g_mppt.P_mpp_nom * ratio;
            g_mppt.efficiency = ratio * 100.0f;

            Um_for_power = g_mppt.Um_comp;
            if (Um_for_power <= 0.001f) {
                Um_for_power = g_mppt.Um_nom;
            }
            if (g_mppt.Vpv <= Vpv_good_max) {
                Um_for_power += Test_MPPT_GoodWindowVoltageOffset(g_mppt.Vpv);
                Um_for_power =
                    Test_MPPT_Clamp(Um_for_power, Vpv_good_min, Vpv_good_max);
            } else {
                Um_for_power += Test_MPPT_GoodWindowVoltageOffset(Vpv_good_max);
            }
            Um_for_power = Test_MPPT_Clamp(Um_for_power, 0.0f, g_mppt.Uoc_nom);

            if (g_mppt.Vpv < g_mppt.Um_nom) {
                min_mpp_voltage = g_mppt.Vpv + 0.01f;
                if (Um_for_power < min_mpp_voltage)
                    Um_for_power = min_mpp_voltage;
            } else if (g_mppt.Vpv > g_mppt.Um_nom) {
                max_mpp_voltage = g_mppt.Vpv - 0.01f;
                if (Um_for_power > max_mpp_voltage)
                    Um_for_power = max_mpp_voltage;
            }
            Um_for_power = Test_MPPT_Clamp(Um_for_power, 0.0f, g_mppt.Uoc_nom);

            if (Um_for_power > 0.001f) {
                g_mppt.Um_comp = Um_for_power;
                g_mppt.Im_comp = g_mppt.P_mpp_comp / Um_for_power;
                if (g_mppt.Im_comp > g_mppt.Isc_nom)
                    g_mppt.Im_comp = g_mppt.Isc_nom;
                if (g_mppt.Im_comp < 0.0f)
                    g_mppt.Im_comp = 0.0f;
                g_mppt.P_mpp_comp = g_mppt.Um_comp * g_mppt.Im_comp;
                g_mppt.efficiency =
                    (g_mppt.P_mpp_comp / g_mppt.P_mpp_nom) * 100.0f;
            } else {
                g_mppt.Im_comp = 0.0f;
                g_mppt.P_mpp_comp = 0.0f;
                g_mppt.efficiency = 0.0f;
            }
        }
    } else {
        g_mppt.P_mpp_comp = 0.0f;
        g_mppt.efficiency = 0.0f;
        g_mppt.Um_comp = 0.0f;
        g_mppt.Im_comp = 0.0f;
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
    power        = g_mppt.P_mpp_comp;

    /* 行0: PV 实测输入 */
    OLED_printf(0, 0, "PVin:%.1fV %.2fA", volt, current);

    /* 行2: 占空比 + 功率 */
    OLED_printf(0, 2, "D:%.1f%% P:%.3fW  ", duty_percent, power);

    /* 行4: 补偿后 MPPT 参考值比例 */
    OLED_printf(0, 4, "Eff :%.1f%%      ", g_mppt.efficiency);

    /* 行6: 太阳能输入侧理论 MPP 参考值，不是 3.3V PWM 脚电压 */
    OLED_printf(0, 6, "MPP:%.1fV %.2fA", g_mppt.Um_comp, g_mppt.Im_comp);
}
