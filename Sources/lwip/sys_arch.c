/*=============================================================================================================*/
/*!
\file sys_arch.c
\brief    ���� ������ ������������ ������ lwIP ��� uC/OS-II
\details  ������������� ��������� ����� ����� lwIP � ����� ������������ �������. ������� ������ ��� ����� 
          ���������� � ����� ������������ sys_arch.txt ������ ������ �� ������.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/
/*=============================================================================================================*/

#include "lwip/include/arch/sys_arch.h"
#include "lwip/include/arch/cc.h"
#include "common_lib/memory.h"

#include "lwip/debug.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include "T8_atomic_heap.h"

/*=============================================================================================================*/

#define __ESC_NULL ((void *)0xffffffffUL)

/*=============================================================================================================*/

static sys_mbox             queue_memory_pool[MAX_QUEUES];
static OS_MEM               *ptr_queue_memory_pool;

static u8_t                 task_id;                                        /*! ������� ������� lwip */
OS_STK                      LWIP_TASK_STK[LWIP_TASK_MAX][LWIP_STK_SIZE];

/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief �������� ��������� � ����� uC/OS  */
/*=============================================================================================================*/
static inline u32 ticks_to_ms(u32 ticks)
{
    return ticks * 1000 / OS_TICKS_PER_SEC;
}

/*=============================================================================================================*/
/*!  \brief �������� ������ uC/OS � �������� � �����  */
/*=============================================================================================================*/
static inline u32 ms_to_ticks(u32 ms)
{
    return ms * OS_TICKS_PER_SEC / 1000;
}


/*=============================================================================================================*/
/*!  \brief �������������� ���� ������  */
/*=============================================================================================================*/
void sys_init(void)
{
    u8_t i;
    u8_t err;
    
    ptr_queue_memory_pool = OSMemCreate(&queue_memory_pool[0], MAX_QUEUES, sizeof(sys_mbox), &err);
    LWIP_ASSERT("OSMemCreate", err == OS_ERR_NONE);    
    
    task_id = 0;
}


/*-----------------------------------------��������---------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief ������� ����� �������.   */
/*=============================================================================================================*/
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    sem->sem = OSSemCreate((INT16U)count);
    LWIP_ASSERT("OSSemCreate ", sem->sem != NULL);
    sem->valid = 1;
    return ERR_OK;
}

/*=============================================================================================================*/
/*!  \brief ������� �������.   */
/*=============================================================================================================*/
void sys_sem_free(sys_sem_t *sem)
{
    INT8U     err;

    sem->valid = 0;
    OSSemDel(sem->sem, OS_DEL_NO_PEND, &err);
    LWIP_ASSERT("OSSemDel ", err == OS_ERR_NONE);
}


/*=============================================================================================================*/
/*!  \brief ����������� �������� ������� �� �������.   */
/*=============================================================================================================*/
void sys_sem_signal(sys_sem_t *sem)
{
    (void)OSSemPost(sem->sem);
}


/*=============================================================================================================*/
/*!  \brief ���� ����������� �������� � ��������� ���������. */
/*=============================================================================================================*/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    INT8U   err;
    INT32U  ucos_timeout;
    INT32U  begin_time;

    /* Convert lwIP timeout (in milliseconds) to uC/OS-II timeout (in OS_TICKS) */
    if( timeout ) {
        ucos_timeout = ms_to_ticks(timeout);
        if(ucos_timeout < 1) {
            ucos_timeout = 1;
        }
        else if(ucos_timeout > 65535) {
            ucos_timeout = 65535;
        }
    } else {
        ucos_timeout = 0;
    }
    
    begin_time = OSTimeGet();
    OSSemPend( sem->sem, ucos_timeout, &err );
    
    if(err == OS_ERR_TIMEOUT) {
        return SYS_ARCH_TIMEOUT;
    }
    
    return ticks_to_ms(OSTimeGet() - begin_time);
}


/*=============================================================================================================*/
/*!  \brief     ��������� ���� �� � ����� �������� ��������
 */
/*=============================================================================================================*/
u32_t   sys_arch_sem_fetch (sys_sem_t *sem)
{
    LWIP_ASSERT("msg NULL", sem != NULL);
        
    if ( sem->valid == FALSE ) {
        return 0;
    }    
    return OSSemAccept(sem->sem);
}


/*=============================================================================================================*/
/*!  \brief     ��������� ��� �� ���� �������  
 *   \retval    1 for valid, 0 for invalid
 */
/*=============================================================================================================*/
int sys_sem_valid(sys_sem_t *sem) 
{
    if ( sem != NULL )  {
        return    sem->valid;
    }
  
    return 0;
}

/*=============================================================================================================*/
/*!  \brief ���������� ����� ��������� ��������.   */
/*=============================================================================================================*/
void sys_sem_set_invalid(sys_sem_t *sem)
{
    if ( sem != NULL )  {
        sem->valid = 0;
    }
}




/*-----------------------------------------������� ���������---------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief ������� ����� ������� ��������� ������� size */
/*=============================================================================================================*/
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    sys_mbox_t      tmp_mbox;
    INT8U           err;

    LWIP_ASSERT("MAX_QUEUE_ENTRIES is too small", size <= MAX_QUEUE_ENTRIES);

    tmp_mbox = (sys_mbox_t)OSMemGet(ptr_queue_memory_pool, &err);
    if (tmp_mbox) {
	tmp_mbox->q = OSQCreate(tmp_mbox->start, MAX_QUEUE_ENTRIES);
	if (tmp_mbox->q) {
		tmp_mbox->sem = OSSemCreate(MAX_QUEUE_ENTRIES);
		if (tmp_mbox->sem) {
                        tmp_mbox->valid = 1;
			*mbox = tmp_mbox;
			return (err_t)ERR_OK;
		}
		OSQDel(tmp_mbox->q, OS_DEL_ALWAYS, &err);
		LWIP_ASSERT("OSQDel", err == OS_ERR_NONE);
	}
	err = OSMemPut(ptr_queue_memory_pool, tmp_mbox);
	LWIP_ASSERT("OSMemPut", err == OS_ERR_NONE);
    }

    return (err_t)ERR_MEM;    
}

/*=============================================================================================================*/
/*!  \brief ������� ������� ���������, ����������� ������  */
/*=============================================================================================================*/
void sys_mbox_free(sys_mbox_t *mbox)
{
    INT8U err;
    sys_mbox_t      tmp_mbox = *mbox;

    LWIP_ASSERT("free noinit mbox", mbox != NULL );

    OSSemDel(tmp_mbox->sem, OS_DEL_ALWAYS, &err);
    LWIP_ASSERT("OSSemDel", err == OS_ERR_NONE);
    OSQDel(tmp_mbox->q, OS_DEL_ALWAYS, &err);
    LWIP_ASSERT("OSQDel", err == OS_ERR_NONE);
    tmp_mbox->valid = 0;    
    err = OSMemPut(ptr_queue_memory_pool, tmp_mbox);
    LWIP_ASSERT("OSMemPut", err == OS_ERR_NONE);
}


/*=============================================================================================================*/
/*!  \brief �������� ��������� ��������� � �������      */
/*=============================================================================================================*/
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    INT8U       err;
    sys_mbox_t  tmp_mbox = *mbox;

    /* can be NULL */
    if (!msg) {
        msg = __ESC_NULL;
    }
        
    OSSemPend(tmp_mbox->sem, 0, &err);
    LWIP_ASSERT("OSSemPend", err == OS_ERR_NONE);
    err = OSQPost(tmp_mbox->q, msg);
    LWIP_ASSERT("OSQPost", err == OS_ERR_NONE);
}


/*=============================================================================================================*/
/*!  \brief �������� �������� ��������� ��������� � ������� (������� � ������������ ���������)  */
/*=============================================================================================================*/
err_t   sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    INT8U       err;
    sys_mbox_t  tmp_mbox = *mbox;

    if (!msg) {
	msg = __ESC_NULL;
    }
    
    if ( OSSemAccept(tmp_mbox->sem) ) {
	err = OSQPost(tmp_mbox->q, msg);
	LWIP_ASSERT("OSQPost", err == OS_ERR_NONE);
	return (err_t)ERR_OK;
    } else {
	return (err_t)ERR_MEM;
    }
}


/*=============================================================================================================*/
/*!  \brief ���� ���� ��������� ������ � ������� � ������� ��������� �������� */
/*=============================================================================================================*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    INT8U       err;
    sys_mbox_t  tmp_mbox = *mbox;
    INT32U      begin_time;
    INT32U      ucos_timeout;

	if (timeout) {
		ucos_timeout = ms_to_ticks(timeout);
		if (!ucos_timeout) {
		    ucos_timeout = 1;
                }
		else if (ucos_timeout > 65535) {
		    ucos_timeout = 65535;
                }
	}
	begin_time = OSTimeGet();
        
        LWIP_ASSERT("msg NULL", msg != NULL);
	*msg = OSQPend(tmp_mbox->q, ucos_timeout, &err);
	if (err == OS_ERR_NONE) {
		LWIP_ASSERT("OSQPend", *msg);
		err = OSSemPost(tmp_mbox->sem);
		LWIP_ASSERT("OSSemPost", err == OS_ERR_NONE);
		if ((*msg) == __ESC_NULL) {
		    *msg = NULL;
                }
		return ticks_to_ms(OSTimeGet() - begin_time);
	} else {
		LWIP_ASSERT("OSQPend", err == OS_ERR_TIMEOUT);
		return SYS_ARCH_TIMEOUT;
	}
}



/*=============================================================================================================*/
/*!  \brief     ��������� ���� �� ��������� � ������� ��� �������� ��������  */
/*=============================================================================================================*/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    INT8U       err;
    sys_mbox_t  tmp_mbox = *mbox;

    LWIP_ASSERT("msg NULL", msg != NULL);
    *msg = OSQAccept(tmp_mbox->q, &err);
    
    if (err == OS_ERR_NONE) {
	LWIP_ASSERT("OSQPend", *msg);
	err = OSSemPost(tmp_mbox->sem);
	LWIP_ASSERT("OSSemPost", err == OS_ERR_NONE);
	if ((*msg) == __ESC_NULL) {
	    *msg = NULL;
        }
        return 0;
    } else {
	LWIP_ASSERT("OSQPend", err == OS_ERR_Q_EMPTY);
	return SYS_MBOX_EMPTY;
    }
    
}

/*=============================================================================================================*/
/*!  \brief     ��������� ���� �� ��� �������  
 *   \retval    1 for valid, 0 for invalid
 */
/*=============================================================================================================*/
int sys_mbox_valid(sys_mbox_t *mbox)
{
    if ( (*mbox) != NULL )     {
        return (*mbox)->valid ;
    }
    
    return 0;
}


/*=============================================================================================================*/
/*!  \brief ���������� ����� ��������� �������.   */
/*=============================================================================================================*/
void sys_mbox_set_invalid(sys_mbox_t *mbox) 
{
    if ( (*mbox) != NULL  )    {      
      (*mbox)->valid = 0; 
    }
}



/*-----------------------------------------������---------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief ������� ����� �����.  */
/*=============================================================================================================*/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    u8 ret ;
    
    LWIP_PLATFORM_ASSERT ( stacksize <= LWIP_STK_SIZE );
    
    if (task_id < LWIP_TASK_MAX) {        
        ret = OSTaskCreateExt(thread, arg, &LWIP_TASK_STK[task_id][stacksize-1], prio+task_id, prio+task_id, &LWIP_TASK_STK[task_id][0], stacksize, NULL, OS_TASK_OPT_STK_CHK );
	LWIP_PLATFORM_ASSERT(ret == OS_ERR_NONE );
        OSTaskNameSet( prio+task_id, (INT8U*)name, (INT8U*)&ret ) ;
        LWIP_PLATFORM_ASSERT( ret == OS_ERR_NONE ) ;
        task_id++;
    } 
    
    return (prio+task_id);
}

/*=============================================================================================================*/
/*!  \brief ������ � ����������� ������.   */
/*=============================================================================================================*/
sys_prot_t sys_arch_protect(void)
{
    sys_prot_t cpu_sr;
    
    OS_ENTER_CRITICAL();    
    return cpu_sr;
}

/*=============================================================================================================*/
/*!  \brief ������� �� ����������� ������.   */
/*=============================================================================================================*/
void sys_arch_unprotect(sys_prot_t pval)
{
    sys_prot_t cpu_sr;
    
    cpu_sr = pval;
    OS_EXIT_CRITICAL();
}


/*=============================================================================================================*/
/*!  \brief ���������� ����� � �������������.   */
/*=============================================================================================================*/
u32_t sys_now(void)
{
    return ticks_to_ms(OSTimeGet());
}