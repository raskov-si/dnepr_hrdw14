/*!
\file Periph_Interface.h
\brief Базовый требуемый интерфейс микросхем с SPI 
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 29.05.2012
*/

#ifndef SPI_INTERFACE_H_
#define SPI_INTERFACE_H_

#include "support_common.h"

typedef struct __SPI_PeriphInterfaceTypedef{
	_BOOL (*SPI_GenericByteTransaction)( 	u8* anDataToSend, u32 tx_len, 
											u8* anDataToGet, u32 rx_len );
}SPI_PeriphInterfaceTypedef;

#endif /* SPI_INTERFACE_H_ */
