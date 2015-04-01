#ifndef _RSRV_OS_H_
#define _RSRV_OS_H_

#ifdef	__cplusplus
    extern "C" {
#endif

/*=============================================================================================================*/

          
/*=============================================================================================================*/

typedef  void       TrsrvOsThreadArg;
typedef  int        TrsrvOsSem;
typedef  int        TrsrvOsMtx;
      
/*=============================================================================================================*/

int rsrv_os_lock_create (TrsrvOsSem  *sem);
int rsrv_os_lock        (TrsrvOsSem  *sem);
int rsrv_os_unlock      (TrsrvOsSem  *sem);

/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_OS_H_ */
