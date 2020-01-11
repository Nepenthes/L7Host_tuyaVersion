#ifndef __BSPDRV_OPTDEVRELAY_H_
#define __BSPDRV_OPTDEVRELAY_H_

#include "dataManage.h"

#define actRelay_ON		1
#define actRelay_OFF	0

#define CURTAIN_ORBITAL_PERIOD_INITTIME	0	//
#define CURTAIN_RELAY_UPSIDE_DOWN		1	//

#define PIN_RELAY_1_SET(x)		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, x)
#define PIN_RELAY_2_SET(x)		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, x)
#define PIN_RELAY_3_SET(x)		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, x)

typedef enum{

	statusSave_enable = 1,
	statusSave_disable,
}status_ifSave;

typedef enum{

	relay_flip = 1, //
	relay_OnOff, //
	actionNull,
}rly_methodType;

typedef struct{

	u8 act_counter;
	u8 act_period;
	
	enum{
	
		cTact_stop = 0,
		cTact_close,
		cTact_open,
	}act:8;

}stt_motorAttr;

typedef struct{

	u8 objRelay;
	rly_methodType actMethod;
}relay_Command;

typedef struct{

	bool push_IF; //
	u8 dats_Push; //
}relayStatus_PUSH;

extern u8 status_actuatorRelay;
extern status_ifSave relayStatus_ifSave;
extern relay_Command swCommand_fromUsr;
extern stt_motorAttr curtainAct_Param;
extern bool devStatus_pushIF;
extern relayStatus_PUSH devActionPush_IF;

void bsp_actuatorRelay_Init(void);

#endif

