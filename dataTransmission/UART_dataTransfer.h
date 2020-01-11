#ifndef __DATATRANSFER_H_
#define __DATATRANSFER_H_

#include "Driver_USART.h"

#include "dataManage.h"

#define WIFI_FRAME_HEAD		0x7F
#define ZIGB_FRAME_HEAD		0xFE

#define WIFI_MODE_AP_STA	0x0A
#define WIFI_MODE_AP		0x0B
#define WIFI_MODE_STA		0x0C

#define CONNECT0		0x01
#define CONNECT1		0x02
#define CONNECT2		0x04
#define CONNECT3		0x08
#define CONNECT4		0x10

#define USRAPP_OSSIGNAL_DEF_UART1_RX_TOUT			(1UL << 0)
#define USRAPP_OSSIGNAL_DEF_UART2_RX_TOUT			(1UL << 1)
#define USRAPP_OSSIGNAL_DEF_UART6_RX_TOUT			(1UL << 2)

#define DbugP1			Driver_USART1
#define DataTransP1		Driver_USART2
#define DataTransP2		Driver_USART6

#define DbugP1TX		Driver_USART1.Send
#define DbugP1RX		Driver_USART1.Receive

#define DBLOG_PRINTF(fmt, ...) memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));\
							   snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, fmt, ##__VA_ARGS__);\
							   DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));\
							   osDelay(10)

//#define DBLOG_PRINTF(tWait, fmt, ...)  memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));\
//									   snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, fmt, ##__VA_ARGS__);\
//									   osDelay(tWait);\
//									   DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));\

#define DataTransP1_RST	PAout(1)
#define DataTransP2_RST	PAout(6)
#define DataTransP3_RST	PAout(7)

extern ARM_DRIVER_USART Driver_USART1;
extern ARM_DRIVER_USART Driver_USART2;
extern ARM_DRIVER_USART Driver_USART6;

typedef enum{

	comObj_DbugP1 = 0xA1,
	comObj_DataTransP1,
	comObj_DataTransP2,
}type_uart_OBJ;

typedef enum{

	ftOBJ_ZIGB = 0x0A,
	ftOBJ_WIFI,
	ftOBJ_DEBUG,
}type_ftOBJ;

typedef struct{

	type_uart_OBJ 	uart_OBJ;		//���ڶ���
	type_ftOBJ		funTrans_OBJ;	//ͨ�ŷ�ʽ����zigbee��wifi
}paramLaunch_OBJ;	//��۳�ʼ�����̼��ض��� ���ݽṹ

typedef struct WIFI_Init_datsAttr{

	char *wifiInit_reqCMD;		//ATָ��
	char *wifiInit_REPLY[2];	//��Ӧָ��
	u8 	  REPLY_DATLENTAB[2];	//��Ӧָ���
	u16   timeTab_waitAnsr;		//�ȴ���Ӧʱ��
}datsAttr_wifiInit;

typedef enum datsWIFI_structType{

	wifiTP_NULL = 0,
	wifiTP_MSGCOMMING,
	wifiTP_ntCONNECT,
	wifiTP_ntDISCONNECT,
}datsWIFI_sttType;

typedef struct WIFI_datsTrans_datsAttr{

	datsWIFI_sttType datsType:5;
	
	u8	linkObj;
	u8	CMD[2];
	u8	dats[100];
	u8	datsLen;
}datsAttr_WIFITrans;

typedef struct listNodeFun_exAdd_tuya{

	uint8_t swStatus;
	uint8_t keyLightColor[2];
}funExAdd_byTuya;

/*tuyaģ���ڼ�¼�ڵ㻺��*/
typedef struct tuyaMoudle_nodeRecordInfo_format{

	uint8_t nodeMac[5];
	uint8_t nodeType;
	
}_sttTuyaNodeRecordDatsForm;

/*Zigbee�ڵ��豸��Ϣ�������ݽṹ*/
#define DEVMAC_LEN	5
typedef struct ZigB_nwkState_Form{

	u16	nwkAddr;				//����̵�ַ
	u8	macAddr[DEVMAC_LEN];	//MAC��ַ
	u8	devType;				//�豸����
	u16	onlineDectect_LCount;	//������������ʵʱ����
	
	funExAdd_byTuya funcDataMem; //������Ҫ�����Ϳѻ��ת����
	
	struct ZigB_nwkState_Form *next;
}nwkStateAttr_Zigb;

typedef struct ZigB_Init_datsAttr{

	u8 	 zigbInit_reqCMD[2];	//����ָ��
	u8 	 zigbInit_reqDAT[150];	//��������
	u8	 reqDAT_num;			//�������ݳ���
	u8 	 zigbInit_REPLY[150];	//��Ӧ����
	u8 	 REPLY_num;				//��Ӧ�����ܳ���
	u16  timeTab_waitAnsr;		//�ȴ���Ӧʱ��
}datsAttr_ZigbInit;

typedef struct ZigB_datsRXAttr_typeMSG{

	bool ifBroadcast;	//�Ƿ�Ϊ�㲥
	u16	 Addr_from;		//����Դ��ַ��������
	u8	 srcEP;			//�����ն˵�
	u8	 dstEP;			//�ռ��ն˵�
	u16	 ClusterID;		//��ID
	u8	 LQI;			//��������
	u8 	 datsLen;		//���ݳ�
	u8	 dats[100];		//����
}datsAttr_ZigbRX_tpMSG;

typedef struct ZigB_datsRXAttr_typeONLINE{

	u16	 nwkAddr_fresh;		//�����߽ڵ�����̵�ַ
}datsAttr_ZigbRX_tpONLINE;

typedef union ZigB_datsRXAttr{

	datsAttr_ZigbRX_tpMSG 		stt_MSG;
	datsAttr_ZigbRX_tpONLINE	stt_ONLINE;
}ZigbAttr_datsZigbRX;

typedef enum datsZigb_structType{

	zigbTP_NULL = 0,
	zigbTP_MSGCOMMING,
	zigbTP_ntCONNECT,
}datsZigb_sttType;

typedef struct ZigBAttr_datsRX{

	datsZigb_sttType datsType:4;	//�������ͣ���Զ����Ϣ���ݻ���ϵͳ���ݵȣ�
	ZigbAttr_datsZigbRX datsSTT;
}datsAttr_ZigbTrans;

extern osThreadId tid_com3DataTransP1_Thread;
extern osThreadId tid_com4DataTransP2_Thread;
extern osThreadId tid_com5DataTransP3_Thread;

extern osMutexId (mid_mtxUartDblog);

extern bool volatile dblogUart_txCmpFlg;

/*��������*/
void USART1_DbugP1_Init(void);
void USART2_DataTransP1_Init(void);
void USART6_DataTransP2_Init(void);

/*�ڴ����*/
void msgQueueInit(void);
void osMemoryInit(void);

void com1DbugP1_Thread (const void *argument);
void com2DataTransP1_Thread	(const void *argument);
void com5DataTransP2_Thread	(const void *argument);

void TimerZigbDevManage_Callback(void const *arg);

void usrApp_usartEventCb_register(type_uart_OBJ uartObj, void (*funCb)(uint32_t event));
ARM_DRIVER_USART *uartDrvObjGet(type_uart_OBJ uartObj);
uint32_t uartRxTimeout_signalDefGet(type_uart_OBJ uartObj);
void communicationActive(type_uart_OBJ comObj,type_ftOBJ ftTransObj);

#endif

