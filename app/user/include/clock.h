/*
 * Переменные и процедуры обслуживания часов
 *
 */

#ifndef APP_USER_INCLUDE_CLOCK_H_
#define APP_USER_INCLUDE_CLOCK_H_

#include "c_types.h"
#include "clock_web.h"

//Определения общие для часов и для esp
#define u08	unsigned char
#define u16	uint16
#define u32	uint32
#define s08	sint8

#define CLOCK_NAME_MAX		32						//Максимальная длина строки с именем часов
#define TEXT_CUST_MAX		96						//Маскимальная длина произвольной строки

#define ALARM_MAX			9						//Количество будильников
#define ALARM_EVRY_WEEK_NUM	4						//Количество будильников в еженедельном режиме, остальные по датам
#define ALARM_DURAT_MASK	0b01111111				//Биты длительности сигнала будильника
#define ALARM_DURATION_MAX	31						//Максимальное количество минут срабатывания будильника
#define FONT_NUM_MAX		6						//Максимальное количество шрифтов

struct sClockValue{									//Значения времени для часов и будильников. В BCD-формате.
	u08	Second;										//Текущее время обновляется с частотой REFRESH_CLOCK.
	u08	Minute;
	u08	Hour;
	//u08	Day;										//День недели в импортном формате, 1 - воскресенье, 2 - понедельник и т.д.
	u08	Date;										//Дата месяца
	u08	Month;
	u08	Year;										//От 0 до 99, всегда 21 век.
	u08	Mode;										//Режим работы - см. ниже
};

struct sAlarm{
	u08 Id;											//Номер будильника
	u08 EnableDayRus;								//Разрешение срабатывания по дням недели.Биты: 7 - включение-выключение будильника, 0-вс.1-пн,2-вт,3-ср,4-чт,5-пт,6-сб ВНИМАНИЕ! В микросхеме RTC 1-вс, 2-пн, и т.д.!
	u08 Duration;									//Длительность звучания будильника в секундах. от 1 до 99. Причем старший бит всегда равен 0, т.к. он используется как флаг паузы в CurrentDuration
	u08 CurrentDuration;							//Оставшаяся длительность звучания после срабатывания будильника. Старший бит используется как флаг установки звука будильника пользователем на паузу. Установка на паузу не начинает отсчет длительности звучания сначала.
	struct sClockValue Clock;						//Время срабатывания
};

#define ALARM_ON_BIT		0						//Бит EnableDayRus отвечающий за включение-выключение будильника

#define VOL_TYPE_COUNT		4						//Количество поддерживаемых типов регулирования громкости
#define VOLUME_LEVEL_MAX	64						//Максимальное значение громкости выше которого регулирование громкости не заметно

#define SENSOR_MAX			3						//Количество поддерживаемых сенсоров
#define SENSOR_LEN_NAME		3						//Длина имени датчика

//------------------------------- Биты байта State в структуре sSensor, должно быть аналогично файлу ...\ClockMatrix\sensors.h в прошивке часов на avr
// TODO: Объединить описание структуры байта статуса в один совместный файл с прошивкой датчика
													//Биты от 7 до 4 поступают от датчика
#define	SENSOR_NO_SENSOR	7						//Датчик не обнаружен 1 - нет датчика на шине (у радиодатчика это означает, что ответ от радиодатчика есть, но внутри него сенсор не ответил)
#define	SENSOR_LOW_POWER	6						//Низкое напряжение батарейки 1 - слишком низкое напряжение на батарее
#define	SENSOR_NO_RF		5						//Имеет смысл только для радиодатчика. 1 - радиодатчик не прислал вовремя пакет ответа
#define	SENSOR_PRESSURE		4						//Тип датчика: 1-датчик давления
													//Биты от 3 до 0 обрабатываются самими часами
#define SENSOR_SHOW_ON		3						//Разрешен вывод значения датчика на дисплей
#define SENSOR_RESERV_CLK1	2						//-------- Зарезервировано для будущего использования
#define SENSOR_RESERV_CLK2	1
#define SENSOR_RESERV_CLK3	0

#define SensorShowOff(st)	st.State &= ~(1<<SENSOR_SHOW_ON) 				//Выключить вывод значения датчика на дисплей
#define SensorShowOn(st)	st.State |= (1<<SENSOR_SHOW_ON)					//Включить
#define SensorSetInBus(st)	st.State &= ~(1<<SENSOR_NO_SENSOR)				//Датчик на шине есть. Для радиодатчика это означает что есть ответ от радиопередатчика, но может нет ответа от самого датчика. Наличи датчика в радиодатчике проверяется через значение Value (см. флаги TMPR_NO_SENS...)
#define SensorNoInBus(st)	st.State |= (1<<SENSOR_NO_SENSOR)				//Пометить отсутствие датчика на шине
#define SensorFlgClr(st)	(st.State & (~SENSOR_STATE_MASK))				//Сброс флагов самого датчика
#define SensorIsNo(st)		((st.State & (1<<(SENSOR_NO_SENSOR))) != 0)		//Датчик отсутствует на шине?
#define SensorIsSet(st)		((st.State & (1<<(SENSOR_NO_SENSOR))) == 0)		//Датчик отвечает на шине?
#define	SensorBatareyIsLow(st)	((st.State & (1<<(SENSOR_LOW_POWER))) != 0) //Батарея разряжена
#define SensotTypeTemp(st)	st.State &= ~(1<<SENSOR_PRESSURE)				//Это датчик температуры
#define SensorTypePress(st)	st.State |= (1<<SENSOR_PRESSURE)				//Это датчик давления
#define SensorIsPress(st)	((st.State & (1<<(SENSOR_PRESSURE))) != 0) 		//Это датчик давления?
#define SensorRFInBus(st)	st.State &= ~(1<<SENSOR_NO_RF)					//Радиодатчик вовремя не ответил
#define SensorRFNoBus(st)	st.State |= (1<<SENSOR_NO_RF)
#define SensorRFIsNo(st)	((st.State & (1<<(SENSOR_NO_RF))) != 0)

//Команды для функции отображения значения датчиков. Значения выбраны исходя из того что отрицательные температуры выводятся в дополнительном коде от 0xc9 (-55 C) до 0xff (-1 C)
#define SENS_NO_SENS		"NS"					//Текст для вывода в веб-морду для состояния TMPR_NO_SENS
#define SENS_BAT_LOW		"BL"					//Текст для вывода в веб-морду для состояния TMPR_BAT_LOW
#define SENS_NO_RADIO		"NR"					//Текст для вывода в веб-морду для состояния TMPR_UNDEF

struct sSensor{										//Датчик температуры, давления и т.п. В eeprom сохраняются только Adr, State, Name
	u08 Adr;										//Адрес датчика. Значимыми являются биты 1,2,3. Бит 0 всегда равен 0 и фактический адрес сдвинут на 1 влево. Это сделано для того что бы различать сигнал от датчика и от ИК-приемника
	u08 Value;										//Значение измеряемой величины, возможно нужно использовать u16
	u08 State;										//Состояние датчика. Тестирование, батарея разряжена и пр. Причем старшие 4 бита определяются внешним датчиком, а младшие 4 вычисляются внутри программы часов
	u08 SleepPeriod;								//Счетчик обратного отсчета периода в течении которого не поступал сигнал от датчика. В минутах
	char Name[SENSOR_LEN_NAME];						//Имя датчика для вывода на дисплей
};

//Для датчика давления значения сдвинуты на PressureBase
#define PressureBase	641
#define PressureNormal(shortPress)	((u16)(PressureBase+shortPress))

struct sNtpPeriod{									//Периодичность получения времени из Интернета
	uint8 Hour;										//Номер часа
	uint8 PeriodType;								//Тип периода
	uint8 Value;									//Значение периода. Для разных типов имеет разную интерпритацию
};

extern struct sNtpPeriod NtpPeriod;

//Макросы вычислений. Все BCD в виде байта! За исключением Yea в IsLeapYear(Yea)
#define BCDtoInt(bcd)	((((bcd & 0xf0) >> 4)*10) + (bcd & 0xf))				//BCD в Int без знака
#define AddDecBCD(Mc)	(((Mc & 0xf0)+0x10) & 0xf0)								//Увеличение десятков в формате BCD
#define AddOneBCD(bcd)	(((bcd & 0x0f) == 9)? AddDecBCD(bcd) : (bcd+1))			//Увеличить bcd на 1.
#define DecDecBCD(Mc)	(((Mc & 0xf0) == 0)?Mc : (((Mc & 0xf0)-0x10) | 0x9))	//уменьшение десятков в формате BCD
#define DecOneBCD(bcd)	(((bcd & 0x0f) == 0)? DecDecBCD(bcd) : (bcd-1))			//Уменьшить bcd на 1.
#define IsLeapYear(Yea) ((Yea % 4) == 0)										//Високосный год. Кратность 100 и 400 не проверяется поскольку часы работают только в 21 веке
#define LastDayFeb(Ye)	(IsLeapYear((int)(BCDtoInt(Ye)+2000))? 0x29 : 0x28)		//Количество дней в феврале заданного года
#define LastDayMonth(Mo, Ye) ((Mo == 0x2)? LastDayFeb(Ye) : (((Mo == 0x4) || (Mo == 0x6) || (Mo == 0x9) || (Mo == 0x11))?0x30:0x31))  //Последний день месяца

#define Tens(bcd)		((bcd >> 4) & 0xf)										//Взять цифру десятков в bcd в виде числа
#define	Unit(bcd)		(bcd & 0xf)												//Взять цифру едениц

#define SEC_IN_MIN		60														//Секунд в минуте
#define SEC_IN_HOUR		3600													//Секунд в часе
#define YEAR_LIMIT		0x13													//Год ограничивающий границу рассчетов дат в переводе секунд в dateime, в формате BCD
#define DAYS_01_01_2013	41272UL													//Точное количество дней прошедших с 01.01.1900 по 01.01.2013
#define SEC_IN_DAY		86400UL													//Секунд в сутках

extern struct sClockValue Watch;												//Часы
extern struct sAlarm Alarms[ALARM_MAX];											//Будильники
extern sint8 ClockZone;															//Часовой пояс. Используется int т.к. лень делать преобразования
extern uint8 ClockName[CLOCK_NAME_MAX];											//Имя часов для html-страницы
extern uint8 TextCustom[TEXT_CUST_MAX];											//Произвольный текст выводимый на часы
extern uint8 FontNum;															//Номер шрифта
extern struct sSensor Sensors[SENSOR_MAX];										//Массив сенсоров
extern struct espVolume VolumeClock[VOL_TYPE_COUNT];							//Громкости
extern uint8 HZSpeed;															//Скорость горизонтальной прокрутки

u08 ICACHE_FLASH_ATTR SecundToDateTime(u32 Secunds,								//Перевод количества секунд в дату и время с учетом пояса GMT
			struct sClockValue *Value, s08 HourOffset);
u08 ICACHE_FLASH_ATTR UnixTimeToDateTime(u32 Secunds,							//UNIX время в человекочитаемую дату
		struct sClockValue *Value, s08 HourOffset);
void ICACHE_FLASH_ATTR itoa(int n, char s[3]);									//Конвертация целого в строку
uint8 strtodate(uint8 *pvar, struct sClockValue* Value);						//Конвертирует строку в дату
u08 what_day(const u08 date, const u08 month, const u08 year);					//Рассчитывает день по дате
void nextDate(struct sClockValue *value);										//Устанавливает следующий день
u32 ICACHE_FLASH_ATTR bin2bcd_u32(u32 data, u08 result_bytes);
void UTF8toWin1251Cyr(char *str);												//Коневертор строк из UTF-8 в win-1251


#endif /* APP_USER_INCLUDE_CLOCK_H_ */
