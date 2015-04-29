#ifndef _RSRV_MCUMCU_PRTCL_STMCH_H_
#define _RSRV_MCUMCU_PRTCL_STMCH_H_

#ifdef  __cplusplus
extern "C" {
#endif

/*=============================================================================================================*/

#include "reserv/core/inc/stmch.h"

/*=============================================================================================================*/
  
#define RSRV_MCUMCU_START_TIMEOUT    (0u)
  
/*=============================================================================================================*/
  
enum _STM_MCUMCU_STATES {
    STATE_MCUMCU_INIT           = 0,
    STATE_WAIT_UARTPOLL         = 1,
    STATE_PING_SEND_UART        = 2,
    STATE_PONG_SEND_UART        = 3,
    
//    STATE_PING_SEND_I2CRX       = 3,
//    STATE_WAIT_FOR_ANSWER_I2C   = 4,
//    STATE_VOTE                  = 5,
//    STATE_PONGSEND_UART         = 6,
//    STATE_PONGSEND_I2C          = 7,
//    STATE_AGREED_SEND           = 8,
//    STATE_ROGER_ROLE_MASTER     = 9,
//    STATE_ROGER_ROLE_SLAVE      = 10
};
  
/*=============================================================================================================*/

extern const struct STM_TRANSITION   rsrv_mcu_mcu_stchtbl[11][6];
extern const struct STM_TRANSITION   rsrv_mcumcu_show_stchtbl[4][3];

/*=============================================================================================================*/

enum _STM_MCUMCU_STATES *rsrv_mcumcu_prtcl_get_currect_state (void);
int                      rsrv_mcumcu_prtcl_get_currect_signal (void);
int                      resrv_mcumcu_prtcl_recv_signal (void);
//int *rsrv_mcumcu_prtcl_answ_get_currect_state (void);
//int resrv_mcumcu_answ_prtcl_recv_signal (void);
//int rsrv_mcumcu_prtcl_get_answ_currect_signal (void);

/*=============================================================================================================*/

#ifdef  __cplusplus
}
#endif

#endif  /* _RESERV_PROTOCOL_H_ */


