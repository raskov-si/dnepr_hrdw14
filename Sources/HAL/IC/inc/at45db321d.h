/*!
\file AT_AT45DB321D.h
\brief Драйвер AT45DB321D
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date nov 2012
*/

#ifndef __AT_45DB321D
#define __AT_45DB321D

#include "support_common.h"
#include "HAL\IC\inc\SPI_Interface.h"

/*!
\defgroup AT_45DB321D_Driver
\{
*/

//! адрес сектора и смещение в секторе, используется для задания wAddr
#define AT_AT45DB321D_ADDRESS(SECTOR,SHIFT)	(((SECTOR << 10) & 0x7FFC00) | (SHIFT & 0x3F))

void AT_AT45DB321D_InitPeripherialInterface(SPI_PeriphInterfaceTypedef* tSpiPeriphInterface);

//! Код производителя, устройства и т.п., читается из самой микросхемы
typedef struct AT_AT45DB321D_JEDEC__ {
	u8 manufacturer ;	//!< 0x1F для Atmel, м.б. больше 1го байта, но в этом драйвере не рассматривается
	u8 family_code ; 	//!< 1 -- Data Flash
	u8 density_code ;	//!< кол-во бит
	u8 mlc_code ;		//!< 
	u8 prod_version ;	//!< 
} AT_AT45DB321D_JEDEC ;

#define AT_AT45DB321D_SECTOR_LEN	528
#define AT_AT45DB321D_DATA_LEN		512

//! Возвращает регистр статуса
u16 AT_AT45DB321D_ReadStatus();
//! заполняет структуру JEDEC данными из микросхемы
void AT_AT45DB321D_ReadJEDEC( AT_AT45DB321D_JEDEC *pJEDEC );

//! Читает данные напрямую из флеш-памяти в обход обоих буферов, максимум 528 байт (на 512 -- 1.16 мс)
void AT_AT45DB321D_DirectReadBytes( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber );
//! Заполняет первый буфер (на 512 байт 1.16 мс)
void AT_AT45DB321D_WriteBuffer1( const u32 wAddress, u8* anBytesToWrite, u16 nBytesNumber );
//! Заполняет второй буфер
void AT_AT45DB321D_WriteBuffer2( const u32 wAddress, u8* anBytesToWrite, u16 nBytesNumber );
//! Пишет содержимое первого буфера по адресу wAddr (10.49 мс)
void AT_AT45DB321D_ProgrammBuffer1( const u32 wAddr );
//! Пишет содержимое второго буфера по адресу wAddr
void AT_AT45DB321D_ProgrammBuffer2( const u32 wAddr );
//! Читает содержимое 1го буфера 
void AT_AT45DB321D_ReadBuff1( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber );
//! Читает содержимое 2го буфера 
void AT_AT45DB321D_ReadBuff2( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber );
//! стирает блок (8 страниц) номер nBlock
void AT_AT45DB321D_ErraseBlock( const u32 nBlock );

//! Проверяет готовность флеш-памяти
_BOOL AT_AT45DB321D_ReadyToWrite();

/*!
\}
*/

#endif
