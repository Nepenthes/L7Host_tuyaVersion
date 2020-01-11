#include "timer_Activing.h"

#include "UART_dataTransfer.h"

static void timActivingProcess_Thread(const void *argument);

osThreadId tid_timActivingProcess_Thread;
osThreadDef(timActivingProcess_Thread, osPriorityNormal, 1, 512);

bool ifNightMode_sw_running_FLAG = false;

u8 dbSysLogLoopCounter = DBLOG_INFO_PRINTF_PERIOD;

static void timActivingProcess_Thread(const void *argument){

	for(;;){
	
		{ //系统时间自更新
			
			if(sysTimeKeep_counter < 3600){
			
				
			
			}else{ //
			
				sysTimeKeep_counter = 0;
			
				if(systemTime_current.time_Hour >= 23){
			
					systemTime_current.time_Hour = 0;
					(systemTime_current.time_Week > 7)?(systemTime_current.time_Week = 1):(systemTime_current.time_Week ++);
				
				}else{
			
					systemTime_current.time_Hour ++;
				}
			}

			systemTime_current.time_Minute = sysTimeKeep_counter / 60;
			systemTime_current.time_Second = sysTimeKeep_counter % 60;
		}
		
		{
			if(!dbSysLogLoopCounter){
			
				dbSysLogLoopCounter = DBLOG_INFO_PRINTF_PERIOD;
				DBLOG_PRINTF("20%02d/%02d/%02d-W%01d\n    %02d:%02d:%02d\n devTotelNum:%d.\n\n", 
							 (int)systemTime_current.time_Year,
							 (int)systemTime_current.time_Month,
							 (int)systemTime_current.time_Day,
							 (int)systemTime_current.time_Week,
							 (int)systemTime_current.time_Hour,
							 (int)systemTime_current.time_Minute,
							 (int)systemTime_current.time_Second,
							 (int)zigbNwkReserveNodeNum_currentValue);
			}
		}
		
		osDelay(10);
	}
}

void usrAppTimActiving_processInit(void){

	tid_timActivingProcess_Thread = osThreadCreate(osThread(timActivingProcess_Thread), NULL);
}








