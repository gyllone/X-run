#include "bsp_adc.h"
#include "bsp_rtc.h"
#include "bsp_iwdg.h"
#include "eeprom.h"
#include "functional.h"
#include <inttypes.h>

<<<<<<< Updated upstream
const uint32_t device_id = 0x2A16E4E8;

//ï¿½ï¿½×´Ì¬ï¿½ï¿½0x1AÎªï¿½ó¶¨³É¹ï¿½
volatile uint8_t binding_flag = 0x1A;
//APP ID
volatile uint32_t app_id;
//Eï¿½ï¿½×´Ì¬ï¿½ï¿½0ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½1ï¿½ï¿½ï¿½ï¿½
volatile uint8_t eeprom_status = 0;
//ï¿½ï¿½Øµï¿½Ñ¹
=======
//¼¤»îÂë
const uint32_t activation_code = 0x2A16E4E8; //0x80C5A77A£¬0x4A8B3FFF
const uint16_t sw_version = 0x3039;

//°ó¶¨×´Ì¬(0Î´°ó¶¨£¬1°ó¶¨)
uint8_t binding_flag = 1; //20191005¸ü¸ÄÎª1
//E·½×´Ì¬(0Îª¹ÊÕÏ£¬1Õý³£)
uint8_t eeprom_status = 1;
//×ßÂ·Ïà¹ØÐÅÏ¢
Step walking = {0.8f, 0};
//ÅÜ²½Ïà¹ØÐÅÏ¢
Step running = {1.1f, 0};
//FFTÌî³ä¼ÆÊý
uint16_t fillcounter = 0;
//¾²ÖÃÑ¹Á¦
float hanging = 3.27f;
//Ñ¹Á¦Öµ
float pressure = 0;
//¼Æ²½±êÖ¾(0±íÊ¾Î´ÔÊÐí¼Æ²½£¬1±íÊ¾¼Æ²½ÖÐ£¬2±íÊ¾ÌåÖØ±ê¶¨£¬3±íÊ¾¾²ÖÃ±ê¶¨)
uint8_t step_flag = 0;
//³äµç×´Ì¬(0Î´³äµç£¬1³äµç) ¿¼ÂÇ·ÅÔÚRTCÖÐ¶ÏÖÐÅÐ¶Ï
volatile uint8_t charging_flag = 0;
//´®¿Ú»Ø¸´±êÖ¾
volatile uint8_t response_flag = 0;
//µç³ØÊµ¼ÊSOC
volatile float battsoc = 0;
//µç³ØµçÑ¹
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
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
=======
static void Initialization(void) {
	//adc³õÊ¼»¯
	ADCx_Init();
	//¶ÁÈ¡E·½³õÊ¼Öµ
	EEP_Initial_Read();
	//³õÊ¼»¯ÂË²¨µçÑ¹²¢¼ÆËã³õÊ¼SOC
	FUNC_Functional_Initial();
	//³äµçÓë·ñ
	FUNC_ChargeOrNot();
	//ÊÇ·ñÊÇ´Ó´ý»úÄ£Ê½ÖÐÍË³öµÄ
	if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) {
		PWR_ClearFlag(PWR_FLAG_SB);
		//Èç¹ûÈÔ´¦ÓÚ¾²ÖÃ×´Ì¬£¬¼ÌÐøÐÝÃß
		if (pressure > HANG_RATIO * hanging && charging_flag == 0) {
			RTC_SetAlarm(RTC_GetCounter() + 5); //5sºó»½ÐÑ
			RTC_WaitForLastTask();
			IWDG_Feed();
			IWDG_Config(IWDG_Prescaler_256, 938);
			PWR_EnterSTANDBYMode();
		}
	}
	//LED³õÊ¼»¯
	LED_GPIO_Config();
	//USART³õÊ¼ÅäÖÃ
	USART_Config();
	//20191005Ìí¼Ó
	binding_flag = 1;
	//Èô°ó¶¨Ôò¿ªÆô¼Æ²½
	if (binding_flag == 1) {
		step_flag = 1;
	}
	//¿´ÃÅ¹·³õÊ¼»¯
	IWDG_Init();
>>>>>>> Stashed changes
}

//RTCï¿½Ð¶Ï´ï¿½ï¿½ï¿½
void RTC_IRQHandler(void)
{
	//ÃëÖÐ¶Ï
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET) 
	{
		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);
		RTC_WaitForLastTask();
<<<<<<< Updated upstream
		//ï¿½ï¿½ï¿½ï¿½ï¿½Ð¶Ï´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
		FUNC_battSOC_caculation();
		listen_reset();
=======
		FUNC_Led_Breath();
		FUNC_ChargeOrNot();
		FUNC_BattSOC_Caculation();
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
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
			//LEDDELAY;
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
=======
int main(void) {
>>>>>>> Stashed changes
	Initialization();
	//ï¿½ï¿½Ñ­ï¿½ï¿½5msÒ»ï¿½ï¿½
	while (SleepOrNot())
	{
		sendmessage();
		FUNC_step_counter();
		IWDG_Feed();
	}
<<<<<<< Updated upstream
	EEP_sleep_write();
	LEDDELAY;
	RTC_SetAlarm(RTC_GetCounter() + 25); //25sï¿½ï¿½ï¿½ï¿½
	RTC_WaitForLastTask();
	IWDG_Config(IWDG_Prescaler_256 ,4095); //ï¿½ï¿½ï¿½Å¹ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Îªï¿½ï¿½ï¿½
=======
	EEP_Sleep_Write();
	RTC_SetAlarm(RTC_GetCounter() + 5); //5sºó»½ÐÑ
	RTC_WaitForLastTask();
	IWDG_Config(IWDG_Prescaler_256, 938);
>>>>>>> Stashed changes
	IWDG_Feed();
	//½øÈëµÍ¹¦ºÄÄ£Ê½
	PWR_EnterSTANDBYMode();
}

/*********************************************END OF FILE**********************/
