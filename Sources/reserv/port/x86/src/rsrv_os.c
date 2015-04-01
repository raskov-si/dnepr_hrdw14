/*! 
      \file порт windows
*/

#include "reserv/core/inc/rsrv_typedef.h"
#include "rsrv_os.h"
#include <windows.h>


/*=============================================================================================================*/

int rsrv_os_lock_create(TrsrvOsSem  *sem)
{
//    CreateMutex(NULL, TRUE, ew);
//    OpenMutex();
//    if (sem != NULL) {
        return RSRV_OK;
//    }

//    return RSRV_OS_ERR;
}

int rsrv_os_lock(TrsrvOsSem  *sem)
{
//    WaitForSingleObject(sem);
    sem = sem;
    return RSRV_OK;
}

int rsrv_os_unlock(TrsrvOsSem  *sem)
{
//    ReleaseMutex();
    sem = sem;
    return RSRV_OK;
}

/*=============================================================================================================*/

