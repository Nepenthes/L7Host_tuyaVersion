/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include "stm32f4xx_hal.h"

#include "UART_dataTransfer.h"
#include "bspDrv_iptDevice.h"
#include "bspDrv_optDevTips.h"
#include "bspDrv_optDevRelay.h"
#include "bussiness_timerHard.h"
#include "bussiness_timerSoft.h"
#include "timer_Activing.h"

/*
 * main: initialize and start the system
 */
int main (void) {
	
  osKernelInitialize ();                    // initialize CMSIS-RTOS

  // initialize peripherals here

  // create 'thread' functions that start executing,
  // example: tid_name = osThreadCreate (osThread(name), NULL);

	HAL_Init();
	
	bsp_dataManageInit();
	
	osMemoryInit();
	msgQueueInit();
	
	communicationActive(comObj_DbugP1, ftOBJ_DEBUG);
	communicationActive(comObj_DataTransP1, ftOBJ_WIFI);
	communicationActive(comObj_DataTransP2, ftOBJ_ZIGB);
	
	usrApp_nvsOpreation_test();
	
	bsp_hwTimerInit();
	bsp_iwdgInit();
	bsp_actuatorRelay_Init();
	bsp_opreatReflectTips_Init();
	usrAppNormalBussiness_softTimerInit();
	usrAppTimActiving_processInit();
	devDrvIpt_threadProcessActive();
	
	beeps_usrActive(5, 40, 2);

  osKernelStart ();                         // start thread execution 
}
