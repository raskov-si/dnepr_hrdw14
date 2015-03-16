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
#include "uCOS_II.H"
#include "lwip/opt.h" 
           
#define LWIP_STK_SIZE                  MAX(DEFAULT_THREAD_STACKSIZE,TCPIP_THREAD_STACKSIZE)
#define LWIP_TASK_MAX                  5
#define LWIP_START_PRIO                36     
#define MAX_QUEUES                     35
#define MAX_QUEUE_ENTRIES              TCPIP_MBOX_SIZE
#define SYS_MBOX_NULL                  NULL
#define SYS_SEM_NULL                   NULL

      
#if defined(TCPIP_MBOX_SIZE) && TCPIP_MBOX_SIZE > MAX_QUEUE_ENTRIES
# undef MAX_QUEUE_ENTRIES
# define MAX_QUEUE_ENTRIES TCPIP_MBOX_SIZE
#endif

#if defined(DEFAULT_ACCEPTMBOX_SIZE) && DEFAULT_ACCEPTMBOX_SIZE > MAX_QUEUE_ENTRIES
# undef MAX_QUEUE_ENTRIES
# define MAX_QUEUE_ENTRIES DEFAULT_ACCEPTMBOX_SIZE
#endif

#if defined(DEFAULT_RAW_RECVMBOX_SIZE) && DEFAULT_RAW_RECVMBOX_SIZE > MAX_QUEUE_ENTRIES
# undef MAX_QUEUE_ENTRIES
# define MAX_QUEUE_ENTRIES DEFAULT_RAW_RECVMBOX_SIZE
#endif

#if defined(DEFAULT_UDP_RECVMBOX_SIZE) && DEFAULT_UDP_RECVMBOX_SIZE > MAX_QUEUE_ENTRIES
# undef MAX_QUEUE_ENTRIES
# define MAX_QUEUE_ENTRIES DEFAULT_UDP_RECVMBOX_SIZE
#endif

#if defined(DEFAULT_TCP_RECVMBOX_SIZE) && DEFAULT_TCP_RECVMBOX_SIZE > MAX_QUEUE_ENTRIES
# undef MAX_QUEUE_ENTRIES
# define MAX_QUEUE_ENTRIES DEFAULT_TCP_RECVMBOX_SIZE
#endif
      
      
struct _STR_SYS_MBOX {
	OS_EVENT	*q;
	OS_EVENT	*sem;
	void		*start[MAX_QUEUE_ENTRIES];
        _BOOL           valid;
};

struct _STR_SYS_SEM  {
        OS_EVENT*   sem;
        _BOOL       valid;
};
      
typedef void (*lwip_thread_fn)(void *arg);
typedef struct _STR_SYS_SEM     sys_sem_t;
typedef struct _STR_SYS_MBOX    sys_mbox, *sys_mbox_t;
typedef OS_CPU_SR               sys_prot_t;
typedef INT8U                   sys_thread_t;


#ifdef	__cplusplus
    }
#endif

#endif	/* __SYS_ARCH_H__ */
