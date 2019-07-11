# stepcounter

stepcounter implemented by STM32f103

20190711更改点:

1. filter函数优化

2. SOC计算参数修改

3. 调整部分参数值


# 底层驱动备注

1、PA4纽扣电池电压采集；PA5锂离子电池电压采集；PA6力敏电阻电压VR采集，力敏阻值换算R=1000000*VR/(3.3-VR)；

2、uint8_t filter()用于VR的消抖滤波，防止噪声干扰；

3、PC0为LED1;PC2为LED2;

4、实时时钟由两个中断：秒中断与闹钟中断。秒中断预留给上层应用，闹钟中断用于将MCU从待机模式唤醒，每隔5秒唤醒一次，唤醒后立刻喂狗

并修改iwdg溢出时间；

5、PWR_EnterSTANDBYMode()为待机模式，进入待机前需要设置iwdg的溢出时间，溢出时间需要大于闹钟周期，暂时设为8s；待

机模式的触发条件需要上层提供，测试程序的触发条件设为每走两步触发一次；

6、RTC_Configuration()用于第一次烧写，该函数影响MCU初始化速度，之后烧写屏蔽该函数，后续烧写使用RTC_CheckAndConfig(&systmtime);

7、EEPROM为AT24C16，读写前必须运行函数ee_CheckOk()，否则驱动无法正常读写；

8、函数Get_ChipID()读取MCU的ID,每个MCU都有唯一的ID，上层程序把ID和用户账户进行关联。
