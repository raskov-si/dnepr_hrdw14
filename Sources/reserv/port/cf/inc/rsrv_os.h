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

typedef  void       TrsrvOsThreadArg;
typedef  OS_EVENT   TrsrvOsSem;
typedef  OS_EVENT   TrsrvOsMtx;
typedef  OS_EVENT   *TrsrvOsMbox;

      
/*=============================================================================================================*/

int   rsrv_os_lock_create (TrsrvOsSem  *sem);
int   rsrv_os_lock        (TrsrvOsSem  *sem);
int   rsrv_os_unlock      (TrsrvOsSem  *sem);
int   rsrv_os_mbox_new    (TrsrvOsMbox *mbox, void *msg);
int   rsrv_os_mbox_post   (TrsrvOsMbox *mbox, void *msg);
int   rsrv_os_mbox_fetch  (TrsrvOsMbox *mbox, void **msg, uint32_t timeout);


/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_OS_H_ */
