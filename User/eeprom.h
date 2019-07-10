// @ gyl
#ifndef __EEP_ROM
#define	__EEP_ROM

#include "stm32f10x.h"
#include "bsp_i2c_ee.h"
#include "bsp_rtc.h"
#include "functional.h"

void EEP_initial_read(void);
void EEP_weight_write(void);
void EEP_hang_write(void);
void EEP_walkstep_write(void);
void EEP_runstep_write(void);
void EEP_sleep_write(void);

//首次flash标志位，0x1F表示之前已刷新
#define FIRST_FLASH_SIZE 1
#define FIRST_FLASH_ADDR 0x0000

//设备绑定，第一个字节为绑定状态，后四个字节为绑定APP的ID
#define ACTIVE_STATUS_SIZE 13
#define ACTIVE_STATUS_ADDR 0x0001

//走路步数
#define CYCLE_WALKSTEP_SIZE 4
#define CYCLE_WAKLSTEP_ADDR 0x000E

//跑步步数
#define CYCLE_RUNSTEP_SIZE 4
#define CYCLE_RUNSTEP_ADDR 0x0013

//走路总步数
#define TOTAL_WALKSTEP_SIZE 4
#define TOTAL_WALKSTEP_ADDR 0x0018

//跑步总步数
#define TOTAL_RUNSTEP_SIZE 4
#define TOTAL_RUNSTEP_ADDR 0x001C

//体重标定量
#define WEIGHT_PRESSURE_SIZE 4
#define WEIGHT_PRESSURE_ADDR 0x0020

//静置压力标定量
#define HANG_PRESSURE_SIZE 4
#define HANG_RPRESSURE_ADDR 0x0024

#endif