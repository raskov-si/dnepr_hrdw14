#ifndef _RSRV_UART_H_
#define _RSRV_UART_H_

#ifdef	__cplusplus
    extern "C" {
#endif
/*=============================================================================================================*/
      
    #include <stdint.h>
    #include <time.h>
      
/*=============================================================================================================*/

int rsrv_mcumcu_uart_rx_init (void);
int rsrv_mcumcu_uart_tx_init (void);
int rsrv_mcumcu_uart_send    (uint8_t* buf, uint16_t len,clock_t timeout);
int rsrv_mcumcu_uart_receive (uint8_t*, uint16_t*, uint16_t, clock_t);

/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_H_ */
