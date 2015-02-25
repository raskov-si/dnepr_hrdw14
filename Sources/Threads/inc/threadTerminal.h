/*!
\file threadTerminal.h
\brief поток связи по UART с ПК для отладки
\detail В callback (вызывается из isr) принимается всё сообщение целиком и посылается в taskTerminal, где копируется в локальный буфер и обрабатывается.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date sep 2013
*/

#ifndef _THREAD_TERMINAL_H__
#define _THREAD_TERMINAL_H__

#include "support_common.h"



#define REGISTER_COMMAND(name, func, chklog, sendlog, stoplog)    _Pragma("location=\"console_commands\"") \
                                                                  const term_command_definition func##_handler = \
                                                                  { name, func, chklog, sendlog, stoplog };
typedef enum T_LOGCMD
{
    TERMINAL_NOTHING         = 0,            /* не отсылать лог                                      */
    TERMINAL_SND_LOG_CMD     = 1,            /* отсылать лог real-time приходящий с другого модуля   */
    TERMINAL_SND_LOG_ONLINE  = 2,            /* прекратить отсылать лог и ждать ввода команды        */
    TERMINAL_SND_LOG_BREAK   = 3,            /* прекратить отсылать лог и ждать ввода команды        */
}t_log_cmd;

typedef int  (*t_term_cmd_callback_ptr)     (const char* in, char* out, size_t out_len, t_log_cmd *sendlog_flag);
typedef int  (*t_term_chklog_callback_ptr)  (void);
typedef int  (*t_term_sendlog_callback_ptr) (char *outlog_buf, size_t maxlen);
typedef void (*t_term_stoplog_callback_ptr) (void);


typedef struct T_TERM_COMMAND_HANDLER
{
  const char                  *cmd_name;
  t_term_cmd_callback_ptr     callback;
  t_term_chklog_callback_ptr  get_log_message_num;
  t_term_sendlog_callback_ptr get_log_message;
  t_term_stoplog_callback_ptr stop_log;
  
}term_command_definition;

void task_terminal(void *pdata);

//void terminal_uart_rcv_callback( const unsigned int ch );
//unsigned int terminal_uart_send_callback( u8 * p, const unsigned int sz );

#endif
