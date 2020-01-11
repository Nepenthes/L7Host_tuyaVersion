#include "bspDrv_optDevTips.h"

#include "timer_Activing.h"
#include "bspDrv_optDevRelay.h"
#include "bspDrv_iptDevice.h"

extern bool nwkZigbOnline_IF;
extern bool nwkInternetOnline_IF;

static void usrTipsProcess_Thread(const void *argument);

osThreadId tid_usrTipsProcess_Thread;
osThreadDef(usrTipsProcess_Thread, osPriorityNormal, 1, 512);

u16 counter_tipsAct = 0;

const func_ledRGB color_Tab[TIPS_SWBKCOLOR_TYPENUM] = {

	{ 0,  0,  0}, {20, 10, 31}, {31,  0,  0},
	{31,  0, 10}, {8,   0, 16}, {0,  31,  0},
	{16, 31,  0}, {31, 10,  0}, {0,   0, 31},
	{ 0, 10, 31},
};

func_ledRGB relay1_Tips 	= {0};
func_ledRGB relay2_Tips 	= {0};
func_ledRGB relay3_Tips 	= {0};
func_ledRGB nwk_Tips 		= {0};

const func_ledRGB tips_relayUnused = {0, 0, 0};

u8 	 counter_ifTipsFree = TIPS_SWFREELOOP_TIME; //
bool countEN_ifTipsFree	= true;

sound_Attr devTips_beep    = {0, 0, 0}; //
enum_beeps dev_statusBeeps = beepsMode_null; 

static bkLightColorInsert_paramAttr devBackgroundLight_param = {

	.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open = 8,
	.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close = 5,
}; 

u8 timeCount_zigNwkOpen = 0; //

static bool ifHorsingLight_running_FLAG = true; //

static tips_Status devTips_status 		   = status_Normal; //
static tips_devNwkStatus devNwkTips_status = devNwkStaute_nwkAllAbnormal; //

LOCAL void tips_sysButtonReales(void);
LOCAL void tips_breath(void);
LOCAL void tips_specified(u8 tips_Type);
LOCAL void tips_sysTouchReset(void);
LOCAL void tips_sysStandBy(void);
LOCAL void thread_tipsGetDark(u8 funSet);

/**/
void 
tips_statusChangeToNormal(void){

	counter_ifTipsFree = TIPS_SWFREELOOP_TIME;
	devTips_status = status_Normal;
}

/**/
void 
tips_statusChangeToAPFind(void){

	devTips_status = status_tipsAPFind;
	devNwkTips_status = devNwkStaute_wifiNwkFind;
	thread_tipsGetDark(0x0F);
}

/**/
void 
tips_statusChangeToZigbNwkOpen(u8 timeopen){

	timeCount_zigNwkOpen = timeopen;
	devTips_status = status_tipsNwkOpen;
	devNwkTips_status = devNwkStaute_zigbNwkOpen;
	thread_tipsGetDark(0x0F);
}

/**/
void 
tips_statusChangeToTouchReset(void){

	devTips_status = status_touchReset;
	thread_tipsGetDark(0x0F);
}

/**/
void 
tips_statusChangeToFactoryRecover(void){

	devTips_status = status_sysStandBy;
	thread_tipsGetDark(0x0F);
}

/**/
void beeps_usrActive(u8 tons, u8 time, u8 loop){

	if(!ifNightMode_sw_running_FLAG){ //

		devTips_beep.tips_Period = tons;
		devTips_beep.tips_time = time;
		devTips_beep.tips_loop = loop;
		dev_statusBeeps = beepsMode_standBy;
	}
}

tips_Status devTips_status_get(void){

	return devTips_status;
}

void tipsLED_colorSet(tipsLED_Obj obj, u8 gray_R, u8 gray_G, u8 gray_B){

	if((devTips_status == status_Normal) ||
	   (devTips_status == status_Night)){
		   
		if(devTips_status == status_Night){	//夜间模式，背光亮度调整
			
			const uint8_t bright_coef = 4;
		
			gray_R /= bright_coef;
			gray_G /= bright_coef;
			gray_B /= bright_coef;
		}
	
		switch(obj){
		
			case obj_Relay1:{
			
				relay1_Tips.color_R = gray_R;
				relay1_Tips.color_G = gray_G;
				relay1_Tips.color_B = gray_B;
				
			}break;
			
			case obj_Relay2:{
			
				relay2_Tips.color_R = gray_R;
				relay2_Tips.color_G = gray_G;
				relay2_Tips.color_B = gray_B;
				
			}break;
			
			case obj_Relay3:{
			
				relay3_Tips.color_R = gray_R;
				relay3_Tips.color_G = gray_G;
				relay3_Tips.color_B = gray_B;
				
			}break;
					
			case obj_nwk:{
			
				nwk_Tips.color_R = gray_R;
				nwk_Tips.color_G = gray_G;
				nwk_Tips.color_B = gray_B;
				
			}break;
			
			default:break;
		}
	}
}

LOCAL void 
thread_tipsGetDark(u8 funSet){ //

	relay1_Tips.color_R = relay2_Tips.color_R = relay3_Tips.color_R = nwk_Tips.color_R = 0;
	relay1_Tips.color_G = relay2_Tips.color_G = relay3_Tips.color_G = nwk_Tips.color_G = 0;
	relay1_Tips.color_B = relay2_Tips.color_B = relay3_Tips.color_B = nwk_Tips.color_B = 0;
}

LOCAL void 
devNwkStatusTips_refresh(void){

	u8 nwkStatus = 0;

	if(nwkZigbOnline_IF)nwkStatus |= 0x01;	//
	if(nwkInternetOnline_IF)nwkStatus |= 0x02; //

	if( (devNwkTips_status != devNwkStaute_zigbNwkOpen) &&
		(devNwkTips_status != devNwkStaute_wifiNwkFind) ){

		switch(nwkStatus){

			case 0:{

				devNwkTips_status = devNwkStaute_nwkAllAbnormal;

			}break;

			case 1:{

				devNwkTips_status = devNwkStaute_zigbNwkNormalOnly;

			}break;

			case 2:{

				devNwkTips_status = devNwkStaute_wifiNwkNormalOnly;

			}break;

			case 3:{

				devNwkTips_status = devNwkStaute_nwkAllNormal;

			}break;
		}		
	}
}

void devTipsParamSet_horsingLight(bool param){

	ifHorsingLight_running_FLAG = param;
}

bool devTipsParamGet_horsingLight(void){

	return ifHorsingLight_running_FLAG;
}

void devTipsParamSet_keyLightColor(bkLightColorInsert_paramAttr *param){

	memcpy(&devBackgroundLight_param, param, sizeof(bkLightColorInsert_paramAttr));
}

void devTipsParamGet_keyLightColor(bkLightColorInsert_paramAttr *param){

	memcpy(param, &devBackgroundLight_param, sizeof(bkLightColorInsert_paramAttr));
}

static void usrTipsProcess_Thread(const void *argument){

	for(;;){

		if(ifNightMode_sw_running_FLAG){ //
		
			if(devTips_status == status_Normal || //
			   devTips_status == status_keyFree){

				devTips_status = status_Night;
			}
			
		}else{ //
			
			if(devTips_status == status_Night)tips_statusChangeToNormal(); //
		
			if(!counter_ifTipsFree &&  //
			   !timeCounter_smartConfig_start &&
			   !timeCount_zigNwkOpen &&
			   (devTips_status == status_Normal) && //
			   ifHorsingLight_running_FLAG ){ //
			    
				thread_tipsGetDark(0x0F);
				devTips_status = status_keyFree;
			}
		}

		switch(devTips_status){

			case status_sysStandBy:{

				tips_sysStandBy();

			}break;

			case status_keyFree:{

				tips_sysButtonReales();
//				tips_breath();
			
			}break;

			case status_Night:
			case status_Normal:{

				u8 relayStatus_tipsTemp = 0;

				if(!timeCount_zigNwkOpen && !timeCounter_smartConfig_start)devNwkTips_status = devNwkStaute_nwkAllNormal;
				
				if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
				
					relayStatus_tipsTemp |= status_actuatorRelay & 0x01; //
					relayStatus_tipsTemp = relayStatus_tipsTemp << 1; //
	
				}else
				if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){

					relayStatus_tipsTemp |= status_actuatorRelay & 0x02; //
					relayStatus_tipsTemp = relayStatus_tipsTemp << 1; //
					relayStatus_tipsTemp |= status_actuatorRelay & 0x01; //

				}else
				if(SWITCH_TYPE == SWITCH_TYPE_SWBIT3)
				{

					relayStatus_tipsTemp = status_actuatorRelay; //
					
				}else
				if(SWITCH_TYPE == SWITCH_TYPE_CURTAIN)
				{

					relayStatus_tipsTemp = status_actuatorRelay; //
				}

				/**/
				switch(SWITCH_TYPE){

					case SWITCH_TYPE_CURTAIN:{

						switch(relayStatus_tipsTemp){ //

#if(CURTAIN_RELAY_UPSIDE_DOWN)
							case 0x04:{
#else
							case 0x01:{
#endif

								(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_B));
								(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_B));
								(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_B));
							}break;

#if(CURTAIN_RELAY_UPSIDE_DOWN)
								case 0x01:{
#else
								case 0x04:{
#endif
							
								(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_B));
								(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_B));
								(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_B));
							}break;

							default:{

								(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_B));
								(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press].color_B));
								(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce].color_B));

							}break;
						}

					}break;

					default:{

						(DEV_actReserve & 0x01)?\
							((relayStatus_tipsTemp & 0x01)?\
								(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_B)):\
								(tipsLED_colorSet(obj_Relay3, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_B))):\
							(tipsLED_colorSet(obj_Relay3, tips_relayUnused.color_R, tips_relayUnused.color_G, tips_relayUnused.color_B)); 			
						(DEV_actReserve & 0x02)?\
							((relayStatus_tipsTemp & 0x02)?\
								(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_B)):\
								(tipsLED_colorSet(obj_Relay2, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_B))):\
							(tipsLED_colorSet(obj_Relay2, tips_relayUnused.color_R, tips_relayUnused.color_G, tips_relayUnused.color_B)); 			
						(DEV_actReserve & 0x04)?\
							((relayStatus_tipsTemp & 0x04)?\
								(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_open].color_B)):\
								(tipsLED_colorSet(obj_Relay1, color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_R, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_G, 
															  color_Tab[devBackgroundLight_param.sw3bitIcurtain_BKlight_Param.sw3bit_BKlight_Param.sw3bit_BKlightColorInsert_close].color_B))):\
							(tipsLED_colorSet(obj_Relay1, tips_relayUnused.color_R, tips_relayUnused.color_G, tips_relayUnused.color_B));

					}break;
				}

				/**/
				{
					static u16 tips_Counter = 0;
					const u16 tips_Period = 40;

					static bool tips_Type = 0;

					devNwkStatusTips_refresh(); //
					
					if(tips_Counter < tips_Period)tips_Counter ++;
					else{

						tips_Type = !tips_Type;
						tips_Counter = 0;

						switch(devNwkTips_status){
						
							case devNwkStaute_nwkAllNormal:{ //
						
								(tips_Type)?(tipsLED_colorSet(obj_nwk, 0, 31, 0)):(tipsLED_colorSet(obj_nwk, 0, 31, 0));
							
							}break;
						
							case devNwkStaute_zigbNwkNormalOnly:{ //

								(tips_Type)?(tipsLED_colorSet(obj_nwk, 0, 0, 31)):(tipsLED_colorSet(obj_nwk, 0, 0, 31));
						
							}break;
						
							case devNwkStaute_wifiNwkNormalOnly:{ //

								(tips_Type)?(tipsLED_colorSet(obj_nwk, 15, 10, 31)):(tipsLED_colorSet(obj_nwk, 15, 10, 31));
						
							}break;
						
							case devNwkStaute_nwkAllAbnormal:{ //

								(tips_Type)?(tipsLED_colorSet(obj_nwk, 31, 0, 0)):(tipsLED_colorSet(obj_nwk, 31, 0, 0));
						
							}break;
						
							case devNwkStaute_zigbNwkOpen:{ //

								(tips_Type)?(tipsLED_colorSet(obj_nwk, 0, 31, 0)):(tipsLED_colorSet(obj_nwk, 0, 0, 0));
						
							}break;
							
							case devNwkStaute_wifiNwkFind:{ //

								(tips_Type)?(tipsLED_colorSet(obj_nwk, 15, 10, 31)):(tipsLED_colorSet(obj_nwk, 0, 0, 0));
						
							}break;
						
							default:break;
						}
					}
				}
			}break;

			case status_tipsNwkOpen:{

				if(timeCount_zigNwkOpen)tips_specified(0);
				else{

					tips_statusChangeToNormal();
				}

			}break;

			case status_tipsAPFind:{

				tips_specified(1);

			}break;

			case status_touchReset:{

				tips_sysTouchReset();

			}break;

			default:break;
		}

		osDelay(1);
	}
}

LOCAL void 
tips_breath(void){

	static u8 	localTips_Count = RLY_TIPS_PERIODUNITS - 1; //
	static bool count_FLG = 1;
	static u8 	tipsStep = 0;

	const u8 speed = 3;
	const u8 step_period = 3;
	
//	if(!localTips_Count && !count_FLG)(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++); //
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count >= (RLY_TIPS_PERIODUNITS - 1)){
		
		count_FLG = 0;
		
		localTips_Count = RLY_TIPS_PERIODUNITS - 2; //
		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = speed * (localTips_Count --)):(counter_tipsAct = speed * (localTips_Count ++)); //
	}	
	
	switch(tipsStep){
	
		case 0:{
		
			relay1_Tips.color_R = 31 - localTips_Count;
			
		}break;
		
		case 2:{
		
			relay3_Tips.color_R = 31 - localTips_Count;
			
		}break;
		
		default:break;
	}
}

LOCAL void 
tips_fadeOut(void){

	static u8 	localTips_Count = 0; //
	static bool count_FLG = 1;
	static u8 	tipsStep = 0;
	static u8 	pwmType_A = 0,
				pwmType_B = 0,
				pwmType_C = 0,
				pwmType_D = 0;
	
	const u8 speed = 1;
	const u8 step_period = 1;
	
	if(!localTips_Count && !count_FLG)(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++); //
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count > (RLY_TIPS_PERIODUNITS * 4)){
	
		count_FLG = 0;
		
//		localTips_Count = RLY_TIPS_PERIODUNITS - 2; //
//		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = speed * ((localTips_Count --) % RLY_TIPS_PERIODUNITS)):(counter_tipsAct = speed * ((localTips_Count ++) % RLY_TIPS_PERIODUNITS));  //
		
		if(localTips_Count 		 && localTips_Count< 32)pwmType_A = localTips_Count - 0;
		if(localTips_Count >= 32 && localTips_Count< 64)pwmType_B = localTips_Count - 32;
		if(localTips_Count >= 64 && localTips_Count< 96)pwmType_C = localTips_Count - 64;
		if(localTips_Count >= 96 && localTips_Count< 128)pwmType_D = localTips_Count - 96;
	}
	
	switch(tipsStep){
	
		case 0:{
			
			if(count_FLG){ 
				
				nwk_Tips.color_B = pwmType_A;
				relay1_Tips.color_B = pwmType_B;
				relay2_Tips.color_B = pwmType_C;
				relay3_Tips.color_B = pwmType_D;
				
				nwk_Tips.color_R = 31 - pwmType_A;
				relay1_Tips.color_R = 31 - pwmType_B;
				relay2_Tips.color_R = 31 - pwmType_C;
				relay3_Tips.color_R = 31 - pwmType_D;
				
			}else{ 
				
				nwk_Tips.color_R = 31 - pwmType_D;
				relay1_Tips.color_R = 31 - pwmType_C;
				relay2_Tips.color_R = 31 - pwmType_B;
				relay3_Tips.color_R = 31 - pwmType_A;
				
				nwk_Tips.color_B = pwmType_D;
				relay1_Tips.color_B = pwmType_C;
				relay2_Tips.color_B = pwmType_B;
				relay3_Tips.color_B = pwmType_A;
			}
			
		}break;
			
		case 1:{
			
			if(count_FLG){ 
				
				nwk_Tips.color_G = pwmType_A / 4;
				relay1_Tips.color_G = pwmType_B / 4;
				relay2_Tips.color_G = pwmType_C / 4;
				relay3_Tips.color_G = pwmType_D / 4;
				
				nwk_Tips.color_R = 31 - pwmType_A;
				relay1_Tips.color_R = 31 - pwmType_B;
				relay2_Tips.color_R = 31 - pwmType_C;
				relay3_Tips.color_R = 31 - pwmType_D;
				
			}else{ 
				
				nwk_Tips.color_R = 31 - pwmType_D;
				relay1_Tips.color_R = 31 - pwmType_C;
				relay2_Tips.color_R = 31 - pwmType_B;
				relay3_Tips.color_R = 31 - pwmType_A;
				
				nwk_Tips.color_G = pwmType_D / 4;
				relay1_Tips.color_G = pwmType_C / 4;
				relay2_Tips.color_G = pwmType_B / 4;
				relay3_Tips.color_G = pwmType_A / 4;
			}
			
		}break;
			
		default:break;
	}
}

void tips_sysTouchReset(void){
	
	static bool tipsStep = 0;

	if(counter_tipsAct){
	
		if(tipsStep){
		
			nwk_Tips.color_R = 0;
			relay1_Tips.color_R = 0;
			relay2_Tips.color_R = 0;
			relay3_Tips.color_R = 0;

		}else{
		
			nwk_Tips.color_R = 31;
			relay1_Tips.color_R = 31;
			relay2_Tips.color_R = 31;
			relay3_Tips.color_R = 31;
		}
		
	}else{
	
		counter_tipsAct = 400;
		tipsStep = !tipsStep;
	}
}

void tips_sysStandBy(void){

	nwk_Tips.color_R = 31;
	relay1_Tips.color_R = 31;
	relay2_Tips.color_R = 31;
	relay3_Tips.color_R = 31;
}


LOCAL void 
tips_sysButtonReales(void){

//	u8 code timUnit_period = 0;
//	static u8 timUnit_count = 0;
	
	static u8 localTips_Count = 0;
	const  u8 localTips_Period = 128;
	static u8 tipsStep = 0;
	static u8 pwmType_A = 0,
		      pwmType_B = 0,
			  pwmType_C = 0,
			  pwmType_D = 0;
	
	if(!counter_tipsAct){
		
		counter_tipsAct = 10; //
	
		if(localTips_Count > localTips_Period){
		
			localTips_Count = 0;
			(tipsStep > 7)?(tipsStep = 0):(tipsStep ++);
			pwmType_A = pwmType_B = pwmType_C = pwmType_D = 0;
		}
		else{
		
			localTips_Count ++;
			
			if(localTips_Count 		 && localTips_Count< 32)pwmType_A = localTips_Count - 0;
			if(localTips_Count >= 32 && localTips_Count< 64)pwmType_B = localTips_Count - 32;
			if(localTips_Count >= 64 && localTips_Count< 96)pwmType_C = localTips_Count - 64;
			if(localTips_Count >= 96 && localTips_Count< 128)pwmType_D = localTips_Count - 96;
		}
	}
	
	switch(tipsStep){
	
		case 0:{ /*??*///
			
			nwk_Tips.color_G = pwmType_A / 5;
			relay1_Tips.color_G = pwmType_B / 5;
			relay2_Tips.color_G = pwmType_C / 5;
			relay3_Tips.color_G = pwmType_D / 5;
			
		}break;
		
		case 1:{ /*??*///
			
			nwk_Tips.color_R = pwmType_A;
			relay1_Tips.color_R = pwmType_B;
			relay2_Tips.color_R = pwmType_C;
			relay3_Tips.color_R = pwmType_D;
			
		}break;
		
		case 2:{ /*??*///
			
			nwk_Tips.color_G = 6 - pwmType_A / 5;
			relay1_Tips.color_G = 6 - pwmType_B / 5;
			relay2_Tips.color_G = 6 - pwmType_C / 5;
			relay3_Tips.color_G = 6 - pwmType_D / 5;
			
		}break;
		
		case 3:{ /*??*///
			
			nwk_Tips.color_B = pwmType_A / 2;
			relay1_Tips.color_B = pwmType_B / 2;
			relay2_Tips.color_B = pwmType_C / 2;
			relay3_Tips.color_B = pwmType_D / 2;
			
		}break;
		
		case 4:{ /*??*///
		
			nwk_Tips.color_R = 31 - pwmType_A;
			relay1_Tips.color_R = 31 - pwmType_B;
			relay2_Tips.color_R = 31 - pwmType_C;
			relay3_Tips.color_R = 31 - pwmType_D;
			
		}break;
		
		case 5:{ /*??*///
		
			nwk_Tips.color_G = pwmType_A / 3;
			relay1_Tips.color_G = pwmType_B / 3;
			relay2_Tips.color_G = pwmType_C / 3;
			relay3_Tips.color_G = pwmType_D / 3;
			
			nwk_Tips.color_R = pwmType_A / 2;
			relay1_Tips.color_R = pwmType_B / 2;
			relay2_Tips.color_R = pwmType_C / 2;
			relay3_Tips.color_R = pwmType_D / 2;
			
		}break;
		
		case 6:{ /*dark*///
		
			nwk_Tips.color_B = 15 - pwmType_A / 2;
			relay1_Tips.color_B = 15 - pwmType_B / 2;
			relay2_Tips.color_B = 15 - pwmType_C / 2;
			relay3_Tips.color_B = 15 - pwmType_D / 2;
			
			nwk_Tips.color_G = 10 - pwmType_A / 3;
			relay1_Tips.color_G = 10 - pwmType_B / 3;
			relay2_Tips.color_G = 10 - pwmType_C / 3;
			relay3_Tips.color_G = 10 - pwmType_D / 3;
			
			nwk_Tips.color_R = 15 - pwmType_A / 2;
			relay1_Tips.color_R = 15 - pwmType_B / 2;
			relay2_Tips.color_R = 15 - pwmType_C / 2;
			relay3_Tips.color_R = 15 - pwmType_D / 2;
			
		}break;
		
		default:{}break;
	}
}

LOCAL void 
tips_specified(u8 tips_Type){ //

	static u8 	localTips_Count = 0; //
	static bool count_FLG = 1;
	static u8 	tipsStep = 0;
	static u8 	pwmType_A = 0,
				pwmType_B = 0,
				pwmType_C = 0,
				pwmType_D = 0;
	
	const u8 speed_Mol = 5,
	   		 speed_Den = 2;
	const u8 step_period = 0;
	
	if(!localTips_Count && !count_FLG){
	
		if(tipsStep < step_period)tipsStep ++; //
		else{
		
			tipsStep = 0;
			
			pwmType_A = pwmType_B = pwmType_C = pwmType_D = 0;
		}
	}
	if(!localTips_Count)count_FLG = 1;
	else 
	if(localTips_Count > 80){
	
		count_FLG = 0;
		
//		localTips_Count = COLORGRAY_MAX - 2; //
//		(tipsStep >= step_period)?(tipsStep = 0):(tipsStep ++);  //
	}
	
	if(!counter_tipsAct){
	
		(!count_FLG)?(counter_tipsAct = ((localTips_Count --) % RLY_TIPS_PERIODUNITS) / speed_Mol * speed_Den):(counter_tipsAct = ((localTips_Count ++) % RLY_TIPS_PERIODUNITS) / speed_Mol * speed_Den); //
		
		if(localTips_Count 	 	 && localTips_Count < 32)pwmType_A = localTips_Count - 0; 
		if(localTips_Count >= 16 && localTips_Count < 48)pwmType_B = localTips_Count - 16;
		if(localTips_Count >= 32 && localTips_Count < 64)pwmType_C = localTips_Count - 32;
		if(localTips_Count >= 48 && localTips_Count < 80)pwmType_D = localTips_Count - 48;
	}
	
	switch(tips_Type){
	
		case 0:{
		
			switch(tipsStep){
			
				case 0:{
					
					if(count_FLG){ 
						
						if(localTips_Count < 46){
						
							if(localTips_Count % 15 > 10)nwk_Tips.color_G = 31;
							else nwk_Tips.color_G = 0;
						
						}else{
						
							if(localTips_Count > 45)relay3_Tips.color_G = 31;
							if(localTips_Count > 50)relay2_Tips.color_G = 31;
							if(localTips_Count > 60)relay1_Tips.color_G = 31;
							if(localTips_Count > 70)nwk_Tips.color_G = 31;
						}
						
					}else{ 
						
						relay3_Tips.color_G = pwmType_D;
						relay2_Tips.color_G = pwmType_C;
						relay1_Tips.color_G = pwmType_B;
						nwk_Tips.color_G = pwmType_A;
					}
					
				}break;
					
				default:break;
			}
			
		}break;
		
		case 1:{
		
			switch(tipsStep){
			
				case 0:{
					
					if(count_FLG){ 
						
						if(localTips_Count < 46){
						
							if(localTips_Count % 15 > 10){

								nwk_Tips.color_R = 15;
								nwk_Tips.color_G = 10;
								nwk_Tips.color_B = 15;
							}
							else{
								
								nwk_Tips.color_R = 0;
								nwk_Tips.color_G = 0;
								nwk_Tips.color_B = 0;						
							}
						
						}else{
						
							if(localTips_Count > 45){
								
								nwk_Tips.color_R = 15;
								nwk_Tips.color_G = 10;
								nwk_Tips.color_B = 15;
							}
							if(localTips_Count > 50){
								
								relay1_Tips.color_R = 15;
								relay1_Tips.color_G = 10;
								relay1_Tips.color_B = 15;
							}
							if(localTips_Count > 60){
								
								relay2_Tips.color_R = 15;
								relay2_Tips.color_G = 10;
								relay2_Tips.color_B = 15;
							}
							if(localTips_Count > 70){
								
								relay3_Tips.color_R = 15;
								relay3_Tips.color_G = 10;
								relay3_Tips.color_B = 15;
							}
						}
						
					}
					else{ 
						
						nwk_Tips.color_R = pwmType_D / 2;
						nwk_Tips.color_G = pwmType_D / 3;
						nwk_Tips.color_B = pwmType_D / 2;

						relay3_Tips.color_R = pwmType_A / 2;
						relay3_Tips.color_G = pwmType_A / 3;
						relay3_Tips.color_B = pwmType_A / 2;

						relay2_Tips.color_R = pwmType_B / 2;
						relay2_Tips.color_G = pwmType_B / 3;
						relay2_Tips.color_B = pwmType_B / 2;

						relay1_Tips.color_R = pwmType_C / 2;
						relay1_Tips.color_G = pwmType_C / 3;
						relay1_Tips.color_B = pwmType_C / 2;
					}
					
				}break;
					
				default:break;
			}
			
		}break;
		
		default:break;
	}
}

void usrApp_tipsActionFunctionReales_period1ms(void){

	//tips动作周期定时业务
	if(counter_tipsAct)counter_tipsAct --;
	if(combinationFunFLG_3S5S_cancel_counter)combinationFunFLG_3S5S_cancel_counter --;
}

void usrApp_tipsBeepFunctionReales_period100us(void){
		
	static u8 period_beep = 10;	//beep专用
	static u8 count_beep = 0;
	
	if(count_beep < period_beep)count_beep ++;
	else{
		
		static u16  tips_Period = 20 * 50 / 2;
		static u16  tips_Count 	= 0;
		static u8  	tips_Loop 	= 2 * 4;
		static bool beeps_en 	= true;
	
		count_beep = 0;

		switch(dev_statusBeeps){ //状态机
			
			case beepsMode_standBy:{
				
				period_beep = devTips_beep.tips_Period;
				tips_Period = 20 * devTips_beep.tips_time / period_beep;
				tips_Loop 	= 2 * devTips_beep.tips_loop;
				tips_Count 	= 0;
				beeps_en 	= 1;
				dev_statusBeeps = beepsWorking;
	
			}break;
			
			case beepsWorking:{
			
				if(tips_Loop){
				
					if(tips_Count < tips_Period){
						
						static bool flipLevel = false;
						
						flipLevel = !flipLevel;
					
						tips_Count ++;
						(beeps_en)?(PIN_TIPS_BEEP_PIN_SET((GPIO_PinState)flipLevel)):(PIN_TIPS_BEEP_PIN_SET(!BEEP_OPEN_LEVEL));
						
					}else{
					
						tips_Count = 0;
						beeps_en = !beeps_en;
						tips_Loop --;
					}
					
				}else{
				
					dev_statusBeeps = beepsComplete;
				}
			
			}break;
			
			case beepsComplete:{
			
				tips_Count = 0;
				beeps_en = 1;
				PIN_TIPS_BEEP_PIN_SET(!BEEP_OPEN_LEVEL);
				dev_statusBeeps = beepsMode_null;
				
			}break;
		
			default:{
			
				PIN_TIPS_BEEP_PIN_SET(!BEEP_OPEN_LEVEL);
				
			}break;
		}
	}
}

void usrApp_tipsLightFunctionReales_period100us(void){ //tips_Led 刷新业务
		
	u8 loop = 0;
	
	GPIO_PinState PinState_tempRly1 = GPIO_PIN_RESET,
				  PinState_tempRly2 = GPIO_PIN_RESET,
				  PinState_tempRly3 = GPIO_PIN_RESET,
				  PinState_tempNwk  = GPIO_PIN_RESET;			 
	
	const  u8 rlyTips_reales_Period = TIPS_BASECOLOR_NUM * RLY_TIPS_PERIODUNITS;
	static u8 P_rlyTips_reales = TIPS_BASECOLOR_NUM * RLY_TIPS_PERIODUNITS + 1;
	static u8 period_rgbCnt[RLY_TIPS_NUM][TIPS_BASECOLOR_NUM] = {0};
	
	PIN_TIPS_PPP_R_SET(GPIO_PIN_SET);
	PIN_TIPS_PPP_G_SET(GPIO_PIN_SET);
	PIN_TIPS_PPP_B_SET(GPIO_PIN_SET);
	
	if(P_rlyTips_reales > rlyTips_reales_Period){	//

		P_rlyTips_reales = 0;
		
		(relay1_Tips.color_R > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[0][0] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[0][0] = relay1_Tips.color_R);
		(relay1_Tips.color_G > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[0][1] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[0][1] = relay1_Tips.color_G);
		(relay1_Tips.color_B > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[0][2] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[0][2] = relay1_Tips.color_B);

		
		(relay2_Tips.color_R > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[1][0] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[1][0] = relay2_Tips.color_R);
		(relay2_Tips.color_G > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[1][1] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[1][1] = relay2_Tips.color_G);
		(relay2_Tips.color_B > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[1][2] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[1][2] = relay2_Tips.color_B);
		
		(relay3_Tips.color_R > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[2][0] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[2][0] = relay3_Tips.color_R);
		(relay3_Tips.color_G > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[2][1] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[2][1] = relay3_Tips.color_G);
		(relay3_Tips.color_B > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[2][2] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[2][2] = relay3_Tips.color_B);
			
		(nwk_Tips.color_R > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[3][0] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[3][0] = nwk_Tips.color_R);
		(nwk_Tips.color_G > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[3][1] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[3][1] = nwk_Tips.color_G);
		(nwk_Tips.color_B > RLY_TIPS_PERIODUNITS)?
			(period_rgbCnt[3][2] = RLY_TIPS_PERIODUNITS):(period_rgbCnt[3][2] = nwk_Tips.color_B);
	}
	else
	{
		P_rlyTips_reales ++;
	}

	if(P_rlyTips_reales && P_rlyTips_reales < (RLY_TIPS_PERIODUNITS * 1)){ //

		PIN_TIPS_PPP_R_SET(GPIO_PIN_RESET);
		for(loop = 0; loop < RLY_TIPS_NUM; loop ++){

			if(period_rgbCnt[loop][0]){

				switch(loop){

					case 0:{ //
						
						period_rgbCnt[loop][0] --;
						if(DEV_actReserve & 0x04)PinState_tempRly1 = GPIO_PIN_SET; //

					}break;

					case 1:{ //

						period_rgbCnt[loop][0] --;
						if(DEV_actReserve & 0x02)PinState_tempRly2 = GPIO_PIN_SET; //

					}break;

					case 2:{ //

						period_rgbCnt[loop][0] --;
						if(DEV_actReserve & 0x01)PinState_tempRly3 = GPIO_PIN_SET; //

					}break;

					case 3:{ //

						period_rgbCnt[loop][0] --;
						PinState_tempNwk = GPIO_PIN_SET;					

					}break;

					default:{}break;
				}	
			}
		}
	}
	else
	if(P_rlyTips_reales < (RLY_TIPS_PERIODUNITS * 2) && P_rlyTips_reales >= (RLY_TIPS_PERIODUNITS * 1)){ //
	
		PIN_TIPS_PPP_G_SET(GPIO_PIN_RESET);
		for(loop = 0; loop < RLY_TIPS_NUM; loop ++){

			if(period_rgbCnt[loop][1]){

				switch(loop){

					case 0:{ //
						
						period_rgbCnt[loop][1] --;
						if(DEV_actReserve & 0x04)PinState_tempRly1 = GPIO_PIN_SET; //

					}break;

					case 1:{ //

						period_rgbCnt[loop][1] --;
						if(DEV_actReserve & 0x02)PinState_tempRly2 = GPIO_PIN_SET; //

					}break;

					case 2:{ //

						period_rgbCnt[loop][1] --;
						if(DEV_actReserve & 0x01)PinState_tempRly3 = GPIO_PIN_SET; //

					}break;

					case 3:{ //

						period_rgbCnt[loop][1] --;
						PinState_tempNwk = GPIO_PIN_SET;					

					}break;

					default:{}break;
				}	
			}
		}
	}
	else
	if(P_rlyTips_reales < (RLY_TIPS_PERIODUNITS * 3) && P_rlyTips_reales >= (RLY_TIPS_PERIODUNITS * 2)){	//
	
		PIN_TIPS_PPP_B_SET(GPIO_PIN_RESET);
		for(loop = 0; loop < RLY_TIPS_NUM; loop ++){

			if(period_rgbCnt[loop][2]){

				switch(loop){

					case 0:{ //
						
						period_rgbCnt[loop][2] --;
						if(DEV_actReserve & 0x04)PinState_tempRly1 = GPIO_PIN_SET; //

					}break;

					case 1:{ //

						period_rgbCnt[loop][2] --;
						if(DEV_actReserve & 0x02)PinState_tempRly2 = GPIO_PIN_SET; //

					}break;

					case 2:{ //

						period_rgbCnt[loop][2] --;
						if(DEV_actReserve & 0x01)PinState_tempRly3 = GPIO_PIN_SET; //

					}break;

					case 3:{ //

						period_rgbCnt[loop][2] --;
						PinState_tempNwk = GPIO_PIN_SET;					

					}break;

					default:{}break;
				}	
			}
		}
	}
	
	PIN_TIPS_RELAY1_PIN_SET(PinState_tempRly1);		
	PIN_TIPS_RELAY2_PIN_SET(PinState_tempRly2);	
	PIN_TIPS_RELAY3_PIN_SET(PinState_tempRly3);	
	PIN_TIPS_NWK_PIN_SET(PinState_tempNwk);	
}

void bsp_opreatReflectTips_Init(void){

	GPIO_InitTypeDef gpio_initstruct;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	gpio_initstruct.Mode 	= GPIO_MODE_OUTPUT_PP;
	gpio_initstruct.Speed	= GPIO_SPEED_FREQ_HIGH;
	gpio_initstruct.Pull	= GPIO_PULLDOWN;
	gpio_initstruct.Pin		= GPIO_PIN_7;                     
	HAL_GPIO_Init(GPIOA, &gpio_initstruct);
	gpio_initstruct.Pin		= GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_14;
	HAL_GPIO_Init(GPIOB, &gpio_initstruct);
	gpio_initstruct.Pin		= GPIO_PIN_14;                     
	HAL_GPIO_Init(GPIOC, &gpio_initstruct);
	
//	if(relayStatus_ifSave == statusSave_enable){

//		
//	}
	
	tid_usrTipsProcess_Thread = osThreadCreate(osThread(usrTipsProcess_Thread), NULL);
}

