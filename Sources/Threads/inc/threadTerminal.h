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


#define REGISTER_COMMAND(name, func)    _Pragma("location=\"console_commands\"") \
                                        const term_command_definition func##_handler = \
                                        { name, func };


typedef int (*t_term_cmd_callback_ptr)(const char* in, char* out, size_t out_len);
typedef struct T_TERM_COMMAND_HANDLER
{
  const char                *cmd_name;
  t_term_cmd_callback_ptr   callback;
}term_command_definition;

void task_terminal(void *pdata);

//void terminal_uart_rcv_callback( const unsigned int ch );
//unsigned int terminal_uart_send_callback( u8 * p, const unsigned int sz );

#endif
