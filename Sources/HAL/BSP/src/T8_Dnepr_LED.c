/*!
\file T8_Dnepr_LED.c
\brief Код работы со светодиодами лицевой панели Днепра
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date oct 2013
*/


#include "support_common.h"
#include "HAL/BSP/inc/T8_Dnepr_LED.h"
#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/IC/inc/NXP_PCA9551.h"

#define PCA9551_addr 	0xC4

static PCA9551_LED_States __led_states ;


_BOOL T8_Dnepr_Led_Init()
{
	// первый таймер 4 Гц, скважность 50%
	return PCA9551_SetFreq0( Dnepr_I2C_Get_PMBUS_INT_Driver(), PCA9551_addr, 25, 128 );
}

static void __set_led_state( PCA9551_LED_State_t* green, PCA9551_LED_State_t* red, T8_Dnepr_LedColorTypedef color, _BOOL bBlink)
{
	// светодиод выключен
	if( color == NONE ){
		*green = PCA9551_LED_OFF ;
		*red = PCA9551_LED_OFF ;
		return ;
	}
	// горит зелёный
	if( (color == GREEN) || (color == YELLOW) ){
		// мигает
		if( bBlink ){
			*green = PCA9551_LED_PWM0 ;
		// горит постоянно
		} else {
			*green = PCA9551_LED_ON ;
		}
	} else {
		*green = PCA9551_LED_OFF ;
	}
	// горит красный
	if( (color == RED) || (color == YELLOW) ){
		// мигает
		if( bBlink ){
			*red = PCA9551_LED_PWM0 ;
		// горит постоянно
		} else {
			*red = PCA9551_LED_ON ;
		}
	} else {
		*red = PCA9551_LED_OFF ;
	}
}

_BOOL T8_Dnepr_SetLedStatus(T8_Dnepr_LedStatusTypedef* tLedStatusStructure)
{
	__led_states.state0 = PCA9551_LED_OFF ;
	__led_states.state4 = PCA9551_LED_OFF ;
	__set_led_state( &__led_states.state1, &__led_states.state2,
			tLedStatusStructure->tPower.mColor, tLedStatusStructure->tPower.bBlink );
	__set_led_state( &__led_states.state3, &__led_states.state5,
			tLedStatusStructure->tCPU.mColor, tLedStatusStructure->tCPU.bBlink );
	__set_led_state( &__led_states.state6, &__led_states.state7,
			tLedStatusStructure->tAlarm.mColor, tLedStatusStructure->tAlarm.bBlink );

	return PCA9551_SetLedState( Dnepr_I2C_Get_PMBUS_INT_Driver(), PCA9551_addr, &__led_states );
}
