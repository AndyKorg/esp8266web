/*
 * web interface for clock matrix.
 *
 */
#include "../include/ntp.h"
#include "../include/weather.h"
#include "../Include/web_utils.h"
#include "flash_eep.h"
#include "sdk/mem_manager.h"
#include "../include/my_esp8266.h"
#include "../../include/user_interface.h"
#include "../include/wifi.h"
#include "customer_html.h"
#include "customer_uart.h"
#include "include/clock.h"
#include "include/clock_web.h"
#include "sdk/add_func.h"
#include "../include/my_esp8266.h"


#ifndef USE_PING
	#error "Set define USE_LWIP_PING for ping ntp in user_config.h\n"
#else
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/app/ping.h"
struct ping_option *pingopt;
#endif

#define PING_COUNT		3			//Количество запросов ping. Не забудь исправить в HTML странице

uint8 idObject;						//Индекс объета в текущей команде если команда вида cmdParent_N_cmdChild. (например alarm_0_on
err_t pingState = ERR_VAL;			//Состояние пинга до хоста
uint8 ClockWebVer[] = CLOCK_WEB_VER;//Версия вебморды

//Прототипы функций обработки переменных и команд
//HTML =============================
//-------- Get
//weather
uint8 ICACHE_FLASH_ATTR weatherCurrent(WEB_SRV_CONN* web_conn, uint8* cstr);
//volume
uint8 ICACHE_FLASH_ATTR volumeGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR volumeNtypeGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR volumeNmaxGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR volumeNonGet(WEB_SRV_CONN* web_conn, uint8* cstr);
//ntp
uint8 ICACHE_FLASH_ATTR ntpGetIP(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR watchGetZone(WEB_SRV_CONN* web_conn, uint8* cstr);
//alarm
uint8 ICACHE_FLASH_ATTR alarmsGet(WEB_SRV_CONN* web_conn, uint8* cstr);						//Alarms
uint8 ICACHE_FLASH_ATTR alarmNhourGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR alarmNminuteGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR alarmNdateGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR alarmNmonthGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR alarmNyearGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR alarmNduratGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR alarmN_OnGet(WEB_SRV_CONN* web_conn, uint8* cstr);
//sensor
uint8 ICACHE_FLASH_ATTR sensorGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR sensNnameGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR sensNadrGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR sensNonGet(WEB_SRV_CONN* web_conn, uint8* cstr);
uint8 ICACHE_FLASH_ATTR sensNdataGet(WEB_SRV_CONN* web_conn, uint8* cstr);
//watch
uint8 ICACHE_FLASH_ATTR watchGetDay(WEB_SRV_CONN* web_conn, uint8* cstr);					//День недели

//-------- Set и команды
//weather
uint8 ICACHE_FLASH_ATTR weatherSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR weatherTest(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//volume
uint8 ICACHE_FLASH_ATTR volumeSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR volumeNtypeSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR volumrNmaxSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR volumeNonSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR volumeNtest(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR volumeNsave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//clock
uint8 ICACHE_FLASH_ATTR clockSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//font
uint8 ICACHE_FLASH_ATTR fontSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//ping
uint8 ICACHE_FLASH_ATTR pingStart(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//ntp
uint8 ICACHE_FLASH_ATTR ntpClientGetTime(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR ntpCfgSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR ntpSetIP(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//watch
uint8 ICACHE_FLASH_ATTR watchSetDate(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR watchSetZone(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR watchSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//alarm
uint8 ICACHE_FLASH_ATTR alarmsSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);			//Alarms
uint8 ICACHE_FLASH_ATTR alarmNhourSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR alarmNminuteSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR alarmNdateSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR alarmNduratSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR alarmN_OnSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR alarmNSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//sensor
uint8 ICACHE_FLASH_ATTR sensorSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR sensNnameSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR sensNadrSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR sensNonSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR sensNtest(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
uint8 ICACHE_FLASH_ATTR sensNsave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);
//STOP
uint8 ICACHE_FLASH_ATTR STOP_Set(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar);

//HTML ============================= Список переменных и команд http
//weather
pHttpVar HttpWeather[7]={
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"api", 	vtString,	"%s",		weatherSet.APIkey,	NULL,		NULL,				NULL,		0},
	{"city", 	vtWord32,	"%d",		&weatherSet.city,	NULL,		NULL,				NULL, 		0},
	{"lon", 	vtString,	"%s",		weatherSet.lon,		NULL,		NULL,				NULL, 		0},
	{"lat", 	vtString,	"%s",		weatherSet.lat,		NULL,		NULL,				NULL, 		0},
	{"save", 	vtNULL,		NULL,			NULL,			NULL,		weatherSave,		NULL, 		0},
	{"test", 	vtNULL,		NULL,			NULL,			NULL,		weatherTest,		NULL, 		0},
	{"current",	vtNULL,		NULL,			NULL,		weatherCurrent,	NULL,				NULL, 		0}
};
//volume_N
pHttpVar HttpVolumeN[5]={
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"type", 	vtNULL,		NULL,			NULL,		volumeNtypeGet,	volumeNtypeSet,		NULL, 		0},
	{"max", 	vtNULL,		NULL,			NULL,		volumeNmaxGet,	volumrNmaxSet,		NULL, 		0},
	{"on", 		vtNULL,		NULL,			NULL,		volumeNonGet,	volumeNonSet,		NULL, 		0},
	{"test", 	vtNULL,		NULL,			NULL,			NULL,		volumeNtest,		NULL, 		0},
	{"save", 	vtNULL,		NULL,			NULL,			NULL,		volumeNsave,		NULL, 		0}
};
//Список индексов для команды volume
uint8* VolumeType[VOL_TYPE_COUNT]={
		"key_",											//vtButton = 0, громкость звука кнопок
		"chime_",										//vtEachHour = 1, куранты
		"alarm_",										//vtAlarm = 2, будильник
		"sensor_"										//vtSensorTest = 3, получения тестового сигнала от передатчика внешних датчиков TODO:Громкость теста сенсора пока не используется
};
//clock_
pHttpVar HttpClock[6]={
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"speed", 	vtByte,		"%d", 			&HZSpeed,		NULL,		NULL,				NULL, 		0},
	{"ver", 	vtString,	"%s", 			ClockWebVer,	NULL,		NULL,				NULL, 		0},
	{"stop", 	vtNULL,		NULL, 			NULL,			NULL,		STOP_Set,			NULL, 		0},
	{"name", 	vtString,	"%s",			ClockName,		NULL,		NULL,				NULL, 		0},
	{"txtcust", vtString,	"%s",			TextCustom,		NULL,		NULL,				NULL, 		0},
	{"save", 	vtNULL,		NULL,			NULL,			NULL,	 	clockSave,			NULL, 		0}
};
//font_
pHttpVar HttpFont[2]={
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"num", 	vtByte,		"%d",			&FontNum,		NULL,		NULL,				NULL, 		0},
	{"save", 	vtNULL,		NULL,			NULL,			NULL,	 	fontSet,			NULL, 		0}
};
//sensor_N_
pHttpVar HttpSensorN[6]={
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"data", 	vtNULL,		NULL,			NULL,		   sensNdataGet,NULL,				NULL, 		0},
	{"name", 	vtNULL,		NULL,			NULL,		   sensNnameGet,sensNnameSet,		NULL, 		0},
	{"adr", 	vtNULL,		NULL,			NULL,			sensNadrGet,sensNadrSet,		NULL, 		0},
	{"on", 		vtNULL,		NULL,			NULL,			sensNonGet,	sensNonSet,			NULL, 		0},
	{"test", 	vtNULL,		NULL,			NULL,			NULL,	 	sensNtest,			NULL, 		0},
	{"save", 	vtNULL,		NULL,			NULL,			NULL,	 	sensNsave,			NULL, 		0}
};
//alarm_N_ - команды и переменные для конкретного будильника кроме переменных дня недели, обрабатывается в alarmsGet и alarmsSet
pHttpVar HttpAlarmN[8]={
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"hour", 	vtNULL,		NULL,			NULL,		alarmNhourGet,	alarmNhourSet,		NULL, 		0},
	{"minute", 	vtNULL,		NULL,			NULL,		alarmNminuteGet,alarmNminuteSet,	NULL, 		0},
	{"date", 	vtNULL,		NULL,			NULL,		alarmNdateGet,	alarmNdateSet,		NULL, 		0},
	{"month", 	vtNULL,		NULL,			NULL,		alarmNmonthGet,	NULL,				NULL, 		0},
	{"year", 	vtNULL,		NULL,			NULL,		alarmNyearGet,	NULL,				NULL, 		0},
	{"on", 		vtNULL,		NULL,			NULL,		alarmN_OnGet,  	alarmN_OnSet,		NULL, 		0},
	{"durat", 	vtNULL,		NULL,			NULL,		alarmNduratGet,	alarmNduratSet,		NULL, 		0},
	{"save", 	vtNULL,		NULL,			NULL,			NULL,		alarmNSave,			NULL, 		0}
};
//ping_
pHttpVar HttpPing[2] = {
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"start", 	vtNULL,		NULL, 			NULL,			NULL,		pingStart,			NULL, 		0},
	{"result", 	vtErr,		"%d", 			&pingState,		NULL,		NULL,				NULL, 		0}
};
//ntp_
pHttpVar HttpNTP[7] = {
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 			Child;		ChildLen
	{"ip", 		vtNULL,		NULL, 			NULL,			ntpGetIP,	ntpSetIP,			NULL, 		0},
	{"port", 	vtWord16,	"%d", 		&ntp_srv_ip.port,	NULL,		NULL,				NULL, 		0},
	{"per_hour",vtByte,		"%x", 		&NtpPeriod.Hour,	NULL,		NULL,				NULL, 		0},
	{"per_type",vtByte,		"%x", 	&NtpPeriod.PeriodType,	NULL,		NULL,				NULL, 		0},
	{"per_value",vtByte,	"%d", 	   &NtpPeriod.Value,	NULL,		NULL,				NULL, 		0},
	{"save", 	vtNULL,		NULL, 			NULL,			NULL,		ntpCfgSave,			NULL, 		0},
	{"gettime", vtNULL, 	NULL, 			NULL,			NULL,		ntpClientGetTime,	NULL, 		0}
};
//watch_
pHttpVar HttpWatch[9] = {
	// Name;	varType; 	ArgPrnHttp; 	Value; 			execGet; 	execSet; 	Child;		ChildLen
	{"zone", 	vtNULL,		NULL, 			NULL,  	   watchGetZone, watchSetZone,	NULL, 		0},
	{"save", 	vtNULL,		NULL, 			NULL,			NULL,		watchSave,	NULL, 		0},
	{"second", 	vtByte, 	"%02x", 		&Watch.Second,	NULL,		NULL, 		NULL, 		0},
	{"minute", 	vtByte, 	"%02x", 		&Watch.Minute,	NULL,		NULL, 		NULL,		0},
	{"hour", 	vtByte, 	"%02x", 		&Watch.Hour,	NULL,		NULL, 		NULL,		0},
	{"day", 	vtNULL, 	NULL, 			NULL,		watchGetDay,	NULL, 		NULL,		0},
	{"date", 	vtByte, 	"%02x", 		&Watch.Date,	NULL,	watchSetDate, 	NULL,		0},
	{"month", 	vtByte, 	"%02x", 		&Watch.Month,	NULL,		NULL, 		NULL,		0},
	{"year", 	vtByte, 	"%02x", 		&Watch.Year,	NULL,		NULL, 		NULL,		0}
};

//ROOT, объявлен customer_html.c
pHttpVar HttpMyRoot[HTTP_ROOT_LEN] = {
	// Name;	vType; 		ArgPrnHttp; 	Value; 	execGet; 	execSet; 	Child;			ChildLen
	{"ntp_", 	vtNULL, 	NULL, 			NULL,	NULL,		NULL,		&HttpNTP, 		SizeArray(HttpNTP)},
	{"watch_", 	vtNULL, 	NULL, 			NULL,	NULL,		NULL,		&HttpWatch, 	SizeArray(HttpWatch)},
	{"alarm_", 	vtNULL, 	NULL, 			NULL,	alarmsGet,	alarmsSet,	NULL, 			0},
	{"sensor_", vtNULL, 	NULL, 			NULL,	sensorGet,	sensorSet,	NULL, 			0},
	{"ping_", 	vtNULL,		NULL, 			NULL,	NULL,		NULL,		&HttpPing, 		SizeArray(HttpPing)},
	{"font_", 	vtNULL,		NULL, 			NULL,	NULL,		NULL,		&HttpFont, 		SizeArray(HttpFont)},
	{"clock_", 	vtNULL,		NULL, 			NULL,	NULL,		NULL,		&HttpClock, 	SizeArray(HttpClock)},
	{"volume_", vtNULL,		NULL, 			NULL,	volumeGet,	volumeSet,	NULL, 			0},
	{"weather_",vtNULL,		NULL, 			NULL,	NULL,		NULL,		&HttpWeather, 	SizeArray(HttpWeather)}
};

//UART =============================
uint8 ICACHE_FLASH_ATTR UartAlarmSet(uint8 cmd, uint8* pval, uint8 valLen);
uint8 ICACHE_FLASH_ATTR UartNtpStart(uint8 cmd, uint8* pval, uint8 valLen);
uint8 ICACHE_FLASH_ATTR UartVolumeSet(uint8 cmd, uint8* pval, uint8 valLen);
uint8 ICACHE_FLASH_ATTR UartSensorSet(uint8 cmd, uint8* pval, uint8 valLen);
uint8 ICACHE_FLASH_ATTR UartWatchReciv(uint8 cmd, uint8* pval, uint8 valLen);	//Использется как односекундный таймер
uint8 ICACHE_FLASH_ATTR UartStAuth(uint8 cmd, uint8* pval, uint8 valLen);		//Пароль и имя сети для station
uint8 ICACHE_FLASH_ATTR UartStIP(uint8 cmd, uint8* pval, uint8 valLen);			//IP адрес
uint8 ICACHE_FLASH_ATTR UartTextCustom(uint8 cmd, uint8* pval, uint8 valLen);	//Произвольный текст выводимый перед датой

//UART =============================  Список переменных и команд uart -------------------
//По этому списку отрабатываются приняты команды по uart. Передача происходит отдельыми процедурами
pClockUartCmd ClockUartCmd[UART_NUM_COUNT] = {
/*	CmdCode, 			Value, 		LenData, 				Func */
	{CLK_WATCH, 		&Watch, 	sizeof(Watch),			UartWatchReciv},
	{CLK_ALARM, 		NULL, 		sizeof(Alarms[0]),		UartAlarmSet},
	{CLK_NTP_START, 	NULL, 		1, 						UartNtpStart},
	{CLK_VOLUME,		NULL,		sizeof(VolumeClock[0]),	UartVolumeSet},
	{CLK_SENS,			NULL,		(sizeof(Sensors[0])+1),	UartSensorSet},	//Плюс один байт для номера датчика в массиве
	{CLK_FONT,			&FontNum,	sizeof(FontNum),		NULL},
	{CLK_ST_WIFI_AUTH,	NULL,		0, 						UartStAuth},
	{CLK_ST_WIFI_IP,	NULL,		0, 						UartStIP},
	{CLK_CUSTOM_TXT,	NULL,		0, 						UartTextCustom},
	{CLK_HZ_SPEED,		&HZSpeed,   sizeof(HZSpeed), 		NULL}
};

//COMMON =============================  общие функции
// Вывод времени полученного от ntp сервера в часы
void ICACHE_FLASH_ATTR SetWatchInClock(uint32 Seconds){
	if (Seconds != NTP_TIME_UNKNOWN){
		SecundToDateTime(Seconds, &Watch, ClockZone);
		ClockUartTx(ClkWrite(CLK_WATCH), (uint8 *) &Watch, sizeof(Watch));
	}
#if (DEBUGSOO==1)
	else{
		os_printf("ntp bad answer\n");
	}
#endif
}

//HTML ============================= "STOP"
uint8 ICACHE_FLASH_ATTR STOP_Set(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ClockUartTx(ClkRead(CLK_STOP), NULL, 0);
	return 0;
}

//HTML ============================= "weather_"
uint8 ICACHE_FLASH_ATTR weatherSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	uint8 noErr = weatherSet.city;
	if (!noErr){
		noErr = (strlen(weatherSet.lat)==0)?0:strlen(weatherSet.lon);
	}
	noErr += strlen(weatherSet.APIkey);
	if (!noErr){
		if (flash_read_cfg(&weatherSet, ID_WEATHER, sizeof(weatherSet)) != sizeof(weatherSet)){
			ets_memset(weatherSet.APIkey, 0, wAPIkeyLen);
			weatherSet.city = 0;
			ets_memset(weatherSet.lon, 0, wLonLatLen);
			ets_memset(weatherSet.lat, 0, wLonLatLen);
		}
		return 0;
	}
	uint8 rnd = (uint8)bin2bcd_u32((uint32)weatherSet.APIkey[0], 1);//Как дачтик случайного числа
	rnd &= 0x1f;
	weatherSet.second = rnd;
	if (!flash_save_cfg(&weatherSet, ID_WEATHER, sizeof(weatherSet))){
#if (DEBUGSOO>0)
		os_printf("no save\n");
#endif
	}
	return 0;
}

uint8 ICACHE_FLASH_ATTR weatherTest(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
#if (DEBUGSOO>0)
		os_printf("[AK]test start\n");
#endif
	if (weatherResult.state  == WEATHER_STOP){
		weatherSet.cnt = WE_TYPE_CURRENT;	//Запросить текущую погоду
		weatherResFunc = NULL;				//По окончании ничего не делать
		weatherStart();
	}
	return 0;
}

uint8 ICACHE_FLASH_ATTR weatherCurrent(WEB_SRV_CONN* web_conn, uint8* cstr){
	if (weatherResult.state == WEATHER_STOP){
		if (weatherResult.weathers[WE_TYPE_CURRENT].Time){
			#define CURR_TEXT  "today t=%d°C %s"
			char *tmp = os_zalloc(os_strlen(weatherResult.weathers[WE_TYPE_CURRENT].DescriptUTF8)+os_strlen(CURR_TEXT)+10);
			os_sprintf_fd(tmp, CURR_TEXT, weatherResult.weathers[WE_TYPE_CURRENT].Temperature, weatherResult.weathers[WE_TYPE_CURRENT].DescriptUTF8);
			UTF8toWin1251Cyr(tmp);
			tcp_puts("%s", tmp);
			os_free(tmp);
#if (DEBUGSOO>0)
		os_printf("[AK]curr out\n");
#endif
		}
		else{
			tcp_puts("current weather no recived");
		}
	}
	else{
		tcp_puts("weather in progress");
	}

	return 0;
}

//HTML ============================= "volume_N_"
//Запись параметра громкости в командах volume_N_ где N от 0 до VOL_TYPE_COUNT
uint8 ICACHE_FLASH_ATTR volumeSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
#if SENSOR_MAX >9
	#error"VOL_TYPE_COUNT more then 9, need rewrite function sensorSet"
#else
	uint8 Ret = 0;
	uint8 i = ParceNumObject(pcmd, VOL_TYPE_COUNT, VolumeType, &Ret);

	if (i <= VOL_TYPE_COUNT){
		idObject = i;													//индекс громкости
		pcmd += Ret;
		Ret += parseHttpSetVar(HttpVolumeN, SizeArray(HttpVolumeN), web_con, pcmd, pvar);
	}
	return Ret;
#endif
}
//Чтение параметра громкости в командах volume_N_ где N от 0 до VOL_TYPE_COUNT
uint8 ICACHE_FLASH_ATTR volumeGet(WEB_SRV_CONN* web_conn, uint8* cstr){
#if VOL_TYPE_COUNT >9
	#error"VOL_TYPE_COUNT more then 9, need rewrite function sensorSet"
#else
	uint8 Ret = 0;
	uint8 i = ParceNumObject(cstr, VOL_TYPE_COUNT, VolumeType, &Ret);

	if (i <= VOL_TYPE_COUNT){
		idObject = i;													//индекс сенсора
		cstr += Ret;
		Ret += parseHttpGetVar(HttpVolumeN, SizeArray(HttpVolumeN), web_conn, cstr);
	}
	return Ret;
#endif
}
//Тестовое воспроизведение громоксти
uint8 ICACHE_FLASH_ATTR volumeNtest(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ClockUartTx(ClkTest(CLK_VOLUME), (uint8 *)&VolumeClock[idObject], sizeof(VolumeClock[idObject]));
	return 0;
}
//Запись громкости
uint8 ICACHE_FLASH_ATTR volumeNsave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ClockUartTx(ClkWrite(CLK_VOLUME), (uint8 *)&VolumeClock[idObject], sizeof(VolumeClock[idObject]));
	return 0;
}
//Тип громкости
uint8 ICACHE_FLASH_ATTR volumeNtypeSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	VolumeClock[idObject].LevelMetod = rom_atoi(pvar);
	return 0;
}
uint8 ICACHE_FLASH_ATTR volumeNtypeGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%d", VolumeClock[idObject].LevelMetod);
	return 0;
}
//Включение-выключение звука
uint8 ICACHE_FLASH_ATTR volumeNonSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	if (pvar[0]=='1')
		VolumeClock[idObject].State = 1;
	else
		VolumeClock[idObject].State = 0;
	return 0;
}
uint8 ICACHE_FLASH_ATTR volumeNonGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%d", VolumeClock[idObject].State);
	return 0;
}
//Максимальное значение громкости
uint8 ICACHE_FLASH_ATTR volumrNmaxSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	VolumeClock[idObject].Volume = rom_atoi(pvar);
	return 0;
}
uint8 ICACHE_FLASH_ATTR volumeNmaxGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%d", VolumeClock[idObject].Volume);
	return 0;
}

//HTML ============================= "clock_"
uint8 ICACHE_FLASH_ATTR clockSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	if (flash_save_cfg((void*)ClockName, ID_CLOCK_NAME, CLOCK_NAME_MAX)){
#if DEBUGSOO==1
		os_printf("clock name save ok\npostsave clock name = %s\n", ClockName);
#endif
	}
#if DEBUGSOO==1
	else{
		os_printf("clock name save error/n");
	}
#endif
	if (flash_save_cfg((void*)TextCustom, ID_TXT_CUSTOM, TEXT_CUST_MAX)){
#if DEBUGSOO==1
		os_printf("customer text save ok\ncustomer text = %s\n", TextCustom);
#endif
	}
#if DEBUGSOO==1
	else{
		os_printf("customer text save error/n");
	}
#endif
	ClockUartTx(ClkWrite(CLK_HZ_SPEED), (uint8*) &HZSpeed, sizeof(HZSpeed));
	return 0;
}

//HTML ============================= "font_
uint8 ICACHE_FLASH_ATTR fontSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ClockUartTx(ClkWrite(CLK_FONT), (uint8*) &FontNum, sizeof(FontNum));
	return 0;
}

//HTML ============================= "sensor_"
//Запись параметра сенсоров в командах sensor_N_ где N от 0 до SENSOR_MAX
uint8 ICACHE_FLASH_ATTR sensorSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
#if SENSOR_MAX >9
	#error"SENSOR_MAX more then 9, need rewrite function sensorSet"
#else
	uint8 Ret = 0;
	uint8 i = ParceNumObject(pcmd, SENSOR_MAX, NULL, &Ret);

	if (i <= SENSOR_MAX){
		idObject = i;													//индекс сенсора
		pcmd += Ret;
		Ret += parseHttpSetVar(HttpSensorN, SizeArray(HttpSensorN), web_con, pcmd, pvar);
	}
	return Ret;
#endif
}
//Чтение параметра сенсоров в командах sensor_N_ где N от 0 до SENSOR_MAX
uint8 ICACHE_FLASH_ATTR sensorGet(WEB_SRV_CONN* web_conn, uint8* cstr){
#if SENSOR_MAX >9
	#error"SENSOR_MAX more then 9, need rewrite function sensorSet"
#else
	uint8 Ret = 0;
	uint8 i = ParceNumObject(cstr, SENSOR_MAX, NULL, &Ret);

	if (i <= SENSOR_MAX){
		idObject = i;													//индекс сенсора
		cstr += Ret;
		Ret += parseHttpGetVar(HttpSensorN, SizeArray(HttpSensorN), web_conn, cstr);
	}
	return Ret;
#endif
}

uint8 ICACHE_FLASH_ATTR sensNnameSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	uint8 len = strlen(pvar);
	if (len>SENSOR_LEN_NAME) len=SENSOR_LEN_NAME;						//Это на будущее-вдруг поменю длину имени датчика
	os_memcpy(&Sensors[idObject].Name, pvar, len);
	return 0;
}
uint8 ICACHE_FLASH_ATTR sensNnameGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	uint8 str[SENSOR_LEN_NAME+1];
	os_memcpy(str, Sensors[idObject].Name, SENSOR_LEN_NAME);
	str[SENSOR_LEN_NAME] = 0;
	tcp_puts("%s", str);
	return 0;
}
uint8 ICACHE_FLASH_ATTR sensNadrSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	Sensors[idObject].Adr = ((uint8) rom_atoi(pvar)<<1)&0xfe;				//Адрес датчика сдвинут на 1 бит влево, см. объявление Sensors
	return 0;
}
uint8 ICACHE_FLASH_ATTR sensNadrGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%d", (Sensors[idObject].Adr>>1)&0x0f );					//Адрес датчика сдвинут на 1 бит влево, см. объявление Sensors
	return 0;
}
uint8 ICACHE_FLASH_ATTR sensNonSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	if (pvar[0]=='1')
		SensorShowOn(Sensors[idObject]);
	else
		SensorShowOff(Sensors[idObject]);
	return 0;
}
uint8 ICACHE_FLASH_ATTR sensNonGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	if (Sensors[idObject].State & (1<<SENSOR_SHOW_ON))
		tcp_puts("1");
	else
		tcp_puts("0");
	return 0;
}
//Запустть ожидание или проверку датчика
uint8 ICACHE_FLASH_ATTR sensNtest(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ClockUartTx(ClkTest(CLK_SENS), (uint8 *)&(Sensors[idObject].Adr), sizeof(Sensors[idObject].Adr));
	return 0;
}
//Запись значения сенсора в часы
uint8 ICACHE_FLASH_ATTR sensNsave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	uint8 cmd[sizeof(Sensors[0])+1];
	uint8* cmd_ptr = cmd;
	os_memcpy((void*)(cmd_ptr+1), (void *)&Sensors[idObject], sizeof(Sensors[idObject]));		//Формируется команда записи датчика плюс его индекс
	cmd[0] = idObject;
	ClockUartTx(ClkWrite(CLK_SENS), cmd, sizeof(cmd));
	return 0;
}
//Чтение данных датчика
uint8 ICACHE_FLASH_ATTR sensNdataGet(WEB_SRV_CONN* web_conn, uint8* cstr){

	if SensorIsNo(Sensors[idObject]){
		tcp_puts(SENS_NO_SENS);											//Нет датчика на шине
	}
	else if SensorBatareyIsLow(Sensors[idObject]){						//батарея датчика разряжена
		tcp_puts(SENS_BAT_LOW);
	}
	else if SensorRFIsNo(Sensors[idObject]){							//Нет сообщений от радиодатчика
		tcp_puts(SENS_NO_RADIO);
	}
	else{																//Есть какие-то результаты измерений
		if SensorIsPress(Sensors[idObject]){							//Это датчик давления
			tcp_puts("%d", PressureNormal(Sensors[idObject].Value));
		}
		else{															//Датчик температуры
			if (Sensors[idObject].Value & 0x80){						//Отрицательное значение
				uint8 sensVal = Sensors[idObject].Value;
				sensVal = ~sensVal;
				tcp_puts("-%d", sensVal);
			}
			else{
				tcp_puts("%d", Sensors[idObject].Value);
			}
		}
	}
	return 0;
}

//HTML ============================= "alarms_"
//Запись параметра будильников в командах alarms_N_ где N от 0 до ALARM_MAX
uint8 ICACHE_FLASH_ATTR alarmsSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
#if ALARM_MAX >9
	#error"ALARM_MAX more then 9, need rewrite function alarmsSet"
#else
	uint8 Ret = 0;
	uint8 i = ParceNumObject(pcmd, ALARM_MAX, NULL, &Ret);

	if (i <= ALARM_MAX){
		idObject = i;													//индекс будильника
		pcmd += Ret;
		if ((pcmd[0] <'1') || (pcmd[0]>'7') || (strlen(pcmd)!=1)){		//Не индекс дня недели
			Ret += parseHttpSetVar(HttpAlarmN, SizeArray(HttpAlarmN), web_con, pcmd, pvar);
		}
		else{
			Ret += 1;
			if (pvar[0]=='1')
				Alarms[idObject].EnableDayRus |= (1<<(pcmd[0]&0x0f));
			else
				Alarms[idObject].EnableDayRus &= ~(1<<(pcmd[0]&0x0f));
		}
	}																		//Другие команды alarm_ обрабатываются как дочки дальше
	return Ret;
#endif
}

//Чтение параметров будильников в командах alarms_N_ где N от 0 до ALARM_MAX
uint8 ICACHE_FLASH_ATTR alarmsGet(WEB_SRV_CONN* web_conn, uint8* cstr){
#if ALARM_MAX >9
	#error "ALARM_MAX more then 9, need rewrite function alarmsGet"
#else
	uint8 Ret = 0;
	uint8 i = ParceNumObject(cstr, ALARM_MAX, NULL, &Ret);

	if (i <= ALARM_MAX){
		idObject = i;													//индекс будильника
		cstr += 2;
		Ret = 2;
		if ((cstr[0] <'1') || (cstr[0]>'7') || (strlen(cstr)!=1)){		//Не индекс дня недели
			Ret += parseHttpGetVar(HttpAlarmN, SizeArray(HttpAlarmN), web_conn, cstr);//Вместо данных о соединении NULL, т.к. не используется дальше
		}
		else{															//День недели
			Ret++;
			if (Alarms[idObject].EnableDayRus & (1<<(cstr[0] & 0x0f)))
				tcp_puts("1");
			else
				tcp_puts("0");
		}
	}																	//Другие команды alarm_ обрабатываются как дочки дальше
	return Ret;
#endif
}

//Часы будильника
uint8 ICACHE_FLASH_ATTR alarmNhourSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	Alarms[idObject].Clock.Hour = (uint8)hextoul(pvar);
	return 0;
}
uint8 ICACHE_FLASH_ATTR alarmNhourGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%02x", Alarms[idObject].Clock.Hour);
	return 0;
}
//Минуты будильника
uint8 ICACHE_FLASH_ATTR alarmNminuteSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	Alarms[idObject].Clock.Minute = (uint8)hextoul(pvar);
	return 0;
}
uint8 ICACHE_FLASH_ATTR alarmNminuteGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%02x", Alarms[idObject].Clock.Minute);
	return 0;
}
//Дата будильника
uint8 ICACHE_FLASH_ATTR alarmNdateSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	strtodate(pvar, &Alarms[idObject].Clock);
	return 0;
}
uint8 ICACHE_FLASH_ATTR alarmNdateGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%02x", Alarms[idObject].Clock.Date);
	return 0;
}
//Месяц будильника
uint8 ICACHE_FLASH_ATTR alarmNmonthGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%02x", Alarms[idObject].Clock.Month);
	return 0;
}
//Год будильника
uint8 ICACHE_FLASH_ATTR alarmNyearGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%02x", Alarms[idObject].Clock.Year);
	return 0;
}
//Длительность звучания будильника
uint8 ICACHE_FLASH_ATTR alarmNduratSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	Alarms[idObject].Duration = ALARM_DURAT_MASK & (uint8)rom_atoi(pvar);
	return 0;
}
uint8 ICACHE_FLASH_ATTR alarmNduratGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%d", Alarms[idObject].Duration & ALARM_DURAT_MASK);
	return 0;
}
//Включение выключение будильника
uint8 ICACHE_FLASH_ATTR alarmN_OnSet(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	if (pvar[0]=='1')
		Alarms[idObject].EnableDayRus |= (1<<ALARM_ON_BIT);
	else
		Alarms[idObject].EnableDayRus &= ~(1<<ALARM_ON_BIT);
	return 0;
}
uint8 ICACHE_FLASH_ATTR alarmN_OnGet(WEB_SRV_CONN* web_conn, uint8* cstr){
	if (Alarms[idObject].EnableDayRus & (1<<ALARM_ON_BIT))
		tcp_puts("1");
	else
		tcp_puts("0");
	return 0;
}
//Запись будильника в часы
uint8 ICACHE_FLASH_ATTR alarmNSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ClockUartTx(ClkWrite(CLK_ALARM), (uint8 *) &Alarms[idObject], sizeof(Alarms[idObject]));	//Установить время
	return 0;
}

//HTML ============================= "ping_"
//Результат пинга принят
void ICACHE_FLASH_ATTR ping_recv(void* arg, void *pdata){
	struct ping_resp *resp = (struct ping_resp*)pdata;
	if (pingState == ERR_VAL){
		pingState = ERR_OK;
	}
	if (resp->ping_err == ERR_OK){
		pingState++;
	}
	else{
		pingState = resp->ping_err;
	}
}
//Все пинги переданы
void ICACHE_FLASH_ATTR ping_send(void* arg, void *pdata){
	//struct ping_resp *resp = (struct ping_resp*)pdata;
	if (!(pingopt == NULL)){
		os_free(pingopt);
		pingopt = NULL;
	}
}

//Послать ping на внешний сервер времени
uint8 ICACHE_FLASH_ATTR pingStart(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
#ifdef USE_PING
	pingState = ERR_VAL;
	if (pingopt == NULL)											//Хапнуть память для параметров пинга
		pingopt = (struct ping_option*)os_zalloc(sizeof(struct ping_option));
	pingopt->ip = ipaddr_addr(pvar);
	pingopt->count = PING_COUNT;
	pingopt->recv_function = ping_recv;
	pingopt->sent_function = ping_send;
	if (!ping_start(pingopt)){
		pingState = ERR_VAL;
		if (!(pingopt == NULL)){
			os_free(pingopt);
			pingopt = NULL;
		}
	}
#endif
	return 0;
}

//HTML ============================= "ntp_"
// Записать ip адрес внешнего ntp сервера
uint8 ICACHE_FLASH_ATTR ntpCfgSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	flash_save_cfg(&ntp_srv_ip.ip.addr, ID_NTP_IP, sizeof(ntp_srv_ip.ip.addr));
	flash_save_cfg(&NtpPeriod, ID_NTP_PERIOD, sizeof(NtpPeriod));
	return 0;
}
// Запросить время с внешнего ntp сервера
uint8 ICACHE_FLASH_ATTR ntpClientGetTime(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	if (ntpGetSeconds == NULL)
		ntpGetSeconds = SetWatchInClock;
	ntpStart();
	return 0;
}
//Записать значение nat.ip
uint8 ICACHE_FLASH_ATTR ntpSetIP(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ntp_srv_ip.ip.addr = ipaddr_addr(pvar);
	return 0;
}
//Прочитать значение nat.ip
uint8 ICACHE_FLASH_ATTR ntpGetIP(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts(IPSTR, IP2STR(&ntp_srv_ip.ip.addr));
	return 0;
}


//HTML ============================= "watch_"
//День, месяц, год
uint8 ICACHE_FLASH_ATTR watchSetDate(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	strtodate(pvar, &Watch);
	return 0;
}

//Записать значение счетчиков в часы и тайм-зону во флаш
uint8 ICACHE_FLASH_ATTR watchSave(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	ClockUartTx(ClkWrite(CLK_WATCH), (uint8 *) &Watch, sizeof(Watch));	//Установить время
	flash_save_cfg(&ClockZone, ID_CLOCK_ZONE, sizeof(ClockZone));
	return 0;
}
//День недели
uint8 ICACHE_FLASH_ATTR watchGetDay(WEB_SRV_CONN* web_conn, uint8* cstr){
	tcp_puts("%02x", what_day(Watch.Date, Watch.Month, Watch.Year));
	return 0;
}

//Записать смещение временной зоны
uint8 ICACHE_FLASH_ATTR watchSetZone(WEB_SRV_CONN* web_con, uint8 *pcmd, uint8 *pvar){
	uint8 i = (uint8)rom_atoi(pvar);
	if ((i>=0) && (i<=24))
		ClockZone = ((sint8)i)-12;										//Индекс в смещение часовой зоны
	return 0;
}

//Прочитать смещение временной зоны
uint8 ICACHE_FLASH_ATTR watchGetZone(WEB_SRV_CONN* web_conn, uint8* cstr){
	uint8 i=0;

	if ((ClockZone>=-12) && (ClockZone<=12))
		i = (uint8) (ClockZone+12);										//Смещение часовой щоны в индекс
	tcp_puts("%d", i);

	return 0;
}


//UART =============================  Переменные и команды uart -------------------
//Установить описание сенсора в модуле
uint8 ICACHE_FLASH_ATTR UartSensorSet(uint8 cmd, uint8* pval, uint8 valLen){
	uint8 Ret = 0;
	if ClkIsWrite(cmd){
		if (pval[0]<=SENSOR_MAX){
			u08 ptr[sizeof(struct sSensor)+1];					//Промежуточный буфер
			os_memcpy((void*)ptr, (void*)pval, valLen);			//Заполнить буфер
			u08 *p = ptr;										//Указатель на первый элемент
			p++;
			os_memcpy(&Sensors[pval[0]], (void*)p, valLen-1);
			Ret = 1;
		}
	}
	return Ret;
}

/*
 * Установить значение будиdльника в модуле
 */
uint8 ICACHE_FLASH_ATTR UartAlarmSet(uint8 cmd, uint8* pval, uint8 valLen){
	uint8 Ret = 0;
	if ClkIsWrite(cmd){
		if (pval[0]<=ALARM_MAX){
			os_memcpy(&Alarms[pval[0]], (void*)pval, valLen);
			Ret = 1;
		}
	}
#if DEBUGSOO==1
	os_printf_plus("read from alarms not support\n");
#endif
	return Ret;
}

/*
 * Установить значение громкости в модуле
 */
uint8 ICACHE_FLASH_ATTR UartVolumeSet(uint8 cmd, uint8* pval, uint8 valLen){
	uint8 Ret = 0;
	if ClkIsWrite(cmd){
		if (pval[0]<=VOL_TYPE_COUNT){
			os_memcpy(&VolumeClock[pval[0]], (void*)pval, valLen);
			Ret = 1;
		}
	}
	return Ret;
}
/*
 * Запуск запроса времени через ntp
 */
uint8 ICACHE_FLASH_ATTR UartNtpStart(uint8 cmd, uint8* pval, uint8 valLen){
	err_t ntpErr;

	if (ntpGetSeconds == NULL)
		ntpGetSeconds = SetWatchInClock;
	ntpErr = ntpStart();
	if (ntpErr != ERR_OK)
		ClockUartTx(ClkWrite(CLK_NTP_ERROR), (uint8 *) &ntpErr, sizeof(ntpErr));
	return 1;
}

ETSTimer weather_repeat_timet;							//Таймер повторного запроса погоды
void ICACHE_FLASH_ATTR  weatherRepeat(void){
#if DEBUGSOO>1
	os_printf("repeat wheather ok\n");
#endif
	os_timer_disarm(&weather_repeat_timet);
	weatherResFunc = NULL;
	weatherStart();
}
//Выввается по окончании процесса получения погоды
void ICACHE_FLASH_ATTR weatherEndProcess(err_t result){
#if DEBUGSOO>1
	os_printf("wheather end process res=%d cnt=%d\n", result, weatherSet.cnt);
#endif
	if (weatherResult.state == WEATHER_STOP){//На всякий случай
		if (result == ERR_OK){
			if (weatherSet.cnt == WE_TYPE_CURRENT){			//После запроса текущей погоды запрашивается прогноз
				weatherSet.cnt = WE_TYPE_FORECAST;
			}
			else{
				os_timer_disarm(&weather_repeat_timet);		//Остановить таймер
#if DEBUGSOO>1
	os_printf("wheather STOP\n");
#endif
				weatherResFunc = NULL;
				return;										//Все запрошено
			}
			weatherResult.state = WEATHER_IN_PROGRESS;			//Процесс еще не завершен
			os_timer_disarm(&weather_repeat_timet);				//Повторный запрос
			os_timer_setfn(&weather_repeat_timet, (os_timer_func_t *) weatherRepeat, NULL);
			ets_timer_arm_new(&weather_repeat_timet, 1000, OS_TIMER_SIMPLE, OS_TIMER_MS);
		}
		else {
#if DEBUGSOO>1
	os_printf("wheather end process ERR\n");
#endif
			weatherResFunc = NULL;
			os_timer_disarm(&weather_repeat_timet);		//Остановить таймер
		}
	}
	else{
#if DEBUGSOO>1
	os_printf("NO STOP\n");
#endif
		weatherResFunc = NULL;
		os_timer_disarm(&weather_repeat_timet);		//Остановить таймер
	}
}
/*
 * Использется как односекундный таймер
 */
uint8 ICACHE_FLASH_ATTR UartWatchReciv(uint8 cmd, uint8* pval, uint8 valLen){
	if (Watch.Month){									//Если значение месяца не 0, то в структуре часов какое-то законное время
		if ((Watch.Hour == NtpPeriod.Hour) && (Watch.Minute == 0) && (Watch.Second == 0)){	//Проверка необходимости запроса времени у внешнего NTP сервера Час совпадает
			switch (NtpPeriod.PeriodType){
				case NTP_PERIOD_ONCE_DAY:				//Раз в день
					ntpClientGetTime(NULL, NULL, NULL);
					break;
				case NTP_PERIOD_ONCE_WEEK:				//Раз в неделю
					if (NtpPeriod.Value == what_day(Watch.Date, Watch.Month, Watch.Year)){
						ntpClientGetTime(NULL, NULL, NULL);
					}
					break;
				case NTP_PERIOD_ONCE_MONTH:				//Раз в месяц
					if (NtpPeriod.Value == Watch.Date){
						ntpClientGetTime(NULL, NULL, NULL);
					}
					break;
				default:
					break;
			};
		}
		if (os_strlen(weatherSet.APIkey)){				//Запрос погоды если настроено
			if ((Watch.Second == weatherSet.second) && (Watch.Minute == 0)){
				if (weatherResult.state == WEATHER_STOP){
					weatherSet.cnt = WE_TYPE_CURRENT;
					weatherResFunc = &weatherEndProcess;	//По окончании запросить прогноз
					weatherStart();
				}
			}
		}
	}
	return 1;
}

//Запускается каждую секунду
void ICACHE_FLASH_ATTR OneSecundFunc(void){
}

/*
 * Пароль домашней сети для режима station
 */
uint8 ICACHE_FLASH_ATTR UartStAuth(uint8 cmd, uint8* pval, uint8 valLen){
	struct station_config wicfgs;
	uint8 Ret = 0;

	if ClkIsWrite(cmd){
		if (strlen(pval)){
			Read_WiFi_config(&wificonfig, 0xffffffff); 	//Прочитать конфигурацию wifi

			uint8 *pas, *netName;
			uint8 netNameLen = strlen(pval)+1, pasLen, change = 0;

			netName = (uint8 *)os_zalloc(netNameLen+1);	//Извлечение имени сети
			os_memset(netName, 0, netNameLen);
			os_memcpy(netName, pval, netNameLen);
			pval += netNameLen;							//Пропустить имя сети
			pasLen = strlen(pval)+1;					//Извлечение пароля
			pas = (uint8 *) os_zalloc(pasLen);
			os_memset(pas, 0, pasLen);
			os_memcpy(pas, pval, pasLen);
			if (os_strcmp(pas, wificonfig.st.config.ssid)){	//Старое и новое имя сети не совпадают, меняем
				os_memset(wificonfig.st.config.ssid, 0, sizeof(wificonfig.st.config.ssid));//Память очищается
				os_sprintf_fd(wificonfig.st.config.ssid, "%s", netName);
				change++;
			}
			if (os_strcmp(pas, wificonfig.st.config.password)){	//Старый и новый пароли не совпадают, меняем
				os_memset(wificonfig.st.config.password, 0, sizeof(wificonfig.st.config.password));//Память очищается
				os_sprintf_fd(wificonfig.st.config.password, "%s", pas);
				change++;
			}
			os_free(pas);
			os_free(netName);
			if (change)
				New_WiFi_config(0x17E00);			//Запись новой конфигурации и рестарт wifi
		}
	}
	else if ClkIsRead(cmd){
		if(wifi_station_get_config(&wicfgs)) {
			uint8 *ResStr;
			uint8 Len = os_strlen(wicfgs.password)+os_strlen(wicfgs.ssid)+2;
			ResStr = (uint8*)os_zalloc(Len);
			os_memset(ResStr, 0, Len);
			os_memcpy(ResStr, wicfgs.ssid, os_strlen(wicfgs.ssid));
			os_memcpy(ResStr+os_strlen(wicfgs.ssid)+1, wicfgs.password, os_strlen(wicfgs.password));
			ClockUartTx(ClkWrite(CLK_ST_WIFI_AUTH), ResStr, Len);
			os_free(ResStr);
		}
	}
	return Ret;
}

/*
 * IP адрес staion
 */
uint8 ICACHE_FLASH_ATTR UartStIP(uint8 cmd, uint8* pval, uint8 valLen){
	#define IP_LEN_STR (3*4+5)						//четыре октета по три знака плюс точки и нулевой байт

	uint8 Ret = 0, ip_str[IP_LEN_STR];
    struct ip_info wifi_info;

    if (ClkIsRead(cmd)){
		IP4_ADDR(&wifi_info.ip, 0,0,0,0);
		if (wifi_station_get_connect_status() == STATION_GOT_IP){ //IP адрес получен
			wifi_get_ip_info(STATION_IF, &wifi_info);
		}
		os_sprintf_fd(ip_str, IPSTR, IP2STR(&wifi_info.ip));
		ClockUartTx(ClkWrite(CLK_ST_WIFI_IP), ip_str, strlen(ip_str)+1);
    }
	return Ret;
}

/*
 * Формирует описание погоды на заданную дату и час
 */
void ICACHE_FLASH_ATTR WeatherGetFromDate(struct sClockValue *needTime, char *text, char *prefix){
	uint8 i=WE_TYPE_CURRENT+1;
	struct sClockValue time;

	for (;i<WE_TYPE_FORECAST;i++){
		if (UnixTimeToDateTime(weatherResult.weathers[i].Time, &time, ClockZone)){
			if ((time.Date == needTime->Date)
				 && (time.Month == needTime->Month)
				 && (time.Year == needTime->Year)
				 && (BCDtoInt(time.Hour) >= BCDtoInt((needTime->Hour)))){
				text += os_strlen(text);
				os_sprintf_fd(text, "%s t %d°C %s.", prefix, weatherResult.weathers[i].Temperature, weatherResult.weathers[i].DescriptUTF8);
#if DEBUGSOO>1
					os_printf("find weather ok\n");
#endif
				break;
			}
		}
	}
}

err_t TemperatureSensorIsHave(void){
	uint8 i=0;
	for(;i<SENSOR_MAX;i++){
		if (!SensorIsPress(Sensors[i]) && SensorIsSet(Sensors[i])){
			return ERR_OK;
		}
	}
	return ERR_VAL;
}

/*
 * Текст для вывода вместе с датой или по команде
 */
uint8 ICACHE_FLASH_ATTR UartTextCustom(uint8 cmd, uint8* pval, uint8 valLen){
	uint8 Ret = 0;

	if (ClkIsRead(cmd)){
		if (flash_read_cfg(TextCustom, ID_TXT_CUSTOM, TEXT_CUST_MAX)<0){
			os_strcpy(TextCustom, TEXT_CUST_DEFAULT);
		}
		//Погода выводится только если не определена произвольная строка
		if (os_strlen(TextCustom) == 0 || ((os_strlen(TextCustom)==1) && (TextCustom[0] == ' '))){

			char *text = (char *)TextCustom;

			if (weatherResult.state == WEATHER_STOP){
				if ((weatherResult.weathers[WE_TYPE_CURRENT].Time) && (weatherResult.weathers[WE_TYPE_CURRENT+1].Time)){
					if (TemperatureSensorIsHave() != ERR_OK){	//Текущая температура и погода выводится только если нет датчика температуры
						struct sClockValue currTime;
						if (UnixTimeToDateTime(weatherResult.weathers[WE_TYPE_CURRENT].Time, &currTime, ClockZone)){
							text += os_strlen(text);
							os_sprintf_fd(text, "на %02x:%02x", currTime.Hour, currTime.Minute);
						}
						text += os_strlen(text);
						os_sprintf_fd(text, " t %d°C %s.", weatherResult.weathers[WE_TYPE_CURRENT].Temperature, weatherResult.weathers[WE_TYPE_CURRENT].DescriptUTF8);
					}

					uint8 hour = BCDtoInt(Watch.Hour);
					struct sClockValue needDate = Watch;
					if(	((hour >=0) && (hour <6))		//Ночь, прогноз на следующее утро (6 часов) и день (15 часов)
						|| (hour >=18)){
						nextDate(&needDate);
						needDate.Hour = 0x06;
						WeatherGetFromDate(&needDate, text, " утром");
						needDate.Hour = 0x15;
						WeatherGetFromDate(&needDate, text, " днем");
					}
					else if ((hour >=6) && (hour <12)){//Утро, прогноз на день (15) и вечер (18)
						needDate.Hour = 0x15;
						WeatherGetFromDate(&needDate, text, " днем");
						needDate.Hour = 0x18;
						WeatherGetFromDate(&needDate, text, " вечером");
					}
					else if((hour >=12) && (hour <18)){//День, прогноз на вечер (21) и следующее утро(6)
						needDate.Hour = 0x21;
						WeatherGetFromDate(&needDate, text, " вечером");
						nextDate(&needDate);
						needDate.Hour = 0x06;
						WeatherGetFromDate(&needDate, text, " утром");
					}
				}
				else{									//Похоже погоду еще не запрашвал, запрашиваем
					if (os_strlen(weatherSet.APIkey)){
						weatherSet.cnt = WE_TYPE_CURRENT;
						weatherResFunc = &weatherEndProcess;	//По окончании запросить прогноз
						weatherStart();
						os_strcpy(text, "старт загрузки прогноза ");
					}
				}
			}
			else{
				os_strcpy(text, "прогноз загружается ");
			}
			UTF8toWin1251Cyr(TextCustom);
			ClockUartTx(ClkWrite(CLK_CUSTOM_TXT), TextCustom, TEXT_CUST_MAX);
			*TextCustom = 0;
		}
		else{
			ClockUartTx(ClkWrite(CLK_CUSTOM_TXT), TextCustom, TEXT_CUST_MAX);
		}
	}
	return Ret;
}

// инициализация
void ICACHE_FLASH_ATTR ClockWebInit(void){

	uint8 i = 0;

	//Имя часов в вебморде
	if (flash_read_cfg(ClockName, ID_CLOCK_NAME, CLOCK_NAME_MAX)<0){
		os_strcpy(ClockName, CLOCK_NAME_DAFAULT);
	}
	//Временная зона
	if (flash_read_cfg(&ClockZone, ID_CLOCK_ZONE, sizeof(ClockZone)) != sizeof(ClockZone)){
		ClockZone = CLOCK_ZONE_DEFAULT;
	}
	//Адрес сервера NTP
	if (flash_read_cfg(&ntp_srv_ip.ip.addr, ID_NTP_IP, sizeof(ntp_srv_ip.ip.addr)) != sizeof(ntp_srv_ip.ip.addr)){
		ntp_srv_ip.ip.addr = NTP_IP_SRV_DEFAULT;
	}
	//Период запроса времени из сервера NTP
	if (flash_read_cfg(&NtpPeriod, ID_NTP_PERIOD, sizeof(NtpPeriod)) != sizeof(NtpPeriod)){
		NtpPeriod.Hour = NTP_PERIOD_HOUR_DEF;
		NtpPeriod.PeriodType = NTP_PERIOD_TYPE_DEF;
		NtpPeriod.Value = NTP_PERIOD_VALUE_DEF;
	}
	//Произвольный текст пользователя
	if (flash_read_cfg(TextCustom, ID_TXT_CUSTOM, TEXT_CUST_MAX)<0){
		os_strcpy(TextCustom, TEXT_CUST_DEFAULT);
	}

	//Клиент погоды
	if (flash_read_cfg(&weatherSet, ID_WEATHER, sizeof(weatherSet)) != sizeof(weatherSet)){
		ets_memset(weatherSet.APIkey, 0, wAPIkeyLen);
		weatherSet.city = 0;
		ets_memset(weatherSet.lon, 0, wLonLatLen);
		ets_memset(weatherSet.lat, 0, wLonLatLen);
		weatherSet.cnt = WEATHER_MAX;
	}

	weatherResult.state = WEATHER_STOP;
	for(i=0;i<WEATHER_MAX;i++){
		weatherResult.weathers[i].Time = 0;
	}

	for (i=0; i < ALARM_MAX-1; ++i) {		//Присвоить номера будильникам
		Alarms[i].Id = i;
	}
	for (i=0;i<SENSOR_MAX;i++){
		SensorNoInBus(Sensors[i]);			//Датчика на шине нет
	}
	for (i=0;i<VOL_TYPE_COUNT;i++){
		VolumeClock[i].id = i;
	}
	Watch.Second = 0;
	Watch.Minute = 0;
	Watch.Hour = 0;
	Watch.Date = 0;							//Нулевая дата как признак неправильного времени
	Watch.Month = 0;
	Watch.Year = 0;
	ClockUartTx(ClkWrite(CLK_ALL), &i, 1);	//Запросить переменные у часов
}
