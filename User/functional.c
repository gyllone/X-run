// @gyl
#include "functional.h"

const extern uint32_t device_id;
extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];
extern volatile uint32_t app_id;
extern volatile float battvolt;
extern volatile float pressure;
extern volatile uint8_t battsoc;
extern volatile uint8_t stepflag;
extern volatile uint8_t eeprom_status;
extern volatile Step walking;
extern volatile Step running;
extern volatile float weight;
extern volatile float hanging;

extern uint8_t receivecounter;
extern volatile uint8_t connectstatus;

extern uint32_t Crypto_CalcKey(uint32_t wSeed, uint32_t security);
extern void Crypto_Random(uint32_t *randseedset);

//��ǰcycle���еĽ׶α�־
static uint8_t cycleflag = 0;
//�ϸ�cycle���еĽ׶α�־
static uint8_t cyclelastflag = 0;
//��ǰcycle��ĳ���׶γ���������
static uint8_t cyclecounter = 0;
//��ǰcycle�ĳ�ʱ������
static uint8_t overtimecounter = 0;

//seed key
static uint32_t seedset[3];
static uint32_t keyset[3];

//ѹ���˲�������2ms���ȡ5����ѹ����ֵ��Ȼ����һ���ͺ��˲�
static void FUNC_pressure_filter(void) {
	uint8_t filtercounter;
	float pressurebuffer;
	for (filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		pressurebuffer = ((float)filtercounter * pressurebuffer + (float) ADC_ConvertedValue[2] / 4096 * 3.3) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure = (1 - PRESSURE_FACTOR) * pressurebuffer + PRESSURE_FACTOR * pressure;
}

//���SOC����
void FUNC_battSOC_caculation(void) {
	battvolt = (1 - BATTVOLT_FACTOR) * ((float) ADC_ConvertedValue[1] / 2048 * 3.3) + BATTVOLT_FACTOR * battvolt;
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

//�����Գ�ʼ��
void FUNC_functional_initial(void) {
	uint8_t i;
	for (i = 0; i < 50; i++) {
		FUNC_pressure_filter();
		FUNC_battSOC_caculation();
	}
}

//cycle��ʱ�жϣ�����100������reset
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
	
/*                    �Ʋ�ͼ��

            * * * 
----------*------ *--------------------------------�ܲ�ѹ��
         *|       |* 
        * |       | * 
-------*--|-------|--*-----------------------------��·ѹ��
      *|  |       |  |*
     * |  |       |  | *   ����3/4    �˳�3/4������5/6�����һ��Cycle���ع�0
----*--|--|-------|--|--*---|-----------|---*------������
    0  �� ��      �� ��  *  |           |  *
       �� ��      �� ��   * |           | *
       1  2       2  1     *|           |*
----------------------------*-----------*----------����ѹ����
														 *         *
                              *       *
                                * * *
*/
//stepflag = 0ʱ�����Ʋ�����
void FUNC_step_counter(void) {
	FUNC_pressure_filter();
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

/****************
*****************
��������ͨѶ����
*****************
*****************/

//����������
void Usart_binding_listen(void) {
	if (USART_ReceiveData(USART1) == 0x11) {
		connectstatus++;
	}
	else {
		connectstatus = 0;
	}
}

//�����ϴ�����
void Usart_upload_listen(void) {
	//��ͨ����
	if (USART_ReceiveData(USART1) == 0x22) {
		connectstatus++;
	}
	//��Ҫ��ȫ����
	else if (USART_ReceiveData(USART1) == 0x33) {
		connectstatus = 3;
		stepflag = 0;
	}
}

//����seed
void Usart_binding_sendseed(void) {
	uint8_t i;
	uint8_t data[12];
	Crypto_Random(seedset);
	for (i = 0; i < 3; i++) {
		data[4 * i] = (uint8_t)(seedset[i] >> 24);
		data[4 * i] = (uint8_t)(seedset[i] >> 16);
		data[4 * i] = (uint8_t)(seedset[i] >> 8);
		data[4 * i] = (uint8_t)seedset[i];
	}
	Usart_SendArray(USART1, data, 12);
	connectstatus++;
}

//����key
void Usart_sendkey(void) {
	uint8_t i;
	uint8_t data[12];
	for (i = 0; i < 3; i++) {
		keyset[i] = Crypto_CalcKey(seedset[i], app_id);
		data[4 * i] = (uint8_t)(keyset[i] >> 24);
		data[4 * i] = (uint8_t)(keyset[i] >> 16);
		data[4 * i] = (uint8_t)(keyset[i] >> 8);
		data[4 * i] = (uint8_t)keyset[i];
	}
	Usart_SendArray(USART1, data, 12);
	connectstatus++;
}

//����key
void Usart_binding_receivekey(void) {
	if (receivecounter == 0 || receivecounter == 4 || receivecounter == 8) {
		keyset[receivecounter / 4] = (uint32_t)(USART_ReceiveData(USART1) << 24);
		receivecounter++;
	}
	else if (receivecounter == 1 || receivecounter == 5 || receivecounter == 9) {
		keyset[receivecounter / 4] = keyset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 16);
		receivecounter++;
	}
	else if (receivecounter == 2 || receivecounter == 6 || receivecounter == 10) {
		keyset[receivecounter / 4] = keyset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 8);
		receivecounter++;
	}
	else if (receivecounter == 3 || receivecounter == 7) {
		keyset[receivecounter / 4] = keyset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 8);
		receivecounter++;
	}
	else if (receivecounter == 11) {
		keyset[receivecounter / 4] = keyset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 8);
		receivecounter = 0;
		connectstatus++;
	}
	else {
		connectstatus = 0;
		receivecounter = 0;
	}
}

//����seed
void Usart_receiveseed(void) {
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
		stepflag = 1; //�Ʋ�����
	}
}

//��֤seed-key
void Usart_binding_verify(void) {
	uint8_t i;
	for (i = 0; i < 3; i++) {
		if (Crypto_CalcKey(seedset[i], device_id) != keyset[i]) {
			Usart_SendByte(USART1, 0xFF); //��֤��ͨ��
			connectstatus = 0;
			return;
		}
	}
	Usart_SendByte(USART1, 0xAA); //��֤ͨ��
	connectstatus++;
}

//����APP ID, ��0xBB��ͷ��0xCC��β
void Usart_binding_receiveid(void) {
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
		receivecounter = 0;
	}
	else {
		connectstatus = 0;
		receivecounter = 0;
	}
}

//�ϴ�SOC
void Usart_sendsoc(void) {
	Usart_SendByte(USART1, eeprom_status);
	Usart_SendByte(USART1, battsoc);
	connectstatus = 0;
}

//�ϴ�����
void Usart_sendstep(void) {
	uint8_t data[16];
	data[0] = (uint8_t)(walking.current_steps >> 24);
	data[1] = (uint8_t)(walking.current_steps >> 16);
	data[2] = (uint8_t)(walking.current_steps >> 8);
	data[3] = (uint8_t)(walking.current_steps);
	data[4] = (uint8_t)(walking.total_steps >> 24);
	data[5] = (uint8_t)(walking.total_steps >> 16);
	data[6] = (uint8_t)(walking.total_steps >> 8);
	data[7] = (uint8_t)(walking.total_steps);
	data[8] = (uint8_t)(running.current_steps >> 24);
	data[9] = (uint8_t)(running.current_steps >> 16);
	data[10] = (uint8_t)(running.current_steps >> 8);
	data[11] = (uint8_t)(running.current_steps);
	data[12] = (uint8_t)(running.total_steps >> 24);
	data[13] = (uint8_t)(running.total_steps >> 16);
	data[14] = (uint8_t)(running.total_steps >> 8);
	data[15] = (uint8_t)(running.total_steps);
	Usart_SendArray(USART1, data, 16);
	connectstatus = 0;
	stepflag = 1;
}
