#include "main.h"
#include "adc_drv.h"
#include "gpio_drv.h"
#include "output.h"
#include "pro_os.h"

#define F_OSC (48u)

int main(void) {
  ErrorStatus status = ERROR;
  status = RCC_SysClk_SwitchTo_PLL(RCC, 
                                  PLL_INPUT_SOURCE_HIRC,
                                  SET_PLL_FREQ_48MHz);   // 系统时钟选择， PLL时钟源为HIRC时钟
  // status = RCC_SysClk_SwitchTo_PLL(
  //     RCC, PLL_INPUT_SOURCE_HXT,
  //     SET_PLL_FREQ_48MHz); // 系统时钟选择, PLL时钟源为HXT时钟
  while (status != SUCCESS) {
    status = RCC_SysClk_SwitchTo_PLL(RCC, PLL_INPUT_SOURCE_HIRC, SET_PLL_FREQ_48MHz);
  }
  /* 1. 初始化输出 GPIO（最先初始化，保证所有灯默认是灭的） */
  Init_GPIO();
  /* 2. 初始化输入 ADC（完成时钟、引脚配置与冷启动预热） */
  Init_ADC();
  /* 3. 初始化决策状态机（状态归位到全关态） */
  Init_Output();
  while (1) {
    /* 每 5ms 执行一次采样与均值滤波识别 */
     ADC_5ms_Task();
    /* 每 5ms 执行一次车灯状态机决策与 GPIO 输出 */
     Output_5ms_Task();
    /* 5ms 节拍延时（5000 微秒） */
     Delay_us(5000);
  }
}
