#ifndef __DATSPROCESS_UARTZIGBEE_H__
#define __DATSPROCESS_UARTZIGBEE_H__

#include "dataManage.h"
#include "usrAppMethod.h"

#include "UART_dataTransfer.h"

#define UARTCON_ZIGB_DATA_RCV_TEMP_LENGTH		(256 * 6)

#define ZIGB_UTCTIME_START	946684800UL //

#define WIFI_FRAME_HEAD		0x7F
#define ZIGB_FRAME_HEAD		0xFE

#define DEVZIGB_DEFAULT			0x33	//链表对应默认设备类型

#define DTMODEKEEPACESS_FRAMEHEAD_ONLINE	0xFA	//
#define DTMODEKEEPACESS_FRAMEHEAD_OFFLINE	0xFB	//
#define	DTMODEKEEPACESS_FRAMECMD_ASR		0xA1	//
#define	DTMODEKEEPACESS_FRAMECMD_PST		0xA2	//

#define ZIGB_FRAMEHEAD_CTRLLOCAL		0xAA //
#define ZIGB_FRAMEHEAD_CTRLREMOTE		0xCC //
#define ZIGB_FRAMEHEAD_HEARTBEAT		0xAB //
#define ZIGB_OFFLINEFRAMEHEAD_HEARTBEAT	0xBB //

#define STATUSLOCALEACTRL_VALMASKRESERVE_ON		0x0A //
#define STATUSLOCALEACTRL_VALMASKRESERVE_OFF	0x0B //

#define CTRLEATHER_PORT_NUMSTART		0x10 //
#define CTRLEATHER_PORT_NUMTAIL			0xFF //

#define CTRLSECENARIO_RESPCMD_SPECIAL	0xCE //

#define ZIGB_ENDPOINT_CTRLSECENARIO		12 //
#define ZIGB_ENDPOINT_CTRLNORMAL		13 //
#define ZIGB_ENDPOINT_CTRLSYSZIGB		14 //

#define ZIGB_CLUSTER_DEFAULT_DEVID		13
#define ZIGB_CLUSTER_DEFAULT_CULSTERID	13

#define ZIGBPANID_CURRENT_REALESPERIOD	60 //

#define ZIGBNWKOPENTIME_DEFAULT	15 //

#define ZIGB_PANID_MAXVAL     	0x3FFF //

#define ZIGB_SYSCMD_NWKOPEN					0x68 //zigb
#define ZIGB_SYSCMD_TIMESET					0x69 //zigb
#define ZIGB_SYSCMD_DEVHOLD					0x6A //zigb
#define ZIGB_SYSCMD_EACHCTRL_REPORT			0x6B //zigb
#define ZIGB_SYSCMD_COLONYPARAM_REQPERIOD	0x6C //zigb
#define ZIGB_SYSCMD_DATATRANS_HOLD			0x6D //zigb

//#define zigB_remoteDataTransASY_QbuffLen 		40  //
#define zigB_remoteDataTransASY_QbuffLen 		(USRAPP_DEVNODE_MAX_SUM / 2 * 3) //异步发送队列长度，直接等于节点限制数 * 2
#define zigB_remoteDataTransASY_txPeriod		200 //
#define zigB_remoteDataTransASY_txReapt			5	//
#define zigB_remoteDataTransASY_txUartOnceWait	25	//

#define zigB_reconnectCauseDataReqFailLoop		3	//

#define zigB_ScenarioCtrlDataTransASY_txBatchs_EN			0	//
#define zigB_ScenarioCtrlDataTransASY_opreatOnceNum			10	//

#define zigB_ScenarioCtrlDataTransASY_timeRoundPause		0	//

#define UARTZIGB_DATA_HANDLE_TEMP_LENGTH	(256 * 2)

#define ZIGB_HARDWARE_RESET_LEVEL			GPIO_PIN_SET
#define ZIGB_HARDWARE_NORMAL_LEVEL			GPIO_PIN_RESET

#define ZIGB_MOUDLE_RST_OPT(x)			    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, x)

typedef enum{

	msgFunZigb_null = 0,
	msgFunZigb_nwkOpen, //
	msgFunZigb_rootSystimeReales,
	msgFunZigb_nodeSystimeSynchronous, //
	msgFunZigb_localSystimeZigbAdjust, //
	msgFunZigb_portCtrlEachoRegister, //
	msgFunZigb_panidRealesNwkCreat, //
	msgFunZigb_scenarioCrtl, //
	msgFunZigb_dtPeriodHoldPst, //
	msgFunZigb_dtPeriodHoldCancelAdvance,  //
}enum_zigbFunMsg; //

typedef struct{

	enum_zigbFunMsg funcType;
	
}stt_zigbFunMsg;

typedef struct{

	u16 deviveID; //ZNP
	u8  endPoint; //ZNP
}devDatsTrans_portAttr;

typedef struct{

	u8 dats[32];
	u8 datsLen;
}sttUartZigbRcv_sysDat; //

typedef struct{

	u8 dats[128 + 25]; //
	u8 datsLen; //
}sttUartZigbRcv_rmoteDatComming; //

typedef struct{

	u8 dats[16];
	u8 datsLen;
}sttUartZigbRcv_rmDatsReqResp; //

typedef struct{

	u8 dats[32];
	u8 datsLen;
}sttUartRcv_sysDat;

typedef struct{

	u8 cmdResp[2];
	u8 frameResp[96];
	u8 frameRespLen;
}datsZigb_reqGet;

typedef struct{

	u8 command; //
	u8 dats[32]; //
	u8 datsLen; //
}frame_zigbSysCtrl;

typedef struct{

	u16 keepTxUntilCmp_IF:1; //
	u16 datsTxKeep_Period:15; //
}remoteDataReq_method;

#define dataRemote_RESPLEN 8
typedef struct{

	u8 	dataReq[128 + 25];
	u8 	dataReq_Len;
	u8 	dataResp[dataRemote_RESPLEN];
	u8 	dataResp_Len;

	u16 dataReqPeriod;	//
	u8 	repeat_Loop;	//
}stt_dataRemoteReq;

void zigbFunction_callFromThread(enum_zigbFunMsg funcType);
void ZigB_mainThread(type_uart_OBJ UART_OBJ);

void zigbFunction_callFromThread(enum_zigbFunMsg funcType);

#endif
