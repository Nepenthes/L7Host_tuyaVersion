#include "datsProcess_uartWifi.h"

#include "datsProcess_uartZigbee.h"

#include "bspDrv_optDevTips.h"

#include "bspDrv_optDevRelay.h"

extern osPoolId (threadDP_poolAttr_id);
extern osMessageQId  mqID_threadDP_Z2W;

static const char tuyaPid_Tab[10][17] = {

	PRODUCT_KEY_GATEWAY,
	PRODUCT_KEY_SWBIT3,
	PRODUCT_KEY_SWBIT2,
	PRODUCT_KEY_SWBIT1,
	PRODUCT_KEY_DIMMER,
	PRODUCT_KEY_CURTAIN,
	PRODUCT_KEY_HEATER,
	PRODUCT_KEY_SOCKET,
	PRODUCT_KEY_FANS,
};

bool nwkInternetOnline_IF = false;

bool nwkWifi_cfg_method = false;

const u8 period_devRootHbReport = 20; //
	  u8 counter_devRootHbReport = 0;

uint8_t loopCounter_gatewayNwkOpen = 0;

struct _loopTrigger_wifiCfg{

	u8 timeCounter:7;
	u8 funcEnable:1;
}wifiCfg_triggerParam = {0};

static void uartWifiDataRcvHandle_Thread(const void *argument);
osThreadId tid_uartWifiDataRcvHandle_Thread;
osThreadDef(uartWifiDataRcvHandle_Thread, osPriorityNormal,	1, 1024);

osPoolId  (WifiUartMsgDatatrans_poolAttr_id); // Memory pool ID              
osPoolDef (WifiUartMsgDatatrans_pool, 5, stt_tuyaWifiMoudle_frameParam); // 

osPoolId  (WifiFuncRemind_poolAttr_id); // Memory pool ID              
osPoolDef (WifiFuncRemind_pool, 10, stt_wifiFunMsg); // 

osMessageQId  mqID_threadDP_W2Z; // messageQ ID
osMessageQDef(MsgBox_threadDP_W2Z, 20, &stt_threadDatsPass); // Define message queue	//WIFI到ZigB消息队列

osMessageQId  mqID_uartWifiToutDats_datatrans; // messageQ ID              	
osMessageQDef(MsgBox_uartWifiToutDats_datatrans, 3, &stt_tuyaWifiMoudle_frameParam); //

osMessageQId  mqID_wifiFuncRemind; // messageQ ID              	
osMessageQDef(MsgBox_wifiFuncRemind, 8, &stt_wifiFunMsg);

static enum_wifiNwkStatus devWifi_nwkStatus = wifiNwkStatus_unKnown;

static osThreadId tid_wifiThread = NULL;
static osThreadId tid_wifiUartDatsRcvThread = NULL;
static type_uart_OBJ uartObj_current = comObj_DbugP1;
static bool localUart_txCmpFlg = false;
static ARM_DRIVER_USART *uartDrvObj_local = NULL;
static uint32_t localSigVal_uartWifi_rxTout = 0;

static _sttTuyaNodeRecordDatsForm tuyaNodeRecord_infoHandle[USRAPP_DEVNODE_MAX_SUM] = {0}, //虚表，处理缓存，防止处理阶段被使用，用于应对异步操作
								  tuyaNodeRecord_infoCfm[USRAPP_DEVNODE_MAX_SUM] = {0}; //实表，处理结果存储，防止处理阶段被使用，用于应对异步操作
static uint8_t tuyaNodeRecord_nodeSum = 0;

void uartConWifi_eventFunCb(uint32_t event){

	if(event & (ARM_USART_EVENT_RX_TIMEOUT | ARM_USART_EVENT_RX_OVERFLOW | ARM_USART_EVENT_RECEIVE_COMPLETE)){
	
		osSignalSet(tid_wifiUartDatsRcvThread, localSigVal_uartWifi_rxTout);
	}
	
	if(event & ARM_USART_EVENT_TX_COMPLETE){
	
		localUart_txCmpFlg = true;
	}
}

void uartConWifi_dataTxPort(unsigned char *data, unsigned short dataLen){

	uartDrvObj_local->Send(data, dataLen);
}

u8 frame_Check(unsigned char frame_temp[], u8 check_num){
  
	u8 loop 		= 0;
	u8 val_Check 	= 0;
	
	for(loop = 0; loop < check_num; loop ++){
	
		val_Check += frame_temp[loop];
	}
	
	val_Check  = ~val_Check;
	val_Check ^= 0xa7;
	
	return val_Check;
}

static uint8_t usrApp_checkNumCal_add8(uint8_t *dats, uint16_t datsLen){

	uint8_t checkNum_res = 0;
	uint16_t loop = 0;
	
	for(loop = 0; loop < datsLen; loop ++){
	
		checkNum_res += dats[loop];
	}
	
	return checkNum_res;
}

static bool tuyaApp_frameWifiCheck(uint8_t *frame, uint16_t frameLen){

	bool res = false;
	
	bool checkedFlg_frameLen = false,
		 checkedFlg_frameCheckNum = false;
	uint8_t  frameCheckNum		= frame[frameLen - 1];
	uint16_t frameKernelDataLen = (((uint16_t)frame[4] << 8) & 0xFF00) |
								  (((uint16_t)frame[5] << 0) & 0x00FF);
	uint8_t  frameCheckNum_cal	= 0;
	
	if(frameLen == (frameKernelDataLen + tuyaF_FRAMESHELL_DATALEN))checkedFlg_frameLen = true;
	if(checkedFlg_frameLen){
		
		frameCheckNum_cal = usrApp_checkNumCal_add8(frame, frameLen - 1);
		if(frameCheckNum_cal == frameCheckNum)checkedFlg_frameCheckNum = true;
	}
	
	if(checkedFlg_frameLen & checkedFlg_frameCheckNum){
	
		res = true;
	}
	
	return res;
}

void usrApp_gateWayfuncNwkopen_opreat(bool opreatParam){

	if(opreatParam){
	
		loopCounter_gatewayNwkOpen = 212;
	}
	else{
	
		loopCounter_gatewayNwkOpen = 0;
	}
}

void usrApp_gateWayfuncNwkopen_terminate(void){

	if(loopCounter_gatewayNwkOpen > 10)
		loopCounter_gatewayNwkOpen = 0;
}

//static uint8_t usrApp_tuyaDpDnloadBykeepAccess_dataLoad(uint8_t *dataCont, uint8_t dataType, void *dataParam){

//	
//}

static uint8_t tuyaApp_devMulitAddStrCid_get(char *strTemp, uint8_t devType){

	uint8_t devList_info[6 * 73] = {0};
	uint8_t devNum = 0;
	uint8_t reserveNum = 0;
	
	extern nwkStateAttr_Zigb *zigbDevList_Head;
	extern u8 ZigBdevDispList(nwkStateAttr_Zigb *pHead, u8 DevInform[]);
	
	devNum += 1;
	memcpy(devList_info, devRootNode_subidMac, 5 * sizeof(uint8_t));
	devList_info[5] = SWITCH_TYPE;
	devNum += ZigBdevDispList(zigbDevList_Head, &devList_info[6 * 1]);
	
	if(devNum){
	
		uint8_t loop = 0;
		
		char devInfo_unit[16] = {0};
		
		for(loop = 0; loop < devNum; loop ++){
			
			if(devList_info[loop * (DEVMAC_LEN + 1) + 5] == devType){
				
				uint8_t loopA = 0;
				bool targetRsr_flg = true;
				
				for(loopA = 0; loopA < tuyaNodeRecord_nodeSum; loopA ++){
					
					if(tuyaNodeRecord_infoCfm[loopA].nodeType == devType){
					
						if(!memcmp((const void *)tuyaNodeRecord_infoCfm[loopA].nodeMac, &devList_info[loop * (DEVMAC_LEN + 1)], 5)){
							
							targetRsr_flg = false;
							break;
						}
					}
				}
				
				if(targetRsr_flg){
				
					if(reserveNum)strcat(strTemp, ",");
				
					sprintf(devInfo_unit, "\"%02X%02X%02X%02X%02X%02X\"", 
										  devType,
										  MAC5_2STR(&devList_info[loop * (DEVMAC_LEN + 1)]));
					
					strcat(strTemp, devInfo_unit);
					
					reserveNum ++;
				}
			}
		}
	}
	
	return reserveNum;
}

static uint16_t tuyaApp_datapointFormatLoad(uint8_t *cont, uint8_t dpid, uint8_t type, uint8_t *dats, uint16_t datsLen){

	uint16_t totelLen = 0;
	
	cont[0] = dpid;
	cont[1] = type;
	cont[2] = (uint8_t)((datsLen >> 8) & 0x00ff);
	cont[3] = (uint8_t)((datsLen >> 0) & 0x00ff);
	memcpy(&cont[4], dats, sizeof(uint8_t) * datsLen);
	
	totelLen = 4 + datsLen;
	
	return totelLen;
}

static uint16_t tuyaApp_frameWifiLoad(uint8_t *frameTemp, stt_tuyaWifiMoudle_frameParam param){

	frameTemp[0] = tuyaF_FRAME_HEAD_A;
	frameTemp[1] = tuyaF_FRAME_HEAD_B;
	frameTemp[2] = tuyaF_FRAME_HEAD_VER;
	frameTemp[3] = param.frameCmd;
	frameTemp[4] = (uint8_t)((param.dataKernelLen >> 8) & 0x00ff);
	frameTemp[5] = (uint8_t)((param.dataKernelLen >> 0) & 0x00ff);
	memcpy(&frameTemp[6], param.dataKernel, param.dataKernelLen * sizeof(uint8_t));
	frameTemp[param.dataKernelLen + 6] = usrApp_checkNumCal_add8(frameTemp, param.dataKernelLen + 6);
	
	return (param.dataKernelLen + tuyaF_FRAMESHELL_DATALEN);
}

char *tuyaApp_devTypeDefPid_strGet(uint8_t devType){

	const char *pidStr = NULL;
	
//	PRODUCT_KEY_GATEWAY,	0
//	PRODUCT_KEY_SWBIT3,		1
//	PRODUCT_KEY_SWBIT2,		2
//	PRODUCT_KEY_SWBIT1,		3
//	PRODUCT_KEY_DIMMER,		4
//	PRODUCT_KEY_CURTAIN,	5
//	PRODUCT_KEY_HEATER,		6
//	PRODUCT_KEY_SOCKET,		7
//	PRODUCT_KEY_FANS,		8
	
	switch(devType){
	
		case SWITCH_TYPE_SWBIT1:	{pidStr = tuyaPid_Tab[3];}break;
			
		case SWITCH_TYPE_SWBIT2:	{pidStr = tuyaPid_Tab[2];}break;
		
		case SWITCH_TYPE_SWBIT3:	{pidStr = tuyaPid_Tab[1];}break;
			
		case SWITCH_TYPE_CURTAIN:	{pidStr = tuyaPid_Tab[5];}break;		

		case SWITCH_TYPE_DIMMER:	{pidStr = tuyaPid_Tab[4];}break;	
			
		case SWITCH_TYPE_FANS:		{pidStr = tuyaPid_Tab[8];}break;	
			
//		case SWITCH_TYPE_INFRARED:	{pidStr = tuyaPid_Tab[1];}break;
			
		case SWITCH_TYPE_SOCKETS:	{pidStr = tuyaPid_Tab[7];}break;	
			
//		case SWITCH_TYPE_SCENARIO:	{pidStr = tuyaPid_Tab[1];}break;		
			
		case SWITCH_TYPE_HEATER:	{pidStr = tuyaPid_Tab[6];}break;				
			
		default:break;
	}
	
	return (char *)pidStr;
}

static void wifiThreadPrt_uartDataRcv_process(void){
	
	osEvent evt;
	static uint32_t uartDataRcv_len = 0;
	static bool uartRcv_startFlg = false;
	static uint8_t uartDataRcv_temp[UARTWIFI_DATA_HANDLE_TEMP_LENGTH] = {0};
	
	if(!uartRcv_startFlg){
	
		uartDrvObj_local->Control(ARM_USART_ABORT_RECEIVE, 0);
		uartDrvObj_local->Receive(uartDataRcv_temp, UARTWIFI_DATA_HANDLE_TEMP_LENGTH);
		uartRcv_startFlg = true;
		
//		DBLOG_PRINTF("utWifi str.\n");
	}
	
	evt = osSignalWait(localSigVal_uartWifi_rxTout, 0);
	if(osEventSignal == evt.status){
		
		uint16_t frameLen_temp = 0;

		if(uartDrvObj_local->GetRxCount()){ //tout
		
			frameLen_temp = uartDrvObj_local->GetRxCount();
		}
		else //overflow
		{
			if((uartDataRcv_temp[0] == tuyaF_FRAME_HEAD_A) &&
			   (uartDataRcv_temp[1] == tuyaF_FRAME_HEAD_B)){
			   
				frameLen_temp = (((uint16_t)uartDataRcv_temp[4] << 8) & 0xff00) | ((uint16_t)uartDataRcv_temp[5] & 0x00ff);
				frameLen_temp += 5; 
			}
		}
		
//		DBLOG_PRINTF("utWf rcv(%d).\n", frameLen_temp);
		
		if(frameLen_temp){
		
			if(tuyaApp_frameWifiCheck(uartDataRcv_temp, frameLen_temp)){
				
				stt_tuyaWifiMoudle_frameParam *mptr_uartWifiData = (stt_tuyaWifiMoudle_frameParam *)osPoolAlloc(WifiUartMsgDatatrans_poolAttr_id);
				
				if(mptr_uartWifiData != NULL){
				
					osStatus res = 0;
					
					memcpy(mptr_uartWifiData->dataKernel, &uartDataRcv_temp[6], frameLen_temp - 7);
					mptr_uartWifiData->dataKernelLen = frameLen_temp - 7;
					mptr_uartWifiData->frameCmd = uartDataRcv_temp[3];
					
					res = osMessagePut(mqID_uartWifiToutDats_datatrans, (uint32_t)mptr_uartWifiData, 0);
					
					if(res){
					
						DBLOG_PRINTF("utWifi msg ptRes:%d.\n", res);
					}
				}
				else
				{
					DBLOG_PRINTF("ptrWfUtMsg full.\n");
				}
			}
		}
		
//		DBLOG_PRINTF("uartWifi dataRcv[len:%d]:%02X, %02X, %02X.\n", 
//					 frameLen_temp,
//					 uartDataRcv_temp[0],
//					 uartDataRcv_temp[1],
//					 uartDataRcv_temp[2]);
		
		memset(uartDataRcv_temp, 0, sizeof(uint8_t) * UARTWIFI_DATA_HANDLE_TEMP_LENGTH);
		uartRcv_startFlg = false;
	}
}

static void uartWifiDataRcvHandle_Thread(const void *argument){

	tid_wifiUartDatsRcvThread = osThreadGetId();
	
	for(;;){
	
		wifiThreadPrt_uartDataRcv_process();
	}
}

static void threadProcessWifi_initialization(type_uart_OBJ UART_OBJ){

	tid_wifiThread = osThreadGetId();
	uartObj_current = UART_OBJ;
	localSigVal_uartWifi_rxTout = uartRxTimeout_signalDefGet(UART_OBJ);
	uartDrvObj_local = uartDrvObjGet(UART_OBJ);
	
	WifiUartMsgDatatrans_poolAttr_id = osPoolCreate(osPool(WifiUartMsgDatatrans_pool)); //wifi数据传输 数据结构堆内存池初始化
	
	mqID_threadDP_W2Z 				= osMessageCreate(osMessageQ(MsgBox_threadDP_W2Z), 				 NULL);	//wifi到zigbee消息队列初始化
	mqID_uartWifiToutDats_datatrans = osMessageCreate(osMessageQ(MsgBox_uartWifiToutDats_datatrans), NULL);	//
	
	WifiFuncRemind_poolAttr_id 		= osPoolCreate(osPool(WifiFuncRemind_pool));	//
	mqID_wifiFuncRemind 			= osMessageCreate(osMessageQ(MsgBox_wifiFuncRemind), NULL);	//
	
	tid_uartWifiDataRcvHandle_Thread = osThreadCreate(osThread(uartWifiDataRcvHandle_Thread), NULL);
	usrApp_usartEventCb_register(UART_OBJ, uartConWifi_eventFunCb); //串口回调注册
	
	DBLOG_PRINTF("uartWifi Initialize.\n");
}

void wifiFunction_nwkCfgStart(void){

	wifiCfg_triggerParam.funcEnable = 1;
	
	usrApp_gateWayfuncNwkopen_terminate();
}

void wifiFunction_callFromThread(enum_wifiFunMsg funcType, bool forceOpt_if){
	
	stt_wifiFunMsg *mptr_wifiFuncRemind = (stt_wifiFunMsg *)osPoolAlloc(WifiFuncRemind_poolAttr_id);

	if(!nwkInternetOnline_IF && !forceOpt_if)return;
	else{
	
		mptr_wifiFuncRemind->funcType = funcType;
		osMessagePut(mqID_wifiFuncRemind, (uint32_t)mptr_wifiFuncRemind, 0);
	}
}

void usrApp_devWifiStatus_set(enum_wifiNwkStatus status){

	devWifi_nwkStatus = status;
}

enum_wifiNwkStatus usrApp_devWifiStatus_get(void){

	return devWifi_nwkStatus;
}

/*WIFI主线程*/
void WIFI_mainThread(type_uart_OBJ UART_OBJ){
	
	osEvent evt = {0};
	uint8_t uartDataReq_temp[UARTWIFI_DATA_HANDLE_TEMP_LENGTH] = {0};
	uint16_t uartDataReq_len = 0;
	stt_tuyaWifiMoudle_frameParam uartWifiDataReq_paramTemp = {0};
	bool dataReq_trigFlg = false;
	
	const  uint8_t devType_sum = 8;
	static uint8_t loopCountRecord_searchResp = 0;
	
//	struct processRunningTips{
//	
//		const uint16_t period;
//			  uint16_t counter[5];
//		
//	}lRcod_processTips = {
//	
//		.period = 3000,
//	};
	
	threadProcessWifi_initialization(UART_OBJ);
	osDelay(1000);
	
	for(;;){
		
		{ //开机查询一下模块内子设备
		
			static uint8_t localRecord_checkTime = 3;
			static uint8_t localTimeCount_tuyaChildCheck = 0;
			const  uint8_t timePeriod_tuyaChildCheck = 10;
			
			if(localRecord_checkTime){

				if(localTimeCount_tuyaChildCheck != (systemTime_current.time_Second / timePeriod_tuyaChildCheck)){
					
					localTimeCount_tuyaChildCheck = (systemTime_current.time_Second / timePeriod_tuyaChildCheck);
				
					localRecord_checkTime --;
					
					wifiFunction_callFromThread(msgFunWifi_tuyaNodeRecordCheck, true); //触发tuya设备列表记录更新
				}
			}
		}
		
//		{ //开机查询一次网络状态
//		
//			static bool nwkCheckOnce_flg = false;
//			
//			if(!nwkCheckOnce_flg && 
//			   ((systemTime_current.time_Second % 9) == 1)){
//			
//				wifiFunction_callFromThread(msgFunWifi_nwkStatusGet, true);
//				
//				nwkCheckOnce_flg = true;
//			}
//		}
		
		//设备搜索时大间断异步轮流响应，(响应频率过高，会导致wifi模块无法完整接收所有开关信息)
		{
			
			if(loopCountRecord_searchResp){
			
				const uint8_t devType_Tab[8] = {
					
					SWITCH_TYPE_SWBIT1, SWITCH_TYPE_SWBIT2, SWITCH_TYPE_SWBIT3,
					SWITCH_TYPE_CURTAIN,
					SWITCH_TYPE_DIMMER,
					SWITCH_TYPE_HEATER,
					SWITCH_TYPE_FANS,
					SWITCH_TYPE_SOCKETS
				};
				
				char cidsStr_temp[512] = {0};
				
				static uint8_t localTimeCount = 0;
				const  uint8_t timePeriod_trans = 3;
				
				static uint8_t localTimeCount_tuyaChildCheck = 0;
				const  uint8_t timePeriod_tuyaChildCheck = 10;
				
				uint8_t cidNum = 0;
				
				if(localTimeCount_tuyaChildCheck != (systemTime_current.time_Second / timePeriod_tuyaChildCheck)){
					
					localTimeCount_tuyaChildCheck = (systemTime_current.time_Second / timePeriod_tuyaChildCheck);
				
					wifiFunction_callFromThread(msgFunWifi_tuyaNodeRecordCheck, true); //触发tuya设备列表记录更新
				}
				
				if(localTimeCount != (systemTime_current.time_Second / timePeriod_trans)){ //频率：s
				
					localTimeCount = (systemTime_current.time_Second / timePeriod_trans);
					
					loopCountRecord_searchResp --;
					
					memset(&uartWifiDataReq_paramTemp, 0, sizeof(stt_tuyaWifiMoudle_frameParam));
					memset(uartDataReq_temp, 0, sizeof(uint8_t) * UARTWIFI_DATA_HANDLE_TEMP_LENGTH);
					memset(cidsStr_temp, 0, sizeof(uint8_t) * 512);
					
					cidNum = tuyaApp_devMulitAddStrCid_get(cidsStr_temp, devType_Tab[loopCountRecord_searchResp]);
//					DBLOG_PRINTF("cidUnitNum:%d.\n", cidNum);
					
					if(cidNum){
					
						uartWifiDataReq_paramTemp.frameCmd = tuyaF_CHILDDEV_MULITADD_CMD;
						sprintf((char *)uartWifiDataReq_paramTemp.dataKernel, "{\"pid\":\"%s\",\"cids\":[%s],\"ver\":\""MCU_VER"\"}", 
																			  tuyaApp_devTypeDefPid_strGet(devType_Tab[loopCountRecord_searchResp]),
																			  cidsStr_temp);
						uartWifiDataReq_paramTemp.dataKernelLen = strlen((const char *)uartWifiDataReq_paramTemp.dataKernel);
						
//						DBLOG_PRINTF(">>>>len:%d.\n", uartWifiDataReq_paramTemp.dataKernelLen);
						
						uartDataReq_len = tuyaApp_frameWifiLoad(uartDataReq_temp, uartWifiDataReq_paramTemp);
						uartDrvObj_local->Send(uartDataReq_temp, uartDataReq_len);
					}
					else
					{ //当前扫描品类无设备加速过渡到下一品类扫描
						localTimeCount += timePeriod_trans;
					}
				}
			}
		}
		
		//Z2W线程消息接收
		evt = osMessageGet(mqID_threadDP_Z2W, 0);
		if(evt.status == osEventMessage){
		
			stt_threadDatsPass *rptr_Z2W = evt.value.p;
			
			memset(&uartWifiDataReq_paramTemp, 0, sizeof(stt_tuyaWifiMoudle_frameParam));
			memset(uartDataReq_temp, 0, sizeof(uint8_t) * UARTWIFI_DATA_HANDLE_TEMP_LENGTH);
			
			switch(rptr_Z2W->msgType){
			
				case conventional:{
				
					switch(rptr_Z2W->dats.dats_conv.datsFrom){

						/*远端*/
						case datsFrom_ctrlRemote:{}break;
							
						/*本地*/
						case datsFrom_ctrlLocal:{
							
							uint8_t devCmd_coming = rptr_Z2W->dats.dats_conv.dats[3];
							
							switch(devCmd_coming){
							
								case FRAME_MtoSCMD_cmdConfigSearch:{
									
									dataReq_trigFlg = false;
									
									if(!loopCountRecord_searchResp){  //搜索响应逻辑出发，本地表内搜寻所有八种类型开关
									
										loopCountRecord_searchResp = devType_sum;
									}
									
//									uartWifiDataReq_paramTemp.frameCmd = tuyaF_CHILDDEV_ADD_CMD;
//									sprintf((char *)uartWifiDataReq_paramTemp.dataKernel, "{\"pk_type\":%02d,\"sub_id\":\"%02X%02X%02X%02X%02X\",\"pid\":\"""%s""\",\"ver\":\""MCU_VER"\"}", rptr_Z2W->dats.dats_conv.dats[10],
//																																															 rptr_Z2W->dats.dats_conv.dats[5],
//																																															 rptr_Z2W->dats.dats_conv.dats[6],
//																																															 rptr_Z2W->dats.dats_conv.dats[7],
//																																															 rptr_Z2W->dats.dats_conv.dats[8],
//																																															 rptr_Z2W->dats.dats_conv.dats[9],
//																																															 tuyaApp_devTypeDefPid_strGet(rptr_Z2W->dats.dats_conv.dats[10]));
//									uartWifiDataReq_paramTemp.dataKernelLen = strlen(uartWifiDataReq_paramTemp.dataKernel);
//									
//									dataReq_trigFlg = true;

								}break;
								
								case FRAME_MtoSCMD_cmdControl:{
								
//									const uint8_t tuyaSubId_len = (5 + 1) * 2;
//									uint8_t tuyaDpKernel_temp[16] = {0};
//									
//									uartWifiDataReq_paramTemp.frameCmd = tuyaF_DP_REPORT_CMD;
//									
//									uartWifiDataReq_paramTemp.dataKernel[0] = tuyaSubId_len;
//									sprintf((char *)&uartWifiDataReq_paramTemp.dataKernel[1], "%02X"MAC5STR, 
//																							  MAC5_2STR(rptr_Z2W->dats.dats_conv.macAddr),
//																							  rptr_Z2W->dats.dats_conv.devType);
//									
//									tuyaDpKernel_temp[3] = rptr_Z2W->dats.dats_conv.dats[11] & 0x07;
//									
//									uartWifiDataReq_paramTemp.dataKernelLen =\
//										tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[1 + tuyaSubId_len], 
//																	tuyaF_APP_DP_CMD_DEVSTAUS,
//																	0x02,
//																	tuyaDpKernel_temp,
//																	4) + tuyaSubId_len + 1;
//									
//									dataReq_trigFlg = true;
									
								}break;
								
								default:break;
							}
						
						}break;
							
						/*keepAccess/heartbeat*/
						case datsFrom_heartBtRemote:{
						
							//keepaccess机制没有自家服务器处理，所以网关自行回复
							stt_threadDatsPass *mptr_W2Z = (stt_threadDatsPass *)osPoolAlloc(threadDP_poolAttr_id);
							
							if(mptr_W2Z == NULL){
							
								DBLOG_PRINTF("mPool_threaDp null.\n");
							}
							
							memcpy(mptr_W2Z, rptr_Z2W, sizeof(stt_threadDatsPass));
							
							switch(rptr_Z2W->dats.dats_conv.dats[0]){
								
								case FRAME_HEAD_HEARTB:{

									if(rptr_Z2W->dats.dats_conv.dats[2] == FRAME_HEARTBEAT_cmdEven){ //
									
										mptr_W2Z->dats.dats_conv.dats[0] = ZIGB_OFFLINEFRAMEHEAD_HEARTBEAT;
										mptr_W2Z->dats.dats_conv.datsFrom = datsFrom_heartBtRemote;
										
										osMessagePut(mqID_threadDP_W2Z, (uint32_t)mptr_W2Z, 0);
									
									}else
									if(rptr_Z2W->dats.dats_conv.dats[2] == FRAME_HEARTBEAT_cmdOdd){ //
									
									
									}
									
								}break;
							
								case DTMODEKEEPACESS_FRAMEHEAD_ONLINE:{
									
									osStatus mqRes = osOK;
									stt_devOpreatDataPonit dev_dataPointTemp = {0};
									const uint8_t tuyaSubId_len = (5 + 1) * 2;
									const uint8_t tuyaDpKernel_tempLen = 16;
									uint8_t tuyaDpKernel_temp[16] = {0};
									uint8_t dataFrameLoad_ist = 0;

									memcpy(&dev_dataPointTemp, &rptr_Z2W->dats.dats_conv.dats[15], sizeof(stt_devOpreatDataPonit)); //数据结构化
									memcpy(mptr_W2Z, rptr_Z2W, sizeof(stt_threadDatsPass));
									mptr_W2Z->dats.dats_conv.datsFrom = datsFrom_heartBtRemote;
									
									if(rptr_Z2W->dats.dats_conv.dats[8] == DTMODEKEEPACESS_FRAMECMD_ASR){
										
										uint8_t frameLen = mptr_W2Z->dats.dats_conv.dats[1];
										
										memset(&mptr_W2Z->dats.dats_conv.dats[15], 0, sizeof(stt_agingDataSet_bitHold)); //0xA1时时效位清零
										mptr_W2Z->dats.dats_conv.dats[frameLen - 1] = \
											frame_Check(&mptr_W2Z->dats.dats_conv.dats[1], frameLen - 2);
									}
									
									mqRes = osMessagePut(mqID_threadDP_W2Z, (uint32_t)mptr_W2Z, 0); //keepaccess回复
									if(mqRes){
									
										DBLOG_PRINTF("W2Z mq.0 txFailRes:%08X.\n", mqRes); //keepaccess回复
									}	

									{ //指定对象打印
									
//										if(!memcmp((const void *)debugLogOut_targetMAC, rptr_Z2W->dats.dats_conv.macAddr, 5)){
										
											osDelay(10);
											DBLOG_PRINTF("KPAC<%02X> rcv mac"MAC5STR"-[%s]{%02X}\n", 
														 rptr_Z2W->dats.dats_conv.dats[8],
														 MAC5_2STR(rptr_Z2W->dats.dats_conv.macAddr),
														 typeStrgetByNum(rptr_Z2W->dats.dats_conv.devType),
														 rptr_Z2W->dats.dats_conv.dats[21]);
//										}
									}
									
									//tuyaDp上报
									uartWifiDataReq_paramTemp.dataKernel[0] = tuyaSubId_len;
									sprintf((char *)&uartWifiDataReq_paramTemp.dataKernel[1], "%02X"MAC5STR, 
																							  rptr_Z2W->dats.dats_conv.devType,
																							  MAC5_2STR(rptr_Z2W->dats.dats_conv.macAddr));
									
									dataFrameLoad_ist += (1 + tuyaSubId_len);

									switch(rptr_Z2W->dats.dats_conv.devType){
									
										case SWITCH_TYPE_SWBIT1:{
										
											(dev_dataPointTemp.devStatus_Reference.statusRef_swStatus)?
												(tuyaDpKernel_temp[0] = 1):
												(tuyaDpKernel_temp[0] = 0);
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG1_DU_SWITCH,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devStatus_Reference.statusRef_swStatus >> 0) & 0x01;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG1_DU_BIT1,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG1_DU_AURORA,
//																			dpType_bool,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[0];
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG1_DU_KCOLOR_PRE,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[1];
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG1_DU_KCOLOR_REA,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											
										}break;
											
										case SWITCH_TYPE_SWBIT2:{
										
											(dev_dataPointTemp.devStatus_Reference.statusRef_swStatus)?
												(tuyaDpKernel_temp[0] = 1):
												(tuyaDpKernel_temp[0] = 0);
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG2_DU_SWITCH,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);		
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devStatus_Reference.statusRef_swStatus >> 0) & 0x01;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG2_DU_BIT1,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devStatus_Reference.statusRef_swStatus >> 1) & 0x01;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG2_DU_BIT2,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG2_DU_AURORA,
//																			dpType_bool,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[0];
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG2_DU_KCOLOR_PRE,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[1];
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG2_DU_KCOLOR_REA,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											
										}break;
											
										case SWITCH_TYPE_SWBIT3:{
										
											(dev_dataPointTemp.devStatus_Reference.statusRef_swStatus)?
												(tuyaDpKernel_temp[0] = 1):
												(tuyaDpKernel_temp[0] = 0);
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG3_DU_SWITCH,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devStatus_Reference.statusRef_swStatus >> 0) & 0x01;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG3_DU_BIT1,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devStatus_Reference.statusRef_swStatus >> 1) & 0x01;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG3_DU_BIT2,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devStatus_Reference.statusRef_swStatus >> 2) & 0x01;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SWGANG3_DU_BIT3,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG3_DU_AURORA,
//																			dpType_bool,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[0];
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG3_DU_KCOLOR_PRE,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[1];
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_SWGANG3_DU_KCOLOR_REA,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											
										}break; 
											
										case SWITCH_TYPE_SOCKETS:{
										
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devStatus_Reference.statusRef_swStatus >> 0) & 0x01;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SOCKET_DU_SWITCH,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0 + 2] = dev_dataPointTemp.union_devParam.socket_param.data_elePower[0];
											tuyaDpKernel_temp[0 + 3] = dev_dataPointTemp.union_devParam.socket_param.data_elePower[1];
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_SOCKET_UU_POWER,
																			dpType_value,
																			tuyaDpKernel_temp,
																			4);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											
										}break;
											
										case SWITCH_TYPE_CURTAIN:{
										
											tuyaDpKernel_temp[0] = devCurtain_statusCodeChg_D2S(dev_dataPointTemp.devStatus_Reference.statusRef_swStatus);
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_CURTAIN_DU_ACTION,
																			dpType_enum,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0 + 3] = dev_dataPointTemp.union_devParam.curtain_param.orbital_Period;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_CURTAIN_DU_ORBITAL_TIME,
																			dpType_value,
																			tuyaDpKernel_temp,
																			4);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_CURTAIN_DU_AURORA,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[0];
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_CURTAIN_DU_KCOLOR_PRE,
																			dpType_enum,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = dev_dataPointTemp.devData_bkLight[1];
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_CURTAIN_DU_KCOLOR_REA,
																			dpType_enum,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											
										}break;
											
										case SWITCH_TYPE_FANS:{
										
											(dev_dataPointTemp.devStatus_Reference.statusRef_swStatus)?(tuyaDpKernel_temp[0] = 1):(tuyaDpKernel_temp[0] = 0);
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_FANS_DU_SWITCH,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[3] = dev_dataPointTemp.devStatus_Reference.statusRef_swStatus;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_FANS_DU_GEAR,
																			dpType_value,
																			tuyaDpKernel_temp,
																			4);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_FANS_DU_AURORA,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[0] >> 0) & 0x0f;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_FANS_DU_TCOLOR_OFF,
																			dpType_enum,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[0] >> 4) & 0x0f;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_FANS_DU_TCOLOR_GEAR1,
																			dpType_enum,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[1] >> 0) & 0x0f;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_FANS_DU_TCOLOR_GEAR2,
																			dpType_enum,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[1] >> 4) & 0x0f;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_FANS_DU_TCOLOR_GEAR3,
																			dpType_enum,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											
										}break;
											
										case SWITCH_TYPE_HEATER:{
										
											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_swStatus;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_HEATER_DU_SWITCH,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_HEATER_DU_AURORA,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);

										}break;
											
										case SWITCH_TYPE_DIMMER:{
										
											(dev_dataPointTemp.devStatus_Reference.statusRef_swStatus)?
												(tuyaDpKernel_temp[0] = 1):
												(tuyaDpKernel_temp[0] = 0);
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_DIMMER_DU_SWITCH,
																			dpType_bool,
																			tuyaDpKernel_temp,
																			1);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											tuyaDpKernel_temp[3] = mptr_W2Z->dats.dats_conv.dats[21] & 0x7F;
											dataFrameLoad_ist += \
												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
																			tuyaDP_DEV_DIMMER_DU_BRIGHTNESS,
																			dpType_value,
																			tuyaDpKernel_temp,
																			4);
											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
				
//											tuyaDpKernel_temp[0] = dev_dataPointTemp.devStatus_Reference.statusRef_horsingLight;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_DIMMER_DU_AURORA,
//																			dpType_bool,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[0] >> 0) & 0x0f;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_DIMMER_DU_KCOLOR_TRIG,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[0] >> 4) & 0x0f;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_DIMMER_DU_TCOLOR_BMIN,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[1] >> 0) & 0x0f;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_DIMMER_DU_TCOLOR_BNOR,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
//											tuyaDpKernel_temp[0] = (dev_dataPointTemp.devData_bkLight[1] >> 4) & 0x0f;
//											dataFrameLoad_ist += \
//												tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
//																			tuyaDP_DEV_DIMMER_DU_TCOLOR_BMAX,
//																			dpType_enum,
//																			tuyaDpKernel_temp,
//																			1);
//											memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
											
										}break;
											
										default:break;
									}	
									
									uartWifiDataReq_paramTemp.frameCmd 		= tuyaF_DP_REPORT_CMD;
									uartWifiDataReq_paramTemp.dataKernelLen = dataFrameLoad_ist;
									
									switch(rptr_Z2W->dats.dats_conv.devType){ //tuya网关支持八种类型子设备
									
										case SWITCH_TYPE_DIMMER:				
										case SWITCH_TYPE_SWBIT1:
										case SWITCH_TYPE_SWBIT2:	
										case SWITCH_TYPE_SWBIT3: 			
										case SWITCH_TYPE_CURTAIN:
										case SWITCH_TYPE_FANS:				
										case SWITCH_TYPE_SOCKETS:							 
										case SWITCH_TYPE_HEATER:{
										
											dataReq_trigFlg = true;		
											
										}break;
										
										default:{
										
											dataReq_trigFlg = false;		
											
										}break;
									}		

//									DBLOG_PRINTF("keepaccess cmCmd[%02X].\n", rptr_Z2W->dats.dats_conv.dats[8]);								

								}break;
							
								default:{}break;
							}
						
						}break;
						
						default:break;
					}
				
				}break;
				
				default:break;
			}
			
			if(dataReq_trigFlg){
			
				uartDataReq_len = tuyaApp_frameWifiLoad(uartDataReq_temp, uartWifiDataReq_paramTemp);
				uartDrvObj_local->Send(uartDataReq_temp, uartDataReq_len);
				osDelay(20);
				dataReq_trigFlg = false;
			}
			
			osPoolFree(threadDP_poolAttr_id, rptr_Z2W);
		}
		
		//系统功能消息触发
		evt = osMessageGet(mqID_wifiFuncRemind, 0);
		if((evt.status == osEventMessage)){
		
			stt_wifiFunMsg *rptr_wifiFunRm = evt.value.p;
			
			memset(&uartWifiDataReq_paramTemp, 0, sizeof(stt_tuyaWifiMoudle_frameParam));
			memset(uartDataReq_temp, 0, sizeof(uint8_t) * UARTWIFI_DATA_HANDLE_TEMP_LENGTH);
			
			switch(rptr_wifiFunRm->funcType){
			
				case msgFunWifi_localTimeReales:{
				
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_GET_LOCAL_TIME_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
				
				}break;
				
				case msgFunWifi_nwkSelfOpen:{
				
					stt_threadDatsPass *mptr_W2Z = (stt_threadDatsPass *)osPoolAlloc(threadDP_poolAttr_id);
					
					if(mptr_W2Z == NULL){
					
						DBLOG_PRINTF("mPool_threaDp null.\n");
					}
					
					mptr_W2Z->msgType = conventional;
					
					mptr_W2Z->dats.dats_conv.dats[0] 	= FRAME_HEAD_MOBILE;
					mptr_W2Z->dats.dats_conv.dats[1] 	= dataTransLength_objLOCAL;
					mptr_W2Z->dats.dats_conv.dats[2] 	= FRAME_TYPE_MtoS_CMD;
					mptr_W2Z->dats.dats_conv.dats[3] 	= FRAME_MtoSCMD_cmdConfigSearch;
					mptr_W2Z->dats.dats_conv.dats[4] 	= frame_Check(&mptr_W2Z->dats.dats_conv.dats[5], 28);
					
					mptr_W2Z->dats.dats_conv.datsLen = dataTransLength_objLOCAL;
					mptr_W2Z->dats.dats_conv.devType = DEVZIGB_DEFAULT;
					mptr_W2Z->dats.dats_conv.datsFrom = datsFrom_ctrlLocal;
					
					osMessagePut(mqID_threadDP_W2Z, (uint32_t)mptr_W2Z, 0);
					
					beeps_usrActive(4, 30, 2);
					
					DBLOG_PRINTF("zbNwk open\n");
					
				}break;
				
				case msgFunWifi_devRootHeartbeatTrig:{
					
					const uint8_t tuyaSubId_len = (5 + 1) * 2;
					const uint8_t tuyaDpKernel_tempLen = 16;
					uint8_t tuyaDpKernel_temp[16] = {0};
					uint8_t dataFrameLoad_ist = 0;
				
					//tuyaDp上报
					uartWifiDataReq_paramTemp.dataKernel[0] = tuyaSubId_len;
					sprintf((char *)&uartWifiDataReq_paramTemp.dataKernel[1], "%02X"MAC5STR, 
																			  SWITCH_TYPE,
																			  MAC5_2STR(devRootNode_subidMac));
					
					dataFrameLoad_ist += (1 + tuyaSubId_len);
					
					switch(SWITCH_TYPE){
					
						case SWITCH_TYPE_SWBIT1:{

							(status_actuatorRelay)?
								(tuyaDpKernel_temp[0] = 1):
								(tuyaDpKernel_temp[0] = 0);
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG1_DU_SWITCH,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
							tuyaDpKernel_temp[0] = (status_actuatorRelay >> 0) & 0x01;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG1_DU_BIT1,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
							
						}break;
							
						case SWITCH_TYPE_SWBIT2:{
						
							(status_actuatorRelay)?
								(tuyaDpKernel_temp[0] = 1):
								(tuyaDpKernel_temp[0] = 0);
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG2_DU_SWITCH,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	

							tuyaDpKernel_temp[0] = (status_actuatorRelay >> 0) & 0x01;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG2_DU_BIT1,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
							tuyaDpKernel_temp[0] = (status_actuatorRelay >> 1) & 0x01;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG2_DU_BIT2,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
						
						}break;
							
						case SWITCH_TYPE_SWBIT3:{
						
							(status_actuatorRelay)?
								(tuyaDpKernel_temp[0] = 1):
								(tuyaDpKernel_temp[0] = 0);
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG3_DU_SWITCH,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	

							tuyaDpKernel_temp[0] = (status_actuatorRelay >> 0) & 0x01;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG3_DU_BIT1,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
							tuyaDpKernel_temp[0] = (status_actuatorRelay >> 1) & 0x01;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG3_DU_BIT2,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
							tuyaDpKernel_temp[0] = (status_actuatorRelay >> 2) & 0x01;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_SWGANG3_DU_BIT3,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
						
						}break;
							
						case SWITCH_TYPE_CURTAIN:{
							
							bkLightColorInsert_paramAttr kColor_param = {0};
							
							devTipsParamGet_keyLightColor(&kColor_param);
						
							tuyaDpKernel_temp[0] = devCurtain_statusCodeChg_D2S(status_actuatorRelay & 0x07);
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_CURTAIN_DU_ACTION,
															dpType_enum,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
							tuyaDpKernel_temp[0 + 3] = curtainAct_Param.act_period; //轨道时间
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_CURTAIN_DU_ORBITAL_TIME,
															dpType_enum,
															tuyaDpKernel_temp,
															4);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);	
							tuyaDpKernel_temp[0] = devTipsParamGet_horsingLight();
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_CURTAIN_DU_AURORA,
															dpType_bool,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
							tuyaDpKernel_temp[0] = kColor_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_CURTAIN_DU_KCOLOR_PRE,
															dpType_enum,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
							tuyaDpKernel_temp[0] = kColor_param.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce;
							dataFrameLoad_ist += \
								tuyaApp_datapointFormatLoad(&uartWifiDataReq_paramTemp.dataKernel[dataFrameLoad_ist], 
															tuyaDP_DEV_CURTAIN_DU_KCOLOR_REA,
															dpType_enum,
															tuyaDpKernel_temp,
															1);
							memset(tuyaDpKernel_temp, 0, sizeof(uint8_t) * tuyaDpKernel_tempLen);
							
						}break;
						
						default:break;
					}
					
					uartWifiDataReq_paramTemp.frameCmd 		= tuyaF_DP_REPORT_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = dataFrameLoad_ist;
					
					dataReq_trigFlg = true;
				
				}break;
				
				case msgFunWifi_tuyaNodeRecordCheck:{

					uartWifiDataReq_paramTemp.frameCmd = tuyaF_CHILDDEV_CHILDCHECK_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
					
				}break;
				
				case msgFunWifi_nwkStatusGet:{
				
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_WIFI_STATE_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
					
				}break;
				
//				case msgFunWifi_workmodeGet:{}break;
				
				case msgFunWifi_nwkWifiReset:{
				
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_WIFI_RESET_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
				
				}break;
				
				case msgFunWifi_nwkCfg:{
				
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_WIFI_MODE_CMD;
					uartWifiDataReq_paramTemp.dataKernel[0] = nwkWifi_cfg_method;
					uartWifiDataReq_paramTemp.dataKernelLen = 1;
					
					dataReq_trigFlg = true;
				
				}break;
				
				default:break;
			}
			
			if(dataReq_trigFlg){
			
				uartDataReq_len = tuyaApp_frameWifiLoad(uartDataReq_temp, uartWifiDataReq_paramTemp);
				uartDrvObj_local->Send(uartDataReq_temp, uartDataReq_len);
				osDelay(20);
				dataReq_trigFlg = false;
			}
			
			osPoolFree(WifiFuncRemind_poolAttr_id, rptr_wifiFunRm);
		}
		
		//被动接收消息触发
		evt = osMessageGet(mqID_uartWifiToutDats_datatrans, 0);
		if((evt.status == osEventMessage)){
		
			stt_tuyaWifiMoudle_frameParam *rptr_uartWifiData = evt.value.p;
			
			memset(&uartWifiDataReq_paramTemp, 0, sizeof(stt_tuyaWifiMoudle_frameParam));
			memset(uartDataReq_temp, 0, sizeof(uint8_t) * UARTWIFI_DATA_HANDLE_TEMP_LENGTH);
			
			DBLOG_PRINTF("tuya uart data rcv[cmd:%02X].\n", rptr_uartWifiData->frameCmd);
			
			switch(rptr_uartWifiData->frameCmd){
			
				case tuyaF_HEAT_BEAT_CMD:{
					
					const char strSubid_kWord[10] = "\"sub_id\"";
					int kWordLoc_ist = 0;
					uint8_t handle_step = 0;
					uint8_t devType = 0;
					
					kWordLoc_ist = memloc(rptr_uartWifiData->dataKernel, rptr_uartWifiData->dataKernelLen, (uint8_t *)strSubid_kWord, 8);
					if(kWordLoc_ist > 0){
					
						uint8_t devMac_temp[5] = {0};
						
						handle_step |= (1 << 0);
						
						if(str2Mac5((char *)&(rptr_uartWifiData->dataKernel[kWordLoc_ist + 12]), devMac_temp)){
						
							extern nwkStateAttr_Zigb *zigbDevList_Head;
							extern nwkStateAttr_Zigb *zigbDev_eptPutout_BYpsy(nwkStateAttr_Zigb *pHead,u8 macAddr[DEVMAC_LEN], u8 devType,bool method);
							extern osPoolId (ZigbnwkState_poolAttr_id); 
							
							str2devType(&(rptr_uartWifiData->dataKernel[kWordLoc_ist + 10]), &devType);
							nwkStateAttr_Zigb *infoZigbDevRet_temp = zigbDev_eptPutout_BYpsy(zigbDevList_Head, devMac_temp, devType, false);
							
							handle_step |= (1 << 1);
							
							if(infoZigbDevRet_temp){
								
								handle_step |= (1 << 2);
							
								uartWifiDataReq_paramTemp.frameCmd = tuyaF_HEAT_BEAT_CMD;
								sprintf((char *)uartWifiDataReq_paramTemp.dataKernel, "{\"sub_id\":\""MAC5STR"\",\"hb_time\":60}", 
																					  devType,
																					  MAC5_2STR(devMac_temp));
								uartWifiDataReq_paramTemp.dataKernelLen = strlen((const char *)uartWifiDataReq_paramTemp.dataKernel);
								
								dataReq_trigFlg = true;
								
								osPoolFree(ZigbnwkState_poolAttr_id, infoZigbDevRet_temp);
							}
							else
							{
								if(!memcmp(devRootNode_subidMac, devMac_temp, 5)){
								
									handle_step |= (1 << 2);
								
									uartWifiDataReq_paramTemp.frameCmd = tuyaF_HEAT_BEAT_CMD;
									sprintf((char *)uartWifiDataReq_paramTemp.dataKernel, "{\"sub_id\":\""MAC5STR"\",\"hb_time\":60}", 
																						  SWITCH_TYPE,
																						  MAC5_2STR(devRootNode_subidMac));
									uartWifiDataReq_paramTemp.dataKernelLen = strlen((const char *)uartWifiDataReq_paramTemp.dataKernel);
									
									dataReq_trigFlg = true;
								}
							}
						}
					}
					
//					DBLOG_PRINTF("hb_handle:%02X,loc:%d,%c\n", 
//								 handle_step,
//								 kWordLoc_ist,
//								 rptr_uartWifiData->dataKernel[11]);
					
				}break;
				
				case tuyaF_PRODUCT_INFO_CMD:{
					
//					const char *localDev_info = "{\"p\":\""PRODUCT_KEY_GATEWAY"\",\"v\":\""MCU_VER"\",\"m\":0"",\"cap\":7""}";
					const char *localDev_info = "{\"v\":\""MCU_VER"\",\"m\":0"",\"cap\":0""}";
					
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_PRODUCT_INFO_CMD;
					sprintf((char *)uartWifiDataReq_paramTemp.dataKernel, "%s", localDev_info);
					uartWifiDataReq_paramTemp.dataKernelLen = strlen(localDev_info);
					
					dataReq_trigFlg = true;
				
				}break;
					
				case tuyaF_WORK_MODE_CMD:{
				
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_WORK_MODE_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
					
				}break;
					
				case tuyaF_WIFI_STATE_CMD:{
					
					enum_wifiNwkStatus statusTemp = wifiNwkStatus_unKnown;
					
					switch(rptr_uartWifiData->dataKernel[0]){
						
						case 0:{statusTemp = wifiNwkStatus_cfg;}break;
						case 1:{statusTemp = wifiNwkStatus_ap;}break;
						case 2:{statusTemp = wifiNwkStatus_noConnect;}break;
						case 3:{statusTemp = wifiNwkStatus_routerConnect;}break;
						case 4:{statusTemp = wifiNwkStatus_cloudConnect;}break;
						case 5:{statusTemp = wifiNwkStatus_lowPower;}break;
					
						default:break;
					}
					usrApp_devWifiStatus_set(statusTemp);
					
					switch(statusTemp){
					
						case wifiNwkStatus_cloudConnect:{
						
							nwkInternetOnline_IF = true;
							
							if(devTips_status_get() == status_tipsAPFind){
							
								tips_statusChangeToNormal();
								usrApp_wifiNwkCfg_stop();
							}
						
						}break;
						
						default:{nwkInternetOnline_IF = false;}break;
					}
				
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_WIFI_STATE_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
				
				}break;
					
//				case tuyaF_WIFI_RESET_CMD
				
				case tuyaF_WIFI_MODE_CMD:{
				
					wifiCfg_triggerParam.funcEnable = 0;
				
				}break;
					
				case tuyaF_CHILDDEV_ADD_CMD:{
				
//					DBLOG_PRINTF("child dev addRes[%02X].\n", rptr_uartWifiData->dataKernel[6]);
					
					dataReq_trigFlg = false;
				
				}break;
				
				case tuyaF_CHILDDEV_ADD_CFM:{
				
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_CHILDDEV_ADD_CFM;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
					
				}break;
				
				case tuyaF_CHILDDEV_DEL_CMD:{
				
					wifiFunction_callFromThread(msgFunWifi_tuyaNodeRecordCheck, true);
					
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_CHILDDEV_DEL_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
				
				}break;
				
				case tuyaF_CHILDDEV_CHILDCHECK_CMD:{
				
					uint8_t loop = 0;
					uint8_t pagNodeNum_temp = 0;
					uint8_t dataBaseIst = 0;
					static uint8_t pagNodeNum_max = 0;
					struct pagRef{
					
						uint8_t packIst:7;
						uint8_t mulitPackIF:1;
					}childList_packRef = {0};

					memcpy(&childList_packRef, &rptr_uartWifiData->dataKernel[0], sizeof(struct pagRef)); 
					pagNodeNum_temp = rptr_uartWifiData->dataKernel[1];

//					DBLOG_PRINTF("pag ist:%d.\n", childList_packRef.packIst);
					
					if(childList_packRef.mulitPackIF){
						
						pagNodeNum_max = pagNodeNum_temp;
//						DBLOG_PRINTF("maxpag get:%d.\n", pagNodeNum_max);
					}
					
					dataBaseIst = pagNodeNum_max * childList_packRef.packIst;
					
					if(0 == childList_packRef.packIst){
						
						memset(tuyaNodeRecord_infoHandle, 0, sizeof(_sttTuyaNodeRecordDatsForm) * USRAPP_DEVNODE_MAX_SUM); //清虚表
					}
					
					for(loop = 0; loop < pagNodeNum_temp; loop ++){
					
						str2devType((char *)&rptr_uartWifiData->dataKernel[2 + (loop * 13) + 1], &tuyaNodeRecord_infoHandle[dataBaseIst + loop].nodeType);
						str2Mac5(	(char *)&rptr_uartWifiData->dataKernel[2 + (loop * 13) + 3],  tuyaNodeRecord_infoHandle[dataBaseIst + loop].nodeMac);	
					}
					
					if(!childList_packRef.mulitPackIF){
						
						tuyaNodeRecord_nodeSum = pagNodeNum_max * childList_packRef.packIst + pagNodeNum_temp;
						memset(tuyaNodeRecord_infoCfm, 0, sizeof(_sttTuyaNodeRecordDatsForm) * USRAPP_DEVNODE_MAX_SUM); //清实表
						memcpy(tuyaNodeRecord_infoCfm, tuyaNodeRecord_infoHandle, sizeof(_sttTuyaNodeRecordDatsForm) * tuyaNodeRecord_nodeSum);
						DBLOG_PRINTF("tuya nodeList record reales:%d.\n", tuyaNodeRecord_nodeSum);
						osDelay(20);
						{
							static bool printfOnce_flg = true;
							
							if(printfOnce_flg){
							
								printfOnce_flg = false;
								
								DBLOG_PRINTF("example<1>-MAC:["MAC5STR"]Type:{%02X},""example<2>-MAC:["MAC5STR"]Type:{%02X}.\n", 
											 MAC5_2STR(tuyaNodeRecord_infoHandle[30].nodeMac),
											 tuyaNodeRecord_infoHandle[30].nodeType,
											 MAC5_2STR(tuyaNodeRecord_infoHandle[30].nodeMac),
											 tuyaNodeRecord_infoHandle[30].nodeType);
							}
						}
					}
					
				}break;
				
				case tuyaF_RECOVERY_FACTORY:{
				
					usrApp_gateWayfuncNwkopen_terminate();
					memset(tuyaNodeRecord_infoCfm, 0, sizeof(_sttTuyaNodeRecordDatsForm) * USRAPP_DEVNODE_MAX_SUM);
				
				}break;
				
				case tuyaF_CTRL_DPCOME_CMD:{
					
					if(rptr_uartWifiData->dataKernelLen){
					
						extern nwkStateAttr_Zigb *zigbDev_eptPutout_BYpsy(nwkStateAttr_Zigb *pHead,u8 macAddr[DEVMAC_LEN],u8 devType,bool method);
						extern nwkStateAttr_Zigb *zigbDevList_Head;
						extern osPoolId  (ZigbnwkState_poolAttr_id);
						
						osStatus mqRes = osOK;
						
						/*数据缓存定义*/
						uint8_t  devMac_temp[5] = {0};
						uint8_t  devType_temp = 0;
						nwkStateAttr_Zigb *nodeDev_infoTemp = NULL;
						uint8_t  dpDn_idLen = rptr_uartWifiData->dataKernel[0];
						stt_devOpreatDataPonit devDp_keepaccessDataTemp = {0};
						
						uint16_t tayaDpDn_dpAllTotelLen = rptr_uartWifiData->dataKernelLen - (dpDn_idLen + 1);
						uint16_t tayaDpDn_dataLen = 0;
						uint16_t tayaDpDn_dataIst = 0;
						uint8_t  *tayaDpDn_dataPtr = NULL;
						uint8_t  frameTotelLen = 15 + sizeof(stt_devOpreatDataPonit) + 2;
						
						struct __sttFunc_tayaDpDn{
						
							uint8_t dataDpDn_dpId;
							uint8_t dataDpDn_dpType;
							uint8_t dataDpDn_dataLen[2];
						}tayaDpDn_dataFunc = {0};
						
						stt_threadDatsPass *mptr_W2Z = NULL;
					
						/*数据帧头填装*/
						str2Mac5((char *)&rptr_uartWifiData->dataKernel[3], devMac_temp);
						str2devType((char *)&rptr_uartWifiData->dataKernel[1], &devType_temp);
						
						if(!memcmp(devMac_temp, devRootNode_subidMac, 5)){
							
							beeps_usrActive(3, 25, 1); //tips
							
							do{
								
								tayaDpDn_dataIst += (dpDn_idLen + 1); //单位数据点帧头属性下标索引
								memcpy(&tayaDpDn_dataFunc, &rptr_uartWifiData->dataKernel[tayaDpDn_dataIst], sizeof(struct __sttFunc_tayaDpDn)); //帧头属性值加载
								tayaDpDn_dataIst += sizeof(struct __sttFunc_tayaDpDn); //实际数据下标索引
								tayaDpDn_dataPtr = &rptr_uartWifiData->dataKernel[tayaDpDn_dataIst]; //实际数据加载
								
								switch(devType_temp){
								
									case SWITCH_TYPE_SWBIT1:{
									
										switch(tayaDpDn_dataFunc.dataDpDn_dpId){
											
											case tuyaDP_DEV_SWGANG1_DU_SWITCH:{
												
												swCommand_fromUsr.actMethod = relay_OnOff;
												(tayaDpDn_dataPtr[0])?
													(swCommand_fromUsr.objRelay = 7):
													(swCommand_fromUsr.objRelay = 0);
												
											}break;
											
											case tuyaDP_DEV_SWGANG1_DU_BIT1:{
											
												swCommand_fromUsr.actMethod = relay_OnOff;
												swCommand_fromUsr.objRelay  = status_actuatorRelay & 0x06 | ((tayaDpDn_dataPtr[0] & 0x01) << 0);
												
											}break;
												
//											case tuyaDP_DEV_SWGANG1_DU_AURORA:{}break;
//											case tuyaDP_DEV_SWGANG1_DU_KCOLOR_PRE:{}break;
//											case tuyaDP_DEV_SWGANG1_DU_KCOLOR_REA:{}break;
										
											default:break;
										}
									
									}break;
										
									case SWITCH_TYPE_SWBIT2:{
									
										switch(tayaDpDn_dataFunc.dataDpDn_dpId){
										
											case tuyaDP_DEV_SWGANG2_DU_SWITCH:{
												
												swCommand_fromUsr.actMethod = relay_OnOff;
												(tayaDpDn_dataPtr[0])?
													(swCommand_fromUsr.objRelay = 7):
													(swCommand_fromUsr.objRelay = 0);
												
											}break;
											
											case tuyaDP_DEV_SWGANG2_DU_BIT1:{
											
												swCommand_fromUsr.actMethod = relay_OnOff;
												swCommand_fromUsr.objRelay  = status_actuatorRelay & 0x06 | ((tayaDpDn_dataPtr[0] & 0x01) << 0);
												
											}break;
											
											case tuyaDP_DEV_SWGANG2_DU_BIT2:{
											
												swCommand_fromUsr.actMethod = relay_OnOff;
												swCommand_fromUsr.objRelay  = status_actuatorRelay & 0x05 | ((tayaDpDn_dataPtr[0] & 0x01) << 1);
												
											}break;
												
//											case tuyaDP_DEV_SWGANG2_DU_AURORA:{}break;
//											case tuyaDP_DEV_SWGANG2_DU_KCOLOR_PRE:{}break;
//											case tuyaDP_DEV_SWGANG2_DU_KCOLOR_REA:{}break;
											
											default:break;
										}
										
									}break;
										
									case SWITCH_TYPE_SWBIT3:{
									
										switch(tayaDpDn_dataFunc.dataDpDn_dpId){
											
											case tuyaDP_DEV_SWGANG3_DU_SWITCH:{
												
												swCommand_fromUsr.actMethod = relay_OnOff;
												(tayaDpDn_dataPtr[0])?
													(swCommand_fromUsr.objRelay = 7):
													(swCommand_fromUsr.objRelay = 0);
												
											}break;
											
											case tuyaDP_DEV_SWGANG3_DU_BIT1:{
											
												swCommand_fromUsr.actMethod = relay_OnOff;
												swCommand_fromUsr.objRelay  = status_actuatorRelay & 0x06 | ((tayaDpDn_dataPtr[0] & 0x01) << 0);
												
											}break;
											
											case tuyaDP_DEV_SWGANG3_DU_BIT2:{
											
												swCommand_fromUsr.actMethod = relay_OnOff;
												swCommand_fromUsr.objRelay  = status_actuatorRelay & 0x05 | ((tayaDpDn_dataPtr[0] & 0x01) << 1);
												
											}break;
												
											case tuyaDP_DEV_SWGANG3_DU_BIT3:{
							
												swCommand_fromUsr.actMethod = relay_OnOff;
												swCommand_fromUsr.objRelay  = status_actuatorRelay & 0x03 | ((tayaDpDn_dataPtr[0] & 0x01) << 2);
												
											}break;
												
//											case tuyaDP_DEV_SWGANG3_DU_AURORA:{}break;
//											case tuyaDP_DEV_SWGANG3_DU_KCOLOR_PRE:{}break;
//											case tuyaDP_DEV_SWGANG3_DU_KCOLOR_REA:{}break;
												
											default:break;
										}
									
									}break;
										
									case SWITCH_TYPE_CURTAIN:{
									
										bkLightColorInsert_paramAttr kColor_temp = {0};
										
										devTipsParamGet_keyLightColor(&kColor_temp);
										
										switch(tayaDpDn_dataFunc.dataDpDn_dpId){
											
											case tuyaDP_DEV_CURTAIN_DU_ACTION:{
											
												swCommand_fromUsr.actMethod = relay_OnOff;
												swCommand_fromUsr.objRelay = devCurtain_statusCodeChg_S2D(tayaDpDn_dataPtr[0]);
												
											}break;
												
											case tuyaDP_DEV_CURTAIN_DU_ORBITAL_TIME:{
											
												curtainAct_Param.act_period = tayaDpDn_dataPtr[0 + 3];
											
											}break;
												
											case tuyaDP_DEV_CURTAIN_DU_AURORA:{
											
												devTipsParamSet_horsingLight(tayaDpDn_dataPtr[0]);
												
											}break;
												
											case tuyaDP_DEV_CURTAIN_DU_KCOLOR_PRE:{
											
												kColor_temp.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_press = tayaDpDn_dataPtr[0];
												devTipsParamSet_keyLightColor(&kColor_temp);
												
											}break;
												
											case tuyaDP_DEV_CURTAIN_DU_KCOLOR_REA:{
									
												kColor_temp.sw3bitIcurtain_BKlight_Param.cuertain_BKlight_Param.cuertain_BKlightColorInsert_bounce = tayaDpDn_dataPtr[0];
												devTipsParamSet_keyLightColor(&kColor_temp);
												
											}break;
											
											default:break;
										}	
									
									}break;
									
									default:break;
								}								
								
								tayaDpDn_dataLen = (tayaDpDn_dataFunc.dataDpDn_dataLen[0] << 8) | tayaDpDn_dataFunc.dataDpDn_dataLen[1];
								tayaDpDn_dataIst += tayaDpDn_dataLen; //数据下标越过当前实际数据长度，指向下一数据点属性描述头下标
		
							}while(tayaDpDn_dataIst < tayaDpDn_dpAllTotelLen);	
							
							osDelay(50);
							counter_devRootHbReport = 0;
							wifiFunction_callFromThread(msgFunWifi_devRootHeartbeatTrig, false); //提前触发心跳上报回复
							
							break;
						}
						
						/*从链表数据中读取涂鸦数据点相关记忆缓存*/
						nodeDev_infoTemp = zigbDev_eptPutout_BYpsy(zigbDevList_Head, devMac_temp, devType_temp, false);
						
						/*消息队列缓存申请*/
						mptr_W2Z = (stt_threadDatsPass *)osPoolAlloc(threadDP_poolAttr_id);
						
						if(mptr_W2Z == NULL){
						
							DBLOG_PRINTF("mPool_threaDp null.\n");
						}
						
						/*消息数据预填充*/
						mptr_W2Z->msgType = conventional;
						mptr_W2Z->dats.dats_conv.dats[0] = DTMODEKEEPACESS_FRAMEHEAD_ONLINE;
						mptr_W2Z->dats.dats_conv.dats[1] = frameTotelLen;
						memcpy(&mptr_W2Z->dats.dats_conv.dats[2], devMac_temp, 5);
						mptr_W2Z->dats.dats_conv.dats[8] = DTMODEKEEPACESS_FRAMECMD_ASR;
						mptr_W2Z->dats.dats_conv.dats[9] = devType_temp;
						
						do{ /*数据点相关消息数据填充*/
						
							tayaDpDn_dataIst += (dpDn_idLen + 1); //单位数据点帧头属性下标索引
							memcpy(&tayaDpDn_dataFunc, &rptr_uartWifiData->dataKernel[tayaDpDn_dataIst], sizeof(struct __sttFunc_tayaDpDn)); //帧头属性值加载
							tayaDpDn_dataIst += sizeof(struct __sttFunc_tayaDpDn); //实际数据下标索引
							tayaDpDn_dataPtr = &rptr_uartWifiData->dataKernel[tayaDpDn_dataIst]; //实际数据加载
							
							switch(devType_temp){
							
								case SWITCH_TYPE_SWBIT1:{
								
									switch(tayaDpDn_dataFunc.dataDpDn_dpId){
										
										case tuyaDP_DEV_SWGANG1_DU_SWITCH:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1;
											(tayaDpDn_dataPtr[0])?
												(devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 1):
												(devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 0);
										
										}break;
											
										case tuyaDP_DEV_SWGANG1_DU_BIT1:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = nodeDev_infoTemp->funcDataMem.swStatus & 0x06 | ((tayaDpDn_dataPtr[0] & 0x01) << 0);
										
										}break;
											
//										case tuyaDP_DEV_SWGANG1_DU_AURORA:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_horsingLight = 1; //时效置位
//											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_horsingLight = tayaDpDn_dataPtr[0] & 0x01; //数据装填
//											
//										}break;
//										
//										case tuyaDP_DEV_SWGANG1_DU_KCOLOR_PRE:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
//											
//											devDp_keepaccessDataTemp.devData_bkLight[0] = tayaDpDn_dataPtr[0];
//											devDp_keepaccessDataTemp.devData_bkLight[1] = nodeDev_infoTemp->funcDataMem.keyLightColor[1];
//											
//										}break;
//										
//										case tuyaDP_DEV_SWGANG1_DU_KCOLOR_REA:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
//											
//											devDp_keepaccessDataTemp.devData_bkLight[1] = tayaDpDn_dataPtr[0];
//											devDp_keepaccessDataTemp.devData_bkLight[0] = nodeDev_infoTemp->funcDataMem.keyLightColor[0];
//											
//										}break;
											
										default:break;
									}
								
								}break;
									
								case SWITCH_TYPE_SWBIT2:{
								
									switch(tayaDpDn_dataFunc.dataDpDn_dpId){
										
										case tuyaDP_DEV_SWGANG2_DU_SWITCH:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1;
											(tayaDpDn_dataPtr[0])?
												(devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 3):
												(devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 0);
										
										}break;
										
										case tuyaDP_DEV_SWGANG2_DU_BIT1:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = nodeDev_infoTemp->funcDataMem.swStatus & 0x06 | ((tayaDpDn_dataPtr[0] & 0x01) << 0);
										
										}break;
											
										case tuyaDP_DEV_SWGANG2_DU_BIT2:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = nodeDev_infoTemp->funcDataMem.swStatus & 0x05 | ((tayaDpDn_dataPtr[0] & 0x01) << 1);
											
										}break;
											
//										case tuyaDP_DEV_SWGANG2_DU_AURORA:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_horsingLight = 1; //时效置位
//											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_horsingLight = tayaDpDn_dataPtr[0] & 0x01; //数据装填
//											
//										}break;		
//											
//										case tuyaDP_DEV_SWGANG2_DU_KCOLOR_PRE:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
//											
//											devDp_keepaccessDataTemp.devData_bkLight[0] = tayaDpDn_dataPtr[0];
//											devDp_keepaccessDataTemp.devData_bkLight[1] = nodeDev_infoTemp->funcDataMem.keyLightColor[1];
//											
//										}break;	 
//											
//										case tuyaDP_DEV_SWGANG2_DU_KCOLOR_REA:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
//											
//											devDp_keepaccessDataTemp.devData_bkLight[1] = tayaDpDn_dataPtr[0];
//											devDp_keepaccessDataTemp.devData_bkLight[0] = nodeDev_infoTemp->funcDataMem.keyLightColor[0];
//											
//										}break;
										
										default:break;
									}
								
								}break;
									
								case SWITCH_TYPE_SWBIT3:{
								
									switch(tayaDpDn_dataFunc.dataDpDn_dpId){
											
										case tuyaDP_DEV_SWGANG3_DU_SWITCH:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1;
											(tayaDpDn_dataPtr[0])?
												(devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 7):
												(devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 0);
										
										}break;
										
										case tuyaDP_DEV_SWGANG3_DU_BIT1:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = nodeDev_infoTemp->funcDataMem.swStatus & 0x06 | ((tayaDpDn_dataPtr[0] & 0x01) << 0);
										
										}break;
											
										case tuyaDP_DEV_SWGANG3_DU_BIT2:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = nodeDev_infoTemp->funcDataMem.swStatus & 0x05 | ((tayaDpDn_dataPtr[0] & 0x01) << 1);
											
										}break;
											
										case tuyaDP_DEV_SWGANG3_DU_BIT3:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = nodeDev_infoTemp->funcDataMem.swStatus & 0x03 | ((tayaDpDn_dataPtr[0] & 0x01) << 2);
											
										}break;
											
//										case tuyaDP_DEV_SWGANG3_DU_AURORA:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_horsingLight = 1; //时效置位
//											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_horsingLight = tayaDpDn_dataPtr[0] & 0x01; //数据装填
//											
//										}break;		
//											
//										case tuyaDP_DEV_SWGANG3_DU_KCOLOR_PRE:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
//											
//											devDp_keepaccessDataTemp.devData_bkLight[0] = tayaDpDn_dataPtr[0];
//											devDp_keepaccessDataTemp.devData_bkLight[1] = nodeDev_infoTemp->funcDataMem.keyLightColor[1];
//											
//										}break;		 
//											
//										case tuyaDP_DEV_SWGANG3_DU_KCOLOR_REA:{
//										
//											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
//											
//											devDp_keepaccessDataTemp.devData_bkLight[1] = tayaDpDn_dataPtr[0];
//											devDp_keepaccessDataTemp.devData_bkLight[0] = nodeDev_infoTemp->funcDataMem.keyLightColor[0];
//											
//										}break;
										
										default:break;
									}									
								
								}break;
									
								case SWITCH_TYPE_SOCKETS:{
								
									switch(tayaDpDn_dataFunc.dataDpDn_dpId){

										case tuyaDP_DEV_SOCKET_DU_SWITCH:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = nodeDev_infoTemp->funcDataMem.swStatus & 0x06 | ((tayaDpDn_dataPtr[0] & 0x01) << 0);
										
										}break;
										
//										case tuyaDP_DEV_SOCKET_UU_POWER:{}break;
										
										default:break;
									}									
									
								}break;
									
								case SWITCH_TYPE_CURTAIN:{

									switch(tayaDpDn_dataFunc.dataDpDn_dpId){
										
										case tuyaDP_DEV_CURTAIN_DU_ACTION:{
											
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = devCurtain_statusCodeChg_S2D(tayaDpDn_dataPtr[0]);
										
										}break;
										
										case tuyaDP_DEV_CURTAIN_DU_ORBITAL_TIME:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_curtainOpPeriodSetOpreat = 1;
											devDp_keepaccessDataTemp.union_devParam.curtain_param.orbital_Period = tayaDpDn_dataPtr[0 + 3];
										
										}break;
											
										case tuyaDP_DEV_CURTAIN_DU_AURORA:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_horsingLight = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_horsingLight = tayaDpDn_dataPtr[0] & 0x01; //数据装填
											
										}break;	
										
										case tuyaDP_DEV_CURTAIN_DU_KCOLOR_PRE:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											devDp_keepaccessDataTemp.devData_bkLight[0] = tayaDpDn_dataPtr[0];
											devDp_keepaccessDataTemp.devData_bkLight[1] = nodeDev_infoTemp->funcDataMem.keyLightColor[1];
											
										}break;	
											
										case tuyaDP_DEV_CURTAIN_DU_KCOLOR_REA:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											devDp_keepaccessDataTemp.devData_bkLight[1] = tayaDpDn_dataPtr[0];
											devDp_keepaccessDataTemp.devData_bkLight[0] = nodeDev_infoTemp->funcDataMem.keyLightColor[0];
											
										}break;
										
										default:break;
									}									
								
								}break;
									
								case SWITCH_TYPE_FANS:{

									switch(tayaDpDn_dataFunc.dataDpDn_dpId){
											
										case tuyaDP_DEV_FANS_DU_SWITCH:{
											
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											
											if(tayaDpDn_dataPtr[0]){
											
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 3;
											}
											else
											{
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 0;
											}
																
										}break;
										
										case tuyaDP_DEV_FANS_DU_GEAR:{
											
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = tayaDpDn_dataPtr[0 + 3];
										
										}break;
											
										case tuyaDP_DEV_FANS_DU_AURORA:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_horsingLight = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_horsingLight = tayaDpDn_dataPtr[0] & 0x01; //数据装填
											
										}break;	
											
										case tuyaDP_DEV_FANS_DU_TCOLOR_OFF:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[0] &= 0xf0;
											devDp_keepaccessDataTemp.devData_bkLight[0] |= ((tayaDpDn_dataPtr[0] << 0) & 0x0f);
										
										}break;	 
											
										case tuyaDP_DEV_FANS_DU_TCOLOR_GEAR1:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[0] &= 0x0f;
											devDp_keepaccessDataTemp.devData_bkLight[0] |= ((tayaDpDn_dataPtr[0] << 4) & 0xf0);
										
										}break;
											
										case tuyaDP_DEV_FANS_DU_TCOLOR_GEAR2:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[1] &= 0xf0;
											devDp_keepaccessDataTemp.devData_bkLight[1] |= ((tayaDpDn_dataPtr[0] << 0) & 0x0f);
											
										}break;
											
										case tuyaDP_DEV_FANS_DU_TCOLOR_GEAR3:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[1] &= 0x0f;
											devDp_keepaccessDataTemp.devData_bkLight[1] |= ((tayaDpDn_dataPtr[0] << 4) & 0xf0);
											
										}break;
										
										default:break;
									}									
								
								}break;	
									
								case SWITCH_TYPE_HEATER:{

									switch(tayaDpDn_dataFunc.dataDpDn_dpId){
											
										case tuyaDP_DEV_HEATER_DU_SWITCH:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = tayaDpDn_dataPtr[0];
											
										}break;
											
										case tuyaDP_DEV_HEATER_DU_AURORA:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_horsingLight = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_horsingLight = tayaDpDn_dataPtr[0] & 0x01; //数据装填
											
										}break;		
										
										default:break;
									}									
								
								}break;
									
								case SWITCH_TYPE_DIMMER:{

									switch(tayaDpDn_dataFunc.dataDpDn_dpId){
											
										case tuyaDP_DEV_DIMMER_DU_SWITCH:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											
											if(tayaDpDn_dataPtr[0]){
											
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 0xff; //数据装填
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_reserve 	= 0xff;
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swPush 	= 0xff;
											}
											else
											{
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = 0; //数据装填
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_reserve 	= 0;
												devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swPush 	= 0;
											}
										
										}break;
										
										case tuyaDP_DEV_DIMMER_DU_BRIGHTNESS:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_swOpreat = 1; //时效置位
											
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swStatus = ((tayaDpDn_dataPtr[0 + 3] & 0x07) >> 0); //数据装填
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_reserve 	= ((tayaDpDn_dataPtr[0 + 3] & 0x18) >> 3);
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_swPush 	= ((tayaDpDn_dataPtr[0 + 3] & 0xE0) >> 5);
											
										}break;
											
										case tuyaDP_DEV_DIMMER_DU_AURORA:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_horsingLight = 1; //时效置位
											devDp_keepaccessDataTemp.devStatus_Reference.statusRef_horsingLight = tayaDpDn_dataPtr[0] & 0x01; //数据装填
											
										}break;	
											
										case tuyaDP_DEV_DIMMER_DU_KCOLOR_TRIG:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[0] &= 0xf0;
											devDp_keepaccessDataTemp.devData_bkLight[0] |= ((tayaDpDn_dataPtr[0] << 0) & 0x0f);
										
										}break;	 
											
										case tuyaDP_DEV_DIMMER_DU_TCOLOR_BMIN:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[0] &= 0x0f;
											devDp_keepaccessDataTemp.devData_bkLight[0] |= ((tayaDpDn_dataPtr[0] << 4) & 0xf0);
										
										}break;
											
										case tuyaDP_DEV_DIMMER_DU_TCOLOR_BNOR:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[1] &= 0xf0;
											devDp_keepaccessDataTemp.devData_bkLight[1] |= ((tayaDpDn_dataPtr[0] << 0) & 0x0f);
											
										}break;
											
										case tuyaDP_DEV_DIMMER_DU_TCOLOR_BMAX:{
										
											devDp_keepaccessDataTemp.devAgingOpreat_agingReference.agingCmd_bkLightSetOpreat = 1; //时效置位
											
											memcpy(&devDp_keepaccessDataTemp.devData_bkLight[0], nodeDev_infoTemp->funcDataMem.keyLightColor, sizeof(uint8_t) * 2);
											devDp_keepaccessDataTemp.devData_bkLight[1] &= 0x0f;
											devDp_keepaccessDataTemp.devData_bkLight[1] |= ((tayaDpDn_dataPtr[0] << 4) & 0xf0);
											
										}break;
										
										default:break;
									}									
								
								}break;
									
								default:break;
							}
							
							tayaDpDn_dataLen = (tayaDpDn_dataFunc.dataDpDn_dataLen[0] << 8) | tayaDpDn_dataFunc.dataDpDn_dataLen[1];
							tayaDpDn_dataIst += tayaDpDn_dataLen; //数据下标越过当前实际数据长度，指向下一数据点属性描述头下标
						
						}while(tayaDpDn_dataIst < tayaDpDn_dpAllTotelLen);
						
						memcpy(&mptr_W2Z->dats.dats_conv.dats[15], &devDp_keepaccessDataTemp, sizeof(stt_devOpreatDataPonit));
						mptr_W2Z->dats.dats_conv.dats[frameTotelLen - 2] = systemTime_current.time_Second;//随机数为当前秒
						mptr_W2Z->dats.dats_conv.dats[frameTotelLen - 1] = \
							frame_Check(&mptr_W2Z->dats.dats_conv.dats[1], frameTotelLen - 2);
						
						memcpy(mptr_W2Z->dats.dats_conv.macAddr, devMac_temp, 5);
						mptr_W2Z->dats.dats_conv.datsLen = frameTotelLen;
						mptr_W2Z->dats.dats_conv.devType = devType_temp;
						mptr_W2Z->dats.dats_conv.datsFrom = datsFrom_ctrlRemote;

						mqRes = osMessagePut(mqID_threadDP_W2Z, (uint32_t)mptr_W2Z, 0); //keepaccess回复
						if(mqRes){
						
							DBLOG_PRINTF("W2Z mq.1 txFailRes:%08X.\n", mqRes); //keepaccess回复
						}
						else
						{
							
						}
						
						if(nodeDev_infoTemp){
						
							osPoolFree(ZigbnwkState_poolAttr_id, nodeDev_infoTemp);
						}
					}
					
				}break;
				
				case tuyaF_NWK_OPEN_CMD:{
				
					usrApp_gateWayfuncNwkopen_opreat(true);
					
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_NWK_OPEN_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					if(!zigbNwkReserveNodeNum_currentValue){
						
						loopCountRecord_searchResp = devType_sum;
					}
					
					dataReq_trigFlg = true;
					
				}break;
					
				case tuyaF_NWK_CLOSE_CMD:{
				
					usrApp_gateWayfuncNwkopen_opreat(false);
					
					uartWifiDataReq_paramTemp.frameCmd = tuyaF_NWK_CLOSE_CMD;
					uartWifiDataReq_paramTemp.dataKernelLen = 0;
					
					dataReq_trigFlg = true;
				
				}break;
					
//				case tuyaF_UPDATE_START_CMD
//				case tuyaF_UPDATE_TRANS_CMD    
//				case tuyaF_GET_ONLINE_TIME_CMD
//				case tuyaF_FACTORY_MODE_CMD
//				case tuyaF_WIFI_TEST_CMD   
				
				case tuyaF_GET_LOCAL_TIME_CMD:{
				
					if(rptr_uartWifiData->dataKernel[0]){ //时间获取成功
					
						extern bool zigB_sysTimeSet_detailedFormatTuya(unsigned char timeParam[7]);
						
						zigB_sysTimeSet_detailedFormatTuya(&(rptr_uartWifiData->dataKernel[1]));
						
						if(!nwkInternetOnline_IF)
							nwkInternetOnline_IF = true;
					}
					
					dataReq_trigFlg = false;
					
				}break;
					
//				case tuyaF_WEATHER_OPEN_CMD    
//				case tuyaF_WEATHER_DATA_CMD   
//				case tuyaF_HEAT_BEAT_STOP  
//				case tuyaF_STREAM_OPEN_CMD 
//				case tuyaF_STREAM_START_CMD 
//				case tuyaF_STREAM_TRANS_CMD 
//				case tuyaF_STREAM_STOP_CMD 
				
				default:break;
			}
			
			if(dataReq_trigFlg){
			
				uartDataReq_len = tuyaApp_frameWifiLoad(uartDataReq_temp, uartWifiDataReq_paramTemp);
				uartDrvObj_local->Send(uartDataReq_temp, uartDataReq_len);
				osDelay(20);
				dataReq_trigFlg = false;
			}
			
			osPoolFree(WifiUartMsgDatatrans_poolAttr_id, rptr_uartWifiData);
		}	
	}
}
