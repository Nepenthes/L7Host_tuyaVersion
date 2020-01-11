#include "usrAppMethod.h"

void *memmem(void *start, unsigned int s_len, void *find,unsigned int f_len){
	
	char *p, *q;
	unsigned int len;
	p = start, q = find;
	len = 0;
	while((p - (char *)start + f_len) <= s_len){
			while(*p++ == *q++){
					len++;
					if(len == f_len)
							return(p - f_len);
			};
			q = find;
			len = 0;
	};
	return(NULL);
}

int memloc(u8 str2[],u8 num_s2,u8 str1[],u8 num_s1)
{
	int la = num_s1;
	int i, j;
	int lb = num_s2;
	for(i = 0; i < lb; i ++)
	{
		for(j = 0; j < la && i + j < lb && str1[j] == str2[i + j]; j ++);
		if(j == la)return i;
	}
	return -1;
}

int strloc(char *str2,char *str1)
{
	int la = strlen(str1);
	int i, j;
	int lb = strlen(str2);
	for(i = 0; i < lb; i ++)
	{
		for(j = 0; j < la && i + j < lb && str1[j] == str2[i + j]; j ++);
		if(j == la)return i;
	}
	return -1;
}

//void printf_datsHtoA(const u8 *TipsHead, u8 *dats , u8 datsLen){

//	u8 dats_Log[DEBUG_LOGLEN] = {0};
//	u8 loop = 0;

//	memset(&dats_Log[0], 0, DEBUG_LOGLEN * sizeof(u8));
//	for(loop = 0; loop < datsLen; loop ++){

//		sprintf((char *)&dats_Log[loop * 3], "%02X ", *dats ++);
//	}
//	os_printf("%s<datsLen: %d> %s.\n", TipsHead, datsLen, dats_Log);
//}

float 
bytesTo_float(u8 dat[4]){

	const float decimal_prtCoefficient = 10000.0F;

	float res = 0.0F;
	u16 integer_prt = ((u16)dat[0] << 8) + ((u16)dat[1]);
	u16 decimal_prt = ((u16)dat[2] << 8) + ((u16)dat[3]);

	res = (float)integer_prt + (((float)decimal_prt) / decimal_prtCoefficient);
	
	return res;
}

int 
ftoa(char *str, float num, int n){
	
	int 	sumI;
	float	sumF;
	int		sign = 0;
	int		temp;
	int		count = 0;
	
	char	*p;
	char	*pp;
	
	if(str == NULL)return -1;
	p = str;
	
	if(num < 0){
		
		sign = 1;
		num = 0 - num;
	}
	
	sumI = (int)num;
	sumF = num - sumI;
	
	do{
		
		temp = sumI % 10;
		*(str ++) = temp + '0';
		
	}while((sumI = sumI / 10) != 0);
	
	if(sign == 1){
		
		*(str ++) = '-';
	}
	
	pp = str;
	pp --;
	
	while(p < pp){
		
		*p = *p + *pp;
		*pp = *p - *pp;
		*p = *p - *pp;
		p ++;
		pp --;
	}
	
	*(str ++) = '.';
	
	do{
		
		temp = (int)(sumF * 10);
		*(str ++) = temp + '0';
		
		if((++ count) == n)break;
		
		sumF = sumF * 10 - temp;
		
	}while(!(sumF > -0.000001F && sumF < 0.00001F));
	
	
	*str = 0;
	
	return 0;
}	

bool str2devType(char *str, uint8_t *devType){

	uint8_t loop = 0;
	bool chg_res = false;
	
	for(loop = 0; loop < 1 * 2; loop ++){
	
		if(str[loop] >= '0' && str[loop] <= '9'){
			
			if(!(loop % 2))
			{
				
				*devType |= (str[loop] - '0' << 4);
			}
			else
			{
				*devType |= (str[loop] - '0' << 0);
			}
		
		}else
		if(str[loop] >= 'A' && str[loop] <= 'Z')
		{
			if(!(loop % 2))
			{
				
				*devType |= ((str[loop] - 'A' + 0x0A) << 4);
			}
			else
			{
				*devType |= ((str[loop] - 'A' + 0x0A) << 0);
			}
		}
		else
		{
			break;
		}
	}
	
	if(loop != 1 * 2)return chg_res;
	
	chg_res = true;
	
	return chg_res;
}

bool str2Mac5(char *str, uint8_t mac[5]){

	uint8_t loop = 0;
	bool chg_res = false;
	
	for(loop = 0; loop < 5 * 2; loop ++){
	
		if(str[loop] >= '0' && str[loop] <= '9');
		else{
		
			break;
		}
	}
	if(loop != 5 * 2)return chg_res;
	
	for(loop = 0; loop < 5; loop ++){
		
		mac[loop] = ((str[loop * 2] - '0') << 4) + (str[loop * 2 + 1] - '0');
	}
	
	chg_res = true;
	
	return chg_res;
}

uint8_t devCurtain_statusCodeChg_D2S(uint8_t code){

	uint8_t res = 1;
	
	switch(code){
	
		case 1:res = 0;break;
			
		case 2:res = 1;break;
			
		case 4:res = 2;break;
		
		default:break;
	}
	
	return res;
}

uint8_t devCurtain_statusCodeChg_S2D(uint8_t code){

	uint8_t res = 2;
	
	switch(code){
	
		case 0:res = 1;break;
			
		case 1:res = 2;break;
			
		case 2:res = 4;break;
		
		default:break;
	}
	
	return res;
}

