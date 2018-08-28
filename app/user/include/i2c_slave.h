/*
 * i2c_slave.h
 *
 *  Created on: 19 авг. 2018 г.
 *      Author: Administrator
 */

#ifndef APP_USER_INCLUDE_I2C_SLAVE_H_
#define APP_USER_INCLUDE_I2C_SLAVE_H_

#define I2C_SLAVE_SDA		4
#define I2C_SLAVE_SCL		5

#define I2C_SLAVE_ADR		0b11000000	//Адрес устройства, нулевой бит всегда 0!
#define I2C_SLAVE_SIZE		5			//Размер пакетов

#define	I2C_BUF_FULL		1
#define	I2C_BUF_EMPTY		0

struct ti2c_Buf{
	uint8 volatile buf[I2C_SLAVE_SIZE+1];
	uint8 volatile ready;
};

void i2c_slave_Init(void* func, void* param);	//Принимает адрес и параметры callback-функции вызываемой при окончании сеанса приема

extern struct ti2c_Buf	i2c_InputBuf,	//Буфер приема
						i2c_OutputBuf;	//Буфер передачи


#endif /* APP_USER_INCLUDE_I2C_SLAVE_H_ */
