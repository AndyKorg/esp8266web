/*
 * i2c_slave.h
 *
 *  Не отлажено! С аппаратным работает, с программным нет
 */

#ifndef APP_USER_INCLUDE_I2C_SLAVE_H_
#define APP_USER_INCLUDE_I2C_SLAVE_H_

#define I2C_SLAVE_SDA		4
#define I2C_SLAVE_SCL		5

#define I2C_SLAVE_ADR		0b11000000	//Адрес устройства, нулевой бит всегда 0!
#define I2C_SLAVE_SIZE		5			//Максимальный размер пакетов.

#define i2c_BufClear(x)		do{ x.len = 0;} while(0)
#define i2c_BufIsEmpty(x)	(x.len == 0)
#define i2c_BufIsFull(x)	(x.len)


struct ti2c_Buf{
	uint8 volatile buf[I2C_SLAVE_SIZE+1];
	uint8 volatile len;
};

typedef void (*OW_CallbackFunct)(void);

extern OW_CallbackFunct 	i2c_ReadOk,	//Вызывается когда принят пакет от мастера, в i2c_InputBuf принятые байты
							i2c_AdrOk,	//Принят адрес при операции Write - Repeat Start - Read or Write
							i2c_WriteOk;//Пердача буфера i2c_OutputBuf окончена


void i2c_slave_Init(void* func, void* param);	//Принимает адрес и параметры callback-функции вызываемой при окончании сеанса приема

extern struct ti2c_Buf	i2c_InputBuf,	//Буфер приема
						i2c_AdrBuf,		//Адрес внутри чипа
						i2c_OutputBuf;	//Буфер передачи


#endif /* APP_USER_INCLUDE_I2C_SLAVE_H_ */
