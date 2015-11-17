/*
 * inttask_messages.c
 *
 *  Created on: 23 окт. 2015 г.
 *      Author: kiryanov
 */

/*includes==========================================================================================================*/

#include <string.h>
#include "Threads/inc/inttsk_mgs.h"
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"


/*defines===========================================================================================================*/

#define   PMBUS_ALARM_TIMEOUT		      2500 //!< таймаут на реагигорвание на PMBUS_alarm


#define   MSGQUELEN_TASK_DEVICE_CONTROLLER    8
#define   MSGQUELEN_TASK_POWERMNGMNT          4


/* task device_controller */
static void                   *_dc_messages_array[SIZEOF_INTTASK]; // указатели для очереди сообщений
static OS_EVENT               *_dc_queue = NULL;
static msg_inttask_t          *_dc_currnt_recv_msg;
static msg_inttask_t          _dc_currnt_send_msg[MSGQUELEN_TASK_DEVICE_CONTROLLER];
static int                    _dc_data_index = 0;
static msg_pwr_data_t         _dc_pwmngmnt_pwrdata_msg[MSGQUELEN_TASK_DEVICE_CONTROLLER];
static int                    _dc_pwmngmnt_pwrdata_index = 0;



/* task_power_managment */    
static void                   *_pwmngmnt_messages_array[SIZEOF_INTTASK] ; // указатели для очереди сообщений
static OS_EVENT               *_pwmngmnt_queue = NULL;
static msg_inttask_t          *_pwmngmnt_currnt_recv_msg;
static msg_inttask_t          _pwmngmnt_currnt_send_msg[MSGQUELEN_TASK_POWERMNGMNT];
static int                    _pwmngmnt_index       = 0;
static msg_dc_state_data_t    _pwmngmnt_dc_data_state_msg[MSGQUELEN_TASK_POWERMNGMNT];
static msg_dc_power_data_t    _pwmngmnt_dc_data_power_msg[MSGQUELEN_TASK_POWERMNGMNT];
static int                    _pwmngmnt_dc_recv_state_index       = 0;
static int                    _pwmngmnt_dc_recv_power_index       = 0;




int open_inttask_message (void)
{
    _dc_queue = OSQCreate( _dc_messages_array, MSGQUELEN_TASK_DEVICE_CONTROLLER ) ;
    _pwmngmnt_queue = OSQCreate( _pwmngmnt_messages_array, MSGQUELEN_TASK_POWERMNGMNT ) ;
  
    return 0;
}



int close_inttask_message (void)
{
    INT8U err;
    
    OSQDel(_dc_queue, OS_DEL_ALWAYS, &err);
    OSQDel(_pwmngmnt_queue, OS_DEL_ALWAYS, &err);

    return 0;
}


static void send_to_dc(int sender_module, msg_inttask_t* message)
{
  
    sender_module = sender_module;
    
    if ( message == NULL  ) {
        return;
    }
    
    if ( message->data != NULL ) {
      switch ( message->msg )
      {
      case  GET_SHELF_STATE:
      case  SET_SHELF_STATE: 
      case  FAILURE_SIGNALING: 
      case  GET_SHELF_POWER:   {      
          memcpy(&_dc_pwmngmnt_pwrdata_msg[_dc_pwmngmnt_pwrdata_index], message->data, sizeof _dc_pwmngmnt_pwrdata_msg[_dc_pwmngmnt_pwrdata_index]);
          message->data = &_dc_pwmngmnt_pwrdata_msg[_dc_pwmngmnt_pwrdata_index];
          _dc_pwmngmnt_pwrdata_index = ( _dc_pwmngmnt_pwrdata_index + 1 == MSGQUELEN_TASK_DEVICE_CONTROLLER ) ? 0 : _dc_pwmngmnt_pwrdata_index + 1;      
      }  break;
          
      } /* switch ( message->data ) */
    } /* if ( message->data != NULL ) */
    
    memcpy(&_dc_currnt_send_msg[_dc_data_index], message, sizeof _dc_currnt_send_msg[_dc_data_index]);
    OSQPost( _dc_queue, &_dc_currnt_send_msg[_dc_data_index] );
    _dc_data_index = ( _dc_data_index + 1 == MSGQUELEN_TASK_DEVICE_CONTROLLER ) ? 0 : _dc_data_index + 1;      
}



static void send_to_pwmng(int sender_module, msg_inttask_t* message)
{
  sender_module = sender_module;
  
  if ( message->data != NULL ) {
      switch ( message->msg )
      {
      case  GET_SHELF_STATE:
      case  SET_SHELF_STATE:  {      
          memcpy(&_pwmngmnt_dc_data_state_msg[_pwmngmnt_dc_recv_state_index], message->data, sizeof _pwmngmnt_dc_data_state_msg[_pwmngmnt_dc_recv_state_index]);
          message->data = &_pwmngmnt_dc_data_state_msg[_pwmngmnt_dc_recv_state_index];
          _pwmngmnt_dc_recv_state_index = ( _pwmngmnt_dc_recv_state_index + 1 == MSGQUELEN_TASK_POWERMNGMNT ) ? 0 : _pwmngmnt_dc_recv_state_index + 1;       
      }  break;

      case GET_SHELF_POWER: {
          memcpy(&_pwmngmnt_dc_data_power_msg[_pwmngmnt_dc_recv_power_index], message->data, sizeof _pwmngmnt_dc_data_power_msg[_pwmngmnt_dc_recv_power_index]);
          message->data = &_pwmngmnt_dc_data_power_msg[_pwmngmnt_dc_recv_power_index];
          _pwmngmnt_dc_recv_power_index = ( _pwmngmnt_dc_recv_power_index + 1 == MSGQUELEN_TASK_POWERMNGMNT ) ? 0 : _pwmngmnt_dc_recv_power_index + 1;       
      } break;
          
      } /* switch ( message->data ) */
  }

    memcpy(&_pwmngmnt_currnt_send_msg[_pwmngmnt_index], message, sizeof _pwmngmnt_currnt_send_msg[_pwmngmnt_index]);
    OSQPost( _pwmngmnt_queue, &_pwmngmnt_currnt_send_msg[_pwmngmnt_index] );
    _pwmngmnt_index = ( _pwmngmnt_index + 1 == MSGQUELEN_TASK_POWERMNGMNT ) ? 0 : _pwmngmnt_index + 1;      
}



static msg_inttask_t *recv_dc(int sender_module, void* message_data)
{
    INT8U         return_code;
    
    _dc_currnt_recv_msg = OSQPend( _dc_queue, PMBUS_ALARM_TIMEOUT, &return_code );
    if (return_code != OS_ERR_NONE) {
      _dc_currnt_recv_msg = NULL; 
    }
    if ( message_data != NULL && _dc_currnt_recv_msg != NULL ) {
      message_data = _dc_currnt_recv_msg->data;
    }    
    
    return _dc_currnt_recv_msg;
}



static msg_inttask_t *recv_pwmng(int sender_module, void* message_data)
{
    INT8U         return_code;
    
    _pwmngmnt_currnt_recv_msg = OSQPend( _pwmngmnt_queue, PMBUS_ALARM_TIMEOUT, &return_code );
    if (return_code != OS_ERR_NONE) {
      _pwmngmnt_currnt_recv_msg = NULL; 
    }
    if ( message_data != NULL && _pwmngmnt_currnt_recv_msg != NULL ) {
      message_data = _pwmngmnt_currnt_recv_msg->data;
    }    
    
    return _pwmngmnt_currnt_recv_msg;
}


static msg_inttask_t *tryrecv_dc(int sender_module, void* message_data) 
{
    INT8U         return_code;
    
    _dc_currnt_recv_msg = OSQAccept( _dc_queue, &return_code );
    if (return_code != OS_ERR_NONE) {
      _dc_currnt_recv_msg = NULL; 
    }
    if ( message_data != NULL && _dc_currnt_recv_msg != NULL ) {
      message_data = _dc_currnt_recv_msg->data;
    }    
    
    return _dc_currnt_recv_msg;
  
}

static msg_inttask_t *tryrecv_pwmng(int sender_module, void* message_data) 
{
    INT8U         return_code;
    
    _pwmngmnt_currnt_recv_msg = OSQAccept( _pwmngmnt_queue, &return_code );
    if (return_code != OS_ERR_NONE) {
      _pwmngmnt_currnt_recv_msg = NULL; 
    }
    if ( message_data != NULL && _pwmngmnt_currnt_recv_msg != NULL ) {
      message_data = _pwmngmnt_currnt_recv_msg->data;
    }    
    
    return _pwmngmnt_currnt_recv_msg;  
}



int   send_inttask_message (int recv_module, int sender_module, msg_inttask_t* message)
{
    switch ( recv_module )
    {
    case DEVICE_CONTROLLER: { send_to_dc(sender_module, message);     }  break;
    case POWER_MANAGMENT:   { send_to_pwmng(sender_module, message);  }  break;
    }
    
    return 0;
}


msg_inttask_t* 	recv_inttask_message (int recv_module, int sender_module, void* message_data)
{
    switch ( recv_module )
    {
    case DEVICE_CONTROLLER: return recv_dc(sender_module, message_data);
    case POWER_MANAGMENT:   return recv_pwmng(sender_module, message_data);              
    }
    
    return NULL;
}


msg_inttask_t* 	tryrecv_inttask_message (int recv_module, int sender_module, void* message_data)
{
    switch ( recv_module )
    {
    case DEVICE_CONTROLLER: return tryrecv_dc(sender_module, message_data);
    case POWER_MANAGMENT:   return tryrecv_pwmng(sender_module, message_data);              
    }
    
    return NULL;
}

