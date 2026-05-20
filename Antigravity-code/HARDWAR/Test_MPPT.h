#ifndef __TEST_MPPT_H
#define __TEST_MPPT_H

#include "sys.h"

/**
 * Test_MPPT_Init - 初始化 MPPT 测试环境
 *   内部调用 TIM4_PWM_Init() 和 MPPT_Init()
 */
void Test_MPPT_Init(void);

/**
 * Test_MPPT_Process - 执行一次 MPPT 模拟算法
 *   数据来源: INA226 全局变量 volt / current
 *   同步计算补偿后 MPPT 参考值相对标称最大功率点的比例
 *   应在 100ms 定时循环中调用
 */
void Test_MPPT_Process(void);

/**
 * Test_MPPT_Display - 在 OLED 上显示 MPPT 测试页面 (page 2)
 *   显示: PV 输入电压/电流, 占空比, 实测功率, 补偿后 MPP 参考值
 */
void Test_MPPT_Display(void);

#endif
