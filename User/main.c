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
#include "communication.h"

const uint32_t device_id = 0x2A16E4E8;
const uint8_t sw_version = 100;

//绑定状态(0未绑定，1绑定)
uint8_t binding_flag = 0;
//APP ID
uint32_t app_id;
//E方状态(0为故障，1正常)
uint8_t eeprom_status = 1;
//充电状态(0未充电，1充电) 考虑放在RTC中断中判断
volatile uint8_t charging_flag = 1;
//走路相关信息
Step walking = {1.8, 0, 0};
//跑步相关信息
Step running = {1.5, 0, 0};
//体重压力
float weight = 2.1;
//静置压力
float hanging = 2.55;
//计步标志位
volatile uint8_t step_flag = 0;
//通讯连接状态
volatile uint8_t interactstatus = 0;

//系统时间
struct rtc_time systmtime = {
	0, 0, 0, 8, 1, 2019, 0
};

//电池实际SOC
float actualsoc = 0;
//电池SOC
uint8_t displaysoc = 0;
//电池电压
volatile float battvolt = 0;
//压力值
float pressure = 0;

//蓝牙与E方供电
static void Powersupply_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	//打开供电
	GPIO_SetBits(GPIOA, GPIO_Pin_2);
}

static void Initialization(void) {
	//打开供电
	Powersupply_Init();
	//adc初始化
	ADCx_Init();
	//读取E方初始值
	EEP_Initial_Read();
	//初始化滤波电压并计算初始SOC
	FUNC_Functional_Initial();
	//是否是从待机模式中退出的
	if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) {
		PWR_ClearFlag(PWR_FLAG_SB);
		//如果仍处于静置状态，继续休眠
		if (pressure > hanging) {
			RTC_SetAlarm(RTC_GetCounter() + 25); //25s后唤醒
			RTC_WaitForLastTask();
			GPIO_SetBits(GPIOA, GPIO_Pin_2); //关闭供电
			IWDG_Feed();
			IWDG_Config(IWDG_Prescaler_256 ,4095);
			PWR_EnterSTANDBYMode();
		}
	}
	//LED初始化
	LED_GPIO_Config();
	//USART初始配置
	USART_Config();
	//若绑定则开启计步
	if (binding_flag == 1) {
		step_flag = 1;
	}
	//看门狗初始化
	IWDG_Init();
	LEDDELAY;
}

//RTC中断
void RTC_IRQHandler(void)
{
	//2秒中断(计算SOC)
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET) 
	{
		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);
		RTC_WaitForLastTask();
		//FUNC_Check_Charging();
		FUNC_BattSOC_Caculation();
	}
}

//串口接收中断
void USART1_IRQHandler(void) {
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		COM_Listening();
	}
}

int main(void)
{	
	Initialization();
	//主循环5ms一次
	while (FUNC_SleepOrNot())
	{
		COM_Listen_Reset();
		COM_Response();
		FUNC_Step_Counter();
		IWDG_Feed();
	}
	EEP_Sleep_Write();
	LEDDELAY;
	RTC_SetAlarm(RTC_GetCounter() + 25); //25s后
	RTC_WaitForLastTask();
	GPIO_ResetBits(GPIOA, GPIO_Pin_2); //关闭供电
	IWDG_Config(IWDG_Prescaler_256 ,4095);
	IWDG_Feed();
	//进入待机模式
	PWR_EnterSTANDBYMode();
}

/*********************************************END OF FILE**********************/
