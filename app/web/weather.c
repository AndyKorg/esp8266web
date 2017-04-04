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

struct ip_addr ipaddr;				//IP сервера - resolve from DNS
struct tcp_pcb *TCPSockJSON = NULL;
char *query_template = NULL;

/*
 * Конвертирование строки плавающего в целое через плавающее.
 * Округляет по математически
 */
signed int ICACHE_FLASH_ATTR strtoIntfromFloat(char *s)
{
        double a = 0.0;
        int e = 0;
        int c;
        int negative = 0;

        c = *s;
        if (c == '-'){
        	negative++;
        	s++;
        }
        else if (c == '+'){
        	s++;
        }

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
        if ((a-((int)a)) >0.5){
        	a++;
        }

        return negative? (0-((int)a)) : (int)a; //Приводится к целому именно положительное плавающее, т.к. компилятор отрицательное плавающее преобразует в 0
}

/*
 * Обработка принятого пакета
 */
#define STATE_RESET 	1
#define STATE_NOP 		0

err_t ICACHE_FLASH_ATTR weatherProcess(struct pbuf *p, uint8 Reset)
{

	#define STATE_HTTP_WAITE	0				//Ожидается ответ свервера HTTP/1.1 200 OK
	#define STATE_START		1					//Ожидается начальный апостроф
	#define STATE_SIMBOL	2					//Ожидается символ из строки params с индексом currParam
	#define STATE_STOP		3					//Ожидается конечный апостроф
	#define STATE_END_PARAM	4					//двоеточие
	#define STATE_END_VALUE	5					//Пока не встретится запятая все записывать в буфер значения
	#define STATE_STRING	6					//Идет прием строки

	const char *httpOk =	"HTTP/1.1 200 OK\n";

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


	static uint8
				 currParam = 0,					//Текущий обрабатываемый параметр
				 posParam = 0, 					//Позиция в параметре
				 state = STATE_HTTP_WAITE;		//Стостояние кончного автомата

	if (Reset == STATE_RESET){
		state = STATE_HTTP_WAITE;
		return ERR_OK;
	}

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
			case STATE_HTTP_WAITE: 			//Ожидается ответ сервера HTTP/1.1 200 OK
				if (simbol == httpOk[posParam]){
					posParam++;
					if (posParam == os_strlen(httpOk)){
						state++;
						posParam = 0;
					}
				}
				else if(i == (p->len-2)){ 	//Уже конец первой порции данных, а HTTP 200 так и не было, ошибка
					posParam = 0;
					return ERR_CLSD;
				}
				break;
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
						case WE_ORDER_TEMPR:{
							weatherResult.weathers[weatherResult.currWrite].Temperature = strtoIntfromFloat(startBuf);
							break;
						}
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

#if DEBUGSOO > 0
	os_printf("Json close\n");
#endif
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);

	tcp_close(pcb);

	if (TCPSockJSON != NULL){
		os_free(TCPSockJSON);
		TCPSockJSON = NULL;
	}

	weatherProcess(NULL, STATE_RESET);
	weatherResult.state = WEATHER_STOP;
	if (weatherResFunc != NULL){
		weatherResFunc(ERR_OK);
	}
}

/*
 * Получаем ответ сервера
 */
static err_t ICACHE_FLASH_ATTR json_receive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	LWIP_UNUSED_ARG(arg);
	err_t res = ERR_OK;
#if DEBUGSOO > 0
		os_printf("Json recive err = %d\n", err);
#endif

	os_timer_disarm(&weatherResult.timeout_timet);	//Отключить таймер таймаута ответа сервера
	if((err == ERR_OK) && (p != NULL))
	{
		res = weatherProcess(p, STATE_NOP);			//Обрабатываем принятые данные
	}
	if(p != NULL){
		tcp_recved(pcb, p->tot_len);				//Прием порции данных закончен
		pbuf_free(p);
	}
	if ((res == ERR_CLSD) || (err != ERR_OK)){
		json_client_close(pcb);						//Все принято, можно закрывать соединение
	}
	else{											//Продолжаем прием, таймаут взводим
		os_timer_setfn(&weatherResult.timeout_timet, (os_timer_func_t *) json_client_close, TCPSockJSON);
		ets_timer_arm_new(&weatherResult.timeout_timet, WEATHER_TIMEOUT, OS_TIMER_SIMPLE, OS_TIMER_MS);
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
	if (query_template == NULL)
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
		query_template += os_strlen(query_template);
	}
	os_sprintf_fd(query_template, WEATHER_QUERY_PARAM, weatherSet.APIkey);

	LWIP_UNUSED_ARG(arg);
	if(err == ERR_OK)
	{
		uint8 i;
		tcp_write(pcb, queryOut, os_strlen(queryOut), 0);	//Подготовить запрос
		if (tcp_output(pcb) == ERR_OK){						//Отправить запрос
			weatherResult.currWrite = weatherSet.cnt?1:0; 	//Начать с текущей погоды или с прогноза
			for(i = weatherResult.currWrite; i<(weatherSet.cnt?WEATHER_MAX:1); i++){
				weatherResult.weathers[i].Time = WEATHER_UNDEFINE;
			}
		}
	}
//	os_free(query_template);
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
	if (TCPSockJSON == NULL)
	{
#if DEBUGSOO > 0
		os_printf("weather out of memory!\n");
#endif
		return;
	}

	tcp_recv(TCPSockJSON, json_receive);
	tcp_sent(TCPSockJSON, json_client_sent);
	tcp_connect(TCPSockJSON, &ip_addr, WEATHER_SERVER_PORT, json_connected);//Начать соединение

	os_timer_disarm(&weatherResult.timeout_timet);			//Запуск тамера таймаута
	os_timer_setfn(&weatherResult.timeout_timet, (os_timer_func_t *) json_client_close, TCPSockJSON);
	ets_timer_arm_new(&weatherResult.timeout_timet, WEATHER_TIMEOUT, OS_TIMER_SIMPLE, OS_TIMER_MS);
}

/*
 * Адрес получен от DNS или таймаут
 */
static void ICACHE_FLASH_ATTR resolve(const char *name, struct ip_addr *ipaddr, void *arg){
	if ((ipaddr) && (ipaddr->addr)){	//Адрес полученв
		weatherGet(*ipaddr);
		return;
	}
	weatherResult.state = WEATHER_STOP;	//Не удалось получить адрес
	if (weatherResFunc != NULL){
		weatherResFunc(ERR_RTE);
#if DEBUGSOO > 0
		os_printf("DNS wheather error!\n");
#endif
	}
}

/*
 * Запуск запроса погоды
 */
void ICACHE_FLASH_ATTR weatherStart(void)
{

	if (TCPSockJSON == NULL)
		TCPSockJSON = tcp_new();
#if DEBUGSOO > 0
	if (TCPSockJSON == NULL)
		os_printf("\nTCPSockJSON IS NULL!\n");
#endif

	uint8 noErr = weatherSet.city;
	if (!noErr){
		noErr = (os_strlen(weatherSet.lat)==0)?0:os_strlen(weatherSet.lon);
	}
	noErr += os_strlen(weatherSet.APIkey);
	if (!noErr){						//Нет корректных данных для запроса
#if DEBUGSOO > 0
		os_printf("\nno param weather\n");
#endif
		return;
	}

	weatherResult.state = WEATHER_IN_PROGRESS;

	char *serverName = WEATHER_SERVER_NAME;

	//Сначала получаем IP сервера по имени
	switch (dns_gethostbyname(serverName, &ipaddr, (dns_found_callback) resolve, NULL)) {
		case ERR_OK:					//Адрес разрешен из кэша или локальной таблицы
#if DEBUGSOO > 0
		os_printf("\nadres resolved\n");
#endif
			weatherGet(ipaddr);
			break;
		case ERR_INPROGRESS: 			//Запущен процесс разрешения имени с внешнего DNS
#if DEBUGSOO > 0
		os_printf("\nadres resolv start\n");
#endif
			break;
		default:
#if DEBUGSOO > 0
		os_printf("\nadres error\n");
#endif
			weatherResult.state = WEATHER_STOP;
			if (weatherResFunc != NULL){
				weatherResFunc(ERR_ABRT);
			}
			break;
	}
}
