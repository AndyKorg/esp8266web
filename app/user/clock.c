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
uint8 HZSpeed;										//Скорость горизонтальной прокрутки


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
u08 ICACHE_FLASH_ATTR SecundToDateTime(u32 Secunds, struct sClockValue *RetValue, s08 HourOffset){
	u32 restTime, restDay, Tmp = DAYS_01_01_2013;

	#define DAYS_IN_MONTH	(BCDtoInt(LastDayMonth((*RetValue).Month, (*RetValue).Year)))
	#define DAYS_IN_YEAR	(IsLeapYear((uint16)(BCDtoInt((*RetValue).Year)+2000))?366:365)

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
		(*RetValue).Hour = restTime/SEC_IN_HOUR;				//Часов
		restTime -= (((u32)(*RetValue).Hour)*SEC_IN_HOUR);		//Остаток минут-секунд
		(*RetValue).Minute = restTime/SEC_IN_MIN;				//Минут
		(*RetValue).Second = restTime-(((u32)(*RetValue).Minute)*SEC_IN_MIN);	//Секунд
		(*RetValue).Hour = bin2bcd_u32((*RetValue).Hour, 1);		//Приводим к BCD виду
		(*RetValue).Minute = bin2bcd_u32((*RetValue).Minute, 1);
		(*RetValue).Second = bin2bcd_u32((*RetValue).Second, 1);
		//Расчет даты
		(*RetValue).Year = YEAR_LIMIT;							//Стартовый год (2013)
		while(restDay > (Tmp+DAYS_IN_YEAR)){				//Определяется количество целых лет в интервале (с учетом високосных)
			Tmp += DAYS_IN_YEAR;
			(*RetValue).Year = AddOneBCD((*RetValue).Year);
		}
		restDay -= Tmp;										//Остаток дней в году
		(*RetValue).Month=1;
		Tmp = 0;
		while(restDay > (Tmp+DAYS_IN_MONTH)){				//Определяется количество целых месяцев в интервале оставшемся после вычитания лет
			Tmp += DAYS_IN_MONTH;
			(*RetValue).Month = AddOneBCD((*RetValue).Month);
		}
		(*RetValue).Date = bin2bcd_u32(restDay - Tmp, 1);		//Остаток дней в месяце это день месяца. Сразу приводится к BCD виду
		return 1;
	}
	return 0;
}

/*
 * Преобразование UNIX времени в человекочитаемую дату
 */
#define DAYS_01_01_1970 	25567UL 						//Количество дней прошедших между 01.01.1900 и 01.01.1970
u08 ICACHE_FLASH_ATTR UnixTimeToDateTime(u32 Secunds, struct sClockValue *RetValue, s08 HourOffset){
	return SecundToDateTime(Secunds+(DAYS_01_01_1970*SEC_IN_DAY), RetValue, HourOffset);
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

/*
 * Конвертируется строка из utf-8 в win-1251. Только для русского языка, знака градуса и символов до 0x7f!
 * Коневертированая строка помещется на тот же адрес что и входная
 */
void UTF8toWin1251Cyr(char *str){

	#define UTF_8_MASK	0b0001111100111111
	#define UTF_8_HIGTH 0b0001111100000000
	#define UTF_8_LOW 	0b0000000000111111
	#define DEGREE_SYM	0xc2b0
	#define CYR_OFFSET	0x350

	uint16 utf8, j, i;

	j = 0;
	i=0;
	while(str[i]){
		if (!(str[i] & 0x80)){ //Однобайтовый код
			str[j++] = str[i];
		}
		else{
			utf8 = (((uint16)str[i]<<8) + ((uint16)str[i+1]));
			i++;
			if (utf8 == DEGREE_SYM){
				str[j++] = str[i];
			}
			else{
				utf8 &= UTF_8_MASK;
				utf8 = ((utf8 & UTF_8_HIGTH) >>2) + (utf8 & UTF_8_LOW); //Восстанавливются биты символа UTF8
				str[j++] = (char)(utf8-CYR_OFFSET);	//Конвертируем в win-1251
			}
		}
		i++;
	}
	str[j++] = 0;	//Конец строки
}

/*
 * Устанавливает следующую дату, время не изменяется
 */
void nextDate(struct sClockValue *value){
	if (LastDayMonth(value->Month, value->Year) == value->Date){
		if (value->Month == 0x12){
			value->Year = AddOneBCD(value->Year);
			value->Month = 0;
		}
		value->Month = AddOneBCD(value->Month);
		value->Date = 0;
	}
	value->Date = AddOneBCD(value->Date);
}
