// @gyl
#include "functional.h"

extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];
extern float battvolt;
extern float pressure;
extern uint8_t battsoc;
extern volatile Step walking;
extern volatile Step running;
extern volatile float hanging;
extern volatile uint8_t stepflag;

//当前cycle进行的阶段标志
static uint8_t cycleflag = 0;
//上个cycle进行的阶段标志
static uint8_t cyclelastflag = 0;
//当前cycle的某个阶段持续周期数
static uint8_t cyclecounter = 0;
//当前cycle的超时周期数
static uint8_t overtimecounter = 0;

//压力滤波，先以2ms间隔取5个电压做均值，然后做一次滞后滤波
static void FUNC_pressure_filter(void) {
	uint8_t filtercounter;
	float pressurebuffer;
	for (filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		pressurebuffer = ((float)filtercounter * pressurebuffer + (float) ADC_ConvertedValue[2] / 4096 * 3.3) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure = (1 - PRESSURE_FACTOR) * pressurebuffer + PRESSURE_FACTOR * pressure;
}

//电池SOC计算
void FUNC_battSOC_caculation(void) {
	battvolt = (1 - BATTVOLT_FACTOR) * ((float) ADC_ConvertedValue[1] / 2048 * 3.3) + BATTVOLT_FACTOR * battvolt;
	if (battvolt > 4.2) {
		battsoc = 100;
	}
	else if (battvolt < 3.35) {
		battsoc = 0;
	}
	else {
		battsoc = (uint8_t)((battvolt - 3.35) / 0.85 * 100);
	}
}

//功能性初始化
void FUNC_functional_initial(void) {
	uint8_t i;
	for (i = 0; i < 30; i++) {
		FUNC_pressure_filter();
		FUNC_battSOC_caculation();
	}
}

//cycle超时判断，超过100个周期reset
static void FUNC_overtime_reset(void) {
	if (overtimecounter > 100) {
		LED1_OFF;
		LED2_OFF;
		cyclelastflag = 0;
		cycleflag = 0;
		overtimecounter = 0;
		cyclecounter = 0;
	}
	if (cyclelastflag == 0 && cycleflag > 0) {
		overtimecounter = 1;
	}
	else if (cyclelastflag > 0 && cycleflag > 0) {
		overtimecounter++;
	}
	else if (cyclelastflag > 4 && cycleflag == 0) {
		overtimecounter = 0;
	}
	cyclelastflag = cycleflag;
}
	
/*                    计步图解

            * * * 
----------*------ *--------------------------------跑步压力
         *|       |* 
        * |       | * 
-------*--|-------|--*-----------------------------走路压力
      *|  |       |  |*
     * |  |       |  | *   进入3/4    退出3/4，进入5/6，完成一个Cycle，回归0
----*--|--|-------|--|--*---|-----------|---*------重力线
    0  进 进      退 退  *  |           |  *
       入 入      出 出   * |           | *
       1  2       2  1     *|           |*
----------------------------*-----------*----------悬空压力线
														 *         *
                              *       *
                                * * *
*/
//stepflag = 0时开启计步功能
void FUNC_step_counter(void) {
	FUNC_pressure_filter();
	if (stepflag < 1) {
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
					cyclecounter = 0;
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
