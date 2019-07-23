// @gyl
#include "functional.h"

const extern uint32_t device_id;

extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];
extern volatile uint8_t step_flag;
extern volatile uint8_t charging_flag;
extern volatile uint8_t displaysoc;
extern volatile uint8_t interactstatus;
extern volatile float actualsoc;
extern volatile float battvolt;

extern uint32_t app_id;
extern float pressure;
extern float hanging;
extern Step walking;
extern Step running;

//当前步数阶段
static uint8_t cycleflag = 0;
//上一阶段
static uint8_t cyclelastflag = 0;
//阶段进入计数
static uint8_t cyclecounter = 0;
//超时计数
static uint8_t overtimecounter = 0;
//休眠计数
static uint16_t standingcounter = 0;

static void battary_filter(void) {
	battvolt = (1 - BATTVOLT_FACTOR) * (float) ADC_ConvertedValue[0] / 2048 * 3.3 + BATTVOLT_FACTOR * battvolt;
}

//SOC电压估算
static float caculate_soc(void) {
	float soc;
	if (battvolt > 4.11) {
		soc = 100;
	}
	else if (battvolt < 3.31) {
		soc = 0.02;
	}
	else {
		soc = (battvolt - 3.31) / 0.8 * 100;
	}
	return soc;
}

//采样值5次均值滤波 + 1次时滞滤波
static void pressure_filter(void) {
	uint8_t filtercounter;
	float pressurebuffer = 0;
	for (filtercounter = 0 ; filtercounter < 5; filtercounter++) {
		pressurebuffer = ((float)filtercounter * pressurebuffer + (float) ADC_ConvertedValue[1] / 4096 * 3.3) / (float)(filtercounter + 1);
		SAMPLEDELAY;
	}
	pressure = (1 - PRESSURE_FACTOR) * pressurebuffer + PRESSURE_FACTOR * pressure;
}

//超时reset
static void overtime_reset(void) {
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
	cyclelastflag = cycleflag;
}

//SOC修正
void FUNC_BattSOC_Caculation(void) {
	battary_filter();
	if (charging_flag > 0) {
		if (caculate_soc() > actualsoc) {
			actualsoc += 0.02;
		}
	}
	else {
		if (caculate_soc() < actualsoc) {
			actualsoc -= 0.02;
		}
	}
	displaysoc = (uint8_t)actualsoc;
}

//滤波初始化
void FUNC_Functional_Initial(void) {
	uint8_t i;
	for (i = 0; i < 50; i++) {
		pressure_filter();
		battary_filter();
	}
	actualsoc = caculate_soc();
	displaysoc = (uint8_t)actualsoc;
}

//判断是否进入待机
uint8_t FUNC_SleepOrNot(void) {
	//SOC低于1休眠
	if (actualsoc < 1) {
		return 0;
	}
	//非静置状态、通讯状态、充电状态不休眠
	if (pressure > hanging && interactstatus < 1 && charging_flag < 1) {
		standingcounter++;
		if (standingcounter > 5000) {
			return 0;
		}
	}
	else {
		standingcounter = 0;
	}
	return 1;
}
	
/*                    计步原理

            * * * 
----------*------ *--------------------------------
         *|       |* 
        * |       | * 
-------*--|-------|--*-----------------------------
      *|  |       |  |*
     * |  |       |  | *   ����3/4    �˳�3/4������5/6
----*--|--|-------|--|--*---|-----------|---*------
    0  �� ��      �� ��  *  |           |  *
       �� ��      �� ��   * |           | *
       1  2       2  1     *|           |*
----------------------------*-----------*----------
														 *         *
                              *       *
                                * * *
*/

//计步器
void FUNC_Step_Counter(void) {
	pressure_filter();
	//stepflag为1时允许计步
	if (step_flag) {
		overtime_reset();
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
