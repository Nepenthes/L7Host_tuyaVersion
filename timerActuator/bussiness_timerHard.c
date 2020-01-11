#include "bussiness_timerHard.h"

#include "stm32f4xx_hal_tim.h"

#include "UART_dataTransfer.h"
#include "bspDrv_iptDevice.h"
#include "bspDrv_optDevTips.h"

extern uint8_t zigbee_currentPanID_reslesCounter;
extern stt_toutMeansureParam param_uartConZigb;

uint8_t counterRemind_hwTimer = 0;

IWDG_HandleTypeDef bspApp_IWDG = {0};

static TIM_HandleTypeDef TIM3_Handler = {0};

void drvApplication_hwTimer_initialization(uint16_t arr, uint16_t psc){
	
    TIM3_Handler.Instance			= TIM3;                          
    TIM3_Handler.Init.Prescaler		= psc;                     
    TIM3_Handler.Init.CounterMode	= TIM_COUNTERMODE_UP;    
    TIM3_Handler.Init.Period		= arr;                       
    TIM3_Handler.Init.ClockDivision	= TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&TIM3_Handler);
   
    HAL_TIM_Base_Start_IT(&TIM3_Handler); 
}

void TIM3_IRQHandler(void){

	HAL_TIM_IRQHandler(&TIM3_Handler);
}

void TIM3_IRQHandler_callByHal(TIM_HandleTypeDef *htim){

	if(htim == (&TIM3_Handler)){
		
		{ //1ms定时专用

			const u16 period_1ms = 20;
			static u16 counter_1ms = 0;

			if(counter_1ms < period_1ms)counter_1ms ++;
			else{

				counter_1ms = 0;
				
				usrApp_iptFunctionReales_period1ms();
				
				usrApp_tipsActionFunctionReales_period1ms();
				
				{ //zigb串口超时检测
				
					if(param_uartConZigb.tout_counter)
					   param_uartConZigb.tout_counter --;
				}
				
				{ //tips计时业务
					
					const uint8_t period_tipsLightAct = 3;
					static uint8_t counter_tipsLightAct = 0;
					
					if(counter_tipsLightAct < period_tipsLightAct)counter_tipsLightAct ++;
					else{
						
						counter_tipsLightAct = 0;
					}
				}
			}
		}
		
		{ //1s定时专用

			const u16 period_1second = 20000;
			static u16 counter_1second = 0;

			if(counter_1second < period_1second)counter_1second ++;
			else{

				counter_1second = 0;
				
				if(counterRemind_hwTimer)counterRemind_hwTimer --;
				
				if(zigbee_currentPanID_reslesCounter)zigbee_currentPanID_reslesCounter --;
				
				/*UTC时间本地保持计数*/
				sysTimeKeep_counter ++;
				
				/*tips空闲释放计时计数业务*/
				if(counter_ifTipsFree && countEN_ifTipsFree)counter_ifTipsFree --;
				
				/*smartconfig开启时间倒计时*/
				if(smartconfigOpen_flg){
				
					if(timeCounter_smartConfig_start)timeCounter_smartConfig_start --; 
					else{

						usrApp_wifiNwkCfg_stop();
					}
				}
			}
		}
	}	

	usrApp_tipsLightFunctionReales_period100us();
	usrApp_tipsBeepFunctionReales_period100us();
}

void bsp_iwdgFeed(void){

	HAL_IWDG_Refresh(&bspApp_IWDG);
}

void bsp_iwdgInit(void){

	bspApp_IWDG.Instance = IWDG;
	bspApp_IWDG.Init.Prescaler = IWDG_PRESCALER_128;
	bspApp_IWDG.Init.Reload = 625;
	HAL_IWDG_Init(&bspApp_IWDG);
	HAL_IWDG_Start(&bspApp_IWDG);
}

void bsp_hwTimerInit(void){

	drvApplication_hwTimer_initialization(50 - 1, 16 - 1);
	
	DBLOG_PRINTF("hwTim3 init.\n");
}

