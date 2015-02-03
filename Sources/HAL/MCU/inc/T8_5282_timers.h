/*!
\file T8_5282_timers.h
\brief Driver for MCF5282 internal timers, based on NEVA's mcf5282.c
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#ifndef MCF5282_PIT_H_
#define MCF5282_PIT_H_

#include "support_common.h"

void PIT_Init(	const u8 PIT, const u8 PCSR, const u16 PMR,
				const u16 irq_level, const u16 irq_priority);

void HWait( const u32 usec );

#endif /* MCF5282_PIT_H_ */
