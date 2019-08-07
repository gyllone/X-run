#include "eeprom.h"
#include "bsp_rtc.h"
#include "functional.h"
#include "communication.h"

extern struct rtc_time systmtime;

extern uint8_t eeprom_status;
extern uint8_t binding_flag;
extern Step walking;
extern Step running;
extern float hanging_a;
extern float hanging_b;
extern volatile float battsoc;

void EEP_Initial_Read(void) {
	uint8_t first_flash_flag = 0;
	uint8_t tempbuffer[5];
	uint32_t tempvalue;
	//检查E方是否正常
	eeprom_status = ee_CheckOk();
	if (eeprom_status != 1) {
		return;
	}
	//检查是否为首次刷新
	eeprom_status = ee_ReadBytes(&first_flash_flag, FIRST_FLASH_ADDR, FIRST_FLASH_SIZE);
	if (first_flash_flag != 1) {
		RTC_Configuration();
		first_flash_flag = 1;
		eeprom_status = ee_WriteBytes(&first_flash_flag, FIRST_FLASH_ADDR, FIRST_FLASH_SIZE);
	}
	else {
		RTC_CheckAndConfig(&systmtime);
	}
	//读取上一周期SOC
	eeprom_status = ee_ReadBytes(tempbuffer, SOC_INITIAL_ADDR, SOC_INITIAL_SIZE);
	if (eeprom_status != 1) {
		return;
	}
	tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
							(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
	if (COM_Checksum(tempvalue) == tempbuffer[4]) {
		battsoc = (float)tempvalue / 1000000000;
	}
	//检查设备绑定状态
	eeprom_status = ee_ReadBytes(&binding_flag, ACTIVE_STATUS_ADDR, ACTIVE_STATUS_SIZE);
	if (eeprom_status != 1) {
		return;
	}
	if (binding_flag == 1) {
		//走路信息
		eeprom_status = ee_ReadBytes(tempbuffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			walking.total_steps = tempvalue;
		}
		//跑步信息
		eeprom_status = ee_ReadBytes(tempbuffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			running.total_steps = tempvalue;
		}
		//走路标定值
		eeprom_status = ee_ReadBytes(tempbuffer, WALK_CALIBRATION_ADDR_A, WALK_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			walking.threshold_a = (float)tempvalue / 1000000000;
		}
		eeprom_status = ee_ReadBytes(tempbuffer, WALK_CALIBRATION_ADDR_B, WALK_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			walking.threshold_b = (float)tempvalue / 1000000000;
		}
		//跑步标定值
		eeprom_status = ee_ReadBytes(tempbuffer, RUN_CALIBRATION_ADDR_A, RUN_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			running.threshold_a = (float)tempvalue / 1000000000;
		}
		eeprom_status = ee_ReadBytes(tempbuffer, RUN_CALIBRATION_ADDR_B, RUN_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			running.threshold_b = (float)tempvalue / 1000000000;
		}
		//静置标定值
		eeprom_status = ee_ReadBytes(tempbuffer, HANG_CALIBRATION_ADDR_A, HANG_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			hanging_a = (float)tempvalue / 1000000000;
		}
		eeprom_status = ee_ReadBytes(tempbuffer, HANG_CALIBRATION_ADDR_B, HANG_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			hanging_b = (float)tempvalue / 1000000000;
		}
	}
}

//绑定使用
void EEP_Binding_Write(void) {
	uint8_t buffer[5] = {0, 0, 0, 0, 0};
	eeprom_status = ee_WriteBytes(buffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE);
	if (eeprom_status != 1) {
		return;
	}
	eeprom_status = ee_WriteBytes(buffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE);
	if (eeprom_status != 1) {
		return;
	}
	eeprom_status = ee_WriteBytes(&binding_flag, ACTIVE_STATUS_ADDR, ACTIVE_STATUS_SIZE);
}

//步幅标定
void EEP_StepCalibration_Write(void) {
	uint8_t thresholdchar[5];
	uint32_t value = (uint32_t)(walking.threshold_a * 1000000000);
	thresholdchar[0] = (uint8_t)(value >> 24);
	thresholdchar[1] = (uint8_t)(value >> 16);
	thresholdchar[2] = (uint8_t)(value >> 8);
	thresholdchar[3] = (uint8_t)value;
	thresholdchar[4] = COM_Checksum(value);
	eeprom_status = ee_WriteBytes(thresholdchar, WALK_CALIBRATION_ADDR_A, WALK_CALIBRATION_SIZE);
	if (eeprom_status != 1) {
		return;
	}
	value = (uint32_t)(walking.threshold_b * 1000000000);
	thresholdchar[0] = (uint8_t)(value >> 24);
	thresholdchar[1] = (uint8_t)(value >> 16);
	thresholdchar[2] = (uint8_t)(value >> 8);
	thresholdchar[3] = (uint8_t)value;
	thresholdchar[4] = COM_Checksum(value);
	eeprom_status = ee_WriteBytes(thresholdchar, WALK_CALIBRATION_ADDR_B, WALK_CALIBRATION_SIZE);
	if (eeprom_status != 1) {
		return;
	}	
	value = (uint32_t)(running.threshold_a * 1000000000);
	thresholdchar[0] = (uint8_t)(value >> 24);
	thresholdchar[1] = (uint8_t)(value >> 16);
	thresholdchar[2] = (uint8_t)(value >> 8);
	thresholdchar[3] = (uint8_t)value;
	thresholdchar[4] = COM_Checksum(value);
	eeprom_status = ee_WriteBytes(thresholdchar, RUN_CALIBRATION_ADDR_A, RUN_CALIBRATION_SIZE);
	if (eeprom_status != 1) {
		return;
	}	
	value = (uint32_t)(running.threshold_b * 1000000000);
	thresholdchar[0] = (uint8_t)(value >> 24);
	thresholdchar[1] = (uint8_t)(value >> 16);
	thresholdchar[2] = (uint8_t)(value >> 8);
	thresholdchar[3] = (uint8_t)value;
	thresholdchar[4] = COM_Checksum(value);
	eeprom_status = ee_WriteBytes(thresholdchar, RUN_CALIBRATION_ADDR_B, RUN_CALIBRATION_SIZE);
}

//静置标定
void EEP_HangCalibration_Write(void) {
	uint8_t hangchar[HANG_CALIBRATION_SIZE];
	uint32_t temphang = (uint32_t)(hanging_a * 1000000000);
	hangchar[0] = (uint8_t)(temphang >> 24);
	hangchar[1] = (uint8_t)(temphang >> 16);
	hangchar[2] = (uint8_t)(temphang >> 8);
	hangchar[3] = (uint8_t)temphang;
	hangchar[4] = COM_Checksum(temphang);
	eeprom_status = ee_WriteBytes(hangchar, HANG_CALIBRATION_ADDR_A, HANG_CALIBRATION_SIZE);
	if (eeprom_status != 1) {
		return;
	}
	temphang = (uint32_t)(hanging_b * 1000000000);
	hangchar[0] = (uint8_t)(temphang >> 24);
	hangchar[1] = (uint8_t)(temphang >> 16);
	hangchar[2] = (uint8_t)(temphang >> 8);
	hangchar[3] = (uint8_t)temphang;
	hangchar[4] = COM_Checksum(temphang);
	eeprom_status = ee_WriteBytes(hangchar, HANG_CALIBRATION_ADDR_B, HANG_CALIBRATION_SIZE);
}

//休眠写E方
void EEP_Sleep_Write(void) {
	uint8_t buffer[5];
	if (binding_flag != 1) {
		return;
	}
	buffer[0] = (uint8_t)(walking.total_steps >> 24);
	buffer[1] = (uint8_t)(walking.total_steps >> 16);
	buffer[2] = (uint8_t)(walking.total_steps >> 8);
	buffer[3] = (uint8_t)(walking.total_steps);
	buffer[4] = COM_Checksum(walking.total_steps);
	eeprom_status = ee_WriteBytes(buffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE);
	
	buffer[0] = (uint8_t)(running.total_steps >> 24);
	buffer[1] = (uint8_t)(running.total_steps >> 16);
	buffer[2] = (uint8_t)(running.total_steps >> 8);
	buffer[3] = (uint8_t)(running.total_steps);
	buffer[4] = COM_Checksum(running.total_steps);
	eeprom_status = ee_WriteBytes(buffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE);
	
	uint32_t tempsoc = (uint32_t)(battsoc * 1000000000);
	buffer[0] = (uint8_t)(tempsoc >> 24);
	buffer[1] = (uint8_t)(tempsoc >> 16);
	buffer[2] = (uint8_t)(tempsoc >> 8);
	buffer[3] = (uint8_t)tempsoc;
	buffer[4] = COM_Checksum(tempsoc);
	eeprom_status = ee_WriteBytes(buffer, SOC_INITIAL_ADDR, SOC_INITIAL_SIZE);
}
