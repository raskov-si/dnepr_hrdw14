/*!
\file threadCU.h
\brief поток связи по UART с CU
\detail В callback (вызывается из isr) принимается всё сообщение целиком и посылается в taskCU, где копируется в локальный буфер и обрабатывается.
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
#include "Application/inc/T8_Dnepr_ProfileStorage.h"

void backplane_uart_isr_hook(const unsigned int);
void taskCU (void *pdata);

extern PARAM_INDEX param_ix[];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! массив содержимых сообщений в taskCU
#define taskCU_messages_len 8
static taskCU_message_t taskCU_message[taskCU_messages_len] ;
static size_t taskCU_message_cur_num = 0 ;
static OS_EVENT  *cuRcvQueue = 0;

// TODO: выровнить начало
//! Буфер для принятой команды из UART
CIRCBUFFER( uart_rcv_buff, 512 ); // размер буфера знать нигде не нужно!

//! буфер для текущей обработки принятой команды
#define TASK_CU_RECV_BUFF_LEN 512 
s8 taskCU_recv_buff[TASK_CU_RECV_BUFF_LEN] ;

#define UART_ANSWER_BUFFER_LEN	2048
size_t uart_answer_buff_cnt = 0 ; //!< текущий байт на передачу из буфера
//! буфер, для записи ответа в UART
s8 uart_answer_buff[UART_ANSWER_BUFFER_LEN] ;

//! на шине UART встретился наш адрес
_BOOL   met_address_on_uart = 0 ;
//! количество байтов в посылке [0x8* ... 0x0D]
size_t  uart_rcvmsg_len = 0 ;

//! определяет получаем или нет команды по uart'у
static volatile enum UART_CU_LISTENING {
    NOT_LISTEN, // команды по uart не проходят
    LISTEN      // прослушиваем uart в обычном режиме
} __uart_listening = NOT_LISTEN ;

static u32 __arrive_time = 0 ;
static u32 __dispatch_time = 0 ;
static u32 __max_delay_time = 0 ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief callback из прерывания от UART1
//! \param bt собсно принятые биты

static void backplane_uart_isr_hook(const unsigned int bt)
{
    // На 75 МГц сам обработчик длится следующие времена:
	// 		* просто приём прибл. --				6 мкс
	//		* передача в taskCU через очередь --	12 мкс
	// Время между передачей сообщения из ISR и до начала его обработки в потоке -- 20 мкс
	// Время между приходом прерывания и началом обработки соотв. сообщения в потоке -- 25.6 мкс

    if( __uart_listening == NOT_LISTEN ){
        return ;
    }
    // наш адрес -- начинаем записывать в буфер
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
        
    	// кладём в очередь taskCU 
        met_address_on_uart = FALSE ;
        taskCU_message[taskCU_message_cur_num].message_type = PROFILE_STRING ;
        taskCU_message[taskCU_message_cur_num].rcvmsg_len = uart_rcvmsg_len ;
        // важно: считается, что в taskCU сообщения обрабатываются последовательно,
        // поэтому передаётся указатель на один и тот же приёмный кольцевой буфер,
        // из которого в принимающем потоке вынимаются команды
        taskCU_message[taskCU_message_cur_num].rcv_cbuff = &uart_rcv_buff ;
		CU_send_queue_message(&taskCU_message[taskCU_message_cur_num]) ;
		taskCU_message_cur_num++ ;
        if(taskCU_message_cur_num >= taskCU_messages_len )
            taskCU_message_cur_num = 0 ;
    }
    
} 


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief callback вызывается из UART ISR при освобождении передатчика UART1
//! \details пишет байты в snd_buff, возвращает количество записанных байт
//! \param snd_buff массив, куда будут записаны байты на передачу
//! \param len_max размер массива snd_buff
//! \retval сколько байт было записано

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

T8_Dnepr_LedColorTypedef get_now_led_state(void);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief Задача обмена с ПК по UART
void  taskCU (void *pdata)
{
	size_t recv_buff_len = 0 ; // длина строки в приёмном буфереxx
    INT8U return_code = OS_ERR_NONE;
    void    *messages_array[taskCU_messages_len] ; // указатели для очереди сообщений
    taskCU_message_t *qCurMessage ;
    
    pdata = pdata;      // чтобы не было warning'а о неиспользовании

    ////////////////////////////////////////////////////////////////////////////
    // инициализируем профиль до того, как включаем uart

	// очередь сообщений в этот таск
    cuRcvQueue = OSQCreate( messages_array, taskCU_messages_len ) ;

    UART_Profile_Init( &backplane_uart_isr_hook, &backplane_uart_isr_snd_hook ) ;

    // Команда прочитать блоки питания.
    Dnepr_DControl_ReinitPowerLimitSource() ;

    ////////////////////////////////////////////////////////////////////////////
    // настройка самого uart'а, функция выводов настроенов в t8_dnepr_pins_init()

    while (TRUE) {                          /* Task body, always written as an infinite loop.          */
        __uart_listening = LISTEN ;
        qCurMessage = (taskCU_message_t*)OSQPend( cuRcvQueue, 2000, &return_code );
        assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_TIMEOUT) );

        // если вышли по таймауту -- контрона точно нет, гасим светодиод
        if(return_code == OS_ERR_TIMEOUT){
//            cpu_profile_flag = FALSE;
            Dnepr_Measure_SetCPULed( (T8_Dnepr_LedTypedef){NONE, FALSE} );
			continue ;
        }
//        cpu_profile_flag = TRUE;

        // если пришла команда, о том что нужно перечитать параметры sfp
        if(qCurMessage->message_type == RENEW_SFP_PARAMS){
            __uart_listening = NOT_LISTEN ;
            Dnepr_Params_renew_sfp_thresholds();
            continue;
        // пришла команда профиля переформатировать eeprom backplane'а
        } else if(qCurMessage->message_type == FORMAT_BPEEPROM){
            // выключаем приём команд по uart'у, чтобы после выполнения форматирования не отсылать ответы на принятые
            __uart_listening = NOT_LISTEN ;
            // переформатирование и синхронизация ~ 30 секунд
            // Dnepr_eeprom_format() ;
            // // к этому моменту поток Dnepr_BPEEPROM_SyncParam_thread уже запущен, но  так как у этого потока (taskCU)
            // // приоритет выше, сначала выполнится все текущие действия, а потом поток Dnepr_BPEEPROM_SyncParam_thread 
            // // начнет писать недозаписанные параметры в заново отформатированную eeprom backplane'а
            // Dnepr_eeprom_sync() ;
            continue ;
        // надо сбросить параметры в умолчальные значения
        } else if( qCurMessage->message_type == RESET2DEFAULT ){
            Dnepr_IPADDRValueUpdate( &param_ix[IPAddress], (void*)"192.168.1.1" );
            Dnepr_IPADDRValueUpdate( &param_ix[IPMask], (void*)"255.255.255.0" );
            Dnepr_IPADDRValueUpdate( &param_ix[IPGateway], (void*)"192.168.1.1" );
            continue ;
        // пришла команда от контрона -- зажигаем светодиод
        } else {
            T8_Dnepr_LedColorTypedef  nowled = get_now_led_state();
            Dnepr_Measure_SetCPULed( (T8_Dnepr_LedTypedef){nowled, FALSE} );
        }

        // сюда будет записан ответ
        uart_answer_buff[0] = 0 ;
        // копируем к себе, чтобы работать линейным массивом, и чтобы у приёмника освободить
        // место
        if(qCurMessage->rcvmsg_len > TASK_CU_RECV_BUFF_LEN){
            break ;
        }
        if(CircBuffer_read( qCurMessage->rcv_cbuff, (u8*)&taskCU_recv_buff[0], qCurMessage->rcvmsg_len,
             &recv_buff_len ) == ERROR)
            // произошло переполнение приёмного буфера, ничего не делаем
            break ;
		taskCU_recv_buff[recv_buff_len] = 0 ;

		uart_answer_buff[0] = 0 ;
		uart_answer_buff_cnt = 0 ;

        // Защищаем двойной буфер динамических параметров от переключения.
        // Здесь, чтобы профильная система ничего не знала о функциях ОС.
        // Всё равно абсолютное большинство времени работаем только с динмаическими
        // параметрами.
        Dnepr_Measure_DynBuff_Lock();

        // в буфере uart_rcv_buff принятая посылка
		// формируем ответ, на 75 МГц одна строка статических данных формируется
        // за 140 мкс начиная с момента приёма сообщения потоком и заканчивая
        // началом отправки
        if( Profile_message_processing( taskCU_recv_buff, qCurMessage->rcvmsg_len,
        		 uart_answer_buff ) == OK ){
            // отсылаем ответ
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
