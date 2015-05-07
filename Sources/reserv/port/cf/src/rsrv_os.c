/*!
    \file порт coldfire
*/

#include <assert.h>
#include <stddef.h>
#include "reserv/core/inc/rsrv_typedef.h"
#include "rsrv_os.h"

/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief пересчет тактов uC/OS в таймауты в млсек  */
/*=============================================================================================================*/
static inline u32 ms_to_ticks(u32 ms)
{
    return ms * OS_TICKS_PER_SEC / 1000;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_os_lock_create(TrsrvOsSem  *sem)
{
    sem = OSSemCreate(1);
    if (sem != NULL) {
        return RSRV_OK;
    }

    return RSRV_OS_ERR;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_os_lock(TrsrvOsSem  *sem)
{
    uint8_t  code;

    assert(sem != NULL);

    OSSemPend(sem, 0, (INT8U*)&code);
    if ( code == OS_ERR_NONE) {
        return RSRV_OK;
    }

    return RSRV_OS_ERR;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_os_unlock(TrsrvOsSem  *sem)
{
    assert(sem != NULL);

    int8_t  code = OSSemPost(sem);

    if ( code == OS_ERR_NONE) {
        return RSRV_OK;
    }

    return RSRV_OS_ERR;
}

/*=============================================================================================================*/
/*!  \brief Создает новый почтовый ящик */
/*=============================================================================================================*/
int rsrv_os_mbox_new(TrsrvOsMbox *mbox, TrsrvIntertaskMessage **msg)
{
    TrsrvOsMbox     tmp_mbox;
    
    tmp_mbox = OSQCreate((void**)msg, 1);
    
    if ( tmp_mbox != NULL ) {      
        *mbox = tmp_mbox;
        return 0;
    }
    
    return -1;    
}

/*=============================================================================================================*/
/*!  \brief Посылает в ящик сообщение      */
/*=============================================================================================================*/
int rsrv_os_mbox_post(TrsrvOsMbox mbox, TrsrvIntertaskMessage *msg)
{
    int8_t         err;    
    
    err = OSQPost (mbox, (void*)msg);
    if (err == OS_ERR_NONE) {
      return 0;
    }      
      
    return -1;
}

/*=============================================================================================================*/
/*!  \brief Ждет пока сообщение упадет в ящик в течение заданного таймаута */
/*=============================================================================================================*/
int rsrv_os_mbox_fetch(TrsrvOsMbox mbox, TrsrvIntertaskMessage **msg, uint32_t timeout)
{
    uint8_t       err;
    int32_t       ucos_timeout = 0;
    TrsrvIntertaskMessage *message;
    

    if (timeout) {
        ucos_timeout = ms_to_ticks(timeout);
        if (!ucos_timeout) {
            ucos_timeout = 1;
        } else if (ucos_timeout > 65535) {
            ucos_timeout = 65535;
        }
    }

    message = (TrsrvIntertaskMessage*)OSQPend(mbox, ucos_timeout, (INT8U*)&err);
    
    if (err == OS_ERR_NONE) {
        *msg = message;
        return 0;
    } else if (err == OS_ERR_TIMEOUT ) {
        return 1; 
    }
       
    return -1;
}



/*=============================================================================================================*/
/*!  \brief Ждет пока сообщение упадет в ящик в течение заданного таймаута */
/*=============================================================================================================*/
int rsrv_os_mbox_accept(TrsrvOsMbox mbox, TrsrvIntertaskMessage **msg)
{
    uint8_t               err;
    TrsrvIntertaskMessage *message;
    
    message = (TrsrvIntertaskMessage*)OSQAccept(mbox, (INT8U*)&err);
    
    if (err == OS_ERR_NONE) {
        *msg = message;
        return 0;
    } else if (err == OS_ERR_Q_EMPTY ) {
        return 1; 
    }
       
    return -1;
}







/*=============================================================================================================*/

