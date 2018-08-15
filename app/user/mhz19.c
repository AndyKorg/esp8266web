/*
 * mhz19.c
 *
 *  Created on: 17 сент. 2017 г.
 *      Author: Administrator
 */
#include <stdlib.h>
#include "c_types.h"
#include "os_type.h"
#include "lwip/err.h"
#include "sdk/add_func.h"
#include "osapi.h"
#include "my_esp8266.h"
#include "mqtt.h"
#include "mhz19.h"
#include "customer_uart.h"
#include "hc595.h"
#include "wifi.h"

#define MHZ19_TIMER 1000 //Период опроса состояния соединения с сервереом
#define MHZ19_MEASURE_TIMEOUT (30) //Раз в MHZ19_TIMER передавать

MQTT_Client mqttClient;

ETSTimer mqttPublishTimer;

#define MQTT_CAYENNE_VER	"v1/"
#define MQTT_CAYENNE_DELEMITER	"/things/"
#define	MQTT_CAYENNE_TYPE_SYS_MODEL "/sys/model"
#define	MQTT_CAYENNE_TYPE_SYS_VER "/sys/version"
#define	MQTT_CAYENNE_TYPE_SYS_CPU_MODEL "/sys/cpu/model"
#define	MQTT_CAYENNE_TYPE_SYS_CPU_SPEED "/sys/cpu/speed"
#define	MQTT_CAYENNE_TYPE_DATA "/data/"
#define	MQTT_CAYENNE_TYPE_CMD "/cmd/"
#define	MQTT_CAYENNE_CHANNAL "0"
#define	MQTT_CAYENNE_MEASURE_PERIOD "1"
#define	MQTT_CAYENNE_MEASURE_START "2"
#define	MQTT_CAYENNE_MEASURE_LEFT "3"

uint8_t* CayenneTopic(uint8_t* type, uint8_t* channal){
	uint8_t temp = os_strlen(MQTT_CAYENNE_VER)
			+ os_strlen(MQTT_USER)
			+ os_strlen(MQTT_CAYENNE_DELEMITER)
			+ os_strlen(MQTT_CLIENT_ID)
			+ os_strlen(type)
			;
	if (channal)
		temp += os_strlen(channal);
	uint8_t* msg = (uint8_t*)os_zalloc(temp + 1);
	if (msg){
		os_strcpy(msg, MQTT_CAYENNE_VER);
		os_strcpy(msg+os_strlen(msg), MQTT_USER);
		os_strcpy(msg+os_strlen(msg), MQTT_CAYENNE_DELEMITER);
		os_strcpy(msg+os_strlen(msg), MQTT_CLIENT_ID);
		os_strcpy(msg+os_strlen(msg), type);
		if (channal)
			os_strcpy(msg+os_strlen(msg), channal);
	}
	return msg;
}

void ICACHE_FLASH_ATTR mhz19Timer(void *arg){

	static uint32 Timeout = 0, SubscribeNeed = 1;

	os_sprintf_fd(Digit, "%04d", MHZ19Result.Result);

	if (Timeout == 0){
		mhz19StartMeasurUartTx();
		Timeout = MHZ19_MEASURE_TIMEOUT;
#if DEBUGSOO>0
	os_printf("MQTT: Measure end. WiFi status = %d State client = %d\r\n", wifi_station_get_connect_status(), mqttClient.connState );
#endif
	}

	if (wifi_station_get_connect_status() == STATION_GOT_IP){ //IP адрес получен
		if (mqttClient.connState == TCP_DISCONNECTED){
			MQTT_Connect(&mqttClient);
			SubscribeNeed = 1;
		}
		else{
			DispFlagSet(DISP_FLASH_OFF_ALL, DISP_FLASH_MASK);
			if (SubscribeNeed){	//Необходимо подписатся
				uint8_t* topicSubscb = CayenneTopic(MQTT_CAYENNE_TYPE_DATA, MQTT_CAYENNE_CHANNAL);
				MQTT_Subscribe(&mqttClient, topicSubscb, 0);
				os_free(topicSubscb);
			}
			if  (MHZ19Result.IsReady){
				uint8_t* topic = CayenneTopic(MQTT_CAYENNE_TYPE_DATA, MQTT_CAYENNE_CHANNAL);
				uint8_t* result;
				result = os_zalloc(10);
				os_sprintf_fd(result, "co2,ppm=%d", MHZ19Result.Result);
				if (MQTT_Publish(&mqttClient
						, topic
						, result
						, strlen(result)
						, MQTT_QOS_TYPE_AT_MOST_ONCE
						, MQTT_RETAIN_OFF) != ERR_OK){
					MQTT_Disconnect(&mqttClient);
					DispFlagSet(DISP_FLASH_ON_ALL, DISP_FLASH_MASK);
				}
				os_free(result);
				os_free(topic);
				MHZ19Result.IsReady = 0;
			}
		}
	}
	else{
		DispFlagSet(DISP_FLASH_ON_ALL, DISP_FLASH_MASK);
	}
	Timeout--;
}

void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	//MQTT_Subscribe(client, "/mqtt/topic/0", 0);
	uint8_t* topic = CayenneTopic(MQTT_CAYENNE_TYPE_SYS_MODEL, NULL);
#define CAYENNE_MODEL "esp8266pvvx"
	MQTT_Publish(client, topic, CAYENNE_MODEL, strlen(CAYENNE_MODEL), MQTT_QOS_TYPE_AT_MOST_ONCE, MQTT_RETAIN_OFF);
	os_free(topic);
#if DEBUGSOO>0
	os_printf("MQTT: connect and publeshed\r\n");
#endif

}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;

	os_timer_disarm(&mqttPublishTimer);
#if DEBUGSOO>0
	os_printf("MQTT: Disconnected %d\r\n", client->connState);
#endif
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
#if DEBUGSOO>0
	os_printf("MQTT: mqttPublishedCb %d\r\n", client->connState);
#endif
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	//MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

#if DEBUGSOO>0
	os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
#endif
	os_free(topicBuf);
	os_free(dataBuf);
}

/*
 * Запустить измерение и если есть возможность то передачу mqtt пакетов
 */
void ICACHE_FLASH_ATTR mhz19Init(void){

//	MQTT_InitClient(&mqttClient, "123", "jzakaouf", "cF89nfN2yf3o", 120, 1);
//	MQTT_InitConnection(&mqttClient, "m20.cloudmqtt.com", 18889, 0);

	MQTT_InitClient(&mqttClient, MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, 120, 1);
	//MQTT_InitConnection(&mqttClient, "messaging.internetofthings.ibmcloud.com", 1883, 0);
	MQTT_InitConnection(&mqttClient, MQTT_HOST, MQTT_PORT, 0);

	uint8_t* tmp = CayenneTopic(MQTT_CAYENNE_TYPE_SYS_MODEL, NULL);
	if (tmp){
#if DEBUGSOO>0
		os_printf("Init topic: %s\r\n", tmp);
#endif
		MQTT_InitLWT(&mqttClient, tmp, "ESP8266pvvx", MQTT_QOS_TYPE_AT_MOST_ONCE, 0);
		os_free(tmp);

		MQTT_OnConnected(&mqttClient, mqttConnectedCb);
		MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
		MQTT_OnPublished(&mqttClient, mqttPublishedCb);
		MQTT_OnData(&mqttClient, mqttDataCb);

		os_timer_disarm(&mqttPublishTimer);
		os_timer_setfn(&mqttPublishTimer, (os_timer_func_t *)mhz19Timer, &mqttClient);
		ets_timer_arm_new(&mqttPublishTimer, MHZ19_TIMER, OS_TIMER_REPEAT, OS_TIMER_MS);
	}
#if DEBUGSOO>0
	else{
		os_printf("Init topic: out of memory!\r\n");
	}
#endif

}



