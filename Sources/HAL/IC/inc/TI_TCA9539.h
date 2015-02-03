/*!
\file TI_TCA9539.h
\brief Драйвер TCA9539
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date oct 2013
*/

#ifndef __TI_TCA9539_H_
#define __TI_TCA9539_H_

#include "support_common.h"
#include "HAL/IC/inc/PMB_interface.h"

//! Прочитать состояние всех выводов (с учетом инверсии).
_BOOL TCA9539_ReadGPIO( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 *value );
//! Записать состояние выводов.
_BOOL TCA9539_WriteGPIO( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value );
//! Инвертировать значение выводов.
_BOOL TCA9539_SetInversion( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value );
//! Установит выводы на чтение (1) или запись (0).
_BOOL TCA9539_SetDirectionRead( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value );

#endif
