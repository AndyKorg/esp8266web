/*
 * ntp.h
 * Пока поддерживается только протокол TIME
 * TODO: добавить протокол SNTP https://tools.ietf.org/html/rfc4330
 */

#ifndef APP_INCLUDE_NTP_H_
#define APP_INCLUDE_NTP_H_

#ifdef LWIP_OPEN_SRC
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#else
#include "ip_addr.h"
#endif

#define NTP_PRT_OUT		37				//Порт ntp внешнего ntp-сервера
#define NTP_IP_SRV_DEFAULT	0xac8d8a80	//Адрес внешнего ntp сервера по умолчанию "128.138.141.172"

#define NTP_TIME_UNKNOWN	0
typedef void (*NTP_GET_SECOND)(uint32 sec);

typedef struct {						//Адрес ntp внешнего сервера
	struct ip_addr ip;
	uint16 port;
} sHost;

extern sHost ntp_srv_ip;
extern NTP_GET_SECOND ntpGetSeconds;	//Callback функция вызываемая при получении времени из внешнего сервера

err_t ICACHE_FLASH_ATTR ntpStart(void);


extern u8_t JsonGo;
void InitJSON();

#endif /* APP_INCLUDE_NTP_H_ */
