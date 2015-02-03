;************************************************************************************************
;                                     uC/OS-II
;                                The Real-Time Kernel
; 
;                   (c) Copyright 1999, Jean J. Labrosse, Weston, FL
;                                 All Rights Reserved
;  
; File        : OS_CPU_A.S
; By          : Jean J. Labrosse
; Modified    : Mark D. Medonis, Sekidenko, Inc. 6/18/2002
; Modified    : Carlton Heyer, Avnet Design Services 4/28/2004
;************************************************************************************************


;************************************************************************************************
;
; Note: This port uses the movem.l (a7),d0-d7/a0-a6, lea 60(a7)a7	     
; construct in place of the traditional 68xxx movem.l (a7)+,d0-d7/a0-a6. It
; is perfectly in order to push/pop individual registers using movem.l (a7)+,d2 , etc.   
; but it's a bit slower (and more verbose).			    
;								    	    
; The lea instruction is required because the ColdFire can't    	    
; push multiple registers directly to the stack.		    
;
; Changed OSIntCtxSw for new uC/OS-II version 2.51 features. OSTickISR was changed too, to
; deal with the Stack Pointer in a different way than in the 2.04 version. See the uC/OS-II
; release notes for details on how this is accomplished.
;
; rev 0.b
; Fixed _OSStartHighRdy. The original version was popping D0 back off the stack, when D4 had
; been popped there and used for setting OSRunning to 1. Now getting D4 back off the stack as
; it should be.
; M. Medonis
;
; Modified for the CF5282
; Carlton Heyer	    
;*************************************************************************************************

IPSBAR		EQU	0x40000000
_PIF		EQU	0x04
PIT0		EQU	0x00150000
PCSR		EQU	0x0
_PIT0_PCSR	EQU	(IPSBAR + PIT0 + PCSR)

;*************************************************************************************************
;                                   PUBLIC DECLARATIONS
;*************************************************************************************************


	    PUBLIC OSCtxSw
	    PUBLIC OSIntCtxSw
	    PUBLIC OSIntExitCF
	    PUBLIC OSStartHighRdy
	    PUBLIC _OSTickISR
	    ;PUBLIC _CPUFormatError
	    PUBLIC _InitVBR


;**************************************************************************************************
;                                EXTERNAL DECLARATIONS
;**************************************************************************************************

	EXTERN  OSCtxSwCtr
	EXTERN  OSIntExit
	EXTERN  OSIntNesting
	EXTERN  OSLockNesting
	EXTERN  OSPrioCur
	EXTERN  OSPrioHighRdy
	EXTERN  OSRdyGrp
	EXTERN  OSRdyTbl
	EXTERN  OSRunning    
	EXTERN  OSTaskSwHook
	EXTERN  OSTCBCur
	EXTERN  OSTCBHighRdy
	EXTERN  OSTCBPrioTbl
	EXTERN  OSTimeTick
	EXTERN  OSUnMapTbl

	EXTERN  OSIntEnter



;*************************************************************************************************
;     VECTOR BASE REGISTER INITIALIZATION
;
; !!! В нашем приложении используется умолчальное значение, эта процедура не вызывается!
; This is to set the Vector Base Register to 0x0, in case the startup bootloader moved
; it somewhere else.
;
;*************************************************************************************************
	RSEG  CODE

_InitVBR:

   MOVE.L #0x00000000,D0
   MOVEC D0, VBR
   RTS



;**************************************************************************************************
;                            START HIGHEST PRIORITY TASK READY-TO-RUN
;
; Description: This function is called by OSStart() to start the highest priority task that was
;              created by your application before calling OSStart().
;
; Arguments  : none
;
; Note(s)    : 1) The stack frame is assumed to look as follows:
;             
;                  OSTCBHighRdy->OSTCBStkPtr +  0  ---->  D0         Low Memory
;                                            +  4         D1 
;                                            +  8         D2
;                                            + 12         D3
;                                            + 16         D4
;                                            + 20         D5
;                                            + 24         D6
;                                            + 28         D7
;                                            + 32         A0
;                                            + 36         A1
;                                            + 40         A2
;                                            + 44         A3
;                                            + 48         A4
;                                            + 52         A5
;                                            + 56         A6
;                                            + 60         Format, Vector, OS_INITIAL_SR
;                                            + 64         task
;                                            + 68         task
;                                            + 72         pdata       High Memory
;
;      2) OSStartHighRdy() MUST:
;            a) Call OSTaskSwHook() then,
;            b) Set OSRunning to TRUE,
;            c) Switch to the highest priority task.
;*************************************************************************************************

OSStartHighRdy:
   JSR      (OSTaskSwHook)        ; Invoke user defined context switch hook

   MOVE.L D4,-(A7)               ;
   MOVEQ.L  #1, D4
   MOVE.B   D4,(OSRunning)        ; Indicate that we are multitasking
   MOVE.L (A7)+, D4              ; Bug here! was D0, now D4

   MOVEA   (OSTCBHighRdy),A1   ; Point to TCB of highest priority task ready to run
   MOVEA   (A1),A7              ; Get the stack pointer of the task to resume

   MOVEM.L  (A7),D0-D7/A0-A6     ; Store all the regs
   LEA      (60,A7),A7			 ; Advance the stack pointer

   RTE                           ; Return to task



;************************************************************************************************
;                                     TASK LEVEL CONTEXT SWITCH
;
; Description : This function is called when a task makes a higher priority task ready-to-run.
; 
; Arguments   : none
;
; Note(s)     : 1) Upon entry,
;                   OSTCBCur     points to the OS_TCB of the task to suspend
;                   OSTCBHighRdy points to the OS_TCB of the task to resume
;
;               2) The stack frame of the task to suspend looks as follows (the registers for
;                  task to suspend need to be saved):
;
;                                         SP +  0  ---->  Format, Vector, SR   Low Memory
;                                            +  4         PC of task           High Memory
;
;               3) The stack frame of the task to resume looks as follows:
; 
;                  OSTCBHighRdy->OSTCBStkPtr +  0  ---->  D0                   Low Memory
;                                            +  4         D1
;                                            +  8         D2
;                                            + 12         D3
;                                            + 16         D4
;                                            + 20         D5
;                                            + 24         D6
;                                            + 28         D7
;                                            + 32         A0
;                                            + 36         A1
;                                            + 40         A2
;                                            + 44         A3
;                                            + 48         A4
;                                            + 52         A5
;                                            + 56         A6
;                                            + 60         OS_INITIAL_SR       (See OS_CPU.H)
;                                            + 64         PC of task           High Memory
;
;*****************************************************************************

OSCtxSw:	
  LEA	   (-60,A7),A7
  MOVEM.L  D0-D7/A0-A6,(A7)     ; Save the registers of the current task

  MOVEA   (OSTCBCur),A1       ; Save the stack pointer in the suspended task TCB
  MOVE.L   A7,(A1)

  JSR      (OSTaskSwHook)        ; Invoke user defined context switch hook

  MOVEA   (OSTCBHighRdy),A1   ; OSTCBCur = OSTCBHighRdy
  MOVE.L   A1,(OSTCBCur)
  MOVEA   (A1),A7			    ; Get the stack pointer of the task to resume
  
  MOVE.B   (OSPrioHighRdy),D0  ; OSPrioCur = OSPrioHighRdy
  MOVE.B   D0,(OSPrioCur)
  
  MOVEM.L  (A7),D0-D7/A0-A6     ; Restore the CPU registers
  LEA	   (60,A7),A7

  RTE                           ; Run task


;******************************************************************************************
;                             INTERRUPT LEVEL CONTEXT SWITCH
;
;
; Description : This function is provided for backward compatibility.
;
; Arguments   : none
;
; Note(s)     : 1) The stack frame upon entry:
;
;                  SP +  0  ---->  Return PC of OSIntCtxSw()         Low Memory
;                     +  4         Return PC of OSIntExit()

;                     +  8         D0    (H)           
;                     + 12         D1    (H)
;                     + 16         D2    (H)
;                     + 20         D3    (H)
;                     + 24         D4    (H)
;                     + 28         D5    (H)
;                     + 32         D6    (H)
;                     + 36         D7    (H)

;                     + 40         A0    (H)
;                     + 44         A1    (H)
;                     + 48         A2    (H)
;                     + 52         A3    (H)
;                     + 56         A4    (H)
;                     + 60         A5    (H)
;                     + 64         A6    (H)

;                     + 68         SR of interrupted task  --   ColdFire SR area is 32 bits.
;                     + 72         PC of task                          High Memory
;********************************************************************************************************

OSIntCtxSw: 
  JSR      (OSTaskSwHook)        ; Invoke user defined context switch hook

  MOVEA   (OSTCBHighRdy),A1   ; OSTCBCur = OSTCBHighRdy
  MOVE.L   A1,(OSTCBCur)
  MOVEA   (A1),A7              ; CPU stack pointer = OSTCBHighRdy->OSTCBStkPtr
  
  MOVE.B   (OSPrioHighRdy),D0  ; OSPrioCur = OSPrioHighRdy
  MOVE.B   D0,(OSPrioCur)
  
  MOVEM.L  (A7),D0-D7/A0-A6     ; Pop all the CPU registers from the new task's stack
  LEA	   (60,A7),A7

  RTE                           ; Run task



;*******************************************************************************************************
;
;
;*******************************************************************************************************

OSIntExitCF:
    MOVE      SR, D0
    ORI.L     #0700,D0             ; Disable interrupts
    MOVE      D0, SR
    
    MOVE.B    (OSIntNesting),D0   
    SUBQ.L    #1,D0                ; OSIntNesting--
    MOVE.B    D0,(OSIntNesting)     ; if (OSIntNesting == 0)
    TST.B     (OSIntNesting)
    BNE       OSIntExitCF_Exit

    TST.B     (OSLockNesting)        ; if (OSLockNesting == 0) 
    BNE       OSIntExitCF_Exit
                    
    MOVEQ.L   #0,D0                ; y = OSUnMapTbl[OSRdyGrp]
    MOVE.B    (OSRdyGrp),D0
    LEA       (OSUnMapTbl),A0
    MOVE.B    (0,A0,D0.L*1),D3         ; y is in D3


    MOVEQ.L   #0,D0                ;
    MOVE.B    D3,D0                ; copy y into D0
    LEA       (OSRdyTbl),A0         ;
    MOVEQ.L   #0,D1                ;
    MOVE.B    (0,A0,D0.L*1),D1         ; D1 = OSRdyTbl[y]
    LEA       (OSUnMapTbl),A0       ; OSPrioHighRdy = (INT8U)((y << 3) + OSUnMapTbl[OSRdyTbl[y]])
    MOVE.B    D3,D0
    LSL.L     #3,D0                ; D0 = (y << 3)
    MOVE.B    (0,A0,D1.L*1),D1         ; D1 = OSUnMapTbl[OSRdyTbl[y]]
    ADD.L     D1,D0                ; 
    MOVE.B    D0,(OSPrioHighRdy)    ;
    
    MOVE.B    (OSPrioHighRdy),D0
    MOVE.B    (OSPrioCur),D1
    ANDI.L    #0xFF,D1
    ANDI.L    #0xFF,D0
    CMP.L     D1,D0                ; if (OSPrioHighRdy != OSPrioCur) { 
    BEQ       OSIntExitCF_Exit
          
    MOVEQ.L   #0,D0                ; OSTCBHighRdy  = OSTCBPrioTbl[OSPrioHighRdy];
    MOVE.B    (OSPrioHighRdy),D0
    LEA       (OSTCBPrioTbl),A0
    MOVEA.L   (0,A0,D0.L*4),A0
    MOVE.L    A0,(OSTCBHighRdy)
    
    ADDQ.L    #1,(OSCtxSwCtr)      ; OSCtxSwCtr++; 
                   
                                  ; PERFORM INTERRUPT LEVEL CONTEXT SWITCH: 
    JSR      (OSTaskSwHook)        ; Invoke user defined context switch hook

    MOVEA.L   (OSTCBHighRdy),A1   ; OSTCBCur = OSTCBHighRdy
    MOVE.L   A1,(OSTCBCur)
    MOVEA.L   (A1),A7              ; CPU stack pointer = OSTCBHighRdy->OSTCBStkPtr
  
    MOVE.B   (OSPrioHighRdy),D0  ; OSPrioCur = OSPrioHighRdy
    MOVE.B   D0,(OSPrioCur)

OSIntExitCF_Exit:
    MOVEM.L  (A7),D0-D7/A0-A6      ; Restore processor registers from stack
    LEA      (60,A7),A7
    RTE                            ; Return to task or nested ISR



;********************************************************************************
;                               SYSTEM TICK ISR
;
; Description : This function is the ISR used to notify uC/OS-II that a system tick has occurred.  
;
; Arguments   : none
;
; Notes       : 1) You MUST increment 'OSIntNesting' and NOT call OSIntEnter()
;               2) You MUST save ALL the CPU registers as shown below
;               3) You MUST JUMP to OSIntExitCF() instead of call the function.
;               4) You MUST NOT use OSIntExit() to exit an ISR with the 68K.
;
;  Procedure : OSTickISR					    
;								    
;  The procedure is installed as the timer tick ISR and called   
;  each time TIMER0 on the MCF5282 reaches it's reference value.
;
;  Notice that all ISRs *must* leave the same stack context
;  at the time of the call to OSIntExit() since if a context
;  switch does occur, OSIntCtxSw() must be able to find this
;  frame.
;*********************************************************************

_OSTickISR:
  LEA      (-60,A7),A7         ; Save processor registers onto stack
  MOVEM.L  D0-D7/A0-A6,(A7)

  MOVEQ.L  #0,D0              ; This use of D0 is absolutely necessary when
  MOVE.B   (OSIntNesting),D0 ; using an optimizing compiler. Since ColdFire
  ADDQ.L   #1,D0              ; can only add Long, whereas OSIntNesting is defined
  MOVE.B   D0,(OSIntNesting) ; as INT8U in C code.

  CMPI.L   #1, d0		      ; if OSIntNesting == 1, then continue
  BNE 	   _SkipSave          ; if Timer interrupts while in another ISR, skip the save
  MOVEA.L   (OSTCBCur), A1    ; Otherwise, save stack pointer onto current task's stack
  MOVE.L   A7,(A1)

 _SkipSave:
   
  MOVEA.L  #_PIT0_PCSR,A0     ; Clear the reference event
  MOVE.W   (A0),D0            ; Again a point of note, since ORI only works with
  ORI.L    #_PIF,D0   		  ; data registers you must do the copy unlike 68k which
  MOVE.W   D0,(A0)            ; has more assembly language options.
 
  JSR      (OSTimeTick)
  JSR      (OSIntExit)        ; Can either jump to C IntExit or use assembly
;  JMP      (OSIntExitCF)       ; language, either method seems to work

  MOVEM.L  (A7),D0-D7/A0-A6   ; Restore processor registers from stack after JSR
  LEA      (60,A7),A7          ; If using JMP to IntExitCF, these commands are
  RTE                         ; skipped and the restore is at the end of IntExitCF
	END

