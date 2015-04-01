#include <stdio.h>
#include "reserv/core/inc/rsrv_typedef.h"
#include "rsrv_uart.h"


/*=============================================================================================================*/

int rsrv_mcumcu_uart_rx_init (void)
{
    return RSRV_OK;
}

int rsrv_mcumcu_uart_tx_init (void)
{
    return RSRV_OK;
}


int rsrv_mcumcu_uart_send    (uint8_t* buf, uint16_t len, clock_t timeout)
{
    buf[len] = 0;
    printf(buf);
    return RSRV_OK;
}

int rsrv_mcumcu_uart_receive
(
    uint8_t     *out_buf, 
    uint16_t    *readed_len, 
    uint16_t    len, 
    clock_t     timeout
)
{
    return RSRV_OK;
}


/*=============================================================================================================*/

