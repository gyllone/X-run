// @gyl
#include "communication.h"

const extern uint32_t device_id;
const extern uint8_t sw_version;

extern uint32_t app_id;
extern uint8_t binding_flag;
extern uint8_t eeprom_status;
extern Step walking;
extern Step running;
extern float weight;
extern float hanging;
extern float pressure;

extern volatile uint8_t step_flag;
extern volatile uint8_t interactstatus;
extern volatile uint8_t charging_flag;
extern volatile uint8_t displaysoc;
extern volatile float actualsoc;

//上一cycle状态
static uint8_t lastinteractstatus = 0;
//超时计数
static uint16_t overtimecounter = 0;
//数据接收缓存
static volatile uint8_t receivebuffer[6];
//接收计数
static volatile uint8_t receivecounter = 0;

extern uint32_t Crypto_CalcKey(uint32_t wSeed, uint32_t security);

//发送key
static void send_key(uint32_t useed, uint32_t security) {
	uint32_t tempkey;
	uint8_t sendbuffer[8];
	tempkey = Crypto_CalcKey(useed, security);
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
static void send_deny(uint8_t error) {
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
static void send_positive(void) {
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
	sendbuffer[2] = sw_version;
	sendbuffer[3] = step_flag << 4 | eeprom_status;
	sendbuffer[4] = charging_flag << 4 | binding_flag;
	sendbuffer[5] = displaysoc;
	sendbuffer[6] = sw_version + (step_flag << 4 | eeprom_status) + (charging_flag << 4 | binding_flag) + displaysoc;
	sendbuffer[7] = 0x88;
	Usart_SendArray(USART1, sendbuffer, 8);
}

//检查标定值是否合法
static void weight_isvalid(void) {
	if (weight < MAX_WEIGHT && weight > MIN_WEIGHT) {
		weight = pressure;
		walking.threshold = WALK_WEIGHT_RATIO * weight;
		running.threshold = RUN_WEIGHT_RATIO * weight;
		EEP_Weight_Write();
		send_positive();
		interactstatus = 3; //标定成功，继续等待类型请求
	}
	else {
		send_deny(INVALID_WEIGHT);
		interactstatus = 3; //标定失败，继续等待类型请求
	}
}

//检查标定值是否合法
static void hang_isvalid(void) {
	if (pressure * HANG_RATIO < MAX_HANG && pressure * HANG_RATIO > MIN_HANG) {
		hanging = pressure * HANG_RATIO;
		EEP_Hang_Write();
		send_positive();
		interactstatus = 3; //标定成功，继续等待类型请求
	}
	else {
		send_deny(INVALID_HANG);
		interactstatus = 3; //标定失败，继续等待类型请求
	}
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
				interactstatus = 0;
				send_deny(INVALID_TYPE);
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
				send_deny(INVALID_TYPE);
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
				send_deny(INVALID_TYPE);
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

//侦听复位(超过5s复位)
void COM_Listen_Reset(void) {
	if (lastinteractstatus < 1 && interactstatus > 0) {
		overtimecounter = 1;
	}
	else if (lastinteractstatus > 0 && interactstatus < 1) {
		overtimecounter = 0;
	}
	else {
		overtimecounter++;
	}
	if (overtimecounter > 1000) {
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
	if (interactstatus > 3 || (actualsoc < 3 && charging_flag < 1)) {
		return;
	}
	if (receivecounter == 0 && data == 0x66) {
		if (interactstatus < 1) {
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
	if (interactstatus < 4 || (actualsoc < 3 && charging_flag < 1)) {
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
					send_deny(CHECKSUM_ERROR);
					interactstatus = 0; //失败，回到开始阶段
				}
			}
			else {
				send_deny(ALREADY_BINED);
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
				send_positive();
				step_flag = 1;
				interactstatus = 0; //绑定成功，回到开始阶段
			}
			else {
				send_deny(CHECKSUM_ERROR);
				interactstatus = 2; //失败，继续等待接收APP ID
			}
			break;
		case 6:
			if (binding_flag == 1) {
				temp = (uint32_t)receivebuffer[1] << 24 | (uint32_t)receivebuffer[2] << 16 |
							 (uint32_t)receivebuffer[3] << 8 | (uint32_t)receivebuffer[4];
				if (COM_Checksum(temp) == receivebuffer[5]) {
					step_flag = 0;
					send_key(temp, app_id);
					interactstatus = 3; //等待类型请求
				}
				else {
					send_deny(CHECKSUM_ERROR);
					interactstatus = 0; //失败，回到开始阶段
				}
			}
			else {
				send_deny(NOT_BINED);
				interactstatus = 0; //失败，回到开始阶段
			}
			break;
		case 7:
			switch(receivebuffer[0]) {
				case 0x1A:
					walking.current_steps = 0;
					EEP_WalkStep_Write();
					send_steps(walking.current_steps);
					interactstatus = 3; //成功，继续等待类型请求
					break;
				case 0x2A:
					running.current_steps = 0;
					EEP_RunStep_Write();
					send_steps(running.current_steps);
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
					weight_isvalid();
					break;
				case 0x6A:
					hang_isvalid();
					break;
			}
			break;
		case 8:
			send_info();
			interactstatus = 0; //回到开始阶段
			break;
	}
}
