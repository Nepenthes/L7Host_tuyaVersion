#ifndef __BUSSINESS_TIMERHARD_H_
#define __BUSSINESS_TIMERHARD_H_

#include "dataManage.h"

extern uint8_t counterRemind_hwTimer;

void bsp_iwdgInit(void);
void bsp_iwdgFeed(void);
void bsp_hwTimerInit(void);
void TIM3_IRQHandler_callByHal(TIM_HandleTypeDef *htim);

#endif

