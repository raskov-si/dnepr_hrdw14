#include <stddef.h>
#include <time.h>

#include "reserv/core/inc/rsrv_mcumcu_protocol_stmchn.h"
#include "reserv/core/inc/rsrv_mcumcu_protocol_func.h"
#include "reserv/core/inc/rsrv_typedef.h"


/*=============================================================================================================*/
/*------------------------------------------- ������� MCUMCU --------------------------------------------------*/

enum _STM_MCUMCU_STATES {
    STATE_INIT                  = 0,
    STATE_PING_SEND_UARTRX      = 1,
    STATE_WAIT_FOR_ANSWER_UART  = 2,
    STATE_PING_SEND_I2CRX       = 3,
    STATE_WAIT_FOR_ANSWER_I2C   = 4,
    STATE_VOTE                  = 5,
    STATE_PONGSEND_UART         = 6,
    STATE_PONGSEND_I2C          = 7,
    STATE_AGREED_SEND           = 8,
    STATE_ROGER_ROLE_MASTER     = 9,
    STATE_ROGER_ROLE_SLAVE      = 10
};

/*--------------------------------------------------------------------------------------------------------------*/

enum _STM_MCUMCU_SIGNALS {
    SIG_STATIC                      = 0,    

    SIG_INITIAL_GOTO_PINGUART       = 1,

    SIG_PINGUART_GOTO_ANSWUART      = 1,

    SIG_ANSWUART_GOTO_UARTPING      = 1,
    SIG_ANSWUART_GOTO_I2CPING       = 2,
    SIG_ANSWUART_GOTO_UARTPONG      = 3,
    SIG_ANSWUART_GOTO_I2CPONG       = 4,
    SIG_ANSWUART_GOTO_VOTE          = 5,

    SIG_PINGI2C_GOTO_ANSWI2C        = 1,

    SIG_ANSWI2C_GOTO_I2CPING        = 1,
    SIG_ANSWI2C_GOTO_VOTE           = 2,

    SIG_VOTE_GOTO_AGRDESC           = 1,

};

/*--------------------------------------------------------------------------------------------------------------*/

static void stmch_init_protocol(int state, int signal);
static void stmch_ping_send(int state, int signal);

static void stmch_pong_uart_answer_wait(int state, int signal);
static void stmch_pong_i2c_answer_wait(int state, int signal);
static void stmch_vote(int state, int signal);

static void stmch_receive_protocol_events(int state, int signal);
static void stmch_send_pong(int state, int signal);
static void stmch_send_agreed(int state, int signal);


/*--------------------------------------------------------------------------------------------------------------*/

const struct STM_TRANSITION       rsrv_mcu_mcu_stchtbl[11][6] = {

[STATE_INIT][SIG_STATIC]                                    = { STATE_INIT,                 stmch_init_protocol },
[STATE_INIT][SIG_INITIAL_GOTO_PINGUART]                     = { STATE_PING_SEND_UARTRX,     NULL },

[STATE_PING_SEND_UARTRX][SIG_STATIC]                        = { STATE_PING_SEND_UARTRX,     stmch_ping_send },
[STATE_PING_SEND_UARTRX][SIG_PINGUART_GOTO_ANSWUART]        = { STATE_WAIT_FOR_ANSWER_UART, NULL },
[STATE_PING_SEND_UARTRX][2]                                 = { STATE_ROGER_ROLE_MASTER,    NULL },

[STATE_WAIT_FOR_ANSWER_UART][SIG_STATIC]                    = { STATE_WAIT_FOR_ANSWER_UART, stmch_pong_uart_answer_wait },
[STATE_WAIT_FOR_ANSWER_UART][SIG_ANSWUART_GOTO_UARTPING]    = { STATE_PING_SEND_UARTRX,     NULL },
[STATE_WAIT_FOR_ANSWER_UART][SIG_ANSWUART_GOTO_I2CPING]     = { STATE_PING_SEND_I2CRX,      NULL },
[STATE_WAIT_FOR_ANSWER_UART][SIG_ANSWUART_GOTO_UARTPONG]    = { STATE_PONGSEND_UART,        NULL },         /* nested */
[STATE_WAIT_FOR_ANSWER_UART][SIG_ANSWUART_GOTO_I2CPONG]     = { STATE_PONGSEND_I2C,         NULL },         /* nested */
[STATE_WAIT_FOR_ANSWER_UART][SIG_ANSWUART_GOTO_VOTE]        = { STATE_VOTE,                 NULL },         /* nested */

[STATE_PING_SEND_I2CRX][SIG_STATIC]                         = { STATE_PING_SEND_I2CRX,      stmch_ping_send },
[STATE_PING_SEND_I2CRX][SIG_PINGI2C_GOTO_ANSWI2C]           = { STATE_WAIT_FOR_ANSWER_I2C,  NULL },
[STATE_PING_SEND_I2CRX][2]                                  = { STATE_ROGER_ROLE_MASTER,    NULL },

[STATE_WAIT_FOR_ANSWER_I2C][SIG_STATIC]                     = { STATE_WAIT_FOR_ANSWER_I2C,  stmch_pong_i2c_answer_wait },
[STATE_WAIT_FOR_ANSWER_I2C][SIG_ANSWI2C_GOTO_I2CPING]       = { STATE_PING_SEND_I2CRX,      NULL },
[STATE_WAIT_FOR_ANSWER_I2C][SIG_ANSWI2C_GOTO_VOTE]          = { STATE_VOTE,                 NULL },

[STATE_VOTE][SIG_STATIC]                                    = { STATE_VOTE,                 stmch_vote },
[STATE_VOTE][SIG_VOTE_GOTO_AGRDESC]                         = { STATE_ROGER_ROLE_MASTER,    NULL },

[STATE_PONGSEND_UART][SIG_STATIC]                           = { STATE_PONGSEND_UART,        stmch_send_pong},
[STATE_PONGSEND_UART][1]                                    = { STATE_WAIT_FOR_ANSWER_UART, NULL},
[STATE_PONGSEND_UART][2]                                    = { STATE_WAIT_FOR_ANSWER_I2C,  NULL},
[STATE_PONGSEND_UART][3]                                    = { STATE_ROGER_ROLE_SLAVE,     NULL},

[STATE_PONGSEND_I2C][SIG_STATIC]                            = { STATE_PONGSEND_I2C,         stmch_send_pong},
[STATE_PONGSEND_I2C][1]                                     = { STATE_WAIT_FOR_ANSWER_UART, NULL},
[STATE_PONGSEND_I2C][2]                                     = { STATE_WAIT_FOR_ANSWER_I2C,  NULL},
[STATE_PONGSEND_I2C][3]                                     = { STATE_ROGER_ROLE_SLAVE,     NULL},

[STATE_AGREED_SEND][SIG_STATIC]                             = { STATE_AGREED_SEND,          NULL},

[STATE_ROGER_ROLE_MASTER][SIG_STATIC]                       = { STATE_ROGER_ROLE_MASTER,    NULL},
[STATE_ROGER_ROLE_MASTER][1]                                = { STATE_PING_SEND_UARTRX,     NULL},
[STATE_ROGER_ROLE_MASTER][2]                                = { STATE_PING_SEND_I2CRX,      NULL},
[STATE_ROGER_ROLE_MASTER][3]                                = { STATE_INIT,                 NULL},

[STATE_ROGER_ROLE_SLAVE][SIG_STATIC]                        = { STATE_ROGER_ROLE_SLAVE,     NULL},
[STATE_ROGER_ROLE_SLAVE][1]                                 = { STATE_PONGSEND_UART,        NULL},
[STATE_ROGER_ROLE_SLAVE][2]                                 = { STATE_PONGSEND_I2C,         NULL},
[STATE_ROGER_ROLE_SLAVE][3]                                 = { STATE_INIT,                 NULL},

};

/*=============================================================================================================*/

enum _STM_MCUMCU_STATES             current_mcumcu_state            = STATE_INIT;
enum _STM_MCUMCU_SIGNALS            current_mcumcu_signal           = SIG_STATIC;
enum _STM_MCUMCU_SIGNALS            mcumcu_signal_transition        = SIG_STATIC;

const struct STR_RSRV_SETTINGS  {
    int           Kretry;
    clock_t       TimeColWait;
    clock_t       PongTimeout;
    clock_t       PongTimeoutI2C;
    clock_t       PingTimeout;
    clock_t       PingTimeoutI2C;
}RsrvSettings = {
    3,                  /* Kretry */
    100,                /* TimeColWait */
    100                 /* PongTimeout ����� */
};

extern  struct _MCU_VIEW_PAIR   McuViewPair;
int                             retry_times           = 3;


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int *rsrv_mcumcu_prtcl_get_currect_state (void)
{
    return (int*)&current_mcumcu_state;
}



/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int resrv_mcumcu_prtcl_recv_signal (void)
{
    current_mcumcu_signal       = mcumcu_signal_transition;
    mcumcu_signal_transition    = SIG_STATIC;
    return current_mcumcu_signal;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int rsrv_mcumcu_prtcl_get_currect_signal (void)
{
    return current_mcumcu_signal;
}


/*--------------------------------------������� ��������� �������� MCUMCU--------------------------------------*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_init_protocol(int state, int signal)
{
    clock_t     timeout_timer   = clock();
    int         adress          ;
    
    rsrv_os_lock(&McuViewPair.Sem);                                      /* �������������� �������� */
    McuViewPair.Remote.UARTRx = RESERV_TREESTATE_UNKNOWN;
    McuViewPair.Remote.UARTTx = RESERV_TREESTATE_UNKNOWN;
    McuViewPair.Local.UARTRx = RESERV_TREESTATE_UNKNOWN;
    McuViewPair.Local.UARTTx = RESERV_TREESTATE_UNKNOWN;
    McuViewPair.Remote.I2CMCUComm = RESERV_TREESTATE_UNKNOWN;
    McuViewPair.Local.I2CMCUComm = RESERV_TREESTATE_UNKNOWN;
    McuViewPair.Local.Phase = RESERV_MCU_NOT_STARTED;
    rsrv_os_unlock(&McuViewPair.Sem);

    rsrv_mcumcu_protocol_init_rx();                                 /* �������������� ���������� */
                                                                        
    adress = McuViewPair.Local.Address;                            /* ���� ���������� ������� */
    while ((timeout_timer + RsrvSettings.TimeColWait*adress) > clock() ) {           
        continue;
    }

    rsrv_mcumcu_protocol_init_tx();                                     /* �������������� tx */

    mcumcu_signal_transition = SIG_INITIAL_GOTO_PINGUART;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_ping_send(int state, int signal)
{
    retry_times--;
    if (state == STATE_PING_SEND_UARTRX) {
        rsrv_mcumcu_protocol_ping_seng(RESRV_UART_INTERFACE, RsrvSettings.PingTimeout);
        mcumcu_signal_transition = SIG_PINGUART_GOTO_ANSWUART;
    } else if (state == STATE_PING_SEND_I2CRX) {
        rsrv_mcumcu_protocol_ping_seng(RESRV_I2C_INTERFACE, RsrvSettings.PingTimeoutI2C);
        mcumcu_signal_transition = SIG_PINGI2C_GOTO_ANSWI2C;
    }
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_pong_uart_answer_wait(int state, int signal)
{
    TProtocolEvent  evnt;
    int             ret;

    ret = rsrv_mcumcu_protocol_receive_events(RESRV_UART_INTERFACE, RsrvSettings.PongTimeout, &evnt);

    if (ret == RSRV_TIMEOUT) {
        retry_times--;
        if (retry_times == 0) {
            /* ���������� �����������, ��������� �� ������ ����������� i2c */
            rsrv_os_lock(&McuViewPair.Sem);                                      
            McuViewPair.Local.UARTRx  = RESERV_TREESTATE_DAMAGED;
            McuViewPair.Remote.UARTTx = RESERV_TREESTATE_DAMAGED;
            rsrv_os_unlock(&McuViewPair.Sem);                                      
            retry_times = RsrvSettings.Kretry;
            mcumcu_signal_transition = SIG_ANSWUART_GOTO_I2CPING;
            return;
        }

        mcumcu_signal_transition = SIG_ANSWUART_GOTO_UARTPING;
        return;
    }

    if (ret != RSRV_OK) {
        mcumcu_signal_transition = SIG_ANSWUART_GOTO_UARTPING;  /* �������� ����� �������� �� init */
        return;
    }

    switch ( evnt )
    {
    case RESRV_PROTOCOL_PONG_UART:
        /* ���������� �����������, ��������� �� ������ ����������� i2c */
        rsrv_os_lock(&McuViewPair.Sem);                                      
        McuViewPair.Remote.UARTTx = RESERV_TREESTATE_CHECKED;
        McuViewPair.Local.UARTRx  = RESERV_TREESTATE_CHECKED;
        rsrv_os_unlock(&McuViewPair.Sem);                                      

        retry_times = RsrvSettings.Kretry;
        mcumcu_signal_transition = SIG_ANSWUART_GOTO_I2CPING;
        break;

    case RESRV_PROTOCOL_PING_UART:                                      /* nested ping */
        mcumcu_signal_transition = SIG_ANSWUART_GOTO_UARTPONG;      
        break;

    case RESRV_PROTOCOL_PING_I2C:                                       /* nested pingi2c */
        mcumcu_signal_transition = SIG_ANSWUART_GOTO_I2CPONG;       
        break;

    case RESRV_PROTOCOL_VOTE:                                           /* nested vote */
        mcumcu_signal_transition = SIG_ANSWUART_GOTO_VOTE;          
        break;

    default:
        retry_times--;
        if (retry_times == 0) {
            /* ���������� �����������, ��������� �� ������ ����������� i2c */
            rsrv_os_lock(&McuViewPair.Sem);                                      
//            McuViewPair.Local.UARTTx = RESERV_TREESTATE_DAMAGED; ??
            McuViewPair.Local.UARTRx  = RESERV_TREESTATE_DAMAGED;
            McuViewPair.Remote.UARTTx = RESERV_TREESTATE_DAMAGED;
            rsrv_os_unlock(&McuViewPair.Sem);                                      
            retry_times = RsrvSettings.Kretry;
            mcumcu_signal_transition = SIG_ANSWUART_GOTO_I2CPING;
            return;
        }
        mcumcu_signal_transition = SIG_ANSWUART_GOTO_UARTPING;
        break;
    }
}



/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_pong_i2c_answer_wait(int state, int signal)
{
    TProtocolEvent  evnt;
    int             ret;

    ret = rsrv_mcumcu_protocol_receive_events(RESRV_I2C_INTERFACE, RsrvSettings.PongTimeout, &evnt);

    if (ret == RSRV_TIMEOUT) {
        retry_times--;
        if (retry_times == 0) {
            /* ���������� �����������, ��������� �� ������ ����������� i2c */
            rsrv_os_lock(&McuViewPair.Sem);                                      
            McuViewPair.Local.UARTTx      = RESERV_TREESTATE_DAMAGED;
            McuViewPair.Local.I2CMCUComm  = RESERV_TREESTATE_DAMAGED;
            rsrv_os_unlock(&McuViewPair.Sem);                        
                          
            retry_times = RsrvSettings.Kretry;
            mcumcu_signal_transition = SIG_ANSWI2C_GOTO_VOTE;
            return;
        }

        mcumcu_signal_transition = SIG_ANSWI2C_GOTO_I2CPING;
        return;
    }

    switch ( evnt )
    {
    case RESRV_PROTOCOL_PONG_I2C:
        /* ���������� �����������, ��������� �� ������ ����������� i2c */
        rsrv_os_lock(&McuViewPair.Sem); 
        McuViewPair.Local.UARTTx      = RESERV_TREESTATE_CHECKED;
        McuViewPair.Local.I2CMCUComm  = RESERV_TREESTATE_CHECKED;

        McuViewPair.Remote.I2CMCUComm = RESERV_TREESTATE_CHECKED;
        McuViewPair.Remote.UARTRx     = RESERV_TREESTATE_CHECKED;
        rsrv_os_unlock(&McuViewPair.Sem);                                      

        retry_times = RsrvSettings.Kretry;
        mcumcu_signal_transition = SIG_ANSWI2C_GOTO_I2CPING;
        break;

    default:
        if (ret == RSRV_TIMEOUT) {
            retry_times--;
            if (retry_times == 0) {
                /* ���������� �����������, ��������� �� ������ ����������� i2c */
                rsrv_os_lock(&McuViewPair.Sem);                                      
                McuViewPair.Local.UARTTx      = RESERV_TREESTATE_DAMAGED;
                McuViewPair.Local.I2CMCUComm  = RESERV_TREESTATE_DAMAGED;

                rsrv_os_unlock(&McuViewPair.Sem);                                      
                retry_times = RsrvSettings.Kretry;
                mcumcu_signal_transition = SIG_ANSWI2C_GOTO_VOTE;
                return;
            }

            mcumcu_signal_transition = SIG_ANSWI2C_GOTO_I2CPING;
            return;
        }

        break;
    }


}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_vote(int state, int signal)
{
    rsrv_mcumcu_protocol_send_vote(RESRV_UART_INTERFACE, RsrvSettings.PingTimeout);
    mcumcu_signal_transition = SIG_VOTE_GOTO_AGRDESC;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_send_pong(int state, int signal)
{
    if (state == STATE_PONGSEND_UART) {
        rsrv_mcumcu_protocol_send_pong(RESRV_UART_INTERFACE, RsrvSettings.PingTimeout);
//        mcumcu_ans_signal_transition = SIG_UART_GOTO_INCOMING;
    }
    else if (state == STATE_PONGSEND_I2C) {
        rsrv_mcumcu_protocol_send_pong(RESRV_I2C_INTERFACE, RsrvSettings.PingTimeoutI2C);
//        mcumcu_ans_signal_transition = SIG_I2C_GOTO_INCOMING;
    }
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_send_agreed(int state, int signal)
{
    rsrv_mcumcu_protocol_send_agreed(RESRV_UART_INTERFACE, RsrvSettings.PingTimeout);
//    mcumcu_ans_signal_transition = SIG_AGREED_GOTO_INCOMING;
}


