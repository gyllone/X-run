#ifndef __FUNCTION_
#define __FUNCTION_

#include "stm32f10x.h"
#include "bsp_led.h"

//复数乘法
#define ProductRe(a, b, c, d) ((a) * (c) - (b) * (d))
#define ProductIm(a, b, c, d) ((a) * (d) + (b) * (c))

#define INITIAL_FACTOR 						0.55f //初始滤波系数
#define BATTVOLT_FACTOR 					0.75f //电池滤波系数
#define PRESSURE_FACTOR 					0.65f //压力滤波系数

#define HANG_RATIO 								0.98f //悬空系数
#define WALK_AMP_RATIO 						0.9f
#define RUN_AMP_RATIO 						1.2f

#define HANG_AMP_THRESHOLD 				0.001f
#define WEIGHT_AMP_THRESHOLD 			0.05f

#define MIN_HANG 									2.45f
#define MAX_WEIGHT 								2.2f

#define INITIALDELAY delay_ms(1); //隔1ms读一次电压
#define SAMPLEDELAY delay_ms(2); //隔2ms读一次电压
#define LEDDELAY twinkle(100);

//表示步数，run和walk通用
typedef struct Stepping {
	float 	 threshold;
	uint32_t current_steps;
	uint32_t total_steps;
} Step;

typedef struct Complex {
	float re;
	float im;
} complex;

typedef struct Amplitude {
	int16_t  index;
	float    value;
} amplitude;

uint8_t FUNC_SleepOrNot(void);
void FUNC_ChargeOrNot(void);
void FUNC_BattSOC_Caculation(void);
void FUNC_Functional_Initial(void);
void FUNC_Pressure_Filter(void);
void FUNC_Step_CountOrCalibrate(void);

//延时函数
static void delay_ms(uint16_t nms)
{
	uint32_t temp;
	SysTick->LOAD = 9000 * nms;
	SysTick->VAL = 0x00;//清空计数器
	SysTick->CTRL = 0x01;//开始倒数
	do {
		temp = SysTick -> CTRL; //读取倒计时
	}
	while((temp & 0x01) && (!(temp & (1 << 16))));//等待时间到达
  SysTick->CTRL = 0x00; //关闭计数器
  SysTick->VAL = 0x00; //清空计数器
}

static void twinkle(uint16_t nms) {
	LED1_ON;
	LED2_ON;
	delay_ms(nms);
	LED1_OFF;
	LED2_OFF;
	delay_ms(nms);
	LED1_ON;
	LED2_ON;
	delay_ms(nms);
	LED1_OFF;
	LED2_OFF;
}

#endif
