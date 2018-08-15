/* mqtt.c
*  Protocol: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "sdk/add_func.h"
#include "../../include/os_type.h"
#include "../include/web_utils.h"
//#include "../../include/osapi.h"

#include "../include/my_esp8266.h"

#include "../include/mqtt_msg.h"
#include "../include/mqtt.h"
#include "../include/str_queue.h"

#define MQTT_TASK_PRIO        		0
#define MQTT_TASK_QUEUE_SIZE    	1
#define MQTT_TIMER_REPEAT_MS		1000
#define MQTT_SEND_TIMOUT			5 //from MQTT_TIMER_REPEAT_MS

#ifndef QUEUE_BUFFER_SIZE
#define QUEUE_BUFFER_SIZE		 	2048
#endif

#define INFO os_printf

unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

ETSTimer mqttDNSRepeat;
os_event_t mqtt_procTaskQueue[MQTT_TASK_QUEUE_SIZE];

err_t ICACHE_FLASH_ATTR resolve(MQTT_Client *mqttClient);

LOCAL void ICACHE_FLASH_ATTR
deliver_publish(MQTT_Client* client, uint8_t* message, int length)
{
	mqtt_event_data_t event_data;

	event_data.topic_length = length;
	event_data.topic = mqtt_get_publish_topic(message, &event_data.topic_length);
	event_data.data_length = length;
	event_data.data = mqtt_get_publish_data(message, &event_data.data_length);

	if(client->dataCb)
		client->dataCb((uint32_t*)client, event_data.topic, event_data.topic_length, event_data.data, event_data.data_length);

}


/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
mqtt_tcpclient_recv(void *arg, char *pdata, unsigned short len)
  */
static err_t ICACHE_FLASH_ATTR mqtt_tcpclient_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	uint8_t msg_type;
	uint8_t msg_qos;
	uint16_t msg_id, len;

	MQTT_Client *client = (MQTT_Client *)arg;

READPACKET:
	INFO("TCP: data received %d bytes, state = %d\r\n", p->len,client->connState);

#if DEBUGSOO > 0
	for(len=0;len<p->len;len++){
		os_printf("%02x\r\n",*(uint8*)(p->payload+len));
	}
#endif

	if(p->len< MQTT_BUF_SIZE && p->len > 0 && err == ERR_OK){
		os_memcpy(client->mqtt_state.in_buffer, p->payload, p->len);

		msg_type = mqtt_get_type(client->mqtt_state.in_buffer);
		msg_qos = mqtt_get_qos(client->mqtt_state.in_buffer);
		msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);
		switch(client->connState){
			case MQTT_CONNECT_SENDING:
				if(msg_type == MQTT_MSG_TYPE_CONNACK){
					if(client->mqtt_state.pending_msg_type != MQTT_MSG_TYPE_CONNECT){
						INFO("MQTT: Invalid packet\r\n");
						tcp_close(pcb);//TODO: возможно надо освободить дискрипторы приема, сеодинения и пр.
					} else {
						INFO("MQTT: Connected to %s:%d\r\n", client->host, client->port);
						client->connState = MQTT_DATA;
						if(client->connectedCb)
							client->connectedCb((uint32_t*)client);
					}
				}
				break;
			case MQTT_DATA:
				client->mqtt_state.message_length_read = p->len;
				client->mqtt_state.message_length = mqtt_get_total_length(client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);

				switch(msg_type)
				{
				  case MQTT_MSG_TYPE_SUBACK:
					if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_SUBSCRIBE && client->mqtt_state.pending_msg_id == msg_id)
					  INFO("MQTT: Subscribe successful\r\n");
					break;
				  case MQTT_MSG_TYPE_UNSUBACK:
					if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && client->mqtt_state.pending_msg_id == msg_id)
					  INFO("MQTT: UnSubscribe successful\r\n");
					break;
				  case MQTT_MSG_TYPE_PUBLISH:
					if(msg_qos == MQTT_QOS_TYPE_AT_LEAST_ONCE)
						client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);
					else if(msg_qos == MQTT_QOS_TYPE_EXACTLY_ONE)
						client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);
					if(msg_qos == MQTT_QOS_TYPE_AT_LEAST_ONCE || msg_qos == MQTT_QOS_TYPE_EXACTLY_ONE){
						INFO("MQTT: Queue response QoS: %d\r\n", msg_qos);
						if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
							INFO("MQTT: Queue full\r\n");
						}
					}
					deliver_publish(client, client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
					break;
				  case MQTT_MSG_TYPE_PUBACK:
					if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id){
					  INFO("MQTT: received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish\r\n");
					}

					break;
				  case MQTT_MSG_TYPE_PUBREC:
					  client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
					  if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
						INFO("MQTT: Queue full\r\n");
					  }
					break;
				  case MQTT_MSG_TYPE_PUBREL:
					  client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
					  if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
						INFO("MQTT: Queue full\r\n");
					  }
					break;
				  case MQTT_MSG_TYPE_PUBCOMP:
					if(client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id){
					  INFO("MQTT: receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish\r\n");
					}
					break;
				  case MQTT_MSG_TYPE_PINGREQ:
					  client->mqtt_state.outbound_message = mqtt_msg_pingresp(&client->mqtt_state.mqtt_connection);
					  if(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
						INFO("MQTT: Queue full\r\n");
					  }
					break;
				  case MQTT_MSG_TYPE_PINGRESP:
#if DEBUGSOO > 0
					  os_printf("MQTT: Pong\r\n");
#endif
					// Ignore
					break;
				}//switch
				// NOTE: this is done down here and not in the switch case above
				// because the PSOCK_READBUF_LEN() won't work inside a switch
				// statement due to the way protothreads resume.
				if(msg_type == MQTT_MSG_TYPE_PUBLISH)
				{
				  len = client->mqtt_state.message_length_read;

				  if(client->mqtt_state.message_length < client->mqtt_state.message_length_read)
				  {
					  //client->connState = MQTT_PUBLISH_RECV;
					  //Not Implement yet
					  len -= client->mqtt_state.message_length;
					  //pdata += client->mqtt_state.message_length;

					  INFO("Get another published message\r\n");
					  goto READPACKET;
				  }

				}
				break;
				default:
					break;
		}//switch
	} else {
		INFO("ERROR: Message too long or err = %d\r\n", err);
	}

	if(p != NULL){
		tcp_recved(pcb, p->tot_len);				//Прием порции данных закончен
		pbuf_free(p);
	}

	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
	return err;
}

/**
  * @brief  Client send over callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static err_t ICACHE_FLASH_ATTR mqtt_tcpclient_sent_cb(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	MQTT_Client* client = (MQTT_Client *)arg;
	INFO("TCP: Sent\r\n");
	client->sendTimeout = 0;
	if(client->connState == MQTT_CONNECT_SENDING){
		switch (client->mqtt_state.pending_msg_type){
			case MQTT_MSG_TYPE_PUBLISH:
				if(client->publishedCb)
					client->publishedCb((uint32_t*)client);
				if(client->mqtt_state.pending_publish_qos == MQTT_QOS_TYPE_AT_MOST_ONCE)
					client->connState = MQTT_DATA;
				break;
			case MQTT_MSG_TYPE_PINGRESP:
				client->connState = MQTT_DATA;
				break;
			default:
				break;
		}
	}
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
	return ERR_OK;
}

void ICACHE_FLASH_ATTR mqtt_timer(void *arg)
{

	MQTT_Client* client = (MQTT_Client*)arg;

	if(client->connState == MQTT_DATA){
		client->keepAliveTick ++;
		if(client->keepAliveTick > client->mqtt_state.connect_info->keepalive){

			INFO("\r\nMQTT: Send keepalive packet to %s:%d!\r\n", client->host, client->port);
			client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);
			client->mqtt_state.pending_msg_type = MQTT_MSG_TYPE_PINGRESP;
			client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);

			client->sendTimeout = MQTT_SEND_TIMOUT;
			err_t err = tcp_write(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length, 0);
			if (err == ERR_OK){//Подготовить запрос
				err = tcp_output(client->pCon);
				if (err == ERR_OK){						//Отправить запрос
					client->connState = MQTT_CONNECT_SENDING;
					INFO("MQTT: Timer sending, type: %d, id: %04X\r\n",client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);
				}
			}
#if DEBUGSOO > 0
			if (err != ERR_OK){
				client->connState = TCP_RECONNECT;
				os_printf("MQTT: connected error = %d\r\n", err);
			}
#endif
			client->mqtt_state.outbound_message = NULL;
			client->keepAliveTick = 0;
			system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
		}

	} else if(client->connState == TCP_RECONNECT_REQ){
		client->reconnectTick ++;
		if(client->reconnectTick > MQTT_RECONNECT_TIMEOUT) {
			client->reconnectTick = 0;
			client->connState = TCP_RECONNECT;
			system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
		}
	}
	if(client->sendTimeout > 0)
		client->sendTimeout --;
}
/*
void ICACHE_FLASH_ATTR
mqtt_tcpclient_discon_cb(void *arg)
{

	MQTT_Client* client = (MQTT_Client *)pespconn->reverse;
	INFO("TCP: Disconnected callback\r\n");
	client->connState = TCP_RECONNECT_REQ;
	if(client->disconnectedCb)
		client->disconnectedCb((uint32_t*)client);

	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
}
*/

/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static err_t ICACHE_FLASH_ATTR mqtt_tcpclient_connect_cb(void *arg, struct tcp_pcb *pcb, err_t err)
{
	MQTT_Client* client = (MQTT_Client *)arg;

	tcp_recv(client->pCon, mqtt_tcpclient_recv);
	tcp_sent(client->pCon, mqtt_tcpclient_sent_cb);

	//espconn_regist_disconcb(client->pCon, mqtt_tcpclient_discon_cb); //TODO: Как отсоединятся пока не понятно
	INFO("MQTT: Connected to broker %s:%d\r\n", client->host, client->port);

	mqtt_msg_init(&client->mqtt_state.mqtt_connection, client->mqtt_state.out_buffer, client->mqtt_state.out_buffer_length);
	client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection, client->mqtt_state.connect_info);
	client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
	client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);

	client->sendTimeout = MQTT_SEND_TIMOUT;
	INFO("MQTT: Sending, type: %d, id: %04X\r\n",client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);

	err = tcp_write(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length, 0);
	if (err == ERR_OK){//Подготовить запрос
		err = tcp_output(client->pCon);
		if (err == ERR_OK){						//Отправить запрос
			client->mqtt_state.outbound_message = NULL;
			client->connState = MQTT_CONNECT_SENDING;
			system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
		}
	}
#if DEBUGSOO > 0
	if (err != ERR_OK){
		os_printf("MQTT: connected error = %d\r\n", err);
		//TODO: Закрыть соединение если не удалось отправить запрос?
	}
#endif

	return err;
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
/*
ПОКа не испольуется
void ICACHE_FLASH_ATTR
mqtt_tcpclient_recon_cb(void *arg, sint8 errType)
{
	struct espconn *pCon = (struct espconn *)arg;
	MQTT_Client* client = (MQTT_Client *)pCon->reverse;

	INFO("TCP: Reconnect to %s:%d\r\n", client->host, client->port);

	client->connState = TCP_RECONNECT_REQ;

	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);

}
*/

/**
  * @brief  MQTT publish function.
  * @param  client: 	MQTT_Client reference
  * @param  topic: 		string topic will publish to
  * @param  data: 		buffer data send point to
  * @param  data_length: length of data
  * @param  qos:		qos
  * @param  retain:		retain
  * @retval TRUE if success queue
  */
err_t ICACHE_FLASH_ATTR
MQTT_Publish(MQTT_Client *client, const char* topic, const char* data, int data_length, int qos, int retain)
{
	uint8_t dataBuffer[MQTT_BUF_SIZE];
	uint16_t dataLen;
	INFO("MQTT: Publish task start\r\n");
	client->mqtt_state.outbound_message = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
										 topic, data, data_length,
										 qos, retain,
										 &client->mqtt_state.pending_msg_id);
	if(client->mqtt_state.outbound_message->length == 0){
		INFO("MQTT: Queuing publish failed\r\n");
		return ERR_BUF;
	}
	INFO("MQTT: queuing publish, length: %d, queue size(%d/%d)\r\n", client->mqtt_state.outbound_message->length, client->msgQueue.rb.fill_cnt, client->msgQueue.rb.size);
	while(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
		INFO("MQTT: Queue full\r\n");
		if(QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1) {
			INFO("MQTT: Serious buffer error\r\n");
		}
		return ERR_BUF;
	}
	INFO("MQTT: Publish task ok\r\n");
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
	return ERR_OK;
}

/**
  * @brief  MQTT subscibe function.
  * @param  client: 	MQTT_Client reference
  * @param  topic: 		string topic will subscribe
  * @param  qos:		qos
  * @retval TRUE if success queue
  */
BOOL ICACHE_FLASH_ATTR
MQTT_Subscribe(MQTT_Client *client, char* topic, uint8_t qos)
{
	uint8_t dataBuffer[MQTT_BUF_SIZE];
	uint16_t dataLen;

	client->mqtt_state.outbound_message = mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,
											topic, 0,
											&client->mqtt_state.pending_msg_id);
	INFO("MQTT: queue subscribe, topic start\"%s\", id: %d\r\n",topic, client->mqtt_state.pending_msg_id);
	while(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1){
		INFO("MQTT: Queue full\r\n");
		if(QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1) {
			INFO("MQTT: Serious buffer error\r\n");
		}
		return FALSE;
	}
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
	INFO("MQTT: queue subscribe, topic\"%s\", id: %d\r\n",topic, client->mqtt_state.pending_msg_id);
	return TRUE;
}

void ICACHE_FLASH_ATTR
MQTT_Task(os_event_t *e)
{
	//uint8_t dataBuffer[MQTT_BUF_SIZE];
	//uint16_t dataLen;
	if(e->par == 0)
		return;

	MQTT_Client* client = (MQTT_Client*)e->par;
	switch(client->connState){
		case TCP_RECONNECT_REQ:
			break;
		case TCP_RECONNECT:
			if (resolve(client) != ERR_OK){
				INFO("TCP: Reconnect to: %s:%d\r\n", client->host, client->port);
				client->connState = TCP_CONNECTING;
			}
			break;
		case MQTT_DATA:
			if(QUEUE_IsEmpty(&client->msgQueue) || client->sendTimeout != 0) {
				break;
			}
			//if(QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == 0){
			if(QUEUE_Gets(&client->msgQueue, client->mqtt_state.outbound_message->data, &client->mqtt_state.outbound_message->length, MQTT_BUF_SIZE) == 0){
				client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
				client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);

				client->sendTimeout = MQTT_SEND_TIMOUT;
				INFO("MQTT: Task sending, type: %d, id: %04X\r\n",client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);
				err_t err = tcp_write(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length, 0);
				if (err == ERR_OK){//Подготовить запрос
					err = tcp_output(client->pCon);
					if (err == ERR_OK){						//Отправить запрос
						client->mqtt_state.outbound_message = NULL;
						client->connState = MQTT_CONNECT_SENDING;
#if DEBUGSOO > 0
						os_printf("\nOutput OK\r\n");
#endif
					}
				}
				if (err != ERR_OK){
					client->connState = TCP_RECONNECT_REQ;
#if DEBUGSOO > 0
		os_printf("\noutput in task err = %d\n", err);
#endif
				}
				client->mqtt_state.outbound_message = NULL;
				system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
				break;
			}
			break;
		default:
			break;
	}
}

/**
  * @brief  MQTT initialization connection function
  * @param  client: 	MQTT_Client reference
  * @param  host: 	Domain or IP string
  * @param  port: 	Port to connect
  * @param  security:		1 for ssl, 0 for none
  * @retval None
  */
void ICACHE_FLASH_ATTR
MQTT_InitConnection(MQTT_Client *mqttClient, uint8_t* host, uint32 port, uint8_t security)
{
	uint32_t temp = os_strlen(host);
	//TODO: Если есть соединение надо сообщить о его отключении
//	TCP_DISCONNECTED
	if (mqttClient->host != NULL){
		os_free(mqttClient->host);
		mqttClient->host = NULL;
	}
	if (temp){
		mqttClient->host = (uint8_t*)os_zalloc(temp + 1);
		os_strcpy(mqttClient->host, host);
		mqttClient->host[temp] = 0;
		mqttClient->port = port;
		mqttClient->security = security;
		INFO("MQTT_InitConnection\r\n");
	}
}

/**
  * @brief  MQTT initialization mqtt client function. It is called only once!
  * @param  client: 	MQTT_Client reference
  * @param  clientid: 	MQTT client id
  * @param  client_user:MQTT client user
  * @param  client_pass:MQTT client password
  * @param  client_pass:MQTT keep alive timer, in second
  * @retval None
  */
void ICACHE_FLASH_ATTR
MQTT_InitClient(MQTT_Client *mqttClient, uint8_t* client_id, uint8_t* client_user, uint8_t* client_pass, uint32_t keepAliveTime, uint8_t cleanSession)
{
	uint32_t temp;

	os_memset(mqttClient, 0, sizeof(MQTT_Client));
	os_memset(&mqttClient->connect_info, 0, sizeof(mqtt_connect_info_t));

	temp = os_strlen(client_id);
	mqttClient->connect_info.client_id = (uint8_t*)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.client_id, client_id);
	mqttClient->connect_info.client_id[temp] = 0;

	temp = os_strlen(client_user);
	mqttClient->connect_info.username = (uint8_t*)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.username, client_user);
	mqttClient->connect_info.username[temp] = 0;

	temp = os_strlen(client_pass);
	mqttClient->connect_info.password = (uint8_t*)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.password, client_pass);
	mqttClient->connect_info.password[temp] = 0;


	mqttClient->connect_info.keepalive = keepAliveTime;
	mqttClient->connect_info.clean_session = cleanSession;

	mqttClient->mqtt_state.in_buffer = (uint8_t *)os_zalloc(MQTT_BUF_SIZE);
	mqttClient->mqtt_state.in_buffer_length = MQTT_BUF_SIZE;
	mqttClient->mqtt_state.out_buffer =  (uint8_t *)os_zalloc(MQTT_BUF_SIZE);
	mqttClient->mqtt_state.out_buffer_length = MQTT_BUF_SIZE;
	mqttClient->mqtt_state.connect_info = &mqttClient->connect_info;

	mqtt_msg_init(&mqttClient->mqtt_state.mqtt_connection, mqttClient->mqtt_state.out_buffer, mqttClient->mqtt_state.out_buffer_length);

	mqttClient->connState = TCP_DISCONNECTED;
	QUEUE_Init(&mqttClient->msgQueue, QUEUE_BUFFER_SIZE);

	system_os_task(MQTT_Task, MQTT_TASK_PRIO, mqtt_procTaskQueue, MQTT_TASK_QUEUE_SIZE);
	system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)mqttClient);

	INFO("MQTT_InitClient\r\n");

}
void ICACHE_FLASH_ATTR
MQTT_InitLWT(MQTT_Client *mqttClient, uint8_t* will_topic, uint8_t* will_msg, uint8_t will_qos, uint8_t will_retain)
{
	uint32_t temp;
	temp = os_strlen(will_topic);
	mqttClient->connect_info.will_topic = (uint8_t*)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.will_topic, will_topic);
	mqttClient->connect_info.will_topic[temp] = 0;

	temp = os_strlen(will_msg);
	mqttClient->connect_info.will_message = (uint8_t*)os_zalloc(temp + 1);
	os_strcpy(mqttClient->connect_info.will_message, will_msg);
	mqttClient->connect_info.will_message[temp] = 0;

	mqttClient->connect_info.will_qos = will_qos;
	mqttClient->connect_info.will_retain = will_retain;
}


/*
 * Адрес получен от DNS или таймаут
 */
static void ICACHE_FLASH_ATTR mqtt_dns_found(const char *name, struct ip_addr *ipaddr, void *arg){

	MQTT_Client* client = (MQTT_Client *)arg;

	if ((ipaddr) && (ipaddr->addr)){	//Адрес получен
		os_timer_disarm(&mqttDNSRepeat);
		client->ip.addr = ipaddr->addr;
		tcp_connect(client->pCon, &client->ip, client->port, mqtt_tcpclient_connect_cb);//Начать соединение
		system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);
		return;
	}
	client->connState = TCP_RECONNECT_REQ;//Не удалось получить адрес
#if DEBUGSOO > 0
		os_printf("DNS: not resolvet\r\n");
#endif
}

err_t ICACHE_FLASH_ATTR resolve(MQTT_Client *mqttClient){
	err_t errDNS;

	errDNS = dns_gethostbyname(mqttClient->host, &mqttClient->ip, (dns_found_callback) mqtt_dns_found, mqttClient);
	switch (errDNS) {
		case ERR_OK:					//Адрес разрешен из кэша или локальной таблицы
			os_timer_disarm(&mqttDNSRepeat);
			tcp_connect(mqttClient->pCon, &mqttClient->ip, mqttClient->port, mqtt_tcpclient_connect_cb);//Начать соединение
			break;
		case ERR_INPROGRESS: 			//Запущен процесс разрешения имени с внешнего DNS

			os_timer_disarm(&mqttDNSRepeat);
			os_timer_setfn(&mqttDNSRepeat, (os_timer_func_t *)resolve, mqttClient);
			ets_timer_arm_new(&mqttDNSRepeat, 500, OS_TIMER_SIMPLE, OS_TIMER_MS);
#if DEBUGSOO > 0
	os_printf("\ndns in progress\n", errDNS);
#endif
			break;
		default:
	#if DEBUGSOO > 0
		os_printf("\nadres error dns ret = %d\n", errDNS);
	#endif
			break;
	}
	return errDNS;
}


/**
  * @brief  Begin connect to MQTT broker
  * @param  client: MQTT_Client reference
  * @retval None
  */
err_t ICACHE_FLASH_ATTR
MQTT_Connect(MQTT_Client *mqttClient)
{
	if ((os_strlen(mqttClient->host) == 0 || (mqttClient->port == 0))){
		return ERR_VAL;
}

	MQTT_Disconnect(mqttClient);

	if (mqttClient->pCon == NULL)
		mqttClient->pCon = tcp_new();
	if (mqttClient->pCon == NULL){
#if DEBUGSOO > 0
		os_printf("\nmqttClient->pCon IS NULL!\r\n");
#endif
		return ERR_MEM;
	}

	mqttClient->pCon->callback_arg = (void*)mqttClient;
	mqttClient->keepAliveTick = 0;
	mqttClient->reconnectTick = 0;

	os_timer_disarm(&mqttClient->mqttTimer);
	os_timer_setfn(&mqttClient->mqttTimer, (os_timer_func_t *)mqtt_timer, mqttClient);
	ets_timer_arm_new(&mqttClient->mqttTimer, MQTT_TIMER_REPEAT_MS, OS_TIMER_REPEAT, OS_TIMER_MS);

	//Сначала получаем IP сервера по имени
#if DEBUGSOO > 0
		os_printf("\nMQTT host %s\r\n", mqttClient->host);
#endif
	err_t err = resolve(mqttClient);
	if (err == ERR_OK)
		mqttClient->connState = TCP_CONNECTING;
	if (err == ERR_INPROGRESS){
		mqttClient->connState = TCP_DNS_REQUEST;
		err = ERR_OK;
	}
	return err;
}

void ICACHE_FLASH_ATTR
MQTT_Disconnect(MQTT_Client *mqttClient)
{

	if (mqttClient->pCon != NULL){

		tcp_arg(mqttClient->pCon, NULL);
		tcp_sent(mqttClient->pCon, NULL);
		tcp_recv(mqttClient->pCon, NULL);

		tcp_close(mqttClient->pCon);

		if(mqttClient->pCon){
			INFO("MQTT: disconnect. Free memory\r\n");
			os_free(mqttClient->pCon);
			mqttClient->pCon = NULL;
		}
	}
	os_timer_disarm(&mqttClient->mqttTimer);
	mqttClient->connState = TCP_DISCONNECTED;
}

void ICACHE_FLASH_ATTR
MQTT_OnConnected(MQTT_Client *mqttClient, MqttCallback connectedCb)
{
	mqttClient->connectedCb = connectedCb;
}

void ICACHE_FLASH_ATTR
MQTT_OnDisconnected(MQTT_Client *mqttClient, MqttCallback disconnectedCb)
{
	mqttClient->disconnectedCb = disconnectedCb;
}

void ICACHE_FLASH_ATTR
MQTT_OnData(MQTT_Client *mqttClient, MqttDataCallback dataCb)
{
	mqttClient->dataCb = dataCb;
}

void ICACHE_FLASH_ATTR
MQTT_OnPublished(MQTT_Client *mqttClient, MqttCallback publishedCb)
{
	mqttClient->publishedCb = publishedCb;
}
