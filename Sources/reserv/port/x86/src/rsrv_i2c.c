#include <stdio.h>
#include "reserv/core/inc/rsrv_typedef.h"
#include "rsrv_i2c.h"


/*=============================================================================================================*/

int rsrv_mcumcu_i2c_rx_init (void)
{
    return RSRV_OK;
}

int rsrv_mcumcu_i2c_tx_init (void)
{
    return RSRV_OK;
}

int rsrv_i2c_send    (uint8_t* buf, uint16_t len)
{
    buf[len] = 0;
    printf(buf);
    return RSRV_OK;
}

int rsrv_mcumcu_i2c_receive
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

