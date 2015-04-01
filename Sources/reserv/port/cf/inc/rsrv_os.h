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
      
/*=============================================================================================================*/

int rsrv_os_lock_create (TrsrvOsSem  *sem);
int rsrv_os_lock        (TrsrvOsSem  *sem);
int rsrv_os_unlock      (TrsrvOsSem  *sem);

/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_OS_H_ */
