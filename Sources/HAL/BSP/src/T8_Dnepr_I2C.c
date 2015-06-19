/*!
\file T8_Dnepr_I2C.c
\brief Функции I2C, абстрагирующие шину I2C Днепра для драйверов микросхем
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/



#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/BSP/inc/T8_Dnepr_FPGA.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Select.h"
#include "HAL/IC/inc/TI_PCA9544A.h"
#include "HAL/MCU/inc/T8_5282_timers.h"
#include "Application/inc/t8_dnepr_time_date.h"


#include "HAL/MCU/inc/PMBus_GenericDriver.h"
#include "HAL/MCU/inc/T8_5282_i2c.h"
#include "HAL/MCU/inc/I2C_GenericDriver.h"

#include "Application/inc/T8_Dnepr_TaskSynchronization.h"

#ifdef DEBUG_I2C
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "Threads/inc/threadTerminal.h"
#endif

#include "uCOS_II.H"

/*=============================================================================================================*/
#ifdef DEBUG_I2C

    #define BBOX_I2C_LEN        (1200)
    #define ALLOW               (0)
    #define DISALLOW            (1)

    #define DEV_RESET           0xE0
    #define BYTE_WRITE          0xE1
    #define BYTE_READ           0xE2
    #define WORD_WRITE          0xE3
    #define WORD_READ           0xE4
    #define SEND_CMD            0xE5
    #define READ_CMD            0xE6
    #define BYTE_ARRAY_READ     0xE7
    #define BYTE_ARRAY_WRITE    0xE8
    #define BYTE16_ARRAY_READ   0xE9
    #define BYTE16_ARRAY_WRITE  0xEA
    #define WORD_ARRAY_READ     0xEB
    #define WORD_ARRAY_WRITE    0xEC

#endif
/*=============================================================================================================*/

//! структура, указатель на которую кладётся в PMB_PeriphInterfaceTypedef::bus_info
//! и I2C_PeriphInterfaceTypedef::bus_info
typedef struct __Dnepr_I2C_driver_internal_info {
	I2C_DNEPR_BusTypedef bus_channel ; //!< номер канала I2C, который нужно включить перед транзакцией
	//! если TRUE, значит функцию драйвера шины вызвали из I2C_DNEPR_SelectBus,
	//! чтобы переключить шину, и значит мьютекс уже заблокирован и его блокировать
	//! не нужно
	_BOOL dont_block ;
	//! номер слота, SELECT которого надо включить в FPGA
	u8 nSelect ;
} Dnepr_I2C_driver_internal_info ;


#ifdef DEBUG_I2C
typedef struct I2C_LOG_STRUCT
{
//  char      operation_status[2+1];   /* "ОК", "NG" */
  u16       index;                      /* 0-65535 */
  _BOOL     operation_status;
  u8        event_code;
  u8        bus_id;
  u8        i2c_addr;
  u8        reg_addr;
  u32       time;
} t_i2c_lof;

#endif

/*=============================================================================================================*/

#ifdef DEBUG_I2C
    int         debug_i2c_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog);
    int         debug_i2c_term_get_message_num (void);
    int         debug_i2c_term_get_message     (char *outlog_buf, size_t maxlen);
    int         dnepr_i2c_debug_print_log_event (char *outlog_buf, size_t maxlen, u16 index);
    static u16  dnepr_i2c_debug_search_last(void);    
#endif

/*=============================================================================================================*/
static  clock_t     pause_pmbus_timer;
static  clock_t     pause_i2c_timer;

#ifdef DEBUG_I2C

static const char *char_num_set = "0123456789";

REGISTER_COMMAND("i2clog", debug_i2c_term, debug_i2c_term_get_message_num, debug_i2c_term_get_message, NULL);

static u16      now_event_index = 0;
static u16      print_end_index = 0;
static u16      print_beg_index = 0;
static _BOOL    only_ng_flag    = 0;
static clock_t  i2c_timer_begin_trans;
static clock_t  i2c_timer_end_trans;

_Pragma("location=\"nonvolatile_sram\"")
/* ч0рный ящик, хранящиеся в ОЗУ, питаемой от батарейки на плате. */
__no_init static t_i2c_lof  nv_bbox_i2c[BBOX_I2C_LEN];
_Pragma("location=\"nonvolatile_sram\"")
__no_init static   u8       allow_log;  
_Pragma("location=\"nonvolatile_sram\"")
__no_init static   u8       err_cnt;
_Pragma("location=\"nonvolatile_sram\"")
__no_init static   _BOOL    restart_flag;
#endif


/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_I2C
int  debug_i2c_term_get_message_num (void)
{
  if (print_beg_index >= print_end_index)  {
      only_ng_flag = FALSE;
   }

   return print_end_index - print_beg_index;
}
#endif

/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_I2C
int  debug_i2c_term_get_message     (char *outlog_buf, size_t maxlen)
{
    while (   ( only_ng_flag == TRUE )
           && ( print_beg_index < print_end_index )
           && ( nv_bbox_i2c[print_beg_index].operation_status == OK )
           && ( nv_bbox_i2c[print_beg_index].event_code != DEV_RESET )             
    ) { 
        print_beg_index++; 
    }  
   
   if (print_beg_index < print_end_index)  {
      return dnepr_i2c_debug_print_log_event(outlog_buf, maxlen, print_beg_index++);
   } else {
      only_ng_flag = FALSE;
   }
   
   
   return 0;
}
#endif



/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/

#ifdef DEBUG_I2C
int debug_i2c_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog)
{
  char  *second_word;
  char  pars_buf[5+1];
  u8    idx_beg;
  
  if ( (second_word = strstr (in, "show")) != NULL )  {
     
     char  *third_word = strstr (second_word, "all");
     
     if ( third_word != NULL ) {
        /* команда на распечатку буфера i2C */
        /* опеределяем количество записей для распечатки */
        print_beg_index = 0;
        print_end_index = (now_event_index > BBOX_I2C_LEN) ? BBOX_I2C_LEN : now_event_index;
       
        /* возвращаем в основную терминалку флаг о том что должен возвращаться лог */
        *sendlog = TERMINAL_SND_LOG_CMD;        
        return 0;
     }
     
     third_word = strstr (second_word, "state");
     if ( third_word != NULL ) {
        /* текущее состояние */       
        return snprintf(out, out_len_max, "currindex=%u, max_err=%u, err_num=%u\r\n", now_event_index, allow_log, err_cnt);
     }
     
     third_word = strstr (second_word, "ng");
     if ( third_word != NULL ) {
       /* только плохие события и ресет из всего ч.я. */
        only_ng_flag = TRUE;
        print_beg_index = 0;
        print_end_index = (now_event_index > BBOX_I2C_LEN) ? BBOX_I2C_LEN : now_event_index;
       
        /* возвращаем в основную терминалку флаг о том что должен возвращаться лог */
        *sendlog = TERMINAL_SND_LOG_CMD;        
        return 0;       
     }
     
      
     if ( (idx_beg = strcspn( second_word, char_num_set ))  > 4  )  {
        u8  sym_num = strspn (&second_word[idx_beg], char_num_set);
        u16 index;
        
        if (sym_num == 0) {
            return 0;
        }
        
        third_word = strstr (second_word, "-");
        if ( third_word != NULL ) {
            idx_beg = strcspn( third_word, char_num_set );            
            sym_num = strspn (&third_word[idx_beg], char_num_set);
            
            if (sym_num == 0) {
                return 0;
            }
            
            third_word[idx_beg + sym_num] = '\0';        
            strncpy(pars_buf, &third_word[idx_beg], 5);
            index = atoi (pars_buf)+1;
            if ( index > BBOX_I2C_LEN ) {  
                u8  circles_num =  index / BBOX_I2C_LEN ;
                index  = index - circles_num*BBOX_I2C_LEN;
            }
            print_end_index = index;
            
            idx_beg = strcspn( second_word, char_num_set );            
            sym_num = strspn (&second_word[idx_beg], char_num_set);
            
            second_word[idx_beg + sym_num] = '\0';        
            strncpy(pars_buf, &second_word[idx_beg], 5);
        
            index = atoi (pars_buf);
            if ( index > BBOX_I2C_LEN ) {  
                u8  circles_num =  index / BBOX_I2C_LEN ;
                index  = index - circles_num*BBOX_I2C_LEN;
            }
            print_beg_index = index;
       
            /* возвращаем в основную терминалку флаг о том что должен возвращаться лог */
            *sendlog = TERMINAL_SND_LOG_CMD;
            return 0;
        } else {
                
            second_word[idx_beg + sym_num] = '\0';        
            strncpy(pars_buf, &second_word[idx_beg], 5);
        
            index = atoi (pars_buf);
            if ( index > BBOX_I2C_LEN ) {  
                u8  circles_num =  index / BBOX_I2C_LEN ;
                index  = index - circles_num*BBOX_I2C_LEN;
            }
            return dnepr_i2c_debug_print_log_event(out, out_len_max, index);         
        }
    }     
         
  } else if ( (second_word = strstr (in, "seterr")) != NULL  ) {
      size_t  num_beg;
        
      if ( (num_beg = strcspn( second_word, char_num_set ))  > 8  )  {
        return 0;
      }
        
      strncpy(pars_buf, &second_word[num_beg], 2);
      pars_buf[2] = '\0';
      allow_log = atoi(pars_buf);
                
      return snprintf(out, out_len_max, "OK\r\n");
      
  } else if ( (second_word = strstr (in, "restart")) != NULL  ) {
     err_cnt = allow_log + 1;    
     memset (nv_bbox_i2c, 0xCC, BBOX_I2C_LEN * sizeof nv_bbox_i2c[0]);
     restart_flag = 1;
     
     if ( (idx_beg = strcspn( second_word, char_num_set ))  > 7  )  {
        u8  sym_num = strspn (&second_word[idx_beg], char_num_set);
        if ( sym_num == 0 )  {
            allow_log = 3;
        } else {
            second_word[idx_beg + sym_num] = '\0';        
            strncpy(pars_buf, &second_word[idx_beg], 3);        
            allow_log = atoi (pars_buf);          
        }
     } else {
        allow_log = 3;
     }
       
     err_cnt = 0;
     return snprintf(out, out_len_max, "RESTART, max_err=%u\r\n", allow_log);      
      
  } else if ( (second_word = strstr (in, "start")) != NULL  ) {
    /* разрешает логгировать шину снова */
      err_cnt = 0;      
     return snprintf(out, out_len_max, "OK\r\n");     
      
  } else if ( (second_word = strstr (in, "stop")) != NULL  ) {
     /* запрещает дальнейшую запись событий */
     err_cnt = allow_log + 1;
     
     return snprintf(out, out_len_max, "OK\r\n");     
  } else if ( (second_word = strstr (in, "clear")) != NULL  ) {
     /* запрещает дальнейшую запись событий */
     err_cnt = allow_log + 1;    
     memset (nv_bbox_i2c, 0xCC, BBOX_I2C_LEN * sizeof nv_bbox_i2c[0]);
     dnepr_i2c_debug_search_last();
     return snprintf(out, out_len_max, "CLEAR OK\r\n");     
  }

  
  
  return 0;
}
#endif


/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_I2C
static _BOOL check_right_record (t_i2c_lof   *element)
{
  t_i2c_lof *temp;
  if ( element->operation_status == TRUE ||  element->operation_status == FALSE )   {
        if (element->event_code >= DEV_RESET && element->event_code <= WORD_ARRAY_WRITE) {
            return TRUE;
        }          
  }
  temp = element;
  element = (t_i2c_lof*)debug_i2c_term_handler.cmd_name;
  element = temp;
  return FALSE;
}
#endif

/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_I2C
static u16 dnepr_i2c_debug_search_last(void)
{  
  u16   rel_current_left;
  u16   rel_current_right;
  u16   rel_half_index;
  u16   abs_current_left;
  u16   abs_current_right;
  u16   abs_half_index;
  
  /* берем середину интервала */
  rel_current_left  = abs_current_left  =  0;
  rel_current_right = abs_current_right = (BBOX_I2C_LEN - 1);
  rel_half_index    = abs_half_index    = (BBOX_I2C_LEN - 1) / 2;
  
  
  /* оперделяем текущий абсолютный индекс начала интервала */
  if ( check_right_record (&nv_bbox_i2c[rel_current_left]) == TRUE ) {
        abs_current_left = nv_bbox_i2c[rel_current_left].index;
  } else {
   /* нулевая запись не инициализирована - все в первый раз, текущий индекс - 0*/
        now_event_index = 0;
        err_cnt   = 0;
        if ( restart_flag == TRUE ) {          
            restart_flag = FALSE;
        } else {
            allow_log = 3;
        }
        return 0;
  }
  
  
  if ( abs_current_left != 0 )  {
      /* предполагаем что на конце интервала индекс события на 1 меньше чем в нулевом элементе массива */
      abs_current_right = abs_current_left -1;
      
      /* проверяем последний элемент массива */
      if ( check_right_record (&nv_bbox_i2c[rel_current_right]) == TRUE )   {
            /* достаем индекс события из конца массива */
            abs_current_right = nv_bbox_i2c[rel_current_right].index;
            
            if ( abs_current_right > abs_current_left - 1 ) {
                /* буфер закончился записываться как раз на последнем элементе */
                now_event_index = abs_current_right + 1;
                return now_event_index;
            }
      }
  } /* if ( abs_current_left != 0 ) */
  
    do
    {
        rel_half_index = (rel_current_right + rel_current_left) / 2;

        if ( check_right_record (&nv_bbox_i2c[rel_half_index]) == TRUE )  {
            abs_half_index = nv_bbox_i2c[rel_half_index].index;
        } else  {
            rel_current_right =  rel_half_index;
            abs_current_right =  nv_bbox_i2c[rel_current_right].index;
            continue;
        }

        if ( abs_half_index > abs_current_left )  {
            rel_current_left =  rel_half_index;
            abs_current_left =  abs_half_index;
        }
        else if ( abs_half_index < abs_current_left ) {
            rel_current_right =  rel_half_index;
            abs_current_right =  abs_half_index;
        }
        else {
            break;
        }
   } while ( rel_current_right - rel_current_left != 0 );

   now_event_index = abs_half_index + 1;  
   return now_event_index;
}
#endif


/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/
uint32_t     dnepr_i2c_get_timer_and_reset(clock_t  *timer_begin, clock_t  *timer_prev_end)
{
    uint32_t    interval_from_begin  =  (uint32_t)timer_get_value(timer_begin); 
    uint32_t    interval_from_prvend =  (uint32_t)timer_get_value(timer_prev_end);
    
    timer_reset(timer_prev_end);
     
    
    return interval_from_prvend - interval_from_begin;  
}

/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_I2C
static void dnepr_i2c_debug_set_event 
(
    _BOOL                   ack_status,
    u8                      event_code,
    I2C_DNEPR_BusTypedef    bus,
    u8                      addr,
    u8                      cmnd
)
{
    if ( (allow_log >= err_cnt) && (restart_flag != TRUE) )  {
        u8  circles_num = now_event_index / BBOX_I2C_LEN ;
        u16 real_index  = now_event_index - circles_num*BBOX_I2C_LEN;
        
        nv_bbox_i2c[real_index].index               = now_event_index;
        nv_bbox_i2c[real_index].operation_status    = ack_status;  
        nv_bbox_i2c[real_index].event_code          = event_code;  
        nv_bbox_i2c[real_index].bus_id              = bus;
        nv_bbox_i2c[real_index].i2c_addr            = addr;
        nv_bbox_i2c[real_index].reg_addr            = cmnd;        
        nv_bbox_i2c[real_index].time                = dnepr_i2c_get_timer_and_reset(&i2c_timer_begin_trans, &i2c_timer_end_trans);
        
        now_event_index++;
        err_cnt += (ack_status == FALSE);        
    }
}
#endif

/*=============================================================================================================*/
/*!  \brief 

     \return 
     \retval 
     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_I2C
int dnepr_i2c_debug_print_log_event (char *outlog_buf, size_t maxlen, u16 index)
{
  register int buf_index = 0;
  
  if ( (index > 1200) || (check_right_record(&nv_bbox_i2c[index]) != TRUE) )  {
      buf_index = snprintf (outlog_buf, maxlen, "bad entry\r\n");
      return buf_index;
  }

  buf_index += sprintf(&outlog_buf[buf_index], "%05u ", nv_bbox_i2c[index].index);
  
  switch (nv_bbox_i2c[index].operation_status)
  {
  case TRUE:    buf_index += sprintf(&outlog_buf[buf_index], "OK ");  break;
  case FALSE:   buf_index += sprintf(&outlog_buf[buf_index], "NG ");  break;
  }
  
  switch (nv_bbox_i2c[index].event_code)
  {
  case  DEV_RESET:          buf_index += sprintf(&outlog_buf[buf_index], "DEV_RES ");  break;
  case  BYTE_WRITE:         buf_index += sprintf(&outlog_buf[buf_index], "WR_BYTE ");  break;
  case  BYTE_READ:          buf_index += sprintf(&outlog_buf[buf_index], "RD_BYTE ");  break;
  case  WORD_WRITE:         buf_index += sprintf(&outlog_buf[buf_index], "WR_WORD ");  break;
  case  WORD_READ:          buf_index += sprintf(&outlog_buf[buf_index], "RD_WORD ");  break;
  case  SEND_CMD:           buf_index += sprintf(&outlog_buf[buf_index], "SND_CMD ");  break;
  case  READ_CMD:           buf_index += sprintf(&outlog_buf[buf_index], "RD__CMD ");  break;
  case  BYTE_ARRAY_READ:    buf_index += sprintf(&outlog_buf[buf_index], "RDB_ARR ");  break;
  case  BYTE_ARRAY_WRITE:   buf_index += sprintf(&outlog_buf[buf_index], "WRB_ARR ");  break;
  case  BYTE16_ARRAY_READ:  buf_index += sprintf(&outlog_buf[buf_index], "RB16ARR ");  break;
  case  BYTE16_ARRAY_WRITE: buf_index += sprintf(&outlog_buf[buf_index], "WB16ARR ");  break;
  case  WORD_ARRAY_READ:    buf_index += sprintf(&outlog_buf[buf_index], "RDW_ARR ");  break;
  case  WORD_ARRAY_WRITE:   buf_index += sprintf(&outlog_buf[buf_index], "WRW_ARR ");  break;
  }
  
  buf_index += sprintf(&outlog_buf[buf_index], "bus 0x%02X ", nv_bbox_i2c[index].bus_id);
  buf_index += sprintf(&outlog_buf[buf_index], "adr 0x%02X ", nv_bbox_i2c[index].i2c_addr);
  buf_index += sprintf(&outlog_buf[buf_index], "reg 0x%02X ", nv_bbox_i2c[index].reg_addr);

  //время
  buf_index += sprintf(&outlog_buf[buf_index], "intrvl %u uSec ", nv_bbox_i2c[index].time );
  
  buf_index += sprintf(&outlog_buf[buf_index], "\r\n");
  

  return buf_index;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_I2C_init()
{
    I2C_InitPorts();
    I2CInit_Timer( (I2CWaitTimer_t){ (u32 (*)())OSTimeGet, 10 } ); // 10 тиков таймера -- 10 мс ждём повисвший i2c
    timer_reset(&pause_i2c_timer);
    timer_reset(&pause_pmbus_timer);
        
#ifdef DEBUG_I2C
    
    /* находим последний элемент бинарным поиском, записываем событие RESET */
    now_event_index = dnepr_i2c_debug_search_last();
    timer_reset(&i2c_timer_begin_trans);
    timer_reset(&i2c_timer_end_trans);
    
    dnepr_i2c_debug_set_event(TRUE, DEV_RESET, I2C_Dnepr_CurrentBus(), 0 ,0);
    
#endif        
}



static I2C_DNEPR_BusTypedef __Bus_Switch_curState ;

I2C_DNEPR_BusTypedef I2C_DNEPR_SelectBus(I2C_DNEPR_BusTypedef mBusName)
{
	/*!
	\brief Configures switch devices to connect certain bus to MCU.
	\param mBusName Bus to connect (see \ref I2C_DNEPR_BusTypedef).
	\retval Previous switched channel
	\todo Everything about Power sequencer and Hot-Swap controller.
	*/
	I2C_DNEPR_BusTypedef prevBusState = __Bus_Switch_curState ;
	__Bus_Switch_curState = mBusName ;

	switch(mBusName){
	
	case I2C_DNEPR_PMBUS_EXT:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_5 );
		break;
		
	case I2C_DNEPR_PMBUS_INT:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_3 );
		break;
	
	case I2C_DNEPR_BP_SERNUM:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_2 );
		break;	
		
	case I2C_DNEPR_SFP_U:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_1 );
		TI_PCA9544A_EnableChannel( Dnepr_I2C_Get_I2C_9544A_Driver(), I2C_DNEPR_SWITCH_2_ADDRESS, I2C_DNEPR_SFP_U_CH);
		break;
		
	case I2C_DNEPR_SFP_L:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_1 );
		TI_PCA9544A_EnableChannel( Dnepr_I2C_Get_I2C_9544A_Driver(), I2C_DNEPR_SWITCH_2_ADDRESS, I2C_DNEPR_SFP_L_CH);
		break;		
		
	case I2C_DNEPR_IO_EXPANDER:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_1 );
		TI_PCA9544A_EnableChannel( Dnepr_I2C_Get_I2C_9544A_Driver(), I2C_DNEPR_SWITCH_2_ADDRESS, I2C_DNEPR_IO_EXP_CH);
		break;
		
	case I2C_DNEPR_PSU :
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_7 );
		break ;
	}
    return prevBusState ;
}

/*!
\brief Returns current switched channel on i2c bus
\retval Current switched channel
*/
I2C_DNEPR_BusTypedef I2C_Dnepr_CurrentBus()
{
	return __Bus_Switch_curState ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u8 Dnepr_I2C_Read_ARA()
{
	u8 ret ;
	u8 i ;
	_BOOL swtch ;
        
	T8_Dnepr_TS_I2C_Lock() ;
	// 5 раза пытаемся включить нужный select
	for( i = 0 ; i < 5; i++ ){
		if( Dnepr_Select( SELECT_SLOT_ALL, &swtch ) ){
			break ;
		}
	}
	// если было переключение (до этого SELECT был на другом слоте) --
	// делаем задержку, чтобы свитч i2c успел переключиться
	if( swtch ){
		OSTimeDly( 1 );
	}
	if( I2C_Dnepr_CurrentBus() != I2C_DNEPR_PMBUS_EXT ) {
		I2C_DNEPR_SelectBus( I2C_DNEPR_PMBUS_EXT );
        }

	I2C_ReadCommand( 0x18, &ret ) ;
        
        for( i = 0 ; i < 5 && swtch; i++ ){
	    if( Dnepr_Select( SELECT_SLOT_NONE, &swtch ) ){
	          break ;
	    }
	}        
	T8_Dnepr_TS_I2C_Unlock() ;
	
	return ret ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// функции обёртки над PMB_PeriphInterfaceTypedef, блокируют мьютекс и включают
// нужный канал i2c

#define PMBUS_TRANSACTION_BEGIN()	_BOOL   ret ;																				\
									_BOOL   swtch ;																				\
									u8		i ;																					\
									const u8 slot_num = ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->nSelect ;				\
									assert( p );																				\
                                                                        if (times == 0)  times = 1;\
									if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){						\
										T8_Dnepr_TS_I2C_Lock() ;																\
									} 																							\
									/* надо переключить SELECT */ 																\
									if( slot_num != DNEPR_I2C_NOSLOT ){															\
										for( i = 0; i < 5; i++ ){																\
											if( Dnepr_Select( (Dnepr_Select_t)slot_num, &swtch ) ){												\
												break ;																			\
											}																					\
										}																						\
										/* если правда переключали -- вставляем паузу */										\
										if( swtch ){																			\
											OSTimeDly( 1 );																		\
										}																						\
									}																						\
                                                                        if( I2C_Dnepr_CurrentBus() != ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel ) { \
										I2C_DNEPR_SelectBus( ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel ); \
                                                                        }\
                                                                        do {  \
                                                                            while ( !timer_is_expired(&pause_pmbus_timer, pause_interval) );\


#define PMBUS_TRANSACTION_END()						    timer_reset(&pause_i2c_timer);\
                                                                            timer_reset(&pause_pmbus_timer);\
                                                                        } while ( (ret == FALSE) && (--times > 0) );	\
									if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){						\
										T8_Dnepr_TS_I2C_Unlock() ;																\
									}																							\
									return ret ;



static _BOOL
__PMB_GetAcknowledge(PMB_PeriphInterfaceTypedef *p, u8 mAddr, clock_t pause_interval, u8 times )
{        
	PMBUS_TRANSACTION_BEGIN();
	ret = PMB_GetAcknowledge( mAddr );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadByte(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *pbResult, clock_t pause_interval, u8 times )
{
	PMBUS_TRANSACTION_BEGIN();
  
#ifdef DEBUG_I2C
            timer_reset(&i2c_timer_begin_trans);
#endif      
	    ret = PMB_ReadByte( mAddr, mCmd, pbResult );
#ifdef DEBUG_I2C
            dnepr_i2c_debug_set_event(ret, BYTE_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif      
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadWord(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 *pwResult, clock_t pause_interval, u8 times )
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = PMB_ReadWord( mAddr, mCmd, pwResult );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteByte(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 nData, clock_t pause_interval, u8 times)
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
                timer_reset(&i2c_timer_begin_trans);
#endif      
        	ret = PMB_WriteByte( mAddr, mCmd, nData );
#ifdef DEBUG_I2C
                dnepr_i2c_debug_set_event(ret, BYTE_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif          
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteWord(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 nData, clock_t pause_interval, u8 times)
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = PMB_WriteWord( mAddr, mCmd, nData );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                        
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_SendCommand(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, clock_t pause_interval, u8 times)
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = PMB_SendCommand( mAddr, mCmd);
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, SEND_CMD, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                        
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadMultipleBytes(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = PMB_ReadMultipleBytes( mAddr, mCmd, anData, nBytesQuantity);
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE_ARRAY_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteMultipleBytes(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = PMB_WriteMultipleBytes( mAddr, mCmd, anData, nBytesQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE_ARRAY_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                        
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadMultipleWords(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = PMB_ReadMultipleWords( mAddr, mCmd, anData, nBytesQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_ARRAY_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                        
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteMultipleWords(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	PMBUS_TRANSACTION_BEGIN() ;
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = PMB_WriteMultipleWords( mAddr, mCmd, anData, nBytesQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_ARRAY_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                        
	PMBUS_TRANSACTION_END() ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

#define I2C_TRANSACTION_BEGIN() _BOOL ret ; 																			\
	u8 i ;																												\
	_BOOL swtch ;																										\
	I2C_DNEPR_BusTypedef channel_switch_to	=	((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel ;			\
	u8 slot_num = ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->nSelect ;											\
	assert( p );																										\
        if (times == 0) times = 1; \
        if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){												\
		T8_Dnepr_TS_I2C_Lock() ;																						\
	}																													\
	if( slot_num != DNEPR_I2C_NOSLOT ){																					\
		for( i = 0; i < 5; i++ ){																\
			if( Dnepr_Select( (Dnepr_Select_t)slot_num, &swtch ) ){												\
				break ;																			\
			}																					\
		}																						\
		/* если правда переключали -- вставляем паузу */										\
		if( swtch ){																			\
			OSTimeDly( 1 );																		\
		}																						\
	}																													\
        if( (channel_switch_to != 0) && (I2C_Dnepr_CurrentBus() != channel_switch_to) )		{								\
		I2C_DNEPR_SelectBus( ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel ); \
        }\
        do { \
            while ( !timer_is_expired(&pause_i2c_timer, pause_interval) );\

								
#define I2C_TRANSACTION_END()           timer_reset(&pause_i2c_timer);\
                                        timer_reset(&pause_pmbus_timer);\
                                     } while ( (ret == FALSE) && (--times > 0) );	\
                                     if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){	\
                            		T8_Dnepr_TS_I2C_Unlock() ;				\
	                             }							\
	                            return ret ;

static _BOOL
__I2C_ReadByte( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *pbResult, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = I2C_ReadByte( mAddr, mCmd, pbResult ) ;
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                        
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadWord( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 *pwResult, clock_t pause_interval, u8 times )
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_ReadWord( mAddr, mCmd, pwResult );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                        
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteByte( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 nData, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =   I2C_WriteByte( mAddr, mCmd, nData);
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteWord( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 nData, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_WriteWord( mAddr, mCmd, nData );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_SendCommand( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =   I2C_SendCommand( mAddr, mCmd );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, SEND_CMD, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadCommand( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 *pbResult, clock_t pause_interval, u8 times )
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret = I2C_ReadCommand( mAddr, pbResult );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, READ_CMD, I2C_Dnepr_CurrentBus(), mAddr, *pbResult);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadMultipleBytes( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_ReadMultipleBytes( mAddr, mCmd, anData, nBytesQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE_ARRAY_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                        
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteMultipleBytes( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_WriteMultipleBytes( mAddr, mCmd, anData, nBytesQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE_ARRAY_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadMultipleBytes_16( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_ReadMultipleBytes_16( mAddr, mCmd, anData, nBytesQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_ARRAY_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteMultipleBytes_16( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_WriteMultipleBytes_16( mAddr, mCmd, anData, nBytesQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE16_ARRAY_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadMultipleWords( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_ReadMultipleWords( mAddr, mCmd, anData, nWordsQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, BYTE16_ARRAY_READ, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteMultipleWords( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity, clock_t pause_interval, u8 times)
{
	I2C_TRANSACTION_BEGIN();
#ifdef DEBUG_I2C
        timer_reset(&i2c_timer_begin_trans);
#endif      
	ret =  I2C_WriteMultipleWords( mAddr, mCmd, anData, nWordsQuantity );
#ifdef DEBUG_I2C
        dnepr_i2c_debug_set_event(ret, WORD_ARRAY_WRITE, I2C_Dnepr_CurrentBus(), mAddr, mCmd);
#endif                                
	I2C_TRANSACTION_END();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// функции, возвращают указатели на заполненные структуры специально для доступа
// в указанные в их названиях каналы

//! конструирует I2C_PeriphInterfaceTypedef для устройств на PMBus
#define DEFINE_PMBus_Struct(name, bus_info) static		 						\
PMB_PeriphInterfaceTypedef name =												\
{ 	__PMB_ReadByte, __PMB_ReadWord, __PMB_WriteByte, __PMB_WriteWord, 			\
	__PMB_SendCommand, __PMB_ReadMultipleBytes, __PMB_WriteMultipleBytes,		\
	__PMB_ReadMultipleWords, __PMB_WriteMultipleWords, __PMB_GetAcknowledge, bus_info }

Dnepr_I2C_driver_internal_info __pmb_businfo_ext = { I2C_DNEPR_PMBUS_EXT, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_ext, &__pmb_businfo_ext );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_EXT_Driver( const u8 nSlot )
{
	assert( nSlot < I2C_DNEPR_NUMBER_OF_SLOTS );
	((Dnepr_I2C_driver_internal_info*)(__pmb_ext.bus_info))->nSelect = nSlot ;
	return &__pmb_ext ;
}

Dnepr_I2C_driver_internal_info __pmb_businfo_psu = { I2C_DNEPR_PSU, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_psu, &__pmb_businfo_psu );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_PSU_Driver()
{	return &__pmb_psu ; }

Dnepr_I2C_driver_internal_info __pmb_businfo_int = { I2C_DNEPR_PMBUS_INT, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_int, &__pmb_businfo_int );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_INT_Driver()
{	return &__pmb_int ; }

Dnepr_I2C_driver_internal_info __pmb_businfo_int_select = { I2C_DNEPR_PMBUS_INT, TRUE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_int_select, &__pmb_businfo_int_select );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver()
{	return &__pmb_int_select ; }


//! конструирует I2C_PeriphInterfaceTypedef для устройств на I2C
#define DEFINE_I2C_Struct(name, bus_info) static		 						\
I2C_PeriphInterfaceTypedef name =												\
{ 	__I2C_ReadByte, __I2C_ReadWord, __I2C_WriteByte, __I2C_WriteWord, 			\
	__I2C_SendCommand, __I2C_ReadCommand, __I2C_ReadMultipleBytes, 				\
	__I2C_WriteMultipleBytes, __I2C_ReadMultipleWords, __I2C_WriteMultipleWords,\
	__I2C_ReadMultipleBytes_16, __I2C_WriteMultipleBytes_16, 					\
	bus_info }

Dnepr_I2C_driver_internal_info __i2c_businfo_bp_sernum = { I2C_DNEPR_BP_SERNUM, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_bp_sernum, &__i2c_businfo_bp_sernum );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_BP_SerNum_Driver()
{	return &__i2c_bp_sernum ; }

Dnepr_I2C_driver_internal_info __i2c_businfo_io_expander = { I2C_DNEPR_IO_EXPANDER, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_io_expander, &__i2c_businfo_io_expander );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_IO_Expander_Driver()
{	return &__i2c_io_expander ; }

Dnepr_I2C_driver_internal_info __i2c_businfo_sfp_u = { I2C_DNEPR_SFP_U, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_sfp_u, &__i2c_businfo_sfp_u );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_SFP_U_Driver()
{	return &__i2c_sfp_u ; }

Dnepr_I2C_driver_internal_info __i2c_businfo_sfp_l = { I2C_DNEPR_SFP_L, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_sfp_l, &__i2c_businfo_sfp_l );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_SFP_L_Driver()
{	return &__i2c_sfp_l ; }

DEFINE_I2C_Struct( __i2c_ext, &__pmb_businfo_int );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_INT_Driver()
{	return &__i2c_ext ; }

DEFINE_I2C_Struct( __i2c_bp, &__pmb_businfo_ext );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_BP_Driver( const u8 nSlot )
{
	assert( nSlot < I2C_DNEPR_NUMBER_OF_SLOTS );
	((Dnepr_I2C_driver_internal_info*)(__i2c_bp.bus_info))->nSelect = nSlot ;
	return &__i2c_bp ;
}

Dnepr_I2C_driver_internal_info __i2c_businfo_9544A = { I2C_DNEPR_NONE, TRUE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_9544A, &__i2c_businfo_9544A );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_9544A_Driver()
{	return &__i2c_9544A ; }
