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

//��״̬��0x1AΪ�󶨳ɹ�
volatile uint8_t binding_flag = 0x1A;
//APP ID
volatile uint32_t app_id;
//E��״̬��0������1����
volatile uint8_t eeprom_status = 0;
//��ص�ѹ
volatile float battvolt = 0;
//ѹ�������˲�ֵ
volatile float pressure = 0;
//���SOC(%)
volatile uint8_t battsoc = 0;
//��·�����Ϣ
volatile Step walking = {2.1, 0, 0};
//�ܲ������Ϣ
volatile Step running = {1.6, 0, 0};
//����
volatile float weight = 2.4;
//����ѹ��
volatile float hanging = 2.65;
//���ü���
static uint16_t standingcounter = 0;
//�Ʋ�״̬(0Ϊ�Ʋ���1Ϊ�ر�)
volatile uint8_t stepflag = 1;

//ϵͳʱ��
struct rtc_time systmtime = {
	0, 0, 0, 7, 1, 2019, 0
};

//�������ڽ��ռ���
uint8_t receivecounter = 0;
//������������״̬
volatile uint8_t connectstatus = 0;
static uint8_t lastconnectstatus = 0;

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
				stepflag = 0;
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
				EEP_weight_write();
				Usart_SendByte(USART1, 0xAA);
				connectstatus = 0;
			  stepflag = 0;
			case 7:
				EEP_hang_write();
			  Usart_SendByte(USART1, 0xAA);
				connectstatus = 0;
			  stepflag = 0;
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
					stepflag = 0;
				}
				break;
		}
	}
}

void listen_reset(void) {
	lastconnectstatus = connectstatus;
	if (lastconnectstatus > 0 && connectstatus > 0) {
		connectstatus = 0;
		lastconnectstatus = 0;
		receivecounter = 0;
		if (binding_flag == 0x1A) {
			stepflag = 0;
		}
	}
}

//RTC�жϴ���
void RTC_IRQHandler(void)
{
	//�����ж�
	if (RTC_GetITStatus(RTC_IT_ALR) != RESET)
	{
		RTC_ClearITPendingBit(RTC_IT_ALR);
		RTC_WaitForLastTask();
	}
	//5s�ж�
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET) 
	{
		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);
		RTC_WaitForLastTask();
		//�����жϴ�������
		FUNC_battSOC_caculation();
		listen_reset();
	}
}

//�����жϴ���
void USART1_IRQHandler(void) {
	//�����ж�
	 if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		 USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		 receivemessage();
	 }
 }	 

//����͹���ģʽ�ж�
uint8_t SleepOrNot(void) {
	//SOC = 0ֱ������
	if (battsoc < 1) {
		return 0;
	}
	//����״̬ά��10min������
	if (pressure > hanging && stepflag < 1) {
		standingcounter++;
		if (standingcounter > 6000) {
			return 0;
		}
	}
	else {
		standingcounter = 0;
	}
	return 1;
}

void Initialization(void) {
  //led��ʼ��
	LED_GPIO_Config();
	LED1_ON;
	LED2_ON;
	//adc��ʼ��
	ADCx_Init();
	//оƬID
	Get_ChipID();
	//E����ʼ����ȡ
	EEP_initial_read();
	//���ڳ�ʼ��
	USART_Config();
	//�����빦�ܳ�ʼ��
	FUNC_functional_initial();
	//δ��ʱ�������Ʋ�����
	if (binding_flag == 0x1A) {
		stepflag = 0;
	}
	//���Ź���ʼ��
	IWDG_Init();
	IWDG_Config(IWDG_Prescaler_16, 250);//���Ź����ʱ��100ms
	LED1_OFF;
	LED2_OFF;
}

int main(void)
{	
	Initialization();
	//��ѭ��10msһ��
	while (SleepOrNot())
	{
		sendmessage();
		FUNC_step_counter();
		IWDG_Feed();
	}
	EEP_sleep_write();
	RTC_SetAlarm(RTC_GetCounter() + 420); //420s����
	RTC_WaitForLastTask();
	IWDG_Config(IWDG_Prescaler_256 ,65535); //���Ź����ʱ������Ϊ���
	IWDG_Feed();
	//����ʱ��˸һ��
	LED1_ON;
	LED2_ON;
	LEDDELAY;
	LED1_OFF;
	LED2_OFF;
	//����͹���ģʽ
	PWR_EnterSTANDBYMode();
}

/*********************************************END OF FILE**********************/
