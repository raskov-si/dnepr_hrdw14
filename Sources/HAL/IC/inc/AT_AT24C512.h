/*!
\file AT_AT24C512.h
\brief ��������� ��� ������ �� ������������ EEPROM �� ���� i2c
\author <a href="mailto:leonov@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#ifndef __AT_AT24C512_H_
#define __AT_AT24C512_H_

#include "support_common.h"
#include "HAL/IC/inc/I2C_interface.h"

//! \brief �������������� ��������� ������� � ����������
//! \param tI2CPeriphInterface	��������� ����������� ������� � ����������
//! \param devAddr ����� ���������� �� i2c ����
void AT_AT24C512_InitPeripherialInterface(I2C_PeriphInterfaceTypedef* tI2CPeriphInterface, const u8 devAddr );

_BOOL AT_AT24C512_ReadArray( I2C_PeriphInterfaceTypedef *tI2CPeriphInterface, const u8 devAddr, size_t addr, u8* pData, size_t len );
_BOOL AT_AT24C512_WriteArray( I2C_PeriphInterfaceTypedef *tI2CPeriphInterface, const u8 devAddr, size_t addr, u8* pData, size_t len );

#endif
