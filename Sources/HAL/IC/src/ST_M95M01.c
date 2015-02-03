/*!
\file ST_M95M01.h
\brief Driver for eeprom M95M01
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date sep 2012
*/

#include "HAL\IC\inc\ST_M95M01.h"
#include "HAL\IC\inc\SPI_Interface.h"
#include <string.h> /* memmove(), etc. */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Pointer to peripherial access stricture and a \c define assigned to it */
const static  SPI_PeriphInterfaceTypedef* tSpiEEPROMInterfaceStructure = NULL;
#define SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE tSpiEEPROMInterfaceStructure
#define __SPI_GenericByteTransaction ( *SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction )


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void __Write_Array(const u8* src, const size_t address, const size_t length );
static void __WriteEnable();
static void __WriteDisable();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! Write Enable
#define __M95_WREN_COMM	 		0x06
//! Write Disable
#define __M95_WRDI_COMM	 		0x04
//! Read Status Register
#define __M95_RDSR_COMM	 		0x05
//! Write Status Register
#define __M95_WRSR_COMM	 		0x01
//! Read from Memory Array
#define __M95_READ_COMM	 		0x03
//! Write to Memory Array
#define __M95_WRITE_COMM	 	0x02

//! биты регистра статуса
#define _M95_STATUS_WIP			0x01	//!< Write in Progress bit
#define _M95_STATUS_WEL			0x02	//!< Write enable latch

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
\addtogroup ST_M95M01_Driver
\{
*/

/*!
\defgroup ST_M95M01_Driver_Exported_Functions
\{
*/

void ST_M95M01_InitPeripherialInterface( const SPI_PeriphInterfaceTypedef* tSpiPeriphInterface )
{
	assert( tSpiPeriphInterface != NULL );
	SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE = tSpiPeriphInterface;
	assert( SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE != NULL );
}

void ST_M95M01_ReadArray( u8* dest, const size_t address, const size_t length )
{
	// FIXME: мы здесь килобайт стэка выкидываем -- правим драйвер spi
	u8 anSendBuffer[ 4 ];
	u8 anReceiveBuffer[ ST_M95M01_PAGESIZE + 4 ];
	assert( SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE != NULL );

	anSendBuffer[0] = __M95_READ_COMM ;
	anSendBuffer[1] = _BYTE(address, 2);
	anSendBuffer[2] = _BYTE(address, 1);
	anSendBuffer[3] = _BYTE(address, 0);

	SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction( anSendBuffer, 4, anReceiveBuffer, length + 4 );
	memcpy( dest, anReceiveBuffer+4, length );
}

void ST_M95M01_WriteArray( const u8* src, const size_t address, size_t length )
{
	size_t len1, len2 ;
	size_t addr1, addr2 ;

	if( length > 256 ){
		length = 256 ;
	}

	// если данные при записи пересекают границу страницы в 256 байт
	if( ((address + length - 1) & 0xFFFFFF00) != (address & 0xFFFFFF00 )){
		addr1 = address ;
		addr2 = (address + length) & 0xFFFFFF00 ;
		len1 = addr2 - addr1 ;
		len2 = length - len1 ;

		if( (len1 & 0x0000000F) == 0x0C ){
			__Write_Array( src, addr1, 4 );
			__Write_Array( src + 4, addr1 + 4, len1 - 4 );
		} else {
			__Write_Array( src, addr1, len1 );
		}
		
		if( (len2 & 0x0000000F ) == 0x0C ){
			__Write_Array( src + len1, addr2, 4 );
			__Write_Array( src + len1 + 4, addr2 + 4, len2 - 4 );
		} else {
			__Write_Array( src + len1, addr2, len2 );
		}
	// данные при записи находятся в рамках одной страницы
	} else {
		if( (length & 0x0000000F) == 0x0C ){
			__Write_Array( src, address, 4 );
			__Write_Array( src + 4, address + 4, length - 4 );
		} else {
			__Write_Array( src, address, length );
		}
	}
}

u8 ST_M95M01_ReadStatus()
{
	u8 anSendBuffer[1], anReceiveBuffer[2] ;
	assert( SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE != NULL );
	anSendBuffer[0] = __M95_RDSR_COMM ;

	tSpiEEPROMInterfaceStructure->SPI_GenericByteTransaction( anSendBuffer, 1, anReceiveBuffer, 2 );
	return anReceiveBuffer[1] ;
}

_BOOL ST_M95M01_CheckPresence()
{
	u8 status ;
	assert( SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE != NULL );
	__WriteEnable();
	status = ST_M95M01_ReadStatus();
	__WriteDisable();
	return ((status & _M95_STATUS_WEL) > 0) && (status != 0xFF) ;
}

/*!
\}
*/ //ST_M95M01_Driver_Exported_Functions

static void __Write_Array(const u8* src, const size_t address, const size_t length )
{
	// FIXME: мы здесь килобайт стэка выкидываем -- правим драйвер spi
	u8 anSendBuffer[ST_M95M01_PAGESIZE+4] ;
	size_t nActualByteNumber, i ;
	assert( SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE != NULL );

	anSendBuffer[0] = __M95_WRITE_COMM ;
	anSendBuffer[1] = _BYTE(address, 2);
	anSendBuffer[2] = _BYTE(address, 1);
	anSendBuffer[3] = _BYTE(address, 0);

	nActualByteNumber = MIN( length, ST_M95M01_PAGESIZE ) ;
	memcpy( anSendBuffer+4, src, nActualByteNumber );

	__WriteEnable() ;
	tSpiEEPROMInterfaceStructure->SPI_GenericByteTransaction( anSendBuffer, nActualByteNumber+4,
																NULL, 0 );
	__WriteDisable();

	// ждём окончания записи (в д.ш. 5 мс), но не больше прибл. 20 мс
	i = 0 ;
	while( (ST_M95M01_ReadStatus() & _M95_STATUS_WIP) && (i++ < 1000) );
}

static void __WriteEnable()
{
	u8 anSendBuffer, anReceiveBuffer ;
	assert( SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE != NULL );
	
	anSendBuffer = __M95_WREN_COMM ;
	tSpiEEPROMInterfaceStructure->SPI_GenericByteTransaction( &anSendBuffer, 1, &anReceiveBuffer, 1 );
}

static void __WriteDisable()
{
	u8 anSendBuffer, anReceiveBuffer ;
	assert( SPI_EEPROM_PERIPH_INTERFACE_STRUCTURE != NULL );
	
	anSendBuffer = __M95_WRDI_COMM ;
	tSpiEEPROMInterfaceStructure->SPI_GenericByteTransaction( &anSendBuffer, 1, &anReceiveBuffer, 1 );
}

/*!
\}
*/ //ST_M95M01_Driver
