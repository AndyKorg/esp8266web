/*
 * CustomerHttpVar.c
 * Processing variables user
 */
#include "customer_html.h"
#include "web_utils.h"

/*
 *	Ищет в строке pcmd либо числовой индекс от 0 до maxNum, либо совпадающую строку в массиве Ident
 *	Возвращает найденный индекс или число на 1 большее maxNum. В *lenCmd помещается количество
 *	обработанных символов в строке cmd
 */
uint8 ICACHE_FLASH_ATTR ParceNumObject(uint8* pcmd, uint8 maxNum, uint8 *Ident[], uint8 *lenCmd){
#if MAX_CMD_ID >9
	#error"MAX_CMD_ID more then 9, need rewrite function ParceNumObject"
#else
	uint8 Ret = MAX_CMD_ID+1;

	*lenCmd = 0;
	if (Ident == NULL){												//Нет списка индексов
		if (maxNum <= MAX_CMD_ID){
			if ((pcmd[0]>='0') && (pcmd[0]<=(0x30+maxNum)) && (strlen(pcmd) >= 3)){
				Ret = pcmd[0] & 0x0f;
				*lenCmd = NUM_LEN_CMD;
			}
		}
#if DEBUGSOO>1
		else{
			os_printf_plus("bad max index cmd maxNum = %d\n", maxNum);
		}
#endif
	}
	else{															//Индексы представлены словами
		uint8 i;
		for (i=0; i < maxNum; ++i) {
			if (!os_memcmp((void *) pcmd, Ident[i], os_strlen(Ident[i]))){
				Ret = i;
				*lenCmd = os_strlen(Ident[i]);
				break;
			}
		}
	}
#endif
	return Ret;
}

/*
ICACHE_RODATA_ATTR - можно сюда пихать данные, но читать отуда надо специальной процедурой
что-то типа:
    	WAV_HEADER * ptr = (WAV_HEADER *) &web_conn->msgbuf[web_conn->msgbuflen];
    	os_memcpy(ptr, &wav_header, WAV_HEADER_SIZE);
*/
/*
 * Вернуть массив переменных http
 */
pHttpVar* ICACHE_FLASH_ATTR GetArrayVar(pHttpVar* Array, uint8* ArrayLen){
	if (Array == NULL){
		*ArrayLen = SizeArray(HttpMyRoot);
		return HttpMyRoot;
	}
	else
		return Array;
}

uint8 idObject;

/*
 * Обработка вывода значения в http
 */
uint8 ICACHE_FLASH_ATTR parseHttpGetVar(pHttpVar *ParentVar, uint8 ParentLen, WEB_SRV_CONN *web_conn, uint8 *cstr){

	uint8 i, Ret = 0;

	if (!strlen(cstr)) return Ret;
	pHttpVar *VarList;

	VarList = GetArrayVar(ParentVar, &ParentLen);

	for (i = 0; i < ParentLen; i++) {
		if (!os_memcmp((void *) cstr, VarList[i].Name, os_strlen(VarList[i].Name))){
			Ret = os_strlen(VarList[i].Name);
			cstr += Ret;
			if ((VarList[i].Value != NULL) && (VarList[i].varType != vtNULL)){
				switch (VarList[i].varType){
					case vtErr:
						tcp_puts(VarList[i].ArgPrnHttp, *((err_t*)(VarList[i].Value)));
						break;
					case vtByte:
						tcp_puts(VarList[i].ArgPrnHttp, *((uint8*)(VarList[i].Value)));
						break;
					case vtString:
						tcp_puts(VarList[i].ArgPrnHttp, ((uint8*)(VarList[i].Value)));
						break;
					case vtWord16:
						tcp_puts(VarList[i].ArgPrnHttp, *((uint16*)(VarList[i].Value)));
						break;
					case vtWord32:
						tcp_puts(VarList[i].ArgPrnHttp, *((uint32*)(VarList[i].Value)));
						break;
					default:
						tcp_puts("bad var type");
						break;
				}
			}
			if (VarList[i].execGet != NULL){
				uint8 j = VarList[i].execGet(web_conn, cstr);
				cstr += j;
				Ret += j;
			}
			if (VarList[i].Child != NULL){
				Ret += (parseHttpGetVar(VarList[i].Child, VarList[i].ChildLen, web_conn, cstr));
			}
			break;
		}
	}
	return Ret;
}

/*
 * Write httpvar to variable
 */
uint8 ICACHE_FLASH_ATTR parseHttpSetVar(pHttpVar *ParentVar, uint8 ParentLen, WEB_SRV_CONN *web_conn, uint8 *pcmd, uint8 *pvar){

	uint8 i, Ret = 0;

	if (!strlen(pcmd)) return Ret;

	char *cstr = pcmd;

	pHttpVar *VarList;

	VarList = GetArrayVar(ParentVar, &ParentLen);

	for (i = 0; i < ParentLen; i++) {
		if (!os_memcmp((void *) cstr, VarList[i].Name, os_strlen(VarList[i].Name))){
			cstr += os_strlen(VarList[i].Name);
			Ret = os_strlen(VarList[i].Name);
			if ((VarList[i].Value != NULL) && (VarList[i].varType != vtNULL)){
				switch (VarList[i].varType){
					case vtErr:
						*((err_t*)VarList[i].Value) = (err_t)rom_atoi(pvar);
						break;
					case vtByte:
						if ((VarList[i].ArgPrnHttp[strlen(VarList[i].ArgPrnHttp)-1]=='x') || (VarList[i].ArgPrnHttp[strlen(VarList[i].ArgPrnHttp)-1]=='X'))
							*((uint8*)VarList[i].Value) = (uint8)hextoul(pvar);
						else
							*((uint8*)VarList[i].Value) = (uint8)rom_atoi(pvar);
						break;
					case vtString:
#if DEBUGSOO>1
		os_printf("custom https param = %s val = %s\n", pcmd, pvar);
#endif

						if (os_strlen(pvar) == 0){	//Пустая строка не хораняется, поэтому исопьзуется пробел
							os_sprintf_fd((uint8*)VarList[i].Value, " ");
						}
						else{
							os_sprintf_fd((uint8*)VarList[i].Value, "%s",pvar);
						}
#if DEBUGSOO>1
		os_printf("after save = %s\n", ((uint8*)VarList[i].Value));
#endif
						break;
					case vtWord16:
						if ((VarList[i].ArgPrnHttp[strlen(VarList[i].ArgPrnHttp)-1]=='x') || (VarList[i].ArgPrnHttp[strlen(VarList[i].ArgPrnHttp)-1]=='X'))
							*((uint16*)VarList[i].Value) = (uint16)hextoul(pvar);
						else
							*((uint16*)VarList[i].Value) = (uint16)rom_atoi(pvar);
						break;
					case vtWord32:
						if ((VarList[i].ArgPrnHttp[strlen(VarList[i].ArgPrnHttp)-1]=='x') || (VarList[i].ArgPrnHttp[strlen(VarList[i].ArgPrnHttp)-1]=='X'))
							*((uint32*)VarList[i].Value) = hextoul(pvar);
						else
							*((uint32*)VarList[i].Value) = rom_atoi(pvar);
						break;
					default:
						break;
				}
			}
			if (VarList[i].execSet != NULL){
				uint8 j = VarList[i].execSet(web_conn, cstr, pvar);
				cstr += j;
				Ret += j;
			}
			if (VarList[i].Child != NULL){
				Ret += (parseHttpSetVar(VarList[i].Child, VarList[i].ChildLen, web_conn, cstr, pvar));
			}
			break;
		}
	}
	return Ret;
}

