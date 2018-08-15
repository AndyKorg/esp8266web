/*
 * hc595.c
 *
 *  Created on: 24 сент. 2017 г.
 *      Author: Administrator
 */
#include <stdlib.h>
#include "os_type.h"
#include "hw/esp8266.h"
#include "web_iohw.h"
#include "sdk/add_func.h"
#include "osapi.h"
#include "../include/sdk/rom2ram.h"

#include "../include/my_esp8266.h"

#define DISP_SPI_MOSI 		5						//Display SPI MOSI
#define DISP_SPI_CLK		4						//Display SPI CLK
#define DISP_SPI_UPD		13						//Display SPI update output pins

#define DispClckLow()		GPIO_OUTPUT_SET(DISP_SPI_CLK, 0)
#define DispClckHight()		GPIO_OUTPUT_SET(DISP_SPI_CLK, 1)

#define DispUpdLow()		GPIO_OUTPUT_SET(DISP_SPI_UPD, 0)
#define DispUpdHight()		GPIO_OUTPUT_SET(DISP_SPI_UPD, 1)

#define DISP_REFRESH		500 					//Display refersh period, us
#define DISP_FLASH_PERIOD	500						//Flash digit period, ms

os_timer_t refreshDisp_timer, flashTimer;

//Сегменты индикатора
#define sA 	0b10000000	// A
#define sB 	0b00000010 	// B
#define sC 	0b00000001 	// C
#define sD 	0b00000100	// D
#define sE 	0b00010000	// E
#define sF 	0b00100000	// F
#define sG 	0b01000000	// G
#define sDP 0b00001000	// DP

//Знакогенератор
uint8_t Font[] = {		// Символ
		 0b10110111 	// 0
		,0b00000011 	// 1
		,0b11010110		// 2
		,0b11000111		// 3
		,0b01100011		// 4
		,0b11100101		// 5
		,0b11110101		// 6
		,0b10000011 	// 7
		,0b11110111 	// 8
		,0b11100111 	// 9
		,0b11110100 	// E A
		,0b01010000 	// r B
		,0b00000000 	// пока пусто C
		,0b00000000 	// пока пусто D
		,0b00000000 	// пока пусто E
		,0b00000000 	// пока пусто F
};

uint8_t Digit[5] = "0000";	//Дисплей
uint32_t DigitFlags = 0;	//Флаги для индикатора - 31 - 4 - не используются, 3,2,1,0 - мигать разрядом
uint8_t flashPeriod = 0;

void ICACHE_FLASH_ATTR out_byte_disp(uint16_t Value){
	uint16_t i;

	for (i=0; i<16; i++){
		if (Value & (1<<i))
			GPIO_OUTPUT_SET(DISP_SPI_MOSI, 1);
		else
			GPIO_OUTPUT_SET(DISP_SPI_MOSI, 0);
		DispClckLow();
		DispClckHight();
		DispClckLow();
	}
}

//Вывод одного разряда
void ICACHE_FLASH_ATTR refreshDisplay(void)
{
	static uint16_t digit = 0;

	DispUpdHight();
	if ((DigitFlags & (1<<digit)) && flashPeriod ){ //Мигание
		out_byte_disp( ((1<<digit)<<8) & 0xff00);
	}
	else{
		out_byte_disp((Font[Digit[3-digit] & 0x0f]  & 0x00ff) | (((1<<digit)<<8) & 0xff00));
	}
	digit++;
	if (digit >3 ){
		digit =  0;
	}
	DispUpdLow();
}

//Флаг мигания
void ICACHE_FLASH_ATTR flashDisplay(void)
{
	flashPeriod ^= 1;
}

void ICACHE_FLASH_ATTR InitDisplay(void)
{
	set_gpiox_mux_func_ioport(DISP_SPI_MOSI);
	set_gpiox_mux_func_ioport(DISP_SPI_CLK);
	set_gpiox_mux_func_ioport(DISP_SPI_UPD);

	GPIO_ENABLE_W1TC = 1 << DISP_SPI_MOSI;
	GPIO_ENABLE_W1TC = 1 << DISP_SPI_CLK;
	GPIO_ENABLE_W1TC = 1 << DISP_SPI_UPD;

	DispClckLow();
	DispUpdLow();

	os_timer_disarm(&refreshDisp_timer);
	os_timer_setfn(&refreshDisp_timer, (os_timer_func_t *) refreshDisplay, NULL);
	ets_timer_arm_new(&refreshDisp_timer, DISP_REFRESH, OS_TIMER_REPEAT, OS_TIMER_US);

	os_timer_disarm(&flashTimer);
	os_timer_setfn(&flashTimer, (os_timer_func_t *) flashDisplay, NULL);
	ets_timer_arm_new(&flashTimer, DISP_FLASH_PERIOD, OS_TIMER_REPEAT, OS_TIMER_MS);
}
