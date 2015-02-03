/*!
\file T8_Dnepr_LED.h
\brief Код работы со светодиодами лицевой панели Днепра
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date oct 2013
*/

#ifndef __T8_DNEPR_LED_H_
#define __T8_DNEPR_LED_H_

#include "support_common.h"
#include "HAL/IC/inc/NXP_PCA9551.h"

/*! LED colors */
typedef enum __LedColorTypedef{
	NONE,
	GREEN,
	RED,
	YELLOW
} T8_Dnepr_LedColorTypedef ;

/*! Settings for a single LED. */
typedef struct __LedTypedef{
	T8_Dnepr_LedColorTypedef mColor; /*!< LED color */
	_BOOL                    bBlink; /*!< Blink state */	
} T8_Dnepr_LedTypedef ;

/*! Settings for all LEDs. */
typedef struct __LedStatusTypedef{
	T8_Dnepr_LedTypedef tPower; /*!< POWER LED */
	T8_Dnepr_LedTypedef tCPU;   /*!< CPU LED */
	T8_Dnepr_LedTypedef tAlarm; /*!< ALARM LED */
} T8_Dnepr_LedStatusTypedef ;

_BOOL T8_Dnepr_Led_Init();
_BOOL T8_Dnepr_SetLedStatus(T8_Dnepr_LedStatusTypedef* tLedStatusStructure);

#endif
