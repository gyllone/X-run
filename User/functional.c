// @gyl
#include "functional.h"
#include "bsp_adc.h"
#include "math.h"
#include "eeprom.h"
#include "trigonometric.h"
#include "communication.h"

const extern uint32_t device_id;

extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];
extern volatile uint8_t step_flag;
extern volatile uint8_t charging_flag;
extern volatile uint8_t interactstatus;
extern volatile float battsoc;
extern volatile float battvolt;

extern uint16_t fillcounter;
extern uint32_t app_id;
extern Step walking;
extern Step running;
extern float pressure;
extern float hanging;

//休眠计数
static uint16_t standingcounter = 0;
static complex A1[1024];
static complex A2[1024];
float a_amp_offset = 0;
amplitude a_amp_1 = {0, 0};
amplitude a_amp_2 = {0, 0};

static void fft_stepfrequency(void) {
	uint8_t q;
	uint16_t k, j;
	float temp;
	a_amp_1.value = 0;
	a_amp_2.value = 0;
	for (q = 1; q < 11; q++) {
		switch (q) {
			case 1:
				for (k = 0; k < 512; k++) {
					A2[2 * k].re = A1[k].re + A1[k + 512].re;
					A2[2 * k].im = 0;
					A2[2 * k + 1].re = (A1[k].re - A1[k + 512].re) * cosine[k];
					A2[2 * k + 1].im = (A1[k].re - A1[k + 512].re) * sine[k];
				}
				break;
			case 2:
				for (k = 0; k < 256; k++) {
					for (j = 0; j < 2; j++) {
						A1[4 * k + j].re = A2[2 * k + j].re + A2[2 * k + j + 512].re;
						A1[4 * k + j].im = A2[2 * k + j].im + A2[2 * k + j + 512].im;				
						A1[4 * k + j + 2].re = ProductRe(A2[2 * k + j].re - A2[2 * k + j + 512].re, A2[2 * k + j].im - A2[2 * k + j + 512].im, cosine[2 * k], sine[2 * k]);
						A1[4 * k + j + 2].im = ProductIm(A2[2 * k + j].re - A2[2 * k + j + 512].re, A2[2 * k + j].im - A2[2 * k + j + 512].im, cosine[2 * k], sine[2 * k]);
					}
				}
				break;
			case 3:
				for (k = 0; k < 128; k++) {
					for (j = 0; j < 4; j++) {
						A2[8 * k + j].re = A1[4 * k + j].re + A1[4 * k + j + 512].re;
						A2[8 * k + j].im = A1[4 * k + j].im + A1[4 * k + j + 512].im;
						A2[8 * k + j + 4].re = ProductRe(A1[4 * k + j].re - A1[4 * k + j + 512].re, A1[4 * k + j].im - A1[4 * k + j + 512].im, cosine[4 * k], sine[4 * k]);
						A2[8 * k + j + 4].im = ProductIm(A1[4 * k + j].re - A1[4 * k + j + 512].re, A1[4 * k + j].im - A1[4 * k + j + 512].im, cosine[4 * k], sine[4 * k]);
					}
				}
				break;
			case 4:
				for (k = 0; k < 64; k++) {
					for (j = 0; j < 8; j++) {
						A1[16 * k + j].re = A2[8 * k + j].re + A2[8 * k + j + 512].re;
						A1[16 * k + j].im = A2[8 * k + j].im + A2[8 * k + j + 512].im;				
						A1[16 * k + j + 8].re = ProductRe(A2[8 * k + j].re - A2[8 * k + j + 512].re, A2[8 * k + j].im - A2[8 * k + j + 512].im, cosine[8 * k], sine[8 * k]);
						A1[16 * k + j + 8].im = ProductIm(A2[8 * k + j].re - A2[8 * k + j + 512].re, A2[8 * k + j].im - A2[8 * k + j + 512].im, cosine[8 * k], sine[8 * k]);
					}
				}
				break;
			case 5:
				for (k = 0; k < 32; k++) {
					for (j = 0; j < 16; j++) {
						A2[32 * k + j].re = A1[16 * k + j].re + A1[16 * k + j + 512].re;
						A2[32 * k + j].im = A1[16 * k + j].im + A1[16 * k + j + 512].im;
						A2[32 * k + j + 16].re = ProductRe(A1[16 * k + j].re - A1[16 * k + j + 512].re, A1[16 * k + j].im - A1[16 * k + j + 512].im, cosine[16 * k], sine[16 * k]);
						A2[32 * k + j + 16].im = ProductIm(A1[16 * k + j].re - A1[16 * k + j + 512].re, A1[16 * k + j].im - A1[16 * k + j + 512].im, cosine[16 * k], sine[16 * k]);
					}
				}
				break;
			case 6:
				for (k = 0; k < 16; k++) {
					for (j = 0; j < 32; j++) {
						A1[64 * k + j].re = A2[32 * k + j].re + A2[32 * k + j + 512].re;
						A1[64 * k + j].im = A2[32 * k + j].im + A2[32 * k + j + 512].im;				
						A1[64 * k + j + 32].re = ProductRe(A2[32 * k + j].re - A2[32 * k + j + 512].re, A2[32 * k + j].im - A2[32 * k + j + 512].im, cosine[32 * k], sine[32 * k]);
						A1[64 * k + j + 32].im = ProductIm(A2[32 * k + j].re - A2[32 * k + j + 512].re, A2[32 * k + j].im - A2[32 * k + j + 512].im, cosine[32 * k], sine[32 * k]);
					}
				}
				break;
			case 7:
				for (k = 0; k < 8; k++) {
					for (j = 0; j < 64; j++) {
						A2[128 * k + j].re = A1[64 * k + j].re + A1[64 * k + j + 512].re;
						A2[128 * k + j].im = A1[64 * k + j].im + A1[64 * k + j + 512].im;
						A2[128 * k + j + 64].re = ProductRe(A1[64 * k + j].re - A1[64 * k + j + 512].re, A1[64 * k + j].im - A1[64 * k + j + 512].im, cosine[64 * k], sine[64 * k]);
						A2[128 * k + j + 64].im = ProductIm(A1[64 * k + j].re - A1[64 * k + j + 512].re, A1[64 * k + j].im - A1[64 * k + j + 512].im, cosine[64 * k], sine[64 * k]);
					}
				}
				break;
			case 8:
				for (k = 0; k < 4; k++) {
					for (j = 0; j < 128; j++) {
						A1[256 * k + j].re = A2[128 * k + j].re + A2[128 * k + j + 512].re;
						A1[256 * k + j].im = A2[128 * k + j].im + A2[128 * k + j + 512].im;
						A1[256 * k + j + 128].re = ProductRe(A2[128 * k + j].re - A2[128 * k + j + 512].re, A2[128 * k + j].im - A2[128 * k + j + 512].im, cosine[128 * k], sine[128 * k]);
						A1[256 * k + j + 128].im = ProductIm(A2[128 * k + j].re - A2[128 * k + j + 512].re, A2[128 * k + j].im - A2[128 * k + j + 512].im, cosine[128 * k], sine[128 * k]);
					}
				}
				break;
			case 9:
				for (k = 0; k < 2; k++) {
					for (j = 0; j < 256; j++) {
						A2[512 * k + j].re = A1[256 * k + j].re + A1[256 * k + j + 512].re;
						A2[512 * k + j].im = A1[256 * k + j].im + A1[256 * k + j + 512].im;
						A2[512 * k + j + 256].re = ProductRe(A1[256 * k + j].re - A1[256 * k + j + 512].re, A1[256 * k + j].im - A1[256 * k + j + 512].im, cosine[256 * k], sine[256 * k]);
						A2[512 * k + j + 256].im = ProductIm(A1[256 * k + j].re - A1[256 * k + j + 512].re, A1[256 * k + j].im - A1[256 * k + j + 512].im, cosine[256 * k], sine[256 * k]);
					}
				}
				break;				
			case 10:
				A1[0].re = A2[0].re + A2[512].re;
				for (j = 1; j < 100; j++) {
					A1[j].re = A2[j].re + A2[j + 512].re;
					A1[j].im = A2[j].im + A2[j + 512].im;
				}
				break;
		}
	}
	a_amp_offset = A1[0].re / 1024;
	
	for (k = 1; k < 100; k++) {
		temp = sqrt((A1[k].re / 512) * (A1[k].re / 512) + (A1[k].im / 512) * (A1[k].im / 512));
		if (temp > a_amp_1.value) {
			a_amp_1.value = temp;
			a_amp_1.index = k;
		}
	}
	
	for (k = 1; k < 100; k++) {
		temp = sqrt((A1[k].re / 512) * (A1[k].re / 512) + (A1[k].im / 512) * (A1[k].im / 512));
		if (temp > a_amp_2.value && temp < a_amp_1.value) {
			a_amp_2.value = temp;
			a_amp_2.index = k;
		}
	}
	fillcounter = 0;
}

static void step_counter(void) {
	uint8_t tempstep;
	if (a_amp_1.index - a_amp_2.index > 2 || a_amp_1.index - a_amp_2.index < -2) {
		return;
	}
	if (a_amp_1.value + a_amp_2.value < walking.threshold) {
		return;
	}
	
	if (a_amp_1.index > a_amp_2.index) {
		tempstep = a_amp_2.index;
	}
	else {
		tempstep = a_amp_1.index;
	}
	
	if (a_amp_1.value + a_amp_2.value < running.threshold) {
		walking.current_steps += tempstep;
		walking.total_steps += tempstep;
	}
	else {
		running.current_steps += tempstep;
		running.total_steps += tempstep;
	}
}

static void step_calibration(void) {
	if (a_amp_1.value < WEIGHT_AMP_THRESHOLD && a_amp_offset < MAX_WEIGHT) {
		walking.threshold = (a_amp_offset - hanging) * WALK_AMP_RATIO;
		running.threshold = (a_amp_offset - hanging) * RUN_AMP_RATIO;
		EEP_StepCalibration_Write();
		COM_Send_Positive();
	}
	else {
		COM_Send_Deny(INVALID_WEIGHT);
	}
	step_flag = 1;
	interactstatus = 0;
}

static void hang_calibration(void) {
	if (a_amp_1.value < HANG_AMP_THRESHOLD && a_amp_offset > MIN_HANG) {
		hanging = a_amp_offset;
		EEP_HangCalibration_Write();
		COM_Send_Positive();
	}
	else {
		COM_Send_Deny(INVALID_HANG);
	}
	step_flag = 1;
	interactstatus = 0;
}

//SOC电压估算
static float caculate_soc(void) {
	float soc;
	if (battvolt > 4.18f) {
		soc = 100;
	}
	else if (battvolt < 3.35f) {
		soc = 0.021f;
	}
	else {
		soc = (battvolt - 3.35f) / 0.83f * 100;
	}
	return soc;
}

static void get_pressureset(void) {
	A1[fillcounter].re = pressure;
	fillcounter++;
}

//充电与否判断
void FUNC_ChargeOrNot(void) {
	if ((float)ADC_ConvertedValue[3] / 2048 * 3.3f > 4.35f) {
		charging_flag = 1;
	}
	else {
		charging_flag = 0;
	}
}

//SOC修正
void FUNC_BattSOC_Caculation(void) {
	battvolt = (1 - BATTVOLT_FACTOR) * (float) ADC_ConvertedValue[0] / 2048 * 3.3f + BATTVOLT_FACTOR * battvolt;	
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
	uint8_t i;
	for (i = 0; i < 100; i++) {
		pressure = (1 - INITIAL_FACTOR) * (float) ADC_ConvertedValue[1] / 4096 * 3.3f + INITIAL_FACTOR * pressure;
		battvolt = (1 - INITIAL_FACTOR) * (float) ADC_ConvertedValue[0] / 2048 * 3.3f + INITIAL_FACTOR * battvolt;
		INITIALDELAY;
	}
	battsoc = caculate_soc();
}

//判断是否进入待机
uint8_t FUNC_SleepOrNot(void) {
	//SOC低于1休眠
	if (battsoc < 1) {
		return 0;
	}
	//非静置状态、通讯状态、充电状态不休眠
	if (pressure > HANG_RATIO * hanging && interactstatus < 1 && charging_flag != 1) {
		standingcounter++;
		//1min后进入待机
		if (standingcounter > 3000) {
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
	uint8_t filtercounter;
	float temppressure;
	for (filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		temppressure = ((float)filtercounter * temppressure + (float) ADC_ConvertedValue[1] / 4096 * 3.3) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure = (1 - PRESSURE_FACTOR) * temppressure + PRESSURE_FACTOR * pressure;
}

void FUNC_Step_CountOrCalibrate(void) {
	if (step_flag == 0) {
		return;
	}
	if (fillcounter < 1024) {
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
