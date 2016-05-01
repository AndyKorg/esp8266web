/*
 * Типы для объявления переменных и команд в страницах http
 *
 */
#ifndef __CUTOMER_HTTP__
#define __CUTOMER_HTTP__

#include "c_types.h"
#include "../web/include/web_srv_int.h"
#include "lwip/err.h"

#define HTTP_ROOT_LEN	9			//Количество элементов в корневом списке переменых и команд
#define pHttpVarSize	sizeof(*pHttpVar)
#define SizeArray(a)	((sizeof(a)/sizeof(a[0])))	//Размер массива
#define MAX_CMD_ID		9			//Максимальное значение индекса N в команде cmdParent_N_cmdChild.
#define NUM_LEN_CMD		2			//Количество символов в индексе команды вида cmdParent_N_cmdChild (1-символ для цифры + 1 символ для подчеркивания)


typedef uint8						//returns the number of processed characters
		(*VOID_PTR_GET_VAR)			//Get function for http-variable
			(WEB_SRV_CONN* web_con,	//Connect to tcp-client
			uint8* cstr);			//Text adres line


typedef uint8						//returns the number of processed characters
		(*VOID_PTR_SET_VAR)(		//Set function http-varialbe
			WEB_SRV_CONN* web_con,	//Connect to tcp-client
			uint8 *pcmd,			//command line
			uint8 *pvar); 			//value variable

typedef enum {
		vtNULL,
		vtString,
		vtErr,						//Код ошибки из файла err.h
		vtByte,						//Беззнаковый байт
		vtWord16,					//Беззнаковое слово
		vtWord32					//Беззнаковое двойное слово
} varHttpType;

//Описание переменных http
typedef struct{
	uint8 *Name;					//Имя переменной
	varHttpType varType;			//Тип
	char *ArgPrnHttp;				//строка форматирования вывода в http вида "%d"
	void *Value;					//указатель на переменную
	VOID_PTR_GET_VAR execGet;		//Функция вызываемая для чтения значения переменной, вызывается первой если определена
	VOID_PTR_SET_VAR execSet;		//Функция вызываемая для сохранения значения переменной
	void *Child;					//следующая часть переменной или команды
	uint8 ChildLen;					//Размер дочернего массива, берется как SizeArray(Child)
} pHttpVar;

extern pHttpVar HttpMyRoot[HTTP_ROOT_LEN];	//корневые переменные. Должен быть объявлен в каком-нибудь из файлов


//Определение номера N в составе команды cmdParent_N_cmdChild, возвращает MAX_CMD_ID+1 если номер не удалось определить
//maxNum должно быть больше или равно MAX_CMD_ID.
//Так же может использоватся массив индексов указываемый в Ident
//lenCmd возвращает количество символов в найденной команде
uint8 ParceNumObject(uint8* pcmd, uint8 maxNum, uint8 *Ident[], uint8 *lenCmd);
//Для вызова из web_int_vars.h или "вручную"
extern uint8 ICACHE_FLASH_ATTR parseHttpSetVar(pHttpVar *ParentVar, uint8 ParentLen,	//Родительские переменные
		WEB_SRV_CONN *web_conn, uint8 *pcmd, uint8 *pvar);
//Для вызова из web_int_callbacks.c или "вручную"
extern uint8 ICACHE_FLASH_ATTR parseHttpGetVar(pHttpVar *ParentVar, uint8 ParentLen,	//Родительские переменные
		WEB_SRV_CONN *web_conn, uint8 *cstr);

#endif /*__CUTOMER_HTTP__*/
