/*!
\file I2C_interface.h
\brief Базовый требуемый интерфейс микросхем с I2C 
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 29.05.2012
*/

#ifndef I2C_INTERFACE_H_
#define I2C_INTERFACE_H_
#include <time.h>
#include "support_common.h"

struct __I2C_PeriphInterfaceTypedef;
typedef struct __I2C_PeriphInterfaceTypedef I2C_PeriphInterfaceTypedef ;
struct __I2C_PeriphInterfaceTypedef{
	_BOOL (*I2C_ReadByte) 			(I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *pbResult, clock_t pause_interval, u8 times);
	_BOOL (*I2C_ReadWord)			(I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 *pwResult, clock_t pause_interval, u8 times);
	_BOOL (*I2C_WriteByte)                  (I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 nData, clock_t pause_interval, u8 times);
	_BOOL (*I2C_WriteWord)          (I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 nData, clock_t pause_interval, u8 times);
	_BOOL (*I2C_SendCommand)        (I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, clock_t pause_interval, u8 times);
	_BOOL (*I2C_ReadCommand)		(I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 *pbResult, clock_t pause_interval, u8 times);
	_BOOL (*I2C_ReadMultipleBytes)  (I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*I2C_WriteMultipleBytes) (I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*I2C_ReadMultipleWords)  (I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*I2C_WriteMultipleWords) (I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);

	_BOOL (*I2C_ReadMultipleBytes_16)	(I2C_PeriphInterfaceTypedef *p, u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);
	_BOOL (*I2C_WriteMultipleBytes_16) 	(I2C_PeriphInterfaceTypedef *p, u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times);

	void *bus_info ; //!< локальная информация для функций I2C
} ;

#endif /* I2C_INTERFACE_H_ */
