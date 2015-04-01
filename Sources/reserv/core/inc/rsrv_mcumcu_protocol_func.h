#ifndef _RSRV_PROTOCOL_FUNC_H_
#define _RSRV_PROTOCOL_FUNC_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <time.h>
#include "reserv/core/inc/rsrv_typedef.h"

/*=============================================================================================================*/

#define     RESRV_UART_INTERFACE    (0x01u)
#define     RESRV_I2C_INTERFACE     (0x02u)

/*=============================================================================================================*/

    enum RESRV_PROTOCOL_EVENT {
        RESRV_PROTOCOL_PING_UART = 1,
        RESRV_PROTOCOL_PONG_UART = 2,
        RESRV_PROTOCOL_PING_I2C  = 3,
        RESRV_PROTOCOL_PONG_I2C  = 4,
        RESRV_PROTOCOL_VOTE      = 5,
        RESRV_PROTOCOL_AGREED    = 6
    };
    
    typedef enum RESRV_PROTOCOL_EVENT  TProtocolEvent;

/*=============================================================================================================*/

    int             rsrv_mcumcu_protocol_init_rx(void);
    int             rsrv_mcumcu_protocol_init_tx(void);
    int             rsrv_mcumcu_protocol_ping_seng(int, clock_t);
    int             rsrv_mcumcu_protocol_receive_events(int, clock_t, TProtocolEvent*);
    int             rsrv_mcumcu_protocol_send_pong(int, clock_t);
    int             rsrv_mcumcu_protocol_send_vote(int, clock_t);
    int             rsrv_mcumcu_protocol_send_agreed(int, clock_t);

/*=============================================================================================================*/


#ifdef  __cplusplus
}
#endif

#endif  /* _RSRV_PROTOCOL_FUNC_H_ */


