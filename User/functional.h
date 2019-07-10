#ifndef __FUNCTION_
#define __FUNCTION_

#include "stm32f10x.h"
#include "bsp_adc.h"
#include "bsp_led.h"

#define PRESSURE_FACTOR 0.5 //滤波系数
#define BATTVOLT_FACTOR 0.3 //滤波系数

#define WALK_WEIGHT_RATIO 0.85 //走路压力比
#define RUN_WEIGHT_RATIO 0.75 //跑步压力比

#define SAMPLEDELAY delay_ms(2); //隔2ms读一次电压

//表示步数，run和walk通用
typedef struct Stepping {
	float threshold;
	uint32_t current_steps;
	uint32_t total_steps;
} Step;

void FUNC_battSOC_caculation(void);
void FUNC_functional_initial(void);
void FUNC_step_counter(void);

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

#endif