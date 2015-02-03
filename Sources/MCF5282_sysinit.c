
/*
 * File:		mcf5282_sysinit.c
 * Purpose:		Reset configuration of the MCF5282, здесь конфигурируется самое базовое железо: watchdog, pll, SystemControlModule
 */

#include "support_common.h"
#include "exceptions.h"

/********************************************************************/

#ifdef __cplusplus
#pragma cplusplus off
#endif // __cplusplus

/********************************************************************/
void 
wtm_init()
{
	/*
	 * Disable Software Watchdog Timer
	 */
	MCF_WTM_WCR = 0;
}
/********************************************************************/
void 
pll_init()
{
	/*
	 * external clock -- mfd не имеет значения, rfd делаем 000 (делится на 1) 
	 */
	MCF_CLOCK_SYNCR = MCF_CLOCK_SYNCR_RFD(0);
}
/****************************************************************/
void
scm_init()
{
	/* 
	 * Enable on-chip modules to access internal SRAM 
	 */
	MCF_SCM_RAMBAR = (0 
		|	MCF_SCM_RAMBAR_BA(RAMBAR_ADDRESS)
		|	MCF_SCM_RAMBAR_BDE);
}

/********************************************************************/


void __initialize_hardware(void)
{
	asm
	{
		/* Initialize IPSBAR */
		move.l	#(__IPSBAR + 1),d0
		move.l	d0,0x40000000
		
		/* Initialize FLASHBAR: locate internal Flash and validate it */
		/* Initialize RAMBAR0: This is the FLASHBAR */
		/** this sets bit 6 of the FLASHBAR.  Bit 6 is not documented, however,
		***	Freescale states that it is a workaround for the FLASH speculative
		*** read issues. See Errata data for the PCF5282, mask 0L95M. ***/ 
		move.l	#(__FLASHBAR + 0x161),d0
	    movec d0,FLASHBAR
	}

    MCF_PAD_PBCDPAR = 0xC0; /* Get access to SDRAM */

	wtm_init();
	pll_init();
	scm_init();
	
	
	initialize_exceptions();
}
