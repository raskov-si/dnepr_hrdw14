/*!
\file ST_M95M01.h
\brief Driver for eeprom M95M01
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date sep 2012
*/

#ifndef __ST_M95M01_
#define __ST_M95M01_ 

#include "support_common.h"
#include "HAL\IC\inc\SPI_Interface.h"

/*!
\defgroup ST_M95M01_Driver
\{
*/

#define ST_M95M01_PAGESIZE	256 //!< размер сектора в байтах

/*!
\brief Inits driver's peripherial interface.
\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
\param tSpiPeriphInterface Pointer to SPI_FpgaPeriphInterfaceTypedef structure.
\param chip_select number of SPI Chip Select line.
\warning This driver does not copy tSpiPeriphInterface fields, it stores only this pointer.
\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
*/	
void ST_M95M01_InitPeripherialInterface( const SPI_PeriphInterfaceTypedef* tSpiPeriphInterface );

/*!
\brief Reads array of bytes from EEPROM, unable detect any error
\param dest pointer to dectination array
\param address lower 17 bits are address [0 0x1FFFF]
\param length is the number of bytes to read
*/	
void ST_M95M01_ReadArray( u8* dest, const size_t address, const size_t length );
/*!
\brief Write array of bytes to EEPROM's page.
\param src pointer to array to write
\param address lower 17 bits are address [0 0x1FFFF]
\param length is the number of bytes to write, must not be longer than 256 bytes
*/
void ST_M95M01_WriteArray( const u8* src, const size_t address, size_t length );
/*!
\brief Returns status register from EEPROM
*/
u8 ST_M95M01_ReadStatus();

//! \brief Returns TRUE if device is accessible
_BOOL ST_M95M01_CheckPresence();

void ST_M95M01_WriteEnable(void);
void ST_M95M01_WriteDisable(void);


/*!
\}
*/ // ST_M95M01_Driver

#endif
