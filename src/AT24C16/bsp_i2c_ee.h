#ifndef __I2C_EE_H
#define	__I2C_EE_H


#include "stm32f10x.h"


/* 

 * 1 0 1 0 0  0  0  0 = 0XA0

 */

/* AT24C01/02每页有8个字节 
 * AT24C04/08A/16A每页有16个字节 
 */
	

#define EEPROM_DEV_ADDR			0xA0		/* 24xx16的设备地址 */
#define EEPROM_PAGE_SIZE		  8			  /* 24xx16的页面大小 */
#define EEPROM_SIZE				  256			  /* 24xx16总容量 */


uint8_t ee_CheckOk(void);
uint8_t ee_ReadBytes(uint8_t *_pReadBuf, uint8_t _usAddress, uint16_t _usSize);
uint8_t ee_WriteBytes(uint8_t *_pWriteBuf, uint8_t _usAddress, uint16_t _usSize);
uint8_t ee_Erase(void);

#endif /* __I2C_EE_H */
