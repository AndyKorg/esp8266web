/*
 * Прием, передача и обработка команд от uart (часов)
 * Биты 7 и 6 определяют направление данных.
 * Если бит 7 равнен 0 то это команда чтения. После получения команды получатель должен ответить командой записи.
 * Если бит 7 равнен 1 то это команда записи (или команда тестова - определяется битом 6).
 * Если значение бита 7 и бита 6 равны 1 то получатель интерпретирует это как команда теста.
 * После получения такой команды получтель должен ответить командой записи со значениями счетчиков.
 * Такой алгоритм используется для восстановления счетчикаов измененных на время передачи тестовой команды.
 * После передачи команды на тестирование любая другая команда прерывает тестирование.
 */

#ifndef __CUSTOMER_UART_H__
#define __CUSTOMER_UART_H__

#include "c_types.h"

#define CLK_BAUND		19200	//Скорость обмена с часами

#define CLK_CMD_EMPTY	0x00	//Специальная пустая команда, или буфер команды пуст
#define CLK_UART0_CHECK	32		//Периодичность проверки буфера приема uart мкс
#define CLK_UART0_PARSE	5		//Периодичность анализа команд поступающих от часов мс
#define CLK_TIMEOUT		1000	//Таймаут ожидания завершения ответа от часов мс

#define CLK_MAX_LEN_CMD	64		//Максимальная длина команды по uart TODO Переделать на динамический размер буфера

typedef uint8 (*VOID_CLK_UART_CMD)(uint8 cmd, uint8* pval, uint8 valLen);//Сама команда и её данные. Возвращает 1 если нужно подтвердить отбработку автоматически

typedef struct{					//Ппинятая команда UART
	uint8 CmdCode;
	void *Value;				//указатель на переменную в которую переписвается данные команды. Если есть то сначала выполняется она
	uint8 LenData;				//Ожидаемая длина данных, если не совпадает с принятым то команда не выполняется. Если 0 то не контролируетсяя
	VOID_CLK_UART_CMD Func;		//Если есть, то вызывается после присовения переменной указанной в Value. Функция должна возвращать не 0 если требуется подтверждение обработки команды
} pClockUartCmd;


// Команды для часов передаваемые через uart
#define CLK_CMD_TYPE	0b11000000				//Биты типа команды
#define CLK_CMD_WRITE	0b10000000				//Команда записи
#define CLK_CMD_TEST	0b11000000				//Команда тестирования
#define CLK_CMD_READ	0b00000000				//Команда чтения
#define ClkWrite(cmd)	((cmd & (~CLK_CMD_TYPE)) | CLK_CMD_WRITE)	//Запись в часы или модуль
#define ClkTest(cmd)	((cmd & (~CLK_CMD_TYPE)) | CLK_CMD_TEST)	//Команда тестирования
#define ClkRead(cmd)	(cmd & (~CLK_CMD_WRITE))//Прочитать из часов или модуля
#define ClkIsRead(cmd)	((cmd & CLK_CMD_TYPE) == CLK_CMD_READ)	//Команда чтения
#define ClkIsWrite(cmd)	((cmd & CLK_CMD_TYPE) == CLK_CMD_WRITE)	//Команда записи
#define ClkIsTest(cmd)	((cmd & CLK_CMD_TYPE) == CLK_CMD_TEST)	//Команда тестирования
#define ClkCmdCode(cmd)	(cmd & (~CLK_CMD_TYPE))	//Код команды без типа

#define CLK_NODATA		0						//Индикатор отсутствия данных в команде. Введно для удобства

#define UART_NUM_COUNT	9						//Количество команд и переенных для приема от внешнего модуля
extern pClockUartCmd ClockUartCmd[UART_NUM_COUNT];//Список команд uart обрабатываемых при приеме данных от часов

void ICACHE_FLASH_ATTR ClockUartInit(void);		//инициализация порта uart
void ICACHE_FLASH_ATTR ClockUartTx(uint8 cmd, uint8 *data, uint8 dataLen);//Передача команды на часы

#endif /* __CUSTOMER_UART_H__ */
