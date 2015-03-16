/*
 * File:		support_common.h
 * Purpose:		Various project configurations.
 *
 * Notes:
 */

#ifndef _SUPPORT_COMMON_H_
#define _SUPPORT_COMMON_H_

/* Common standard types */
#include "ctypes.h"

/*
 * Include the derivative header files
 */
#include "io5282.h"

/* Binary macros */
#include "Binary.h"

/* Assert */
#include "ErrorManagement/assert.h"
#include "ErrorManagement/status_codes.h"

#include "T8_Atomiccode.h"

#define SYSTEM_CLOCK_KHZ  75000     /* system bus frequency in kHz */

/********************************************************************/

// #define __DEBUG			//!< ���� ��������� �� �� ����� �� flash assert � �� ������ BPEEPROM
#define __NWATCHDOG			//!< ���� ��������� �� �������� watchdog
#define WDT_PERIOD_MS					5000

//! ������-�� � IAR'� �� ��������
#define round(x)	((x >= 0) ? (int)(x + 0.5) : (int)(x - 0.5))

#define ADDRESS			u32
#define INSTRUCTION		u16
#define ILLEGAL			0x4AFC
#define CPU_WORD_SIZE		16

#ifndef NULL
#define NULL			0
#endif //NULL

#endif /* _SUPPORT_COMMON_H_ */
