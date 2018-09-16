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
				i2c_AdrBuf,
				i2c_OutputBuf;	//Буфер передачи

#define GPIO_PIN_INTR_POSEDGE	1
#define GPIO_PIN_INTR_NEGEDGE	2
#define GPIO_PIN_INTR_ANYEDGE	3

#define	SCL_POSTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SCL, GPIO_PIN_INTR_POSEDGE);} while(0) // прерывание на 0->1
#define	SCL_NEGTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SCL, GPIO_PIN_INTR_NEGEDGE);} while(0) // прерывание на 1->0
#define	SCL_ANY_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SCL, GPIO_PIN_INTR_ANYEDGE);} while(0) // прерывание на 1->0

#define	SDA_POSTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SDA, GPIO_PIN_INTR_POSEDGE);} while(0) // прерывание на 0->1
#define	SDA_NEGTV_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SDA, GPIO_PIN_INTR_NEGEDGE);} while(0) // прерывание на 1->0
#define	SDA_ANY_EDGE	do{gpio_pin_intr_state_set(I2C_SLAVE_SDA, GPIO_PIN_INTR_ANYEDGE);} while(0) // прерывание на оба перепада

//Порт в режим чтения
#define SCL_READ_SET	do{GPIO_ENABLE_W1TC = (1 << I2C_SLAVE_SCL);} while(0)
//При переключении в режим чтения флаг разрешения прерывания сбрасывается
#define SDA_READ_SET	do{GPIO_ENABLE_W1TC = (1 << I2C_SLAVE_SDA);\
	    					SDA_ANY_EDGE;\
						} while(0)
//Вывод значения
#define	SDA_0_SET		do{	GPIOx_PIN(I2C_SLAVE_SDA) = GPIO_PIN_DRIVER; \
								GPIO_OUT_W1TC = (1 << I2C_SLAVE_SDA);\
								GPIO_ENABLE_W1TS = (1 << I2C_SLAVE_SDA);} while(0) //Прижать к земле
#define	SCL_0_SET		do{	GPIOx_PIN(I2C_SLAVE_SCL) = GPIO_PIN_DRIVER;\
								GPIO_OUT_W1TC = (1 << I2C_SLAVE_SCL);\
								GPIO_ENABLE_W1TS = (1 << I2C_SLAVE_SCL);} while(0)
//Проверка значений
#define SDA_IS_0		(!(GPIO_IN & (1<<I2C_SLAVE_SDA)))
#define SDA_IS_1		(GPIO_IN & (1<<I2C_SLAVE_SDA))
#define SCL_IS_0		(!(GPIO_IN & (1<<I2C_SLAVE_SCL)))
#define SCL_IS_1		(GPIO_IN & (1<<I2C_SLAVE_SCL))

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
#define	isStat(x)			(i2cState == x)
#define	SetStat(x)			do{i2cState = x;} while(0)

#define MASTER_WRITE		0		//Мастер записывает
#define MASTER_READ			1		//читает из slave

#define BIT_IN_BYTE			8		//начальное значение счетчика битов, вдруг изменят количетсво битов в байте

OW_CallbackFunct 			i2c_ReadOk,	//Вызывается когда принят пакет от мастера, в i2c_InputBuf принятые байты
							i2c_AdrOk,	//Принят адрес при операции Write - Repeat Start - Read or Write
							i2c_WriteOk;//Пердача буфера i2c_OutputBuf окончена

uint8 debugBuf[16];
uint8 debugLen;

ETSTimer debugTimer;

ETSTimer TimeOutTimer;

void i2c_int_handler(void);

void debugShow(void){
	uint8 i;
	if (i2c_BufIsFull(i2c_AdrBuf)){
		os_printf("Adr %x%x\n\r", i2c_AdrBuf.buf[0], i2c_AdrBuf.buf[1]);
		i2c_BufClear(i2c_AdrBuf);
		if (GPIO_IN & (1<<debugLed)){
			GPIO_OUTPUT_SET(debugLed, 0);
		}
		else{
			GPIO_OUTPUT_SET(debugLed, 1);
		}
	}
	else if (i2c_BufIsFull(i2c_InputBuf)){
		for (i=0;i<=i2c_InputBuf.len;){
			os_printf("%x ", i2c_InputBuf.buf[i++]);
		}
		GPIO_OUTPUT_SET(debugLed, ((i2c_InputBuf.buf[4] & 1)?1:0));
		i2c_BufClear(i2c_InputBuf);
		os_printf("\n\r");
	}
}

void InputOk(void){
	//uint8 i=0;

	if (i2c_BufIsFull(i2c_InputBuf)){
	/*		for (i=0;i<=i2c_InputBuf.len;i++){
				os_printf("cnt = %d val = %x ", i, i2c_InputBuf.buf[i]);
			}*/
			GPIO_OUTPUT_SET(debugLed, ((i2c_InputBuf.buf[0] & 1)?1:0));
			i2c_BufClear(i2c_InputBuf);
//			os_printf("\n\r");
		}
}

inline void ResetIIC(void){
	ets_intr_lock();
	SDA_READ_SET;
	SCL_READ_SET;
	set_gpiox_mux_func_ioport(I2C_SLAVE_SCL); // установить функцию GPIOx в режим порта i/o
	set_gpiox_mux_func_ioport(I2C_SLAVE_SDA); // установить функцию GPIOx в режим порта i/o
	ets_isr_attach(ETS_GPIO_INUM, i2c_int_handler, NULL);//Прилепить функцию на прерывание
	SCL_NEGTV_EDGE; // Режим срабатывания прерывания
    SDA_ANY_EDGE;
    // разрешить прерывания GPIOs
	GPIO_STATUS_W1TC = (1<<I2C_SLAVE_SCL) | (1<<I2C_SLAVE_SDA);

	i2c_BufClear(i2c_InputBuf);
	i2c_BufClear(i2c_AdrBuf);
	i2c_BufClear(i2c_OutputBuf);
	SetStat(STAT_START_WHITE);
	ets_intr_unlock();
}

void TimeOutFunc(void){
	os_printf("\r\nTIMEOUT!\r\n State = %x", i2cState);
	ResetIIC();
}

//Обработка прерываний от ног, не помещать во флэш!
void i2c_int_handler(void){
	uint8 static count =0, buf = 0, countByte = 0, direct = MASTER_WRITE;

	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;
/*
	uint32 static deb = 0;
	if (deb & 1){
		GPIO_OUTPUT_SET(debugPin2, 0);
	}
	else {
		GPIO_OUTPUT_SET(debugPin2, 1);
	}
	deb++;
*/
	if ((gpio_status & ((1<<I2C_SLAVE_SDA) | (1<<I2C_SLAVE_SDA)))
		&&
		!isStat(STAT_START_WHITE)
		){
		os_timer_disarm(&TimeOutTimer);
		os_timer_setfn(&TimeOutTimer, (os_timer_func_t *) TimeOutFunc, NULL);
		ets_timer_arm_new(&TimeOutTimer, 1000, 0, 1);
	}


	GPIO_OUTPUT_SET(debugPin, 0);
	GPIO_OUTPUT_SET(debugPin2, 0);

	if (gpio_status & (1<<I2C_SLAVE_SDA)){
		GPIO_OUTPUT_SET(debugPin, 1);
		if SCL_IS_1{ 										//Это что-то служебное START or STOP или повторный START
			if (isStat(STAT_START_WHITE) && SDA_IS_0 ){		//Первый START
				SetStat(STAT_START_FIXED);
				return;
			}
			else if (isStat(STAT_DATA) && SDA_IS_0){		//Повторный START, следовательно был принят адрес внутри чипа, пишем его в буфер внутреннего адреса
				if (i2c_BufIsEmpty(i2c_AdrBuf)){
					for(count=0;count<i2c_InputBuf.len;count++){
						i2c_AdrBuf.buf[count] = i2c_InputBuf.buf[count];
					}
					i2c_AdrBuf.len = i2c_InputBuf.len;
					i2c_BufClear(i2c_InputBuf);
					SetStat(STAT_START_FIXED);
					SCL_NEGTV_EDGE;
				}
				else{
					SetStat(STAT_START_WHITE);				//Вдруг адрес еще не обработан, тогда просто сбрасываем
					os_timer_disarm(&TimeOutTimer);			//Таймаут не нужен
				}
				return;
			}
			else if (isStat(STAT_STOP_FIXED) && SDA_IS_1){	//Фиксация стопа
				if (direct == MASTER_WRITE){
					if (i2c_ReadOk != NULL){
						i2c_ReadOk();
					}
				}
				direct = MASTER_WRITE;
				SetStat(STAT_START_WHITE);		//Ждем новый цикл
				ResetIIC();
				os_timer_disarm(&TimeOutTimer);	//Таймаут не нужен
				return;
			}
		}
	}//SDA Interrupt
	if (gpio_status & (1<<I2C_SLAVE_SCL)){
		GPIO_OUTPUT_SET(debugPin2, 1);
		if isStat(STAT_START_FIXED){
			SCL_POSTV_EDGE;						//Теперь Ловим фронты SCL
			SetStat(STAT_ADR_RECIV);
			count = BIT_IN_BYTE;
			buf = 0;
			return;
		}
		else if isStat(STAT_ADR_RECIV){ 		//Прием адреса
			buf = buf | (SDA_IS_1?1:0);
			count--;
			if (!count){						// Прием адреса закончен
				if ((buf & 0xfe)== (I2C_SLAVE_ADR & 0xfe)){	//Адрес верный
					SCL_NEGTV_EDGE;				//Ловим спады SCL
					direct = buf & 0x1;			//Направление передачи
					countByte = 0;
					if (
							((direct == MASTER_WRITE) && i2c_BufIsFull(i2c_InputBuf)) //Не успели освободить буфер приема, к приему не готовы
							||
							((direct == MASTER_READ) && i2c_BufIsFull(i2c_OutputBuf))//В буфере передачи ничего нет, не подтверждаем адрес
						)
					{
						SetStat(STAT_START_WHITE);
						return;
					}
					else{
						SetStat(STAT_ADR_ACK); 	//Все нормально подтверждаем адрес
						return;
					}
				}
				else{							//Не наш адрес, ничего не делаем, т.е. NACK
					SetStat(STAT_START_WHITE);
				}
				return;
			}
			else{
				buf <<= 1;
			}
			return;
		}
		else if isStat(STAT_ADR_ACK){ 			//Выставить подтверждение ACK
			SCL_NEGTV_EDGE;						//Ловим спады SCL
			SDA_0_SET;							//Сформировать ACK
			SetStat(STAT_ADR_ACK_ACK);
			return;
		}
		else if isStat(STAT_ADR_ACK_ACK){		//Есть подтверждение от мастера
/*			if SCL_IS_1{
				return;							//Какая то помеха, прожолжаем ждать нуля на SCL
			}*/
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
			if (i2c_InputBuf.len == I2C_SLAVE_SIZE){//некуда принимать больше, ждем СТОП
				SetStat(STAT_STOP_WHITE);		//Если адрес (внтуренний адрес, а не адрес чипа) 1 байт, то тут может быть повторный старт
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
				SCL_POSTV_EDGE;					//Байт в мастер отправили, ждем ответ мастера
				SDA_READ_SET;
				SetStat(STAT_ACK_MSTER_W);
				return;
			}
			buf = buf | (SDA_IS_1?1:0);			//Принимаем байт
			count--;
			if (!count){						//Байт обработан в режиме записи
				i2c_InputBuf.buf[countByte] = buf;
				i2c_InputBuf.len++;
				SetStat(STAT_ADR_ACK); 			//Ждем спада для выставления ACK
				SCL_NEGTV_EDGE;					//Ловим спады SCL
				return;
			}
			buf <<= 1;
			return;
		}
		else if isStat(STAT_ACK_MSTER_W){		//Ответ при чтении данных мастера получен
			if SDA_IS_1{						//Это NAK, дальше байт не шлем ждем STOP
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
			return;
		}
		else if isStat(STAT_STOP_WHITE){		//Начало STOP или начало повторного старта
			SetStat(STAT_STOP_FIXED);			//Ждем фиксации STOP
			return;
		}
	}//SCL Interrapt
}

//Принимает адрес и параметры callback-функции вызываемой при окончании сеанса приема
void i2c_slave_Init(void* func, void* param){

	i2c_ReadOk = InputOk;

	i2c_OutputBuf.buf[0] = 0x7e;
	i2c_OutputBuf.buf[1] = 0x55;
	i2c_OutputBuf.buf[2] = 0xaa;
	i2c_OutputBuf.buf[3] = 0x81;
	i2c_OutputBuf.buf[4] = 0x7e;
	i2c_OutputBuf.buf[5] = 0xff;

	ResetIIC();
	ets_isr_unmask(1 << ETS_GPIO_INUM);

	set_gpiox_mux_func_ioport(debugPin); // установить функцию GPIOx в режим порта i/o
	GPIO_ENABLE_W1TC = (1<<debugPin);
	set_gpiox_mux_func_ioport(debugPin2); // установить функцию GPIOx в режим порта i/o
	GPIO_ENABLE_W1TC = (1<<debugPin2);
	GPIO_OUTPUT_SET(debugPin, 0);
	GPIO_OUTPUT_SET(debugPin2, 0);

	set_gpiox_mux_func_ioport(debugLed);
	GPIO_ENABLE_W1TC = (1<<debugLed);

	os_printf("\r\nstart i2c\r\n");

	debugLen = 0;
/*	os_timer_disarm(&debugTimer);
	os_timer_setfn(&debugTimer, (os_timer_func_t *) debugShow, NULL);
	ets_timer_arm_new(&debugTimer, 100, 1, 0);*/

}
