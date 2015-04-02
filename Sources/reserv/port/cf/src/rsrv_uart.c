#include <stdio.h>
#include "reserv/core/inc/rsrv_typedef.h"
#include "rsrv_uart.h"
#include "common_lib/T8_CircBuffer.h"
#include "Application/inc/t8_dnepr_time_date.h"



/*=============================================================================================================*/

#define MAX_BUFF_LEN        256
#define UART0_MCUMCU_SPEED  115200

/*=============================================================================================================*/

unsigned int    rsrv_uart_send_callback( unsigned int * p, const unsigned int len_max );
void            rsrv_uart_rcv_callback( const unsigned int ch );

/*=============================================================================================================*/

CIRCBUFFER( rsrv_uart_rcv_buff, MAX_BUFF_LEN ); 
CIRCBUFFER( rsrv_uart_snd_buff, MAX_BUFF_LEN ); 

static uart_desc_t uart0_desc = 
{
    /* Терминал */
     0,
     UART0_MCUMCU_SPEED,
     0,
     rsrv_uart_rcv_callback,    // указатель на callback на приём
     rsrv_uart_send_callback,   // указатель на callback на передачу
     0
};

static uint8_t   stop_tx_flag;
/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
unsigned int rsrv_uart_send_callback( unsigned int * p, const unsigned int len_max )
{
    size_t  actual_sz ;
    
    circbuffer_read_block( &rsrv_uart_snd_buff, (u8*)&p, len_max, &actual_sz );

    if( circbuffer_get_storage_data_size(&rsrv_uart_snd_buff) == 0 )  {
        rsrv_uart_stop_tx(&uart0_desc);
        stop_tx_flag = 1;
    }
    
    return  actual_sz;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void rsrv_uart_rcv_callback( const unsigned int ch )
{  
    CircBuffer_push_one_erasingdata( &rsrv_uart_rcv_buff, (u8)(ch & 0xFF) );
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_uart_rx_init (void)
{
    /* окрываем uart */
    if ( uart0_desc.initdone == 0 ) {
        rsrv_uart_init(&uart0_desc);
    }

    return RSRV_OK;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_uart_tx_init (void)
{
    if ( uart0_desc.initdone == 0 ) {
        rsrv_uart_init(&uart0_desc);
    }

    return RSRV_OK;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_uart_send    (uint8_t* buf, uint16_t len, clock_t timeout)
{
    size_t  actual_sz ;
    clock_t timeout_timer;
    int     val_ret = RSRV_OK;

    timer_set(&timeout_timer);

    circbuffer_write_block( &rsrv_uart_snd_buff, &actual_sz, buf, len );
    uart_terminal_start_tx(&uart0_desc);

    while ( (actual_sz != len) && !timer_is_expired(&timeout_timer, timeout) ) {
        circbuffer_write_block( &rsrv_uart_snd_buff, &actual_sz, buf, len );
    }

    while ( (stop_tx_flag == 0) && !timer_is_expired(&timeout_timer, timeout) ) {
        continue;
    }

    if (stop_tx_flag == 0) {
        uart_terminal_stop_tx(&uart0_desc);
        circbuffer_set_empty( &rsrv_uart_snd_buff );
        val_ret = RSRV_TIMEOUT;
    }

    return val_ret;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_uart_receive
(
    uint8_t     *out_buf, 
    uint16_t    *readed_len, 
    uint16_t    len, 
    clock_t     timeout
)
{
    size_t  actual_sz ;
    clock_t timeout_timer;
    int     val_ret = RSRV_OK;

    timer_set(&timeout_timer);

    do {
       circbuffer_read_block(&rsrv_uart_rcv_buff, out_buf, len, &actual_sz); 
    } while ( (actual_sz != len) && !timer_is_expired(&timeout_timer, timeout) );

    return val_ret;
}

/*=============================================================================================================*/
