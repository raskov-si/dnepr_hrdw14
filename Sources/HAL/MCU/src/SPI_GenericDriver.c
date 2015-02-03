/*!
\file SPI_GenericDriver.c
\brief Generic SPI driver.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 08.06.2012
*/

#include "HAL\MCU\inc\T8_5282_spi.h"
#include "HAL\MCU\inc\SPI_GenericDriver.h"

/* In ColdfFire CS bita are inverted (i.e. "0"="enable"). */
#define SPI_CONVERT_CS(b) ((~b)&0x0F)

_BOOL SPI_DeInitPorts(){
	return 1;
}

_BOOL SPI_ByteTransaction(u8 mChipSelect, u8* anDataToSend, u8* anDataToGet, u16 nBytes){

	if(nBytes==0) return 1;

	/* Normally, just one function for any number of bytes would be necessary, but QSPIRWLongArray()'s timings have yet to be optimized. */
	if(nBytes<=16){
		QSPIRWArray((u8)SPI_CONVERT_CS(mChipSelect), (u8*)(anDataToSend), (u8*)(anDataToGet), nBytes);
	}
	else{
		QSPIRWLongArray((u8)SPI_CONVERT_CS(mChipSelect), (u8*)(anDataToSend), (u8*)(anDataToGet), nBytes);
	}
	
/*	QSPIRWLongArray((u8)SPI_CONVERT_CS(mChipSelect), (u8*)(anDataToSend), (u8*)(anDataToGet), nBuffSize); */

	return 1;
}
