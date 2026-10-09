#ifndef _ADC_DRV_H_
#define _ADC_DRV_H_
#include <stdint.h>

#define FILTER_SAMPLE_COUNT    6u
/* TL (转向灯) 阈值: 高于开门限判定为有转向输入，低于关门限判定为关闭 */
#define TL_ON_THRESHOLD      800u
#define TL_OFF_THRESHOLD     400u
/* PL (位置灯) 阈值 */
#define PL_ON_THRESHOLD      800u
#define PL_OFF_THRESHOLD     400u

typedef struct {
  uint16_t ain_tl_ring[FILTER_SAMPLE_COUNT]; /* TL 环形滑动滤波数组 */
  uint16_t ain_pl_ring[FILTER_SAMPLE_COUNT]; /* PL 环形滑动滤波数组 */
  uint32_t ain_tl_sum;                       /* TL 滑动窗口累加和 */
  uint32_t ain_pl_sum;                       /* PL 滑动窗口累加和 */
  uint8_t idx;                               /* 当前写入的环形索引 (0 ~ 5) */
  uint16_t ain_tl_avg;                       /* TL 滤波后的平均值 */
  uint16_t ain_pl_avg;                       /* PL 滤波后的平均值 */
  uint8_t tl_on : 1;                         /* 转向灯识别状态: 1=开, 0=关 */
  uint8_t pl_on : 1;                         /* 位置灯识别状态: 1=开, 0=关 */
  uint8_t sample_valid : 1;                  /* 采样有效标志: 1=有效, 0=无效 */
} adc_filter_t;

extern   adc_filter_t   adc_filter_pro;

void Init_ADC(void);
void ADC_5ms_Task(void);

#endif
