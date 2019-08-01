#ifndef __COMMUNICATION_
#define __COMMUNICATION_

#include "stm32f10x.h"
#include "bsp_usart.h"

#define INVALID_TYPE 	 		0x01
#define ALREADY_BINED  		0x02
#define NOT_BINED 		 		0x03
#define CHECKSUM_ERROR 		0x04
#define INVALID_WEIGHT 		0x05
#define INVALID_HANG 	 		0x06

uint8_t COM_Checksum(uint32_t value);
void COM_Send_Deny(uint8_t error);
void COM_Send_Positive(void);
void COM_Listen_Reset(void);
void COM_Listening(void);
void COM_Response(void);

#endif
