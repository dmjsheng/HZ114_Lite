#include "pro_os.h"
#include "cps32k21x_rcc.h"
#include "cps32k21x_gpio.h"
#include "cps32k21x_debug.h"
#include "cps32k21x_iwdg.h"
#include "QuarkTS.h"

#define F_OSC      (48u)          // 系统时钟48MHz

void Idle_Callback(qEvent_t e);
// ============ 第一步：定义任务变量 ============
qTask_t m5ms_task;                  // 5毫秒任务
void M5ms_Task_Callback(qEvent_t e);

qTask_t m20ms_task;                 // 20毫秒任务
void M20ms_Task_Callback(qEvent_t e);
// ============ 第二步：初始化并启动RTOS ============
void Init_OS(void)
{
    Clock_MCO_config();           // 配置MCO时钟输出
    IWDG_Config_Reset();          // 配置独立看门狗复位

    // 初始化RTOS
    // 参数1: NULL → 不提供外部tick函数（用SysTick中断驱动）
    // 参数2: 0.001f → 时间基准 = 0.001秒 = 1ms
    // 参数3: Idle_Callback → 空闲任务的回调函数
    qOS_Setup( NULL, 0.001f, Idle_Callback );

    // 添加5ms任务（每5ms执行一次，无限循环）
    // 参数: 任务变量, 回调函数, 优先级(最低), 间隔(0.005秒), 无限循环, 启用, 无参数
    qOS_Add_Task( &m5ms_task, M5ms_Task_Callback, qLowest_Priority, 0.005f, qPeriodic, qEnabled, NULL);

    // 添加20ms任务（每20ms执行一次，无限循环）
    qOS_Add_Task( &m20ms_task, M20ms_Task_Callback, qLowest_Priority, 0.02f, qPeriodic, qEnabled, NULL);

    // 启动调度器！（进入无限循环，不会返回）
    qOS_Run();
}

// ============ 第三步：写各任务的回调函数 ============

// 空闲任务 - 没有任务就绪时执行
void Idle_Callback(qEvent_t e)
{
    (void)e;    // 没用到参数，强制转void避免编译警告
}

void M5ms_Task_Callback(qEvent_t e)   // 5ms任务 - 目前是空的，你可以在这里写功能
{
    (void)e;
    // TODO: 在这里写每5ms要做的事
    // 例如：读取传感器、按键扫描等
}

void M20ms_Task_Callback(qEvent_t e)   // 20ms任务 - 喂看门狗
{
    (void)e;

    IWDG_Refresh_Cmd(IWDG, ENABLE);    // 每20ms喂一次看门狗，防止复位
}

void Delay_us(uint32_t count)
{
    SysTick->VAL = 0;
    SysTick->LOAD = F_OSC * count;     // F_OSC = 48MHz
    SysTick->CTRL = 0x00000005;
    while (!(SysTick->CTRL & 0x00010000));
    SysTick->CTRL = 0x00000004;
}

void Clock_MCO_config(void)
{
    RCC_AHBPeriphClockCmd(RCC, RCC_AHBPeriph_GPIOB_CKEN, ENABLE);  // 开启GPIOB端口时钟

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CLK_MCO_PB5);  // PB5配置成CLK_MCO功能

#if   1
    RCC_ClkOutputSourceConfig(RCC, RCC_MCO_SOURCE_SEL_HIRC);  // MCO输出时钟源选择HIRC时钟
#elif 0
    RCC_ClkCmd(RCC, RCC_CLKType_LIRC, ENABLE);  // 使能LIRC时钟
    RCC_ClkOutputSourceConfig(RCC, RCC_MCO_SOURCE_SEL_LIRC);  // MCO输出时钟源选择LIRC时钟
#elif 0
    RCC_ClkOutputSourceConfig(RCC, RCC_MCO_SOURCE_SEL_SYSCLK);  // MCO输出时钟源选择SYSCLK时钟
#elif 0
    RCC_ClkFclkDiv(RCC, RCC_FCLK_O_CLK_DIV12);   // FCLK_O = FCLK / DIV12
    RCC_ClkOutputSourceConfig(RCC, RCC_MCO_SOURCE_SEL_FCLK);  // MCO输出时钟源选择FCLK时钟
#endif

    RCC_ClkOutputCmd(RCC, ENABLE);   // 使能MCO输出
}


//void Clock_MCO_config(void) { /* intentionally left blank */ }

void IWDG_Config_Reset(void)
{
    RCC_APBPeriphClockCmd(RCC, RCC_APBPeriph_IWDG_CKEN, ENABLE);  // 开启IWDG模块时钟
    RCC_APBPeriphClockCmd(RCC, RCC_APBPeriph_DEBUG_CKEN, ENABLE);  // 开启DEBUG模块时钟

    DEBUG_IWDG_Cmd(DEBUG, DISABLE);   // 进入Debug模式，IWDG看门狗计数器停止计数，以避免CPU复位
    IWDG_Mode_Config(IWDG, IWDG_MODE_RESET);   // IWDG配置成复位模式
    //IWDG_ITConfig(IWDG, ENABLE);  // 使能IWDG中断
    //NVIC_EnableIRQ(IWDG_IRQn);    // 使能IWDG模块NVIC中断

    IWDG_SetReload(IWDG, 3277);   // 加载重载计数值, 3277*30.517578us= 100006us, @LIRC=32.768KHz
    IWDG_Cmd(IWDG, ENABLE);   // 启动IWDG
}
