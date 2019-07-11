# stepcounter

stepcounter implemented by STM32f103

20190711更改点:

1. filter函数优化

2. SOC计算参数修改

3. 调整部分参数值

# 
# 底层驱动备注

1、PA4纽扣电池电压采集；PA5锂离子电池电压采集；PA6力敏电阻电压VR采集，力敏阻值换算R=1000000*VR/(3.3-VR)；

2、uint8_t filter()用于VR的消抖滤波，防止噪声干扰；

3、PC0为LED1;PC2为LED2;

4、实时时钟由两个中断：秒中断与闹钟中断。秒中断预留给上层应用，闹钟中断用于将MCU从待机模式唤醒，每隔5秒唤醒一次，唤醒后立刻喂狗

并修改iwdg溢出时间；

5、PWR_EnterSTANDBYMode()为待机模式，进入待机前需要设置iwdg的溢出时间，溢出时间需要大于闹钟周期，暂时设为8s；待机模式的触发条件需要上层提供，测试程序的触发条件设为每走两步触发一次；

6、RTC_Configuration()用于第一次烧写，该函数影响MCU初始化速度，之后烧写屏蔽该函数，后续烧写使用RTC_CheckAndConfig(&systmtime);

7、EEPROM为AT24C16，读写前必须运行函数ee_CheckOk()，否则驱动无法正常读写；

8、函数Get_ChipID()读取MCU的ID,每个MCU都有唯一的ID，上层程序把ID和用户账户进行关联。

# 
# 接口函数说明

Delay(__IO uint32_t nCount) nCount循环数，调节延时时长

uint8_t filter()消抖滤波，在主程序设置N的值调节滤波效果

void RTC_IRQHandler(void)实时时钟中断

if (RTC_GetITStatus(RTC_IT_SEC) != RESET) 秒中断，判断循环内放中断服务程序

if (RTC_GetITStatus(RTC_IT_ALR) != RESET) 闹钟中断，判断循环内放中断服务程序

RTC_Configuration();实时时钟初始化，用于新MCU第一次烧写

RTC_CheckAndConfig(&systmtime);;实时时钟初始化，systmtime系统时间，需要上层应用提供

RTC_SetAlarm(RTC_GetCounter()+T);闹钟设置，RTC_GetCounter()当前系统时间，RTC_GetCounter()+T闹钟中断发生的时间，T的单位为秒

RTC_WaitForLastTask();等待实时时钟寄存器配置完成，每一次实时时钟设置均需添加

LED_GPIO_Config();LED口初始化

LED1_ON;LED2_ON;LED1_OFF;LED2_OFF；LED灯的点亮与熄灭

ADCx_Init();AD初始化

ADC_ConvertedValue[]；AD采样结果读取

IWDG_Init();看门狗初始化

IWDG_Config(IWDG_Prescaler_64 ,625)设置看门狗溢出时间，括号内分别是（分频系数，累加寄存器溢出值），溢出时间(ms)=分频系数/40*累加寄存器溢出值

IWDG_Feed();喂狗

ee_ReadBytes(read_buf[], address , datalength);EE读，read_buf[]读取的数据，address起始地址，datalength数据字节长度

ee_WriteBytes(write_buf[], address , datalength);EE读，write_buf[]写数据，address起始地址，datalength数据字节长度

uint8_t ee_CheckOk()；EE初始化与通讯链路检查

PWR_EnterSTANDBYMode();MCU进入待机模式

Get_ChipID();获取芯片ID
