/*!
\file sys_arch.h
\brief поток для работы сети -- RSTP и TCP/UDP/IP
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jun 2014
*/

#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#include "support_common.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

typedef OS_EVENT sys_sem_t ;
typedef OS_EVENT sys_mbox_t ;

typedef u8 sys_thread_t ;

typedef void (*lwip_thread_fn)(void *arg);


//! Is called to initialize the sys_arch layer.
void sys_init(void);

//! Creates a new semaphore.
err_t sys_sem_new(sys_sem_t *sem, u8_t count);

void sys_sem_signal(sys_sem_t *sem);
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout);
void sys_sem_free(sys_sem_t *sem);

err_t sys_mbox_new(sys_mbox_t *mbox, int size);
void sys_mbox_free(sys_mbox_t *mbox);
void sys_mbox_post(sys_mbox_t *mbox, void *msg);
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout);
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg);
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg);
int sys_mbox_valid(sys_mbox_t *mbox);


sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio);

#endif
