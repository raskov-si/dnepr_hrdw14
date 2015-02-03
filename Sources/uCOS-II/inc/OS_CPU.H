/*
*********************************************************************************************************
*                                               uC/OS-II
*                                        The Real-Time Kernel
*
*                            (c) Copyright 2000, Jean J. Labrosse, Weston, FL
*                                          All Rights Reserved
*
*                                         MCF5272 Specific code
*                                      Metrowerks CodeWarrior C V2.5
*
* File         : OS_CPU.H
* By           : Jean J. Labrosse
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           REVISION HISTORY
* Modified 3/20/2002
* Verified for MCF5272 uP.
* M. Medonis
*
* Modified 7/17/2001
* Changed OS_STK to unsigned int, for the 32 bit stack of the 68328
* M. Medonis
*
* $Log$
*      
*********************************************************************************************************
*/

/*$PAGE*/
/*
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*/

#include "support_common.h"
#include "HAL/MCU/inc/T8_5282_interrupts.h"

// TODO: Ќадо найти где эти типы определены и вставить сюда нужный инклюд

#pragma diag_remark=Pe301

typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   int   INT32S;                   /* Signed   32 bit quantity                           */
typedef float          FP32;                     /* Single precision floating point                    */
typedef double         FP64;                     /* Double precision floating point                    */

#define BYTE           INT8S                     /* Define data types for backward compatibility ...   */
#define UBYTE          INT8U                     /* ... to uC/OS V1.xx                                 */
#define WORD           INT16S
#define UWORD          INT16U
#define LONG           INT32S
#define ULONG          INT32U

typedef INT32U         OS_STK;                   /* Each stack entry is 32-bit wide                         */
typedef INT32U 		   OS_CPU_SR;                /* Define size of CPU status register, 32 bits on ColdFire */

//#pragma warning on (10122)

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#ifndef  FALSE
#define  FALSE    0
#endif

#ifndef  TRUE
#define  TRUE     1
#endif

/* 
*********************************************************************************************************
*                             Motorola ColdFire MCF5272 Inline Assembly
*
* #1:  Disable/Enable interrupts using simple instructions.  After critical section, interrupts
*      will be enabled even if they were disabled before entering the critical section.  
*
* #2:  Software interrupt using TRAP instruction. 
*
*      Note the particular formatting, necessary for inline assembly used with the preprocessor of
*      CodeWarrior.
*********************************************************************************************************
*/
#include <intrinsics.h>
#define  OS_CRITICAL_METHOD    3

#if      OS_CRITICAL_METHOD == 1
#define  OS_ENTER_CRITICAL()  START_ATOMIC()

#define  OS_EXIT_CRITICAL()   STOP_ATOMIC()

#elif 		OS_CRITICAL_METHOD == 3

#define  OS_ENTER_CRITICAL()  {cpu_sr = __get_status_register(); __disable_interrupts();}

#define  OS_EXIT_CRITICAL()  	{__set_status_register(cpu_sr );}
#endif



/* send Trap #15 software interrupt	*/
#define  OS_TASK_SW()     asm("     \
  				TRAP #15; \
                          ")
                          
#define  OS_STK_GROWTH        1                        /* Define stack growth: 1 = Down, 0 = Up   */


/* 
*********************************************************************************************************
*                                           Motorola 68000
*********************************************************************************************************
*/

/* Disable interrupts           */
#define  CPU_INT_DIS()  asm {  \
             MOVE.L D0,-(A7);  \
             MOVE SR, D0;      \
             ORI.L %0x0700,D0; \
             MOVE D0, SR;      \
             MOVE.L (A7)+, D0; \
            }

/* Enable  interrupts           */
#define  CPU_INT_EN()   asm {   \
             MOVE.L D0,-(A7);   \
             MOVE SR, D0;       \
             ANDI.L %0xF0FF,D0; \
             MOVE D0, SR;       \
             MOVE.L (A7)+, D0;  \
            }

#define  OS_INITIAL_SR        0x2000                        /* Supervisor mode, interrupts enabled     */

#define  OS_TRAP_NBR              15                        /* OSCtxSw() invoked through TRAP #15      */

void     OSVectSet(INT8U vect, void (*addr)(void));
void    *OSVectGet(INT8U vect);

// ќпределены в OS_CPU_A.s
void          OSStartHighRdy          (void);
void          OSIntCtxSw              (void);
void          OSCtxSw                 (void);
