/*!
\file threadCU.h
\brief ����� ����� �� UART � CU
\detail � callback (���������� �� isr) ����������� �� ��������� ������� � ���������� � taskCU, ��� ���������� � ��������� ����� � ��������������.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include "support_common.h"

#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_UART_Profile.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/MCU/inc/T8_5282_timers.h"

#include "Threads/inc/threadCU.h"
#include "Threads/inc/threadMeasure.h"
#include "Threads/inc/threadDeviceController.h"
#include "common_lib/T8_CircBuffer.h"
#include "Profile/inc/profile_processing.h"
#include "Application/inc/T8_Dnepr_Profile_params.h"

void backplane_uart_isr_hook(const unsigned int);
void taskCU (void *pdata);

extern PARAM_INDEX param_ix[];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! ������ ���������� ��������� � taskCU
#define taskCU_messages_len 8
static taskCU_message_t taskCU_message[taskCU_messages_len] ;
static size_t taskCU_message_cur_num = 0 ;
static OS_EVENT  *cuRcvQueue = 0;

// TODO: ��������� ������
//! ����� ��� �������� ������� �� UART
CIRCBUFFER( uart_rcv_buff, 512 ); // ������ ������ ����� ����� �� �����!

//! ����� ��� ������� ��������� �������� �������
#define TASK_CU_RECV_BUFF_LEN 512 
s8 taskCU_recv_buff[TASK_CU_RECV_BUFF_LEN] ;

#define UART_ANSWER_BUFFER_LEN	2048
size_t uart_answer_buff_cnt = 0 ; //!< ������� ���� �� �������� �� ������
//! �����, ��� ������ ������ � UART
s8 uart_answer_buff[UART_ANSWER_BUFFER_LEN] ;

//! �� ���� UART ���������� ��� �����
_BOOL   met_address_on_uart = 0 ;
//! ���������� ������ � ������� [0x8* ... 0x0D]
size_t  uart_rcvmsg_len = 0 ;

//! ���������� �������� ��� ��� ������� �� uart'�
static volatile enum UART_CU_LISTENING {
    NOT_LISTEN, // ������� �� uart �� ��������
    LISTEN      // ������������ uart � ������� ������
} __uart_listening = NOT_LISTEN ;

static u32 __arrive_time = 0 ;
static u32 __dispatch_time = 0 ;
static u32 __max_delay_time = 0 ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief callback �� ���������� �� UART1
//! \param bt ������ �������� ����

static void backplane_uart_isr_hook(const unsigned int bt)
{
    // �� 75 ��� ��� ���������� ������ ��������� �������:
	// 		* ������ ���� �����. --				6 ���
	//		* �������� � taskCU ����� ������� --	12 ���
	// ����� ����� ��������� ��������� �� ISR � �� ������ ��� ��������� � ������ -- 20 ���
	// ����� ����� �������� ���������� � ������� ��������� �����. ��������� � ������ -- 25.6 ���

    if( __uart_listening == NOT_LISTEN ){
        return ;
    }
    // ��� ����� -- �������� ���������� � �����
    if( bt  == (0x80 | Dnepr_Profile_Address())){
        met_address_on_uart = TRUE ;
        uart_rcvmsg_len = 0 ;
        Dnepr_GPIO_timepin1_on() ;
    }
    if( met_address_on_uart ){
        uart_rcvmsg_len++ ;
        CircBuffer_push_one_tender( &uart_rcv_buff, (u8)(bt & 0xFF) ) ;
    }
    if((bt == 0x0D) && met_address_on_uart){
        __arrive_time = 0 ;

        MCF_PIT_PCSR(2) = MCF_PIT_PCSR_OVW|MCF_PIT_PCSR_PIF|MCF_PIT_PCSR_EN | (u16)(MCF_PIT_PCSR_PRE(0x0D)) ;
		MCF_PIT_PMR(2) = 0xFFFF ;
		
		if( uart_rcvmsg_len >= TASK_CU_RECV_BUFF_LEN ){
			return ;
		}
        
    	// ����� � ������� taskCU 
        met_address_on_uart = FALSE ;
        taskCU_message[taskCU_message_cur_num].message_type = PROFILE_STRING ;
        taskCU_message[taskCU_message_cur_num].rcvmsg_len = uart_rcvmsg_len ;
        // �����: ���������, ��� � taskCU ��������� �������������� ���������������,
        // ������� ��������� ��������� �� ���� � ��� �� ������� ��������� �����,
        // �� �������� � ����������� ������ ���������� �������
        taskCU_message[taskCU_message_cur_num].rcv_cbuff = &uart_rcv_buff ;
		CU_send_queue_message(&taskCU_message[taskCU_message_cur_num]) ;
		taskCU_message_cur_num++ ;
        if(taskCU_message_cur_num >= taskCU_messages_len )
            taskCU_message_cur_num = 0 ;
    }
    
} 


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief callback ���������� �� UART ISR ��� ������������ ����������� UART1
//! \details ����� ����� � snd_buff, ���������� ���������� ���������� ����
//! \param snd_buff ������, ���� ����� �������� ����� �� ��������
//! \param len_max ������ ������� snd_buff
//! \retval ������� ���� ���� ��������

static unsigned int backplane_uart_isr_snd_hook( unsigned int *snd_buff, const unsigned int len_max )
{
	u8 i;
    Dnepr_GPIO_timepin2_on() ;
	for(i = 0; 	(uart_answer_buff_cnt < UART_ANSWER_BUFFER_LEN) &&
				(i < len_max) &&
				(uart_answer_buff[uart_answer_buff_cnt]);
		uart_answer_buff_cnt++, i++){
			snd_buff[i] = uart_answer_buff[uart_answer_buff_cnt] ;
	}
	if( !uart_answer_buff[uart_answer_buff_cnt] ){
		uart_backplane_stop_tx();
        __dispatch_time = 0xFFFF-MCF_PIT_PCNTR(2) ;     
        MCF_PIT_PCSR(2) = 0 ;
        // __dispatch_time = OSTimeGet() ;
        if( __dispatch_time > __arrive_time ){
            if( __dispatch_time-__arrive_time > __max_delay_time ){
                __max_delay_time = __dispatch_time - __arrive_time ;
                }
        }
        Dnepr_GPIO_timepin1_off() ;
    }
    Dnepr_GPIO_timepin2_off() ;
    return i ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief ������ ������ � �� �� UART
void  taskCU (void *pdata)
{
	size_t recv_buff_len = 0 ; // ����� ������ � ������� ������xx
    INT8U return_code = OS_ERR_NONE;
    void    *messages_array[taskCU_messages_len] ; // ��������� ��� ������� ���������
    taskCU_message_t *qCurMessage ;
    
    pdata = pdata;      // ����� �� ���� warning'� � ���������������

    ////////////////////////////////////////////////////////////////////////////
    // �������������� ������� �� ����, ��� �������� uart

	// ������� ��������� � ���� ����
    cuRcvQueue = OSQCreate( messages_array, taskCU_messages_len ) ;

    UART_Profile_Init( &backplane_uart_isr_hook, &backplane_uart_isr_snd_hook ) ;

    // ������� ��������� ����� �������.
    Dnepr_DControl_ReinitPowerLimitSource() ;

    ////////////////////////////////////////////////////////////////////////////
    // ��������� ������ uart'�, ������� ������� ���������� � t8_dnepr_pins_init()

    while (TRUE) {                          /* Task body, always written as an infinite loop.          */
        __uart_listening = LISTEN ;
        qCurMessage = (taskCU_message_t*)OSQPend( cuRcvQueue, 2000, &return_code );
        assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_TIMEOUT) );

        // ���� ����� �� �������� -- �������� ����� ���, ����� ���������
        if(return_code == OS_ERR_TIMEOUT){
            Dnepr_Measure_SetCPULed( (T8_Dnepr_LedTypedef){NONE, FALSE} );
			continue ;
        }

        // ���� ������ �������, � ��� ��� ����� ���������� ��������� sfp
        if(qCurMessage->message_type == RENEW_SFP_PARAMS){
            __uart_listening = NOT_LISTEN ;
            Dnepr_Params_renew_sfp_thresholds();
            continue;
        // ������ ������� ������� ����������������� eeprom backplane'�
        } else if(qCurMessage->message_type == FORMAT_BPEEPROM){
            // ��������� ���� ������ �� uart'�, ����� ����� ���������� �������������� �� �������� ������ �� ��������
            __uart_listening = NOT_LISTEN ;
            // ������������������ � ������������� ~ 30 ������
            // Dnepr_eeprom_format() ;
            // // � ����� ������� ����� Dnepr_BPEEPROM_SyncParam_thread ��� �������, ��  ��� ��� � ����� ������ (taskCU)
            // // ��������� ����, ������� ���������� ��� ������� ��������, � ����� ����� Dnepr_BPEEPROM_SyncParam_thread 
            // // ������ ������ �������������� ��������� � ������ ����������������� eeprom backplane'�
            // Dnepr_eeprom_sync() ;
            continue ;
        // ���� �������� ��������� � ����������� ��������
        } else if( qCurMessage->message_type == RESET2DEFAULT ){
            Dnepr_IPADDRValueUpdate( &param_ix[IPAddress], (void*)"192.168.1.1" );
            Dnepr_IPADDRValueUpdate( &param_ix[IPMask], (void*)"255.255.255.0" );
            Dnepr_IPADDRValueUpdate( &param_ix[IPGateway], (void*)"192.168.1.1" );
            continue ;
        // ������ ������� �� �������� -- �������� ���������
        } else {
            Dnepr_Measure_SetCPULed( (T8_Dnepr_LedTypedef){GREEN, FALSE} );
        }

        // ���� ����� ������� �����
        uart_answer_buff[0] = 0 ;
        // �������� � ����, ����� �������� �������� ��������, � ����� � �������� ����������
        // �����
        if(qCurMessage->rcvmsg_len > TASK_CU_RECV_BUFF_LEN){
            break ;
        }
        if(CircBuffer_read( qCurMessage->rcv_cbuff, (u8*)&taskCU_recv_buff[0], qCurMessage->rcvmsg_len,
             &recv_buff_len ) == ERROR)
            // ��������� ������������ �������� ������, ������ �� ������
            break ;
		taskCU_recv_buff[recv_buff_len] = 0 ;

		uart_answer_buff[0] = 0 ;
		uart_answer_buff_cnt = 0 ;

        // �������� ������� ����� ������������ ���������� �� ������������.
        // �����, ����� ���������� ������� ������ �� ����� � �������� ��.
        // �� ����� ���������� ����������� ������� �������� ������ � �������������
        // �����������.
        Dnepr_Measure_DynBuff_Lock();

        // � ������ uart_rcv_buff �������� �������
		// ��������� �����, �� 75 ��� ���� ������ ����������� ������ �����������
        // �� 140 ��� ������� � ������� ����� ��������� ������� � ����������
        // ������� ��������
        if( Profile_message_processing( taskCU_recv_buff, qCurMessage->rcvmsg_len,
        		 uart_answer_buff ) == OK ){
            // �������� �����
            uart_backplane_start_tx() ;
        } 
        Dnepr_Measure_DynBuff_Unlock();
	}  
}


void CU_send_queue_message( const taskCU_message_t *mess_ )
{
    if(cuRcvQueue && mess_ ){
        OSQPost( cuRcvQueue, (void*)mess_ ) ;
    }    
}

u32 CU_get_max_delay()
{
    return __max_delay_time ;
}
