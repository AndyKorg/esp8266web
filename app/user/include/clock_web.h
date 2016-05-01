/*
 * clock_web.h
 *
 *  Created on: 20 марта 2015 г.
 *      Author: Admin
 * сами команды управления часами по uart
 */

#ifndef APP_USER_INCLUDE_CLOCK_WEB_H_
#define APP_USER_INCLUDE_CLOCK_WEB_H_

//Команды UART. Код 0 зарезервирован за пустой командой
#define CLK_WATCH		0x01	//Обращение к счетчику часов
#define CLK_ALARM		0x02	//Обращение к будильнику
#define CLK_NTP_START	0x03	//Запустить получение времени от ntp сервера
#define CLK_VOLUME		0x05	//Обращение к переменной громкости
#define CLK_SENS		0x06	//Обращение к датчику
#define CLK_FONT		0x07	//Обращение к шрифту
#define CLK_STOP		0x08	//Остановить операцию (тестирование или ожидание)
#define CLK_ALL			0x09	//Прочитать все настройки заново
#define CLK_DEBUG		0x10	//Отладочные данные
#define CLK_ST_WIFI_AUTH 0x11	//Авторизация для station имя сети и пароль
#define CLK_ST_WIFI_IP	0x12	//Полученный или присвоенный IP station
#define CLK_CUSTOM_TXT	0x13	//Текст выводимый перед датой. Обрабатывается специальным образом!
#define CLK_HZ_SPEED	0x14	//Скорость бегущей строки

//Ответы
#define CLK_NTP_ERROR	0x70	//Ошибка обращения к серверу ntp

//Идетификаторы переменных для записи во flash, остальные смотри в flash_eep.h
#define ID_CLOCK_ZONE 	0xddd1	//Идетификатор переменной временной зоны во flash
#define ID_NTP_IP 		0xddd2	//Адрес внешнего ntp сервера
#define ID_NTP_PERIOD	0xddd3	//Периодичность получения времени с внешнего сервера NTP
#define ID_CLOCK_NAME	0xddd4	//Имя часов в заголовке html-страницы
#define ID_TXT_CUSTOM	0xddd5	//Произвольный текст выводимый перед датой
#define ID_WEATHER		0xddd6	//Настройка клиента погоды


#define CLOCK_NAME_DAFAULT	"Имя часов"	//Имя часов по умолчанию
#define TEXT_CUST_DEFAULT	" "	//Произвольный текст по умолчанию
#define CLOCK_ZONE_DEFAULT	0	//Часовая зона по умолчанию
#define CLOCK_WEB_VER		"0.0.4"	//Версия веб-морды

//Настройка периода проверки
#define NTP_PERIOD_ONCE_DAY		0		//Один раз в сутки, можно указать только час
#define NTP_PERIOD_ONCE_WEEK	1		//Один раз в неделю, можно указать час и день недели (от 1 до 7)
#define NTP_PERIOD_ONCE_MONTH	2		//Один раз в месяц, можно указать день месяца (от 1 до 28) и час
//Значения по умолчанию
#define NTP_PERIOD_HOUR_DEF		0x10	//номер часа в котором будет запущена задача получения времени
#define NTP_PERIOD_TYPE_DEF		1		//Раз в неделю
#define NTP_PERIOD_VALUE_DEF	6		//в субботу

//Типы настройки регулятора звука см. в файле ClockMatrix\Volume.h Здесь только для справки в web - морде
/*
enum tVolumeType{
	vtButton = 0,									//Громкость звука кнопок
	vtEachHour = 1,									// -/-       -/-  каждый час
	vtAlarm = 2,									// -/-       -/-  будильника
	vtSensorTest = 3								// -/-       -/-  получения тестового сигнала от передатчика внешних датчиков
	};
Типы регулирования громкости
enum tVolumeLevel{
	vlConst = 1,							//Постоянный настраиваемый уровень громкости
	vlIncreas = 2,							//Нарастающий уровень до настраиваемого уровня
	vlIncreaseMax = 3,						//Нарастающий уровень до максимального уровня
	vlLast = 4,								//Последний элемент, используется только как признак конца перечисления
};
*/

//Значение громкости для определенного типа (не путать со структурой sVolume в проекте на AVR
struct espVolume{
	unsigned char id;								//Индекс регулятора
	unsigned char Volume;							//Значение громкости для этого типа
	unsigned char State;							//0 - звук этого типа выключен, 1 - включен
	unsigned char LevelMetod;						//Тип регулирования
};

//Если компилятор не AVR то применить эти определения
#if !defined(__COMPILING_AVR_LIBC__)
void ICACHE_FLASH_ATTR ClockWebInit(void);
#endif

#endif /* APP_USER_INCLUDE_CLOCK_WEB_H_ */
