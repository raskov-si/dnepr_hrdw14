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

#include "Threads/inc/threadCU.h"
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

static void taskInit(void *pdata);
void taskMeasure(void *pdata);
void taskWatchdog(void *pdata);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	INT8U return_code = OS_ERR_NONE;

	////////////////////////////////////////////////////////////////////////////
    // Initialize "uC/OS-II, The Real-Time Kernel"             
    OSInit();  // делаем здесь, потому что драйверы уровня BSP могут иметь мьютексы (например FPGA)
	
	MCU_Interrupt_Init();
	MCF_WTM_WCR &= (0xFFFF-0x1);
	
	// Init Trap15 -- для ОС
	MCU_ConfigureIntr(INTR_ID_GPT_C3,4,1);
	// Enable Trap15
	MCU_EnableIntr(INTR_ID_GPT_C3,1);
	__enable_interrupts();


	/* File system init */
    // FS_Init();
    
    // остальные задачи создаются внутри taskInit
    assert(OSTaskCreateExt(taskInit, (void *)0, (void *)&taskInitStk[255], taskInit_PRIO, taskInit_PRIO, (void *)&taskInitStk, 256, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
    OSTaskNameSet( taskInit_PRIO, "taskInit", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;
    //OSTaskCreate(taskFlash_COMM, (void *)0, (void *)&taskFlash_COMMStk[255], taskFlash_COMM_PRIO );

    OSStart();    // Start multitasking (i.e. give control to uC/OS-II)
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// задача, в которой инициализируются остальные задачи

static void taskInit(void *pdata)
{
    INT8U return_code = OS_ERR_NONE;

	pdata = pdata ;

	// инициализируем системный таймер и задачи

	// As mentioned in the book's text, you MUST initialize the ticker only
	// once multitasking
	// Включаем ticker -- таймер ОС 
	PIT_Init(0, // номер таймера
			1, // делитель PCSR
			18750, // модуль таймера, 75 МГц -> 1 кГц 
			3, 0 // IPL, prio
	);
	
	OSStatInit() ;

	////////////////////////////////////////////////////////////////////////////
	// инициализируем переферию тут, чтобы работал таймер ОС (для таймаута в I2C)
	
	T8_Dnepr_TS_Init();
	// устанавливаем выводы и gpio
	DNEPR_PinsInit();
	// i2c, spi, mdio, свитчи, hotswap, eeprom backplane'а...
	DNEPR_InitPeripherials();
	// задерживаем включение не работающих слотовых устройств
	DeviceController_Init() ;
	// Инициализуруем параметры профиля.
	Dnepr_ProfileStorage_Init() ;
		
	assert(OSTaskCreateExt(taskWatchdog, (void *)0, (void *)&taskWatchdogStk[127], taskWDT_PRIO, taskWDT_PRIO, (void *)&taskWatchdogStk, 127, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
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

    OSTaskDel( OS_PRIO_SELF ) ;
}
