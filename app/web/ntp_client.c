/*
 * Получение времение от внешнего ntp сервера по протоколу TIME
 * ntp_srv_ip - адрес и порт (обычно 37) внешнего сервера TIME
 * err_t ntpStart(void) - запустить получнеие времени. Возвращает ERR_OK при успехе или код ошибки
 * Перед началом получения времени необходимо определить callback функцию ntpGetSeconds
 * NTP_GET_SECOND ntpGetSeconds	функция вызываемая при получении времени из внешнего сервера
 * В случае если время в течение таймаута получить не удалось функция ntpGetSeconds вызывается
 * с аргументом sec = NTP_TIME_UNKNOWN
 * В случае успешного получения времени аргумент sec содержит количество секунд прошедших с 01.01.1900 г.
 */

#include "../include/ntp.h"
#include "lwip/udp.h"
#include "lwip/err.h"
#include "sdk/add_func.h"

#include "my_esp8266.h"
#include "user_interface.h"

struct udp_pcb* udp_ntp = NULL;			//Соединение по udp

#define NTP_OUT_BYTE	0x53			//Отправляемый байт на порт ntp сервера. Может быть любой
#define NTP_OUT_LEN		1				//Количество отправляемых байт
#define	NTP_PRT_IN_AUTO	0				//Автоматический выбор порта приема ответа от NTP-сервера
#define NTP_ANSWER_LEN	4				//Ожидаемая длина ответа сервера ntp

#define NTP_TIMEOUT_MS	5000			//Таймаут ответа сервера

ETSTimer ntp_timeout_timet;				//Таймаут сервера ntp

NTP_GET_SECOND ntpGetSeconds = NULL;	//Callback функция вызываемая при получении времени из внешнего сервера

sHost ntp_srv_ip = {{NTP_IP_SRV_DEFAULT}, NTP_PRT_OUT};

void ICACHE_FLASH_ATTR ExchByte(uint8 *b1, uint8 *b2){
	uint8 i;
	i = *b1;
	*b1 = *b2;
	*b2 = i;
}

/*
 * Освободить соединение и память
 */
void ICACHE_FLASH_ATTR ntp_udp_free(void){
	os_timer_disarm(&ntp_timeout_timet);//Отключить таймер таймаута ответа сервера
	udp_disconnect(udp_ntp);
	udp_remove(udp_ntp);
	udp_ntp = NULL;
}

/*
 * Ответ от сервера получен
 */
void ICACHE_FLASH_ATTR udp_rcv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
    ip_addr_t *addr, u16_t port){

	uint8 *j;
	uint32 ntpSeconds = NTP_TIME_UNKNOWN;
	if (p->len == NTP_ANSWER_LEN){
		j = p->payload;
		ExchByte((j+3), j);
		ExchByte((j+1), (j+2));
		memcpy(&ntpSeconds, p->payload, 4);
	}
	else{									//Неправильный ответ ntp-сервера
#if (DEBUGSOO > 1)
		os_printf("ntp answer bad\n");
#endif
	}
	ntp_udp_free();
	pbuf_free(p);
	if (ntpGetSeconds != NULL)
		ntpGetSeconds(ntpSeconds);
}

/*
 * Вылет по таймауту. NTP сервер не ответил
 */
void ICACHE_FLASH_ATTR ntpTimeout(void){
#if DEBUGSOO>1
	os_printf("ntp answer timeout\n");
#endif
	ntp_udp_free();
	if (ntpGetSeconds != NULL)
		ntpGetSeconds(NTP_TIME_UNKNOWN);
}

/*
 * Запуск запроса времени от ntp-сервера
 */
err_t ICACHE_FLASH_ATTR ntpStart(void){

	err_t err = ERR_OK;
	uint8 *i;

	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_OUT_LEN, PBUF_RAM);	//Захват буфера для пакета
	i = p->payload;
	*i = NTP_OUT_BYTE;

	if (udp_ntp == NULL){										//Если соединения еще нет, то захватить память
		udp_ntp = udp_new();
	}
	else{
		return ERR_ISCONN;										//Соединие еще занято обработкой предыдущей задачи
	}

	if (udp_ntp != NULL){										//Память для соединения захвачена
		udp_recv(udp_ntp, udp_rcv, NULL);						//Зарегистрировать функцию приема
		err = udp_bind(udp_ntp, IP_ADDR_ANY, NTP_PRT_IN_AUTO);	//Открыть порт приема ответа от NTP на всех интерфйесах
		if (err == ERR_OK){
			err = udp_sendto(udp_ntp, p, &(ntp_srv_ip.ip), ntp_srv_ip.port);	//Отправить запрос на NTP сервер
			if (err != ERR_OK){									//Передача не удалась, закрываем соединение
				udp_disconnect(udp_ntp);
			}
			else{												//Все нормально ждем ответа сервера
				os_timer_disarm(&ntp_timeout_timet);			//Запуск тамера таймаута
				os_timer_setfn(&ntp_timeout_timet, (os_timer_func_t *) ntpTimeout, NULL);
				ets_timer_arm_new(&ntp_timeout_timet, NTP_TIMEOUT_MS, OS_TIMER_SIMPLE, OS_TIMER_MS);
			}
		}
		if (err != ERR_OK){
			udp_remove(udp_ntp);								//Освободить соединение
		}
	}
	else {
		err = ERR_MEM;
	}

	pbuf_free(p);
	return err;
}


