#ifndef __BSPDRV_OPTDEVTIPS_H_
#define __BSPDRV_OPTDEVTIPS_H_

#include "dataManage.h"

#define TIPS_SWFREELOOP_TIME	60 //

#define TIPS_SWBKCOLOR_TYPENUM	10 //

#define TIPSBKCOLOR_DEFAULT_ON	8  //
#define TIPSBKCOLOR_DEFAULT_OFF 5  //

#define TIPS_BASECOLOR_NUM		3	//
#define RLY_TIPS_PERIODUNITS	32	//
#define RLY_TIPS_NUM			4

#define BEEP_OPEN_LEVEL			GPIO_PIN_SET

#define PIN_TIPS_PPP_R_SET(a)		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, a)
#define PIN_TIPS_PPP_G_SET(a)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, a)
#define PIN_TIPS_PPP_B_SET(a)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, a)

#define PIN_TIPS_RELAY1_PIN_SET(a)	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12,a)
#define PIN_TIPS_RELAY2_PIN_SET(a)	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10,a)
#define PIN_TIPS_RELAY3_PIN_SET(a)	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, a)
#define PIN_TIPS_NWK_PIN_SET(a)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14,a)

#define PIN_TIPS_BEEP_PIN_SET(a)	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14,a)

typedef union{

	union{
	
		struct{
		
			u8 sw3bit_BKlightColorInsert_open; //
			u8 sw3bit_BKlightColorInsert_close; //
		}sw3bit_BKlight_Param;
		
		struct{
		
			u8 cuertain_BKlightColorInsert_press; //
			u8 cuertain_BKlightColorInsert_bounce; //
		}cuertain_BKlight_Param;
	}sw3bitIcurtain_BKlight_Param;	
}bkLightColorInsert_paramAttr;

typedef struct{

	u8 color_R:5;
	u8 color_G:5;
	u8 color_B:5;
}func_ledRGB;

typedef enum{

	obj_Relay1 = 0,
	obj_Relay2,
	obj_Relay3,
	obj_nwk,
}tipsLED_Obj;

typedef enum{

	status_Null = 0,
	status_sysStandBy,
	status_keyFree,
	status_Normal,
	status_Night,
	status_tipsNwkOpen,
	status_touchReset,
	status_tipsAPFind,
}tips_Status;

typedef enum{

	devNwkStaute_zigbNwkNormalOnly = 0, //
	devNwkStaute_wifiNwkNormalOnly, //
	devNwkStaute_nwkAllNormal, //
	devNwkStaute_nwkAllAbnormal, //
	devNwkStaute_zigbNwkOpen, //
	devNwkStaute_wifiNwkFind, //
}tips_devNwkStatus;

typedef struct{

	u8 tips_Period:3;
	u8 tips_time;
	u8 tips_loop:5;
}sound_Attr;

typedef enum beepsMode{

	beepsMode_null = 0,
	beepsMode_standBy,
	beepsWorking,
	beepsComplete,
}enum_beeps;

extern enum_beeps dev_statusBeeps;
extern sound_Attr devTips_beep;

extern u8 timeCount_zigNwkOpen;
extern u16 counter_tipsAct;
extern u8 counter_ifTipsFree; 
extern bool countEN_ifTipsFree;

extern func_ledRGB relay1_Tips;
extern func_ledRGB relay2_Tips;
extern func_ledRGB relay3_Tips;
extern func_ledRGB nwk_Tips;

void devTipsParamSet_horsingLight(bool param);
bool devTipsParamGet_horsingLight(void);

void devTipsParamSet_keyLightColor(bkLightColorInsert_paramAttr *param);
void devTipsParamGet_keyLightColor(bkLightColorInsert_paramAttr *param);

void usrApp_tipsActionFunctionReales_period1ms(void);
void usrApp_tipsBeepFunctionReales_period100us(void);
void usrApp_tipsLightFunctionReales_period100us(void);

void beeps_usrActive(u8 tons, u8 time, u8 loop);
void tips_statusChangeToNormal(void);
void tips_statusChangeToAPFind(void);
void tips_statusChangeToZigbNwkOpen(u8 timeopen);
void tips_statusChangeToTouchReset(void);
void tips_statusChangeToFactoryRecover(void);

tips_Status devTips_status_get(void);

void bsp_opreatReflectTips_Init(void);

#endif

