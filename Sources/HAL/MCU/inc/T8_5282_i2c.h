//--------------------------------------------------------------------------
//-| FILENAME: i2c.h
//-|
//-| Created on: 21.10.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#include "ctypes.h"
#include "support_common.h"
#include "ErrorManagement\status_codes.h"
#include "T8_5282_mcf5282.h"

#ifndef I2CMY_H_
#define I2CMY_H_

#define ENTER_SAVE_SECTION OS_ENTER_CRITICAL();
#define EXIT_SAVE_SECTION  OS_EXIT_CRITICAL();

typedef enum _I2C_WAIT{
	WAIT_ACK,
	nWAIT_ACK
} BYTE_WAIT_TYPE;

typedef enum _DATA_INTEGRITY{
	NORMAL_DATA_INTEGRITY=1,
	STRONG_DATA_INTEGRITY
}I2C_DATA_INTEGRITY;


typedef enum _ADDRESS_LENGTH{
	I2C_ADDRESS_BYTE=0,
	I2C_ADDRESS_WORD=1
} I2C_ADDRESS_LENGTH;

void I2Cinit(void);

//! \brief для таймаута по занятости шины
typedef struct {
	u32 (*CurrTick)(void) ; 	//!< возвращает текущий отсчет таймера ОС
	u32 wait_time ;			//!< сколько тиков таймера драйверу надо ждать
} I2CWaitTimer_t;
//! \brief инициализирует внутри драйвера соответсвующие значения
void I2CInit_Timer( const I2CWaitTimer_t timer );


void I2CRestore(void);
u32 I2C_receiveByte(u8 id, u8 address, u8* data);
u32 I2CsendByte(u8 id, u8 address, u8 data);
//u32 I2CReceiveArray(u8 id, u8 start_address, u8* buff, u32 data_len);
u32 I2CReceiveArray(u8 id, u32 start_address, I2C_ADDRESS_LENGTH address_len, u8* buff, u32 data_len, I2C_DATA_INTEGRITY integrity);
u32 I2CReceiveArray1(u8 id, u8 start_address, u8* buff, u32 data_len);

/*Routines for word (16-bit) transactions where data byte low goes first*/
u32 I2CWriteShortLH(u8 id, u8 address, u16 data);
u32 I2CReceiveShortLH(u8 id, u8 address, u16* data, I2C_DATA_INTEGRITY integrity);
u32 I2CWriteShortLHArray(u8 id, u8 address, u16* buff, u32 data_len);
u32 I2CReceiveShortLHArray(u8 id, u8 address, u16* buff, u32 data_len, I2C_DATA_INTEGRITY integrity);

/*The same but data byte high first*/
u32 I2CWriteShortHL(u8 id, u8 address, u16 data);
u32 I2CReceiveShortHL(u8 id, u8 address, u16* data, I2C_DATA_INTEGRITY integrity);
u32 I2CWriteShortHLArray(u8 id, u8 address, u16* buff, u32 data_len);
u32 I2CReceiveShortHLArray(u8 id, u8 address, u16* buff, u32 data_len, I2C_DATA_INTEGRITY integrity);

/*For byte arrays*/
u32 I2CReceiveByteArray(u8 id, u8 address, u8* buff, u32 data_len, I2C_DATA_INTEGRITY integrity);
u32 I2CWriteByteArray(u8 id, u8 address, u8* buff, u32 data_len);

// Для массивов байт с 16 битами адреса
u32 I2CReceiveByteArray_16(u8 id, u16 address, u8* buff, u32 data_len, I2C_DATA_INTEGRITY integrity);
u32 I2CWriteByteArray_16(u8 id, u16 address, u8* buff, u32 data_len);

/*This functions are for devices, that don't have internal registers addresses*/
u32 I2CreadByte(u8 id, u8* data);
u32 I2CwriteByte(u8 id, u8 data);
u32 I2CwriteByteADG728(u8 id, u8 data);
u32 I2CDeviceACK(u8 id);

//internal
u32 i2c_wait_byte_transfer(BYTE_WAIT_TYPE type);
u32 i2c_wait_while_bus_busy();

void i2c_GPIO_reset(void);

//test
u32 I2CgetAcknowledge(u8 id);

extern void* I2CRESOURCE_ID;

#endif /* I2C_H_ */
