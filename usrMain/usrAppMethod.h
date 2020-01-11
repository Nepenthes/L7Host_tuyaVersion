#ifndef __USRAPPMETHOD_H__
#define __USRAPPMETHOD_H__

#include "dataManage.h"

#define MAC5_2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4]
#define MAC5STR "%02X%02X%02X%02X%02X"

/*¹¦ÄÜº¯Êý*/
void *memmem(void *start, unsigned int s_len, void *find,unsigned int f_len);
int memloc(u8 str2[],u8 num_s2,u8 str1[],u8 num_s1);
int strloc(char *str2,char *str1);
float bytesTo_float(u8 dat[4]);
int ftoa(char *str, float num, int n);
bool str2devType(char *str, uint8_t *devType);
bool str2Mac5(char *str, uint8_t mac[5]);

uint8_t devCurtain_statusCodeChg_D2S(uint8_t code);
uint8_t devCurtain_statusCodeChg_S2D(uint8_t code);

#endif
