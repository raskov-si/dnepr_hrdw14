/*!
\file SPI_GenericDriver.h
\brief Header for generic SPI driver.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 08.06.2012
*/

#ifndef __SPI_GENERIC_DRIVER_H
#define __SPI_GENERIC_DRIVER_H

#include "support_common.h"
//#include "HAL/IC/inc/SPI_interface.h"

/*!
\addtogroup Peripherial_Driver
\{
*/


/*!
\defgroup Peripherial_General_Functions
\{
*/

/*!
\brief De-inits the peripherials.
\retval 1.
*/
_BOOL SPI_DeInitPorts();

/*!
\}
*/ //Peripherial_General_Functions

/*!
\defgroup SPI_Hardware_Level_Functions
\{
*/

/*!
\brief Generic SPI transaction. Sends nBytes bytes from anDataToSend array to MOSI line and gets nBytes from MISO line to an anDataToGet array.
\param mChipSelect Number of SPI Chip Select line.
\param anDataToSend Array of bytes to send.
\param anDataToGet Array of received bytes.
\param nBytes Number of transaction bytes.
\retval 1 if the transaction was successful, 0 otherwise.
\note Bytes are writen and read simultaneously, i.e. if you need to send 1-byte command to read 2-byte data, you need two 3-byte arrays and \p nBytes = 3.
*/
_BOOL SPI_ByteTransaction(u8 mChipSelect, u8* anDataToSend, u8* anDataToGet, u16 nBytes);

/*!
\}
*/ //SPI_HAL_Functions

/*!
\}
*/ //Peripherial_Driver

#endif //__SPI_GENERIC_DRIVER_H
