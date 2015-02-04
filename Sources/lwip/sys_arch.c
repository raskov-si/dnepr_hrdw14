/*!
\file sys_arch.c
\brief поток для работы сети -- RSTP и TCP/UDP/IP
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jun 2014
*/

#include "arch/sys_arch.h"
#include "common_lib/memory.h"

#include "lwip/debug.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include "T8_atomic_heap.h"


/*=============================================================================================================*/

const  void * const         pvNullPointer = NULL;
static OS_MEM              *pQueueMem;
static char                 pcQueueMemoryPool[MAX_QUEUES * sizeof(TQ_DESCR)];
static u8_t                 curr_prio_offset;
OS_STK                      LWIP_TASK_STK[LWIP_TASK_MAX][LWIP_STK_SIZE];

/*=============================================================================================================*/


/*=============================================================================================================*/
/*!  \brief Инициализирует этот модуль  */
/*=============================================================================================================*/
void sys_init(void)
{
    u8_t i;
    u8_t err;
    
    pQueueMem        = OSMemCreate((void*)pcQueueMemoryPool, MAX_QUEUES, sizeof(TQ_DESCR), &err);
    curr_prio_offset = 0;
}


/*-----------------------------------------Очереди сообщений---------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief Создает новую очередь сообщений размера size */
/*=============================================================================================================*/
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    INT8U       err;
    PQ_DESCR    pQDesc;
    
    size = size;    
    if( mbox == NULL ){
        return (err_t)ERR_VAL ;
    }
    
    pQDesc = (PQ_DESCR)OSMemGet(pQueueMem, &err);
    
    if (err == OS_ERR_NONE) {
        pQDesc->pQ = OSQCreate(&(pQDesc->pvQEntries[0]), MAX_QUEUE_ENTRIES);
        if (pQDesc->pQ != NULL) {
            mbox = pQDesc;
            return (err_t)ERR_OK;
        }
    }
    
    return (err_t)ERR_MEM;
}

/*=============================================================================================================*/
/*!  \brief Удаляет очередь сообщений, освобождает память  */
/*=============================================================================================================*/
void sys_mbox_free(sys_mbox_t *mbox)
{
    u8_t      err;
    u8_t      cnt;

    if( mbox == NULL ) {
        return;
    }
    
    for (cnt=0; cnt<0xfe; cnt++) {
        OSQFlush(mbox->pQ);
        OSQDel(mbox->pQ, OS_DEL_NO_PEND, &err);
        if (err == OS_ERR_NONE) {
            break;
        } else {
            OSTimeDly(1);
        }
    } 
    OSMemPut(pQueueMem, mbox);
}


/*=============================================================================================================*/
/*!  \brief Посылает очередное сообщение в очередь      */
/*=============================================================================================================*/
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    if (msg == NULL) {
        msg = (void *)&pvNullPointer;
    }
    (void)OSQPost(mbox->pQ, msg);
}

/*=============================================================================================================*/
/*!  \brief Пытается посылать очередное сообщение в очередь (посылка с возвращаемым значением)  */
/*=============================================================================================================*/
err_t   sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    u8_t err;
    
    if (mbox == NULL) {
        return (err_t)ERR_VAL ;      
    }
      
    if ( msg == NULL) {
        msg = (void *)&pvNullPointer;
    }
    err = OSQPost(mbox->pQ, msg);
    if (err == OS_ERR_NONE) {
        return (err_t)ERR_OK;
    } else {
        return (err_t)ERR_MEM;
    }
}


/*=============================================================================================================*/
/*!  \brief Ждет пока сообщение упадет в очередь  */
/*=============================================================================================================*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    u8      err;
    u32     ucos_timeout = 0;
    void    *temp;
    
    
    if( timeout != 0 ) {
        /* \todo Корректно определить OS_TICKS_PER_SEC !!!!!! */
        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;
        if (ucos_timeout < 1) {
            ucos_timeout = 1;
        }
        else if (ucos_timeout > 65535) {
            ucos_timeout = 65535;
        }
    }
    
    temp = OSQPend(mbox->pQ, ucos_timeout, &err);
    if (msg != NULL) {
        if (temp == (void *)&pvNullPointer) {
            *msg = NULL;
        } else {
            *msg = temp;
        }
    }
    
    if ( err == OS_ERR_TIMEOUT ) {
        return SYS_ARCH_TIMEOUT;
    }
    
    return 0; 
}

/*=============================================================================================================*/
/*!  \brief     Проверяет жива ли эта очередь  
 *   \retval    1 for valid, 0 for invalid
 */
/*=============================================================================================================*/
int sys_mbox_valid(sys_mbox_t *mbox)
{
    return (mbox != NULL) ;
}


/*=============================================================================================================*/
/*!  \brief Вызывается после обнуления очереди.   */
/*=============================================================================================================*/
void sys_mbox_set_invalid(sys_mbox_t *mbox) 
{
    mbox = NULL;
}



/*-----------------------------------------Семафоры---------------------------------------------------*/


/*=============================================================================================================*/
/*!  \brief Создает новый семафор.   */
/*=============================================================================================================*/
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{    
    if ( (sem = OSSemCreate(count)) != NULL ) {
	return (err_t)ERR_OK ;
    }
    
    return (err_t)ERR_MEM ;
}

/*=============================================================================================================*/
/*!  \brief Удаляет семафор.   */
/*=============================================================================================================*/
void sys_sem_free(sys_sem_t *sem)
{
    u8 err ;
    sem = OSSemDel( sem, OS_DEL_ALWAYS, &err );
}


/*=============================================================================================================*/
/*!  \brief Увеличивает значение ресурса на единицу.   */
/*=============================================================================================================*/
void sys_sem_signal(sys_sem_t *sem)
{
    (void)OSSemPost(sem);
}


/*=============================================================================================================*/
/*!  \brief ждет доступности семафора с указанным таймаутом. */
/*=============================================================================================================*/
/** Wait for a semaphore for the specified timeout
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return time (in milliseconds) waited for the semaphore
 *         or SYS_ARCH_TIMEOUT on timeout */
//u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
////u32_t sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)
//{
//    u8_t err;
//    u32_t ucos_timeout;
//    ucos_timeout = 0;
//    if(timeout != 0) {
//        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;
//        if(ucos_timeout < 1)
//            ucos_timeout = 1;
//        else if(ucos_timeout > 65535)
//        ucos_timeout = 65535;
//    }
//    OSSemPend (sem, ucos_timeout, &err);
//    if (err == OS_TIMEOUT) {
//        return SYS_ARCH_TIMEOUT;  
//    } else {
//        return !SYS_ARCH_TIMEOUT;
//    }
//}




/** Check if a sempahore is valid/allocated: return 1 for valid, 0 for invalid */
int sys_sem_valid(sys_sem_t *sem);

/** Set a semaphore invalid so that sys_sem_valid returns 0 */
void sys_sem_set_invalid(sys_sem_t *sem);










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




//sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
//{
//	u8 *stack = (u8*)npalloc( stacksize );
//	u8 ret ;
//	assert( stack );
//
//	assert(OSTaskCreateExt(thread, (void *)0, (void *)&stack[stacksize-1], prio, prio, (void *)&stack, stacksize, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE );
//    OSTaskNameSet( prio, (INT8U*)name, (INT8U*)&ret ) ;
//    assert( ret == OS_ERR_NONE ) ;
//
//    return 0 ;
//}
//
