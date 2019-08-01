// @gyl
#include "communication.h"
#include "functional.h"
#include "eeprom.h"

const extern uint32_t device_id;
const extern uint16_t sw_version;

extern volatile uint8_t step_flag;
extern volatile uint8_t interactstatus;
extern volatile uint8_t charging_flag;
extern volatile float battsoc;

extern uint8_t binding_flag;
extern uint8_t eeprom_status;
extern uint16_t fillcounter;
extern uint32_t app_id;
extern Step walking;
extern Step running;

//上一cycle状态
static uint8_t lastinteractstatus = 0;
//超时计数
static uint16_t overtimecounter = 0;
//数据接收缓存
static volatile uint8_t receivebuffer[6];
//接收计数
static volatile uint8_t receivecounter = 0;

//计算安全密钥
static uint32_t crypto_key(uint32_t wSeed, uint32_t security) {
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
		wTemp = ((wLastSeed & 0x40000000)/0x40000000) ^ ((wLastSeed & 0x01000000)/0x01000000) ^
						((wLastSeed & 0x1000)/0x1000) ^ ((wLastSeed & 0x04)/0x04) ;
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

//发送key
static void send_key(uint32_t useed, uint32_t security) {
	uint32_t tempkey;
	uint8_t sendbuffer[8];
	tempkey = crypto_key(useed, security);
	sendbuffer[0] = 0x66;
	sendbuffer[1] = 0xAA;
	sendbuffer[2] = (uint8_t)(tempkey >> 24);
	sendbuffer[3] = (uint8_t)(tempkey >> 16);
	sendbuffer[4] = (uint8_t)(tempkey >> 8);
	sendbuffer[5] = (uint8_t)tempkey;
	sendbuffer[6] = COM_Checksum(tempkey);
	sendbuffer[7] = 0x88;
	Usart_SendArray(USART1, sendbuffer, 8);
}

//否定应答
void COM_Send_Deny(uint8_t error) {
	uint8_t sendbuffer[8];
	sendbuffer[0] = 0x66;
	sendbuffer[1] = 0xFF;
	sendbuffer[2] = 0;
	sendbuffer[3] = 0;
	sendbuffer[4] = 0;
	sendbuffer[5] = 0;
	sendbuffer[6] = error;
	sendbuffer[7] = 0x88;
	Usart_SendArray(USART1, sendbuffer, 8);
}

//肯定应答
void COM_Send_Positive(void) {
	uint8_t sendbuffer[8];
	sendbuffer[0] = 0x66;
	sendbuffer[1] = 0xAA;
	sendbuffer[2] = 0;
	sendbuffer[3] = 0;
	sendbuffer[4] = 0;
	sendbuffer[5] = 0;
	sendbuffer[6] = 0;
	sendbuffer[7] = 0x88;
	Usart_SendArray(USART1, sendbuffer, 8);
}

//上传步数
static void send_steps(uint32_t value) {
	uint8_t sendbuffer[8];
	sendbuffer[0] = 0x66;
	sendbuffer[1] = 0xAA;
	sendbuffer[2] = (uint8_t)(value >> 24);
	sendbuffer[3] = (uint8_t)(value >> 16);
	sendbuffer[4] = (uint8_t)(value >> 8);
	sendbuffer[5] = (uint8_t)value;
	sendbuffer[6] = COM_Checksum(value);
	sendbuffer[7] = 0x88;
	Usart_SendArray(USART1, sendbuffer, 8);
}

//发送设备其他信息
static void send_info(void) {
	uint8_t sendbuffer[8];
	sendbuffer[0] = 0x66;
	sendbuffer[1] = 0xAA;
	sendbuffer[2] = (uint8_t)(sw_version >> 8);
	sendbuffer[3] = (uint8_t)sw_version;
	sendbuffer[4] = (eeprom_status << 4) | charging_flag;
	sendbuffer[5] = (uint8_t)((uint32_t)battsoc);
	sendbuffer[6] = sendbuffer[2] + sendbuffer[3] + sendbuffer[4] + sendbuffer[5];
	sendbuffer[7] = 0x88;
	Usart_SendArray(USART1, sendbuffer, 8);
}

//检查请求类型
static void check_request_type(void) {
	switch (interactstatus) {
		case 1:
			if (receivebuffer[0] == 0x11) {
				interactstatus = 4;
			}
			else if (receivebuffer[0] == 0x22) {
				interactstatus = 6;
			}
			else if (receivebuffer[0] == 0x33) {
				interactstatus = 8;
			}
			else {
				if (binding_flag == 1) {
					step_flag = 1;
				}
				interactstatus = 0;
				COM_Send_Deny(INVALID_TYPE);
			}
			break;
		case 2:
			if (receivebuffer[0] == 0xAA) {
				interactstatus = 5;
			}
			else if (receivebuffer[0] == 0xFF) {
				interactstatus = 0;
			}
			else {
				COM_Send_Deny(INVALID_TYPE);
			}
			break;
		case 3:
			if (receivebuffer[0] == 0x1A || receivebuffer[0] == 0x2A || receivebuffer[0] == 0x3A ||
					receivebuffer[0] == 0x4A || receivebuffer[0] == 0x5A || receivebuffer[0] == 0x6A) {
				interactstatus = 7;
			}
			else if (receivebuffer[0] == 0xFF) {
				step_flag = 1;
				interactstatus = 0;
			}
			else {
				COM_Send_Deny(INVALID_TYPE);
			}
			break;
	}
}

//计算checksum
uint8_t COM_Checksum(uint32_t value) {
	return (uint8_t)(value / 1000000000) + (uint8_t)((value % 1000000000) / 100000000) + (uint8_t)((value % 100000000) / 10000000) +
				 (uint8_t)((value % 10000000) / 1000000) + (uint8_t)((value % 1000000) / 100000) + (uint8_t)((value % 100000) / 10000) +
				 (uint8_t)((value % 10000) / 1000) + (uint8_t)((value % 1000) / 100) + (uint8_t)((value % 100) / 10) + (uint8_t)(value % 10);
}

//侦听复位(超过5s复位)$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
void COM_Listen_Reset(void) {
	if (step_flag > 0) {
		return;
	}
	if (lastinteractstatus == 0 && interactstatus > 0) {
		overtimecounter = 1;
	}
	else if (lastinteractstatus > 0 && interactstatus == 0) {
		overtimecounter = 0;
	}
	else {
		overtimecounter++;
	}
	if (overtimecounter > 500) {
		overtimecounter = 0;
		receivecounter = 0;
		interactstatus = 0;
		lastinteractstatus = 0;
		if (binding_flag == 1) {
			step_flag = 1;
		}
	}
	lastinteractstatus = interactstatus;
}

//侦听动作
void COM_Listening(void) {
	uint8_t data = USART_ReceiveData(USART1);
	if (interactstatus > 3 || (battsoc < 2 && charging_flag != 0) || step_flag > 1) {
		return;
	}
	step_flag = 0;
	fillcounter = 0;
	if (receivecounter == 0 && data == 0x66) {
		if (interactstatus == 0) {
			interactstatus = 1;
		}
		receivecounter++;
	}
	else if (receivecounter > 0 && receivecounter < 7) {
		receivebuffer[receivecounter - 1] = data;
		receivecounter++;
	}
	else if (receivecounter == 7 && data == 0x88) {
		receivecounter = 0;
		check_request_type();
	}
	else if (receivecounter == 7 && data != 0x88) {
		interactstatus = 0;
		receivecounter = 0;
	}
}

//回复动作
void COM_Response(void) {
	uint32_t temp;
	if (interactstatus < 4 || (battsoc < 2 && charging_flag != 0) || step_flag > 0) {
		return;
	}
	switch (interactstatus) {
		case 4:
			if (binding_flag != 1) {
				temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
							 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];
				if (COM_Checksum(temp) == receivebuffer[5]) {
					send_key(temp, device_id);
					interactstatus = 2; //等待接收APP ID
				}
				else {
					COM_Send_Deny(CHECKSUM_ERROR);
					interactstatus = 0; //失败，回到开始阶段
				}
			}
			else {
				COM_Send_Deny(ALREADY_BINED);
				step_flag = 1;
				interactstatus = 0; //失败，回到开始阶段
			}
			break;
		case 5:
			temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
						 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];
			if (COM_Checksum(temp) == receivebuffer[5]) {			
 				binding_flag = 1;
				app_id = temp;
				EEP_Binding_Write();
				COM_Send_Positive();
				step_flag = 1;
				interactstatus = 0; //绑定成功，回到开始阶段
			}
			else {
				COM_Send_Deny(CHECKSUM_ERROR);
				interactstatus = 2; //失败，继续等待接收APP ID
			}
			break;
		case 6:
			if (binding_flag == 1) {
				temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
							 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];
				if (COM_Checksum(temp) == receivebuffer[5]) {
					send_key(temp, app_id);
					interactstatus = 3; //等待类型请求
				}
				else {
					COM_Send_Deny(CHECKSUM_ERROR);
					step_flag = 1;
					interactstatus = 0; //失败，回到开始阶段
				}
			}
			else {
				COM_Send_Deny(NOT_BINED);
				interactstatus = 0; //失败，回到开始阶段
			}
			break;
		case 7:
			switch(receivebuffer[0]) {
				case 0x1A:
					send_steps(walking.current_steps);
					walking.current_steps = 0;
					EEP_WalkStep_Write();
					interactstatus = 3; //成功，继续等待类型请求
					break;
				case 0x2A:
					send_steps(running.current_steps);
					running.current_steps = 0;
					EEP_RunStep_Write();
					interactstatus = 3; //成功，继续等待类型请求
					break;
				case 0x3A:
					send_steps(walking.total_steps);
					interactstatus = 3; //成功，继续等待类型请求
					break;
				case 0x4A:
					send_steps(running.total_steps);
					interactstatus = 3; //成功，继续等待类型请求
					break;
				case 0x5A:
					fillcounter = 0;
					step_flag = 2;
					break;
				case 0x6A:
					fillcounter = 0;
					step_flag = 3;
					break;
			}
			break;
		case 8:
			send_info();
			if (binding_flag == 1) {
				step_flag = 1;
			}
			interactstatus = 0; //回到开始阶段
			break;
	}
}
