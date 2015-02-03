/*
 * File:		mcf5282_sysinit.h
 * Purpose:		Generic Power-on Reset configuration
 *
 * Notes:
 *
 */

#ifndef __M5282EVB_SYSINIT_H__
#define __M5282EVB_SYSINIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_CLOCK_KHZ  75000     /* system bus frequency in kHz */




/********************************************************************/
/* __initialize_hardware Startup code routine
 * 
 * __initialize_hardware is called by the startup code right after reset, 
 * with interrupt disabled and SP pre-set to a valid memory area.
 * Here you should initialize memory and some peripherics;
 * at this point global variables are not initialized yet.
 * The startup code will initialize SP on return of this function.
 */
void __initialize_hardware(void);

/********************************************************************/
/* __initialize_system Startup code routine
 * 
 * __initialize_system is called by the startup code when all languages 
 * specific initialization are done to allow additional hardware setup.
 */ 
void __initialize_system(void);

void pll_init();
void wtm_init();
void scm_init();

#ifdef __cplusplus
}
#endif

#endif /* __M5282EVB_SYSINIT_H__ */


