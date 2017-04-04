#ifndef WEATHER_H
#define WEATHER_H

#define WEATHER_SERVER_PORT 	80
#define WEATHER_SERVER_NAME		"api.openweathermap.org"		//Имя сервера погоды. Задается жестко, т.к. алгоритм рассчитан только на него
#define WEATHER_TIMEOUT			(1000*30)

#define WEATHER_QUERY_HEAD		"GET /data/2.5/"
#define WEATHER_FORECAST		"forecast"
#define WEATHER_CURRENT			"weather"
#define WEATHER_QUERY_ID		"id=%d" 						//Запрос по id населенного пункта
#define WEATHER_QUERY_LON_LAT	"lat=%s&lon=%s" 				//по широте-долготе
#define WEATHER_QUERY_CNT		"&cnt=%d"
#define WEATHER_QUERY_PARAM		"&APPID=%s&units=metric&lang=ru HTTP/1.1\r\nHost: esp8266.ru\r\n\r\n" //TODO:Сделать английский язык
#define QUERY_LEN 				512		//Длина запроса с запасом

#define WE_TYPE_FORECAST		8		//Количество отсчтеов для прогноза погоды
#define	WE_TYPE_CURRENT			0		//Текущая погода

#define WEATHER_STOP			0		//Готов к приему следующего запроса
#define WEATHER_IN_PROGRESS		1		//Идет процесс получения погоды

#define	WEATHER_MAX 			9		//Максимальное количество погодных условий (сутки с трехчасовым интервалом, плюс текущая погода
#define WEATHER_UNDEFINE		0		//Погода не определена

#define WE_DISCRIPT_LEN			64		//Длина описания погоды, исходный код может быть в utf-8

//Описание параметров json
#define WE_PARAM_MAX	3				//Количество извлекаемых параметров

#define WE_PARAM_TYPE_INT	0			//Целое число
#define WE_PARAM_TYPE_FLOAT	1			//десятичная дробь
#define WE_PARAM_TYPE_STR	2			//строка

enum weType{
	WE_ORDER_TIME,					 	//переменная Time в sWeather
	WE_ORDER_TEMPR,						//Temperature в sWeather
	WE_ORDER_DESCR						//Descript в sWeather
};

typedef struct{
	char *Name;							//Имя параметра в ответе сервера
	enum weType order;					//Порядковый номер в стуктуре sWeather
} sWeatherParam;

//Готовая погода
typedef struct{
	uint32 Time;						//Время полученное с сервера UNIX формат,UTC
	signed int Temperature;					//Температура
	char DescriptUTF8[WE_DISCRIPT_LEN];	//Описание строкой
} sWeather;

typedef struct{
	uint8 state;						//Состояние
	ETSTimer timeout_timet;				//Таймаут сервера
	uint8 currWrite;					//Текущий записываемый элемент
	sWeather weathers[WEATHER_MAX];		//Нулевой элемент это текущая погода
} sWeatherResult;

//Настройки клиента
#define wAPIkeyLen	64
#define wLonLatLen	13
typedef struct{
	char APIkey[wAPIkeyLen];			//ключ API получаемый от weather.org
	uint32 city;						//Индекс населенного пункта, если 0 то используется долгота-широта
	char lat[wLonLatLen];				//Широта (Latitude) в виде строки-числа с точкой "-11.2222"
	char lon[wLonLatLen];				//Долгота (Longitude)
	uint8 cnt;							//КОличество запрашиваемых структур sWeather. Если cnt = 0 то запрашивается текушая погода
	uint8 second;						//Секунда в нулевой минуте когда запрашивать погоду
} sWeatherSet;

extern sWeatherSet weatherSet;			//Настройка клиента
extern sWeatherResult weatherResult;	//Ответ сервера

typedef void (*VOID_PTR_WEATHER_END)	//Функция вызвываемая по окончании процесса получения погоды
				(err_t result);			//результат отработки

VOID_PTR_WEATHER_END weatherResFunc;		//Callback по окончании обработки команды получения погоды
void ICACHE_FLASH_ATTR weatherStart(void);

#endif	//WEATHER_H
