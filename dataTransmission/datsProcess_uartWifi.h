#ifndef __DATSPROCESS_UARTWIFI_H__
#define __DATSPROCESS_UARTWIFI_H__

#include "dataManage.h"
#include "usrAppMethod.h"

#include "UART_dataTransfer.h"

#define FRAME_HEAD_MOBILE		0xAA
#define FRAME_HEAD_SERVER		0xCC
#define FRAME_HEAD_HEARTB		0xAA

#define DATATRANS_WORKMODE_HEARTBEAT		0x0A //
#define DATATRANS_WORKMODE_KEEPACESS		0x0B //
#define ZIGB_DATATRANS_WORKMODE				DATATRANS_WORKMODE_KEEPACESS //

#define timer_heartBeatKeep_Period			1000UL

#if(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_HEARTBEAT) //
	#define PERIOD_HEARTBEAT_ASR		8000UL  	//
#elif(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_KEEPACESS)
	#define PERIOD_HEARTBEAT_ASR		20000UL  	//
	#define PERIOD_HEARTBEAT_PST		2000UL		//
#endif

#define INTERNET_SERVERPORT_SWITCHPERIOD	(u8)(PERIOD_HEARTBEAT_ASR / 1000UL * 3UL)

#define dataTransLength_objLOCAL			33
#define dataTransLength_objREMOTE			45
#define dataHeartBeatLength_objSERVER		96

#define FRAME_TYPE_MtoS_CMD					0xA0	
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	
//#define FRAME_TYPE_MtoZB_CMD				0xA1	
//#define FRAME_TYPE_ZBtoM_CMD				0x1A	
#define FRAME_TYPE_StoM_RCVfail				0x0C
#define FRAME_TYPE_StoM_upLoad				0x0D
#define FRAME_TYPE_StoM_reaptSMK			0x0E

#define FRAME_HEARTBEAT_cmdOdd				0x23	
#define FRAME_HEARTBEAT_cmdEven				0x22	

#define FRAME_MtoSCMD_cmdControl			0x10	
#define FRAME_MtoSCMD_cmdConfigSearch		0x39	
#define FRAME_MtoSCMD_cmdQuery				0x11	
#define FRAME_MtoSCMD_cmdInterface			0x15	
#define FRAME_MtoSCMD_cmdReset				0x16	
#define FRAME_MtoSCMD_cmdLockON				0x17	
#define FRAME_MtoSCMD_cmdLockOFF			0x18	
#define FRAME_MtoSCMD_cmdswTimQuery			0x19	
#define FRAME_MtoSCMD_cmdConfigAP			0x50	
#define FRAME_MtoSCMD_cmdBeepsON			0x1A	
#define FRAME_MtoSCMD_cmdBeepsOFF			0x1B	
#define FRAME_MtoSCMD_cmdftRecoverRQ		0x22	
#define FRAME_MtoSCMD_cmdRecoverFactory		0x1F	
#define FRAME_MtoSCMD_cmdCfg_swTim			0x14	
#define FRAME_MtoZIGBCMD_cmdCfg_PANID		0x40	
#define FRAME_MtoZIGBCMD_cmdCfg_ctrlEachO	0x41	
#define FRAME_MtoZIGBCMD_cmdQue_ctrlEachO	0x42	
#define FRAME_MtoZIGBCMD_cmdCfg_ledBackSet	0x43	
#define FRAME_MtoZIGBCMD_cmdQue_ledBackSet	0x44	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioSet	0x45	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioCtl	0x47	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioDel	0x48	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioReg 0x50	

#define	cmdConfigTim_normalSwConfig			0xA0	
#define cmdConfigTim_onoffDelaySwConfig		0xA1	
#define cmdConfigTim_closeLoopSwConfig		0xA2	
#define cmdConfigTim_nightModeSwConfig		0xA3	

//tuya define

#define 		PRODUCT_KEY_GATEWAY "4n8avjbtnmn2izo3"    //
#define 		PRODUCT_KEY_SWBIT3	"zg5vvygwewbkasv4"
#define 		PRODUCT_KEY_SWBIT2	"oel94ldrn6xyfmyo"
#define 		PRODUCT_KEY_SWBIT1	"jikkqro4orqpjnrc"
#define 		PRODUCT_KEY_DIMMER	"y09fi1jxkfke9qed"
#define 		PRODUCT_KEY_CURTAIN	"qauxlazvi1a97kfx"
#define 		PRODUCT_KEY_HEATER	"s4dqxlbae8ziapsl"
#define 		PRODUCT_KEY_SOCKET	"iugddgzsfhm6zuyg"
#define 		PRODUCT_KEY_FANS	"62vhqnk99omjlkie"

#define 		MCU_VER "1.0.0"                   //

#define 		MCU_CAP	"7"

#define			tuyaF_FRAMESHELL_DATALEN			  7

#define			tuyaF_FRAME_HEAD_A					  0x55
#define			tuyaF_FRAME_HEAD_B					  0xAA
#define			tuyaF_FRAME_HEAD_VER				  0

#define         tuyaF_PRODUCT_INFO_CMD                0x01                            //
#define         tuyaF_WORK_MODE_CMD                   0x02                            //
#define         tuyaF_WIFI_STATE_CMD                  0x03                            //
#define         tuyaF_WIFI_RESET_CMD                  0x04                            //
#define         tuyaF_WIFI_MODE_CMD                   0x05                            //
#define         tuyaF_NWK_OPEN_CMD                    0x06                            //
#define         tuyaF_NWK_CLOSE_CMD                   0x07                            // 
#define         tuyaF_CHILDDEV_ADD_CMD                0x08                            //
#define         tuyaF_CHILDDEV_DEL_CMD				  0x09 
#define         tuyaF_HEAT_BEAT_CMD                	  0x0a                            //
#define         tuyaF_CTRL_DPCOME_CMD				  0x0c
#define         tuyaF_DP_REPORT_CMD					  0x0d
#define         tuyaF_GET_LOCAL_TIME_CMD              0x11
#define         tuyaF_CHILDDEV_MULITADD_CMD           0x12
#define         tuyaF_CHILDDEV_ADD_CFM				  0x13
#define         tuyaF_RECOVERY_FACTORY				  0x17
#define         tuyaF_CHILDDEV_CHILDCHECK_CMD	  	  0x1c


/*数据点 -集合*/
#define			tuyaDP_DEV_SWGANG1_DU_SWITCH			13
#define			tuyaDP_DEV_SWGANG1_DU_BIT1				1
#define			tuyaDP_DEV_SWGANG1_DU_AURORA			103
#define			tuyaDP_DEV_SWGANG1_DU_KCOLOR_PRE		101
#define			tuyaDP_DEV_SWGANG1_DU_KCOLOR_REA		102

#define			tuyaDP_DEV_SWGANG2_DU_SWITCH			13
#define			tuyaDP_DEV_SWGANG2_DU_BIT1				1
#define			tuyaDP_DEV_SWGANG2_DU_BIT2				2
#define			tuyaDP_DEV_SWGANG2_DU_AURORA			103
#define			tuyaDP_DEV_SWGANG2_DU_KCOLOR_PRE		101
#define			tuyaDP_DEV_SWGANG2_DU_KCOLOR_REA		102

#define			tuyaDP_DEV_SWGANG3_DU_SWITCH			13
#define			tuyaDP_DEV_SWGANG3_DU_BIT1				1
#define			tuyaDP_DEV_SWGANG3_DU_BIT2				2
#define			tuyaDP_DEV_SWGANG3_DU_BIT3				3
#define			tuyaDP_DEV_SWGANG3_DU_AURORA			103
#define			tuyaDP_DEV_SWGANG3_DU_KCOLOR_PRE		101
#define			tuyaDP_DEV_SWGANG3_DU_KCOLOR_REA		102

#define			tuyaDP_DEV_CURTAIN_DU_ACTION			1
#define			tuyaDP_DEV_CURTAIN_DU_ORBITAL_TIME		9
#define			tuyaDP_DEV_CURTAIN_DU_AURORA			103
#define			tuyaDP_DEV_CURTAIN_DU_KCOLOR_PRE		101
#define			tuyaDP_DEV_CURTAIN_DU_KCOLOR_REA		102

#define			tuyaDP_DEV_DIMMER_DU_SWITCH				1
#define			tuyaDP_DEV_DIMMER_DU_BRIGHTNESS			2
#define			tuyaDP_DEV_DIMMER_DU_AURORA				105
#define			tuyaDP_DEV_DIMMER_DU_KCOLOR_TRIG		104
#define			tuyaDP_DEV_DIMMER_DU_TCOLOR_BMIN		101
#define			tuyaDP_DEV_DIMMER_DU_TCOLOR_BNOR		102
#define			tuyaDP_DEV_DIMMER_DU_TCOLOR_BMAX		103

#define			tuyaDP_DEV_FANS_DU_SWITCH				1
#define			tuyaDP_DEV_FANS_DU_GEAR					3
#define			tuyaDP_DEV_FANS_DU_AURORA				105
#define			tuyaDP_DEV_FANS_DU_TCOLOR_OFF			101
#define			tuyaDP_DEV_FANS_DU_TCOLOR_GEAR1			102
#define			tuyaDP_DEV_FANS_DU_TCOLOR_GEAR2			103
#define			tuyaDP_DEV_FANS_DU_TCOLOR_GEAR3			104

#define			tuyaDP_DEV_HEATER_DU_SWITCH				1
#define			tuyaDP_DEV_HEATER_DU_AURORA				101

#define			tuyaDP_DEV_SOCKET_DU_SWITCH				1
#define			tuyaDP_DEV_SOCKET_UU_POWER				19

#define 		UARTWIFI_DATA_HANDLE_TEMP_LENGTH	  896  //大缓存用于应对设备列表数据获取业务，太大无意义，因为队列要跟着变大会加大资源消耗

#define 		HWCON_WIFI_MOUDLE_PIN_RST			  DataTransP3_RST

typedef struct{

	uint8_t  frameCmd;
	uint8_t  dataKernel[768 - tuyaF_FRAMESHELL_DATALEN];
	uint16_t dataKernelLen;
}stt_tuyaWifiMoudle_frameParam;

typedef enum{

	msgFunWifi_null = 0,
	msgFunWifi_localTimeReales,
	msgFunWifi_devRootHeartbeatTrig,
	msgFunWifi_nwkStatusGet,
	msgFunWifi_workmodeGet,
	msgFunWifi_tuyaNodeRecordCheck,
	msgFunWifi_nwkWifiReset,
	msgFunWifi_nwkCfg,
	msgFunWifi_nwkSelfOpen,
}enum_wifiFunMsg; //

typedef enum{

	wifiNwkStatus_cfg = 0,
	wifiNwkStatus_ap,
	wifiNwkStatus_noConnect,
	wifiNwkStatus_routerConnect,
	wifiNwkStatus_cloudConnect,
	wifiNwkStatus_lowPower,
	
	wifiNwkStatus_unKnown = 0xff,
}enum_wifiNwkStatus;

typedef struct{

	enum_wifiFunMsg funcType;
	
}stt_wifiFunMsg;

typedef enum{

	dpType_raw = 0,
	dpType_bool,
	dpType_value,
	dpType_string,
	dpType_enum,
	dpType_bitmap,
}enum_tuyaDpType;

void WIFI_mainThread(type_uart_OBJ UART_OBJ);
void uartConWifi_dataTx(unsigned char dataHex);

u8 frame_Check(unsigned char frame_temp[], u8 check_num);

void wifiFunction_nwkCfgStart(void);
void wifiFunction_callFromThread(enum_wifiFunMsg funcType, bool forceOpt_if);

void usrApp_gateWayfuncNwkopen_terminate(void);

#endif

