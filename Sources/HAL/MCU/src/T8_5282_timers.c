/*!
\file MCF5282_timers.c
\brief Driver for MCF5282 internal timers
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date may 2012
*/

/********************************************************************/

#include "support_common.h"	// типы данных
#include "HAL/MCU/inc/T8_5282_timers.h"

//! \brief инициализирует регистры таймера номер PIT, разрешает соотв. прерывание
void PIT_Init(	const u8 PIT, const u8 PCSR, const u16 PMR,
				const u16 irq_level, const u16 irq_priority )
{
	/* Set tic for timers	*/
	if (PIT>3)
		return;
	/* Divide system clock/2 by 2^PCSR	*/
	MCF_PIT_PCSR(PIT) = MCF_PIT_PCSR_OVW | (u16)(MCF_PIT_PCSR_PRE(PCSR));
	MCF_INTC0_ICR(55+PIT) = (u8)(MCF_INTC_ICR_IL(irq_level) | MCF_INTC_ICR_IP(irq_priority));
	MCF_INTC0_IMRH &= ~(MCF_INTC_IMRH_INT_MASK55<<PIT); // гр€зный хак: прерывани€ от 4х таймеров имеют соседние номера

	MCF_PIT_PMR(PIT) = PMR; 	/* modulo count	*/
	MCF_PIT_PCSR(PIT) |= MCF_PIT_PCSR_OVW|MCF_PIT_PCSR_PIE|MCF_PIT_PCSR_PIF|MCF_PIT_PCSR_RLD|MCF_PIT_PCSR_EN;
}


//! \brief ѕросто цикл дл€ ожидани€ usec микросекунд 
void HWait(const u32 usec)
{
	s32 i = (s32)((SYSTEM_CLOCK_KHZ/1000) * usec) ;
	for(; i > 0; i--){}
}
