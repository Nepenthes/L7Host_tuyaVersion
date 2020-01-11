#include "dataManage.h"

#include "UART_dataTransfer.h"

const char devType_strTab[8][10] = {

	"sw1","sw2","sw3","cur","dim","hea","fans","soc",
};

const u8 devRootNode_subidMac[5] = {0, 0, 0, 0, 1};
	
const u8 debugLogOut_targetMAC[5] = {0x20, 0x15, 0x08, 0x48, 0x86};

osPoolId  (normalMemUint8_t_poolAttr_id); // Memory pool ID
osPoolDef (normalMemUint8_t_pool, 1024, uint8_t);

char printfLog_bufTemp[DBLOB_BUFTEMP_LEN] = {0};

int8_t sysTimeZone_H = 0; 
int8_t sysTimeZone_M = 0; 
uint32_t systemUTC_current = 0;
u16	sysTimeKeep_counter	= 0;
stt_localTime systemTime_current = {0};

u8 zigbNwkReserveNodeNum_currentValue = 0;

u8 DEV_actReserve = 0x07; //有效操作位
u8 SWITCH_TYPE = SWITCH_TYPE_SWBIT3;	

bool deviceLock_flag = false;

u8 MACSTA_ID[DEV_MAC_LEN] = {9, 9, 9, 9, 9, 9};
u8 MACAP_ID[DEV_MAC_LEN] = {0};
u8 MACDST_ID[DEV_MAC_LEN] = {1,1,1,1,1,1}; //

static stt_usrDats_privateSave usrApp_localRecordDatas = {

	.devCurtain_orbitalPeriod = 33,
};

u8 switchTypeReserve_GET(void){

	u8 act_Reserve = 0x07;

	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT3){
		
		act_Reserve = 0x07;
		
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
		
		act_Reserve = 0x05;
	
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
	
		act_Reserve = 0x02;
	}
	
	return act_Reserve;
}

const char *typeStrgetByNum(uint8_t typeNum){

	const char *ptr = NULL;
	
	switch(typeNum){

		case SWITCH_TYPE_SWBIT1:	{ptr = devType_strTab[0];}break;
			
		case SWITCH_TYPE_SWBIT2:	{ptr = devType_strTab[1];}break;
		
		case SWITCH_TYPE_SWBIT3:	{ptr = devType_strTab[2];}break;
			
		case SWITCH_TYPE_CURTAIN:	{ptr = devType_strTab[3];}break;		

		case SWITCH_TYPE_DIMMER:	{ptr = devType_strTab[4];}break;
			
		case SWITCH_TYPE_FANS:		{ptr = devType_strTab[6];}break;
			
		case SWITCH_TYPE_SOCKETS:	{ptr = devType_strTab[7];}break;
			
		case SWITCH_TYPE_HEATER:	{ptr = devType_strTab[5];}break;
	}
	
	return ptr;
}

void usrApp_MEM_If_Init_FS(void){
 
    HAL_FLASH_Unlock(); 
//    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP    | FLASH_FLAG_OPERR  | FLASH_FLAG_WRPERR | 
//                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
 
}


void usrApp_MEM_If_DeInit_FS(void){
	
    HAL_FLASH_Lock();
}

uint32_t usrApp_GetFlashSector(uint32_t Address){
	
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_DEF_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_DEF_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_DEF_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_DEF_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_DEF_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_DEF_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_DEF_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_DEF_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_DEF_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_DEF_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_DEF_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_SECTOR_DEF_11;  
  }

  return sector;
}

void usrApp_nvsOpreation_dataSave(stt_usrDats_privateSave *dataParam){

	uint8_t dataTemp[16] = {0};
	
	FLASH_EraseInitTypeDef EraseInitStruct = {0};
	uint32_t PageError = 0;
	uint8_t loop = 0;
	
    /* Fill EraseInit structure*/
	EraseInitStruct.Banks			= FLASH_BANK_1;
    EraseInitStruct.TypeErase 		= TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange 	= VOLTAGE_RANGE_3;
	EraseInitStruct.Sector 			= FLASH_SECTOR_DEF_11;
	EraseInitStruct.NbSectors 		= 1;
	
//	memcpy(dataTemp, dataParam, sizeof(stt_usrDats_privateSave));
	
//	usrApp_MEM_If_Init_FS();
//	
//	if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK){
//	
//	
//	}
//	
//	for(loop = 0; loop < 2; loop ++){
//	
//		if(HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(ADDR_USRAPP_DATANVS_DEF + loop * 8), dataTemp[loop])){
//		
//			
//		}
//	}
//	
//	usrApp_MEM_If_DeInit_FS();
}

void usrApp_nvsOpreation_dataGet(stt_usrDats_privateSave *dataParam){

//	uint32_t *dataTemp = (__IO uint32_t*)ADDR_USRAPP_DATANVS_DEF;
	
//	memcpy(dataParam, &dataTemp, sizeof(stt_usrDats_privateSave));
}

void usrApp_nvsOpreation_test(void){

	stt_usrDats_privateSave nvsDataRead_temp = {0};
	
	usrApp_nvsOpreation_dataSave(&usrApp_localRecordDatas);
	usrApp_nvsOpreation_dataGet(&nvsDataRead_temp);
	
	DBLOG_PRINTF("nvs dataRD:%d.\n", nvsDataRead_temp.devCurtain_orbitalPeriod);
}

void bsp_dataManageInit(void){

	normalMemUint8_t_poolAttr_id = osPoolCreate(osPool(normalMemUint8_t_pool));
}
