/*
*********************************************************************************************************
*                                               uC/OS-II
*                                         The Real-Time Kernel
*
*                            (c) Copyright 2000, Jean J. Labrosse, Weston, FL
*                                          All Rights Reserved
*
*
*                                          MCF5272 Specific code
*
* File         : OS_CPU_C.C
* Modified By  : Mark Medonis
*                Sekidenko Inc - An Advanced Energy Company
*                mark.medonis@aei.com
*                4/16/2002
* Modified by  : Carlton Heyer
*				 Avnet Design Services
*				 carlton.heyer@avnet.com
*				 4/28/2004
*********************************************************************************************************
*/

#include "OS_CPU.H"
#include "OS_CFG.H"

#include "ucos_ii.H"

/*
*********************************************************************************************************
*                                           REVISION HISTORY
* Rev 0.a
* Initial verison. Added new function OSTaskIdleHook, for version 2.51 of uC/OS.
* M. Medonis 4/16/2002
*
* Added LED indicator for the Idle task
* Carlton Heyer 4/28/2004
* $Log$
*      
*********************************************************************************************************
*/



/*$PAGE*/
/*
*********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*
* Description: This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
*              stack frame of the task being created.  This function is highly processor specific.
*
* Arguments  : task          is a pointer to the task code
*
*              pdata         is a pointer to a user supplied data area that will be passed to the task
*                            when the task first executes.
*
*              ptos          is a pointer to the top of stack.  It is assumed that 'ptos' points to
*                            a 'free' entry on the task stack.  If OS_STK_GROWTH is set to 1 then 
*                            'ptos' will contain the HIGHEST valid address of the stack.  Similarly, if
*                            OS_STK_GROWTH is set to 0, the 'ptos' will contains the LOWEST valid address
*                            of the stack.
*
*              opt           specifies options that can be used to alter the behavior of OSTaskStkInit().
*                            (see uCOS_II.H for OS_TASK_OPT_???).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) The initial value of the Status Register (SR) is OS_INITIAL_SR sets the 683xx processor
*                 to run in SUPERVISOR mode.  It is assumed that all uC/OS-II tasks run in supervisor
*                 mode.
*              2) You can pass the above options in the 'opt' argument.  You MUST only use the upper
*                 8 bits of 'opt' because the lower bits are reserved by uC/OS-II.  If you make changes
*                 to the code below, you will need to ensure that it doesn't affect the behaviour of
*                 OSTaskIdle() and OSTaskStat().
*              3) Registers are initialized to make them easy to differentiate with a debugger.
*********************************************************************************************************
*/

OS_STK *OSTaskStkInit (void (*task)(void *pd), void *pdata, OS_STK *ptos, INT16U opt)
{
    INT32U  *pstk32;
    INT32U  stkframe;
    opt       = opt;                                  /* 'opt' is not used, prevent compiler warning   */

    pstk32    = (INT32U *)((INT32U)ptos);
    stkframe  = 0x40000000;                           /* Start setting up format field of status word  */

    switch( (INT32U)ptos & 0x00000003 )               /* Align the stack on a longword boundary        */
    {                                                    
#define LOFFSETBY(src, o) (OS_STK *)((INT32U)(src) + (o))
     case 0:                              
     pstk32 = LOFFSETBY(pstk32, -0);
     break;
     
     case 1:        
     pstk32 = LOFFSETBY(pstk32, -1);
     break;
     
     case 2:       
     pstk32 = LOFFSETBY(pstk32, -2);
     break;
     
     case 3:
     pstk32 = LOFFSETBY(pstk32, -3);
     break;
#undef LOFFSETBY
    }

    stkframe |= OS_INITIAL_SR;    

    *pstk32 = 0;                                      /* -- SIMULATE CALL TO FUNCTION WITH ARGUMENT -- */
    *--pstk32 = (INT32U)pdata;                        /*    pdata                                      */
    *--pstk32 = (INT32U)task;                         /*    Task return address                        */
                                                      /* ------ SIMULATE INTERRUPT STACK FRAME ------- */
    *--pstk32 = (INT32U)task;                         /*    Task return address                        */
    *--pstk32 = (INT32U)stkframe;                     /* format and status register                    */
                                                      /* ------- SAVE ALL PROCESSOR REGISTERS -------- */
    *--pstk32 = (INT32U)0x00A600A6L;                  /* Register A6                                   */
    *--pstk32 = (INT32U)0x00A500A5L;                  /* Register A5                                   */
    *--pstk32 = (INT32U)0x00A400A4L;                  /* Register A4                                   */
    *--pstk32 = (INT32U)0x00A300A3L;                  /* Register A3                                   */
    *--pstk32 = (INT32U)0x00A200A2L;                  /* Register A2                                   */
    *--pstk32 = (INT32U)0x00A100A1L;                  /* Register A1                                   */
    *--pstk32 = (INT32U)0x00A000A0L;                  /* Register A0                                   */
    *--pstk32 = (INT32U)0x00D700D7L;                  /* Register D7                                   */
    *--pstk32 = (INT32U)0x00D600D6L;                  /* Register D6                                   */
    *--pstk32 = (INT32U)0x00D500D5L;                  /* Register D5                                   */
    *--pstk32 = (INT32U)0x00D400D4L;                  /* Register D4                                   */
    *--pstk32 = (INT32U)0x00D300D3L;                  /* Register D3                                   */
    *--pstk32 = (INT32U)0x00D200D2L;                  /* Register D2                                   */
    *--pstk32 = (INT32U)0x00D100D1L;                  /* Register D1                                   */
    *--pstk32 = (INT32U)0x00D000D0L;                  /* Register D0                                   */
    return ((OS_STK *)pstk32);                        /* Return pointer to new top-of-stack            */
}

/*$PAGE*/
#if OS_CPU_HOOKS_EN
/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*                                            (BEGINNING)
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts should be disabled during this call.
*********************************************************************************************************
*/
#if OS_VERSION > 203
void OSInitHookBegin (void)
{
}
#endif

/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*                                               (END)
*
* Description: This function is called by OSInit() at the end of OSInit().
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts should be disabled during this call.
*********************************************************************************************************
*/
#if OS_VERSION > 203
void OSInitHookEnd (void)
{
}
#endif


/*
*********************************************************************************************************
*                                          TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : ptcb   is a pointer to the task control block of the task being created.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/
void OSTaskCreateHook (OS_TCB *ptcb)
{
    ptcb = ptcb;                       /* Prevent compiler warning                                     */
}


/*
*********************************************************************************************************
*                                           TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/
void OSTaskDelHook (OS_TCB *ptcb)
{
    ptcb = ptcb;                       /* Prevent compiler warning                                     */
}

/*
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                 will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the 
*                 task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/
void OSTaskSwHook (void)
{
}

/*
*********************************************************************************************************
*                                           STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-II's statistics task.  This allows your 
*              application to add functionality to the statistics task.
*
* Arguments  : none
*********************************************************************************************************
*/
void OSTaskStatHook (void)
{
}

/*
*********************************************************************************************************
*                                               TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/
volatile unsigned long long llUptime = 0 ;
void OSTimeTickHook (void)
{
    llUptime++ ;
}

/*
*********************************************************************************************************
*                                           OSTCBInit() HOOK
*
* Description: This function is called by OSTCBInit() after setting up most of the TCB.
*
* Arguments  : ptcb    is a pointer to the TCB of the task being created.
*
* Note(s)    : 1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/
#if OS_VERSION > 203
void OSTCBInitHook (OS_TCB *ptcb)
{
    ptcb = ptcb;                                           /* Prevent Compiler warning                 */
}
#endif


/*
*********************************************************************************************************
*                                             IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do  
*              such things as STOP the CPU to conserve power.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are enabled during this call.
*********************************************************************************************************
*/
#if OS_VERSION >= 251
#define	IDLE_LED		0x80
void OSTaskIdleHook (void)
{
#ifdef OS_CRITICAL_METHOD == 3
	OS_CPU_SR  cpu_sr = 0u;
#endif
	if(!(OSIdleCtr % 50000))
	{
		OS_ENTER_CRITICAL();
		OS_EXIT_CRITICAL();
	}
}
#endif
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                             GET ISR VECTOR
*
* Description: This function is called to get the address of the exception handler specified by 'vect'.
*              It is assumed that the VBR (Vector Base Register) is set to 0x00000000.
*
* Arguments  : vect     is the vector number
*
* Note(s)    : 1) Interrupts are disabled during this call
*              2) It is assumed that the VBR (Vector Base Register) is set to 0x00000000.
*********************************************************************************************************
*/
void *OSVectGet (INT8U vect)
{
    void *addr;
#ifdef OS_CRITICAL_METHOD == 3
	OS_CPU_SR  cpu_sr = 0u;
#endif
    
    
    OS_ENTER_CRITICAL();
    addr = (void *)(*(INT32U *)((INT16U)vect * 4));
    OS_EXIT_CRITICAL();
    return (addr);
}

/*
*********************************************************************************************************
*                                             SET ISR VECTOR
*
* Description: This function is called to set the contents of an exception vector.  The function assumes
*              that the VBR (Vector Base Register) is set to 0x00000000.
*
* Arguments  : vect     is the vector number
*              addr     is the address of the ISR handler
*
* Note(s)    : 1) Interrupts are disabled during this call
*********************************************************************************************************
*/
void OSVectSet (INT8U vect, void (*addr)(void))
{
    INT32U *pvect;
#ifdef OS_CRITICAL_METHOD == 3
	OS_CPU_SR  cpu_sr = 0u;
#endif
    
    pvect = (INT32U *)((INT16U)vect * 4);
    OS_ENTER_CRITICAL();
    *pvect = (INT32U)(addr);
    OS_EXIT_CRITICAL();
}
