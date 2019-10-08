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

<<<<<<< Updated upstream
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
=======
extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];
extern uint8_t step_flag;
extern uint16_t fillcounter;
extern Step walking;
extern Step running;
extern float pressure;
extern float hanging;

//休眠计数
static uint16_t standingcounter = 0;
static complex A1[1024];
static complex A2[1024];
static float Value[99];
float amp_offset = 0;
amplitude amp_1 = {0, 0};
amplitude amp_2 = {0, 0};

static void fft_stepfrequency(void) {
	amp_1.value = 0;
	amp_2.value = 0;
	for (uint8_t q = 1; q < 11; q++) {
		switch (q) {
			case 1:
				for (uint16_t k = 0; k < 512; k++) {
					A2[2 * k].re = A1[k].re + A1[k + 512].re;
					A2[2 * k].im = 0;
					A2[2 * k + 1].re = (A1[k].re - A1[k + 512].re) * cosine[k];
					A2[2 * k + 1].im = (A1[k].re - A1[k + 512].re) * sine[k];
				}
				break;
			case 2:
				for (uint16_t k = 0; k < 256; k++) {
					for (uint16_t j = 0; j < 2; j++) {
						A1[4 * k + j].re = A2[2 * k + j].re + A2[2 * k + j + 512].re;
						A1[4 * k + j].im = A2[2 * k + j].im + A2[2 * k + j + 512].im;				
						A1[4 * k + j + 2].re = ProductRe(A2[2 * k + j].re - A2[2 * k + j + 512].re, A2[2 * k + j].im - A2[2 * k + j + 512].im, cosine[2 * k], sine[2 * k]);
						A1[4 * k + j + 2].im = ProductIm(A2[2 * k + j].re - A2[2 * k + j + 512].re, A2[2 * k + j].im - A2[2 * k + j + 512].im, cosine[2 * k], sine[2 * k]);
>>>>>>> Stashed changes
					}
				}
				break;
			case 3:
<<<<<<< Updated upstream
				if (pressure > hanging) {
					cyclecounter++;
					if (cyclecounter >= 20) {
						cyclecounter = 0;
						cycleflag = 5;
=======
				for (uint16_t k = 0; k < 128; k++) {
					for (uint16_t j = 0; j < 4; j++) {
						A2[8 * k + j].re = A1[4 * k + j].re + A1[4 * k + j + 512].re;
						A2[8 * k + j].im = A1[4 * k + j].im + A1[4 * k + j + 512].im;
						A2[8 * k + j + 4].re = ProductRe(A1[4 * k + j].re - A1[4 * k + j + 512].re, A1[4 * k + j].im - A1[4 * k + j + 512].im, cosine[4 * k], sine[4 * k]);
						A2[8 * k + j + 4].im = ProductIm(A1[4 * k + j].re - A1[4 * k + j + 512].re, A1[4 * k + j].im - A1[4 * k + j + 512].im, cosine[4 * k], sine[4 * k]);
>>>>>>> Stashed changes
					}
				}
				break;
			case 4:
<<<<<<< Updated upstream
				if (pressure > hanging) {
					cyclecounter++;
					if (cyclecounter >= 10) {
						cyclecounter = 0;
						cycleflag = 6;
=======
				for (uint16_t k = 0; k < 64; k++) {
					for (uint16_t j = 0; j < 8; j++) {
						A1[16 * k + j].re = A2[8 * k + j].re + A2[8 * k + j + 512].re;
						A1[16 * k + j].im = A2[8 * k + j].im + A2[8 * k + j + 512].im;				
						A1[16 * k + j + 8].re = ProductRe(A2[8 * k + j].re - A2[8 * k + j + 512].re, A2[8 * k + j].im - A2[8 * k + j + 512].im, cosine[8 * k], sine[8 * k]);
						A1[16 * k + j + 8].im = ProductIm(A2[8 * k + j].re - A2[8 * k + j + 512].re, A2[8 * k + j].im - A2[8 * k + j + 512].im, cosine[8 * k], sine[8 * k]);
>>>>>>> Stashed changes
					}
				}
				break;
			case 5:
<<<<<<< Updated upstream
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
=======
				for (uint16_t k = 0; k < 32; k++) {
					for (uint16_t j = 0; j < 16; j++) {
						A2[32 * k + j].re = A1[16 * k + j].re + A1[16 * k + j + 512].re;
						A2[32 * k + j].im = A1[16 * k + j].im + A1[16 * k + j + 512].im;
						A2[32 * k + j + 16].re = ProductRe(A1[16 * k + j].re - A1[16 * k + j + 512].re, A1[16 * k + j].im - A1[16 * k + j + 512].im, cosine[16 * k], sine[16 * k]);
						A2[32 * k + j + 16].im = ProductIm(A1[16 * k + j].re - A1[16 * k + j + 512].re, A1[16 * k + j].im - A1[16 * k + j + 512].im, cosine[16 * k], sine[16 * k]);
					}
				}
				break;
			case 6:
				for (uint16_t k = 0; k < 16; k++) {
					for (uint16_t j = 0; j < 32; j++) {
						A1[64 * k + j].re = A2[32 * k + j].re + A2[32 * k + j + 512].re;
						A1[64 * k + j].im = A2[32 * k + j].im + A2[32 * k + j + 512].im;				
						A1[64 * k + j + 32].re = ProductRe(A2[32 * k + j].re - A2[32 * k + j + 512].re, A2[32 * k + j].im - A2[32 * k + j + 512].im, cosine[32 * k], sine[32 * k]);
						A1[64 * k + j + 32].im = ProductIm(A2[32 * k + j].re - A2[32 * k + j + 512].re, A2[32 * k + j].im - A2[32 * k + j + 512].im, cosine[32 * k], sine[32 * k]);
					}
				}
				break;
			case 7:
				for (uint16_t k = 0; k < 8; k++) {
					for (uint16_t j = 0; j < 64; j++) {
						A2[128 * k + j].re = A1[64 * k + j].re + A1[64 * k + j + 512].re;
						A2[128 * k + j].im = A1[64 * k + j].im + A1[64 * k + j + 512].im;
						A2[128 * k + j + 64].re = ProductRe(A1[64 * k + j].re - A1[64 * k + j + 512].re, A1[64 * k + j].im - A1[64 * k + j + 512].im, cosine[64 * k], sine[64 * k]);
						A2[128 * k + j + 64].im = ProductIm(A1[64 * k + j].re - A1[64 * k + j + 512].re, A1[64 * k + j].im - A1[64 * k + j + 512].im, cosine[64 * k], sine[64 * k]);
					}
				}
				break;
			case 8:
				for (uint16_t k = 0; k < 4; k++) {
					for (uint16_t j = 0; j < 128; j++) {
						A1[256 * k + j].re = A2[128 * k + j].re + A2[128 * k + j + 512].re;
						A1[256 * k + j].im = A2[128 * k + j].im + A2[128 * k + j + 512].im;
						A1[256 * k + j + 128].re = ProductRe(A2[128 * k + j].re - A2[128 * k + j + 512].re, A2[128 * k + j].im - A2[128 * k + j + 512].im, cosine[128 * k], sine[128 * k]);
						A1[256 * k + j + 128].im = ProductIm(A2[128 * k + j].re - A2[128 * k + j + 512].re, A2[128 * k + j].im - A2[128 * k + j + 512].im, cosine[128 * k], sine[128 * k]);
					}
				}
				break;
			case 9:
				for (uint16_t k = 0; k < 2; k++) {
					for (uint16_t j = 0; j < 256; j++) {
						A2[512 * k + j].re = A1[256 * k + j].re + A1[256 * k + j + 512].re;
						A2[512 * k + j].im = A1[256 * k + j].im + A1[256 * k + j + 512].im;
						A2[512 * k + j + 256].re = ProductRe(A1[256 * k + j].re - A1[256 * k + j + 512].re, A1[256 * k + j].im - A1[256 * k + j + 512].im, cosine[256 * k], sine[256 * k]);
						A2[512 * k + j + 256].im = ProductIm(A1[256 * k + j].re - A1[256 * k + j + 512].re, A1[256 * k + j].im - A1[256 * k + j + 512].im, cosine[256 * k], sine[256 * k]);
					}
				}
				break;
			case 10:
				A1[0].re = A2[0].re + A2[512].re;
				for (uint16_t j = 1; j < 100; j++) {
					A1[j].re = A2[j].re + A2[j + 512].re;
					A1[j].im = A2[j].im + A2[j + 512].im;
				}
				break;
		}
	}
	amp_offset = A1[0].re / 1024;
	for (uint16_t k = 1; k < 100; k++) {
		Value[k - 1] = sqrt((A1[k].re / 512) * (A1[k].re / 512) + (A1[k].im / 512) * (A1[k].im / 512));
		if (Value[k - 1] > amp_1.value) {
			amp_1.value = Value[k - 1];
			amp_1.index = k;
		}
	}
	for (uint16_t k = 1; k < 100; k++) {
		if (Value[k - 1] > amp_2.value && Value[k - 1] < amp_1.value) {
			amp_2.value = Value[k - 1];
			amp_2.index = k;
		}
	}
	fillcounter = 0;
}
//计步
static void step_counter(void) {
	if (amp_1.value >= running.threshold) {
		float tempstep = (amp_1.value * amp_1.index + amp_2.value * amp_2.index) / (amp_1.value + amp_2.value);
		if (tempstep - (float)(uint32_t)tempstep >= 0.5f) {
			running.total_steps += (uint32_t)tempstep + 1;
		}
		else {
			running.total_steps += (uint32_t)tempstep;
		}
	}
	else if (amp_1.value >= walking.threshold && amp_1.value < running.threshold) {
		float tempstep = (amp_1.value * amp_1.index + amp_2.value * amp_2.index) / (amp_1.value + amp_2.value);
		if (tempstep - (float)(uint32_t)tempstep >= 0.5f) {
			walking.total_steps += (uint32_t)tempstep + 1;
		}
		else {
			walking.total_steps += (uint32_t)tempstep;
		}
	}
}

//步数标定
static void step_calibration(void) {
	if (amp_1.value < WEIGHT_AMP_THRESHOLD && amp_offset < MAX_WEIGHT) {
		walking.threshold = (hanging - amp_offset) * WALK_AMP_RATIO;
		running.threshold = (hanging - amp_offset) * RUN_AMP_RATIO;
		EEP_StepCalibration_Write();
		COM_Send_Positive();
>>>>>>> Stashed changes
	}
	else {
		connectstatus = 0;
	}
}

<<<<<<< Updated upstream
//�����ϴ�����
void Usart_upload_listen(void) {
	//��ͨ����
	if (USART_ReceiveData(USART1) == 0x22) {
		connectstatus++;
=======
//静置标定
static void hang_calibration(void) {
	if (amp_1.value < HANG_AMP_THRESHOLD && amp_offset > MIN_HANG) {
		hanging = amp_offset;
		EEP_HangCalibration_Write();
		COM_Send_Positive();
>>>>>>> Stashed changes
	}
	//��Ҫ��ȫ����
	else if (USART_ReceiveData(USART1) == 0x33) {
		connectstatus = 3;
		stepflag = 0;
	}
}

<<<<<<< Updated upstream
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
=======
//SOC估算
static float caculate_soc(void) {
	float soc;
	if (battvolt > 4.2f) {
		soc = 100;
	}
	else if (battvolt < 3.35f) {
		soc = 0.021f;
	}
	else {
		soc = (battvolt - 3.35f) / 0.85f * 100;
	}
	return soc;
}

static void get_pressureset(void) {
	A1[fillcounter].re = pressure;
	fillcounter++;
}

//呼吸灯
void FUNC_Led_Breath(void) {
	if (GPIO_ReadOutputDataBit(LED2_GPIO_PORT, LED2_GPIO_PIN) == (uint8_t)Bit_SET) {
		LED2_ON;
	}
	else {
		LED2_OFF;
	}
}

//充电与否判断
void FUNC_ChargeOrNot(void) {
	if ((float)ADC_ConvertedValue[2] / 2048 * 3.3f > 4.35f) {
		charging_flag = 1;
>>>>>>> Stashed changes
	}
	else if (receivecounter == 1 || receivecounter == 5 || receivecounter == 9) {
		keyset[receivecounter / 4] = keyset[receivecounter / 4] | (uint32_t)(USART_ReceiveData(USART1) << 16);
		receivecounter++;
	}
<<<<<<< Updated upstream
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
=======
}

//SOC修正
void FUNC_BattSOC_Caculation(void) {
	//电压补偿
	battvolt = (1 - BATTVOLT_FACTOR) * ((float) ADC_ConvertedValue[0] / 2048 * 3.21f + 0.13f) + BATTVOLT_FACTOR * battvolt;
	if (charging_flag == 1) {
		if (caculate_soc() > battsoc) {
			battsoc += 0.02f;
		}
>>>>>>> Stashed changes
	}
	else {
		connectstatus = 0;
		receivecounter = 0;
	}
}

<<<<<<< Updated upstream
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
=======
//滤波初始化
void FUNC_Functional_Initial(void) {
	for (uint8_t i = 0; i < 100; i++) {
		battvolt = (1 - INITIAL_FACTOR) * ((float) ADC_ConvertedValue[0] / 2048 * 3.21f + 0.13f) + INITIAL_FACTOR * battvolt;
		pressure = (1 - INITIAL_FACTOR) * (float) ADC_ConvertedValue[1] / 4096 * 3.3f + INITIAL_FACTOR * pressure;
		INITIALDELAY;
	}
	if (caculate_soc() - battsoc < -6 || caculate_soc() - battsoc > 6) {
		battsoc = caculate_soc();
	}
}

//判断是否进入待机
uint8_t FUNC_SleepOrNot(void) {
	//SOC低于1休眠
	if (battsoc < 1) {
		return 0;
	}
	//非静置状态、通讯状态、充电状态不休眠
	if (pressure > HANG_RATIO * hanging && response_flag < 1 && charging_flag != 1) {
		standingcounter++;
		//40s后进入待机
		if (standingcounter > 4000) {
			return 0;
		}
>>>>>>> Stashed changes
	}
	else {
		connectstatus = 0;
		receivecounter = 0;
		stepflag = 1; //�Ʋ�����
	}
}

<<<<<<< Updated upstream
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
=======
//采样值5次均值滤波 + 1次时滞滤波
void FUNC_Pressure_Filter(void) {
	float temppressure;
	for (uint8_t filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		temppressure = ((float)filtercounter * temppressure + (float) ADC_ConvertedValue[1] / 4096 * 3.3f) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure = (1 - PRESSURE_FACTOR) * temppressure + PRESSURE_FACTOR * pressure;
>>>>>>> Stashed changes
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
