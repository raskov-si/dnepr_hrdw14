/*!
\file threadWatchdog.c
\brief ����� ��� ����������� watchdog'�, ����� ����� ������ ��������� ����� ���������� � idle
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
	// ����������� software watchdog, core watchdog �� ��������
	MCU_WDT_Config wdt_conf = { (u16)(WDT_PERIOD_MS * SYSTEM_CLOCK_KHZ / 8192), MCU_WDT_ENABLE,
											// 111.848 �� �� 75 ���
								FALSE, TRUE, MCU_CWDT_DEL_2P23, FALSE } ;
#endif

	pdata = pdata ;
	OSTimeDly( 300 );
	// �������� Watchdog �����, ������ ��� EEPROM ����������� �����, �������������
	// ��� ������, � ������� � eeprom_storage.[h|c] ������ � ��������� �� �������.
	// �������, ��� ��������� ���-�� ����� ����� ������������� ������, � �� �� �����.
#ifndef __NWATCHDOG
//	wdt_conf.WDT_prescaler = (u16)(WDT_PERIOD_MS * SYSTEM_CLOCK_KHZ / 8192) ;
	wdt_conf.WDT_prescaler = (u16)(10 * SYSTEM_CLOCK_KHZ / 8192) ;
	MCU_WDT_Init( wdt_conf ); // � ����� ������� �������� watchdog
#endif 
	while(TRUE){
		MCU_WDT_Service();
		OSTimeDly(50);
	}
}
