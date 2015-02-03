/*!
\file TI_TCA9539.c
\brief Драйвер TCA9539
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date oct 2013
*/

#include "HAL/IC/inc/TI_TCA9539.h"

typedef enum {
	INPUT0 		= 0,
	INPUT1 		= 1,
	OUTPUT0 	= 2,
	OUTPUT1 	= 3,
	INVERSION0 	= 4,
	INVERSION1 	= 5,
	CONFIG0 	= 6,
	CONFIG1 	= 7
} REGISTER ;

_BOOL TCA9539_ReadGPIO( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 *value )
{
	_BOOL ret ;
	u8 in0, in1 ;
	assert( tPmbusPeriphInterface );
	assert( value );

	ret = tPmbusPeriphInterface->PMB_ReadByte( tPmbusPeriphInterface, nAddress, INPUT0, &in0 );
	ret = ret && tPmbusPeriphInterface->PMB_ReadByte( tPmbusPeriphInterface, nAddress, INPUT1, &in1 );

	*value = in0 | (in1 << 8) ;
	return ret ;
}

_BOOL TCA9539_WriteGPIO( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value )
{
	_BOOL ret ;
	assert( tPmbusPeriphInterface );

	ret = tPmbusPeriphInterface->PMB_WriteByte( tPmbusPeriphInterface, nAddress, OUTPUT0, value & 0xFF );
	ret = ret && tPmbusPeriphInterface->PMB_WriteByte( tPmbusPeriphInterface, nAddress, OUTPUT1, (value >> 8) & 0xFF );

	return ret ;	
}

_BOOL TCA9539_SetInversion( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value )
{
	_BOOL ret ;
	assert( tPmbusPeriphInterface );

	ret = tPmbusPeriphInterface->PMB_WriteByte( tPmbusPeriphInterface, nAddress, INVERSION0, value & 0xFF );
	ret = ret && tPmbusPeriphInterface->PMB_WriteByte( tPmbusPeriphInterface, nAddress, INVERSION1, (value >> 8) & 0xFF );

	return ret ;	
}

_BOOL TCA9539_SetDirectionRead( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u16 value )
{
	_BOOL ret ;
	assert( tPmbusPeriphInterface );

	ret = tPmbusPeriphInterface->PMB_WriteByte( tPmbusPeriphInterface, nAddress, CONFIG0, value & 0xFF );
	ret = ret && tPmbusPeriphInterface->PMB_WriteByte( tPmbusPeriphInterface, nAddress, CONFIG1, (value >> 8) & 0xFF );

	return ret ;	
}

