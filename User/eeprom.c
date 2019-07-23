#include "eeprom.h"

extern struct rtc_time systmtime;

extern uint32_t app_id;
extern uint8_t eeprom_status;
extern uint8_t binding_flag;
extern Step walking;
extern Step running;
extern float weight;
extern float hanging;

void EEP_Initial_Read(void) {
	uint8_t first_flash_flag = 0;
	uint8_t bindbuffer[6];
	uint8_t tempbuffer[5];
	uint32_t tempvalue;
	//检查E方是否正常
	eeprom_status = ee_CheckOk();
	if (eeprom_status < 1) {
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
	if (eeprom_status < 1) {
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
		if (eeprom_status < 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			walking.current_steps = tempvalue;
		}
		//走路信息
		eeprom_status = ee_ReadBytes(tempbuffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE);
		if (eeprom_status < 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			walking.total_steps = tempvalue;
		}
		//跑步信息
		eeprom_status = ee_ReadBytes(tempbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE);
		if (eeprom_status < 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			running.current_steps = tempvalue;
		}
		//跑步信息
		eeprom_status = ee_ReadBytes(tempbuffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE);
		if (eeprom_status < 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			running.total_steps = tempvalue;
		}
		//体重信息
		eeprom_status = ee_ReadBytes(tempbuffer, WEIGHT_PRESSURE_ADDR, WEIGHT_PRESSURE_SIZE);
		if (eeprom_status < 1) {
			return;
		}
		tempvalue = (uint32_t)(tempbuffer[0] << 24) | (uint32_t)(tempbuffer[1] << 16) |
								(uint32_t)(tempbuffer[2] << 8) | (uint32_t)tempbuffer[3];
		if (COM_Checksum(tempvalue) == tempbuffer[4]) {
			weight = (float)tempvalue / 1000000000;
			walking.threshold = weight * WALK_WEIGHT_RATIO;
			running.threshold = weight * RUN_WEIGHT_RATIO;
		}
		//静置信息
		eeprom_status = ee_ReadBytes(tempbuffer, HANG_PRESSURE_ADDR, HANG_PRESSURE_SIZE);
		if (eeprom_status < 1) {
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
	uint8_t stepbuffer[5] = {0, 0, 0, 0, 0};
	eeprom_status = ee_WriteBytes(stepbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE);
	eeprom_status = ee_WriteBytes(stepbuffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE);
	eeprom_status = ee_WriteBytes(stepbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE);
	eeprom_status = ee_WriteBytes(stepbuffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE);	
	if (eeprom_status < 1) {
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

//体重标定
void EEP_Weight_Write(void) {
	uint32_t tempweight;
	uint8_t weightchar[WEIGHT_PRESSURE_SIZE];
	tempweight = (uint32_t)(weight * 1000000000);
	weightchar[0] = (uint8_t)(tempweight >> 24);
	weightchar[1] = (uint8_t)(tempweight >> 16);
	weightchar[2] = (uint8_t)(tempweight >> 8);
	weightchar[3] = (uint8_t)tempweight;
	weightchar[4] = COM_Checksum(tempweight);
	eeprom_status = ee_WriteBytes(weightchar, WEIGHT_PRESSURE_ADDR, WEIGHT_PRESSURE_SIZE);
}

//静置标定
void EEP_Hang_Write(void) {
	uint32_t temphang;
	uint8_t hangchar[HANG_PRESSURE_SIZE];
	temphang = (uint32_t)(hanging * 1000000000);
	hangchar[0] = (uint8_t)(temphang >> 24);
	hangchar[1] = (uint8_t)(temphang >> 16);
	hangchar[2] = (uint8_t)(temphang >> 8);
	hangchar[3] = (uint8_t)temphang;
	hangchar[4] = COM_Checksum(temphang);
	eeprom_status = ee_WriteBytes(hangchar, HANG_PRESSURE_ADDR, HANG_PRESSURE_SIZE);
}

//上传步数使用
void EEP_WalkStep_Write(void) {
	uint8_t stepbuffer[CYCLE_WALKSTEP_SIZE] = {0, 0, 0, 0, 0};
	eeprom_status = ee_WriteBytes(stepbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE);
}

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
