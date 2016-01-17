//use code page windovs-1251 because it is used to program the microcontroller device hours
var IL = "ru";//Interface Language, or "en"
var captionRU = {
	title:"��������� ����",
	//-- index
	mpMenuStart:"�����",
	mpMenuWatch:"����",
	mpMenuAlarm:"����������",
	mpMenuSensors:"�������",
	mpMenuNTPServer:"������ �������",
	mpMenuParams:"���������",
	mpMenuWiFiClient:"������ WiFi",
	mpMenuWiFiServer:"������ WiFi",
	mpMenuRestart:"������������",
	mpMenuRestore:"��������������",

	//-- start
	mainCurrentTime:"������� ����� � ����",
	//-- clock
	clkSetTime:"���������� �����",
	clkSetDate:"������� ����",
	clkTimeZone:"������� ����",
	clkChime:"�������",
	clkVol:"��������� (�� 1 �� 64)",
	//--alarm
	alrmNumber:"�����",
	alrmTime:"�����",
	alrmMon:"��",
	alrmTue:"��",
	alrmWed:"��",
	alrmThu:"��",
	alrmFri:"��",
	alrmSat:"��",
	alrmSun:"��",
	alrmDurat:"����.",
	alrmStat:"���������",
	alrmByDate:"�� ���� (��� ������������)",
	alrmWeekly:"�� ���� ������",
	//--sensor
	sensNumber:"��������� � ������",
	sensName:"���",
	sensAdr:"����� �� ����",
	sensState:"���������",
	sensCurrent:"������� ��������",
	sensTransc:"���������� ��������� ��������",
	sensNSNote:"������ � ��������� �� ������",
	sensBLNote:"������� ������� ���������",
	ensNRNote:"����������� �� ��������",
	//-- ntp
	ntpCaption:"������� ������ ������� (�������� TIME)",
	ntpIPadr:"IP ����� �������� ntp �������",
	ntpPortNum:"���� �������� ntp �������",
	ntpPeriodCaption:"������ ������� ������� NTP",
	ntpPeriodTime:"����������� ����� �",
	ntpPeriodType:"���� ��� �",
	ntpPeriodTypeDay:"�����",
	ntpPeriodTypeWeek:"������",
	ntpPeriodTypeMonth:"�����",
	//-- param
	prmAlarm:"������ ����������",
	prmAlarmVol:"��������� (�� 1 �� 64)",
	prmAlarmVolTune:"����������� ���������",
	prmAlarmVolConst:"����������",
	prmAlarmVolToLevel:"����������� �� ���������� ������",
	prmAlarmVolToMax:"����������� �� �������������",
	prmKey:"���� ������",
	prmKeyVol:"��������� (�� 1 �� 64)",
	prmKeySound:"���� �������",
	prmFont:"����� ����",
	prmFontDigit:"��� ����",
	prmFontUsual:"�������",
	prmFontNarrow:"������������� �����",
	prmFontWideSerif:"������������� ������� � ���������",
	prmFontWide:"������������� �������",
	prmFontRect:"������������� ��������",
	prmFontSmoothWide:"������� �������",
	prmFontSmoothNarrow:"������� �����",
	prmClockName:"��� �����",
	prmCusText:"�������������� �����",
	//-- wifi station
	stNetName:"��� ����",
	stPass:"������",
	stUseBSS:"������������ BSSID",
	stState:"���������",
	stIdle:"�� �����",
	stConnecting:"�����������",
	stWrongPass:"�������� ������",
	stNoAP:"��� ����� �������",
	stFailConn:"���� �����������",
	stGotIP:"������� ip",
	stOff:"���������",
	stIP:"����� IP",
	stGate:"����",
	stMask:"����� ����",
	stAutoconn:"���������������",
	stMAC:"��� �����",
	stDHCPon:"�������������� ����� (DHCP)",
	//-- AP
	apMode:"����� WiFi",
	apSTMode:"������ ������",
	apAPOnly:"������ ������",
	apBoth:"� ������ � ������",
	apSSID:"��� ���� � ��� ������������",
	apNameHide:"������ ��� ����",
	apNetPass:"������ ���� � ������ ������������",
	apChanal:"����� ������",
	apAutoChanal:"��������������",
	apModePHY:"�������� WiFi",
	apAuth:"����������",
	apIP:"IP �����",
	apMask:"����� ����",
	apGate:"����",
	apMAC:"MAC �����",
	apDHCP:"C����� DHCP",
	apStartIP:"��������� ����� IP",
	EndIP:"�������� ����� IP",
	apBeacon:"������ ������������� ������� (ms)",
	apSleepMode:"����� ���",
	apNotSleep:"�� ������������",
	apSleepLight:"�����������",
	apSleepModem:"�����",
	apMaxCon:"������������ �����������",
	//-- restart
	restTitle:"���������� ����������",
	restStr1:"������ ���������:",
	restStr2:"���������� ������������� ����� ���������� ���� ��������.<br>���������� ������� ���������� �� ��������� �����. <br>����� ������������� ����������?",
	//-- reset
	rstTitle:"������� � ��������� ����������",
	rstStr1:"������ ���������:",
	rstStr2:'����� �������� � ��������� ����������, ��� ��������� ������������� ����� �������.<br>�� ������ �������� �� �� http://192.168.4.1.����� �������� ��� ������������ ����� "ESP8266" ������ "0123456789"<br>�������� ��������� �� ���������?',
	//-- success
	sucTitle:"������������ ���������!",
	sucStr1:"�������� �������� ��� ���������� ���������� WIFI � ����, � ����� ������� � ��������� ������������."

}
var captionMultiRU = {
	mainOptOff:"���������",
	mainOptOn:"��������"
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
	stMAC:"���",
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
