#ifndef _RESERV_OS_H_
#define _RESERV_OS_H_

#ifdef	__cplusplus
    extern "C" {
#endif

#include "support_common.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"
      
/*=============================================================================================================*/

typedef  void       TrsrvOsThreadArg;
typedef  OS_EVENT   TrsrvOsSem;
typedef  OS_EVENT   TrsrvOsMtx;
      
/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RESERV_OS_H_ */
