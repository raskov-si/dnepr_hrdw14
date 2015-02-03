/*
 * File:		support_common.h
 * Purpose:		Various project configurations.
 *
 * Notes:
 */

#ifndef _SUPPORT_COMMON_H_
#define _SUPPORT_COMMON_H_

/*
 * Include the derivative header files
 */
#include "io5282.h"

/* Common standard types */
#include "ctypes.h"

/* Binary macros */
#include "Binary.h"

/* Assert */
#include "ErrorManagement/assert.h"
#include "ErrorManagement/status_codes.h"

#include "T8_Atomiccode.h"

#define SYSTEM_CLOCK_KHZ  75000     /* system bus frequency in kHz */

/********************************************************************/

// #define __DEBUG			//!< если объявлена не не пишет во flash assert и не читает BPEEPROM
#define __NWATCHDOG			//!< если объявлена не работает watchdog
#define WDT_PERIOD_MS					5000

//! Почему-то в IAR'е не определён
#define round(x)	((x >= 0) ? (int)(x + 0.5) : (int)(x - 0.5))

#define ADDRESS			u32
typedef	u32			size_t;
#define INSTRUCTION		u16
#define ILLEGAL			0x4AFC
#define CPU_WORD_SIZE		16

#ifndef NULL
#define NULL			0
#endif //NULL

#endif /* _SUPPORT_COMMON_H_ */
