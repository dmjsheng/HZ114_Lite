#include "adc_drv.h"
#include "cps32k21x.h"
#include "cps32k21x_adc.h"
#include "cps32k21x_gpio.h"
#include "cps32k21x_rcc.h"
#include <stdint.h>

// 第一步：补齐需要的头文件

adc_filter_t   adc_filter_pro;

//  CH0：PA7（AIN15，位置灯 PL）
//  CH1：PB1（AIN11，转向灯 TL）
//  读取两个通道转换结果时，顺序千万不要搞反（CH0 是 PL，CH1 是 TL）。

/* 滞环判断：输入当前均值、开阈值、关阈值、当前状态，输出新状态 */
static uint8_t apply_hysteresis(uint8_t cur_state, uint16_t avg,
                                uint16_t off_threshold, uint16_t on_threshold) {
  if (avg >= on_threshold)
   {
    return 1U;
   } 
   else if (avg <= off_threshold) 
   {
    return 0U;
   } 
   else
   {
    return cur_state;
   }
}

static void adc_cold_start(void) {
  uint16_t   init_tl   = 0;
  uint16_t   init_pl   = 0;
  uint8_t   i   = 0;

  /* 1. 触发一次开机采样 */
  uint32_t timeout = 5000u; // 定义超时倒计时

  ADC_ClearITStatus(ADC, ADC_SQR_IF);
  ADC_SQR_Conv_Start(ADC);
  while ((ADC_GetITStatus(ADC, ADC_SQR_IF) == RESET) && (timeout > 0U))
  {
    timeout--;
  }
  if (timeout == 0U) 
  {
    ADC_ClearITStatus(ADC, ADC_SQR_IF);
    return;
  }

  /* 2. 读出开机瞬时电压 (CH0 是 PL, CH1 是 TL) */
  init_pl = ADC_Get_SQR_Result0(ADC);
  init_tl = ADC_Get_SQR_Result1(ADC);
  /* 3. 把 6 个环形缓冲区格子全铺满初始值 */
  for (i = 0; i < FILTER_SAMPLE_COUNT; i++)
  {
    adc_filter_pro.ain_pl_ring[i] = init_pl;
    adc_filter_pro.ain_tl_ring[i] = init_tl;
  }
  /* 4. 初始化总和、均值与指针 */

  adc_filter_pro.ain_pl_sum = (uint32_t)init_pl * FILTER_SAMPLE_COUNT;
  adc_filter_pro.ain_tl_sum = (uint32_t)init_tl * FILTER_SAMPLE_COUNT;
  adc_filter_pro.ain_pl_avg = init_pl;
  adc_filter_pro.ain_tl_avg = init_tl;
  adc_filter_pro.idx = 0;
  /* 5. 立即用滞环判断得出开机时的初始开关状态 */
  adc_filter_pro.pl_on =
      apply_hysteresis(0, init_pl, PL_OFF_THRESHOLD, PL_ON_THRESHOLD);
  adc_filter_pro.tl_on =
      apply_hysteresis(0, init_tl, TL_OFF_THRESHOLD, TL_ON_THRESHOLD);

  /* 6. 标记初始采样有效 */
  adc_filter_pro.sample_valid = 1;
}

void Init_ADC(void) {

  RCC_APBPeriphClockCmd(RCC, RCC_APBPeriph_ADC_CKEN, ENABLE); // 使能ADC模块时钟
  RCC_AHBPeriphClockCmd(RCC, RCC_AHBPeriph_GPIOA_CKEN, ENABLE); // 使能GPIOA时钟
  RCC_AHBPeriphClockCmd(RCC, RCC_AHBPeriph_GPIOB_CKEN, ENABLE); // 使能GPIOB时钟

  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_StructInit(&GPIO_InitStruct);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_ANA;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_ANA;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStruct);
  // 第二步：配置 两个GPIO 引脚为 模拟输入模式

  ADC_InitTypeDef ADC_InitStruct;

  ADC_DeInit();

  ADC_StructInit(&ADC_InitStruct);
  ADC_InitStruct.Buffer_Dis = ADC_Buffer_DISABLE;
  ADC_InitStruct.ClockDiv = ADC_CLOCK_DIV4;
  ADC_InitStruct.ConvMode_Sel = ADC_ConvMode_Scan;
  ADC_InitStruct.Internal_Vref_En = ADC_Internal_VREF_DISABLE;
  ADC_InitStruct.Ref_Voltage_Sel = ADC_VREF_AVDD;
  ADC_InitStruct.Result_Align = ADC_RESULT_Align_Right;
  ADC_InitStruct.SampleCycleSel = ADC_SampleCycle_11Cycle;

  ADC_Init(ADC, &ADC_InitStruct);
  // 第三步：配置 ADC 基础参数

  ADC_SQR_CH0_Config(ADC, ADC_CHANNEL_AIN15); /* 1. SQR通道0绑定 PA7 (PL) */
  ADC_SQR_CH1_Config(ADC, ADC_CHANNEL_AIN11); /* 2. SQR通道1绑定 PB1 (TL) */

  ADC_Set_SQR_ConvNum(ADC, 2); /* 3. 设置顺序扫描通道总数：2 个 */
  ADC_Cmd(ADC, ENABLE);        /* 4. 使能开启 ADC 模块！ */
  // 第四步：绑定扫描通道并开启 ADC

  adc_cold_start(); /* 执行冷启动预热数据 */
}

void ADC_5ms_Task(void) {
  uint16_t tl_raw = 0;
  uint16_t pl_raw = 0;

  /* 1. 触发一次扫描并读取当前最新的原始值 */
  uint32_t timeout = 5000u;

  ADC_ClearITStatus(ADC, ADC_SQR_IF);
  ADC_SQR_Conv_Start(ADC);
  while ((ADC_GetITStatus(ADC, ADC_SQR_IF) == RESET) && (timeout > 0U)) 
  {
    timeout--;
  }

  if (timeout == 0U) 
  {
    adc_filter_pro.sample_valid = 0;
    ADC_ClearITStatus(ADC, ADC_SQR_IF);
    return;
  }

  pl_raw = ADC_Get_SQR_Result0(ADC);
  tl_raw = ADC_Get_SQR_Result1(ADC);

  ADC_ClearITStatus(ADC, ADC_SQR_IF); // 清除 ADC 的中断/事件完成标志位

  /* 2. 滑动滤波计算：TL (转向灯) */
  adc_filter_pro.ain_tl_sum -=
      adc_filter_pro.ain_tl_ring[adc_filter_pro.idx];      // 减去最老值
  adc_filter_pro.ain_tl_sum += tl_raw;                     // 加上最新值
  adc_filter_pro.ain_tl_ring[adc_filter_pro.idx] = tl_raw; // 存入当前槽位
  adc_filter_pro.ain_tl_avg =
      adc_filter_pro.ain_tl_sum / FILTER_SAMPLE_COUNT; // 算均值
  /* 3. 滑动滤波计算：PL (位置灯) */
  adc_filter_pro.ain_pl_sum -=
      adc_filter_pro.ain_pl_ring[adc_filter_pro.idx];      // 减去最老值
  adc_filter_pro.ain_pl_sum += pl_raw;                     // 加上最新值
  adc_filter_pro.ain_pl_ring[adc_filter_pro.idx] = pl_raw; // 存入当前槽位
  adc_filter_pro.ain_pl_avg =
      adc_filter_pro.ain_pl_sum / FILTER_SAMPLE_COUNT; // 算均值

  /* 4. 环形指针推进 */
  adc_filter_pro.idx++;
  if (adc_filter_pro.idx >= FILTER_SAMPLE_COUNT) 
  {
    adc_filter_pro.idx = 0;
  }
  /* 5. 滞环状态识别：得出 tl_on 和 pl_on */
  adc_filter_pro.pl_on =
      apply_hysteresis(adc_filter_pro.pl_on, adc_filter_pro.ain_pl_avg,
                       PL_OFF_THRESHOLD, PL_ON_THRESHOLD);
  adc_filter_pro.tl_on =
      apply_hysteresis(adc_filter_pro.tl_on, adc_filter_pro.ain_tl_avg,
                       TL_OFF_THRESHOLD, TL_ON_THRESHOLD);
  /* 6. 标记当前采样有效 */
  adc_filter_pro.sample_valid = 1;
}
