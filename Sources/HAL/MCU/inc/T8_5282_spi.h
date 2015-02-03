//--------------------------------------------------------------------------
//-| FILENAME: spi.h
//-|
//-| Created on: 25.11.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#ifndef SPI_H_
#define SPI_H_

#include "support_common.h"
#include "ErrorManagement\status_codes.h"
#include "ctypes.h"

#include "HAL\MCU\inc\T8_5282_mcf5282.h"

void QSPI_Init();
void QSPI_Init();
_BOOL QSPI_isr();
void QSPI_ReadWriteArray( const u8 cur_cs, const u8 *tx_data, const size_t tx_len,
						u8 *rx_data, const size_t rx_len,
						const u8 divider, _BOOL cpol, _BOOL cpha  );

#define TRANSMIT_RAM 0x0
#define RECEIVE_RAM 0x10
#define COMMAND_RAM 0x20

#define M23K256_BYTEMODE 0x0
#define M23K256_PAGEMODE 0x02
#define M23K256_SEQUENTIALMODE 0x01

void QSPIInit();
void QSPIInit_Flash( const u8 delay, const u32 devider );
u8 QSPIReadByte(u8 cs);
void QSPIWriteByte(u8 cs, u8 data);
void QSPIWriteByte_wait(u8 cs, u8 data);
void QSPIWriteArray(u8 cs, u8* data, u32 data_length);
void QSPIRWArray(u8 cs, u8* data_in_spi, u8* data_from_spi, u32 data_length);
void QSPIRWLongArray(u8 cs, u8* data_in_spi, u8* data_from_spi, u32 data_length);

void AT25DF321WriteArray(u32 addr, const u8* data, u32 data_length);
void AT25DF321ReadArray(u32 addr, u8* data, u32 data_length);

void M23K256ReadArray(u32, u8*, u32);
void M23K256WriteArray(u32, const u8*, u32);
u32 M23K256Clear(u32 addr, u32 len);


#endif /* SPI_H_ */
