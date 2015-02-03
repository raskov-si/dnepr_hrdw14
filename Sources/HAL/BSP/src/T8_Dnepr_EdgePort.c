/*!
\file T8_Dnepr_EdgePort.c
\brief Модуль с обработчиками внешних прерываний (ПЛИС, PMBus, свитчи Marvell)
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/

#include "support_common.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "Threads/inc/threadDeviceController.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_EdgePortInit()
{
	// разрешаем оговоренные прерывания, см. комментарии в prio.h
	MCU_ConfigureIntr( INTR_ID_EPF1, 1, 5 );
	MCU_EnableIntr( INTR_ID_EPF1, TRUE );
	MCU_ConfigureIntr( INTR_ID_EPF3, 1, 5 );
	MCU_EnableIntr( INTR_ID_EPF3, TRUE );
	MCU_ConfigureIntr( INTR_ID_EPF4, 1, 5 );
	MCU_EnableIntr( INTR_ID_EPF4, TRUE );

	MCF_EPORT_EPPAR =
		// MCF_EPORT_EPPAR_EPPA1_FALLING 	|	// SFP Int
		MCF_EPORT_EPPAR_EPPA3_FALLING  	|	// FPGA
		MCF_EPORT_EPPAR_EPPA4_FALLING		// PMBus Alert от backplane'а
		//MCF_EPORT_EPPAR_EPPA5_FALLING | 	// Switch 1
		//MCF_EPORT_EPPAR_EPPA6_FALLING  	// Switch 2
		;

	MCF_EPORT_EPDDR = 0 ;
	MCF_EPORT_EPFR = 0xFE ; // сбрасываем флаг
	MCF_EPORT_EPIER = MCF_EPORT_EPIER_EPIE3 | MCF_EPORT_EPIER_EPIE4 ; //| MCF_EPORT_EPIER_EPIE5 ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL Dnepr_EdgePort_is_IRQ4_active()
{
	return (MCF_EPORT_EPPDR & MCF_EPORT_EPPDR_EPPD4) == 0 ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// SFP Int
void isr_EdgePortISR1()
{
	MCF_EPORT_EPFR |= 0x02 ;
	Dnepr_DControl_SFP_Interrupt() ;
}

void isr_EdgePortISR2()
{
	MCF_EPORT_EPFR |= 0x04 ;
}

// FPGA
void isr_EdgePortISR3()
{
	MCF_EPORT_EPFR |= 0x08 ;
	if( (MCF_EPORT_EPPDR & 0x08) == 0 ){
		Dnepr_DControl_Present_Interrupt() ;
	}
}

// External or Internal PMBus through I2C Switch
void isr_EdgePortISR4()
{
	MCF_EPORT_EPFR |= 0x10 ;
	Dnepr_DControl_PMBusAlarm() ;
}

// Switch 1
void isr_EdgePortISR5()
{
	MCF_EPORT_EPFR |= 0x20 ;
}

// Switch 2
void isr_EdgePortISR6()
{
	MCF_EPORT_EPFR |= 0x40 ;
}

void isr_EdgePortISR7()
{
	MCF_EPORT_EPFR |= 0x80 ;
}
