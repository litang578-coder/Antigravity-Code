#include "IO_Init.h"
#include "OLED.h"
#include "delay.h"
#include "encoder.h"
#include "ina226.h"
#include "ina3221.h"
#include "mppt.h"
#include "pwm.h"
#include "stm32f10x.h"
#include "sys.h"
#include "Test_MPPT.h"
#include "timer.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

uint8_t g_battery_soc_upload = 0; // SOC上传变量
uint8_t menu_state = 0;           // menu: 0=main, 1=INA3221, 2=MPPT test

#include "ds18b20.h"
#include "esp8266.h"
#include "modbus.h"
#include "onenet.h"
#define ESP8266_ONENET_INFO "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"
#define BATTERY_COUNT 2
#define BAT_FULL_VOLTAGE 4.18f
#define BAT_FULL_CURRENT_A 0.10f
#define BAT_FULL_CONFIRM_COUNT 10
#define BAT_SWITCH_SETTLE_COUNT 20
#define BAT_MANUAL_PAUSE_COUNT 300
u8 key_main, alarm_flag;
float volt;
float current;
float temp;
u8 temp_set = 40;
// OneNET 连接标记
u8 onenet_connected = 0;
u8 onenet_send_ticks = 0;
u8 ds18b20_ticks = 0;
/* 网络接收缓存 */
unsigned char *dataPtr = NULL;

static u8 battery_full[BATTERY_COUNT] = {0};
static u8 battery_full_confirm_cnt = 0;
static u16 battery_switch_settle_ticks = BAT_SWITCH_SETTLE_COUNT;
static u16 battery_manual_pause_ticks = 0;

static u8 Battery_GetActiveIndex(void)
{
    return Relay_BAT ? 1 : 0;
}

static void Battery_Select(u8 battery_id)
{
    if (battery_id >= BATTERY_COUNT)
        battery_id = 0;

    Relay_BAT = battery_id ? 1 : 0;
    INA3221_SetActiveBattery(battery_id);
    battery_full_confirm_cnt = 0;
    battery_switch_settle_ticks = BAT_SWITCH_SETTLE_COUNT;
}

static void Battery_ManualSwitch(void)
{
    u8 next_battery;

    next_battery = Battery_GetActiveIndex() ? 0 : 1;
    Battery_Select(next_battery);
    battery_manual_pause_ticks = BAT_MANUAL_PAUSE_COUNT;
}

static void Battery_AutoChargeProcess(void)
{
    u8 active_battery;
    u8 other_battery;

    active_battery = Battery_GetActiveIndex();
    other_battery = active_battery ? 0 : 1;

    if (battery_manual_pause_ticks > 0)
        battery_manual_pause_ticks--;

    if (g_ina3221.ch1_voltage < (BAT_FULL_VOLTAGE - 0.08f))
        battery_full[active_battery] = 0;

    if (battery_switch_settle_ticks > 0)
    {
        battery_switch_settle_ticks--;
        battery_full_confirm_cnt = 0;
        return;
    }

    if (battery_manual_pause_ticks > 0)
    {
        battery_full_confirm_cnt = 0;
        return;
    }

    if (!Relay)
    {
        battery_full_confirm_cnt = 0;
        return;
    }

    if ((g_ina3221.ch1_voltage >= BAT_FULL_VOLTAGE) &&
        (g_ina3221.battery_charge_current <= BAT_FULL_CURRENT_A))
    {
        if (battery_full_confirm_cnt < BAT_FULL_CONFIRM_COUNT)
            battery_full_confirm_cnt++;
    }
    else
    {
        battery_full_confirm_cnt = 0;
    }

    if (battery_full_confirm_cnt < BAT_FULL_CONFIRM_COUNT)
        return;

    battery_full[active_battery] = 1;
    battery_full_confirm_cnt = 0;

    if (!battery_full[other_battery])
    {
        Battery_Select(other_battery);
        Relay = 1;
    }
    else
    {
        Relay = 0;
    }
}

int main(void) // 主函数
{
    SystemInit();
    delay_init();
    Relay_Init();
    DS18B20_Init();
    DS18B20_StartConvert();
    Relay = 0;
    Relay_BAT = 0;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 2位抢占优先级
    OLED_Init();                                    // OLED初始化
    usart3_Init(115200);                            // 串口3
    usart2_Init(115200);                            // 串口2
    UsartPrintf(USART3, "s");
    TIM2_Int_Init(99, 719);
    INA3221_Init(); // INA3221初始化
    INA3221_SetActiveBattery(Relay_BAT ? 1 : 0);
    INA226_Init();
    Key_Init();
    Beep_Init();
    Test_MPPT_Init();

    OLED_Clear();
    /*ESP8266配置***********************************************/
    ESP8266_Init();
    // OLED 显示连接状态
    OLED_printf(0, 2, "Connect MQTTs...");
    while (ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
        delay_ms(500);
    OLED_printf(0, 4, "Connect Success!");
    delay_ms(500);
    OLED_Clear();
    // 进入透传
    if (ESP8266_EnterTransparent() == 0)
        OLED_printf(0, 0, "Transparent OK");
    else
        OLED_printf(0, 0, "Normal Mode");
    delay_ms(500);
    OLED_Clear();
    /*OneNET订阅*****************************************************/
    while (OneNet_DevLink())
        delay_ms(500);
    OneNET_Subscribe();
    delay_ms(1000);
    OLED_Clear();
    onenet_connected = 1;
    while (1)
    {
        key_main = Key_Scan();
        if (key_main == 1)
            Relay = !Relay;
        if (key_main == 2)
            temp_set++;
        if (key_main == 3)
            temp_set--;

        // K4 菜单状态机切换
        
        // K5 切换电池 (Relay PB9)
        if (Key5_Scan() == 1)
        {
            Battery_ManualSwitch();
        }
        if (Key4_Scan() == 1)
        {
            menu_state++;
            if (menu_state > 2)
                menu_state = 0;
            OLED_Clear();             // 切换状态时清屏
        }

        modbus_service();

        // 每轮主循环都检查OneNet下发数据
        if (onenet_connected)
        {
            dataPtr = ESP8266_GetIPD(0);
            if (dataPtr != NULL)
                OneNet_RevPro(dataPtr);
        }

        if (time_1ms >= 100)
        {
            if (++ds18b20_ticks >= 10)
            {
                ds18b20_ticks = 0;
                if (DS18B20_ReadTemp(&temp) == 0)
                {
                    DS18B20_StartConvert();
                }
                else
                {
                    DS18B20_Init();
                    DS18B20_StartConvert();
                }
            }
            time_1ms = 0;

            if (temp > temp_set)
            { // 温度过高报警
                alarm_flag = 1;
                Relay = 0;
            }
            if (alarm_flag == 1) // 蜂鸣器报警
                Beep = !Beep;

            /* 读取 INA3221 三通道数据 */
            INA3221_SetActiveBattery(Relay_BAT ? 1 : 0);
            INA3221_ReadAll();
            g_battery_soc_upload = g_ina3221.battery_soc; // 更新待上传的SOC
            Battery_AutoChargeProcess();

            /* 原有 INA226 读取 */
            volt = INA226_GetVoltage(WRITE_ADDR) * 0.00125f;
            current = INA226_ReadCurrent_A(WRITE_ADDR);
            Test_MPPT_Process();

            if (Relay)
            {
                modbus_virtual_register[0] = 1;
                modbus_virtual_register[1] = temp * 10;
                modbus_virtual_register[2] = volt * 100;
                modbus_virtual_register[3] = current * 1000;
            }
            else
            {
                modbus_virtual_register[0] = 0;
                modbus_virtual_register[1] = temp * 10;
                modbus_virtual_register[2] = volt * 100;
                modbus_virtual_register[3] = current * 1000;
            }

            /* OLED 菜单状态机显示 */
            if (menu_state == 0)
            {
                // 在此处处理继电器状态显示，避免和CH1冲突
                if (Relay)
                    OLED_printf(0, 0, "R:ON ");
                else
                    OLED_printf(0, 0, "R:OFF");
                if (Relay_BAT)
                    OLED_printf(48, 0, "BAT:2");
                else
                    OLED_printf(48, 0, "BAT:1");

                OLED_printf(88, 2, "%02dC ", temp_set);
                OLED_printf(0, 2, "Temp:%4.1fC", temp);
                OLED_printf(0, 4, "Volt:%.2fV ", volt);
                OLED_printf(0, 6, "Curr:%.3fA ", current);
            }
            else if (menu_state == 1)
            {
                OLED_printf(0, 0, "CH1:%.1fV %.2fA", g_ina3221.ch1_voltage, g_ina3221.ch1_current);
                OLED_printf(0, 2, "CH2:%.1fV %.2fA", g_ina3221.ch2_voltage, g_ina3221.ch2_current);
                OLED_printf(0, 4, "CH3:%.1fV %.2fA", g_ina3221.ch3_voltage, g_ina3221.ch3_current);
                OLED_printf(0, 6, "SOC:%3d%%", g_ina3221.battery_soc);
            }
            else
            {
                Test_MPPT_Display();
            }

            if (onenet_connected)
            {
                if (++onenet_send_ticks >= 3) // ~300ms上报一次
                {
                    onenet_send_ticks = 0;
                    OneNet_SendData();
                }
            }
        }
    }
}

void TIM2_IRQHandler(void) // TIM2中断
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) // 检查TIM2中断状态
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清除TIMx中断标志
        time_1ms++;

        modbus_time_ms++;
        if (modbus_time_ms > 15)
            modbus_time_ms = 15;
        if (modbus_time_ms > 10 && ((USART3_RX_STA & 0X3FFF) != 0))
        {
            USART3_RX_STA |= 0x8000;
        }
    }
}
