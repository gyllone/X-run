// @gyl
#include "functional.h"

const extern uint32_t device_id;

extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];
extern volatile uint32_t app_id;
extern volatile float battvolt;
extern volatile uint8_t battsoc;
extern volatile uint8_t stepflag;
extern volatile uint8_t receivecounter;
extern volatile uint8_t connectstatus;

extern uint8_t eeprom_status;
extern float pressure;
extern float weight;
extern float hanging;
extern Step walking;
extern Step running;

extern uint32_t Crypto_CalcKey(uint32_t wSeed, uint32_t security);

//当前步数阶段
static uint8_t cycleflag = 0;
//上一阶段
static uint8_t cyclelastflag = 0;
//阶段进入计数
static uint8_t cyclecounter = 0;
//超时计数
static uint8_t overtimecounter = 0;

//seed key
static volatile uint32_t seedset[3];
static uint32_t keyset[3];

//checksum
static volatile uint8_t checksum;

//压力5次均值滤波
static void FUNC_pressure_filter(void) {
	uint8_t filtercounter;
	float pressurebuffer;
	for (filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		pressurebuffer = ((float)filtercounter * pressurebuffer + (float) ADC_ConvertedValue[1] / 4096 * 3.3) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure = (1 - PRESSURE_FACTOR) * pressurebuffer + PRESSURE_FACTOR * pressure;
}

//计算SOC
void FUNC_BattSOC_Caculation(void) {
	battvolt = (1 - BATTVOLT_FACTOR) * ((float) ADC_ConvertedValue[0] / 2048 * 3.3) + BATTVOLT_FACTOR * battvolt;
	if (battvolt > 4.2) {
		battsoc = 100;
	}
	else if (battvolt < 3.33) {
		battsoc = 0;
	}
	else {
		battsoc = (uint8_t)((battvolt - 3.33) / 0.87 * 100);
	}
}

//滤波初始化
void FUNC_Functional_Initial(void) {
	uint8_t i;
	for (i = 0; i < 50; i++) {
		FUNC_pressure_filter();
		FUNC_BattSOC_Caculation();
	}
}

//超时reset
static void FUNC_overtime_reset(void) {
	cyclelastflag = cycleflag;
	if (cyclelastflag == 0 && cycleflag > 0) {
		overtimecounter = 1;
	}
	else if (cyclelastflag > 0 && cycleflag > 0) {
		overtimecounter++;
	}
	else if (cyclelastflag > 4 && cycleflag == 0) {
		overtimecounter = 0;
	}
	if (overtimecounter > 200) {
		LED1_OFF;
		LED2_OFF;
		cyclelastflag = 0;
		cycleflag = 0;
		overtimecounter = 0;
		cyclecounter = 0;
	}
}
	
/*                    计步原理

            * * * 
----------*------ *--------------------------------
         *|       |* 
        * |       | * 
-------*--|-------|--*-----------------------------
      *|  |       |  |*
     * |  |       |  | *   ����3/4    �˳�3/4������5/6
----*--|--|-------|--|--*---|-----------|---*------
    0  �� ��      �� ��  *  |           |  *
       �� ��      �� ��   * |           | *
       1  2       2  1     *|           |*
----------------------------*-----------*----------
														 *         *
                              *       *
                                * * *
*/

//计步器
void FUNC_Step_Counter(void) {
	FUNC_pressure_filter();
	//stepflag为1时允许计步
	if (stepflag) {
		FUNC_overtime_reset();
		switch (cycleflag) {
			case 0:
				if (pressure < walking.threshold && pressure >= running.threshold) {
					cycleflag = 1;
				}
				else if (pressure < running.threshold) {
					cycleflag = 2;
				}
				break;
			case 1:
				if (pressure < walking.threshold && pressure >= running.threshold) {
					cyclecounter++;
					if (cyclecounter >= 20) {
						LED1_ON;
						cyclecounter = 0;
						cycleflag = 3;
					}
				}
				else if (pressure < running.threshold) {
					LED1_OFF;
					cyclecounter = cyclecounter / 2;
					cycleflag = 2;
				}
				break;
			case 2:
				if (pressure < running.threshold) {
					cyclecounter++;
					if (cyclecounter >= 10) {
						LED2_ON;
						cyclecounter = 0;
						cycleflag = 4;
					}
				}
				break;
			case 3:
				if (pressure > hanging) {
					cyclecounter++;
					if (cyclecounter >= 20) {
						cyclecounter = 0;
						cycleflag = 5;
					}
				}
				break;
			case 4:
				if (pressure > hanging) {
					cyclecounter++;
					if (cyclecounter >= 10) {
						cyclecounter = 0;
						cycleflag = 6;
					}
				}
				break;
			case 5:
				cycleflag = 0;
				walking.current_steps++;
				walking.total_steps++;
				LED1_OFF;
				break;
			case 6:
				cycleflag = 0;
				running.current_steps++;
				running.total_steps++;
				LED2_OFF;
				break;
		}
	}
}

/*********串口收发部分***************/

//绑定请求监听
void FUNC_Binding_Listen(void) {
	if (USART_ReceiveData(USART1) == 0x11) {
		connectstatus++;
	}
	else {
		connectstatus = 0;
	}
}

//上传请求监听
void FUNC_Upload_Listen(void) {
	uint8_t data = USART_ReceiveData(USART1);
	//SOC、E方状态上传
	if (data == 0x22) {
		connectstatus++;
	}
	//步数、标定数据上传（需通过安全访问）
	else if (data == 0x33) {
		connectstatus = 2;
		stepflag = 0;
	}
}

//计算checksum
static uint8_t checkingsum(uint32_t sum) {
	return (uint8_t)(sum / 1000000000) + (uint8_t)((sum % 1000000000) / 100000000) + (uint8_t)((sum % 100000000) / 10000000) +
				 (uint8_t)((sum % 10000000) / 1000000) + (uint8_t)((sum % 1000000) / 100000) + (uint8_t)((sum % 100000) / 10000) +
				 (uint8_t)((sum % 10000) / 1000) + (uint8_t)((sum % 1000) / 100) + (uint8_t)((sum % 100) / 10) + (uint8_t)(sum % 10);
}

//发送key
void FUNC_SendKey(void) {
	uint8_t i;
	uint8_t data[12];
	uint32_t seedbuffer[3];
	seedbuffer[0] = seedset[0];
	seedbuffer[1] = seedset[1];
	seedbuffer[2] = seedset[2];
	for (i = 0; i < 3; i++) {
		keyset[i] = Crypto_CalcKey(seedbuffer[i], app_id);
		data[4 * i] = (uint8_t)(keyset[i] >> 24);
		data[4 * i] = (uint8_t)(keyset[i] >> 16);
		data[4 * i] = (uint8_t)(keyset[i] >> 8);
		data[4 * i] = (uint8_t)keyset[i];
	}
	Usart_SendArray(USART1, data, 12);
	connectstatus++;
}

//接收种子
void FUNC_ReceiveSeed(void) {
	if (receivecounter == 0 || receivecounter == 4 || receivecounter == 8) {
		seedset[receivecounter / 4] = (uint32_t)(USART_ReceiveData(USART1) << 24);
		receivecounter++;
	}
	else if (receivecounter == 1 || receivecounter == 5 || receivecounter == 9) {
		seedset[receivecounter / 4] = seedset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 16);
		receivecounter++;
	}
	else if (receivecounter == 2 || receivecounter == 6 || receivecounter == 10) {
		seedset[receivecounter / 4] = seedset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 8);
		receivecounter++;
	}
	else if (receivecounter == 3 || receivecounter == 7) {
		seedset[receivecounter / 4] = seedset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 8);
		receivecounter++;
	}
	else if (receivecounter == 11) {
		seedset[receivecounter / 4] = seedset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 8);
		receivecounter = 0;
		connectstatus++;
	}
	else {
		connectstatus = 0;
		receivecounter = 0;
		stepflag = 1;
	}
}

//绑定期间接收app ID
void FUNC_Binding_Receiveid(void) {
	if (receivecounter == 0) {
		app_id = (uint32_t)(USART_ReceiveData(USART1) << 24);
		receivecounter++;
	}
	else if (receivecounter == 1) {
		app_id = app_id | (uint32_t)(USART_ReceiveData(USART1) << 16);
		receivecounter++;
	}
	else if (receivecounter == 2) {
		app_id = app_id | (uint32_t)(USART_ReceiveData(USART1) << 8);
		receivecounter++;
	}
	else if (receivecounter == 3) {
		app_id = app_id | (uint32_t)USART_ReceiveData(USART1);
		connectstatus++;
	}
	else if (receivecounter == 4) {
		checksum = USART_ReceiveData(USART1);
		connectstatus++;
		receivecounter = 0;
	}
	else {
		connectstatus = 0;
		receivecounter = 0;
	}
}

//发送SOC
void FUNC_SendSoc(void) {
	uint8_t data[3];
	data[0] = eeprom_status;
	data[1] = battsoc;
	data[2] = data[0] + data[1];
	Usart_SendArray(USART1, data, 3);
	connectstatus = 0;
}

//发送当前步数
void FUNC_SendCurrentStep(void) {
	uint8_t data[10];
	data[0] = (uint8_t)(walking.current_steps >> 24);
	data[1] = (uint8_t)(walking.current_steps >> 16);
	data[2] = (uint8_t)(walking.current_steps >> 8);
	data[3] = (uint8_t)(walking.current_steps);
	data[4] = checkingsum(walking.current_steps);
	data[5] = (uint8_t)(running.current_steps >> 24);
	data[6] = (uint8_t)(running.current_steps >> 16);
	data[7] = (uint8_t)(running.current_steps >> 8);
	data[8] = (uint8_t)(running.current_steps);
	data[9] = checkingsum(running.current_steps);
	Usart_SendArray(USART1, data, 10);
	connectstatus = 0;
	stepflag = 1;
}

//发送当前总步数
void FUNC_SendTotalStep(void) {
	uint8_t data[10];
	data[0] = (uint8_t)(walking.total_steps >> 24);
	data[1] = (uint8_t)(walking.total_steps >> 16);
	data[2] = (uint8_t)(walking.total_steps >> 8);
	data[3] = (uint8_t)(walking.total_steps);
	data[4] = checkingsum(walking.total_steps);
	data[5] = (uint8_t)(running.total_steps >> 24);
	data[6] = (uint8_t)(running.total_steps >> 16);
	data[7] = (uint8_t)(running.total_steps >> 8);
	data[8] = (uint8_t)(running.total_steps);
	data[9] = checkingsum(running.total_steps);
	Usart_SendArray(USART1, data, 10);
	connectstatus = 0;
	stepflag = 1;
}

//APP ID checksum
uint8_t FUNC_Binding_Idchecksum(void) {
	uint32_t buffer = app_id;
	if (checkingsum(buffer) != checksum) {
		return 0;
	}
	else {
		return 1;
	}
}
