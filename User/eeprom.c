#include "eeprom.h"
#include "bsp_rtc.h"
#include "functional.h"
#include "communication.h"

extern struct rtc_time systmtime;

extern uint8_t eeprom_status;
extern uint8_t binding_flag;
extern uint32_t app_id;
extern Step walking;
extern Step running;
extern float hanging;

void EEP_Initial_Read(void) {
	uint8_t first_flash_flag = 0;
	uint8_t bindbuffer[6];
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
	
	//检查设备绑定状态
	eeprom_status = ee_ReadBytes(bindbuffer, ACTIVE_STATUS_ADDR, ACTIVE_STATUS_SIZE);
	if (eeprom_status != 1) {
		return;
	}
	tempvalue = (uint32_t)bindbuffer[1] << 24 | (uint32_t)bindbuffer[2] << 16 |
					    (uint32_t)bindbuffer[3] << 8 | (uint32_t)bindbuffer[4];
	if (bindbuffer[0] == 1 && COM_Checksum(tempvalue) == bindbuffer[5]) {
			binding_flag = bindbuffer[0];
			app_id = tempvalue;
	}
	
	if (binding_flag == 1) {
		//走路信息
		eeprom_status = ee_ReadBytes(tempbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			walking.current_steps = tempvalue;
		}
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
		eeprom_status = ee_ReadBytes(tempbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			running.current_steps = tempvalue;
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
		eeprom_status = ee_ReadBytes(tempbuffer, WALK_CALIBRATION_ADDR, WALK_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			walking.threshold = (float)tempvalue / 1000000000;
		}
		//跑步标定值
		eeprom_status = ee_ReadBytes(tempbuffer, RUN_CALIBRATION_ADDR, RUN_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			running.threshold = (float)tempvalue / 1000000000;
		}
		//静置标定值
		eeprom_status = ee_ReadBytes(tempbuffer, HANG_CALIBRATION_ADDR, HANG_CALIBRATION_SIZE);
		if (eeprom_status != 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			hanging = (float)tempvalue / 1000000000;
		}
	}
}

//绑定使用
void EEP_Binding_Write(void) {
	uint8_t data[ACTIVE_STATUS_SIZE];
	uint8_t buffer[5] = {0, 0, 0, 0, 0};
	eeprom_status = ee_WriteBytes(buffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE);
	eeprom_status = ee_WriteBytes(buffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE);
	eeprom_status = ee_WriteBytes(buffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE);
	eeprom_status = ee_WriteBytes(buffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE);

	buffer[0] = 0x11;
	buffer[1] = 0xe1;
	buffer[2] = 0xa3;
	buffer[3] = 0x00;
	buffer[4] = 3;
	eeprom_status = ee_WriteBytes(buffer, WALK_CALIBRATION_ADDR, WALK_CALIBRATION_SIZE);
	
	buffer[0] = 0x1d;
	buffer[1] = 0xcd;
	buffer[2] = 0x65;
	buffer[3] = 0x00;
	buffer[4] = 5;
	eeprom_status = ee_WriteBytes(buffer, RUN_CALIBRATION_ADDR, RUN_CALIBRATION_SIZE);
	
	buffer[0] = 0x9b;
	buffer[1] = 0x91;
	buffer[2] = 0x70;
	buffer[3] = 0x80;
	buffer[4] = 9;
	eeprom_status = ee_WriteBytes(buffer, HANG_CALIBRATION_ADDR, HANG_CALIBRATION_SIZE);
	
	if (eeprom_status != 1) {
		return;
	}
	data[0] = binding_flag;
	data[1] = (uint8_t)(app_id >> 24);
	data[2] = (uint8_t)(app_id >> 16);
	data[3] = (uint8_t)(app_id >> 8);
	data[4] = (uint8_t)app_id;
	data[5] = COM_Checksum(app_id);
	eeprom_status = ee_WriteBytes(data, ACTIVE_STATUS_ADDR, ACTIVE_STATUS_SIZE);
}

//步幅标定
void EEP_StepCalibration_Write(void) {
	uint8_t thresholdchar[5];
	uint32_t value = (uint32_t)(walking.threshold * 1000000000);
	thresholdchar[0] = (uint8_t)(value >> 24);
	thresholdchar[1] = (uint8_t)(value >> 16);
	thresholdchar[2] = (uint8_t)(value >> 8);
	thresholdchar[3] = (uint8_t)value;
	thresholdchar[4] = COM_Checksum(value);
	eeprom_status = ee_WriteBytes(thresholdchar, WALK_CALIBRATION_ADDR, WALK_CALIBRATION_SIZE);

	if (eeprom_status != 1) {
		return;
	}
	value = (uint32_t)(running.threshold * 1000000000);
	thresholdchar[0] = (uint8_t)(value >> 24);
	thresholdchar[1] = (uint8_t)(value >> 16);
	thresholdchar[2] = (uint8_t)(value >> 8);
	thresholdchar[3] = (uint8_t)value;
	thresholdchar[4] = COM_Checksum(value);
	eeprom_status = ee_WriteBytes(thresholdchar, RUN_CALIBRATION_ADDR, RUN_CALIBRATION_SIZE);
}

//静置标定
void EEP_HangCalibration_Write(void) {
	uint32_t temphang;
	uint8_t hangchar[HANG_CALIBRATION_SIZE];
	temphang = (uint32_t)(hanging * 1000000000);
	hangchar[0] = (uint8_t)(temphang >> 24);
	hangchar[1] = (uint8_t)(temphang >> 16);
	hangchar[2] = (uint8_t)(temphang >> 8);
	hangchar[3] = (uint8_t)temphang;
	hangchar[4] = COM_Checksum(temphang);
	eeprom_status = ee_WriteBytes(hangchar, HANG_CALIBRATION_ADDR, HANG_CALIBRATION_SIZE);
}

//上传走路步数
void EEP_WalkStep_Write(void) {
	uint8_t stepbuffer[CYCLE_WALKSTEP_SIZE] = {0, 0, 0, 0, 0};
	eeprom_status = ee_WriteBytes(stepbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE);
}

//上传跑路步数
void EEP_RunStep_Write(void) {
	uint8_t stepbuffer[CYCLE_RUNSTEP_SIZE] = {0, 0, 0, 0, 0};
	eeprom_status = ee_WriteBytes(stepbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE);
}

//休眠写E方
void EEP_Sleep_Write(void) {
	uint8_t stepbuffer[5];
	if (binding_flag != 1) {
		return;
	}
	stepbuffer[0] = (uint8_t)(walking.current_steps >> 24);
	stepbuffer[1] = (uint8_t)(walking.current_steps >> 16);
	stepbuffer[2] = (uint8_t)(walking.current_steps >> 8);
	stepbuffer[3] = (uint8_t)(walking.current_steps);
	stepbuffer[4] = COM_Checksum(walking.current_steps);
	eeprom_status = ee_WriteBytes(stepbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE);
	
	stepbuffer[0] = (uint8_t)(walking.total_steps >> 24);
	stepbuffer[1] = (uint8_t)(walking.total_steps >> 16);
	stepbuffer[2] = (uint8_t)(walking.total_steps >> 8);
	stepbuffer[3] = (uint8_t)(walking.total_steps);
	stepbuffer[4] = COM_Checksum(walking.total_steps);
	eeprom_status = ee_WriteBytes(stepbuffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE);
	
	stepbuffer[0] = (uint8_t)(running.current_steps >> 24);
	stepbuffer[1] = (uint8_t)(running.current_steps >> 16);
	stepbuffer[2] = (uint8_t)(running.current_steps >> 8);
	stepbuffer[3] = (uint8_t)(running.current_steps);
	stepbuffer[4] = COM_Checksum(running.current_steps);
	eeprom_status = ee_WriteBytes(stepbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE);
	
	stepbuffer[0] = (uint8_t)(running.total_steps >> 24);
	stepbuffer[1] = (uint8_t)(running.total_steps >> 16);
	stepbuffer[2] = (uint8_t)(running.total_steps >> 8);
	stepbuffer[3] = (uint8_t)(running.total_steps);
	stepbuffer[4] = COM_Checksum(running.total_steps);
	eeprom_status = ee_WriteBytes(stepbuffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE);
}
