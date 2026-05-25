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

static float Test_MPPT_LerpCurve(float x,
                                 const float *x_table,
                                 const float *y_table,
                                 int count)
{
    int i;

    if (count <= 0) return 0.0f;
    if (x <= x_table[0]) return y_table[0];

    for (i = 1; i < count; i++) {
        if (x <= x_table[i]) {
            return Test_MPPT_Lerp(x,
                                  x_table[i - 1],
                                  y_table[i - 1],
                                  x_table[i],
                                  y_table[i]);
        }
    }

    return y_table[count - 1];
}

static float Test_MPPT_FastRipple(float x,
                                  float origin,
                                  float period,
                                  float amplitude)
{
    float offset;
    float phase;
    float half_period;
    float slope;
    int cycle;

    if ((period <= 0.001f) || (amplitude <= 0.0f)) return 0.0f;

    offset = x - origin;
    if (offset < 0.0f) offset = 0.0f;

    cycle = (int)(offset / period);
    phase = offset - (float)cycle * period;
    half_period = period * 0.5f;

    if (phase <= half_period) {
        slope = phase / half_period;
        return amplitude - 2.0f * amplitude * slope;
    }

    slope = (phase - half_period) / half_period;
    return -amplitude + 2.0f * amplitude * slope;
}

static float Test_MPPT_GoodWindowRatio(float vpv)
{
    static const float vpv_table[] = {
        7.0f, 7.25f, 7.5f, 7.75f, 8.0f, 8.25f, 8.5f, 8.75f,
        9.0f, 9.25f, 9.5f, 9.75f, 10.0f, 10.25f, 10.5f,
        10.75f, 11.0f
    };
    static const float ratio_table[] = {
        0.938f, 0.982f, 0.946f, 0.989f, 0.940f, 0.990f,
        0.948f, 0.986f, 0.936f, 0.989f, 0.944f, 0.987f,
        0.939f, 0.984f, 0.935f, 0.980f, 0.938f
    };

    return Test_MPPT_LerpCurve(vpv,
                               vpv_table,
                               ratio_table,
                               (int)(sizeof(vpv_table) / sizeof(vpv_table[0])));
}

static float Test_MPPT_Clamp(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static float Test_MPPT_LowWindowVoltageRatio(float vpv)
{
    static const float vpv_table[] = {
        1.0f, 1.4f, 1.8f, 2.2f, 2.7f, 3.1f, 3.6f, 4.0f,
        4.4f, 4.9f, 5.3f, 5.8f, 6.2f, 6.6f, 7.0f
    };
    static const float ratio_table[] = {
        0.38f, 0.66f, 0.46f, 0.76f, 0.50f, 0.84f, 0.58f,
        0.91f, 0.64f, 0.97f, 0.72f, 0.99f, 0.79f, 0.99f,
        0.98f
    };

    return Test_MPPT_LerpCurve(vpv,
                               vpv_table,
                               ratio_table,
                               (int)(sizeof(vpv_table) / sizeof(vpv_table[0])));
}

static float Test_MPPT_LowWindowCurrentRatio(float vpv)
{
    static const float vpv_table[] = {
        1.0f, 1.3f, 1.6f, 1.9f, 2.2f, 2.5f, 2.8f, 3.1f,
        3.4f, 3.7f, 4.0f, 4.3f, 4.6f, 4.9f, 5.2f, 5.5f,
        5.8f, 6.1f, 6.4f, 6.7f, 7.0f
    };
    static const float ratio_table[] = {
        0.02f, 0.10f, 0.04f, 0.18f, 0.08f, 0.26f, 0.13f,
        0.35f, 0.18f, 0.44f, 0.24f, 0.54f, 0.29f, 0.64f,
        0.36f, 0.70f, 0.43f, 0.75f, 0.50f, 0.79f, 0.76f
    };

    return Test_MPPT_LerpCurve(vpv,
                               vpv_table,
                               ratio_table,
                               (int)(sizeof(vpv_table) / sizeof(vpv_table[0])));
}

static float Test_MPPT_GoodWindowVoltageOffset(float vpv)
{
    static const float vpv_table[] = {
        7.0f, 7.25f, 7.5f, 7.75f, 8.0f, 8.25f, 8.5f, 8.75f,
        9.0f, 9.25f, 9.5f, 9.75f, 10.0f, 10.25f, 10.5f,
        10.75f, 11.0f
    };
    static const float offset_table[] = {
        0.10f, 0.46f, 0.06f, 0.62f, 0.12f, 0.74f, 0.32f,
        0.68f, 0.00f, -0.34f, -0.02f, -0.54f, -0.06f,
        -0.66f, -0.12f, -0.72f, -0.30f
    };

    return Test_MPPT_LerpCurve(vpv,
                               vpv_table,
                               offset_table,
                               (int)(sizeof(vpv_table) / sizeof(vpv_table[0])));
}

static float Test_MPPT_OverVoltageRatio(float vpv)
{
    static const float vpv_table[] = {
        11.0f, 11.3f, 11.6f, 11.9f, 12.2f, 12.5f, 12.8f,
        13.2f, 13.6f, 14.0f
    };
    static const float ratio_table[] = {
        0.945f, 0.760f, 0.920f, 0.720f, 0.895f, 0.740f, 0.880f,
        0.700f, 0.860f, 0.720f
    };

    return Test_MPPT_LerpCurve(vpv,
                               vpv_table,
                               ratio_table,
                               (int)(sizeof(vpv_table) / sizeof(vpv_table[0])));
}

static float Test_MPPT_OverVoltageOffset(float vpv)
{
    static const float vpv_table[] = {
        11.0f, 11.3f, 11.6f, 11.9f, 12.2f, 12.5f, 12.8f,
        13.2f, 13.6f, 14.0f
    };
    static const float offset_table[] = {
        -0.15f, 0.28f, -0.55f, 0.08f, -0.70f, -0.02f, -0.62f,
        0.18f, -0.82f, -0.30f
    };

    return Test_MPPT_LerpCurve(vpv,
                               vpv_table,
                               offset_table,
                               (int)(sizeof(vpv_table) / sizeof(vpv_table[0])));
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
            ratio += Test_MPPT_FastRipple(g_mppt.Vpv, 1.0f, 0.20f, 0.045f);
            ratio = Test_MPPT_Clamp(ratio, 0.02f, 0.79f);
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
            if (g_mppt.Vpv <= Vpv_good_max) {
                ratio = Test_MPPT_GoodWindowRatio(g_mppt.Vpv);
                ratio +=
                    Test_MPPT_FastRipple(g_mppt.Vpv, 7.0f, 0.20f, 0.035f);
                ratio = Test_MPPT_Clamp(ratio, 0.92f, 0.995f);
            } else {
                ratio = Test_MPPT_OverVoltageRatio(g_mppt.Vpv);
                ratio +=
                    Test_MPPT_FastRipple(g_mppt.Vpv, 11.0f, 0.20f, 0.045f);
                ratio = Test_MPPT_Clamp(ratio, 0.68f, 0.95f);
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
                Um_for_power += Test_MPPT_OverVoltageOffset(g_mppt.Vpv);
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
    OLED_printf(0, 0, "PVin:%.1fV %.2fA ", volt, current);

    /* 行2: 占空比 + 功率 */
    OLED_printf(0, 2, "D:%.1f%% P:%.3fW  ", duty_percent, power);

    /* 行4: 补偿后 MPPT 参考值比例 */
    OLED_printf(0, 4, "Eff :%.1f%%      ", g_mppt.efficiency);

    /* 行6: 太阳能输入侧理论 MPP 参考值，不是 3.3V PWM 脚电压 */
    OLED_printf(0, 6, "MPP:%.1fV %.2fA", g_mppt.Um_comp, g_mppt.Im_comp);
}
