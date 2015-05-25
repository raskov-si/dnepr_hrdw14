/*!
\file threadTerminal.c
\brief поток связи по UART с ПК для отладки
\detail В callback (вызывается из isr) принимается всё сообщение целиком и посылается в taskTerminal, где копируется в локальный буфер и обрабатывается.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date sep 2013
*/

#include <string.h>
#include <stdio.h>

#include "Threads/inc/threadTerminal.h"
#include "HAL/IC/inc/ST_M95M01.h"
#include "HAL/BSP/inc/T8_Dnepr_UART_Profile.h"
#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "common_lib/T8_CircBuffer.h"
#include "common_lib/crc.h"

/*=============================================================================================================*/

#pragma segment="console_commands"

/*=============================================================================================================*/

/*! \brief  типы сообщений в поток обработки задачи-терминала */


typedef enum __TermMessage_t 
{
	UART_RCV_STRING     = 0,            /* хороший прием                                        */
        UART_RCV_UNALPHA    = 1,            /* прием с непечатными символами                        */
        UART_SND_LOG_ONLINE = 2,            /* команда отсылать лог                                 */
        UART_SND_LOG_BREAK  = 3,            /* команда отсылать лог                                 */
} TermMessage_t;

typedef struct __taskTerm_message_t
{
	TermMessage_t   message_type ;
	size_t          rcvmsg_len ;
} taskTerm_message_t ;


/*=============================================================================================================*/

#define MAX_BUFF_LEN	        512
#define UART0_TERMINAL_SPEED    115200 /*!< скорость работы с терминалом                */
#define ENDLINE_SYM             '\r'

#define taskTerm_messages_len 8         /*!< массив содержимых сообщений в taskCU   */
/*=============================================================================================================*/

static void   terminal_command( const taskTerm_message_t *message_desc, T8_CircBuffer *p_rcv_buff, T8_CircBuffer *p_answ_buff, 
                                                        uart_desc_t *uart_desc, term_command_definition **find_cmd, t_log_cmd *snd_log_cmd);
static void   terminal_send_log( const taskTerm_message_t *message_desc,T8_CircBuffer *p_rcv_buff, T8_CircBuffer *p_answ_buff, 
                                                        uart_desc_t *uart_desc, term_command_definition *cur_cmd, t_log_cmd  *snd_log_cmd);


void            terminal_uart_rcv_callback( const unsigned int ch );
unsigned int    terminal_uart_send_callback( unsigned int * p, const unsigned int len_max );

/*=============================================================================================================*/

void *start_CONSOLE_COMMANDS = __segment_begin("console_commands");
void *stop_CONSOLE_COMMANDS  = __segment_end("console_commands"); 

//! количество байтов в посылке [0x8* ... 0x0D]
static size_t                   uart_rcvmsg_len = 0 ;
//static const size_t             command_len = 6 ;
static taskTerm_message_t       taskTerm_message[taskTerm_messages_len] ;
static size_t                   taskTerm_message_cur_num = 0 ;
static OS_EVENT                 *termRcvQueue = 0;
static void                     *messages_array[taskTerm_messages_len] ; // указатели для очереди сообщений

CIRCBUFFER( terminal_rcv_buff, MAX_BUFF_LEN ); // размер буфера знать нигде не нужно!
CIRCBUFFER( terminal_snd_buff, MAX_BUFF_LEN ); // размер буфера знать нигде не нужно!

uart_desc_t uart0_desc = 
{
    /* Терминал */
     0,
     UART0_TERMINAL_SPEED,
     0,
     terminal_uart_rcv_callback, // указатель на callback на приём
     terminal_uart_send_callback, // указатель на callback на передачу
     0
};

/*=============================================================================================================*/
#ifdef DEBUG_TERMINAL
void UART0_Handler(void) 
{
    uart_isr( &uart0_desc );
}
#else

void UART0_Handler(void) 
{
}

#endif


void terminal_uart_rcv_callback( const unsigned int ch )
{  
  /* принимаем очередной символ */   
  if ( uart_rcvmsg_len == 0 && ch == 0x20 )                                  /* убираем пробелы в начале строки */
  {  return;  }
    
  // askTerm_message[taskTerm_message_cur_num].message_type =  UART_SND_LOG_BREAK          /* прерываем отсылку лога (cntrl С) */                                                                            /* отсылаем эхо если начали получать команду */

  circbuffer_push_byte_erasing( &terminal_snd_buff, (u8)(ch & 0xFF) ) ;        
  uart_terminal_start_tx(&uart0_desc);
  
  
  if (ch == ENDLINE_SYM)                                                    /* приняли конец строки, отсылаем в поток обработки сообщение */
  {     
        circbuffer_push_byte_erasing( &terminal_rcv_buff, (u8)('\0') ) ;        
        ++uart_rcvmsg_len;
        taskTerm_message[taskTerm_message_cur_num].message_type = UART_RCV_STRING ;
        taskTerm_message[taskTerm_message_cur_num].rcvmsg_len = uart_rcvmsg_len ;
        if( termRcvQueue ){
            OSQPost( termRcvQueue, (void*)&taskTerm_message[taskTerm_message_cur_num] ) ;
        }    
        ++taskTerm_message_cur_num ;    
        if( taskTerm_message_cur_num >= taskTerm_messages_len ){
            taskTerm_message_cur_num = 0 ;
        }
        uart_rcvmsg_len = 0;
        return;
  }
       
  circbuffer_push_byte_erasing( &terminal_rcv_buff, (u8)(ch & 0xFF) );
  ++uart_rcvmsg_len;
  
  {;}     // else if ( message_type_current == UART_RCV_STRING && !isalnum(ch) )   /* при непечатном символе */
}


unsigned int terminal_uart_send_callback( unsigned int * p, const unsigned int len_max )
{
    u8            circle_byte;
    
    circbuffer_pop_byte(&terminal_snd_buff,  &circle_byte);
    *p = (unsigned int )circle_byte ;
      
    if( circbuffer_get_storage_data_size ( &terminal_snd_buff ) == 0)  {
        uart_terminal_stop_tx(&uart0_desc);
    }
    
    return 1;
}





static void task_terminal_init (void)
{
    // очередь сообщений в этот таск
    termRcvQueue = OSQCreate( messages_array, taskTerm_messages_len ) ;    
    
    uart_terminal_init( &uart0_desc );      
}


void task_terminal (void *pdata)
{
    INT8U                   return_code = OS_ERR_NONE;
    term_command_definition *last_cmd;
    t_log_cmd               snd_log_flag = TERMINAL_NOTHING;
    taskTerm_message_t       *qCurMessage ;
    size_t                  act_write;
    
    pdata = pdata;      // чтобы не было warning'а о неиспользовании    
    task_terminal_init();    

    while( TRUE ) {
      
        if ( snd_log_flag == TERMINAL_NOTHING )     {
            OSTimeDly( 500 );
            circbuffer_push_block_erasing(&terminal_snd_buff, &act_write, "\r\n>", 3);          /* отсылаем приглашение на ввод */
            uart_terminal_start_tx(&uart0_desc) ;
            
    	    qCurMessage = (taskTerm_message_t*)OSQPend( termRcvQueue, 0, &return_code );         /* дожидаемся приема сообщения (символа окончания ввода) */
            assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_TIMEOUT) );
        } 
        else {
          /* проверяем нет ли сообщений */
            qCurMessage = (taskTerm_message_t*)OSQAccept(termRcvQueue, &return_code);
        }
        
        if ( return_code != OS_ERR_Q_EMPTY )  {                                                  /*  обрабатываем команду и отсылаем ответ */
            terminal_command ( qCurMessage, &terminal_rcv_buff, &terminal_snd_buff, &uart0_desc, &last_cmd, &snd_log_flag);                                                                          
        }
                                                                                                /* отсылаем лог если нужно */
        terminal_send_log( qCurMessage, &terminal_rcv_buff, &terminal_snd_buff, &uart0_desc, last_cmd, &snd_log_flag);    
    }
}


static    char send_buff[ 128 + 1 ];
static    char rcv_buff[ 64 ];


static void terminal_send_log
( 
    const taskTerm_message_t    *message_desc, 
    T8_CircBuffer               *p_rcv_buff, 
    T8_CircBuffer               *p_answ_buff,
    uart_desc_t                 *uart_desc,
    term_command_definition     *cur_cmd,
    t_log_cmd                   *snd_log_cmd
)
{    
  
    if (message_desc != NULL) {
    
        if ( message_desc->message_type == UART_SND_LOG_ONLINE ) { 
            if ( *snd_log_cmd == TERMINAL_NOTHING )  {
                *snd_log_cmd = TERMINAL_SND_LOG_ONLINE;
            }
        }

        /* сообщение прервать лог */
        if ( message_desc->message_type == UART_SND_LOG_BREAK ) { 
            
        }
    }
    
    switch (*snd_log_cmd)
    {          
      
        /* вызываем стоплог обратный вызов, если он есть, выходим */
        case TERMINAL_SND_LOG_BREAK:
            {
                if ( (cur_cmd != NULL) && (cur_cmd->stop_log != NULL) )    {  
                    cur_cmd->stop_log();   
                }
                *snd_log_cmd = TERMINAL_NOTHING;
            }
            break;
            
        /* вывод лога по команде, осуществляется до тех пор пока есть информация в модуле (проверяется обратным вызовом) */
        case  TERMINAL_SND_LOG_CMD:   
            {              
                if ((cur_cmd == NULL) || (cur_cmd->get_log_message_num == NULL) || (cur_cmd->get_log_message == NULL) ) { 
                    *snd_log_cmd = TERMINAL_NOTHING;
                    break;
                }
              
                if ( cur_cmd->get_log_message_num() > 0 )  {
                   int      answ_len = cur_cmd->get_log_message(send_buff, 128);
                   size_t   left_len      = answ_len;
                   size_t   indx          = 0;
                   
//                   while ( circbuffer_get_space_size (p_answ_buff) <  answ_len)  { 
//                        continue;
//                   }
                   if ( answ_len > 0 ) {                     
                      do {
                          size_t   act_write;
//                          if ( indx > 128 ) { 
//                            break; 
//                          }
                          circbuffer_push_block(p_answ_buff, &act_write, (u8*)&send_buff[indx], left_len);
                          if ( strstr(p_answ_buff->data, "w") != NULL && cur_cmd->get_log_message_num() < 1190  )
                          {  
                              memset(send_buff, 0, 128);
                          }
                          left_len     -= act_write;
                          indx         += act_write;
                      } while ( indx < answ_len);
                        
                      uart_terminal_start_tx(uart_desc) ;
                   }
                } else {
                    *snd_log_cmd = TERMINAL_SND_LOG_BREAK; 
                }
            }
            break;
            
        /* вывод лога по инициативе отлаживаемого модуля real-time - висим пока не поступит сообщение прервать лог */
        case  TERMINAL_SND_LOG_ONLINE:   
            {
              ;
            }
            break;
            
        case TERMINAL_NOTHING:
        default:
            break;
            
        }; /* switch (message_desc->message_type) */
    
}






static term_command_definition *find_command(const char *command_body)
{
  term_command_definition *cur_cmd = (term_command_definition*)start_CONSOLE_COMMANDS;
  char  *name_end;
  char  command_name[20+1]; 
  u8 len = 20;
  
  name_end = strstr(command_body, " ");
  if ( name_end != NULL ) { 
        len = name_end - command_body;
        len = (len > 20) ? 20 : len;    
        memcpy(command_name, command_body, len);
  } else {
        strncpy( command_name, command_body, len);
  }
  command_name[len] = '\0';
    
  for (; cur_cmd < stop_CONSOLE_COMMANDS; cur_cmd++) {
     if (strcmp(command_name, cur_cmd->cmd_name) == 0)  {
        return cur_cmd;
     }
  }
  return NULL;
}


/* обработка символьных комманд */
static void terminal_command
( 
    const taskTerm_message_t    *message_desc, 
    T8_CircBuffer               *p_rcv_buff, 
    T8_CircBuffer               *p_answ_buff,
    uart_desc_t                 *uart_desc,
    term_command_definition     **find_cmd,
    t_log_cmd                   *snd_log_cmd    
)
{
    size_t i ;
    size_t recv_buff_len ;
    size_t rcv_len;
//    char send_buff[ 64 ];
//    terminal_answer_t answer ;    
    term_command_definition *ptr_cmd;
    
    if ( (message_desc == NULL) || (message_desc->message_type >= UART_SND_LOG_ONLINE) )  {
        return;
    }      
    
    rcv_len =  message_desc->rcvmsg_len;                                                    /* читаем сообщение из циклического буфера */
    
    if( circbuffer_pop_block( p_rcv_buff, (u8*)rcv_buff, rcv_len, &recv_buff_len ) == ERROR ) {    
        return;
    }
    
    if ( message_desc->message_type == UART_RCV_STRING ) {
    
        ptr_cmd = find_command((char*)&rcv_buff[0]);
        if (ptr_cmd != NULL) {          
            int  answ_len = ptr_cmd->callback((const char*)rcv_buff, send_buff, 64, snd_log_cmd);            
            *find_cmd = ptr_cmd;
            
            if ( answ_len ) {
                circbuffer_push_byte_erasing( &terminal_snd_buff, '\r' );
                circbuffer_push_byte_erasing( &terminal_snd_buff, '\n' );

//                circbuffer_push_block_erasing(&terminal_snd_buff, &act_write, "\r\n>", 3);          /* отсылаем приглашение на ввод */

                for( i = 0; i < answ_len; ++i ){
                    circbuffer_push_byte_erasing( p_answ_buff, (u8)send_buff[i] );
                }
                uart_terminal_start_tx(uart_desc);
                return;
            } else if ( *snd_log_cmd != TERMINAL_NOTHING )  {
                circbuffer_push_byte_erasing( &terminal_snd_buff, '\r' );
                circbuffer_push_byte_erasing( &terminal_snd_buff, '\n' );
                uart_terminal_start_tx(uart_desc);
            }
            
        } /* if (ptr_cmd != NULL) */
        
    } else if ( message_desc->message_type == UART_RCV_UNALPHA ) {
        ;      
    }/* if ( message_desc->message_type == UART_RCV_STRING ) */
        
    return ;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#define ANSWER_DATA_LEN 128
//
//#pragma pack(1)
//typedef struct __terminal_answer_t {
//    u8 switch_addr ;
//    u16 dev_addr ;
//    u16 reg_addr ;
//    u16 value ;
//    u32 crc ;
//} terminal_answer_t ;
//
//#pragma pack(1)
//struct terminal_command_t {
//	u8 switch_addr ;
//    u16 dev_addr ;
//    u16 reg_addr ;
//} ;
//
//_BOOL after_3200 = FALSE ;


//    MV88E6095_Handle *hSwitch ;
//
//    if( CircBuffer_read( p_rcv_buff, (u8*)&rcv_buff[0], rcv_len, &recv_buff_len ) == ERROR ){
//        return FALSE ;
//    }
//    if( (recv_buff_len != 6) || (rcv_buff[5] != 0x0D) ){
//        return FALSE ;
//    }
//    answer.switch_addr = rcv_buff[ 0 ];
//    answer.dev_addr = (rcv_buff[ 1 ] << 8) | rcv_buff[ 2 ] ;
//    answer.reg_addr = (rcv_buff[ 3 ] << 8) | rcv_buff[ 4 ] ;
//
//    if( answer.switch_addr == 0x01 ){
//		hSwitch = Dnepr_Ethernet_GetHandle_sw1() ;
//	} else if( answer.switch_addr == 0x10 ){
//		hSwitch = Dnepr_Ethernet_GetHandle_sw2() ;
//	} else {
//		return FALSE ;
//	}
//	MV88E6095_multichip_smi_read( hSwitch, answer.dev_addr, answer.reg_addr, &answer.value );
//
//    answer.crc = Crc32_mem( (u8*)&answer, sizeof(terminal_answer_t) - 4, 0 );
//
//    CircBuffer_push_one_erasingdata( p_answ_buff, 0x0D );
