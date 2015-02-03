/*!
\file threadWatchdog.c
\brief поток для перезапуска watchdog'а, имеет самый низкий приоритет после статистики и idle
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2012
*/

#include "HAL/MCU/inc/T8_5282_wdt.h"

void MCU_WDT_Init( const MCU_WDT_Config conf)
{
	MCF_WTM_WMR = conf.WDT_prescaler ;
	MCF_WTM_WCR = conf.WDT_control ;
}

void MCU_WDT_Service(void)
{
	MCF_WTM_WSR = 0x5555 ;
	MCF_WTM_WSR = 0xAAAA ;

	MCF_SCM_CWSR = MCF_SCM_CWSR_CWSR(0x55) ;
	MCF_SCM_CWSR = MCF_SCM_CWSR_CWSR(0xAA) ;
}
