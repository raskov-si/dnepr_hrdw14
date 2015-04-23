#ifndef _RSRV_DECISION_H_
#define _RSRV_DECISION_H_

#ifdef	__cplusplus
    extern "C" {
#endif

/*=============================================================================================================*/
      
#include "reserv/core/inc/rsrv_typedef.h"
      
/*=============================================================================================================*/

#pragma pack(push)
#pragma pack(1)
typedef struct _MCU_DIAG_VECTOR {
    TThreeState       UARTTxRx;
    unsigned int      NeighborPresent;
    TRoles            NeighborRole;
    unsigned int      SlotPosition;
} TreservMCUDiagVector;
#pragma pack(pop)
            
/*=============================================================================================================*/

TRoles rsrv_make_mcu_descision 
(
  TreservMCUDiagVector  *input_vector
);

/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_DECISION_H_ */

