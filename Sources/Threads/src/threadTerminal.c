/*!
\file threadTerminal.c
\brief ����� ����� �� UART � �� ��� �������
\detail � callback (���������� �� isr) ����������� �� ��������� ������� � ���������� � taskTerminal, ��� ���������� � ��������� ����� � ��������������.
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

/*! \brief  ���� ��������� � ����� ��������� ������-��������� */

typedef enum __TermMessage_t 
{
	UART_RCV_STRING     = 0,            /* ������� �����                                    */
        UART_RCV_UNALPHA    = 1,            /* ����� � ����������� ���������                    */
        UART_SND_LOG_NEED   = 2,            /* ����� �������� ��������� ��������� �� ���������  */
} TermMessage_t;

typedef struct __taskTerm_message_t
{
	TermMessage_t message_type ;
	size_t rcvmsg_len ;
} taskTerm_message_t ;


/*=============================================================================================================*/

#define MAX_BUFF_LEN	        512
#define UART0_TERMINAL_SPEED    115200 /*!< �������� ������ � ����������                */
#define ENDLINE_SYM             '\r'

#define taskTerm_messages_len 8         /*!< ������ ���������� ��������� � taskCU   */
/*=============================================================================================================*/

static _BOOL  terminal_command( taskTerm_message_t *message_desc, T8_CircBuffer *p_rcv_buff, T8_CircBuffer *p_answ_buff, term_command_definition *find_cmd);
static void   terminal_send_log( taskTerm_message_t *message_desc,T8_CircBuffer *p_rcv_buff, T8_CircBuffer *p_answ_buff, uart_desc_t *uart_desc, term_command_definition *find_cmd);


void            terminal_uart_rcv_callback( const unsigned int ch );
unsigned int    terminal_uart_send_callback( unsigned int * p, const unsigned int len_max );

/*=============================================================================================================*/

void *start_CONSOLE_COMMANDS = __segment_begin("console_commands");
void *stop_CONSOLE_COMMANDS  = __segment_end("console_commands"); 

//! ���������� ������ � ������� [0x8* ... 0x0D]
static size_t                   uart_rcvmsg_len = 0 ;
//static const size_t             command_len = 6 ;
static taskTerm_message_t       taskTerm_message[taskTerm_messages_len] ;
static size_t                   taskTerm_message_cur_num = 0 ;
static OS_EVENT                 *termRcvQueue = 0;
static void                     *messages_array[taskTerm_messages_len] ; // ��������� ��� ������� ���������

CIRCBUFFER( terminal_rcv_buff, MAX_BUFF_LEN ); // ������ ������ ����� ����� �� �����!
CIRCBUFFER( terminal_snd_buff, MAX_BUFF_LEN ); // ������ ������ ����� ����� �� �����!

uart_desc_t uart0_desc = 
{
    /* �������� */
     0,
     UART0_TERMINAL_SPEED,
     0,
     terminal_uart_rcv_callback, // ��������� �� callback �� ����
     terminal_uart_send_callback, // ��������� �� callback �� ��������
     0
};

/*=============================================================================================================*/
void UART0_Handler(void) 
{
    uart_isr( &uart0_desc );
}

void terminal_uart_rcv_callback( const unsigned int ch )
{  
  /* ��������� ��������� ������ */   
  if ( uart_rcvmsg_len == 0 && ch == 0x20 )                                  /* ������� ������� � ������ ������ */
  {  return;  }
                                                                            /* �������� ��� ���� ������ �������� ������� */
  CircBuffer_push_one_tender( &terminal_snd_buff, (u8)(ch & 0xFF) );
  uart_terminal_start_tx(&uart0_desc);
    
      
  if (ch == ENDLINE_SYM)                                                    /* ������� ����� ������, �������� � ����� ��������� ��������� */
  {     
        CircBuffer_push_one_tender( &terminal_rcv_buff, (u8)('\0') ) ;        
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
    
  CircBuffer_push_one_tender( &terminal_rcv_buff, (u8)(ch & 0xFF) ) ;        /* �������� ������ � ��������� ����� */
  ++uart_rcvmsg_len;
  
  {;}     // else if ( message_type_current == UART_RCV_STRING && !isalnum(ch) )                                                /* ��� ���������� ������� */
  

}

unsigned int terminal_uart_send_callback( unsigned int * p, const unsigned int len_max )
{
    u8 i;
    size_t actual_sz ;
    CircBuffer_read( &terminal_snd_buff, (u8*)&i, len_max, &actual_sz );
    *p = (unsigned int )i ;
    if( CircBuffer_actual_size( &terminal_snd_buff ) == 0 ){
		if( terminal_snd_buff.iHead != terminal_snd_buff.iTail ){
			i = *p ;
		}
        // ������ ��������� -- ��������� ��������
        uart_terminal_stop_tx(&uart0_desc);
    }
    return len_max ;
}



static task_terminal_init (void)
{
    // ������� ��������� � ���� ����
    termRcvQueue = OSQCreate( messages_array, taskTerm_messages_len ) ;    
    
    uart_terminal_init( &uart0_desc );      
}



void task_terminal (void *pdata)
{
    INT8U                   return_code = OS_ERR_NONE;
    taskTerm_message_t      *qCurMessage ;
    term_command_definition *last_cmd;
    
    pdata = pdata;      // ����� �� ���� warning'� � ���������������    
    task_terminal_init();

    while( TRUE ){
      
        OSTimeDly( 1000 );      
        CircBuffer_push_one_erasingdata( &terminal_snd_buff, '\r' );                                /* �������� ����������� �� ���� */ 
        CircBuffer_push_one_erasingdata( &terminal_snd_buff, '\n' );
        CircBuffer_push_one_erasingdata( &terminal_snd_buff, '$' );
        uart_terminal_start_tx(&uart0_desc) ;
                                                                                                         
    	qCurMessage = (taskTerm_message_t*)OSQPend( termRcvQueue, 0, &return_code );                /* ���������� ������ ��������� (������� ��������� �����) */
        assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_TIMEOUT) );
              
        if( terminal_command( qCurMessage, &terminal_rcv_buff, &terminal_snd_buff, last_cmd)) {     /*  ������������ ������� � �������� ����� */
            uart_terminal_start_tx(&uart0_desc) ;
        }
                                                                                                
        terminal_send_log( qCurMessage, &terminal_rcv_buff, &terminal_snd_buff, &uart0_desc, last_cmd ); /* �������� ��� ���� ����� */
    }
}


static void terminal_send_log
( 
    taskTerm_message_t          *message_desc, 
    T8_CircBuffer               *p_rcv_buff, 
    T8_CircBuffer               *p_answ_buff,
    uart_desc_t                  *uart_desc,
    term_command_definition     *find_cmd    
)
{
    ;

        /*
            while ( cur_cmd->get_log_message_num() )
            {
                cur_cmd->getmessage()
                uart_terminal_start_tx(&uart0_desc) ;
                while ( uart_terminal_is_transmitte(&uart0_desc) )
                { continue; }
            }
        */
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


/* ��������� ���������� ������� */
static _BOOL terminal_command
( 
    taskTerm_message_t          *message_desc, 
    T8_CircBuffer               *p_rcv_buff, 
    T8_CircBuffer               *p_answ_buff,
    term_command_definition     *find_cmd    
)
{
    size_t i ;
    size_t recv_buff_len ;
    size_t rcv_len;
    char rcv_buff[ 64 ];
    char send_buff[ 64 ];
//    terminal_answer_t answer ;    
    term_command_definition *ptr_cmd;
    
    if ( message_desc->message_type == UART_SND_LOG_NEED )  {
        return FALSE ;
    }      
    
    rcv_len =  message_desc->rcvmsg_len;                                                    /* ������ ��������� �� ������������ ������ */
    if( CircBuffer_read( p_rcv_buff, (u8*)rcv_buff, rcv_len, &recv_buff_len ) == ERROR ) {
        return FALSE ;
    }
    
    if ( message_desc->message_type == UART_RCV_STRING ) {
    
        ptr_cmd = find_command((char*)&rcv_buff[0]);
        if (ptr_cmd != NULL) {
      
            int  answ_len = ptr_cmd->callback((const char*)rcv_buff, send_buff, 64);
            find_cmd = ptr_cmd;
            if ( answ_len ) {
                CircBuffer_push_one_erasingdata( &terminal_snd_buff, '\r' );
                CircBuffer_push_one_erasingdata( &terminal_snd_buff, '\n' );
        
                for( i = 0; i < answ_len; ++i ){
                    CircBuffer_push_one_erasingdata( p_answ_buff, (u8)send_buff[i] );
                }
                find_cmd = find_cmd;
                return TRUE;
            }
        } /* if (ptr_cmd != NULL) */
        
    } else if ( message_desc->message_type == UART_RCV_UNALPHA ) {
        ;      
    }/* if ( message_desc->message_type == UART_RCV_STRING ) */
        
    return FALSE ;
}


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
