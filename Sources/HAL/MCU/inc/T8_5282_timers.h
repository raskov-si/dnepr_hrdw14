/*!
\file T8_5282_timers.h
\brief Driver for MCF5282 internal timers, based on NEVA's mcf5282.c
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#ifndef MCF5282_PIT_H_
#define MCF5282_PIT_H_

#include "support_common.h"

#define PITTIME_MICROSECONDS_PER_TICK(a)    ((a * 128 * 2) / 75)  /*!< количество микросекунд в одном такте таймера  */
#define PITTIME_TICK_PER_MICROSECONDS(a)    ((a * 75) / (128 * 2) )      /*!< количество тактов таймера в одной микросекунде */
#define PITTIME_OVL_SECONDS                 PITTIME_MICROSECONDS_PER_TICK(65535) / 1000000


void PIT_Init(	const u8 PIT, const u8 PCSR, const u16 PMR,
				const u16 irq_level, const u16 irq_priority);
u16 pit_get_value (const u8 PIT);

void HWait( const u32 usec );



#endif /* MCF5282_PIT_H_ */
