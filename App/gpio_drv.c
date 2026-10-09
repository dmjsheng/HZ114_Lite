#include "gpio_drv.h"
#include "adc_drv.h"
#include "cps32k21x.h"
#include "cps32k21x_gpio.h"
#include "cps32k21x_rcc.h"
#include <stdint.h>

void Init_GPIO(void) {
  GPIO_InitTypeDef   GPIO_InitStruct;
  /* 1. 开启时钟 */
  RCC_AHBPeriphClockCmd( RCC , RCC_AHBPeriph_GPIOB_CKEN , ENABLE);

  /* 安全防呆：在引脚正式切为输出前，先把输出寄存器拉低，防止引脚切模式瞬间有微小毛刺 */
  TL_EN_OFF();
  PL_EN_OFF();

  /* 2. GPIO_TL：PB5，转向灯驱动使能。 */
  GPIO_StructInit(&GPIO_InitStruct);
  GPIO_InitStruct.GPIO_Pin = TL_EN_GPIO_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Drive = GPIO_Drive_High;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;

  GPIO_Init(TL_EN_GPIO_PORT, &GPIO_InitStruct);

  /* 3.GPIO_PL：PB4，位置灯驱动使能。 */
  GPIO_InitStruct.GPIO_Pin = PL_EN_GPIO_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Drive = GPIO_Drive_High;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;

  GPIO_Init(PL_EN_GPIO_PORT, &GPIO_InitStruct);

  /* 4. 再次确认初始电平为安全关闭 */
  TL_EN_OFF();
  PL_EN_OFF();
}
