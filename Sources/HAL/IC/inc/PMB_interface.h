/*!
\file Periph_Interface.h
\brief Базовый требуемый интерфейс микросхем с PMBus 
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 29.05.2012
*/

#ifndef PMB_INTERFACE_H_
#define PMB_INTERFACE_H_

#include <time.h>
#include "support_common.h"

struct __PMB_PeriphInterfaceTypedef ;
typedef struct __PMB_PeriphInterfaceTypedef PMB_PeriphInterfaceTypedef ;

struct __PMB_PeriphInterfaceTypedef{
	_BOOL (*PMB_ReadByte)			(PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u8 *pbResult, clock_t pause_interval, u8 times);
	_BOOL (*PMB_ReadWord)			(PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u16 *pwResult, clock_t pause_interval, u8 times);
	_BOOL (*PMB_WriteByte)          (PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u8 nData, clock_t pause_interval, u8 times);
	_BOOL (*PMB_WriteWord)          (PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u16 nData, clock_t pause_interval, u8 times);
	_BOOL (*PMB_SendCommand)        (PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, clock_t pause_interval, u8 times);
	_BOOL (*PMB_ReadMultipleBytes)  (PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*PMB_WriteMultipleBytes) (PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*PMB_ReadMultipleWords)  (PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*PMB_WriteMultipleWords) (PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*PMB_GetAcknowledge)     (PMB_PeriphInterfaceTypedef* p, u8 mAddr, clock_t pause_interval, u8 times);
        
	

	void *bus_info ; //!< локальная информация для функций PMBus
} ;

#endif /* PMB_INTERFACE_H_ */
