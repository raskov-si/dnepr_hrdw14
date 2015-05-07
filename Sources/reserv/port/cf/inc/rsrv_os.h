#ifndef _RSRV_OS_H_
#define _RSRV_OS_H_

#ifdef	__cplusplus
    extern "C" {
#endif

/*=============================================================================================================*/

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"
      
/*=============================================================================================================*/
//    enum EN_RESERV_INTERTASK_CODES {
//        RESERV_INTERCODES_VOIDMSG         = 0,
//        RESERV_INTERCODES_MCUMCU_START    = 1,
//        RESERV_INTERCODES_MCUCPU_START    = 2,
//        RESERV_INTERCODES_MCUMCU_ENDDIAG  = 3,
//    };

#define   RESERV_INTERCODES_VOIDMSG         0
#define   RESERV_INTERCODES_MCUMCU_START    1
#define   RESERV_INTERCODES_MCUCPU_START    2
#define   RESERV_INTERCODES_MCUMCU_ENDDIAG  3
      
/*=============================================================================================================*/

typedef  void                              TrsrvOsThreadArg;
typedef  OS_EVENT                          TrsrvOsSem;
typedef  OS_EVENT                          TrsrvOsMtx;
typedef  OS_EVENT                          *TrsrvOsMbox;
//typedef  enum EN_RESERV_INTERTASK_CODES    TrsrvIntertaskMessage;
typedef  int                                TrsrvIntertaskMessage;

      
/*=============================================================================================================*/

int   rsrv_os_lock_create (TrsrvOsSem  *sem);
int   rsrv_os_lock        (TrsrvOsSem  *sem);
int   rsrv_os_unlock      (TrsrvOsSem  *sem);
int   rsrv_os_mbox_new    (TrsrvOsMbox *mbox, TrsrvIntertaskMessage **msg);
int   rsrv_os_mbox_post   (TrsrvOsMbox mbox, TrsrvIntertaskMessage *msg);
int   rsrv_os_mbox_fetch  (TrsrvOsMbox mbox, TrsrvIntertaskMessage **msg, uint32_t timeout);
int   rsrv_os_mbox_accept (TrsrvOsMbox mbox, TrsrvIntertaskMessage **msg);

/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_OS_H_ */
