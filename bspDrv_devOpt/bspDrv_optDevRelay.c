#include "bspDrv_optDevRelay.h"

#include "datsProcess_uartWifi.h"

extern u16	delayCnt_closeLoop;

static void relayActingProcess_Thread(const void *argument);

osThreadId tid_relayActingProcess_Thread;
osThreadDef(relayActingProcess_Thread, osPriorityNormal, 1, 512);

u8 status_actuatorRelay = 0;
status_ifSave relayStatus_ifSave = statusSave_disable;

relay_Command swCommand_fromUsr	= {0, actionNull};

bool devStatus_ctrlEachO_IF = false; //
u8 EACHCTRL_realesFLG = 0; //

stt_motorAttr curtainAct_Param = {
	
	.act_counter = 0, 
	.act_period = CURTAIN_ORBITAL_PERIOD_INITTIME, 
	.act = cTact_stop
};

LOCAL void 
relay_statusReales(void){

	stt_usrDats_privateSave datsSave_Temp = {0};
	
	switch(SWITCH_TYPE){

		case SWITCH_TYPE_CURTAIN:{
		
				switch(status_actuatorRelay){
				
				case 1:{
					
					PIN_RELAY_1_SET(GPIO_PIN_RESET);
					PIN_RELAY_2_SET(GPIO_PIN_SET);
					PIN_RELAY_3_SET(GPIO_PIN_RESET);
					curtainAct_Param.act = cTact_open;
				}break;
					
				case 4:{
				
					PIN_RELAY_1_SET(GPIO_PIN_SET);
					PIN_RELAY_2_SET(GPIO_PIN_RESET);
					PIN_RELAY_3_SET(GPIO_PIN_RESET);
					curtainAct_Param.act = cTact_close;		
				}break;

				case 2:
				default:{
				
					PIN_RELAY_1_SET(GPIO_PIN_RESET);
					PIN_RELAY_2_SET(GPIO_PIN_RESET);
					PIN_RELAY_3_SET(GPIO_PIN_RESET);
					if(curtainAct_Param.act != cTact_stop)curtainAct_Param.act = cTact_stop;

					datsSave_Temp.devCurtain_orbitalCounter = curtainAct_Param.act_counter; //
//					devParam_flashDataSave(obj_devCurtainOrbitalCounter, datsSave_Temp); //
					
				}break;
			}
		
		}break;
	
		case SWITCH_TYPE_SWBIT1:{ //
		
			if(DEV_actReserve & 0x02)(status_actuatorRelay & 0x01)?(PIN_RELAY_1_SET(GPIO_PIN_SET)):(PIN_RELAY_1_SET(GPIO_PIN_RESET));
			
		}break;
		
		case SWITCH_TYPE_SWBIT2:{ //
		
			if(DEV_actReserve & 0x01)(status_actuatorRelay & 0x01)?(PIN_RELAY_1_SET(GPIO_PIN_SET)):(PIN_RELAY_1_SET(GPIO_PIN_RESET));
			if(DEV_actReserve & 0x04)(status_actuatorRelay & 0x02)?(PIN_RELAY_2_SET(GPIO_PIN_SET)):(PIN_RELAY_2_SET(GPIO_PIN_RESET));
		
		}break;
		
		case SWITCH_TYPE_SWBIT3:{ //
		
			if(DEV_actReserve & 0x01)(status_actuatorRelay & 0x01)?(PIN_RELAY_1_SET(GPIO_PIN_SET)):(PIN_RELAY_1_SET(GPIO_PIN_RESET));
			if(DEV_actReserve & 0x02)(status_actuatorRelay & 0x02)?(PIN_RELAY_2_SET(GPIO_PIN_SET)):(PIN_RELAY_2_SET(GPIO_PIN_RESET));
			if(DEV_actReserve & 0x04)(status_actuatorRelay & 0x04)?(PIN_RELAY_3_SET(GPIO_PIN_SET)):(PIN_RELAY_3_SET(GPIO_PIN_RESET));
		
		}break;

		default:break;
	}

	tips_statusChangeToNormal(); //
}

/**/
LOCAL void 
actuatorRelay_Act(relay_Command dats){
	
	u8 statusTemp = 0;

	statusTemp = status_actuatorRelay; //
	
	switch(dats.actMethod){ //
	
		case relay_flip:{
			
			if(dats.objRelay & 0x01)status_actuatorRelay ^= 1 << 0;
			if(dats.objRelay & 0x02)status_actuatorRelay ^= 1 << 1;
			if(dats.objRelay & 0x04)status_actuatorRelay ^= 1 << 2;
				
		}break;
		
		case relay_OnOff:{
			
			(dats.objRelay & 0x01)?(status_actuatorRelay |= 1 << 0):(status_actuatorRelay &= ~(1 << 0));
			(dats.objRelay & 0x02)?(status_actuatorRelay |= 1 << 1):(status_actuatorRelay &= ~(1 << 1));
			(dats.objRelay & 0x04)?(status_actuatorRelay |= 1 << 2):(status_actuatorRelay &= ~(1 << 2));
			
		}break;
		
		default:break;
		
	}
	relay_statusReales(); //

	wifiFunction_callFromThread(msgFunWifi_devRootHeartbeatTrig, false); //主动上报
	
//	if(status_actuatorRelay)delayCnt_closeLoop = 0; //
	if(EACHCTRL_realesFLG)devStatus_ctrlEachO_IF = true; //
	
	if(relayStatus_ifSave == statusSave_enable){ //

		
	}
}

static void relayActingProcess_Thread(const void *argument){

	u16 log_Counter = 0;
	const u16 log_Period = 200;

	for(;;){

//		{ //???

//			if(log_Counter < log_Period)log_Counter ++;
//			else{

//				log_Counter = 0;
//				os_printf(">>>devType: %02X, act:%d, actCounter: %d, actPeriod: %d.\n", SWITCH_TYPE, curtainAct_Param.act, curtainAct_Param.act_counter, curtainAct_Param.act_period);
//			}
//		}
	
		if(swCommand_fromUsr.actMethod != actionNull){ //
		
			actuatorRelay_Act(swCommand_fromUsr);
			
			swCommand_fromUsr.actMethod = actionNull;
			swCommand_fromUsr.objRelay = 0;
		}

		osDelay(1);
	}
}

void bsp_actuatorRelay_Init(void){

	GPIO_InitTypeDef gpio_initstruct;

	__HAL_RCC_GPIOA_CLK_ENABLE();

	gpio_initstruct.Mode 	= GPIO_MODE_OUTPUT_PP;
	gpio_initstruct.Speed	= GPIO_SPEED_FREQ_HIGH;
	gpio_initstruct.Pull	= GPIO_PULLDOWN;
	gpio_initstruct.Pin		= GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
	HAL_GPIO_Init(GPIOA, &gpio_initstruct);
	
	if(relayStatus_ifSave == statusSave_enable){

		
	}
	
	tid_relayActingProcess_Thread = osThreadCreate(osThread(relayActingProcess_Thread), NULL);
}
