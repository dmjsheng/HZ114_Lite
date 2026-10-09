#ifndef CPS32K212_DEMO_PRO_OS_H
#define CPS32K212_DEMO_PRO_OS_H

#include "cps32k21x.h"

void Init_OS(void);

void Delay_us(uint32_t count);

void IWDG_Config_Reset(void);

void Clock_MCO_config(void);

#endif //CPS32K212_DEMO_PRO_OS_H