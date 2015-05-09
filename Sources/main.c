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
#include "Threads/inc/threadTerminal.h"
#include "Threads/inc/threadMeasure.h"
#include "Threads/inc/threadWatchdog.h"
#include "Threads/inc/threadDeviceController.h"
#include "Threads/inc/thread_snmp.h"

#include "reserv/core/inc/rsrv_main_stmch.h"


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
static OS_STK  task_eth_stk[512];
//#pragma data_alignment=4
//static OS_STK  task_snmp_stk[512];
#pragma data_alignment=4
static OS_STK  task_rsrv_stk[512];
#pragma data_alignment=4
static OS_STK  task_mcumcu_stk[512];


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
    MCF_WTM_WCR &= (0xFFFF - 0x1);

    // Init Trap15 -- для ОС
    MCU_ConfigureIntr(INTR_ID_GPT_C3, 4, 1);
    // Enable Trap15
    MCU_EnableIntr(INTR_ID_GPT_C3, 1);
    __enable_interrupts();


    /* File system init */
    // FS_Init();

    // остальные задачи создаются внутри taskInit
    assert(OSTaskCreateExt(taskInit, (void *)0, (void *)&taskInitStk[255], taskInit_PRIO, taskInit_PRIO, (void *)&taskInitStk, 256, NULL, OS_TASK_OPT_STK_CHK) == OS_ERR_NONE);
    OSTaskNameSet(taskInit_PRIO, "taskInit", &return_code);
    assert(return_code == OS_ERR_NONE);
    //OSTaskCreate(taskFlash_COMM, (void *)0, (void *)&taskFlash_COMMStk[255], taskFlash_COMM_PRIO );

    OSStart();    // Start multitasking (i.e. give control to uC/OS-II)
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// задача, в которой инициализируются остальные задачи

static void taskInit(void *pdata)
{
    INT8U   return_code = OS_ERR_NONE;

    pdata = pdata;

    // инициализируем системный таймер и задачи

    // As mentioned in the book's text, you MUST initialize the ticker only
    // once multitasking
    // Включаем ticker -- таймер ОС
    PIT_Init(0, // номер таймера
        1, // делитель PCSR
        18750, // модуль таймера, 75 МГц -> 1 кГц
        3, 0 // IPL, prio
        );
    PIT_Init(1,
        7, //tick = 128/75 мкс
        65535,
        3, 1);

    OSStatInit();


    ////////////////////////////////////////////////////////////////////////////
    // инициализируем переферию тут, чтобы работал таймер ОС (для таймаута в I2C)

    T8_Dnepr_TS_Init();
    // устанавливаем выводы и gpio
    DNEPR_PinsInit();
    /* Инициализация программных модулей i2c, spi, mdio, свитчи, hotswap, eeprom backplane'а... */
    DNEPR_InitPeripherials();

    // задерживаем включение не работающих слотовых устройств
    DeviceController_Init();

    // Инициализуруем параметры профиля.
    Dnepr_ProfileStorage_Init();
    
//    /* пытаемся обнаружить backplane eeprom с данными профиля в течении 4 секунд, после этого все равно включаемся */
//    dnepr_wait_eeprom_contact(4 * OS_TICKS_PER_SEC);

    {
        /* сторожевой таймер (запуск, периодический сброс) */
        assert(OSTaskCreateExt(taskWatchdog, (void *)0, (void *)&taskWatchdogStk[127], taskWDT_PRIO, taskWDT_PRIO, (void *)&taskWatchdogStk, 128, NULL, OS_TASK_OPT_STK_CHK) == OS_ERR_NONE);
        OSTaskNameSet(taskWDT_PRIO, "taskWatchdog", &return_code);
        assert(return_code == OS_ERR_NONE);
    }

    assert(OSTaskCreateExt(taskMeasure, (void *)0, (void *)&taskMeasureStk[1023], taskMeasure_PRIO, taskMeasure_PRIO, (void *)&taskMeasureStk, 1024, NULL, OS_TASK_OPT_STK_CHK) == OS_ERR_NONE);
    OSTaskNameSet(taskMeasure_PRIO, "taskMeasure", &return_code);
    assert(return_code == OS_ERR_NONE);

    assert(OSTaskCreateExt(taskCU, (void *)0, (void *)&taskCUStk[1023], taskCU_PRIO, taskCU_PRIO, (void *)&taskCUStk, 1024, NULL, OS_TASK_OPT_STK_CHK) == OS_ERR_NONE);
    OSTaskNameSet(taskCU_PRIO, "taskCU", &return_code);
    assert(return_code == OS_ERR_NONE);

    assert(OSTaskCreateExt(taskDeviceController, (void *)0, (void *)&taskDControllerStk[1023], tackDController_PRIO, tackDController_PRIO, (void *)&taskDControllerStk, 1024, NULL, OS_TASK_OPT_STK_CHK) == OS_ERR_NONE);
    OSTaskNameSet(tackDController_PRIO, "taskDeviceController", &return_code);
    assert(return_code == OS_ERR_NONE);

    {
        /* сетевые функции CU */
        assert(OSTaskCreateExt(task_eth, (void *)0, (void *)&task_eth_stk[511], TASK_ETH_PRIORITY, TASK_ETH_PRIORITY, (void *)&task_eth_stk, 512, NULL, OS_TASK_OPT_STK_CHK) == OS_ERR_NONE);
        OSTaskNameSet(TASK_ETH_PRIORITY, "task_eth", &return_code);
        assert(return_code == OS_ERR_NONE);
        
//        assert(OSTaskCreateExt(task_snmp, (void *)0, (void *)&task_snmp_stk[511], TASK_SNMP_PRIORITY, TASK_SNMP_PRIORITY, (void *)&task_snmp_stk, 512, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
//        OSTaskNameSet( TASK_SNMP_PRIORITY, "task_snmp", &return_code ) ;
//        assert( return_code == OS_ERR_NONE ) ;

//        assert(OSTaskCreateExt(task_rstp, (void *)0, (void *)&taskNetStk[511], taskNet_PRIO, taskNet_PRIO, (void *)&taskNetStk, 512, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
//        OSTaskNameSet( taskNet_PRIO, "task_vlan_rstp", &return_code ) ;
//        assert( return_code == OS_ERR_NONE ) ;
    }
    
    {
        assert(OSTaskCreateExt(task_reserv, (void *)0, (void *)&task_rsrv_stk[511], TASK_RSRV_PRIORITY, TASK_RSRV_PRIORITY, (void *)&task_rsrv_stk, 512, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
        OSTaskNameSet( TASK_RSRV_PRIORITY, "task_reserv", &return_code ) ;
        assert( return_code == OS_ERR_NONE ) ;
        
        assert(OSTaskCreateExt(task_mcumcu, (void *)0, (void *)&task_mcumcu_stk[511], TASK_RSRV_MCUMCU_PRIORITY, TASK_RSRV_MCUMCU_PRIORITY, (void *)&task_mcumcu_stk, 512, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE) ;
        OSTaskNameSet( TASK_RSRV_MCUMCU_PRIORITY, "task_mcumcu", &return_code ) ;
        assert( return_code == OS_ERR_NONE ) ;        
    }
    
#ifdef DEBUG_TERMINAL
    assert(OSTaskCreateExt(task_terminal, (void *)0, (void *)&task_terminal_stack[511], TASKTERM_COMM_PRIO, TASKTERM_COMM_PRIO, (void *)&task_terminal_stack, 512, NULL, OS_TASK_OPT_STK_CHK) == OS_ERR_NONE);
    OSTaskNameSet(TASKTERM_COMM_PRIO, "task_terminal", &return_code);
    assert(return_code == OS_ERR_NONE);
#endif

    OSTaskDel(OS_PRIO_SELF);
}
