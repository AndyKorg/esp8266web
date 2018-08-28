/*
 * i2c_slave.c
 *
 *  Created on: 19 авг. 2018 г.
 *      Author: Administrator
 */
#include "os_type.h"
#include "osapi.h"
#include "../include/bios/gpio.h"
#include "web_iohw.h"
#include "hw/esp8266.h"
#include "bios/ets.h"
#include "../include/sdk/add_func.h"
#include "include/i2c_slave.h"

#define debugPin	13
#define debugPin2	14
#define debugLed	12

struct ti2c_Buf	i2c_InputBuf,	//Буфер приема
				i2c_OutputBuf;	//Буфер передачи

#define SDA_READ_SET	do{GPIO_ENABLE_W1TC = 1 << I2C_SLAVE_SDA;} while(0)	//Порт в режим чтения
#define SCL_READ_SET	do{GPIO_ENABLE_W1TC = 1 << I2C_SLAVE_SCL;} while(0)
#define	SDA_0_SET		do{	GPIOx_PIN(I2C_SLAVE_SDA) = GPIO_PIN_DRIVER; \
								GPIO_OUT_W1TC = (1 << I2C_SLAVE_SDA);\
								GPIO_ENABLE_W1TS = 1 << I2C_SLAVE_SDA;} while(0) //Прижать к земле
#define	SCL_0_SET		do{	GPIOx_PIN(I2C_SLAVE_SCL) = GPIO_PIN_DRIVER;\
								GPIO_OUT_W1TC = (1 << I2C_SLAVE_SCL);\
								GPIO_ENABLE_W1TS = 1 << I2C_SLAVE_SCL;} while(0)
#define SDA_IS_0		(!(GPIO_IN & (1<<I2C_SLAVE_SDA)))
#define SCL_IS_0		(!(GPIO_IN & (1<<I2C_SLAVE_SCL)))
#define SDA_IS_1		(GPIO_IN & (1<<I2C_SLAVE_SDA))
#define SCL_IS_1		(GPIO_IN & (1<<I2C_SLAVE_SCL))

#define GPIO_PIN_INTR_POSEDGE	1
#define GPIO_PIN_INTR_NEGEDGE	2
#define GPIO_PIN_INTR_ANYEDGE	3

#define	SCL_POSTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SCL, GPIO_PIN_INTR_POSEDGE);} while(0) // прерывание на 0->1
#define	SCL_NEGTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SCL, GPIO_PIN_INTR_NEGEDGE);} while(0) // прерывание на 1->0
//#define	SCL_NEGTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SCL, GPIO_PIN_INTR_ANYEDGE);} while(0) // прерывание на 1->0

#define	SDA_POSTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SDA, GPIO_PIN_INTR_POSEDGE);} while(0) // прерывание на 0->1
#define	SDA_NEGTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SDA, GPIO_PIN_INTR_NEGEDGE);} while(0) // прерывание на 1->0
#define	SDA_ANY_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SDA, GPIO_PIN_INTR_ANYEDGE);} while(0) // прерывание на оба перепада

#define STAT_START_WHITE	0		//Ожидается старт
#define STAT_START_FIXED	1		//Ждем фиксации старта
#define STAT_ADR_RECIV		2		//Прием адреса
#define STAT_ADR_ACK		3		//Подтверждение приема адреса, ждем опускания SCL
#define STAT_ADR_ACK_ACK	4		//Подтверждение приема ACK от мастера
#define STAT_DATA			5		//Прием данных
#define	STAT_STOP_WHITE		6		//Ждем начала СТОП
#define STAT_ACK_MSTER_W	7		//Ожидание ответа от мастер при приеме мастером данных
#define	STAT_STOP_FIXED		8		//STOP зафиксирован


uint8 volatile i2cState = STAT_START_WHITE;
#define	isStat(x)		(i2cState == x)
#define	SetStat(x)		do{i2cState = x;} while(0)

#define MASTER_WRITE		0		//Мастер записывает
#define MASTER_READ			1		//читает из slave

#define BIT_IN_BYTE			8		//начальное значение счетчика битов, вдруг изменят количетсов

uint8 debugBuf[16];
uint8 debugLen;

ETSTimer debugTimer;

ETSTimer TimeOutTimer;

void debugShow(void){
//	uint8 i;
	uint32 Res;
	if (debugLen){
/*		os_printf("\r\n\r\nDEBUG = ");
			for(i=0; i<debugLen;i++){
				os_printf("%x", debugBuf[i]);
			}
		os_printf("\r\n");*/
		debugLen = 0;
		}
	if (i2c_InputBuf.ready == I2C_BUF_FULL){
		Res = (uint32) i2c_InputBuf.buf[4];
		Res = Res | (((uint32)  i2c_InputBuf.buf[3])<<8);
		Res = Res | (((uint32)  i2c_InputBuf.buf[2])<<16);
		Res = Res | (((uint32)  i2c_InputBuf.buf[1])<<24);
		GPIO_OUTPUT_SET(debugLed, ((Res & 1)?1:0));
		i2c_InputBuf.ready = I2C_BUF_EMPTY;
	}
}

void TimeOutFunc(void){
	ets_intr_lock();
	SDA_READ_SET;
	SCL_READ_SET;
    SCL_NEGTV_EDGE; // Режим срабатывания прерывания
    SDA_NEGTV_EDGE;
	SetStat(STAT_START_WHITE);
	ets_intr_unlock();

	GPIO_OUTPUT_SET(debugPin, 0);
	os_printf("\r\nTIMEOUT!\r\n State = %s", i2cState);
}

//Обработка прерываний от ног, не помещать во флэш!
void i2c_int_handler(void){
	uint8 static count =0, buf = 0, countByte = 0, direct = MASTER_WRITE;

	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;

	if ((gpio_status & ((1<<I2C_SLAVE_SDA) | (1<<I2C_SLAVE_SDA)))
		&&
		!isStat(STAT_START_WHITE)
		){
		os_timer_disarm(&TimeOutTimer);
		os_timer_setfn(&TimeOutTimer, (os_timer_func_t *) TimeOutFunc, NULL);
		ets_timer_arm_new(&TimeOutTimer, 1000, 0, 1);
	}

	if (gpio_status & (1<<I2C_SLAVE_SDA)){
		if SCL_IS_1{ 							//Это что-то служебное START or STOP
			if isStat(STAT_START_WHITE){
				SetStat(STAT_START_FIXED);
				return;
			}
			else if isStat(STAT_STOP_FIXED){	//Фиксация стопа
				SetStat(STAT_START_WHITE);		//Ждем новый цикл
				i2c_InputBuf.ready = I2C_BUF_FULL;
				SCL_NEGTV_EDGE;
				SDA_NEGTV_EDGE;
				os_timer_disarm(&TimeOutTimer);	//Таймаут не нужен
				return;
			}
		}
	}//SDA Interrupt
	if (gpio_status & (1<<I2C_SLAVE_SCL)){
		if isStat(STAT_START_FIXED){
			SetStat(STAT_START_WHITE);
			if SDA_IS_0{						//Подтверждение START
				SCL_POSTV_EDGE;					//Теперь Ловим фронты SCL
				SetStat(STAT_ADR_RECIV);
				count = BIT_IN_BYTE;
				buf = 0;
			}
			return;
		}
		else if isStat(STAT_ADR_RECIV){ 		//Прием адреса
			buf = buf | (SDA_IS_1?1:0);
			count--;
			if (!count){						// Прием адреса закончен
				if ((buf & 0xfe)== (I2C_SLAVE_ADR & 0xfe)){	//Адрес верный
					SetStat(STAT_ADR_ACK); 		//Ждем спада для выставления ACK
					direct = buf & 0x1;			//Направление передачи
					countByte = 0;
				}
				else{							//Не наш адрес, ничего не делаем, т.е. NACK
					SetStat(STAT_START_WHITE);
				}
				SCL_NEGTV_EDGE;					//Ловим спады SCL
				return;
			}
			else{
				buf <<= 1;
			}
		}
		else if isStat(STAT_ADR_ACK){ 			//Выставить подтверждение адреса ACK
			SDA_0_SET;							//Сформировать ACK
			SetStat(STAT_ADR_ACK_ACK);
		}
		else if isStat(STAT_ADR_ACK_ACK){		//Есть подтверждение от мастера
			SDA_READ_SET;
			SetStat(STAT_DATA);					//Начать прием или передачу данных
			count = BIT_IN_BYTE;
			buf = 0;
			if (direct == MASTER_READ){			//Читаем из нас, сразу ставим первый бит
				buf = i2c_OutputBuf.buf[countByte];
				SCL_NEGTV_EDGE;					//Ловим спады если читают из нас
				count--;
				if (!(buf & (1<<count))){
					SDA_0_SET;
				}
				return;
			}
			SCL_POSTV_EDGE;						//Ловим подъемы SCL если пишут в нас
			if ((countByte == I2C_SLAVE_SIZE) || (i2c_InputBuf.ready == I2C_BUF_FULL)){//некуда принимать больше или еще не обработан предыдущий прием, ждем СТОП
				SDA_POSTV_EDGE;
				SetStat(STAT_STOP_WHITE);
			}
			return;
		}
		else if isStat(STAT_DATA){				//Обработка данных
			if (direct == MASTER_READ){			//Отдаем байт
				if (count){						//Есть что отдавать, отдаем
					if (!(buf & (1<<(count-1)))){
						SDA_0_SET;
					}
					else{
						SDA_READ_SET;
					}
					count--;
					return;
				}
				SCL_POSTV_EDGE;				//Байт в мастер отаправили, ждем ответ мастера
				SDA_READ_SET;
				SetStat(STAT_ACK_MSTER_W);
				return;
			}
			buf = buf | (SDA_IS_1?1:0);			//Принимаем байт
			count--;
			if (!count){						//Байт обработан в режиме записи
				i2c_InputBuf.buf[countByte] = buf;
				countByte++;
				SetStat(STAT_ADR_ACK); 			//Ждем спада для выставления ACK
				SCL_NEGTV_EDGE;					//Ловим спады SCL
				return;
			}
			buf <<= 1;
			return;
		}
		else if isStat(STAT_ACK_MSTER_W){		//Ответ при чтении данных мастера получен
			if SDA_IS_1{						//Это NAK, дальше байт не шлем ждем STOP
				SDA_POSTV_EDGE;
				SCL_POSTV_EDGE;
				SetStat(STAT_STOP_WHITE);
				return;
			}
			countByte++;
			if (countByte == I2C_SLAVE_SIZE){	//Весь выходной буфер прочитан, если дальше читать, то отдаем сначала
				countByte = 0;
			}
			buf = i2c_OutputBuf.buf[countByte];
			count = BIT_IN_BYTE;
			SCL_NEGTV_EDGE;						//Подождем когда мастер готов будет читать байт после своего ответа
			SetStat(STAT_DATA);
		}
		else if isStat(STAT_STOP_WHITE){		//Начало STOP
			SetStat(STAT_STOP_FIXED);			//Ждем фиксации STOP
		}
	}//SCL Interrapt
}

//Принимает адрес и параметры callback-функции вызываемой при окончании сеанса приема
void i2c_slave_Init(void* func, void* param){

	i2c_InputBuf.ready = I2C_BUF_EMPTY;
	i2c_OutputBuf.ready = I2C_BUF_EMPTY;

	i2c_OutputBuf.buf[0] = 0x7e;
	i2c_OutputBuf.buf[1] = 0xaa;
	i2c_OutputBuf.buf[2] = 0x55;
	i2c_OutputBuf.buf[3] = 0x81;
	i2c_OutputBuf.buf[4] = 0x7e;
	i2c_OutputBuf.buf[5] = 0xff;


	SCL_READ_SET;
	SDA_READ_SET;
	set_gpiox_mux_func_ioport(I2C_SLAVE_SCL); // установить функцию GPIOx в режим порта i/o
	set_gpiox_mux_func_ioport(I2C_SLAVE_SDA); // установить функцию GPIOx в режим порта i/o
//	GPIO_ENABLE_W1TC = pins_mask; // GPIO OUTPUT DISABLE отключить вывод в портах
	ets_isr_attach(ETS_GPIO_INUM, i2c_int_handler, NULL);//Прилепить функцию на прерывание
    SCL_NEGTV_EDGE; // Режим срабатывания прерывания
    SDA_NEGTV_EDGE;
    SetStat(STAT_START_WHITE);
    // разрешить прерывания GPIOs
	GPIO_STATUS_W1TC = (1<<I2C_SLAVE_SCL) | (1<<I2C_SLAVE_SDA);
	ets_isr_unmask(1 << ETS_GPIO_INUM);

	set_gpiox_mux_func_ioport(debugPin); // установить функцию GPIOx в режим порта i/o
	GPIO_ENABLE_W1TC = (1<<debugPin);
	set_gpiox_mux_func_ioport(debugPin2); // установить функцию GPIOx в режим порта i/o
	GPIO_ENABLE_W1TC = (1<<debugPin2);
	GPIO_OUTPUT_SET(debugPin, 1);

	set_gpiox_mux_func_ioport(debugLed);
	GPIO_ENABLE_W1TC = (1<<debugLed);

	os_printf("\r\nstart i2c\r\n");

	debugLen = 0;
	os_timer_disarm(&debugTimer);
	os_timer_setfn(&debugTimer, (os_timer_func_t *) debugShow, NULL);
	ets_timer_arm_new(&debugTimer, 100, 1, 0);

}
