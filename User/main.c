#include "stm32f10x.h"
#include "bsp_led.h"
#include "bsp_adc.h"
#include "bsp_i2c_ee.h"
#include "bsp_i2c_gpio.h"
#include "bsp_rtc.h"
#include "bsp_usart.h"
#include "bsp_iwdg.h"
#include "bsp_chipid.h"
#include "stm32f10x_iwdg.h"
#include "eeprom.h"
#include "functional.h"
#include <inttypes.h>

const uint32_t device_id = 0x2A16E4E8;

//ï¿½ï¿½×´Ì¬ï¿½ï¿½0x1AÎªï¿½ó¶¨³É¹ï¿½
volatile uint8_t binding_flag = 0x1A;
//APP ID
volatile uint32_t app_id;
//Eï¿½ï¿½×´Ì¬ï¿½ï¿½0ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½1ï¿½ï¿½ï¿½ï¿½
volatile uint8_t eeprom_status = 0;
//ï¿½ï¿½Øµï¿½Ñ¹
volatile float battvolt = 0;
//Ñ¹ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë²ï¿½Öµ
volatile float pressure = 0;
//ï¿½ï¿½ï¿½SOC(%)
volatile uint8_t battsoc = 0;
//ï¿½ï¿½Â·ï¿½ï¿½ï¿½ï¿½ï¿½Ï¢
volatile Step walking = {2.1, 0, 0};
//ï¿½Ü²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ï¢
volatile Step running = {1.6, 0, 0};
//ï¿½ï¿½ï¿½ï¿½
volatile float weight = 2.4;
//ï¿½ï¿½ï¿½ï¿½Ñ¹ï¿½ï¿½
volatile float hanging = 2.65;
//ï¿½ï¿½ï¿½Ã¼ï¿½ï¿½ï¿½
static uint16_t standingcounter = 0;
//ï¿½Æ²ï¿½×´Ì¬(0Îªï¿½Æ²ï¿½ï¿½ï¿½1Îªï¿½Ø±ï¿½)
volatile uint8_t stepflag = 1;

//ÏµÍ³Ê±ï¿½ï¿½
struct rtc_time systmtime = {
	0, 0, 0, 8, 1, 2019, 0
};

//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú½ï¿½ï¿½Õ¼ï¿½ï¿½ï¿½
uint8_t receivecounter = 0;
//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½×´Ì¬
volatile uint8_t connectstatus = 0;

static uint8_t lastconnectstatus = 0;
static uint8_t overtimecounter = 0;

void sendmessage(void) {
	if (battsoc <= 5) {
		return;
	}
	if (binding_flag != 0x1A) {
		switch (connectstatus) {
			case 1:
				Usart_binding_sendseed();
				break;
			case 3:
				Usart_binding_verify();
				break;
			case 5:
				binding_flag = 0x1A;
				EEP_binding_write();
				Usart_SendByte(USART1, 0xAA);
				connectstatus = 0;
				stepflag = 1;
				break;
		}
	}
	else {
		switch (connectstatus) {
			case 1:
				Usart_sendsoc();
			  break;
			case 3:
				Usart_sendkey();
			  break;
			case 5:
				Usart_sendstep();
			  EEP_step_write();
			  break;
			case 6:
				weight = pressure;
				EEP_weight_write();
				Usart_SendByte(USART1, 0xAA);
				connectstatus = 0;
			  stepflag = 1;
			case 7:
				hanging = pressure * 0.98;
				EEP_hang_write();
			  Usart_SendByte(USART1, 0xAA);
				connectstatus = 0;
			  stepflag = 1;
		}
	}
}
		
void receivemessage(void) {
	if (battsoc <= 5) {
		return;
	}
	if (binding_flag != 0x1A) {
		switch (connectstatus) {
			case 0:
				Usart_binding_listen();
				break;
		  case 2:
				Usart_binding_receivekey();
				break;
			case 4:
				Usart_binding_receiveid();
			  break;
		}
	}
  else {
		switch (connectstatus) {
			case 0:
				Usart_upload_listen();
				break;
			case 2:
				Usart_receiveseed();
			  break;
			case 4:
				if (USART_ReceiveData(USART1) == 0xBB) {
					connectstatus++;
				}
				else if (USART_ReceiveData(USART1) == 0xCC) {
				  connectstatus = 6;
				}
				else if (USART_ReceiveData(USART1) == 0xDD) {
					connectstatus = 7;
				}
				else {
					connectstatus = 0;
					stepflag = 1;
				}
				break;
		}
	}
}

void listen_reset(void) {
	lastconnectstatus = connectstatus;
	if (lastconnectstatus == 0 && connectstatus > 0) {
		overtimecounter = 1;
	}
	else if (lastconnectstatus == 0 && connectstatus == 0) {
		overtimecounter = 0;
	}
	else {
		overtimecounter++;
	}
	if (overtimecounter > 5) {
		overtimecounter = 0;
		connectstatus = 0;
		lastconnectstatus = 0;
		receivecounter = 0;
		if (binding_flag == 0x1A) {
			stepflag = 1;
		}
	}
}

//RTCï¿½Ð¶Ï´ï¿½ï¿½ï¿½
void RTC_IRQHandler(void)
{
	//ÃëÖÐ¶Ï
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET) 
	{
		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);
		RTC_WaitForLastTask();
		//ï¿½ï¿½ï¿½ï¿½ï¿½Ð¶Ï´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
		FUNC_battSOC_caculation();
		listen_reset();
	}
}

//ï¿½ï¿½ï¿½ï¿½ï¿½Ð¶Ï´ï¿½ï¿½ï¿½
void USART1_IRQHandler(void) {
	//ï¿½ï¿½ï¿½ï¿½ï¿½Ð¶ï¿½
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		receivemessage();
	}
}

void Initialization(void) {
  //ledï¿½ï¿½Ê¼ï¿½ï¿½
	LED_GPIO_Config();
	//adcï¿½ï¿½Ê¼ï¿½ï¿½
	ADCx_Init();
	//Ð¾Æ¬ID
	Get_ChipID();
	//Eï¿½ï¿½ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½È¡
	EEP_initial_read();
	//ï¿½ï¿½ï¿½Ú³ï¿½Ê¼ï¿½ï¿½
	USART_Config();
	//ï¿½ï¿½ï¿½ï¿½ï¿½ë¹¦ï¿½Ü³ï¿½Ê¼ï¿½ï¿½
	FUNC_functional_initial();
	//ÊÇ·ñÊÇ´ÓstandyÖÐÍË³öµÄ
	if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) {
		PWR_ClearFlag(PWR_FLAG_SB);
		//Èç¹ûÈÔ´¦ÓÚÐü¸¡×´Ì¬£¬¼ÌÐøÐÝÃß
		if (pressure > hanging) {
			RTC_SetAlarm(RTC_GetCounter() + 25); //25sºó»½ÐÑ
			RTC_WaitForLastTask();
			IWDG_Feed();
			IWDG_Config(IWDG_Prescaler_256 ,4095);
			LEDDELAY;
			PWR_EnterSTANDBYMode();
		}
	}
	//Î´ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Æ²ï¿½ï¿½ï¿½ï¿½ï¿½
	if (binding_flag == 0x1A) {
		stepflag = 1;
	}
	//¿´ÃÅ¹·³õÊ¼»¯
	IWDG_Init();
	LEDDELAY;
}

//ï¿½ï¿½ï¿½ï¿½Í¹ï¿½ï¿½ï¿½Ä£Ê½ï¿½Ð¶ï¿½
uint8_t SleepOrNot(void) {
	//SOC = 0Ö±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	if (battsoc < 1) {
		return 0;
	}
	//¾²ÖÃ×´Ì¬ÏÂ25sºó½øÈë´ý»ú
	if (pressure > hanging && stepflag) {
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

int main(void)
{	
	Initialization();
	//ï¿½ï¿½Ñ­ï¿½ï¿½5msÒ»ï¿½ï¿½
	while (SleepOrNot())
	{
		sendmessage();
		FUNC_step_counter();
		IWDG_Feed();
	}
	EEP_sleep_write();
	LEDDELAY;
	RTC_SetAlarm(RTC_GetCounter() + 25); //25sï¿½ï¿½ï¿½ï¿½
	RTC_WaitForLastTask();
	IWDG_Config(IWDG_Prescaler_256 ,4095); //ï¿½ï¿½ï¿½Å¹ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Îªï¿½ï¿½ï¿½
	IWDG_Feed();
	//½øÈëµÍ¹¦ºÄÄ£Ê½
	PWR_EnterSTANDBYMode();
}

/*********************************************END OF FILE**********************/
