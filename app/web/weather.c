/*
 * weather.c
 * Запрашивает погоду в формате json и разбирает ее в строку.
 *
 */
#include <stdlib.h>
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "sdk/add_func.h"
#include "../../include/os_type.h"
#include "../include/web_utils.h"
//#include "../../include/osapi.h"

#include "../include/weather.h"
#include "../user/include/clock.h"
#include "../include/my_esp8266.h"

#define atoi	rom_atoi

sWeatherSet weatherSet;
sWeatherResult weatherResult;
VOID_PTR_WEATHER_END weatherResFunc = NULL;		//Callback по окончании обработки команды получения погоды

#define isdigit(val) ((val>='0')&&(val<='9'))

double ICACHE_FLASH_ATTR strtofloat(char *s)
{
        double a = 0.0;
        int e = 0;
        int c;
        while ((c = *s++) != '\0' && isdigit(c)) {
                a = a*10.0 + (c - '0');
        }
        if (c == '.') {
                while ((c = *s++) != '\0' && isdigit(c)) {
                        a = a*10.0 + (c - '0');
                        e = e-1;
                }
        }
        if (c == 'e' || c == 'E') {
                int sign = 1;
                int i = 0;
                c = *s++;
                if (c == '+')
                        c = *s++;
                else if (c == '-') {
                        c = *s++;
                        sign = -1;
                }
                while (isdigit(c)) {
                        i = i*10 + (c - '0');
                        c = *s++;
                }
                e += i*sign;
        }
        while (e > 0) {
                a *= 10.0;
                e--;
        }
        while (e < 0) {
                a *= 0.1;
                e++;
        }
        return a;
}

/*
 * Обработка принятого пакета
 */
err_t ICACHE_FLASH_ATTR weatherProcess(struct pbuf *p)
{

	#define STATE_START		0					//Ожидается начальный апостроф
	#define STATE_SIMBOL	1					//Ожидается символ из строки params с индексом currParam
	#define STATE_STOP		2					//Ожидается конечный апостроф
	#define STATE_END_PARAM	3					//двоеточие
	#define STATE_END_VALUE	4					//Пока не встретится запятая все записывать в буфер значения
	#define STATE_STRING	5					//Идет прием строки

	//Переменные прогноза
	sWeatherParam ForecastParams[WE_PARAM_MAX] = {
			{"dt", WE_ORDER_TIME},				//Время
			{"temp", WE_ORDER_TEMPR},			//температура
			{"description", WE_ORDER_DESCR}		//Описание погоды
	};
	//Текущая погода
	sWeatherParam WeatherParams[WE_PARAM_MAX] = {
			{"description", WE_ORDER_DESCR},	//Описание погоды
			{"temp", WE_ORDER_TEMPR},			//температура
			{"dt", WE_ORDER_TIME}				//Время
	};

	sWeatherParam *paramsProcess = (weatherSet.cnt==WE_TYPE_CURRENT)?WeatherParams:ForecastParams; //Параметры с которыми идет текущая работа

	static uint8 currParam = 0,					//Текущий обрабатываемый параметр
				 posParam = 0, 					//Позиция в параметре
				 state = STATE_START;			//Стостояние кончного автомата

	char simbol, *data = (char*)(p->payload);
	static char *buf, *startBuf;				//Буфер значения переменной

	weatherSet.cnt = (weatherSet.cnt>WEATHER_MAX)?WEATHER_MAX:weatherSet.cnt;
	if ((weatherResult.currWrite >= WEATHER_MAX)
		||
		(
		 (weatherResult.currWrite == weatherSet.cnt)
		 &&
		 (weatherSet.cnt != WE_TYPE_CURRENT)
		)
		){	//Все данные приняты, остальные игнорируем
		return ERR_CLSD;
	}
	if (buf == NULL){							//Памяти не хватило
		buf = os_zalloc(WE_DISCRIPT_LEN);
		if (buf == NULL){
			return ERR_CLSD;
		}
		startBuf = buf;
	}

	uint16 i;

	for(i = 0; i<p->len;i++){
		simbol = data[i];
		switch(state){
			case STATE_START:				//Ожидаем начало имени параметра
				if (simbol == '"'){
					state++;
				}
				posParam = 0;
				break;
			case STATE_SIMBOL:				//Ожидается очередной символ из строки params
				if (simbol != paramsProcess[currParam].Name[posParam]){
					state = STATE_START;
				}
				else{
					posParam++;
					if (!paramsProcess[currParam].Name[posParam]){	//Должен быть конец имени параметра
						state++;
					}
				}
				break;
			case STATE_STOP:				//Следующий должен быть закрывающий апостроф
				if (simbol == '"'){
					state++;
				}
				else{
					state = STATE_START;
				}
				break;
			case STATE_END_PARAM:			//Должно быть двоеточие - конец имени параметра
				buf = startBuf;
				if (simbol == ':'){
					state++;
				}
				else{
					state = STATE_START;
				}
				break;
			case STATE_END_VALUE:			//ожидается запятая - конец значения
				if (simbol == ','){			//Значение прочитано, переносим из буфера в погодную структуру
					*buf = 0;				//Конец строки
					switch (paramsProcess[currParam].order){
						case WE_ORDER_TIME:
							weatherResult.weathers[weatherResult.currWrite].Time = atoi(startBuf);
							break;
						case WE_ORDER_TEMPR:
							weatherResult.weathers[weatherResult.currWrite].Temperature = strtofloat(startBuf);
							break;
						case WE_ORDER_DESCR:
							os_memcpy(weatherResult.weathers[weatherResult.currWrite].DescriptUTF8, startBuf, WE_DISCRIPT_LEN);
							break;
						default:
							break;
					}
					state = STATE_START;
					buf = startBuf;
					currParam++;
					if (currParam == WE_PARAM_MAX){ //Параметры закончились, берем следующую структуру
						currParam = 0;
						weatherResult.currWrite++;
						if ((weatherResult.currWrite == WEATHER_MAX)
								|| (weatherResult.currWrite == weatherSet.cnt)
								|| (weatherSet.cnt == WE_TYPE_CURRENT)
								){
							return ERR_CLSD;	//Остальное игнорировать
						}
					}
				}
				else{							//Значение еще читается
					if (simbol == '"'){			//Переменная-строка
						state = STATE_STRING;
					}
					else{
						*buf++ = simbol;
					}
				}
				break;
			case STATE_STRING:
				if (simbol == '"'){				//Конец переменной-строки
					state = STATE_END_VALUE;
				}
				else{
					*buf++ = simbol;
				}
				break;
			default:
				break;
		}
	}
	return ERR_OK;	//Можно продолжать принимать данные
}

/*
 * Закрытие соединения
 */
static void ICACHE_FLASH_ATTR  json_client_close(struct tcp_pcb *pcb)
{
	os_timer_disarm(&weatherResult.timeout_timet);	//Отключить таймер таймаута ответа сервера
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_close(pcb);
	weatherResult.state = WEATHER_STOP;
	if (weatherResFunc != NULL){
		weatherResFunc(ERR_OK);
	}
#if DEBUGSOO > 0
	os_printf("Json close\n");
#endif
}

/*
 * Получаем ответ сервера
 */
static err_t ICACHE_FLASH_ATTR json_receive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{

	LWIP_UNUSED_ARG(arg);

	os_timer_disarm(&weatherResult.timeout_timet);	//Отключить таймер таймаута ответа сервера
	if(err == ERR_OK && p != NULL)
	{
		if (weatherProcess(p) == ERR_CLSD){		//Обрабатываем принятые данные
			json_client_close(pcb);				//Все принято, можно закрывать соединение
			return ERR_OK;
		}
		tcp_recved(pcb, p->tot_len);			//Прием порции данных закончен
		pbuf_free(p);
	}
	else{
		json_client_close(pcb);
	}
#if DEBUGSOO > 0
		os_printf("Json recive end\n");
#endif
	return ERR_OK;
}

/*
 * Соединение установлено
 */
static err_t ICACHE_FLASH_ATTR json_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
#if DEBUGSOO > 0
		os_printf("Json connect ");
#endif
	uint8 i;
	char *query_template;
	query_template = os_zalloc(QUERY_LEN);
	if (query_template == NULL){
		return ERR_MEM;
	}
	char *queryOut = query_template;
	os_sprintf_fd(query_template, WEATHER_QUERY_HEAD);	//Заголовок
	query_template += os_strlen(query_template);
	if (weatherSet.cnt){
		os_sprintf_fd(query_template, WEATHER_FORECAST);//Запрос прогноза
	}
	else{
		os_sprintf_fd(query_template, WEATHER_CURRENT);	//Запрос текущей погоды
	}
	query_template += os_strlen(query_template);
	os_sprintf_fd(query_template, "?");					//Дальше параметры
	query_template += os_strlen(query_template);
	if (weatherSet.city){
		os_sprintf_fd(query_template, WEATHER_QUERY_ID, weatherSet.city);
	}
	else{
		os_sprintf_fd(query_template, WEATHER_QUERY_LON_LAT, weatherSet.lat, weatherSet.lon);
	}
	query_template += os_strlen(query_template);
	if (weatherSet.cnt){
		os_sprintf_fd(query_template, WEATHER_QUERY_CNT, weatherSet.cnt);
	}
	query_template += os_strlen(query_template);
	os_sprintf_fd(query_template, WEATHER_QUERY_PARAM, weatherSet.APIkey);

#if DEBUGSOO > 0
		os_printf("q= %s\n", queryOut);
#endif

	LWIP_UNUSED_ARG(arg);
	if(err == ERR_OK)
	{
		tcp_write(pcb, queryOut, os_strlen(queryOut), 0);	//Подготовить запрос
		if (tcp_output(pcb) == ERR_OK){				//Отправить запрос
			weatherResult.currWrite = weatherSet.cnt?1:0;
			for(i = weatherResult.currWrite; i<(weatherSet.cnt?WEATHER_MAX:1); i++){
				weatherResult.weathers[i].Time = WEATHER_UNDEFINE;
			}
#if DEBUGSOO > 0
			os_printf("output ok\n");
#endif
		}
	}
#if DEBUGSOO > 0
		os_printf(" result = %d\n", err);
#endif
	return err;
}

/*
 * Запрос передан нормально
 */
static err_t ICACHE_FLASH_ATTR json_client_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
#if DEBUGSOO > 0
		os_printf("Json send\n");
#endif
	LWIP_UNUSED_ARG(arg);
	return ERR_OK;
}

/*
 * Начать запрос погоды. Открыть соединение
 */
static void ICACHE_FLASH_ATTR weatherGet(struct ip_addr ip_addr)
{
	struct tcp_pcb *TCPSockJSON;

	TCPSockJSON = tcp_new();
	if (TCPSockJSON == NULL)
	{
#if DEBUGSOO > 0
		os_printf("weather out of memory!\n");
#endif
		return;
	}
	tcp_recv(TCPSockJSON, json_receive);
	tcp_sent(TCPSockJSON, json_client_sent);
	tcp_connect(TCPSockJSON, &ip_addr, WEATHER_SERVER_PORT, json_connected);

	os_timer_disarm(&weatherResult.timeout_timet);			//Запуск тамера таймаута
	os_timer_setfn(&weatherResult.timeout_timet, (os_timer_func_t *) json_client_close, TCPSockJSON);
	ets_timer_arm_new(&weatherResult.timeout_timet, WEATHER_TIMEOUT, OS_TIMER_SIMPLE, OS_TIMER_MS);
}

/*
 * Адрес получен от DNS или таймаут
 */
static void ICACHE_FLASH_ATTR resolve(const char *name, struct ip_addr *ipaddr, void *arg){
	if ((ipaddr) && (ipaddr->addr)){	//Адрес получен
		weatherGet(*ipaddr);
		return;
	}
	weatherResult.state = WEATHER_STOP;	//Не удалось получить адрес
	if (weatherResFunc != NULL){
		weatherResFunc(ERR_RTE);
	}
}

/*
 * Запуск запроса погоды
 */
void ICACHE_FLASH_ATTR weatherStart(void)
{

	uint8 noErr = weatherSet.city;
	if (!noErr){
		noErr = (os_strlen(weatherSet.lat)==0)?0:os_strlen(weatherSet.lon);
	}
	noErr += os_strlen(weatherSet.APIkey);
	if (!noErr){						//Нет корректных данных для запроса
		return;
	}

	char *serverName = WEATHER_SERVER_NAME;
	struct ip_addr ipaddr;				//IP сервера - resolve from DNS

	weatherResult.state = WEATHER_IN_PROGRESS;
	//Сначала получаем IP сервера по имени
	switch (dns_gethostbyname(serverName, &ipaddr, resolve, NULL)) {
		case ERR_OK:					//Адрес разрешен из кэша или локальной таблицы
			weatherGet(ipaddr);
			break;
		case ERR_INPROGRESS: 			//Запущен процесс разрешения имени с внешнего DNS
			break;
		default:
			weatherResult.state = WEATHER_STOP;
			if (weatherResFunc != NULL){
				weatherResFunc(ERR_ABRT);
			}
			break;
	}
}
