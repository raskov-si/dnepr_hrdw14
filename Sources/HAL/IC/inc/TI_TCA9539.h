/*!
\file TI_TCA9539.h
\brief ������� TCA9539
\author <a href="mailto:baranovm@t8.ru">������� �. �.</a>
\date oct 2013
*/

#ifndef __TI_TCA9539_H_
#define __TI_TCA9539_H_

#include "support_common.h"
#include "HAL/IC/inc/PMB_interface.h"

//! ��������� ��������� ���� ������� (� ������ ��������).
_BOOL TCA9539_ReadGPIO( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 *value );
//! �������� ��������� �������.
_BOOL TCA9539_WriteGPIO( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value );
//! ������������� �������� �������.
_BOOL TCA9539_SetInversion( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value );
//! ��������� ������ �� ������ (1) ��� ������ (0).
_BOOL TCA9539_SetDirectionRead( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value );

#endif
