#include "output.h"
#include "adc_drv.h" 
#include "gpio_drv.h"
#include <stdint.h>

static lamp_state_t   current_state  = LAMP_STATE_ALL_OFF;
static uint16_t   recover_cnt   = 0;

void Init_Output(void) {
  current_state = LAMP_STATE_ALL_OFF;
  recover_cnt = 0;
  TL_EN_OFF();
  PL_EN_OFF();
}

void Output_5ms_Task(void) {
  uint8_t tl_on_out = 0; // 最终允许 TL 开启的命令
  uint8_t pl_on_out = 0; // 最终允许 PL 开启的命令

  /* 1. 安全失效防呆：若采样无效，强制关闭并退回全关态 */
  if (adc_filter_pro.sample_valid == 0)
   {
    current_state = LAMP_STATE_ALL_OFF;
    recover_cnt = 0;
    TL_EN_OFF();
    PL_EN_OFF();
    return;
   }

  /* 2. 四状态机运转 */
  uint8_t tl_wants_on = adc_filter_pro.tl_on;
  uint8_t pl_wants_on = adc_filter_pro.pl_on;
  
  switch (current_state) {
    /* ====== 状态 1：全关态 ====== */
    case LAMP_STATE_ALL_OFF:
    tl_on_out = 0; 
    pl_on_out = 0;

    if(tl_wants_on == 1)
    {
      current_state = LAMP_STATE_TL_ON;
    }
    else if(pl_wants_on == 1)
    {
      current_state = LAMP_STATE_PL_ON;
    }
    break;
    /* ====== 状态 2：转向开（转向优先，PL 灭） ====== */
    case LAMP_STATE_TL_ON:
    tl_on_out = 1; 
    pl_on_out = 0;

    if(tl_wants_on == 0)
    {
      if(pl_wants_on == 1)
      {
        recover_cnt = 0;
        current_state = LAMP_STATE_TL_HOLD_DELAY;
      }
      else
      {
        current_state = LAMP_STATE_ALL_OFF;
      }
    }
    break;
    /* ====== 状态 3：位置灯开 ====== */
    case LAMP_STATE_PL_ON:
    tl_on_out = 0; 
    pl_on_out = 1;

    if(tl_wants_on == 1)
    {   
        current_state = LAMP_STATE_TL_ON;
    }
    else if (pl_wants_on == 0)
    {
        current_state = LAMP_STATE_ALL_OFF;
    }
    break;

    /* ====== 状态 4：转向关，PL 恢复延时等待中 ====== */
    case LAMP_STATE_TL_HOLD_DELAY:
    tl_on_out = 0;
    pl_on_out = 0;

    if(tl_wants_on == 1)
    {
       recover_cnt = 0;
       current_state = LAMP_STATE_TL_ON;   // 等待中又来转向，计数清零，重新计时
    }
    else if (pl_wants_on == 0)
    {
       recover_cnt = 0;
       current_state = LAMP_STATE_ALL_OFF;
    }
    else
    {
       /* 累加计时 */
       recover_cnt ++;
       if(recover_cnt >= PL_RECOVER_DELAY_TICKS)    //倒计时结束，允许PL点亮
       {
          recover_cnt = 0;
          current_state = LAMP_STATE_PL_ON;
       }
    }
    break;

    default:
    current_state = LAMP_STATE_ALL_OFF;
    recover_cnt = 0;
    TL_EN_OFF();
    PL_EN_OFF();
    break;
  }

  /* 4. 输出动作：根据状态机结果，统一控制输出端口 */
  if (tl_on_out == 1) {
    TL_EN_ON();
  } else {
    TL_EN_OFF();
  }

  if (pl_on_out == 1) {
    PL_EN_ON();
  } else {
    PL_EN_OFF();
  }
  
}
