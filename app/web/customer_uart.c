/*
 * Прием и передача uart команд.
 *
 */
#include "os_type.h"
#include "osapi.h"
#include "bios.h"
#include "flash_eep.h"
#include "hw/uart_register.h"
#include "hw/esp8266.h"
#include "sdk/add_func.h"
#include "tcp2uart.h"

#include "../include/my_esp8266.h"
#include "customer_uart.h"

#define CMD_CLCK_PILOT1		0xaa		//Первый байт пилота команды
#define CMD_CLCK_PILOT2		0x55		//Второй байт пилота команды
#define CLK_CMD_PILOT_NUM	4			//Длина пилота для команды

os_timer_t UartLoadTimer;				//Периодический опрос буфера FIFO
os_timer_t UartRxParceTimer;			//Таймер запуска разбора полученной команды
os_timer_t UartRxTimeoutTimer;			//Таймер таймаута ожидания завершения команды

//Команда поступившая от часов
struct {
	uint8 Cmd;							//Код команды
	uint8 Data[CLK_MAX_LEN_CMD];		//данные
	uint8 CountData;					//Длина данных
	uint8 ClockCmdFlag;					//Текущее сосотояние автомата приема команды
} ClockCmdRx;

/*
 * Передать один байт по usart 0
 */
void uart0_tx(uint8 TxChar)
{
	do {
		MEMW();
		if(((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) <= 125) break;
	} while(1);
	UART0_FIFO = TxChar;
}

/*
 * Разбор команды от uart
 */
void ICACHE_FLASH_ATTR ParceClockCmdOut(void){
	uint8 i;

	if (ClockCmdRx.Cmd != CLK_CMD_EMPTY){
		if (ClkIsTest(ClockCmdRx.Cmd)){									//Тест соединения
			ClockUartTx(ClkTest(CLK_CMD_EMPTY), NULL, 0);				//Подтверждаем получение команды
		}
		else{
			for (i=0; i < (sizeof(ClockUartCmd)/sizeof(ClockUartCmd[0])); i++) {
				if ((ClockUartCmd[i].CmdCode == ClkCmdCode(ClockCmdRx.Cmd)) &&
						((ClockUartCmd[i].LenData)?ClockUartCmd[i].LenData:ClockCmdRx.CountData == ClockCmdRx.CountData)) //Если длина 0, то принимается любая длина
					{													//Команда найдена
					if (ClockUartCmd[i].Value != NULL){
						if (ClkIsWrite(ClockCmdRx.Cmd)){				//Команда записи
							os_memcpy((void*)ClockUartCmd[i].Value, (void*) &ClockCmdRx.Data, ClockCmdRx.CountData);
							ClockUartTx(ClkTest(CLK_CMD_EMPTY), NULL, 0);	//Подтверждаем получение команды
						}
						else if (ClkIsRead(ClockCmdRx.Cmd)){			//Команда чтения
							ClockUartTx(ClkWrite(ClockCmdRx.Cmd), (uint8*)ClockUartCmd[i].Value, ClockCmdRx.CountData);
						}
					}
					if (ClockUartCmd[i].Func != NULL){					//Есть функция обработчик
						if (ClockUartCmd[i].Func(ClockCmdRx.Cmd, ClockCmdRx.Data, ClockCmdRx.CountData)){
							ClockUartTx(ClkTest(CLK_CMD_EMPTY), NULL, 0);	//Если нужно подтверждаем получение команды
						}
					}
					break;
				}
			}
		}
		ClockCmdRx.Cmd = CLK_CMD_EMPTY;
		ClockCmdRx.ClockCmdFlag = 0;
	}
}

/*
 * Не дождались конца команды
 */
void ICACHE_FLASH_ATTR UartRxTimeout(void){
	if ((ClockCmdRx.Cmd == CLK_CMD_EMPTY) && (ClockCmdRx.ClockCmdFlag)){	//Завершающий код не принят и автомат стоит не в стартовой позиции
		ClockCmdRx.ClockCmdFlag = 0;		//Время истекло, возвращаемся в начальную позицию
#if (DEBUGSOO>1)
		os_printf("timeout parce\n");
#endif
	}
	os_timer_disarm(&UartRxTimeoutTimer);
}

/*
 * Проверить наличие данных в буфере приема uart
 */
void ICACHE_FLASH_ATTR ClockLoadUart0(void){
	uint8 RxByte, NumberFifo;

	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
	MEMW();
	UART0_INT_ENA &= ~ UART_RXFIFO_FULL_INT_ENA; // запретить прерывание по приему символа
	ets_intr_unlock(); // ETS_UART_INTR_ENABLE();

	os_timer_disarm(&UartLoadTimer);			//Пока отключить таймер загрузки

	MEMW();
	NumberFifo = ((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT); // кол-во уже принятых символов в rx fifo

	while (NumberFifo){
		if (ClockCmdRx.Cmd == CLK_CMD_EMPTY){ 							//Предыдущая команда отработана
			MEMW();
			RxByte = UART0_FIFO;										//Читаем принятый байт
			if (ClockCmdRx.ClockCmdFlag < CLK_CMD_PILOT_NUM){			//Пилот еще не принят
				if (RxByte == ((ClockCmdRx.ClockCmdFlag & 1)?CMD_CLCK_PILOT2:CMD_CLCK_PILOT1)) //Ждем соответсвующий байт пилота
					ClockCmdRx.ClockCmdFlag++;
				else
					ClockCmdRx.ClockCmdFlag = 0;
			}
			else{													//Пилот прнят, дальше идет команда и данные
				if (ClockCmdRx.ClockCmdFlag == CLK_CMD_PILOT_NUM){	//Длина данных
					ClockCmdRx.CountData = RxByte;
					os_timer_disarm(&UartRxTimeoutTimer);			//Запуск таймера таймаута
					os_timer_setfn(&UartRxTimeoutTimer, (os_timer_func_t *) UartRxTimeout, NULL);
					ets_timer_arm_new(&UartRxTimeoutTimer, CLK_TIMEOUT, OS_TIMER_SIMPLE, OS_TIMER_MS);
				}
				else {												//Данные
					if ((ClockCmdRx.ClockCmdFlag-CLK_CMD_PILOT_NUM) < (ClockCmdRx.CountData+1)){
						ClockCmdRx.Data[ClockCmdRx.ClockCmdFlag-(CLK_CMD_PILOT_NUM+1)] = RxByte;
					}
					else if ((ClockCmdRx.ClockCmdFlag-CLK_CMD_PILOT_NUM) == (ClockCmdRx.CountData+1)){//Код команды
						ClockCmdRx.Cmd = RxByte;
						ClockCmdRx.ClockCmdFlag = 0;
						os_timer_disarm(&UartRxTimeoutTimer);		//Таймер таймаута выключить
					}
				}
				ClockCmdRx.ClockCmdFlag++;
			}
			MEMW();
			NumberFifo = ((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT); // кол-во уже принятых символов в rx fifo
		}
		else														//Предыдущая команда еще не отработана, ждем
			break;
	}
//	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();						//Этот кусок кода включить когда будет использоватся переполнение буфера FIFO
//	SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA); // зарядить прерывание UART rx
//	ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
	os_timer_setfn(&UartLoadTimer, (os_timer_func_t *) ClockLoadUart0, NULL);
	ets_timer_arm_new(&UartLoadTimer, CLK_UART0_CHECK, OS_TIMER_SIMPLE, OS_TIMER_US);
}

/*
 * Инициализация UART интерфейса
 */
void ICACHE_FLASH_ATTR ClockUartInit(void){

	#define FLOW_CONTROL_OFF	0	//Не контролировать поток uart

	UART0_CONF0 = UART0_REGCONFIG0DEF;	//8N1, безо вских инверсий и прочей шняги
	UART0_CONF1 = ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S)			//Порог переполнения fifo, 1 байт, не используется, поскольку скорость достаточно маленькая
		| ((0x01 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S)				//Порог опустошения буфера fifo 1 байт
		| (((128 - 16) & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S) 	//Порог срабатывания для RTS = 16, но тут не используется
		| ((0x04 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) // | UART_RX_TOUT_EN	//Таймаут приема данных, //UART_RX_TOUT_EN - выключено
	;
	PERI_IO_SWAP &= ~PERI_IO_UART0_PIN_SWAP;											//Не менять ноги usart
	update_mux_uart0();

	uart_div_modify(UART0, UART_CLK_FREQ / CLK_BAUND);

	ets_intr_lock(); 					//	ETS_UART_INTR_DISABLE();
	MEMW();
	CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA);	//Отключить прерывание на буфер переполнения
	ets_intr_unlock(); 					// ETS_UART_INTR_ENABLE();

	SET_PERI_REG_MASK  (UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);	//Подготовить сброс FIFO
    CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);	//Сбросить FIFO

    uart0_set_flow(FLOW_CONTROL_OFF);											//Настроить управление потоком

	os_timer_disarm(&UartLoadTimer);
	os_timer_setfn(&UartLoadTimer, (os_timer_func_t *)ClockLoadUart0, NULL);
	ets_timer_arm_new(&UartLoadTimer, CLK_UART0_CHECK, OS_TIMER_SIMPLE, OS_TIMER_US);	//Старт таймера приема данных с rx

	os_timer_disarm(&UartRxParceTimer);										//Таймер разбора команды
	os_timer_setfn(&UartRxParceTimer, (os_timer_func_t *) ParceClockCmdOut, NULL);
	ets_timer_arm_new(&UartRxParceTimer, CLK_UART0_PARSE, OS_TIMER_REPEAT, OS_TIMER_MS);
#if DEBUGSOO > 0
	os_printf("My UART init OK\n");
#endif

}

/*
 * Передать команду на часы. Данные всегда передаются хотя бы в виде одного байта
 */
void ICACHE_FLASH_ATTR ClockUartTx(uint8 cmd, uint8 *data, uint8 dataLen){
	uart0_tx(CMD_CLCK_PILOT1);
	uart0_tx(CMD_CLCK_PILOT2);
	uart0_tx(CMD_CLCK_PILOT1);
	uart0_tx(CMD_CLCK_PILOT2);

	if (dataLen){
		uart0_tx(dataLen);							//Длина данных
		while(dataLen){
			uart0_tx(*data++);						//Данные
			dataLen--;
		}
	}
	else{											//Если данных нет
		uart0_tx(1);
		uart0_tx(0);
	}
	uart0_tx(cmd);									//Команда
}
