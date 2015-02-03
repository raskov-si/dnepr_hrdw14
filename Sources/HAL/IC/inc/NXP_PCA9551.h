/*!
\file NXP_PCA9551.h
\brief Драйвер PCA9551
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date oct 2013
*/

#ifndef __NXP_PCA9551_H_
#define __NXP_PCA9551_H_

#include "support_common.h"
#include "HAL/IC/inc/PMB_interface.h"

//! Режим работы светодиода.
typedef enum {
	PCA9551_LED_ON 		= 0, //!< Включен.
	PCA9551_LED_OFF 	= 1, //!< Выключен.
	PCA9551_LED_PWM0	= 2, //!< Мигает с соотв. с BLINK0 и PWM0.
	PCA9551_LED_PWM1	= 3 //!< Мигает с соотв. с BLINK1 и PWM1.
} PCA9551_LED_State_t ;

//! Состояния всех светодиодов.
typedef struct
{
	PCA9551_LED_State_t state0 ;
	PCA9551_LED_State_t state1 ;
	PCA9551_LED_State_t state2 ;
	PCA9551_LED_State_t state3 ;
	PCA9551_LED_State_t state4 ;
	PCA9551_LED_State_t state5 ;
	PCA9551_LED_State_t state6 ;
	PCA9551_LED_State_t state7 ;
} PCA9551_LED_States;

//! Устанавливает режим светодиода led_num.
_BOOL PCA9551_SetLedState( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, PCA9551_LED_States *pState );

//! Устанавливает частоту и скважность первого таймер.
_BOOL PCA9551_SetFreq0( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u8 freq, u8 pwm );
//! Устанавливает частоту и скважность второго таймер.
_BOOL PCA9551_SetFreq2( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, u8 freq, u8 pwm );

#endif
