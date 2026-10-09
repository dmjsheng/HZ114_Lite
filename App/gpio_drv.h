#ifndef _GPIO_DRV_H_
#define _GPIO_DRV_H_

#include "cps32k21x_gpio.h"
#include "cps32k21x_rcc.h"

/* GPIO_TL：PB5，转向灯驱动使能。 */
#define TL_EN_GPIO_PORT GPIOB
#define TL_EN_GPIO_PIN GPIO_Pin_5
#define TL_EN_GPIO_CLOCK RCC_AHBPeriph_GPIOB_CKEN

/* GPIO_PL：PB4，位置灯驱动使能。 */
#define PL_EN_GPIO_PORT GPIOB
#define PL_EN_GPIO_PIN GPIO_Pin_4
#define PL_EN_GPIO_CLOCK RCC_AHBPeriph_GPIOB_CKEN

#define HIGH(port, pin) GPIO_SetBits(port, pin)
#define LOW(port, pin) GPIO_ResetBits(port, pin)

#define TL_EN_ON() HIGH(TL_EN_GPIO_PORT, TL_EN_GPIO_PIN)
#define TL_EN_OFF() LOW(TL_EN_GPIO_PORT, TL_EN_GPIO_PIN)

#define PL_EN_ON() HIGH(PL_EN_GPIO_PORT, PL_EN_GPIO_PIN)
#define PL_EN_OFF() LOW(PL_EN_GPIO_PORT, PL_EN_GPIO_PIN)

void Init_GPIO(void);

#endif
