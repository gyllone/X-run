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
extern float pressure_a;
extern float pressure_b;
extern float hanging_a;
extern float hanging_b;

//休眠计数
static uint16_t standingcounter = 0;
static complex A1[512];
static complex A2[512];
static complex B1[512];
static complex B2[512];
static float ValueA[99];
static float ValueB[99];
float a_amp_offset = 0;
float b_amp_offset = 0;
amplitude a_amp_1 = {0, 0};
amplitude a_amp_2 = {0, 0};
amplitude b_amp_1 = {0, 0};
amplitude b_amp_2 = {0, 0};

static void fft_stepfrequency(void) {
	a_amp_1.value = 0;
	a_amp_2.value = 0;
	b_amp_1.value = 0;
	b_amp_2.value = 0;
	for (uint8_t q = 1; q < 10; q++) {
		switch (q) {
			case 1:
				for (uint16_t k = 0; k < 256; k++) {
					A2[2 * k].re = A1[k].re + A1[k + 256].re;
					A2[2 * k].im = 0;
					A2[2 * k + 1].re = (A1[k].re - A1[k + 256].re) * cosine[k];
					A2[2 * k + 1].im = (A1[k].re - A1[k + 256].re) * sine[k];
					B2[2 * k].re = B1[k].re + B1[k + 256].re;
					B2[2 * k].im = 0;
					B2[2 * k + 1].re = (B1[k].re - B1[k + 256].re) * cosine[k];
					B2[2 * k + 1].im = (B1[k].re - B1[k + 256].re) * sine[k];
				}
				break;
			case 2:
				for (uint16_t k = 0; k < 128; k++) {
					for (uint16_t j = 0; j < 2; j++) {
						A1[4 * k + j].re = A2[2 * k + j].re + A2[2 * k + j + 256].re;
						A1[4 * k + j].im = A2[2 * k + j].im + A2[2 * k + j + 256].im;				
						A1[4 * k + j + 2].re = ProductRe(A2[2 * k + j].re - A2[2 * k + j + 256].re, A2[2 * k + j].im - A2[2 * k + j + 256].im, cosine[2 * k], sine[2 * k]);
						A1[4 * k + j + 2].im = ProductIm(A2[2 * k + j].re - A2[2 * k + j + 256].re, A2[2 * k + j].im - A2[2 * k + j + 256].im, cosine[2 * k], sine[2 * k]);
						B1[4 * k + j].re = B2[2 * k + j].re + B2[2 * k + j + 256].re;
						B1[4 * k + j].im = B2[2 * k + j].im + B2[2 * k + j + 256].im;				
						B1[4 * k + j + 2].re = ProductRe(B2[2 * k + j].re - B2[2 * k + j + 256].re, B2[2 * k + j].im - B2[2 * k + j + 256].im, cosine[2 * k], sine[2 * k]);
						B1[4 * k + j + 2].im = ProductIm(B2[2 * k + j].re - B2[2 * k + j + 256].re, B2[2 * k + j].im - B2[2 * k + j + 256].im, cosine[2 * k], sine[2 * k]);
					}
				}
				break;
			case 3:
				for (uint16_t k = 0; k < 64; k++) {
					for (uint16_t j = 0; j < 4; j++) {
						A2[8 * k + j].re = A1[4 * k + j].re + A1[4 * k + j + 256].re;
						A2[8 * k + j].im = A1[4 * k + j].im + A1[4 * k + j + 256].im;
						A2[8 * k + j + 4].re = ProductRe(A1[4 * k + j].re - A1[4 * k + j + 256].re, A1[4 * k + j].im - A1[4 * k + j + 256].im, cosine[4 * k], sine[4 * k]);
						A2[8 * k + j + 4].im = ProductIm(A1[4 * k + j].re - A1[4 * k + j + 256].re, A1[4 * k + j].im - A1[4 * k + j + 256].im, cosine[4 * k], sine[4 * k]);
						B2[8 * k + j].re = B1[4 * k + j].re + B1[4 * k + j + 256].re;
						B2[8 * k + j].im = B1[4 * k + j].im + B1[4 * k + j + 256].im;
						B2[8 * k + j + 4].re = ProductRe(B1[4 * k + j].re - B1[4 * k + j + 256].re, B1[4 * k + j].im - B1[4 * k + j + 256].im, cosine[4 * k], sine[4 * k]);
						B2[8 * k + j + 4].im = ProductIm(B1[4 * k + j].re - B1[4 * k + j + 256].re, B1[4 * k + j].im - B1[4 * k + j + 256].im, cosine[4 * k], sine[4 * k]);
					}
				}
				break;
			case 4:
				for (uint16_t k = 0; k < 32; k++) {
					for (uint16_t j = 0; j < 8; j++) {
						A1[16 * k + j].re = A2[8 * k + j].re + A2[8 * k + j + 256].re;
						A1[16 * k + j].im = A2[8 * k + j].im + A2[8 * k + j + 256].im;				
						A1[16 * k + j + 8].re = ProductRe(A2[8 * k + j].re - A2[8 * k + j + 256].re, A2[8 * k + j].im - A2[8 * k + j + 256].im, cosine[8 * k], sine[8 * k]);
						A1[16 * k + j + 8].im = ProductIm(A2[8 * k + j].re - A2[8 * k + j + 256].re, A2[8 * k + j].im - A2[8 * k + j + 256].im, cosine[8 * k], sine[8 * k]);
						B1[16 * k + j].re = B2[8 * k + j].re + B2[8 * k + j + 256].re;
						B1[16 * k + j].im = B2[8 * k + j].im + B2[8 * k + j + 256].im;				
						B1[16 * k + j + 8].re = ProductRe(B2[8 * k + j].re - B2[8 * k + j + 256].re, B2[8 * k + j].im - B2[8 * k + j + 256].im, cosine[8 * k], sine[8 * k]);
						B1[16 * k + j + 8].im = ProductIm(B2[8 * k + j].re - B2[8 * k + j + 256].re, B2[8 * k + j].im - B2[8 * k + j + 256].im, cosine[8 * k], sine[8 * k]);
					}
				}
				break;
			case 5:
				for (uint16_t k = 0; k < 16; k++) {
					for (uint16_t j = 0; j < 16; j++) {
						A2[32 * k + j].re = A1[16 * k + j].re + A1[16 * k + j + 256].re;
						A2[32 * k + j].im = A1[16 * k + j].im + A1[16 * k + j + 256].im;
						A2[32 * k + j + 16].re = ProductRe(A1[16 * k + j].re - A1[16 * k + j + 256].re, A1[16 * k + j].im - A1[16 * k + j + 256].im, cosine[16 * k], sine[16 * k]);
						A2[32 * k + j + 16].im = ProductIm(A1[16 * k + j].re - A1[16 * k + j + 256].re, A1[16 * k + j].im - A1[16 * k + j + 256].im, cosine[16 * k], sine[16 * k]);
						B2[32 * k + j].re = B1[16 * k + j].re + B1[16 * k + j + 256].re;
						B2[32 * k + j].im = B1[16 * k + j].im + B1[16 * k + j + 256].im;
						B2[32 * k + j + 16].re = ProductRe(B1[16 * k + j].re - B1[16 * k + j + 256].re, B1[16 * k + j].im - B1[16 * k + j + 256].im, cosine[16 * k], sine[16 * k]);
						B2[32 * k + j + 16].im = ProductIm(B1[16 * k + j].re - B1[16 * k + j + 256].re, B1[16 * k + j].im - B1[16 * k + j + 256].im, cosine[16 * k], sine[16 * k]);
					}
				}
				break;
			case 6:
				for (uint16_t k = 0; k < 8; k++) {
					for (uint16_t j = 0; j < 32; j++) {
						A1[64 * k + j].re = A2[32 * k + j].re + A2[32 * k + j + 256].re;
						A1[64 * k + j].im = A2[32 * k + j].im + A2[32 * k + j + 256].im;				
						A1[64 * k + j + 32].re = ProductRe(A2[32 * k + j].re - A2[32 * k + j + 256].re, A2[32 * k + j].im - A2[32 * k + j + 256].im, cosine[32 * k], sine[32 * k]);
						A1[64 * k + j + 32].im = ProductIm(A2[32 * k + j].re - A2[32 * k + j + 256].re, A2[32 * k + j].im - A2[32 * k + j + 256].im, cosine[32 * k], sine[32 * k]);
						B1[64 * k + j].re = B2[32 * k + j].re + B2[32 * k + j + 256].re;
						B1[64 * k + j].im = B2[32 * k + j].im + B2[32 * k + j + 256].im;				
						B1[64 * k + j + 32].re = ProductRe(B2[32 * k + j].re - B2[32 * k + j + 256].re, B2[32 * k + j].im - B2[32 * k + j + 256].im, cosine[32 * k], sine[32 * k]);
						B1[64 * k + j + 32].im = ProductIm(B2[32 * k + j].re - B2[32 * k + j + 256].re, B2[32 * k + j].im - B2[32 * k + j + 256].im, cosine[32 * k], sine[32 * k]);
					}
				}
				break;
			case 7:
				for (uint16_t k = 0; k < 4; k++) {
					for (uint16_t j = 0; j < 64; j++) {
						A2[128 * k + j].re = A1[64 * k + j].re + A1[64 * k + j + 256].re;
						A2[128 * k + j].im = A1[64 * k + j].im + A1[64 * k + j + 256].im;
						A2[128 * k + j + 64].re = ProductRe(A1[64 * k + j].re - A1[64 * k + j + 256].re, A1[64 * k + j].im - A1[64 * k + j + 256].im, cosine[64 * k], sine[64 * k]);
						A2[128 * k + j + 64].im = ProductIm(A1[64 * k + j].re - A1[64 * k + j + 256].re, A1[64 * k + j].im - A1[64 * k + j + 256].im, cosine[64 * k], sine[64 * k]);
						B2[128 * k + j].re = B1[64 * k + j].re + B1[64 * k + j + 256].re;
						B2[128 * k + j].im = B1[64 * k + j].im + B1[64 * k + j + 256].im;
						B2[128 * k + j + 64].re = ProductRe(B1[64 * k + j].re - B1[64 * k + j + 256].re, B1[64 * k + j].im - B1[64 * k + j + 256].im, cosine[64 * k], sine[64 * k]);
						B2[128 * k + j + 64].im = ProductIm(B1[64 * k + j].re - B1[64 * k + j + 256].re, B1[64 * k + j].im - B1[64 * k + j + 256].im, cosine[64 * k], sine[64 * k]);
					}
				}
				break;
			case 8:
				for (uint16_t k = 0; k < 2; k++) {
					for (uint16_t j = 0; j < 128; j++) {
						A1[256 * k + j].re = A2[128 * k + j].re + A2[128 * k + j + 256].re;
						A1[256 * k + j].im = A2[128 * k + j].im + A2[128 * k + j + 256].im;
						A1[256 * k + j + 128].re = ProductRe(A2[128 * k + j].re - A2[128 * k + j + 256].re, A2[128 * k + j].im - A2[128 * k + j + 256].im, cosine[128 * k], sine[128 * k]);
						A1[256 * k + j + 128].im = ProductIm(A2[128 * k + j].re - A2[128 * k + j + 256].re, A2[128 * k + j].im - A2[128 * k + j + 256].im, cosine[128 * k], sine[128 * k]);
						B1[256 * k + j].re = B2[128 * k + j].re + B2[128 * k + j + 256].re;
						B1[256 * k + j].im = B2[128 * k + j].im + B2[128 * k + j + 256].im;
						B1[256 * k + j + 128].re = ProductRe(B2[128 * k + j].re - B2[128 * k + j + 256].re, B2[128 * k + j].im - B2[128 * k + j + 256].im, cosine[128 * k], sine[128 * k]);
						B1[256 * k + j + 128].im = ProductIm(B2[128 * k + j].re - B2[128 * k + j + 256].re, B2[128 * k + j].im - B2[128 * k + j + 256].im, cosine[128 * k], sine[128 * k]);
					}
				}
				break;
			case 9:
				A2[0].re = A1[0].re + A1[256].re;
				B2[0].re = B1[0].re + B1[256].re;
				for (uint16_t j = 1; j < 100; j++) {
					A2[j].re = A1[j].re + A1[j + 256].re;
					A2[j].im = A1[j].im + A1[j + 256].im;
					B2[j].re = B1[j].re + B1[j + 256].re;
					B2[j].im = B1[j].im + B1[j + 256].im;
				}
				break;
		}
	}
	a_amp_offset = A2[0].re / 512;
	b_amp_offset = B2[0].re / 512;
	for (uint16_t k = 1; k < 100; k++) {
		ValueA[k - 1] = sqrt((A2[k].re / 256) * (A2[k].re / 256) + (A2[k].im / 256) * (A2[k].im / 256));
		if (ValueA[k - 1] > a_amp_1.value) {
			a_amp_1.value = ValueA[k - 1];
			a_amp_1.index = k;
		}
		ValueB[k - 1] = sqrt((B2[k].re / 256) * (B2[k].re / 256) + (B2[k].im / 256) * (B2[k].im / 256));
		if (ValueB[k - 1] > b_amp_1.value) {
			b_amp_1.value = ValueB[k - 1];
			b_amp_1.index = k;
		}
	}
	for (uint16_t k = 1; k < 100; k++) {
		if (ValueA[k - 1] > a_amp_2.value && ValueA[k - 1] < a_amp_1.value) {
			a_amp_2.value = ValueA[k - 1];
			a_amp_2.index = k;
		}
		if (ValueB[k - 1] > b_amp_2.value && ValueB[k - 1] < b_amp_1.value) {
			b_amp_2.value = ValueB[k - 1];
			b_amp_2.index = k;
		}
	}
	fillcounter = 0;
}

static void step_counter(void) {
	if (a_amp_1.value >= running.threshold_a && b_amp_1.value >= running.threshold_b) {
		float tempstep = ((a_amp_1.value * a_amp_1.index + a_amp_2.value * a_amp_2.index) / (a_amp_1.value + a_amp_2.value) + 
											(b_amp_1.value * b_amp_1.index + b_amp_2.value * b_amp_2.index) / (b_amp_1.value + b_amp_2.value)) / 2;
		if (tempstep - (float)(uint32_t)tempstep >= 0.5) {
			running.total_steps += (uint32_t)tempstep + 1;
		}
		else {
			running.total_steps += (uint32_t)tempstep;
		}
	}
	else if (a_amp_1.value >= walking.threshold_a && a_amp_1.value < running.threshold_a && 
					 b_amp_1.value >= walking.threshold_b && b_amp_1.value < running.threshold_b) {
		float tempstep = ((a_amp_1.value * a_amp_1.index + a_amp_2.value * a_amp_2.index) / (a_amp_1.value + a_amp_2.value) + 
											(b_amp_1.value * b_amp_1.index + b_amp_2.value * b_amp_2.index) / (b_amp_1.value + b_amp_2.value)) / 2;
		if (tempstep - (float)(uint32_t)tempstep >= 0.5) {
			walking.total_steps += (uint32_t)tempstep + 1;
		}
		else {
			walking.total_steps += (uint32_t)tempstep;
		}
	}
}

static void step_calibration(void) {
	if (a_amp_1.value < WEIGHT_AMP_THRESHOLD_A && a_amp_offset < MAX_WEIGHT_A &&
			b_amp_1.value < WEIGHT_AMP_THRESHOLD_B && b_amp_offset < MAX_WEIGHT_B ) {
		walking.threshold_a = (a_amp_offset - hanging_a) * WALK_AMP_RATIO_A;
		walking.threshold_b = (b_amp_offset - hanging_b) * WALK_AMP_RATIO_B;
		running.threshold_a = (a_amp_offset - hanging_a) * RUN_AMP_RATIO_A;
		running.threshold_b = (b_amp_offset - hanging_b) * RUN_AMP_RATIO_B;
		EEP_StepCalibration_Write();
		COM_Send_Positive();
	}
	else {
		COM_Send_Deny(INVALID_WEIGHT);
	}
	step_flag = 0;
}

static void hang_calibration(void) {
	if (a_amp_1.value < HANG_AMP_THRESHOLD && a_amp_offset > MIN_HANG &&
			b_amp_1.value < HANG_AMP_THRESHOLD && a_amp_offset > MIN_HANG) {
		hanging_a = a_amp_offset;
		hanging_b = a_amp_offset;
		EEP_HangCalibration_Write();
		COM_Send_Positive();
	}
	else {
		COM_Send_Deny(INVALID_HANG);
	}
	step_flag = 0;
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
	A1[fillcounter].re = pressure_a;
	B1[fillcounter].re = pressure_b;
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
	for (uint8_t i = 0; i < 100; i++) {
		pressure_a = (1 - INITIAL_FACTOR) * (float) ADC_ConvertedValue[1] / 4096 * 3.3f + INITIAL_FACTOR * pressure_a;
		pressure_b = (1 - INITIAL_FACTOR) * (float) ADC_ConvertedValue[2] / 4096 * 3.3f + INITIAL_FACTOR * pressure_b;
		battvolt = (1 - INITIAL_FACTOR) * (float) ADC_ConvertedValue[0] / 2048 * 3.3f + INITIAL_FACTOR * battvolt;
		INITIALDELAY;
	}
	if (caculate_soc() - battsoc < -12 || caculate_soc() - battsoc > 12) {
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
	if (pressure_a > HANG_RATIO * hanging_a && pressure_b > HANG_RATIO * hanging_b && response_flag < 1 && charging_flag != 1) {
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
	float temppressure_1, temppressure_2 = 0;
	for (uint8_t filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		temppressure_1 = ((float)filtercounter * temppressure_1 + (float) ADC_ConvertedValue[1] / 4096 * 3.3) / (float)(filtercounter + 1);
		temppressure_2 = ((float)filtercounter * temppressure_2 + (float) ADC_ConvertedValue[2] / 4096 * 3.3) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure_a = (1 - PRESSURE_FACTOR) * temppressure_1 + PRESSURE_FACTOR * pressure_a;
	pressure_b = (1 - PRESSURE_FACTOR) * temppressure_2 + PRESSURE_FACTOR * pressure_b;
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
