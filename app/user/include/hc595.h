/*
 * hc595.h
 *
 *  Created on: 24 сент. 2017 г.
 *      Author: Administrator
 */

#ifndef APP_USER_INCLUDE_HC595_H_
#define APP_USER_INCLUDE_HC595_H_

#define DISP_FLASH_ON_1		0b0001
#define DISP_FLASH_ON_2		0b0010
#define DISP_FLASH_ON_3		0b0100
#define DISP_FLASH_ON_4		0b1000
#define DISP_FLASH_ON_ALL	(DISP_FLASH_ON_1 | DISP_FLASH_ON_2 | DISP_FLASH_ON_3 | DISP_FLASH_ON_4) 	//Моргать всеми идикаторами
#define DISP_FLASH_OFF_ALL	(~DISP_FLASH_ON_ALL) 	//Выключить моргание всеми разрядами
#define DISP_FLASH_MASK	0b1111

void ICACHE_FLASH_ATTR InitDisplay(void);
extern uint8_t Digit[5];	//Дисплей
extern uint32_t DigitFlags;	//Флаги для индикатора - 31 - 4 - не используются, 3,2,1,0 - мигать разрядом

#define DispFlagSet(flag, mask) DigitFlags = ((DigitFlags & ~mask) | flag)


#endif /* APP_USER_INCLUDE_HC595_H_ */
