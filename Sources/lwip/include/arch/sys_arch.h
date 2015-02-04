/*!
\file sys_arch.h
\brief поток для работы сети -- RSTP и TCP/UDP/IP
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jun 2014
*/

#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#ifdef	__cplusplus
    extern "C" {
#endif
      
#include "support_common.h"

//#include "OS_CPU.H"
//#include "OS_CFG.H"
#include "uCOS_II.H"

#define LWIP_STK_SIZE                  1200
#define LWIP_TASK_MAX                  5
#define LWIP_START_PRIO                5     //so priority of lwip tasks is from 5-9
#define MAX_QUEUES                     35    // 
#define MAX_QUEUE_ENTRIES              35    // number of mboxs
#define SYS_MBOX_NULL                  (void*)0
#define SYS_SEM_NULL                   (void*)0


typedef struct
{
    OS_EVENT *pQ;
    void     *pvQEntries[MAX_QUEUE_ENTRIES];
} TQ_DESCR,  *PQ_DESCR;

typedef OS_EVENT        sys_sem_t;
typedef TQ_DESCR        sys_mbox_t;
typedef OS_CPU_SR       sys_prot_t;
typedef INT8U           sys_thread_t;
typedef void (*lwip_thread_fn)(void *arg);


//typedef OS_EVENT sys_sem_t ;
//typedef OS_EVENT sys_mbox_t ;
//
//typedef u8 sys_thread_t ;
//


#ifdef	__cplusplus
    }
#endif

#endif	/* __SYS_ARCH_H__ */