// @ gyl
#ifndef __EEP_ROM
#define	__EEP_ROM

#include "stm32f10x.h"
#include "bsp_i2c_ee.h"
#include "bsp_rtc.h"
#include "functional.h"

void EEP_Initial_Read(void);
void EEP_Binding_Write(void);
void EEP_Weight_Write(void);
void EEP_Hang_Write(void);
void EEP_Step_Write(void);
void EEP_Sleep_Write(void);

//首次flash标志位，0x1A表示之前已刷新
#define FIRST_FLASH_SIZE 1
#define FIRST_FLASH_ADDR (uint8_t)0x00

//设备绑定，第一个字节为绑定状态，后四个字节为绑定APP的ID
#define ACTIVE_STATUS_SIZE 5
#define ACTIVE_STATUS_ADDR (uint8_t)0x01

//走路步数
#define CYCLE_WALKSTEP_SIZE 4
#define CYCLE_WAKLSTEP_ADDR (uint8_t)0x06

//跑步步数
#define CYCLE_RUNSTEP_SIZE 4
#define CYCLE_RUNSTEP_ADDR (uint8_t)0x0A

//走路总步数
#define TOTAL_WALKSTEP_SIZE 4
#define TOTAL_WALKSTEP_ADDR (uint8_t)0x0E

//跑步总步数
#define TOTAL_RUNSTEP_SIZE 4
#define TOTAL_RUNSTEP_ADDR (uint8_t)0x12

//体重标定量
#define WEIGHT_PRESSURE_SIZE 4
#define WEIGHT_PRESSURE_ADDR (uint8_t)0x16

//静置压力标定量
#define HANG_PRESSURE_SIZE 4
#define HANG_RPRESSURE_ADDR (uint8_t)0x1A

#endif
