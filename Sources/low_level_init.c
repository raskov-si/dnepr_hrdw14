/*-------------------------------------------------------------------------
 *      This module contains the function `__low_level_init', a function
 *      that is called before the `main' function of the program.  Normally
 *      low-level initializations - such as setting the prefered interrupt
 *      level or setting the watchdog - can be performed here.
 *
 *      Note that this function is called before the data segments are
 *      initialized, this means that this function can't rely on the
 *      values of global or static variables.
 *
 *      When this function returns zero, the startup code will inhibit the
 *      initialization of the data segments.  The result is faster startup,
 *      the drawback is that neither global nor static data will be
 *      initialized.
 *
 *-------------------------------------------------------------------------
 *      Copyright (c) 2007-2008 IAR Systems AB.
 *      $Revision: 1095 $
 *-------------------------------------------------------------------------*/

#include "HAL/BSP/inc/T8_Dnepr_SDRAM.h"

#ifdef __cplusplus
extern "C" {
#endif
int __low_level_init ( void );


int __low_level_init ( void )
{
  /*==================================*/
  /*  Initialize hardware.            */
  /*==================================*/

  Dnepr_SDRAM_init( 0x30000000, 75000 );

  /*==================================*/
  /* Choose if segment initialization */
  /* should be done or not.           */
  /* Return: 0 to skip segment init   */
  /*         1 to do segment init     */
  /*==================================*/
  return 1;
}

#ifdef __cplusplus
}
#endif
