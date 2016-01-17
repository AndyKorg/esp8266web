/*
 * Переменные и функции чаосв
 *
 */
#include "clock.h"
#include "../include/web_utils.h"
#include "string.h"

struct sClockValue Watch;							//Часы
struct sAlarm Alarms[ALARM_MAX];					//Будильники
sint8 ClockZone = 0;								//Часовой пояс
struct sNtpPeriod NtpPeriod;						//Периодичность запроса времени из внешенего сервера NTP
uint8 ClockName[CLOCK_NAME_MAX];					//Имя часов для html-страницы
uint8 TextCustom[TEXT_CUST_MAX];					//Произвольный текст выводимый на часы
uint8 FontNum;										//Номер шрифта
struct espVolume VolumeClock[VOL_TYPE_COUNT];		//Состояние регуляторов громкости
struct sSensor Sensors[SENSOR_MAX];					//Массив сенсоров


/*
 *  reverse:  переворачиваем строку s на месте
 */
void ICACHE_FLASH_ATTR reverse(char *s){
    unsigned int i=0, j=0;
    char c;

    while (s++) ++j;			//длина строки s
    for (i = 0; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/*
 * Конвертация целого в строку (с)K&R
 */
void ICACHE_FLASH_ATTR itoa(int n, char *s)
{
    int i, sign;

    if ((sign = n) < 0)  /* записываем знак */
        n = -n;          /* делаем n положительным числом */
    i = 0;
    do {       /* генерируем цифры в обратном порядке */
        s[i++] = n % 10 + '0';   /* берем следующую цифру */
    } while ((n /= 10) > 0);     /* удаляем */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

/************************************************************************/
/* Преобразование двоичного в BCD                                       */
/************************************************************************/
u32 ICACHE_FLASH_ATTR bin2bcd_u32(u32 data, u08 result_bytes){
	u32 result = 0; /*result*/
	u08 cnt_bytes, cnt_bits;

	for (cnt_bytes=(4 - result_bytes); cnt_bytes; cnt_bytes--) /* adjust input bytes */
	data <<= 8;
	for (cnt_bits=(result_bytes << 3); cnt_bits; cnt_bits--) { /* bits shift loop */
		/*result BCD nibbles correction*/
		result += 0x33333333;
		/*result correction loop*/
		for (cnt_bytes=4; cnt_bytes; cnt_bytes--) {
			u08 corr_byte = result >> 24;
			if (!(corr_byte & 0x08)) corr_byte -= 0x03;
			if (!(corr_byte & 0x80)) corr_byte -= 0x30;
			result <<= 8; /*shift result*/
			result += corr_byte; /*set 8 bits of result*/
		}
		/*shift next bit of input to result*/
		result <<= 1;
		if (((u08)(data >> 24)) & 0x80)
		result |= 1;
		data <<= 1;
	}
	return(result);
}

/************************************************************************/
/* День недели по дате.													*/
/* Возвращает 1-пн, 2-вт и т.д.	7-вс									*/
/************************************************************************/
u08 what_day(const u08 date, const u08 month, const u08 year){
	int d = (int)BCDtoInt(date),
		m = (int)BCDtoInt(month),
		y = (int)(2000+BCDtoInt(year));
	if ((month == 1)||(month==2)){
		y--;
		d += 3;
	}
	return (1 + (d + y + y/4 - y/100 + y/400 + (31*m+10)/12) % 7);
}

#define DAYS_01_01_2013	41272UL						//Точное количество дней прошедших с 01.01.1900 по 01.01.2013
/************************************************************************/
/* Рассчитывает дату и время по количеству прошедших секунд 			*/
/* с 01.01.1900. Количество секунд должно быть для дат от 01.01.2013	*/
/* HourOffset - смещение в часах для требуемой часовой зоны	от -12 до 12*/
/* Возвращает 1 если все нормально и 0 если ошибка при расчете			*/
/************************************************************************/
u08 ICACHE_FLASH_ATTR SecundToDateTime(u32 Secunds, struct sClockValue *Value, s08 HourOffset){
	u32 restTime, restDay, Tmp = DAYS_01_01_2013;

	#define DAYS_IN_MONTH	(BCDtoInt(LastDayMonth((*Value).Month, (*Value).Year)))
	#define DAYS_IN_YEAR	(IsLeapYear((uint16)(BCDtoInt((*Value).Year)+2000))?366:365)

	if ((Secunds > (DAYS_01_01_2013*SEC_IN_DAY))
		&&
		(HourOffset >=-12) && (HourOffset <=12)){
		//Расчет местного времени
		if (HourOffset<0){
			HourOffset = (~HourOffset)+1;
			Secunds -= (u32)(HourOffset * SEC_IN_HOUR);
		}
		else
			Secunds += (u32)(HourOffset * SEC_IN_HOUR);
		restDay = Secunds/SEC_IN_DAY;						//Количество дней в интервале
		//Расчет времени
		restTime = Secunds-(restDay*SEC_IN_DAY);			//Количество секунд оставшихся после вычитания дней, т.е. время дня.
		(*Value).Hour = restTime/SEC_IN_HOUR;				//Часов
		restTime -= (((u32)(*Value).Hour)*SEC_IN_HOUR);		//Остаток минут-секунд
		(*Value).Minute = restTime/SEC_IN_MIN;				//Минут
		(*Value).Second = restTime-(((u32)(*Value).Minute)*SEC_IN_MIN);	//Секунд
		(*Value).Hour = bin2bcd_u32((*Value).Hour, 1);		//Приводим к BCD виду
		(*Value).Minute = bin2bcd_u32((*Value).Minute, 1);
		(*Value).Second = bin2bcd_u32((*Value).Second, 1);
		//Расчет даты
		(*Value).Year = YEAR_LIMIT;							//Стартовый год (2013)
		while(restDay > (Tmp+DAYS_IN_YEAR)){				//Определяется количество целых лет в интервале (с учетом високосных)
			Tmp += DAYS_IN_YEAR;
			(*Value).Year = AddOneBCD((*Value).Year);
		}
		restDay -= Tmp;										//Остаток дней в году
		(*Value).Month=1;
		Tmp = 0;
		while(restDay > (Tmp+DAYS_IN_MONTH)){				//Определяется количество целых месяцев в интервале оставшемся после вычитания лет
			Tmp += DAYS_IN_MONTH;
			(*Value).Month = AddOneBCD((*Value).Month);
		}
		(*Value).Date = bin2bcd_u32(restDay - Tmp, 1);		//Остаток дней в месяце это день месяца. Сразу приводится к BCD виду
		return 1;
	}
	return 0;
}

/*
 * Коневертирует строку xx-xx-xxxx в дату
 * Возвращает 1 если все нормально, 0 - если преобразование не удалось
 */
uint8 strtodate(uint8* pvar, struct sClockValue* Value){
	uint8 partdate[6], d,m, Ret=0;

	pvar = cmpcpystr(partdate, pvar, '\0', '-', 3);						//Дата
	if(pvar != NULL) {
		d = hextoul(partdate);
		pvar = cmpcpystr(partdate, pvar, '-', '-', 3);					//Месяц
	   	if(pvar != NULL) {
	   		m = hextoul(partdate);
	   		pvar = cmpcpystr(partdate, pvar, '-', '\0', 5);				//Год
	   	   	if(pvar != NULL) {
	   	   		uint8 *date_ptr = partdate;
	   	   		if (strlen(partdate)==4) date_ptr += 2;
	   	   		(*Value).Year = hextoul(date_ptr);
	   	   		(*Value).Date = d;
	   	   		(*Value).Month = m;
	   	   		Ret = 1;
	   	   	}
	   	}
	}
	return Ret;
}
