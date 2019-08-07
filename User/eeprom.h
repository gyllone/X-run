// @ gyl
#ifndef __EEPROMG_
#define	__EEPROMG_

#include "stm32f10x.h"
#include "bsp_i2c_ee.h"

void EEP_Initial_Read(void);
void EEP_Binding_Write(void);
void EEP_StepCalibration_Write(void);
void EEP_HangCalibration_Write(void);
void EEP_Sleep_Write(void);

//首次flash标志位，0x1A表示之前已刷新
#define FIRST_FLASH_SIZE 							1
#define FIRST_FLASH_ADDR 		 					0x00

//设备绑定，第一个字节为绑定状态，后四个字节为绑定APP的permission
#define ACTIVE_STATUS_SIZE 						6
#define ACTIVE_STATUS_ADDR 	 					0x10

//走路总步数
#define TOTAL_WALKSTEP_SIZE 					5
#define TOTAL_WALKSTEP_ADDR 					0x20

//跑步总步数
#define TOTAL_RUNSTEP_SIZE 						5
#define TOTAL_RUNSTEP_ADDR            0x30

//走路标定量
#define WALK_CALIBRATION_SIZE 				5
#define WALK_CALIBRATION_ADDR         0x40

//跑步标定量
#define RUN_CALIBRATION_SIZE 					5
#define RUN_CALIBRATION_ADDR          0x50

//静置标定量
#define HANG_CALIBRATION_SIZE 				5
#define HANG_CALIBRATION_ADDR  				0x60

//SOC
#define SOC_INITIAL_SIZE							5
#define SOC_INITIAL_ADDR 							0x70

#endif
