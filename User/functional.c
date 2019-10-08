// @gyl
#include "functional.h"
#include "bsp_adc.h"
#include "math.h"
#include "eeprom.h"
#include "trigonometric.h"
#include "communication.h"

extern volatile uint8_t charging_flag;
extern volatile uint8_t response_flag;
extern volatile float battsoc;
extern volatile float battvolt;

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
					}
				}
				break;
			case 3:
				for (uint16_t k = 0; k < 128; k++) {
					for (uint16_t j = 0; j < 4; j++) {
						A2[8 * k + j].re = A1[4 * k + j].re + A1[4 * k + j + 512].re;
						A2[8 * k + j].im = A1[4 * k + j].im + A1[4 * k + j + 512].im;
						A2[8 * k + j + 4].re = ProductRe(A1[4 * k + j].re - A1[4 * k + j + 512].re, A1[4 * k + j].im - A1[4 * k + j + 512].im, cosine[4 * k], sine[4 * k]);
						A2[8 * k + j + 4].im = ProductIm(A1[4 * k + j].re - A1[4 * k + j + 512].re, A1[4 * k + j].im - A1[4 * k + j + 512].im, cosine[4 * k], sine[4 * k]);
					}
				}
				break;
			case 4:
				for (uint16_t k = 0; k < 64; k++) {
					for (uint16_t j = 0; j < 8; j++) {
						A1[16 * k + j].re = A2[8 * k + j].re + A2[8 * k + j + 512].re;
						A1[16 * k + j].im = A2[8 * k + j].im + A2[8 * k + j + 512].im;				
						A1[16 * k + j + 8].re = ProductRe(A2[8 * k + j].re - A2[8 * k + j + 512].re, A2[8 * k + j].im - A2[8 * k + j + 512].im, cosine[8 * k], sine[8 * k]);
						A1[16 * k + j + 8].im = ProductIm(A2[8 * k + j].re - A2[8 * k + j + 512].re, A2[8 * k + j].im - A2[8 * k + j + 512].im, cosine[8 * k], sine[8 * k]);
					}
				}
				break;
			case 5:
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
	}
	else {
		COM_Send_Deny(INVALID_WEIGHT);
	}
	step_flag = 0;
}

//静置标定
static void hang_calibration(void) {
	if (amp_1.value < HANG_AMP_THRESHOLD && amp_offset > MIN_HANG) {
		hanging = amp_offset;
		EEP_HangCalibration_Write();
		COM_Send_Positive();
	}
	else {
		COM_Send_Deny(INVALID_HANG);
	}
	step_flag = 0;
}

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
	}
	else {
		charging_flag = 0;
	}
}

//SOC修正
void FUNC_BattSOC_Caculation(void) {
	//电压补偿
	battvolt = (1 - BATTVOLT_FACTOR) * ((float) ADC_ConvertedValue[0] / 2048 * 3.21f + 0.13f) + BATTVOLT_FACTOR * battvolt;
	if (charging_flag == 1) {
		if (caculate_soc() > battsoc) {
			battsoc += 0.02f;
		}
	}
	else {
		if (caculate_soc() < battsoc) {
			battsoc -= 0.02f;
		}
	}
}

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
	}
	else {
		standingcounter = 0;
	}
	return 1;
}

//采样值5次均值滤波 + 1次时滞滤波
void FUNC_Pressure_Filter(void) {
	float temppressure;
	for (uint8_t filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		temppressure = ((float)filtercounter * temppressure + (float) ADC_ConvertedValue[1] / 4096 * 3.3f) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure = (1 - PRESSURE_FACTOR) * temppressure + PRESSURE_FACTOR * pressure;
}

void FUNC_Step_CountOrCalibrate(void) {
	if (step_flag == 0) {
		return;
	}
	if (fillcounter < 512) {
		get_pressureset();
	}
	else {
		fft_stepfrequency();
		switch (step_flag) {
			case 1:
				step_counter();
				break;
			case 2:
				step_calibration();
				break;
			case 3:
				hang_calibration();
				break;
		}
	}
}
