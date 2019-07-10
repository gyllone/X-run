#ifndef __I2C_EE_H
#define	__I2C_EE_H


#include "stm32f10x.h"


/* 

 * 1 0 1 0 0  0  0  0 = 0XA0

 */

/* AT24C01/02ÿҳ��8���ֽ� 
 * AT24C04/08A/16Aÿҳ��16���ֽ� 
 */
	

#define EEPROM_DEV_ADDR			0xA0		/* 24xx16���豸��ַ */
#define EEPROM_PAGE_SIZE		  16			  /* 24xx16��ҳ���С */
#define EEPROM_SIZE				  2048			  /* 24xx16������ */


uint8_t ee_CheckOk(void);
uint8_t ee_ReadBytes(uint8_t *_pReadBuf, uint16_t _usAddress, uint16_t _usSize);
uint8_t ee_WriteBytes(uint8_t *_pWriteBuf, uint16_t _usAddress, uint16_t _usSize);
uint8_t ee_Erase(void);

#endif /* __I2C_EE_H */
