//use code page windovs-1251 because it is used to program the microcontroller device hours
var IL = "ru";//Interface Language, or "en"
var captionRU = {
	title:"матричные часы",
	//-- index
	mpMenuStart:"Старт",
	mpMenuWatch:"Часы",
	mpMenuAlarm:"Будильники",
	mpMenuSensors:"Датчики",
	mpMenuNTPServer:"Сервер времени",
	mpMenuParams:"Настройки",
	mpMenuWiFiClient:"Клиент WiFi",
	mpMenuWiFiServer:"Сервер WiFi",
	mpMenuRestart:"Перезагрузка",
	mpMenuRestore:"Восстановление",

	//-- start
	mainCurrentTime:"Текущее время и дата",
	//-- clock
	clkSetTime:"Установить время",
	clkSetDate:"Текущая дата",
	clkTimeZone:"Часовой пояс",
	clkChime:"Куранты",
	clkVol:"Громкость (от 1 до 64)",
	//--alarm
	alrmNumber:"Номер",
	alrmTime:"Время",
	alrmMon:"пн",
	alrmTue:"вт",
	alrmWed:"ср",
	alrmThu:"чт",
	alrmFri:"пт",
	alrmSat:"сб",
	alrmSun:"вс",
	alrmDurat:"Длит.",
	alrmStat:"Состояние",
	alrmByDate:"По дате (год игнорируется)",
	alrmWeekly:"По дням недели",
	//--sensor
	sensNumber:"Положение в строке",
	sensName:"Имя",
	sensAdr:"Адрес на шине",
	sensState:"Состояние",
	sensCurrent:"Текущее значение",
	sensTransc:"Сокращения состояния датчиков",
	sensNSNote:"датчик в устройсте не найден",
	sensBLNote:"батарея датчика разряжена",
	ensNRNote:"радиодатчик не отвечает",
	//-- ntp
	ntpCaption:"Внешний сервер времени (протокол TIME)",
	ntpIPadr:"IP адрес внешнего ntp сервера",
	ntpPortNum:"Порт внешнего ntp сервера",
	ntpPeriodCaption:"Период запроса времени NTP",
	ntpPeriodTime:"Запрашивать время в",
	ntpPeriodType:"один раз в",
	ntpPeriodTypeDay:"сутки",
	ntpPeriodTypeWeek:"неделю",
	ntpPeriodTypeMonth:"месяц",
	//-- param
	prmAlarm:"Сигнал будильника",
	prmAlarmVol:"Громкость (от 1 до 64)",
	prmAlarmVolTune:"Регулировка громкости",
	prmAlarmVolConst:"Постоянная",
	prmAlarmVolToLevel:"Нарастающая до указанного уровня",
	prmAlarmVolToMax:"Нарастающая до максимального",
	prmKey:"Звук кнопок",
	prmKeyVol:"Громкость (от 1 до 64)",
	prmKeySound:"Звук нажатия",
	prmFont:"Шрифт цифр",
	prmFontDigit:"Вид цифр",
	prmFontUsual:"Обычный",
	prmFontNarrow:"Прямоугольный узкий",
	prmFontWideSerif:"Прямоугольный широкий с засечками",
	prmFontWide:"Прямоугольный широкий",
	prmFontRect:"Прямоугольный стильный",
	prmFontSmoothWide:"Гладкий широкий",
	prmFontSmoothNarrow:"Гладкий узкий",
	prmClockName:"Имя часов",
	prmCusText:"Дополнительный текст",
	//-- wifi station
	stNetName:"Имя сети",
	stPass:"Пароль",
	stUseBSS:"Использовать BSSID",
	stState:"Состояние",
	stIdle:"не занят",
	stConnecting:"подключение",
	stWrongPass:"неверный пароль",
	stNoAP:"нет точки доступа",
	stFailConn:"сбой подключения",
	stGotIP:"получен ip",
	stOff:"выключено",
	stIP:"Адрес IP",
	stGate:"Шлюз",
	stMask:"Маска сети",
	stAutoconn:"Автоподключение",
	stMAC:"МАС адрес",
	stDHCPon:"Автоматический адрес (DHCP)",
	//-- AP
	apMode:"Режим WiFi",
	apSTMode:"Только клиент",
	apAPOnly:"Только сервер",
	apBoth:"И сервер и клиент",
	apSSID:"Имя сети и имя пользователя",
	apNameHide:"Скрыть имя сети",
	apNetPass:"Пароль сети и пароль пользователя",
	apChanal:"Номер канала",
	apAutoChanal:"Автоматический",
	apModePHY:"Стандарт WiFi",
	apAuth:"Шифрование",
	apIP:"IP адрес",
	apMask:"Маска сети",
	apGate:"Шлюз",
	apMAC:"MAC адрес",
	apDHCP:"Cервер DHCP",
	apStartIP:"Начальный адрес IP",
	EndIP:"Конечный адрес IP",
	apBeacon:"Период синхронизации станций (ms)",
	apSleepMode:"Режим сна",
	apNotSleep:"Не использовать",
	apSleepLight:"Облегченный",
	apSleepModem:"Модем",
	apMaxCon:"Максимальных подключений",
	//-- restart
	restTitle:"Перезапуск устройства",
	restStr1:"Важное замечание:",
	restStr2:"Перезапуск рекомендуется после завершения всех настроек.<br>Перезапуск прервет соединение на небольшое время. <br>Точно перезагрузить устройство?",
	//-- reset
	rstTitle:"Возврат к заводским настройкам",
	rstStr1:"Важное замечание:",
	rstStr2:'После возврата к заводским настройкам, все настройки пользователей будут удалены.<br>Вы можете изменить их на http://192.168.4.1.После возврата имя пользователя будет "ESP8266" пароль "0123456789"<br>Сбросить настройки на заводские?',
	//-- success
	sucTitle:"Перезагрузка завершена!",
	sucStr1:"Обновите страницу или подключите устройство WIFI к сети, а затем войдите в интерфейс конфигурации."

}
var captionMultiRU = {
	mainOptOff:"Выключено",
	mainOptOn:"Включено"
}

var captionEN = {
	title:"clock matrix",
	//-- index
	mpMenuStart:"Start",
	mpMenuWatch:"Watch",
	mpMenuAlarm:"Alarms",
	mpMenuSensors:"Sensors",
	mpMenuNTPServer:"TIME server",
	mpMenuParams:"Setting",
	mpMenuWiFiClient:"WiFi Station",
	mpMenuWiFiServer:"WiFi AP",
	mpMenuRestart:"Restart",
	mpMenuRestore:"Restore",
	//-- start
	mainCurrentTime:"Now",
	//-- clock
	clkSetTime:"Time set",
	clkSetDate:"Date",
	clkTimeZone:"Time zone",
	clkChime:"Chime",
	clkVol:"Volume (from 1 to 64)",
	//-- alarm
	alrmWeekly:"Weekly",
	alrmNumber:"Number",
	alrmTime:"Time",
	alrmMon:"Mo",
	alrmTue:"Tu",
	alrmWed:"We",
	alrmThu:"Th",
	alrmFri:"Fr",
	alrmSat:"Sa",
	alrmSun:"Su",
	alrmDurat:"Durat.",
	alrmStat:"State",
	alrmByDate:"By date (year ignored)",
	//-sensor
	sensNumber:"Position in the line",
	sensName:"Title",
	sensAdr:"Address bus",
	sensState:"State",
	sensCurrent:"Current value",
	sensTransc:"Reducing state sensors",
	sensNSNote:"sensor in the device is not found",
	sensBLNote:"sensor battery is low",
	ensNRNote:"radio sensor does not respond",
	//-- ntp
	ntpCaption:"External ntp server  (protocol TIME)",
	ntpIPadr:"IP address of the external ntp server",
	ntpPortNum:"Port external ntp server",
	ntpPeriodCaption:"The period of time a query NTP",
	ntpPeriodTime:"Prompt time",
	ntpPeriodType:"once a",
	ntpPeriodTypeDay:"day",
	ntpPeriodTypeWeek:"week",
	ntpPeriodTypeMonth:"month",
	//-- param
	prmAlarm:"The alarm",
	prmAlarmVol:"Volume (1 to 64)",
	prmAlarmVolTune:"Adjusting the volume",
	prmAlarmVolConst:"Constant",
	prmAlarmVolToLevel:"Increasing to a specified level",
	prmAlarmVolToMax:"Increasing to a maximum level",
	prmKey:"Key sound",
	prmKeyVol:"Volume (1 to 64)",
	prmKeySound:"Click sound",
	prmFont:"Font of numbers",
	prmFontDigit:"Type numbers",
	prmFontUsual:"Normal",
	prmFontNarrow:"Narrow rectangular",
	prmFontWideSerif:"Rectangular narrow serif",
	prmFontWide:"Rectangular wide",
	prmFontRect:"Rectangular",
	prmFontSmoothWide:"Smooth wide",
	prmFontSmoothNarrow:"Smooth narrow",
	prmClockName:"Clock name",
	prmCusText:"Adding text",
	//-- wifi station
	stNetName:"SSID",
	stPass:"Password",
	stUseBSS:"Use BSSID",
	stState:"Status",
	stIdle:"IDLE",
	stConnecting:"CONNECTING",
	stWrongPass:"WRONG_PASSWORD",
	stNoAP:"NO_AP_FOUND",
	stFailConn:"CONNECT_FAIL",
	stGotIP:"GOT_IP",
	stOff:"OFF",
	stIP:"IP",
	stGate:"Gateway",
	stMask:"Subnet mask",
	stAutoconn:"AutoConnect",
	stMAC:"МАС",
	stDHCPon:"DHCP",
	//-- AP
	apMode:"WiFi Mode:",
	apSTMode:"STATION_MODE",
	apAPOnly:"SOFTAP_MODE",
	apBoth:"STATIONAP_MODE",
	apSSID:"AP SSID and user name",
	apNameHide:"Hidden SSID",
	apNetPass:"AP and user password:",
	apChanal:"Channel",
	apAutoChanal:"auto",
	apModePHY:"IEEE PHY",
	apAuth:"Auth mode",
	apIP:"IP",
	apMask:"Subnet mask",
	apGate:"Gateway",
	apMAC:"MAC",
	apDHCP:"DHCP",
	apStartIP:"Start IP",
	EndIP:"End IP",
	apBeacon:"Beacon (ms)",
	apSleepMode:"Sleep Mode",
	apNotSleep:"NONE",
	apSleepLight:"LIGHT",
	apSleepModem:"MODEM",
	apMaxCon:"Max connections",
	//-- restart
	restTitle:"Restart Device",
	restStr1:"Important notice:",
	restStr2:"After restart, you will need to re-login the configuration interface.It is recommended to restart after completing all configurations.<br />Restart will interrupt the network for a very short period, are you sure to restart now?",
	//-- reset
	rstTitle:"Restore Factory Setting",
	rstStr1:"Important notice:",
	rstStr2:"After restoring factory settings, all users configuration will be deleted. You can reconfigure it on http://192.168.4.1.<br>Account 'ESP8266' and password '0123456789'",
	//-- success
	sucTitle:"Rebooting Successful!",
	sucStr1:"You can choose to manually close the page or reconnect the WIFI module of network and then login to the configuration interface"
}

var captionMultiEN = {
	mainOptOff:"Off",
	mainOptOn:"On"
}

function letCaption(lang){
	if (lang){
		IL = lang;
	}
	else{
		if (parent){
			IL = parent.IL;
		}
	}
	for (var name in (IL==="en")?captionEN:captionRU){
		if (document.getElementById(name)){
			document.getElementById(name).innerHTML = (IL==="en")?captionEN[name]:captionRU[name];
		}
	}
	var opt = document.getElementsByTagName("option");
	if (opt){
		var capt = (IL==="en")?captionMultiEN:captionMultiRU;
		for (var i=0;i<opt.length; i++){
			ca = capt[opt.item(i).id];
			if (ca){
			 	opt.item(i).innerHTML = ca;
			}
		}
	}
}
