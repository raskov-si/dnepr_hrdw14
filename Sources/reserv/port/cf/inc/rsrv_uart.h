#ifndef _RSRV_UART_H_
#define _RSRV_UART_H_

#ifdef	__cplusplus
    extern "C" {
#endif
/*=============================================================================================================*/
      
    #include <stdint.h>
    #include <time.h>
    #include "HAL/BSP/inc/T8_Dnepr_UART_Profile.h"
      
/*=============================================================================================================*/

int rsrv_mcumcu_uart_rx_init (void);
int rsrv_mcumcu_uart_tx_init (void);
int rsrv_mcumcu_uart_send    (uint8_t* buf, uint16_t len,clock_t timeout);
int rsrv_mcumcu_uart_receive (uint8_t*, uint16_t*, uint16_t, clock_t);

/*=============================================================================================================*/

/*------------------------------ Работа с UART0 (MCUMCU интерфейс)-----------------------*/

static inline void rsrv_uart_init( uart_desc_t *const uart )
{
    uart_init( uart, SYSTEM_CLOCK_KHZ );
}

static inline void rsrv_uart_start_tx( uart_desc_t *const uart )
{
    uart_start_tx( uart );  
}

static inline void rsrv_uart_stop_tx( uart_desc_t *const uart )
{
    uart_stop_tx( uart );
}


#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_H_ */
