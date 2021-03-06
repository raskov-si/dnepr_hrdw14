/*!
\file main.c
\brief main() and all tasks functions
\details main() and all tasks functions, written for iar
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date june 2012
*/

#include <intrinsics.h>

#include <stdio.h>
#include <stdlib.h>

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "prio.h"

#include "common_lib/memory.h"

#include "HAL/MCU/inc/T8_5282_interrupts.h"
#include "HAL/MCU/inc/T8_5282_timers.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_SDRAM.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "Application/inc/T8_Dnepr_ProfileStorage.h"

#include "Threads/inc/inttsk_mgs.h"
#include "Threads/inc/threadCU.h"
#include "Threads/inc/threadTerminal.h"
#include "Threads/inc/threadMeasure.h"
#include "Threads/inc/threadWatchdog.h"
#include "Threads/inc/threadDeviceController.h"
#include <string.h> /* strlen() */

#include "T8_Atomiccode.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma data_alignment=4
static OS_STK  taskCUStk[1024];
#pragma data_alignment=4
static OS_STK  taskWatchdogStk[128];
#pragma data_alignment=4
static OS_STK  taskInitStk[256];
#pragma data_alignment=4
static OS_STK  taskMeasureStk[1024];
#pragma data_alignment=4
static OS_STK  taskDControllerStk[1024];
#pragma data_alignment=4
static OS_STK  task_terminal_stack[512];
#pragma data_alignment=4
static OS_STK  task_shelfmng_stack[512];


static void taskInit(void *pdata);
void taskMeasure(void *pdata);
void taskWatchdog(void *pdata);
void task_shelf_manager(void *pdata);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	INT8U return_code = OS_ERR_NONE;

	////////////////////////////////////////////////////////////////////////////
    // Initialize "uC/OS-II, The Real-Time Kernel"             
    OSInit();  // ������ �����, ������ ��� �������� ������ BSP ����� ����� �������� (�������� FPGA)
	
	MCU_Interrupt_Init();
	MCF_WTM_WCR &= (0xFFFF-0x1);
	
	// Init Trap15 -- ��� ��
	MCU_ConfigureIntr(INTR_ID_GPT_C3,4,1);
	// Enable Trap15
	MCU_EnableIntr(INTR_ID_GPT_C3,1);
	__enable_interrupts();


	/* File system init */
    // FS_Init();
    
    // ��������� ������ ��������� ������ taskInit
    assert(OSTaskCreateExt(taskInit, (void *)0, (void *)&taskInitStk[255], taskInit_PRIO, taskInit_PRIO, (void *)&taskInitStk, 256, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
    OSTaskNameSet( taskInit_PRIO, "taskInit", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;
    //OSTaskCreate(taskFlash_COMM, (void *)0, (void *)&taskFlash_COMMStk[255], taskFlash_COMM_PRIO );

    OSStart();    // Start multitasking (i.e. give control to uC/OS-II)
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ������, � ������� ���������������� ��������� ������

static void taskInit(void *pdata)
{
    INT8U   return_code = OS_ERR_NONE;
    
	pdata = pdata ;

	// �������������� ��������� ������ � ������

	// As mentioned in the book's text, you MUST initialize the ticker only
	// once multitasking
	// �������� ticker -- ������ �� 
	PIT_Init(       0, // ����� �������
			1, // �������� PCSR
			18750, // ������ �������, 75 ��� -> 1 ��� 
			3, 0 // IPL, prio
	);
        
	OSStatInit();
        
        PIT_Init(	1, 
                        7, //tick = 128/75 ���
                        65535,  
			3, 1 );


	////////////////////////////////////////////////////////////////////////////
	// �������������� ��������� ���, ����� ������� ������ �� (��� �������� � I2C)
	
	T8_Dnepr_TS_Init();
	// ������������� ������ � gpio
	DNEPR_PinsInit();
	/* ������������� ����������� ������� i2c, spi, mdio, ������, hotswap, eeprom backplane'�... */
	DNEPR_InitPeripherials();
        
	// ����������� ��������� �� ���������� �������� ���������
	DeviceController_Init() ;
        
        /* �������� ���������� backplane eeprom � ������� ������� � ������� 4 ������, ����� ����� ��� ����� ���������� */
        dnepr_wait_eeprom_contact(4*OS_TICKS_PER_SEC);
        
	// �������������� ��������� �������.
	Dnepr_ProfileStorage_Init() ;
        
        open_inttask_message();
		
    assert(OSTaskCreateExt(taskWatchdog, (void *)0, (void *)&taskWatchdogStk[127], taskWDT_PRIO, taskWDT_PRIO, (void *)&taskWatchdogStk, 128, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
    OSTaskNameSet( taskWDT_PRIO, "taskWatchdog", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;

    assert(OSTaskCreateExt(taskMeasure, (void *)0, (void *)&taskMeasureStk[1023], taskMeasure_PRIO, taskMeasure_PRIO, (void *)&taskMeasureStk, 1024, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE );
    OSTaskNameSet( taskMeasure_PRIO, "taskMeasure", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;

    assert(OSTaskCreateExt(taskCU, (void *)0, (void *)&taskCUStk[1023], taskCU_PRIO, taskCU_PRIO, (void *)&taskCUStk, 1024, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
    OSTaskNameSet( taskCU_PRIO, "taskCU", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;

    assert(OSTaskCreateExt(taskDeviceController, (void *)0, (void *)&taskDControllerStk[1023], tackDController_PRIO, tackDController_PRIO, (void *)&taskDControllerStk, 1024, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
    OSTaskNameSet( tackDController_PRIO, "taskDeviceController", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;
    
    assert(OSTaskCreateExt(task_shelf_manager, (void *)0, (void *)&task_shelfmng_stack[511], PRIO_TASK_SHELFMNG, PRIO_TASK_SHELFMNG, (void *)&task_shelfmng_stack, 512, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
    OSTaskNameSet( PRIO_TASK_SHELFMNG, "task_shelf_manager", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;    
       
#ifdef DEBUG_TERMINAL
    assert(OSTaskCreateExt(task_terminal, (void *)0, (void *)&task_terminal_stack[511], TASKTERM_COMM_PRIO, TASKTERM_COMM_PRIO, (void *)&task_terminal_stack, 512, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
    OSTaskNameSet( TASKTERM_COMM_PRIO, "task_terminal", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;    
#endif
    
    OSTaskDel( OS_PRIO_SELF ) ;
}
