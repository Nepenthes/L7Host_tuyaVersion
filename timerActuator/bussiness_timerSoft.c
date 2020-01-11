#include "bussiness_timerSoft.h"
#include "bussiness_timerHard.h"

#include "datsProcess_uartWifi.h"
#include "datsProcess_uartZigbee.h"

#include "bspDrv_optDevTips.h"

extern u8 dbSysLogLoopCounter;
extern uint8_t loopCounter_gatewayNwkOpen;
extern bool zigbNodeDevDetectManage_runningFLG;

extern struct _loopTrigger_wifiCfg{

	u8 timeCounter:7;
	u8 funcEnable:1;
}wifiCfg_triggerParam;

static void usrAppNormalbussiness_timerCb(void const *arg);

osTimerDef(timerUsrApp_nromalBussiness, usrAppNormalbussiness_timerCb);

static void usrAppNormalbussiness_timerCb(void const *arg){
	
	extern const u8 period_devRootHbReport; //
	extern u8 counter_devRootHbReport;
	
	const u8 period_nodeSystimeSynchronous = 10; //
	static u8 counter_nodeSystimeSynchronous = 0;
	
	const u8 period_rootSystimeReales = 15; //
	static u8 counter_rootSystimeReales = 0;
	
	const u8 period_localSystimeZigbAdjust = 20; //
	static u8 counter_localSystimeZigbAdjust = 0;
	
	bsp_iwdgFeed(); //喂狗
	
	{ /*毫秒周期*/
		
		extern stt_dataRemoteReq localZigbASYDT_bufQueueRemoteReq[zigB_remoteDataTransASY_QbuffLen];
		
		uint8_t loop = 0;
		
		for(loop = 0; loop < zigB_remoteDataTransASY_QbuffLen; loop ++){
		
			if(localZigbASYDT_bufQueueRemoteReq[loop].dataReqPeriod)localZigbASYDT_bufQueueRemoteReq[loop].dataReqPeriod --;
		}
	}
	
	{/*秒周期业务*/
		
		const u16 period_1second = 1000;
		static u16 counter_1second = 0;
		
		if(counter_1second < period_1second)counter_1second ++;
		else{

			counter_1second = 0;
			
			zigbNodeDevDetectManage_runningFLG = true;
			
			if(loopCounter_gatewayNwkOpen){
			
				loopCounter_gatewayNwkOpen --;
				if(!(loopCounter_gatewayNwkOpen % 10)){
					
					wifiFunction_callFromThread(msgFunWifi_nwkSelfOpen, true);
					tips_statusChangeToZigbNwkOpen(ZIGBNWKOPENTIME_DEFAULT);
				}
			}
			
			if(counter_nodeSystimeSynchronous < period_nodeSystimeSynchronous)counter_nodeSystimeSynchronous ++;
			else{

				counter_nodeSystimeSynchronous = 0;

				zigbFunction_callFromThread(msgFunZigb_nodeSystimeSynchronous);
			}
			
			if(counter_rootSystimeReales < period_rootSystimeReales)counter_rootSystimeReales ++;
			else{
			
				counter_rootSystimeReales = 0;
				
				zigbFunction_callFromThread(msgFunZigb_rootSystimeReales);
			}
			
			if(counter_localSystimeZigbAdjust < period_localSystimeZigbAdjust)counter_localSystimeZigbAdjust ++;
			else{
			
				counter_localSystimeZigbAdjust = 0;
				
				zigbFunction_callFromThread(msgFunZigb_localSystimeZigbAdjust);
			}
			
			if(counter_devRootHbReport < period_devRootHbReport)counter_devRootHbReport ++;
			else{
			
				counter_devRootHbReport = 0;
				
				wifiFunction_callFromThread(msgFunWifi_devRootHeartbeatTrig, false);
			}
			
			if(wifiCfg_triggerParam.funcEnable){
			
				static u8 loopLimit = 0;
				
				if(wifiCfg_triggerParam.timeCounter)wifiCfg_triggerParam.timeCounter --;
				else{
				
					wifiCfg_triggerParam.timeCounter = 5;
					
					wifiFunction_callFromThread(msgFunWifi_nwkCfg, true);
					
					if(loopLimit < 5)loopLimit ++;
					else{
					
						loopLimit = 0;
						wifiCfg_triggerParam.funcEnable = 0;
					}
				}
			}
			
			if(timeCount_zigNwkOpen)timeCount_zigNwkOpen --;
			
			if(dbSysLogLoopCounter)dbSysLogLoopCounter --;
		}
	}
}

void usrAppNormalBussiness_softTimerInit(void){

	osTimerId tid_usrAppTimer_normalBussiness = NULL;
	
	tid_usrAppTimer_normalBussiness = osTimerCreate(osTimer(timerUsrApp_nromalBussiness), osTimerPeriodic, NULL);
	osTimerStart(tid_usrAppTimer_normalBussiness, 1UL);
}

