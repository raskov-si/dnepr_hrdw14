/*!
\file T8_5282_wdt.h
\brief заголовочный файл для модуля watchdog'а
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2012
*/

#include "support_common.h"

#define MCU_WDT_WAIT_STOPPED	0x08
#define MCU_WDT_DOZE_STOPPED	0x04
#define MCU_WDT_HALTED_STOPPED	0x02
#define MCU_WDT_ENABLE			0x01

typedef enum {
	MCU_CWDT_DEL_2P9 	= 0,
	MCU_CWDT_DEL_2P11	= 1,
	MCU_CWDT_DEL_2P13	= 2,
	MCU_CWDT_DEL_2P15	= 3,
	MCU_CWDT_DEL_2P19	= 4,
	MCU_CWDT_DEL_2P23	= 5,
	MCU_CWDT_DEL_2P27	= 6,
	MCU_CWDT_DEL_2P31	= 7
} MCU_CWDT_TimeDelay_t ;

#define MCU_CWDT_CWTA			0x80		
#define MCU_CWDT_ENABLE			0x80
#define MCU_CWDT_ENABLE			0x80

//! \brief настройки модуля watchdog'а
typedef struct __WDT_Config
{
	u16 WDT_prescaler; 	//!< Watchdog Modulus Register
	u16 WDT_control; 	//!< Watchdog Control Register, принимает значения находящихся выше define'ов

	u8 CWDT_Enable; 	//!< 1 -- enable core watchdog timer
	u8 CWDT_interrupt; 	//!< Core watchdog reset/interrupt select
	u8 CWDT_delay;		//!< timedelay, MCU_CWDT_TimeDelay_t
	u8 CWDT_transfer_acknowledge; //!< 1 -- Core watchdog transfer acknowledge enable

} MCU_WDT_Config ;

//! \brief начальная настройка, чем раньше в программе будет вызвана тем лучше
void MCU_WDT_Init( const MCU_WDT_Config conf) ;

//! \brief сбрасывает счетчик wdt, чтобы не произошел reset
void MCU_WDT_Service(void);

//! \brief сбрасывает счетчик core wdt, чтобы не произошло прерывание
void MCU_CWDT_Service(void);
