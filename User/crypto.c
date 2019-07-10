// @gyl
#include "stm32f10x.h"
#include "./rtc/bsp_date.h"

extern struct rtc_time systmtime;

static uint32_t Crypto_CalcOneKey(uint32_t security, uint32_t wSeed) {
   uint8_t    iterations;
   uint32_t   wLastSeed;
   uint32_t   wTemp;
   uint32_t   wLSBit;
   uint32_t   wTop31Bits;
   uint8_t    jj, SB1, SB2, SB3;
   uint32_t   temp;
    /* Calculate Number of passes */
    wLastSeed = wSeed;
    temp =(uint32_t)((security & 0x00000800) >> 10) | ((security & 0x00200000)>> 21);   
    switch (temp) {
        case 0:
            wTemp = (uint8_t)((wSeed & 0xff000000) >> 24);
            break;
        case 1:
            wTemp = (uint8_t)((wSeed & 0x00ff0000) >> 16);
            break;       
        case 2:
            wTemp = (uint8_t)((wSeed & 0x0000ff00) >> 8);
            break;     
        case 3:
            wTemp = (uint8_t)(wSeed & 0x000000ff);
            break;
    }
    SB1 = (uint8_t)((security & 0x000003FC) >> 2);
    SB2 = (uint8_t)(((security & 0x7F800000) >> 23) ^ 0xA5);
    SB3 = (uint8_t)(((security & 0x001FE000) >> 13) ^ 0x5A);
    /* SB2 and SB3 determin the maximum number of passes through the loop.
    Size of SB2 and SB3 can be limited to fewer bits, to minimise the maximum number of passes through the algorithm
    The iterations calculation; where wSeedItuint8, SB1, SB2 and SB3 are generated from fixed SECURITYCONSTANT_EXTENDED;
    */
    iterations = (uint8_t)(((wTemp ^ SB1) & SB2)  + SB3);
    for (jj = 0; jj < iterations; jj++) {
        wTemp =   ((wLastSeed & 0x40000000)/0x40000000) ^ ((wLastSeed & 0x01000000)/0x01000000)
                  ^ ((wLastSeed & 0x1000)/0x1000)   	^ ((wLastSeed & 0x04)/0x04) ;
        
        wLSBit = (wTemp & 0x00000001) ;
        
        wLastSeed  = (uint8_t)(wLastSeed << 1); /* Left Shift the bits */
        wTop31Bits = (uint8_t)(wLastSeed & 0xFFFFFFFE) ;    
        wLastSeed  = (uint8_t)(wTop31Bits | wLSBit);
    }   
    /*Do uint8 swap, as per spec  0 1 2 3*/
    if (security & 0x00000001) {
        wTop31Bits = ((wLastSeed & 0x00FF0000)>>16) |  /*KEY[0] = Last_Seed[1]*/ 
                     ((wLastSeed & 0xFF000000)>>8)  |  /*KEY[1] = Last_Seed[0]*/ 
                     ((wLastSeed & 0x000000FF)<<8)  |  /*KEY[2] = Last_Seed[3]*/
                     ((wLastSeed & 0x0000FF00)<<16);   /*KEY[3] = Last_Seed[2]*/
		}
    else {
    	wTop31Bits = wLastSeed;
		}
    wTop31Bits = wTop31Bits ^ security;  
    return wTop31Bits;
}

// 通过Seed和安全算子security计算key
// 一个seed对应一个security，总共3个seed，生成3个key
// 当设备向app发送seed和key，设备计算key校验时，使用设备ID作为security
// 当app向设备发送seed和key，app计算key校验时，使用app ID作为security
void Crypto_CalcKey(uint32_t wSeed, uint32_t *keyset, uint32_t *securityset) {
	uint8_t i;
	for (i = 0; i < 3; i++) {
		keyset[i] = Crypto_CalcOneKey(securityset[i], wSeed);
	}
}

//线性同余法生成3个32位随机数
void Crypto_Random(uint32_t *randseedset) {
	uint8_t i;
	randseedset[0] = mktimev(&systmtime);
	for (i = 0; i < 2; i++) {
		randseedset[i + 1] = (randseedset[i] * 65793 + 4282663) % ((uint32_t)2 << 23);
	}
}