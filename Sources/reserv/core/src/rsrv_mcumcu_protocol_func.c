#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "rsrv_uart.h"
#include "rsrv_i2c.h"
#include "reserv/core/inc/rsrv_mcumcu_protocol_func.h"
#include "reserv/core/inc/rsrv_typedef.h"
#include "common_lib/crc.h"


/*=============================================================================================================*/

#define BIG_ENDIAN_CPU      1

#define BTX                 0x0Du
#define ETX                 0x0Au

#define PING_UART           0x01u
#define PONG_UART           0x02u
#define PING_I2C            0x03u
#define PONG_I2C            0x04u
#define VOTE                0x05u
#define AGREED              0x06u



/*=============================================================================================================*/

//static void reserv_protocol_crc           (uint8_t*, uint8_t*, uint32_t);
static void                       resrv_protocol_mcu_buf_to_view (TmcuView*, uint8_t*);
static int                        rsrv_mcumcu_collect_pocket     (uint8_t, uint8_t);
int                               resrv_protocol_char_to_binary  (uint8_t *, char*, size_t);
int                               rsrv_get_info_from_packet      (enum RESRV_PROTOCOL_EVENT *evnt, struct _MCU_VIEW_PAIR *McuViewPairReceive);



/* STX  [POCKET_NUM  COMMAND   LOCAL    REMOTE  CRC16]  ETX */
/*=============================================================================================================*/
static struct _MCU_VIEW_PAIR    McuViewReceive;
extern struct _MCU_VIEW_PAIR    McuViewPair;
static uint8_t                  charbuf_output[(sizeof McuViewPair.Local + sizeof McuViewPair.Remote + 2 + 1 + 1)*2 + 2 + 1];
static uint8_t                  charbuf_input [(sizeof McuViewPair.Local + sizeof McuViewPair.Remote + 2 + 1 + 1)*2 + 2 + 1];
static uint8_t                  bin_buf[sizeof McuViewPair.Local + sizeof McuViewPair.Remote + 2 + 1 + 1];
static uint8_t                  cur_pocket_num = 0;
static uint8_t                  recv_pocket_num = 0;

/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
struct _MCU_VIEW_PAIR *rsrv_get_recevied_mcupair(void)
{
   return &McuViewReceive;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int  rsrv_get_info_from_packet
(
  enum RESRV_PROTOCOL_EVENT   *evnt, 
  struct _MCU_VIEW_PAIR       *McuViewPairReceive
)
{
  
    recv_pocket_num =  bin_buf[0];
    switch ( bin_buf[1] )
    {
    case PING_UART:   *evnt = RESRV_PROTOCOL_PING_UART; break;
    case PONG_UART:   *evnt = RESRV_PROTOCOL_PONG_UART; break;
    case PING_I2C:    *evnt = RESRV_PROTOCOL_PING_I2C;  break;
    case PONG_I2C:    *evnt = RESRV_PROTOCOL_PONG_I2C;  break;
    case VOTE:        *evnt = RESRV_PROTOCOL_VOTE;      break;
    case AGREED:      *evnt = RESRV_PROTOCOL_AGREED;    break;
    }

    /* принятый local - пришедший в пакете remote */
    resrv_protocol_mcu_buf_to_view(&(McuViewPairReceive->Local), &bin_buf[2 + sizeof (McuViewPairReceive->Local)] );
    /* принятый remote - пришедший в пакете local */
    resrv_protocol_mcu_buf_to_view(&McuViewPairReceive->Remote, &bin_buf[2]);
    
    return 0;    
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_protocol_init_tx(void)
{
    int ret;
   
    ret = rsrv_mcumcu_uart_tx_init();
//    rsrv_i2c_tx_init();

    return ret;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_protocol_init_rx(void)
{
    int ret;

    rsrv_os_lock(&McuViewReceive.Sem);        
    rsrv_mcuview_init(&McuViewReceive.Local);
    rsrv_mcuview_init(&McuViewReceive.Remote);
    rsrv_os_unlock(&McuViewReceive.Sem);
    
    ret = rsrv_mcumcu_uart_rx_init();
//    ret = ret && rsrv_mcumcu_i2c_rx_init();

    return ret;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_protocol_ping_seng
(
    int     interface,
    clock_t timeout
)
{
    int outlen;
                            /* собираем пакет с выходными данными  (плюс считаем CRC, и обкладываем терминаторами) */
    if (interface == RESRV_UART_INTERFACE) {
        outlen = rsrv_mcumcu_collect_pocket(cur_pocket_num, PING_UART);
    } else if (interface == RESRV_I2C_INTERFACE) {
        outlen = rsrv_mcumcu_collect_pocket(cur_pocket_num, PING_I2C);
    }
    cur_pocket_num++;                                            /* сохраняяем номер пакета для приема ответа */
                           
    return rsrv_mcumcu_uart_send(charbuf_output, outlen, timeout);       /* отправляем пакет по уарт */
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_protocol_send_pong
(
    int     interface,
    clock_t timeout
)
{
    int outlen;
    int ret;

    if (interface == RESRV_UART_INTERFACE) {
        outlen = rsrv_mcumcu_collect_pocket(recv_pocket_num, PONG_UART);
        ret    = rsrv_mcumcu_uart_send(charbuf_output, outlen, timeout);
    } else if (interface == RESRV_I2C_INTERFACE) {
        outlen = rsrv_mcumcu_collect_pocket(recv_pocket_num, PONG_I2C);
        rsrv_mcumcu_i2c_tx_init();
        ret    = rsrv_i2c_send(charbuf_output, outlen);
        rsrv_mcumcu_i2c_rx_init();
    }

    return  ret;      /* отправляем пакет по уарт */
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_protocol_send_vote
(
    int     interface,
    clock_t timeout
)
{
    int outlen;
                            /* собираем пакет с выходными данными  (плюс считаем CRC, и обкладываем терминаторами) */
    outlen = rsrv_mcumcu_collect_pocket(cur_pocket_num, VOTE);

    cur_pocket_num++;

    return rsrv_mcumcu_uart_send(charbuf_output, outlen, timeout);       /* отправляем пакет по уарт */
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_protocol_send_agreed
(
    int     interface,
    clock_t timeout
)
{
    int outlen;
    int ret;

    outlen = rsrv_mcumcu_collect_pocket(recv_pocket_num, AGREED);

    if (interface == RESRV_UART_INTERFACE) {
        ret    = rsrv_mcumcu_uart_send(charbuf_output, outlen, timeout);
    } else if (interface == RESRV_I2C_INTERFACE) {
        rsrv_mcumcu_i2c_tx_init();
        ret    = rsrv_i2c_send(charbuf_output, outlen);
        rsrv_mcumcu_i2c_rx_init();
    }

    return  ret;      /* отправляем пакет по уарт */
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static int rsrv_mcumcu_protocol_receive_from
(
    uint8_t     *out_buf,
    uint16_t    *readed_len,
    uint16_t    len,
    clock_t     timeout,
    int         interface
)
{
    int       ret   = RSRV_NOANSW;
    uint16_t  ret_len = 0;

    if (interface == RESRV_UART_INTERFACE) {
        ret = rsrv_mcumcu_uart_receive(out_buf, &ret_len, len, timeout);
    } else if (interface == RESRV_I2C_INTERFACE) {
        ret = rsrv_mcumcu_i2c_receive(out_buf, &ret_len, len, timeout);        
    }
    *readed_len = ret_len;
    return ret;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static int rsrv_mcumcu_protocol_receive_frame
(
    int             interface, 
    clock_t         timeout,
    uint16_t        *readed_len
)
{
    int         ret                 = RSRV_NOANSW;
    clock_t     begin_time          = clock();
    clock_t     now_time            = clock();
    clock_t     residuary_timeout   = timeout;
    uint16_t    real_len;
    uint16_t    need_len = 206;

//    memset(charbuf_input, 0, (sizeof McuViewPair.Local + sizeof McuViewPair.Remote + 2 + 1 + 1)*2 + 2 + 1);
    do {
        /* вычитываем один байтик, смотрим есть ли  BTX */
        ret = rsrv_mcumcu_protocol_receive_from(charbuf_input, &real_len, 1, residuary_timeout, interface);
        if (ret == RSRV_TIMEOUT) {
            return  ret;
        }
        now_time = clock();
   } while ( (charbuf_input[0] != BTX) );
   
   residuary_timeout = timeout - (( now_time - begin_time ) / 1000 ) ;
   need_len--;
        /* вычитываем до того момента пока не найдем ETX или не кончится буфер */
   ret = rsrv_mcumcu_protocol_receive_from(&charbuf_input[1], &real_len, need_len, residuary_timeout, interface);
   if (ret == RSRV_TIMEOUT) {
      return  ret;
   }
      
   if ( charbuf_input[real_len] != ETX) {
      ret = RSRV_STRUCT_ERR;
   }

   if ( readed_len != NULL ) {
      *readed_len = real_len + 1;
   }
     
   return ret;
}



/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_protocol_receive_events
(
    int             interface, 
    clock_t         timeout, 
    TProtocolEvent  *evnt
)
{
    int       ret                 = RSRV_NOANSW;
    clock_t   begin_time          = clock();
    clock_t   now_time            = clock();
    clock_t   residuary_timeout   = timeout;
    uint16_t  inp_len;
    
    *evnt = RESRV_PROTOCOL_DEFAULT;

    do {
        /* принимаем очередной кадр (с терминаторами) */
        ret = rsrv_mcumcu_protocol_receive_frame(interface, residuary_timeout, &inp_len);
        if (ret == RSRV_TIMEOUT) {
            break;
        }

        if ( ret == RSRV_OK ) {
            uint16_t  crc16_calc;
            uint16_t  crc16_recv;
            int       bin_len;
            
            bin_len = resrv_protocol_char_to_binary(bin_buf, (char*)&charbuf_input[1], inp_len-1); /* переводим в бинарный вид           */
            crc16_calc = Crc16(bin_buf, bin_len - 2 );                      /* проверяем целостность пакета CRC16 */
            crc16_recv = _WORD(bin_buf[bin_len - 1],  bin_buf[bin_len - 2]);
            
            if ( crc16_calc != crc16_recv ) {
                ret = RSRV_CRC_ERR;
            }
        } else {
            memset (bin_buf, 0, sizeof McuViewPair.Local + sizeof McuViewPair.Remote + 2 + 1 + 1);
        }

//        if ( ret == RSRV_OK ) {
//                                                                     /* проверяем номер пакета             */
//            ret = RSRV_STRUCT_ERR;
//        }
        now_time = clock() ;
        residuary_timeout = timeout - (( now_time - begin_time )/ 1000);
    } while ( (residuary_timeout > 0) && (ret != RSRV_OK) );

    if ( ret == RSRV_OK ) {
        rsrv_get_info_from_packet(evnt, &McuViewReceive);         /* парсим пакет, определяем событие   */
                                                                  /* запоминаем пришедшие состояния     */        
    }

    return  ret;
}




/*------------------------------------функции преобразования данных -----------------------------------*/


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void resrv_protocol_mcu_view_to_buf
(
    uint8_t     *buf,                /*! [out] буфер с выходным массивом        */
    TmcuView    *mcu_view            /*! [in]  указатель на входящюю структуру  */
)
{
#if BIG_ENDIAN_CPU

    buf[0] = _BYTE(mcu_view->Adr, 0);
    buf[1] = _BYTE(mcu_view->Adr, 1);
    buf[2] = _BYTE(mcu_view->Adr, 2);
    buf[3] = _BYTE(mcu_view->Adr, 3);

    memcpy(&buf[4], &mcu_view->Role, 9);

    buf[14] = _BYTE(mcu_view->MCUrev, 0);
    buf[15] = _BYTE(mcu_view->MCUrev, 1);
    buf[16] = _BYTE(mcu_view->MCUrev, 2);
    buf[17] = _BYTE(mcu_view->MCUrev, 3);

    buf[18] = _BYTE(mcu_view->CPUrev, 0);
    buf[19] = _BYTE(mcu_view->CPUrev, 1);
    buf[20] = _BYTE(mcu_view->CPUrev, 2);
    buf[21] = _BYTE(mcu_view->CPUrev, 3);

    buf[22] = _BYTE(mcu_view->SWITCHrev, 0);
    buf[23] = _BYTE(mcu_view->SWITCHrev, 1);
    buf[24] = _BYTE(mcu_view->SWITCHrev, 2);
    buf[25] = _BYTE(mcu_view->SWITCHrev, 3);

    memcpy(&buf[26], &mcu_view->PWR, 11);

    buf[38] = _BYTE(mcu_view->RackNum, 0);
    buf[39] = _BYTE(mcu_view->RackNum, 1);
    buf[40] = _BYTE(mcu_view->RackNum, 2);
    buf[41] = _BYTE(mcu_view->RackNum, 3);

    buf[42] = _BYTE(mcu_view->RemotePresent, 0);
    buf[43] = _BYTE(mcu_view->RemotePresent, 1);
    buf[44] = _BYTE(mcu_view->RemotePresent, 2);
    buf[45] = _BYTE(mcu_view->RemotePresent, 3);

    buf[46] = (uint8_t)mcu_view->Phase;

    buf[47] = _BYTE(mcu_view->DegradedMode, 0);
    buf[48] = _BYTE(mcu_view->DegradedMode, 1);
    buf[49] = _BYTE(mcu_view->DegradedMode, 2);
    buf[50] = _BYTE(mcu_view->DegradedMode, 3);

#else

    memcpy(buf, (void *)mcu_view, sizeof*mcu_view);

#endif
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void resrv_protocol_mcu_buf_to_view
(
    TmcuView    *mcu_view,            /*! [in]  указатель на исходящюю структуру  */
    uint8_t     *buf                /*! [out] буфер с входным массивом        */
    )
{
#if BIG_ENDIAN_CPU
    mcu_view->Adr = _DWORD(buf[3], buf[2], buf[1], buf[0]);

    memcpy(&mcu_view->Role, &buf[4], 9);

    mcu_view->MCUrev    = _DWORD(buf[17], buf[16], buf[15], buf[14]);
    mcu_view->CPUrev    = _DWORD(buf[21], buf[20], buf[19], buf[18]);
    mcu_view->SWITCHrev = _DWORD(buf[25], buf[24], buf[23], buf[22]);

    memcpy(&mcu_view->PWR, &buf[26], 11);

    mcu_view->RackNum = _DWORD(buf[41], buf[40], buf[39], buf[38]);
    mcu_view->RemotePresent = _DWORD(buf[45], buf[44], buf[43], buf[42]);
    mcu_view->Phase = buf[46];
    mcu_view->DegradedMode = _DWORD(buf[50], buf[49], buf[48], buf[47]);

#else
    memcpy(buf, (void *)mcu_view, sizeof*mcu_view);
#endif
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int     resrv_protocol_binary_to_char
(
    char    *char_buf_out,
    uint8_t *byte_buf,
    size_t  len
    )
{
    uint16_t    indx = 0;

    assert(char_buf_out != NULL);
    assert(byte_buf != NULL);

    if (len == 0) {
        return 0;
    }

    while (len--) {
        sprintf(&char_buf_out[indx * 2], "%02x", byte_buf[indx]);
        indx++;
    }

    return indx*2;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int     resrv_protocol_char_to_binary
(
    uint8_t *bin_buf,
    char    *char_buf_in,
    size_t  len
    )
{
    char     temp_buf[3];
    uint16_t    indx = 0;

    temp_buf[2] = 0;
    len >>= 1;

    while (len--) {
        temp_buf[0] =  char_buf_in[indx * 2];
        temp_buf[1] =  char_buf_in[indx * 2 + 1];
        bin_buf[indx++] = (uint8_t)strtol(temp_buf, NULL, 16);
    }
    return indx;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void reserv_protocol_crc
(
    uint8_t     *outbuf,        /*![out] буфер куда будут записаны 2 байта CRC16 */
    uint8_t     *begin,         /*![in]  начало рассчета CRC16   */
    uint32_t    len             /*![in]  длина буфера по которому рассчитывается CRC16  */
)
{
    uint16_t    crc;

    crc = Crc16(begin, len);

    outbuf[0] = _LSB(crc);
    outbuf[1] = _MSB(crc);
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static int rsrv_mcumcu_collect_pocket
(
    uint8_t     pocket_num,
    uint8_t     command_code
)
{
    int mcview_size = sizeof McuViewPair.Local;
    int len;

    bin_buf[0] = pocket_num;
    bin_buf[1] = command_code;
    resrv_protocol_mcu_view_to_buf(&bin_buf[2], &McuViewPair.Local);
    resrv_protocol_mcu_view_to_buf(&bin_buf[2 + mcview_size], &McuViewPair.Remote);
    reserv_protocol_crc(&bin_buf[2 + mcview_size*2], &bin_buf[0], 2 + mcview_size*2);
    len = resrv_protocol_binary_to_char((char*)&charbuf_output[1], bin_buf, 2 + mcview_size*2 + 2);
    charbuf_output[0] = BTX;
    charbuf_output[1+len] = ETX;

    return len + 2;
}

