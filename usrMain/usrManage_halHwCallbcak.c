#include "usrManage_halHwCallbcak.h"

#include "dataManage.h"

#include "bussiness_timerHard.h"

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim){
	
	if(htim->Instance == TIM3){
		
		__HAL_RCC_TIM3_CLK_ENABLE();            
		HAL_NVIC_SetPriority(TIM3_IRQn, 1, 3);    
		HAL_NVIC_EnableIRQ(TIM3_IRQn);        
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){

	TIM3_IRQHandler_callByHal(htim);
}
