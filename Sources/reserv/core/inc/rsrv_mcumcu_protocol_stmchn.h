#ifndef _RSRV_MCUMCU_PRTCL_STMCH_H_
#define _RSRV_MCUMCU_PRTCL_STMCH_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "reserv/core/inc/stmch.h"

extern const struct STM_TRANSITION   rsrv_mcu_mcu_stchtbl[11][6];
extern const struct STM_TRANSITION   rsrv_mcumcu_answer_stchtbl[5][4];

int *rsrv_mcumcu_prtcl_get_currect_state (void);
int rsrv_mcumcu_prtcl_get_currect_signal (void);
int resrv_mcumcu_prtcl_recv_signal (void);
int *rsrv_mcumcu_prtcl_answ_get_currect_state (void);
int resrv_mcumcu_answ_prtcl_recv_signal (void);
int rsrv_mcumcu_prtcl_get_answ_currect_signal (void);

#ifdef  __cplusplus
}
#endif

#endif  /* _RESERV_PROTOCOL_H_ */


