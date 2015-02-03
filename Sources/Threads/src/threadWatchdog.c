/*!
\file threadWatchdog.c
\brief поток дл€ перезапуска watchdog'а, имеет самый низкий приоритет после статистики и idle
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2012
*/

#include "support_common.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "HAL/MCU/inc/T8_5282_wdt.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void taskWatchdog(void *pdata)
{
#ifndef __NWATCHDOG	
	// настраиваем software watchdog, core watchdog не включаем
	MCU_WDT_Config wdt_conf = { (u16)(WDT_PERIOD_MS * SYSTEM_CLOCK_KHZ / 8192), MCU_WDT_ENABLE,
											// 111.848 мс на 75 ћ√ц
								FALSE, TRUE, MCU_CWDT_DEL_2P23, FALSE } ;
#endif

	pdata = pdata ;
	OSTimeDly( 300 );
	// ¬ключаем Watchdog здесь, потому что EEPROM провер€етс€ долго, форматируетс€
	// ешЄ дольше, а вносить в eeprom_storage.[h|c] работу с вотчдогом не хочетс€.
	// —читаем, что повиснуть что-то может после инициализации железа, а не во врем€.
#ifndef __NWATCHDOG
//	wdt_conf.WDT_prescaler = (u16)(WDT_PERIOD_MS * SYSTEM_CLOCK_KHZ / 8192) ;
	wdt_conf.WDT_prescaler = (u16)(10 * SYSTEM_CLOCK_KHZ / 8192) ;
	MCU_WDT_Init( wdt_conf ); // с этого момента стартует watchdog
#endif 
	while(TRUE){
		MCU_WDT_Service();
		OSTimeDly(50);
	}
}
