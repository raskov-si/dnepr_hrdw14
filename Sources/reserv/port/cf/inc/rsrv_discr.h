#ifndef _RSRV_DISCR_H_
#define _RSRV_DISCR_H_

#ifdef	__cplusplus
    extern "C" {
#endif

/*=============================================================================================================*/

#include "reserv/core/inc/rsrv_typedef.h"
      
/*=============================================================================================================*/

unsigned int rsrv_get_dneprslotnum (void);
unsigned int rsrv_get_dneprpresent (void);
unsigned int rsrv_leds_setstate(TRoles);

/*=============================================================================================================*/
            
#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_DISCR_H_ */

      