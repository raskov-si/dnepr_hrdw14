/*!
\file NXP_PCA9551.c
\brief Драйвер PCA9551
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date oct 2013
*/

#include "HAL/IC/inc/NXP_PCA9551.h"

typedef enum {
	INPUT = 0,
	PSC0 = 1,
	PWM0 = 2,
	PSC1 = 3,
	PWM1 = 4,
	LS0 = 5,
	LS1 = 6
} PCA9551_REG_SELECTOR ;
static _BOOL __write_register( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, PCA9551_REG_SELECTOR reg, u8 value );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL PCA9551_SetLedState( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, PCA9551_LED_States *pState )
{
	_BOOL ret ;
	u8 ls0, ls1 ;
	assert( tPmbusPeriphInterface );

	ls0 = pState->state0 | (pState->state1 << 2) | (pState->state2 << 4) | (pState->state3 << 6) ;
	ls1 = pState->state4 | (pState->state5 << 2) | (pState->state6 << 4) | (pState->state7 << 6) ;
	ret = __write_register( tPmbusPeriphInterface, nAddress, LS0, ls0 );
	ret = ret && __write_register( tPmbusPeriphInterface, nAddress, LS1, ls1 );

	return ret ;
}

_BOOL PCA9551_SetFreq0( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u8 freq, u8 pwm )
{
	_BOOL ret ;
	assert( tPmbusPeriphInterface );
	ret = __write_register( tPmbusPeriphInterface, nAddress, PSC0, freq );
	ret = ret && __write_register( tPmbusPeriphInterface, nAddress, PWM0, pwm );

	return ret ;
}

_BOOL PCA9551_SetFreq2( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u8 freq, u8 pwm )
{
	_BOOL ret ;
	assert( tPmbusPeriphInterface );
	ret = __write_register( tPmbusPeriphInterface, nAddress, PSC1, freq );
	ret = ret && __write_register( tPmbusPeriphInterface, nAddress, PWM1, pwm );

	return ret ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static _BOOL __write_register( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, PCA9551_REG_SELECTOR reg, u8 value )
{
	return tPmbusPeriphInterface->PMB_WriteByte( tPmbusPeriphInterface, nAddress, (u8)reg, value );
}
