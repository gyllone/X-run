#include "eeprom.h"

extern volatile uint8_t eeprom_status;
extern struct rtc_time systmtime;
<<<<<<< Updated upstream
extern volatile uint8_t binding_flag;
extern volatile uint32_t app_id;
extern volatile Step walking;
extern volatile Step running;
extern volatile float weight;
extern volatile float hanging;

void EEP_initial_read(void) {
=======
extern uint8_t eeprom_status;
extern uint8_t binding_flag;
extern Step walking;
extern Step running;
extern float hanging;
extern volatile float battsoc;

void EEP_Initial_Read(void) {
>>>>>>> Stashed changes
	uint8_t first_flash_flag = 0;
  if (ee_CheckOk() > 0) {
		if (ee_ReadBytes(&first_flash_flag, FIRST_FLASH_ADDR, FIRST_FLASH_SIZE) < 1) {
			eeprom_status = 1;
		}
	}
	else {
		eeprom_status = 1;
	}
    
  if (first_flash_flag != 0x1A && eeprom_status < 1) {
		RTC_Configuration(); //MCU首次烧写时使用
    first_flash_flag = 0x1A;
    if (ee_CheckOk() > 0) {
			if (ee_WriteBytes(&first_flash_flag, FIRST_FLASH_ADDR, FIRST_FLASH_SIZE) < 1) {
				eeprom_status = 1;
			}
    }
    else {
			eeprom_status = 1;
    }
	}
  else {
		RTC_CheckAndConfig(&systmtime);
	}

	//检查设备绑定状态
<<<<<<< Updated upstream
  if (ee_CheckOk() > 0 && eeprom_status < 1) {
		uint8_t bindbuffer[ACTIVE_STATUS_SIZE];
		if (ee_ReadBytes(bindbuffer, ACTIVE_STATUS_ADDR, ACTIVE_STATUS_SIZE) > 0) {
			binding_flag = bindbuffer[0];
			if (binding_flag == 0x1A) {
				app_id = (uint32_t)bindbuffer[1] << 24 | (uint32_t)bindbuffer[2] << 16 |
								 (uint32_t)bindbuffer[3] << 8 | (uint32_t)bindbuffer[4];
			}
    }
		else {
			eeprom_status = 1;
		}
	}
  else {
		eeprom_status = 1;
	}

	if (binding_flag == 0x1A) {
		uint8_t stepbuffer[CYCLE_WALKSTEP_SIZE];
		uint8_t totalstepbuffer[TOTAL_WALKSTEP_SIZE];
		uint8_t weightchar[WEIGHT_PRESSURE_SIZE];
		uint8_t hangchar[HANG_PRESSURE_SIZE];
		
		//读取体重E方信息
		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_ReadBytes(weightchar, WEIGHT_PRESSURE_ADDR, WEIGHT_PRESSURE_SIZE) > 0) {
				uint32_t tempweight = (uint32_t)(weightchar[0] << 24) | (uint32_t)(weightchar[1] << 16) |
															(uint32_t)(weightchar[2] << 8) | (uint32_t)(weightchar[3] << 8);
				weight = (float)tempweight / 1000000000;
				walking.threshold = weight * WALK_WEIGHT_RATIO;
				running.threshold = weight * RUN_WEIGHT_RATIO;
			}
			else {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
		}

		//读取静置E方信息
		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_ReadBytes(hangchar, WEIGHT_PRESSURE_ADDR, WEIGHT_PRESSURE_SIZE) > 0) {
				uint32_t temphang = (uint32_t)(hangchar[0] << 24) | (uint32_t)(hangchar[1] << 16) |
														(uint32_t)(hangchar[2] << 8) | (uint32_t)(hangchar[3] << 8);
				hanging = (float)temphang / 1000000000;
			}
			else {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
		}
		
		//读取走路相关E方信息
		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_ReadBytes(stepbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE) > 0) {
				walking.current_steps = (uint32_t)(stepbuffer[0] << 24) | (uint32_t)(stepbuffer[1] << 16) |
																(uint32_t)(stepbuffer[2] << 8) | (uint32_t)(stepbuffer[3] << 8);
			}
			else {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
		}

		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_ReadBytes(totalstepbuffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE) > 0) {
				walking.total_steps = (uint32_t)(totalstepbuffer[0] << 24) | (uint32_t)(totalstepbuffer[1] << 16) |
															(uint32_t)(totalstepbuffer[2] << 8) | (uint32_t)(totalstepbuffer[3] << 8);
			}
			else {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
		}

		//读取跑步相关E方信息
		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_ReadBytes(stepbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE) > 0) {
				running.current_steps = (uint32_t)(stepbuffer[0] << 24) | (uint32_t)(stepbuffer[1] << 16) |
																(uint32_t)(stepbuffer[2] << 8) | (uint32_t)(stepbuffer[3] << 8);
			}
			else {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
		}

		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_ReadBytes(totalstepbuffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE) > 0) {
				running.total_steps = (uint32_t)(totalstepbuffer[0] << 24) | (uint32_t)(totalstepbuffer[1] << 16) |
															(uint32_t)(totalstepbuffer[2] << 8) | (uint32_t)(totalstepbuffer[3] << 8);
			}
			else {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
=======
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
>>>>>>> Stashed changes
		}
	}
}

//绑定使用
void EEP_binding_write(void) {
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		uint8_t data[ACTIVE_STATUS_SIZE];
		data[0] = binding_flag;
		data[1] = (uint8_t)(app_id >> 24);
		data[2] = (uint8_t)(app_id >> 16);
		data[3] = (uint8_t)(app_id >> 8);
		data[4] = (uint8_t)app_id;
		if (ee_WriteBytes(data, ACTIVE_STATUS_ADDR, ACTIVE_STATUS_SIZE) < 1) {
			eeprom_status = 1;
    }
	}
  else {
		eeprom_status = 1;
	}
}

<<<<<<< Updated upstream
//体重标定
void EEP_weight_write(void) {
		uint32_t bigweight = (uint32_t)(weight * 1000000000);
		uint8_t weightchar[WEIGHT_PRESSURE_SIZE];
		weightchar[0] = (uint8_t)(bigweight >> 24);
		weightchar[1] = (uint8_t)(bigweight >> 16);
		weightchar[2] = (uint8_t)(bigweight >> 8);
		weightchar[3] = (uint8_t)bigweight;
	
		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_WriteBytes(weightchar, WEIGHT_PRESSURE_ADDR, WEIGHT_PRESSURE_SIZE) < 1) {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
		}
}

//静置标定
void EEP_hang_write(void) {
		uint32_t bighang = (uint32_t)(hanging * 1000000000);
		uint8_t hangchar[WEIGHT_PRESSURE_SIZE];
		hangchar[0] = (uint8_t)(bighang >> 24);
		hangchar[1] = (uint8_t)(bighang >> 16);
		hangchar[2] = (uint8_t)(bighang >> 8);
		hangchar[3] = (uint8_t)bighang;
		if (ee_CheckOk() > 0 && eeprom_status < 1) {
			if (ee_WriteBytes(hangchar, WEIGHT_PRESSURE_ADDR, WEIGHT_PRESSURE_SIZE) < 1) {
				eeprom_status = 1;
			}
		}
		else {
			eeprom_status = 1;
		}
}

//上传步数使用
void EEP_step_write(void) {
	uint8_t stepbuffer[CYCLE_WALKSTEP_SIZE] = { 0, 0, 0, 0 };
	walking.current_steps = 0;
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		if (ee_WriteBytes(stepbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE) < 1) {
			eeprom_status = 1;
		}
	}
	else {
		eeprom_status = 1;
	}
	
	running.current_steps = 0;
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		if (ee_WriteBytes(stepbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE) < 1) {
			eeprom_status = 1;
		}
	}
	else {
		eeprom_status = 1;
	}
=======
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
	uint8_t hangchar[HANG_CALIBRATION_SIZE];
	uint32_t temphang = (uint32_t)(hanging * 1000000000);
	hangchar[0] = (uint8_t)(temphang >> 24);
	hangchar[1] = (uint8_t)(temphang >> 16);
	hangchar[2] = (uint8_t)(temphang >> 8);
	hangchar[3] = (uint8_t)temphang;
	hangchar[4] = COM_Checksum(temphang);
	eeprom_status = ee_WriteBytes(hangchar, HANG_CALIBRATION_ADDR, HANG_CALIBRATION_SIZE);
>>>>>>> Stashed changes
}

//休眠写E方
void EEP_sleep_write(void) {
	uint8_t stepbuffer[CYCLE_WALKSTEP_SIZE];
	uint8_t totalstepbuffer[TOTAL_WALKSTEP_SIZE];
	stepbuffer[0] = (uint8_t)(walking.current_steps >> 24);
	stepbuffer[1] = (uint8_t)(walking.current_steps >> 16);
	stepbuffer[2] = (uint8_t)(walking.current_steps >> 8);
	stepbuffer[3] = (uint8_t)(walking.current_steps);
	totalstepbuffer[0] = (uint8_t)(walking.total_steps >> 24);
	totalstepbuffer[1] = (uint8_t)(walking.total_steps >> 16);
	totalstepbuffer[2] = (uint8_t)(walking.total_steps >> 8);
	totalstepbuffer[3] = (uint8_t)(walking.total_steps);
	
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		if (ee_WriteBytes(stepbuffer, CYCLE_WAKLSTEP_ADDR, CYCLE_WALKSTEP_SIZE) < 1) {
			eeprom_status = 1;
		}
	}
	else {
		eeprom_status = 1;
	}
	
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		if (ee_WriteBytes(totalstepbuffer, TOTAL_WALKSTEP_ADDR, TOTAL_WALKSTEP_SIZE) < 1) {
			eeprom_status = 1;
		}
	}
	else {
		eeprom_status = 1;
	}	
	
	stepbuffer[0] = (uint8_t)(running.current_steps >> 24);
	stepbuffer[1] = (uint8_t)(running.current_steps >> 16);
	stepbuffer[2] = (uint8_t)(running.current_steps >> 8);
	stepbuffer[3] = (uint8_t)(running.current_steps);
	totalstepbuffer[0] = (uint8_t)(running.total_steps >> 24);
	totalstepbuffer[1] = (uint8_t)(running.total_steps >> 16);
	totalstepbuffer[2] = (uint8_t)(running.total_steps >> 8);
	totalstepbuffer[3] = (uint8_t)(running.total_steps);
	
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		if (ee_WriteBytes(stepbuffer, CYCLE_RUNSTEP_ADDR, CYCLE_RUNSTEP_SIZE) < 1) {
			eeprom_status = 1;
		}
	}
	else {
		eeprom_status = 1;
	}
	
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		if (ee_WriteBytes(totalstepbuffer, TOTAL_RUNSTEP_ADDR, TOTAL_RUNSTEP_SIZE) < 1) {
			eeprom_status = 1;
		}
	}
	else {
		eeprom_status = 1;
	}
}
