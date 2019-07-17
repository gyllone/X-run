#include "eeprom.h"

extern volatile uint32_t app_id;
extern uint8_t eeprom_status;
extern struct rtc_time systmtime;
extern uint8_t binding_flag;
extern Step walking;
extern Step running;
extern float weight;
extern float hanging;
extern float pressure;

void EEP_Initial_Read(void) {
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
		}
	}
}

//绑定使用
void EEP_Binding_Write(void) {
	if (ee_CheckOk() > 0 && eeprom_status < 1) {
		uint8_t data[ACTIVE_STATUS_SIZE];
		uint32_t idbuffer = app_id;
		data[0] = binding_flag;
		data[1] = (uint8_t)(idbuffer >> 24);
		data[2] = (uint8_t)(idbuffer >> 16);
		data[3] = (uint8_t)(idbuffer >> 8);
		data[4] = (uint8_t)idbuffer;
		if (ee_WriteBytes(data, ACTIVE_STATUS_ADDR, ACTIVE_STATUS_SIZE) < 1) {
			eeprom_status = 1;
    }
	}
  else {
		eeprom_status = 1;
	}
}

//体重标定
void EEP_Weight_Write(void) {
	uint32_t bigweight;
	uint8_t weightchar[WEIGHT_PRESSURE_SIZE];
	weight = pressure;
	walking.threshold = WALK_WEIGHT_RATIO * weight;
	running.threshold = RUN_WEIGHT_RATIO * weight;
	bigweight = (uint32_t)(weight * 1000000000);
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
void EEP_Hang_Write(void) {
	uint32_t bighang;
	uint8_t hangchar[WEIGHT_PRESSURE_SIZE];
	hanging = pressure * 0.98;
	bighang = (uint32_t)(hanging * 1000000000);
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
void EEP_Step_Write(void) {
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
}

//休眠写E方
void EEP_Sleep_Write(void) {
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
