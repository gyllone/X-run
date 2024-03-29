#ifndef __FUNCTION_
#define __FUNCTION_

#include "stm32f10x.h"
#include "bsp_led.h"

//复数乘法
#define ProductRe(a, b, c, d) ((a) * (c) - (b) * (d))
#define ProductIm(a, b, c, d) ((a) * (d) + (b) * (c))

#define INITIAL_FACTOR 						0.35f //初始滤波系数
#define BATTVOLT_FACTOR 					0.55f //电池滤波系数
#define PRESSURE_FACTOR 					0.75f //压力滤波系数

#define HANG_RATIO 								0.98f //悬空系数
#define WALK_AMP_RATIO_A					0.4f
#define WALK_AMP_RATIO_B					0.4f
#define RUN_AMP_RATIO_A						0.65f
#define RUN_AMP_RATIO_B						0.65f

#define HANG_AMP_THRESHOLD	 			0.001f
#define WEIGHT_AMP_THRESHOLD_A		0.05f
#define WEIGHT_AMP_THRESHOLD_B		0.05f

#define MIN_HANG 									3.2f
#define MAX_WEIGHT_A							2.9f
#define MAX_WEIGHT_B							2.7f

#define INITIALDELAY delay_ms(1); //隔1ms读一次电压
#define SAMPLEDELAY delay_ms(2); //隔2ms读一次电压

//表示步数，run和walk通用
typedef struct Stepping {
	float 	 threshold_a;
	float 	 threshold_b;
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
void FUNC_Led_Breath(void);
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

#endif
