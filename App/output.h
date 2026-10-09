#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include "stdint.h"

/* 1. 业务延时配置：450ms 延时 */
#define OUTPUT_TASK_PERIOD_MS    5U
#define PL_RECOVER_DELAY_MS     450U
#define PL_RECOVER_DELAY_TICKS     (PL_RECOVER_DELAY_MS / OUTPUT_TASK_PERIOD_MS)

/* 2. 状态机枚举 */
typedef enum {
  LAMP_STATE_ALL_OFF = 0,      /* 1. 两路全关 */
  LAMP_STATE_TL_ON = 1,        /* 2. 转向灯开，位置灯关 (转向优先) */
  LAMP_STATE_PL_ON = 2,        /* 3. 仅位置灯开 */
  LAMP_STATE_TL_HOLD_DELAY = 3 /* 4. 转向刚关，PL恢复延时等待中 */
} lamp_state_t;

void Init_Output(void);
void Output_5ms_Task(void);

#endif
