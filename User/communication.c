// @gyl
#include "communication.h"
#include "functional.h"
#include "eeprom.h"

const extern uint32_t activation_code;
const extern uint16_t sw_version;

extern volatile uint8_t charging_flag;
extern volatile uint8_t response_flag;
extern volatile float battsoc;

extern uint8_t step_flag;
extern uint8_t binding_flag;
extern uint8_t eeprom_status;
extern uint16_t fillcounter;
extern Step walking;
extern Step running;

//数据接收缓存
static volatile uint8_t receivebuffer[6];
//锁
static volatile uint8_t mutex = 0;
//接收计数
static uint8_t receivecounter = 0;
//上一cycle状态
static uint8_t last_response_flag = 0;
//超时计数
static uint16_t overtimecounter = 0;
//seed buffer
static uint32_t seedbuffer = 0;
//key buffer
static uint32_t keybuffer = 0;

//计算安全密钥
static uint32_t crypto_key(uint32_t wSeed) {
  uint32_t   wTemp;
  uint32_t   wLSBit;
  uint32_t   wTop31Bits;
	uint32_t wLastSeed = wSeed;
	
  uint32_t temp = (uint32_t)((activation_code & 0x00000800) >> 10) | ((activation_code & 0x00200000)>> 21);   
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
	uint8_t SB1 = (uint8_t)((activation_code & 0x000003FC) >> 2);
  uint8_t SB2 = (uint8_t)(((activation_code & 0x7F800000) >> 23) ^ 0xA5);
  uint8_t SB3 = (uint8_t)(((activation_code & 0x001FE000) >> 13) ^ 0x5A);
	
	uint8_t iterations = (uint8_t)(((wTemp ^ SB1) & SB2)  + SB3);
	for (uint8_t i = 0; i < iterations; i++) {
		wTemp = ((wLastSeed & 0x40000000)/0x40000000) ^ ((wLastSeed & 0x01000000)/0x01000000) ^
						((wLastSeed & 0x1000)/0x1000) ^ ((wLastSeed & 0x04)/0x04) ;
		wLSBit = (wTemp & 0x00000001) ;
    wLastSeed  = (uint8_t)(wLastSeed << 1); /* Left Shift the bits */
    wTop31Bits = (uint8_t)(wLastSeed & 0xFFFFFFFE) ;    
    wLastSeed  = (uint8_t)(wTop31Bits | wLSBit);
  }

	if (activation_code & 0x00000001) {
		wTop31Bits = ((wLastSeed & 0x00FF0000)>>16) |  /*KEY[0] = Last_Seed[1]*/ 
                 ((wLastSeed & 0xFF000000)>>8)  |  /*KEY[1] = Last_Seed[0]*/ 
                 ((wLastSeed & 0x000000FF)<<8)  |  /*KEY[2] = Last_Seed[3]*/
                 ((wLastSeed & 0x0000FF00)<<16);   /*KEY[3] = Last_Seed[2]*/
	}
	else {
		wTop31Bits = wLastSeed;
	}
  wTop31Bits = wTop31Bits ^ activation_code;  
  return wTop31Bits;
}

//上传数据
static void send_value(uint32_t value) {
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

//计算checksum
uint8_t COM_Checksum(uint32_t value) {
	return (uint8_t)(value / 1000000000) + (uint8_t)((value % 1000000000) / 100000000) + (uint8_t)((value % 100000000) / 10000000) +
				 (uint8_t)((value % 10000000) / 1000000) + (uint8_t)((value % 1000000) / 100000) + (uint8_t)((value % 100000) / 10000) +
				 (uint8_t)((value % 10000) / 1000) + (uint8_t)((value % 1000) / 100) + (uint8_t)((value % 100) / 10) + (uint8_t)(value % 10);
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

//侦听复位(超过5s复位)
void COM_Listen_Reset(void) {
	if (step_flag > 1) {
		return;
	}
	if (last_response_flag == 0 && response_flag > 0) {
		overtimecounter = 1;
	}
	else if (last_response_flag > 0 && response_flag == 0) {
		overtimecounter = 0;
	}
	else {
		overtimecounter++;
	}
	if (overtimecounter > 500) {
		overtimecounter = 0;
		receivecounter = 0;
		response_flag = 0;
		last_response_flag = 0;
		if (binding_flag == 1) {
			step_flag = 1;
		}
		mutex = 0;
	}
	last_response_flag = response_flag;
}

//侦听帧
void COM_Listen(void) {
	if (mutex > 0 || (battsoc < 2 && charging_flag != 0) || step_flag > 1) {
		return;
	}
	uint8_t data = USART_ReceiveData(USART1);
	if (receivecounter == 0 && data == 0x66) {
		if (response_flag == 0) {
			response_flag = 1;
		}
		receivecounter++;
	}
	else if (receivecounter > 0 && receivecounter < 7) {
		receivebuffer[receivecounter - 1] = data;
		receivecounter++;
	}
	else if (receivecounter == 7 && data == 0x88) {
		receivecounter = 0;
		mutex = 1;
	}
	else if (receivecounter == 7 && data != 0x88) {
		receivecounter = 0;
	}
}

void COM_Response(void) {
	uint32_t temp;
	if (mutex != 1 || step_flag > 1) {
		return;
	}
	step_flag = 0;
	fillcounter = 0;
	switch (response_flag) {
		case 1:
			switch (receivebuffer[0]) {
				case 0x11:
					if (binding_flag != 1) {
						temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
									 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];					
						if (COM_Checksum(temp) == receivebuffer[5]) {
							keybuffer = crypto_key(temp);					
							send_value(keybuffer); //发送key
							response_flag = 2;
						}
						else {
							COM_Send_Deny(CHECKSUM_ERROR);
						}
					}
					else {
						COM_Send_Deny(ALREADY_BINED);
						response_flag = 0;
						step_flag = 1;
					}
					break;
				case 0x22:
					if (binding_flag == 1) {
						temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
									 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];
						if (COM_Checksum(temp) == receivebuffer[5]) {
							keybuffer = crypto_key(temp);
							send_value(keybuffer); //发送key
							response_flag = 4;
						}
						else {
							COM_Send_Deny(CHECKSUM_ERROR);
						}
					}
					else {
						COM_Send_Deny(NOT_BINED);
						response_flag = 0;
					}
					break;
				case 0x33:
					if (binding_flag == 1) {
						seedbuffer = RTC_GetCounter();
						send_value(seedbuffer); //发送seed
						keybuffer = crypto_key(seedbuffer);
						response_flag = 5;
					}
					else {
						COM_Send_Deny(NOT_BINED);
						response_flag = 0;
					}
					break;
				case 0x44:
					send_info();
					response_flag = 0;
					if (binding_flag == 1) {
						step_flag = 1;
					}
					break;
				default:
					COM_Send_Deny(INVALID_TYPE);
					response_flag = 0;
					if (binding_flag == 1) {
						step_flag = 1;
					}
					break;
			}
			break;
		case 2:
			if (receivebuffer[0] == 0xAA) {
				seedbuffer = RTC_GetCounter();
				send_value(seedbuffer);
				keybuffer = crypto_key(seedbuffer);
				response_flag = 3;
			}
			else if (receivebuffer[0] == 0xFF) {
				send_value(keybuffer);
			}
			else {
				COM_Send_Deny(INVALID_TYPE);
			}
			break;
		case 3:
			if (receivebuffer[0] == 0xAA) {
				temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
							 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];
				if (COM_Checksum(temp) == receivebuffer[5]) {
					if (keybuffer == temp) {
						COM_Send_Positive();
						binding_flag = 1;
						EEP_Binding_Write();
						step_flag = 1;
					}
					else {
						COM_Send_Deny(INVALID_KEY);
					}
					response_flag = 0;
				}
				else {
					COM_Send_Deny(CHECKSUM_ERROR);
				}
			}
			else if (receivebuffer[0] == 0xFF) {
				send_value(seedbuffer);
			}
			else {
				COM_Send_Deny(INVALID_TYPE);
			}
			break;
		case 4:
			switch (receivebuffer[0]) {
				case 0x1A:
					send_value(walking.total_steps);
					break;
				case 0x2A:
					send_value(running.total_steps);
					break;
				case 0xAA:
					COM_Send_Positive();
					response_flag = 0;
					step_flag = 1;
					break;
				case 0xFF:
					send_value(keybuffer);
					break;
				default:
					COM_Send_Deny(INVALID_TYPE);
			}
			break;
		case 5:
			if (receivebuffer[0] == 0x3A || receivebuffer[0] == 0x4A) {
				temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
							 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];
				if (COM_Checksum(temp) == receivebuffer[5]) {
					if (keybuffer == temp) {
						step_flag = (receivebuffer[0] - 0x3A) / 16 + 2;
					}
					else {
						COM_Send_Deny(INVALID_KEY);
						response_flag = 0;
						step_flag = 1;
					}
				}
				else {
					COM_Send_Deny(CHECKSUM_ERROR);
				}
			}
			else if (receivebuffer[0] == 0xAA) {
				COM_Send_Positive();
				response_flag = 0;
				step_flag = 1;
			}
			else if (receivebuffer[0] == 0xFF) {
				send_value(seedbuffer);
			}
			else {
				COM_Send_Deny(INVALID_TYPE);
			}
			break;
	}
	mutex = 0;
}
