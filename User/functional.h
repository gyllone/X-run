#ifndef __FUNCTION_
#define __FUNCTION_

#include "stm32f10x.h"
#include "bsp_adc.h"
#include "bsp_led.h"

#define PRESSURE_FACTOR 0.55 //滤波系数
#define BATTVOLT_FACTOR 0.05 //滤波系数

#define HANG_RATIO 0.98 //静置压力比
#define WALK_WEIGHT_RATIO 0.85 //走路压力比
#define RUN_WEIGHT_RATIO 0.65 //跑步压力比

#define MIN_HANG 2.53
#define MAX_HANG 2.57

#define MIN_WEIGHT 1.85
#define MAX_WEIGHT 2.25

#define SAMPLEDELAY delay_ms(1); //隔1ms读一次电压
#define LEDDELAY twinkle(100);

//表示步数，run和walk通用
typedef struct Stepping {
	float threshold;
	uint32_t current_steps;
	uint32_t total_steps;
} Step;

uint8_t FUNC_SleepOrNot(void);
void FUNC_BattSOC_Caculation(void);
void FUNC_Functional_Initial(void);
void FUNC_Step_Counter(void);

//延时函数
static void delay_ms(uint16_t nms)
{
	uint32_t temp;
	SysTick->LOAD = 9000 * nms;
	SysTick->VAL = 0X00;//清空计数器
	SysTick->CTRL = 0X01;//开始倒数
	do {
		temp = SysTick -> CTRL; //读取倒计时
	}
	while((temp & 0x01) && (!(temp & (1 << 16))));//等待时间到达
  SysTick->CTRL = 0x00; //关闭计数器
  SysTick->VAL = 0X00; //清空计数器
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
