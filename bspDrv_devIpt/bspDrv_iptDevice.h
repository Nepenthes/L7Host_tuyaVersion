#ifndef __BSPDRV_IPTDEVICE_H_
#define __BSPDRV_IPTDEVICE_H_

#include "dataManage.h"

#define Dcode_FLG_ifAP			0x01
#define Dcode_FLG_ifMemory		0x02
#define Dcode_FLG_bitReserve	0x0C	

#define Dcode_bitReserve(x)		((x & 0x0C) >> 2)

typedef enum{

	press_Null = 1,
	press_Short,
	press_ShortCnt, //
	press_LongA,
	press_LongB,
}keyCfrm_Type;

typedef struct{

	u8 param_combinationFunPreTrig_standBy_FLG:1; //
	u8 param_combinationFunPreTrig_standBy_keyVal:7; //
}param_combinationFunPreTrig;

typedef void funKey_Callback(void);

extern u8 	timeCounter_smartConfig_start;
extern bool smartconfigOpen_flg;

extern u16 	touchPadActCounter; 
extern u16 	touchPadContinueCnt;  

extern bool usrKeyCount_EN;
extern u16	usrKeyCount;

extern u16  combinationFunFLG_3S5S_cancel_counter;

void usrApp_wifiNwkCfg_stop(void);

void usrApp_iptFunctionReales_period1ms(void);
void devDrvIpt_threadProcessActive(void);

#endif
