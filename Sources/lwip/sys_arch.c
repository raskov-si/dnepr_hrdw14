/*!
\file sys_arch.c
\brief поток для работы сети -- RSTP и TCP/UDP/IP
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jun 2014
*/

#include "support_common.h"

#include "lwip/debug.h"

#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include "T8_atomic_heap.h"

void sys_init(void)
{}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	sem = OSSemCreate( count );
	return ERR_OK ;
}

void sys_sem_free(sys_sem_t *sem)
{
	u8 err ;
	OSSemDel( sem, OS_DEL_ALWAYS, &err );
	assert( err == OS_ERR_NONE );
}

void sys_sem_signal(sys_sem_t *sem)
{
	OSSemPost( sem );
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	u8 err ;
	unsigned long long starttime = llUptime ;
	unsigned long long fintime ;
	OSSemPend( sem, timeout, &err );
	fintime = llUptime ;
	assert( (err == OS_ERR_NONE) || (err == OS_ERR_TIMEOUT) );
	if( err == OS_ERR_TIMEOUT ){
		return SYS_ARCH_TIMEOUT ;
	} else {
		if( fintime >= starttime ){
			return fintime - starttime ;
		} else {
			return fintime + (0xFFFFFFFFFFFFFFFF - starttime) + 1;
		}
	}
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	if( !mbox ){
		return ERR_VAL ;
	}
	if( !(mbox = OSMboxCreate( NULL ))){;
		return ERR_VAL ;
	} else {
		return ERR_OK ;
	}
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	u8 err ;
	assert( 0 == OSMboxDel ( mbox, OS_DEL_NO_PEND, &err ));
	assert( err == OS_ERR_NONE );
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	assert( OS_ERR_NONE == OSMboxPost ( mbox, msg ));
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	u8 err ;
	void* result ;
	unsigned long long starttime = llUptime ;
	unsigned long long fintime ;

	result = OSMboxPend( mbox, timeout, &err );
	fintime = llUptime ;
	assert( (err == OS_ERR_NONE) || (err == OS_ERR_TIMEOUT) );

	if( msg ){
		*msg = result ;
	}

	if( err == OS_ERR_TIMEOUT ){
		return SYS_ARCH_TIMEOUT ;
	} else {
		if( fintime >= starttime ){
			return fintime - starttime ;
		} else {
			return fintime + (0xFFFFFFFFFFFFFFFF - starttime) + 1;
		}
	}
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	void* result ;
	OS_MBOX_DATA mbox_data ;

	result = OSMboxAccept( mbox );
	if( !result ){
		return SYS_MBOX_EMPTY ;
	} else if( msg ){
		*msg = result ;
		return 0 ;
	}
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	while( OS_ERR_NONE != OSMboxPost ( mbox, msg ));
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	u8 *stack = (u8*)npalloc( stacksize );
	u8 ret ;
	assert( stack );

	assert(OSTaskCreateExt(thread, (void *)0, (void *)&stack[stacksize-1], prio, prio, (void *)&stack, stacksize, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE );
    OSTaskNameSet( prio, (INT8U*)name, (INT8U*)&ret ) ;
    assert( ret == OS_ERR_NONE ) ;

    return 0 ;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
	return mbox != NULL ;
}
