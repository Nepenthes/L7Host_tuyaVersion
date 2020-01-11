#include "UART_dataTransfer.h"

#include "datsProcess_uartZigbee.h"
#include "datsProcess_uartWifi.h"

#include "stm32f4xx_hal_gpio.h"

#include "bussiness_timerHard.h"

osThreadId tid_com1DbugP1_Thread;
osThreadId tid_com2DataTransP1_Thread;
osThreadId tid_com5DataTransP2_Thread;

osThreadDef(com1DbugP1_Thread,osPriorityNormal,			1,	512);
osThreadDef(com2DataTransP1_Thread,osPriorityNormal,	1,	1024 * 4);
osThreadDef(com5DataTransP2_Thread,osPriorityNormal,	1,	1024 * 8);

osPoolId  (dttAct_poolAttr_id); // Memory pool ID
osPoolDef (dtThreadActInitParm_pool, 5, type_ftOBJ); // Declare memory pool	//数据传输进程初始化信息 数据结构池

osPoolId  (threadDP_poolAttr_id); 
osPoolDef (threadDatsPass_pool, 30, stt_threadDatsPass); // Declare memory pool	//数据传输进程间消息传递 数据结构池 

osMutexDef(mtxUartDblog);    //
osMutexId (mid_mtxUartDblog); //

static void (*usartEventCb[5])(uint32_t event);

void uartConDblog_eventFunCb(uint32_t event){

	if(event & (ARM_USART_EVENT_TX_COMPLETE | ARM_USART_EVENT_TX_UNDERFLOW)){
	
		osMutexRelease(mid_mtxUartDblog);
	}
}

ARM_DRIVER_USART *uartDrvObjGet(type_uart_OBJ uartObj){

	ARM_DRIVER_USART *uartDrvObj = NULL;
	
	switch(uartObj){
	
		case comObj_DbugP1:			{uartDrvObj = &DbugP1;}break;		
		
		case comObj_DataTransP1:	{uartDrvObj = &DataTransP1;}break;		
		
		case comObj_DataTransP2:	{uartDrvObj = &DataTransP2;}break;	

		default:break;
	}
	
	return uartDrvObj;
}

uint32_t uartRxTimeout_signalDefGet(type_uart_OBJ uartObj){

	uint32_t sig = 0;
	
	switch(uartObj){
		
		case comObj_DbugP1:{
		
			sig = USRAPP_OSSIGNAL_DEF_UART1_RX_TOUT;
		
		}break;
		
		case comObj_DataTransP1:{
			
			sig = USRAPP_OSSIGNAL_DEF_UART2_RX_TOUT;
			
		}break;
		
		case comObj_DataTransP2:{
					
			sig = USRAPP_OSSIGNAL_DEF_UART6_RX_TOUT;
			
		}break;
			
		default:{}break;
	}
	
	return sig;
}

static void USART1_DbugP1_Init(void){

//	GPIO_InitTypeDef gpio_initstruct;
	
    /*Initialize the USART driver */
	Driver_USART1.Uninitialize();
    Driver_USART1.Initialize(usartEventCb[0]);
    /*Power up the USART peripheral */
    Driver_USART1.PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 115200 Bits/sec */
    Driver_USART1.Control(ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE, 115200);

    /* Enable Receiver and Transmitter lines */
    Driver_USART1.Control (ARM_USART_CONTROL_TX, 1);
    Driver_USART1.Control (ARM_USART_CONTROL_RX, 1);
	
//   __HAL_RCC_GPIOA_CLK_ENABLE();

//   gpio_initstruct.Mode 	= GPIO_MODE_OUTPUT_PP;
//   gpio_initstruct.Speed	= GPIO_SPEED_FREQ_HIGH;
//   gpio_initstruct.Pull		= GPIO_PULLUP;
//   gpio_initstruct.Pin		= GPIO_PIN_9;
//   HAL_GPIO_Init(GPIOA, &gpio_initstruct);
//   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
	
	osDelay(100);
	
	DBLOG_PRINTF("i'm usart1 for debugP1\r\n");
}

static void USART2_DataTransP1_Init(void){
	
    /*Initialize the USART driver */
	Driver_USART2.Uninitialize();
    Driver_USART2.Initialize(usartEventCb[1]);
    /*Power up the USART peripheral */
    Driver_USART2.PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 115200 Bits/sec */
    Driver_USART2.Control(ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE, 115200);

    /* Enable Receiver and Transmitter lines */
    Driver_USART2.Control (ARM_USART_CONTROL_TX, 1);
    Driver_USART2.Control (ARM_USART_CONTROL_RX, 1);
	
    Driver_USART2.Send("i'm usart2 for dataTransP1\r\n", 28);
}

static void USART6_DataTransP2_Init(void){
	
   GPIO_InitTypeDef gpio_initstruct;

   /*Initialize the USART driver */
   Driver_USART6.Uninitialize();
   Driver_USART6.Initialize(usartEventCb[4]);
   /*Power up the USART peripheral */
   Driver_USART6.PowerControl(ARM_POWER_FULL);
   /*Configure the USART to 115200 Bits/sec */
   Driver_USART6.Control(ARM_USART_MODE_ASYNCHRONOUS |
                         ARM_USART_DATA_BITS_8 |
                         ARM_USART_PARITY_NONE |
                         ARM_USART_STOP_BITS_1 |
                         ARM_USART_FLOW_CONTROL_NONE, 115200);

   /* Enable Receiver and Transmitter lines */
   Driver_USART6.Control (ARM_USART_CONTROL_TX, 1);
   Driver_USART6.Control (ARM_USART_CONTROL_RX, 1);

   __HAL_RCC_GPIOC_CLK_ENABLE();

   gpio_initstruct.Mode 	= GPIO_MODE_OUTPUT_PP;
   gpio_initstruct.Speed	= GPIO_SPEED_FREQ_HIGH;
   gpio_initstruct.Pull		= GPIO_PULLUP;
   gpio_initstruct.Pin		= GPIO_PIN_13;
   HAL_GPIO_Init(GPIOC, &gpio_initstruct);
   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

   Driver_USART6.Send("i'm usart6 for dataTransP2\r\n", 28);
}

void usrApp_usartEventCb_register(type_uart_OBJ uartObj, void (*funCb)(uint32_t event)){
	
	switch(uartObj){
		
		case comObj_DbugP1:{
		
			usartEventCb[0] = funCb;
			USART1_DbugP1_Init();
		
		}break;
		
		case comObj_DataTransP1:{
			
			usartEventCb[1] = funCb;
			USART2_DataTransP1_Init();
			
		}break;
		
		case comObj_DataTransP2:{
					
			usartEventCb[4] = funCb;
			USART6_DataTransP2_Init();
			
		}break;
			
		default:{}break;
	}
	
	DBLOG_PRINTF("com evtCb[%02X] reg.\n", uartObj);
}

/*通讯主线程激活*/
void dataTransThread_Active(const void *argument){

	paramLaunch_OBJ *datsTransActOBJ = (paramLaunch_OBJ *)argument;
	
	switch(datsTransActOBJ->funTrans_OBJ){
	
		case ftOBJ_ZIGB:{
		
			ZigB_mainThread(datsTransActOBJ->uart_OBJ);
		}break;
		
		case ftOBJ_WIFI:{
		
			WIFI_mainThread(datsTransActOBJ->uart_OBJ);
		}break;
		
		case ftOBJ_DEBUG:{
			
			mid_mtxUartDblog = osMutexCreate(osMutex(mtxUartDblog));
			usrApp_usartEventCb_register(datsTransActOBJ->uart_OBJ, uartConDblog_eventFunCb);
			
		}break;
		
		default:break;
	}
	
	osPoolFree(dttAct_poolAttr_id,datsTransActOBJ);
}

void com1DbugP1_Thread(const void *argument){
	
	dataTransThread_Active(argument);
	
	for(;;){
		
//		if(!counterRemind_hwTimer){
//		
//			counterRemind_hwTimer = 3;
//			
//			DBLOG_PRINTF("hwTimer remindCount.\n");
//			osDelay(500);
//		}
//		DBLOG_PRINTF("tt.\n");
		osDelay(500);
	}
}

void com2DataTransP1_Thread(const void *argument){
	
	dataTransThread_Active(argument);
}

void com5DataTransP2_Thread(const void *argument){

	dataTransThread_Active(argument);
}

void osMemoryInit(void){

	dttAct_poolAttr_id = osPoolCreate(osPool(dtThreadActInitParm_pool)); //数据传输进程初始化信息 数据结构堆内存池初始化
}

void msgQueueInit(void){

	threadDP_poolAttr_id = osPoolCreate(osPool(threadDatsPass_pool)); //zigbee与wifi主线程数据传输消息队列 数据结构堆内存池初始化
}

void communicationActive(type_uart_OBJ comObj,type_ftOBJ ftTransObj){
	
	paramLaunch_OBJ *param_DatsTransAct = (paramLaunch_OBJ *)osPoolAlloc(dttAct_poolAttr_id);
	
	param_DatsTransAct->uart_OBJ 		= comObj;
	param_DatsTransAct->funTrans_OBJ 	= ftTransObj;

	switch(param_DatsTransAct->uart_OBJ){
	
		case comObj_DbugP1:		tid_com1DbugP1_Thread = osThreadCreate(osThread(com1DbugP1_Thread),param_DatsTransAct);
								break;
				
		case comObj_DataTransP1:tid_com2DataTransP1_Thread = osThreadCreate(osThread(com2DataTransP1_Thread),param_DatsTransAct);
								break;
				
		case comObj_DataTransP2:tid_com5DataTransP2_Thread = osThreadCreate(osThread(com5DataTransP2_Thread),param_DatsTransAct);
								break;
		
		default:				break;
	}	
}
