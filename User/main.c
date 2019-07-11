#include "stm32f10x.h"
#include "bsp_led.h"
#include "bsp_adc.h"
#include "bsp_i2c_ee.h"
#include "bsp_i2c_gpio.h"
#include "bsp_rtc.h"
#include "bsp_iwdg.h"
#include "bsp_chipid.h"
#include "stm32f10x_iwdg.h"
#include "eeprom.h"
#include "functional.h"
#include <inttypes.h>
#include <math.h>

extern __IO uint16_t ADC_ConvertedValue[NOFCHANEL];
extern uint32_t ChipUniqueID[3];

extern void Crypto_CalcKey(uint32_t wSeed, uint32_t *keyset);
extern void Crypto_Random(uint32_t *randseedset);

//绑定状态，0x1A为绑定成功
uint8_t binding_flag = 0x1A;
//APP ID
uint32_t app_id[3];
//E方状态，0正常，1故障
uint8_t eeprom_status = 0;
//电池电压
float battvolt = 0;
//压力二次滤波值
float pressure = 0;
//电池SOC(%)
uint8_t battsoc = 0;
//走路相关信息
volatile Step walking = {2.1, 0, 0};
//跑步相关信息
volatile Step running = {1.6, 0, 0};
//体重
volatile float weight = 2.4;
//静置压力
volatile float hanging = 2.65;
//静置计数
uint16_t standingcounter = 0;
//计步状态(0为计步，1为关闭)
volatile uint8_t stepflag = 0;

struct rtc_time clocktime = {
	8, 0, 0, 7, 1, 2019, 0
};
//系统时间
struct rtc_time systmtime = {
	0, 0, 0, 7, 1, 2019, 0
};

//RTC中断处理
void RTC_IRQHandler(void)
{
	//2s中断
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET) 
	{
		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);
		RTC_WaitForLastTask();
		//添加中断处理内容
		FUNC_battSOC_caculation();
	}
	//闹钟中断
	if (RTC_GetITStatus(RTC_IT_ALR) != RESET) 
	{
		RTC_ClearITPendingBit(RTC_IT_ALR);
		RTC_WaitForLastTask();
	}
}

//进入低功耗模式判断
uint8_t SleepOrNot(void) {
	//SOC = 0直接休眠
	if (battsoc < 1) {
		//return 0;
	}
	//静置状态维持1min后休眠
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
  //led初始化
	LED_GPIO_Config();
	LED1_ON;
	LED2_ON;
	//adc初始化
	ADCx_Init();
	//芯片ID
	Get_ChipID();
	//E方初始化读取
	EEP_initial_read();
	//查看绑定状态
	if (binding_flag != 0x1A) {
		// ............
		// ............
		// ............
	}
	//采样与功能初始化
	FUNC_functional_initial();
	//看门狗初始化
	IWDG_Init();
	IWDG_Config(IWDG_Prescaler_16, 125);//看门狗溢出时间50ms
	LED1_OFF;
	LED2_OFF;
}

int main(void)
{	
	Initialization();
	//主循环10ms一次
	while (SleepOrNot())
	{
		FUNC_step_counter();
		IWDG_Feed();
	}
	EEP_sleep_write();
	RTC_SetAlarm(RTC_GetCounter() + 420); //420s后唤醒
	RTC_WaitForLastTask();
	IWDG_Config(IWDG_Prescaler_256 ,65535); //看门狗溢出时间设置为最大
	IWDG_Feed();
	//休眠时闪烁一下
	LED1_ON;
	LED2_ON;
	LEDDELAY;
	LED1_OFF;
	LED2_OFF;
	LEDDELAY;
	LED1_ON;
	LED2_ON;
	LEDDELAY;
	//进入低功耗模式
	PWR_EnterSTANDBYMode();
}

/*********************************************END OF FILE**********************/
