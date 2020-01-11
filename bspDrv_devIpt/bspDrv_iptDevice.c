#include "bspDrv_iptDevice.h"

#include "stm32f4xx_hal_gpio.h"

#include "UART_dataTransfer.h"

#include "bspDrv_optDevRelay.h"

#include "bspDrv_optDevTips.h"

#include "datsProcess_uartWifi.h"
#include "datsProcess_uartZigbee.h"

u8 val_DcodeCfm = 0;

u8 timeCounter_smartConfig_start = 0; //
bool smartconfigOpen_flg = false; //

u16	touchPadActCounter 	= 0;  //
u16	touchPadContinueCnt	= 0;  //

bool usrKeyCount_EN	= false;  //
u16	 usrKeyCount	= 0;  //

u8 touchKeepCnt_record	= 1;  //

u16  combinationFunFLG_3S5S_cancel_counter  = 0;	//
LOCAL param_combinationFunPreTrig param_combinationFunTrigger_3S1L = {0};
LOCAL param_combinationFunPreTrig param_combinationFunTrigger_3S5S = {0};

static void devDrvIpt_mainThread(const void *argument);

osThreadId tid_inputDev_process;
osThreadDef(devDrvIpt_mainThread, osPriorityNormal, 1, 512);

void usrApp_wifiNwkCfg_stop(void){

	if(smartconfigOpen_flg){

		smartconfigOpen_flg = false; //
		if(timeCounter_smartConfig_start)timeCounter_smartConfig_start = 0;
		if(status_tipsAPFind == devTips_status_get())tips_statusChangeToNormal(); //

		beeps_usrActive(3, 20, 2);
		
		DBLOG_PRINTF("smartconfig stop.\n");
	}
}

static void usrApp_wifiNwkCfg_start(void){

	wifiFunction_nwkCfgStart();
	
	tips_statusChangeToAPFind();
	
	timeCounter_smartConfig_start = SMARTCONFIG_TIMEOPEN_DEFULT;
	smartconfigOpen_flg = true;

	beeps_usrActive(3, 20, 2);
	
	DBLOG_PRINTF("smartCfg start.\n");
}

LOCAL void 
usrFunCB_pressShort(void){

	beeps_usrActive(3, 160, 3);
	tips_statusChangeToTouchReset();
	osDelay(10000); //
	HAL_NVIC_SystemReset();
}

LOCAL void 
usrFunCB_pressLongA(void){

	usrApp_wifiNwkCfg_start();
}

LOCAL void 
usrFunCB_pressLongB(void){

	beeps_usrActive(1, 254, 3);
	tips_statusChangeToFactoryRecover();
	wifiFunction_callFromThread(msgFunWifi_nwkWifiReset, true);
	osDelay(8000); //
	HAL_NVIC_SystemReset();
}

LOCAL void 
usrFunCB_touchPadTrig_3S1L(void){

	usrApp_wifiNwkCfg_start();
}

LOCAL void 
usrFunCB_touchPadTrig_3S5S(void){

	zigbFunction_callFromThread(msgFunZigb_nwkOpen);
	tips_statusChangeToZigbNwkOpen(ZIGBNWKOPENTIME_DEFAULT);
	
	DBLOG_PRINTF("zbNek open.\n");
}

LOCAL void normalBussiness_shortTouchTrig(u8 statusPad, bool shortPressCnt_IF){

	bool tipsBeep_IF = false;

	switch(statusPad){
		
		case 1:{
			
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
			
				swCommand_fromUsr.objRelay = 0;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN){
			
			#if(CURTAIN_RELAY_UPSIDE_DOWN)
				swCommand_fromUsr.objRelay = 1;
			#else
				swCommand_fromUsr.objRelay = 4;
			#endif
			}
			else{
			
				swCommand_fromUsr.objRelay = statusPad;
			}
			
			if(DEV_actReserve & 0x01)tipsBeep_IF = 1;
		
		}break;
		
		case 2:{
		
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
			
				swCommand_fromUsr.objRelay = 1;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
			
				swCommand_fromUsr.objRelay = 0;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN){
			
				swCommand_fromUsr.objRelay = 2;
			}
			else{
			
				swCommand_fromUsr.objRelay = statusPad;
			}
			
			if(DEV_actReserve & 0x02)tipsBeep_IF = 1;
		
		}break;
		
		case 4:{
	
			if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
			
				swCommand_fromUsr.objRelay = 2;
			}
			else if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN){
			
			#if(CURTAIN_RELAY_UPSIDE_DOWN)
				swCommand_fromUsr.objRelay = 4;
			#else
				swCommand_fromUsr.objRelay = 1;
			#endif
			}
			else{
			
				swCommand_fromUsr.objRelay = statusPad;
			}
			
			if(DEV_actReserve & 0x04)tipsBeep_IF = 1;
			
		}break;
			
		default:{
		
			switch(SWITCH_TYPE){ //
			
				case SWITCH_TYPE_SWBIT1:{
				
					if(statusPad & 0x02)swCommand_fromUsr.objRelay |= 0x01;
					
					if(DEV_actReserve & 0x02)tipsBeep_IF = 1;
					
				}break;
					
				case SWITCH_TYPE_SWBIT2:{
				
					if(statusPad & 0x01)swCommand_fromUsr.objRelay |= 0x01;
					if(statusPad & 0x04)swCommand_fromUsr.objRelay |= 0x02;
					
					if(DEV_actReserve & 0x05)tipsBeep_IF = 1;
					
				}break;
					
				case SWITCH_TYPE_SWBIT3:{
				
					if(statusPad & 0x01)swCommand_fromUsr.objRelay |= 0x01;
					if(statusPad & 0x02)swCommand_fromUsr.objRelay |= 0x02;
					if(statusPad & 0x04)swCommand_fromUsr.objRelay |= 0x04;
					
					if(DEV_actReserve & 0x07)tipsBeep_IF = 1;
					
				}break;
					
				default:{
				
					return; //
				
				}break;
			}
		
		}break;
	}
	
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1 || SWITCH_TYPE == SWITCH_TYPE_SWBIT2 || SWITCH_TYPE == SWITCH_TYPE_SWBIT3){
	
		swCommand_fromUsr.actMethod = relay_flip;
		
	}else{
	
		swCommand_fromUsr.actMethod = relay_OnOff;
	}

	if(!shortPressCnt_IF){ //
	
		
	}
	
	if(tipsBeep_IF)beeps_usrActive(3, 25, 1); //tips
}

LOCAL void touchPad_functionTrigNormal(u8 statusPad, keyCfrm_Type statusCfm){

	memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
	
	switch(statusCfm){
	
		case press_Short:{

//			os_printf("touchShort get:%02X.\n", statusPad);
			snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "T-s:%02X.\n", statusPad);
			
			normalBussiness_shortTouchTrig(statusPad, false); //
			
		}break;
		
		case press_ShortCnt:{

//			os_printf("touchCnt get:%02X.\n", statusPad);
			snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "T-c:%02X.\n", statusPad);

			touchKeepCnt_record ++; //
		
			if(touchKeepCnt_record == 3){
			
				param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_FLG = true; //
				param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_keyVal = statusPad; //
			
			}else{
			
				memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig));
			} 

			normalBussiness_shortTouchTrig(statusPad, true); //

		}break;
		
		case press_LongA:{

			if(param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_FLG && (statusPad == param_combinationFunTrigger_3S1L.param_combinationFunPreTrig_standBy_keyVal)){ //

				memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //

//				os_printf("combination fun<3S1L> trig!\n");
				snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "T-s[3S1L]:%02X.\n", statusPad);
				usrFunCB_touchPadTrig_3S1L();
//				usrFunCB_pressLongA();

			}else{

//				os_printf("touchLongA get:%02X.\n", statusPad);
				snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "T-l.a:%02X.\n", statusPad);
			
				switch(statusPad){
				
					case 1:
					case 2:
					case 4:{
						

					}break;
						
					default:{}break;
				}				
			}
			
		}break;
			
		case press_LongB:{

//			os_printf("touchLongB get:%02X.\n", statusPad);
			snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "T-l.b:%02X.\n", statusPad);
		
			switch(statusPad){
			
				case 1:
				case 2:
				case 4:{

//					touchFunCB_sysGetRestart();
			
				}break;
					
				default:{}break;
			}
			
		}break;
			
		default:{}break;
	}

	{ //
	
		if(statusCfm != press_ShortCnt){

			touchKeepCnt_record = 1; //
			memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //
			
			if(statusCfm != press_Short)memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //
		}
	}
	
	DbugP1TX(printfLog_bufTemp, strlen(printfLog_bufTemp));
}

LOCAL void touchPad_functionTrigContinue(u8 statusPad, u8 loopCount){

	memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));

//	os_printf("touchCnt over:%02X, %02dtime.\n", statusPad, loopCount);
	snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "T-c.d[%02dt]:%02X.\n", loopCount, statusPad);

	switch(loopCount){

		case 2:{

			switch(statusPad){
			
				case 1:{
				
					
				}break;
					
				case 2:{
				
					
				}break;
					
				case 4:{
			
					
				}break;
					
				default:{}break;
			}
			
		}break;

		case 3:{

			param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_FLG = true; //
			param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_keyVal = statusPad; //
			combinationFunFLG_3S5S_cancel_counter = 3000;  //
		
		}break;

		case 5:{

			if(param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_FLG && (statusPad == param_combinationFunTrigger_3S5S.param_combinationFunPreTrig_standBy_keyVal)){ //????????????<3?5?>
			
				memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig));

				DBLOG_PRINTF("combination fun<3S5S> trig!\n");
				snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "T-s[3S5S]:%02X.\n", statusPad);
				usrFunCB_touchPadTrig_3S5S();
			}

		}break;
		
		case 6:{}break;

		case 10:{}break;
	
		default:{}break;
	}

	{ //
	
		touchKeepCnt_record = 1; //
		memset(&param_combinationFunTrigger_3S1L, 0, sizeof(param_combinationFunPreTrig)); //
		if(loopCount != 3){ //
		
			memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //
		}
	}
	
	DbugP1TX(printfLog_bufTemp, strlen(printfLog_bufTemp));
}

LOCAL u8 
DcodeScan_oneShoot(void){
	
	u8 val_Dcode = 0;
	
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15))val_Dcode |= (1 << 0);
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)) val_Dcode |= (1 << 1);
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)) val_Dcode |= (1 << 2);
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5)) val_Dcode |= (1 << 3);
	
	return val_Dcode;
}

LOCAL bool 
UsrKEYScan_oneShoot(void){
	
	bool res = false;
	
	(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13))?
		(res = true):
		(res = false);
	
	return res;
}

LOCAL u8 
touchPadScan_oneShoot(void){

	u8 valKey_Temp = 0;
	
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8))valKey_Temp |= (1 << 0);
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9))valKey_Temp |= (1 << 1);
	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7))valKey_Temp |= (1 << 2);
	
	return valKey_Temp;
}

LOCAL void 
DcodeScan(void){

	static u8 	val_Dcode_Local 	= 0x00, //
				comfirm_Cnt			= 200;  //
	const  u8 	comfirm_Period		= 200;	//
		
		   u8 	val_Dcode_differ	= 0;
	
		   bool	val_CHG				= false;
	
	val_DcodeCfm = DcodeScan_oneShoot();
	
	DEV_actReserve = switchTypeReserve_GET(); //
	
	if(val_Dcode_Local != val_DcodeCfm){
	
		if(comfirm_Cnt < comfirm_Period)comfirm_Cnt ++;
		else{
		
			comfirm_Cnt = 0;
			val_CHG		= 1;
		}
	}
	
	if(val_CHG){
		
		val_CHG				= 0;
	
		val_Dcode_differ 	= val_Dcode_Local ^ val_DcodeCfm;
		val_Dcode_Local		= val_DcodeCfm;

		DBLOG_PRINTF("Dcode chg: %02X.\n", val_Dcode_Local);

		beeps_usrActive(3, 20, 2);
		tips_statusChangeToNormal();

		if(val_Dcode_differ & Dcode_FLG_ifAP){
		
			extern bool nwkWifi_cfg_method;
			
			if(val_Dcode_Local & Dcode_FLG_ifAP){
			
				nwkWifi_cfg_method = true;
			
			}else{
			
				nwkWifi_cfg_method = false;
			}
			
			DBLOG_PRINTF("wifi cfg method chg:%d.\n", nwkWifi_cfg_method);
		}
		
		if(val_Dcode_differ & Dcode_FLG_ifMemory){
		
			if(val_Dcode_Local & Dcode_FLG_ifMemory){

				
				
			}else{
			

			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_bitReserve){
		
			switch(Dcode_bitReserve(val_Dcode_Local)){
			
				case 0:{
				
					SWITCH_TYPE = SWITCH_TYPE_SWBIT3;	
					
				}break;
					
				case 1:{
				
					SWITCH_TYPE = SWITCH_TYPE_SWBIT1;	

				}break;
					
				case 2:{
				
					SWITCH_TYPE = SWITCH_TYPE_SWBIT2;	

				}break;
					
				case 3:{
					
					SWITCH_TYPE = SWITCH_TYPE_SWBIT3;	

				}break;
					
				default:break;
			}
		}
	}
}

LOCAL void 
UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB){
	
	const  u16 	keyCfrmLoop_Short 	= 20,		//
			   	keyCfrmLoop_LongA 	= 3000,		//
			   	keyCfrmLoop_LongB 	= 12000,	//
			   	keyCfrmLoop_MAX		= 60000;	//

	static bool LongA_FLG = 0;
	static bool LongB_FLG = 0;
	
	static bool keyPress_FLG = 0;

	if(true == UsrKEYScan_oneShoot()){		
		
		keyPress_FLG = 1;
	
		if(!usrKeyCount_EN) usrKeyCount_EN= 1;	//
		
		if((usrKeyCount >= keyCfrmLoop_LongA) && (usrKeyCount <= keyCfrmLoop_LongB) && !LongA_FLG){
		
			funCB_LongA();
			
			LongA_FLG = 1;
		}	
		
		if((usrKeyCount >= keyCfrmLoop_LongB) && (usrKeyCount <= keyCfrmLoop_MAX) && !LongB_FLG){
		
			funCB_LongB();
			
			LongB_FLG = 1;
		}
		
	}
	else{		
		
		usrKeyCount_EN = 0;
		
		if(keyPress_FLG){
		
			keyPress_FLG = 0;
			
			if(usrKeyCount < keyCfrmLoop_LongA && usrKeyCount > keyCfrmLoop_Short){
				
				funCB_Short();
			}
			
			usrKeyCount = 0;
			LongA_FLG 	= 0;
			LongB_FLG 	= 0;
		}
	}
}

LOCAL void 
touchPad_Scan(void){

	static u8   touchPad_temp = 0;
	static bool keyPress_FLG = 0;

	static bool	funTrigFLG_LongA = 0;
	static bool funTrigFLG_LongB = 0;

	const  u16 	touchCfrmLoop_Short 	= 20,		//
			   	touchCfrmLoop_LongA 	= 3000,		//
			   	touchCfrmLoop_LongB 	= 10000,	//
			   	touchCfrmLoop_MAX		= 60000;	//

	const  u16  timeDef_touchPressContinue = 400;

	static u8 	pressContinueGet = 0;
		   u8 	pressContinueCfm = 0;

	u16 conterTemp = 0; //

	if(!combinationFunFLG_3S5S_cancel_counter)memset(&param_combinationFunTrigger_3S5S, 0, sizeof(param_combinationFunPreTrig)); //

	if(touchPadScan_oneShoot()){
		
		if(!keyPress_FLG){
		
			keyPress_FLG = true;
			touchPadActCounter 	= touchCfrmLoop_MAX;
			touchPadContinueCnt = timeDef_touchPressContinue;  //
			touchPad_temp = touchPadScan_oneShoot();
		}
		else{
			
			if(touchPad_temp == touchPadScan_oneShoot()){
				
				conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
			
				if(conterTemp > touchCfrmLoop_LongA && conterTemp <= touchCfrmLoop_LongB){
				
					if(false == funTrigFLG_LongA){
					
						funTrigFLG_LongA = true;
						touchPad_functionTrigNormal(touchPad_temp, press_LongA);
					}
				}
				if(conterTemp > touchCfrmLoop_LongB && conterTemp <= touchCfrmLoop_MAX){
				
					if(false == funTrigFLG_LongB){
					
						funTrigFLG_LongB = true;
						touchPad_functionTrigNormal(touchPad_temp, press_LongB);
					}
				}
			}
			else{

				if((touchCfrmLoop_MAX - touchPadActCounter) < touchCfrmLoop_Short){ //
				
					touchPadActCounter = touchCfrmLoop_MAX;
					touchPadContinueCnt = timeDef_touchPressContinue;  //
					touchPad_temp = touchPadScan_oneShoot();
				}
			}
		}
	}
	else{
		
		if(true == keyPress_FLG){
		
			conterTemp = touchCfrmLoop_MAX - touchPadActCounter;
			if(conterTemp > touchCfrmLoop_Short && conterTemp <= touchCfrmLoop_LongA){
			
				if(touchPadContinueCnt)pressContinueGet ++;
				
				if(pressContinueGet <= 1)touchPad_functionTrigNormal(touchPad_temp, press_Short); //
				else touchPad_functionTrigNormal(touchPad_temp, press_ShortCnt); //
			}
		}
	
		if(!touchPadContinueCnt && pressContinueGet){
		
			pressContinueCfm = pressContinueGet;
			pressContinueGet = 0;
			
			if(pressContinueCfm >= 2){
			
				touchPad_functionTrigContinue(touchPad_temp, pressContinueCfm); //
				pressContinueCfm = 0;
			}

			touchPad_temp = 0;
		}

		funTrigFLG_LongA = 0;
		funTrigFLG_LongB = 0;
			
		touchPadActCounter = 0;
		keyPress_FLG = 0;
	}
}

void usrApp_iptFunctionReales_period1ms(void){

	//触摸按键计时逻辑业务
	if(touchPadActCounter)touchPadActCounter --; //
	if(touchPadContinueCnt)touchPadContinueCnt --; //
	
	//轻触按键计时逻辑业务
	if(usrKeyCount_EN)usrKeyCount ++;
	else usrKeyCount = 0;
}

static void threadProcessIptDrv_initialization(void){

	GPIO_InitTypeDef gpio_initstruct;

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	gpio_initstruct.Mode 	= GPIO_MODE_INPUT;
	gpio_initstruct.Speed	= GPIO_SPEED_FREQ_HIGH;
	gpio_initstruct.Pull	= GPIO_PULLUP;
	gpio_initstruct.Pin		= GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_13 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOB, &gpio_initstruct);
	gpio_initstruct.Pin		= GPIO_PIN_8;
	HAL_GPIO_Init(GPIOA, &gpio_initstruct);
}

static void devDrvIpt_mainThread(const void *argument){

	for(;;){
	
		touchPad_Scan();
		UsrKEYScan(usrFunCB_pressShort, usrFunCB_pressLongA, usrFunCB_pressLongB);
		DcodeScan();
	}
}
	
void devDrvIpt_threadProcessActive(void){

	threadProcessIptDrv_initialization();
	
	tid_inputDev_process = osThreadCreate(osThread(devDrvIpt_mainThread), NULL);
	
	DBLOG_PRINTF("ipt kb init.\n");
}


