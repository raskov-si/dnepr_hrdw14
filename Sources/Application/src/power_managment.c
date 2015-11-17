/* Oct 23, 2015 */

/*!  \file 	power_managment.c
 *   \brief  
 *   \details  
 *
 *
 *
 */

/*includes==========================================================================================================*/
#include <stddef.h>
#include "Application/inc/power_managment.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "Threads/inc/inttsk_mgs.h"
//#include "mdlwr/common_lib/common.h"
#include "support_common.h" // для типов данных



/*defines===========================================================================================================*/
#define SLOT_MAX_NUM		13
#define PWR_MAX_NUM		2

#define FAN_MAX_POWER		180.0
#define SLOT_MAX_POWER		200.0

#define POWER_CU		20.0

/*types=============================================================================================================*/

enum _prof_limit_source {
	PSU = 0,
	ProfileValue = 1
};

enum _slot_enable {
	Prohibited = 0,				/*!< Уверенный пуск  */
	SmartlyStart = 1,			/*!< Аккуратный пуск */
	IgnorantStart = 2			/*!< Запрет пуска  */
};

enum _POWER_SIGNALS{
	SIGNAL_STATIC = 0,
	SIGNAL_CRATECHANGE,
	SIGNAL_PROFILECHANGE,
	SIGNAL_POW_POSITIVE,
	SIGNAL_POW_NEGATIVE
};


/*prototypes========================================================================================================*/
extern I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotPresent();


static void _pwmng_get_slot_states	(int state, int signal);
static void _pwmng_power_delay		(int state, int signal);

static void _pwmng_power_ignorant_stop	(int state, int signal);
static void _pwmng_power_source_read	(int state, int signal);
static void _pwmng_power_reserve_calc	(int state, int signal);
static void _pwmng_power_fan_read	(int state, int signal);
static void _pwmng_power_slots_read	(int state, int signal);
static void _pwmng_check_passive	(int state, int signal);
static void _pwmng_fan_on		(int state, int signal);
static void _pwmng_power_calc_prohib	(int state, int signal);
static void _pwmng_power_calc_smartly	(int state, int signal);

/*variables=========================================================================================================*/
const  fsm_table_t   powermang_trans_table[13][5] =  {
/* init */
		[POW_STATE_INIT][SIGNAL_STATIC]				= {	POW_STATE_GETSLOTSSTATE, 	        _pwmng_get_slot_states          },
		[POW_STATE_INIT][SIGNAL_CRATECHANGE]			= {	POW_STATE_GETSLOTSSTATE, 		_pwmng_get_slot_states		},
		[POW_STATE_INIT][SIGNAL_PROFILECHANGE]			= {	POW_STATE_GETSLOTSSTATE, 		_pwmng_get_slot_states		},

		[POW_STATE_GETSLOTSSTATE][SIGNAL_STATIC]		= {	POW_STATE_POWER_DELAY, 			_pwmng_power_delay		},
		[POW_STATE_GETSLOTSSTATE][SIGNAL_CRATECHANGE]		= {	POW_STATE_INIT, 			_pwmng_power_delay	        },
		[POW_STATE_GETSLOTSSTATE][SIGNAL_PROFILECHANGE]		= {	POW_STATE_POWER_DELAY, 			_pwmng_power_delay		},
                

		[POW_STATE_POWER_DELAY][SIGNAL_STATIC]			= {	POW_STATE_POWER_DELAY, 			NULL	                        },
		[POW_STATE_POWER_DELAY][SIGNAL_CRATECHANGE]		= {	POW_STATE_SOURCE_READ, 			_pwmng_power_source_read        },
		[POW_STATE_POWER_DELAY][SIGNAL_PROFILECHANGE]		= {	POW_STATE_SOURCE_READ, 			_pwmng_power_source_read	},
/* init */

/* crate change */
		[POW_STATE_SOURCE_READ][SIGNAL_STATIC]			= {	POW_STATE_FAN_READ, 			_pwmng_power_fan_read		},
		[POW_STATE_SOURCE_READ][SIGNAL_CRATECHANGE]		= {	POW_STATE_SOURCE_READ, 			_pwmng_power_source_read	},
		[POW_STATE_SOURCE_READ][SIGNAL_PROFILECHANGE]		= {	POW_STATE_FAN_READ, 			_pwmng_power_fan_read		},

		[POW_STATE_FAN_READ][SIGNAL_STATIC]			= {	POW_STATE_SLOTS_READ, 			_pwmng_power_slots_read		},
		[POW_STATE_FAN_READ][SIGNAL_CRATECHANGE]		= {	POW_STATE_SOURCE_READ, 			_pwmng_power_source_read	},
		[POW_STATE_FAN_READ][SIGNAL_PROFILECHANGE]		= {	POW_STATE_SLOTS_READ, 			_pwmng_power_slots_read		},

		[POW_STATE_SLOTS_READ][SIGNAL_STATIC]			= {	POW_STATE_PASSIVE_CHECK, 		_pwmng_check_passive		},
		[POW_STATE_SLOTS_READ][SIGNAL_CRATECHANGE]		= {	POW_STATE_SOURCE_READ, 		        _pwmng_power_source_read	},
		[POW_STATE_SLOTS_READ][SIGNAL_PROFILECHANGE]		= {	POW_STATE_PASSIVE_CHECK, 		_pwmng_check_passive		},

		[POW_STATE_PASSIVE_CHECK][SIGNAL_STATIC]		= {	POW_STATE_FAN_ON,			_pwmng_fan_on			},
		[POW_STATE_PASSIVE_CHECK][SIGNAL_CRATECHANGE]		= {	POW_STATE_SOURCE_READ,			_pwmng_power_source_read	},
		[POW_STATE_PASSIVE_CHECK][SIGNAL_PROFILECHANGE]		= {	POW_STATE_FAN_ON,			_pwmng_fan_on			},

		[POW_STATE_FAN_ON][SIGNAL_STATIC]			= {	POW_STATE_POWEROFF_IGNORANT, 	        _pwmng_power_ignorant_stop	},
		[POW_STATE_FAN_ON][SIGNAL_CRATECHANGE]			= {	POW_STATE_SOURCE_READ, 	                _pwmng_power_source_read	},
		[POW_STATE_FAN_ON][SIGNAL_PROFILECHANGE]	        = {	POW_STATE_POWEROFF_IGNORANT, 	        _pwmng_power_ignorant_stop	},

/* profile change */
		[POW_STATE_POWEROFF_IGNORANT][SIGNAL_STATIC]	        = {	POW_STATE_POWER_RESRV_CALC, 	        _pwmng_power_reserve_calc	},
		[POW_STATE_POWEROFF_IGNORANT][SIGNAL_CRATECHANGE]	= {	POW_STATE_SOURCE_READ, 	                _pwmng_power_source_read	},
		[POW_STATE_POWEROFF_IGNORANT][SIGNAL_PROFILECHANGE]	= {	POW_STATE_POWER_RESRV_CALC, 	        _pwmng_power_reserve_calc	},

		[POW_STATE_POWER_RESRV_CALC][SIGNAL_STATIC]		= {	POW_STATE_PROHIBITED, 			_pwmng_power_calc_prohib	},
		[POW_STATE_POWER_RESRV_CALC][SIGNAL_CRATECHANGE]	= {	POW_STATE_SOURCE_READ, 			_pwmng_power_source_read	},
		[POW_STATE_POWER_RESRV_CALC][SIGNAL_PROFILECHANGE]	= {	POW_STATE_POWEROFF_IGNORANT,	        _pwmng_power_ignorant_stop	},

		[POW_STATE_PROHIBITED][SIGNAL_STATIC]			= {	POW_STATE_SMARTSTART, 			_pwmng_power_calc_smartly	},
		[POW_STATE_PROHIBITED][SIGNAL_CRATECHANGE]		= {	POW_STATE_SOURCE_READ, 			_pwmng_power_source_read	},
		[POW_STATE_PROHIBITED][SIGNAL_PROFILECHANGE]		= {	POW_STATE_POWEROFF_IGNORANT, 		_pwmng_power_ignorant_stop	},
		[POW_STATE_PROHIBITED][SIGNAL_POW_POSITIVE]		= {	POW_STATE_SMARTSTART, 			_pwmng_power_calc_smartly	},
		[POW_STATE_PROHIBITED][SIGNAL_POW_NEGATIVE]		= {	POW_STATE_READY, 			NULL			        },

		[POW_STATE_SMARTSTART][SIGNAL_STATIC]			= {	POW_STATE_READY, 			NULL			        },
		[POW_STATE_SMARTSTART][SIGNAL_CRATECHANGE]		= {	POW_STATE_SOURCE_READ, 			_pwmng_power_source_read        },
		[POW_STATE_SMARTSTART][SIGNAL_PROFILECHANGE]		= {	POW_STATE_POWEROFF_IGNORANT, 		_pwmng_power_ignorant_stop      },
		[POW_STATE_SMARTSTART][SIGNAL_POW_POSITIVE]		= {	POW_STATE_READY, 			NULL			        },
		[POW_STATE_SMARTSTART][SIGNAL_POW_NEGATIVE]		= {	POW_STATE_READY, 			NULL			        },

/* waiting changes */
		[POW_STATE_READY][SIGNAL_STATIC]			= {     POW_STATE_READY, 			 NULL			        },
		[POW_STATE_READY][SIGNAL_CRATECHANGE]			= {     POW_STATE_SOURCE_READ, 			_pwmng_power_source_read	},
		[POW_STATE_READY][SIGNAL_PROFILECHANGE]			= {     POW_STATE_POWEROFF_IGNORANT,	        _pwmng_power_ignorant_stop	},
		[POW_STATE_READY][SIGNAL_POW_POSITIVE]		        = {	POW_STATE_READY, 			NULL			        },
		[POW_STATE_READY][SIGNAL_POW_NEGATIVE]		        = {	POW_STATE_READY, 			NULL			        },
};

//_pwmng_
enum _POWER_STATES   		curnt_state  = POW_STATE_INIT;
static enum _POWER_SIGNALS	curnt_signal = SIGNAL_STATIC;
static enum _POWER_SIGNALS	inauto_signal = SIGNAL_STATIC;



/*------------------------------------------------------------------------------------------------------------------*/
struct _SLOTS_STATUS {
	uint32_t	health_status;
	uint8_t		now_present;
	uint8_t		now_on;
	float	        cons_power;
	_BOOL		passive_flag;
};

struct _POWERSOURCE_STATUS {
	uint8_t		health_status;
	uint8_t		now_present;
	uint8_t		now_on;
	float		max_power;
};

struct _FAN_STATUS {
	uint8_t		health_status;
	uint8_t		now_present;
	uint8_t		now_on;
	float		max_power;
};


static struct _CRATE_STATUS {
	struct _SLOTS_STATUS    		slot_status[SLOT_MAX_NUM];
	struct _POWERSOURCE_STATUS		pwrsrc_status[PWR_MAX_NUM];
	struct _FAN_STATUS			fan_status;
	float					powerLimitSource ;						/*!<   */
	float					recuredPower;
	float					actualAvailablePowerLimit;
} crate_status;

/*code==============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
void _set_current_signal(msg_type_t msg) 
{
  switch ( msg )
  {
      case PROF_POWER_RECALC:   curnt_signal = SIGNAL_PROFILECHANGE;    break;
      case SHELF_POWER_RECALC:  curnt_signal = SIGNAL_CRATECHANGE;      break;
  }
  
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
void _set_inauto_signal(msg_type_t msg) 
{
  switch ( msg )
  {
      case PROF_POWER_RECALC:   inauto_signal = SIGNAL_PROFILECHANGE;    break;
      case SHELF_POWER_RECALC:  inauto_signal = SIGNAL_CRATECHANGE;      break;
  }
  
}





/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
int pwmng_get_current_signal (void)
{
  msg_inttask_t			*dc_message;
    
  switch ( curnt_state )
  {
   case	POW_STATE_READY: {
          curnt_signal = SIGNAL_STATIC;
          inauto_signal = SIGNAL_STATIC;
          dc_message = recv_inttask_message(POWER_MANAGMENT, ANY_MODULE, NULL);
	  if ( dc_message == NULL ) {
	      break;
	  }
          _set_current_signal ( dc_message->msg );          
        } break;
        
   case POW_STATE_INIT: {
        curnt_signal = SIGNAL_STATIC;
        inauto_signal = SIGNAL_STATIC;
   } break;
   
  case POW_STATE_POWER_DELAY : {    
        if ( inauto_signal == SIGNAL_STATIC ) {
  	    dc_message = recv_inttask_message(POWER_MANAGMENT, ANY_MODULE, NULL);
	    if ( dc_message == NULL ) {
	        break;
	    }
            _set_current_signal ( dc_message->msg );
        } else {
            curnt_signal = inauto_signal;
            inauto_signal = SIGNAL_STATIC;
        }          
  } break;
    
  case    POW_STATE_PROHIBITED:  {
      if ( crate_status.actualAvailablePowerLimit > 0 ) {
    	  curnt_signal = SIGNAL_POW_POSITIVE;
      } else {
	  curnt_signal = SIGNAL_POW_NEGATIVE;
      }
  } break;
  
  case    POW_STATE_SMARTSTART:  {
      curnt_signal = SIGNAL_STATIC;
      inauto_signal = SIGNAL_STATIC;
  } break;
    
  default: {
      dc_message = tryrecv_inttask_message(POWER_MANAGMENT, ANY_MODULE, NULL);
      if ( dc_message == NULL ) {
        curnt_signal = inauto_signal;
        inauto_signal = SIGNAL_STATIC;
        break;
      }
      _set_current_signal ( dc_message->msg );
  } break;
  
  } /* switch ( curnt_state ) */
  
  return (int)curnt_signal;
}

/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static msg_unit_name_t		pwmng_get_slot_name
(
		int index
)
{
	msg_unit_name_t    	slot_name;

	switch (index) {
	case 0: 	slot_name = SLOT_1;	break;
	case 1: 	slot_name = SLOT_2;	break;
	case 2: 	slot_name = SLOT_3;	break;
	case 3: 	slot_name = SLOT_4;	break;
	case 4: 	slot_name = SLOT_5;	break;
	case 5: 	slot_name = SLOT_6;	break;
	case 6: 	slot_name = SLOT_7;	break;
	case 7: 	slot_name = SLOT_8;	break;
	case 8: 	slot_name = SLOT_9;	break;
	case 9: 	slot_name = SLOT_10;	break;
	case 10: 	slot_name = SLOT_11;	break;
	case 11: 	slot_name = SLOT_12;	break;
	case 12: 	slot_name = SLOT_13;	break;
	}

	return slot_name;
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static msg_unit_name_t	pwmng_get_pwrsrc_name
(
		int index
)
{
	msg_unit_name_t    	name;

	switch (index) {
	case 0: 	name = POWER1;		break;
	case 1: 	name = POWER2;		break;
	case 2: 	name = POWER3;		break;
	case 3: 	name = POWER4;		break;
	}
	return name;
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static enum _slot_enable get_slotconf(int index) {
	enum _slot_enable sltcnf;

	switch ( index )
	{
	case 0:		sltcnf = (enum _slot_enable)val_Slot1Enable;	break;
	case 1:		sltcnf = (enum _slot_enable)val_Slot2Enable;	break;
	case 2:		sltcnf = (enum _slot_enable)val_Slot3Enable;	break;
	case 3:		sltcnf = (enum _slot_enable)val_Slot4Enable;	break;
	case 4:		sltcnf = (enum _slot_enable)val_Slot5Enable;	break;
	case 5:		sltcnf = (enum _slot_enable)val_Slot6Enable;	break;
	case 6:		sltcnf = (enum _slot_enable)val_Slot7Enable;	break;
	case 7:		sltcnf = (enum _slot_enable)val_Slot8Enable;	break;
	case 8:		sltcnf = (enum _slot_enable)val_Slot9Enable;	break;
	case 9:	        sltcnf = (enum _slot_enable)val_Slot10Enable;	break;
	case 10:	sltcnf = (enum _slot_enable)val_Slot11Enable;	break;
	case 11:	sltcnf = (enum _slot_enable)val_Slot12Enable;	break;
	case 12:	sltcnf = (enum _slot_enable)val_Slot13Enable;	break;
	}

	return sltcnf;
}



/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
uint32_t pwmng_get_present (void)
{
        int       i;
        uint32_t  present_flags = 0;
        
        for (i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ) {
            present_flags |= ((uint32_t)Dnepr_DControl_SlotPresent()->bSlotPresent[i]) << i;
        }
	return present_flags;
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_get_slot_states	(int state, int signal)
{
	uint32_t 				i;
	static uint32_t			now_presents = 0;
	msg_inttask_t			pwr_msg;
	msg_inttask_t			*dc_message;
	msg_pwr_data_t			send_data;
	msg_dc_state_data_t		*recv_data;

	/* определяем в каких слотах присутствуют устройства */
	now_presents = pwmng_get_present();

	for (i = 0; i < SLOT_MAX_NUM; i++) {
		crate_status.slot_status[i].now_present = 0;
		crate_status.slot_status[i].now_on = SLOT_OFF;
		crate_status.slot_status[i].cons_power = 0.0;

		if ( 0 != (now_presents & (1 << i)) ) {
			crate_status.slot_status[i].now_present = 1;
			send_data.dev_name = pwmng_get_slot_name(i);
			pwr_msg.msg = GET_SHELF_STATE;
			pwr_msg.data = &send_data;

			/* определяем включено ли устройство */
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT, &pwr_msg);
			dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
			if ( dc_message == NULL ) {
				crate_status.slot_status[i].cons_power = 0.0;
				continue;
			}
                        if ( dc_message->msg == GET_SHELF_STATE ) {
  			  recv_data = dc_message->data;
			  crate_status.slot_status[i].now_present = recv_data->present;
			  crate_status.slot_status[i].now_on = (recv_data->power_state == SLOT_ON) ? SLOT_ON : SLOT_OFF;
      			  crate_status.slot_status[i].health_status = ((recv_data->hotswap_status == CHIP_OK) && (recv_data->eeprom_status == CHIP_OK)) ? OK : ERROR;
                          crate_status.slot_status[i].passive_flag = FALSE;
                        } else {
                          i = 0;
                          _set_inauto_signal(dc_message->msg);                          
                        }
		}
	}
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_delay	(int state, int signal)
{
	uint32_t	i;
	msg_inttask_t			pwr_msg;
	msg_inttask_t			*dc_message;
	msg_pwr_data_t			send_data;
	msg_dc_state_data_t		*recv_data;

	for (i = 0; i < SLOT_MAX_NUM; i++) {
		if ( crate_status.slot_status[i].now_present && crate_status.slot_status[i].now_on == SLOT_OFF ) {
			send_data.dev_name = pwmng_get_slot_name(i);
			send_data.dev_state = SLOT_WAITING;
			pwr_msg.msg = SET_SHELF_STATE;
			pwr_msg.data = &send_data;
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
			dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
			if ( dc_message == NULL ) {
				crate_status.slot_status[i].cons_power = 0.0;
				continue;
			}
                        if ( dc_message->msg == SET_SHELF_STATE ) {                        
  			  recv_data = dc_message->data;
			  crate_status.slot_status[i].now_on = (recv_data->power_state == SLOT_OFF) ? SLOT_OFF : SLOT_ON;
                        } else {
                          _set_inauto_signal(dc_message->msg);
                          return;
                        }
		} /* if ( crate_status.slot_status[i].now_on ) */
	} /* for (i = 0; i < SLOT_MAX_NUM; i++) */
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_ignorant_stop	(int state, int signal)
{
	uint32_t	i;
	msg_inttask_t			pwr_msg;
	msg_inttask_t			*dc_message;
	msg_pwr_data_t			send_data;
	msg_dc_state_data_t		*recv_data;

	for (i = 0; i < SLOT_MAX_NUM; i++) {
		if ( crate_status.slot_status[i].now_on == SLOT_ON && get_slotconf(i) == Prohibited ) {
			send_data.dev_name = pwmng_get_slot_name(i);
			send_data.dev_state = SLOT_OFF;
			pwr_msg.msg = SET_SHELF_STATE;
			pwr_msg.data = &send_data;
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
			dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
			if ( dc_message == NULL ) {
				crate_status.slot_status[i].cons_power = 0.0;
				continue;
			}
                        if ( dc_message->msg == SET_SHELF_STATE ) {                                                
  			  recv_data = dc_message->data;
  			  crate_status.slot_status[i].now_on = (recv_data->power_state == SLOT_OFF) ? SLOT_OFF : SLOT_ON;
                        } else {
                          _set_inauto_signal(dc_message->msg);
                          return;
                        }
		} /* if ( crate_status.slot_status[i].now_on && get_slotconf(i) == IgnorantStart ) */
	} /* for (i = 0; i < SLOT_MAX_NUM; i++) */

}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_source_read	(int state, int signal)
{
	uint32_t				i;
	msg_inttask_t			pwr_msg;
	msg_inttask_t			*dc_message;
	msg_pwr_data_t			send_data;
	msg_pwr_data_t			alrm_data;
	msg_dc_state_data_t		*recv_data;
	msg_dc_power_data_t		*recv2_data;

	for ( i = 0; i < PWR_MAX_NUM; i++ ) {
		send_data.dev_name = pwmng_get_pwrsrc_name(i);
		pwr_msg.msg = GET_SHELF_STATE;
		pwr_msg.data = &send_data;
		send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
		dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);

		if ( dc_message == NULL ) {
			alrm_data.dev_name = send_data.dev_name;
			alrm_data.failure_code = FAIL_POWER_DONT_READING;
			pwr_msg.msg = FAILURE_SIGNALING;
			pwr_msg.data = &alrm_data;
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
			crate_status.pwrsrc_status[i].max_power 	=  0;

			continue;
		}

                if ( dc_message->msg == GET_SHELF_STATE ) {
          		recv_data = dc_message->data;
		        crate_status.pwrsrc_status[i].now_on    	=  (recv_data->power_state == SLOT_OFF) ? SLOT_OFF : SLOT_ON;
          		crate_status.pwrsrc_status[i].now_present 	=  recv_data->present;
                } else {
                    _set_inauto_signal(dc_message->msg);
                    return;                  
                }

		if ( !(crate_status.pwrsrc_status[i].now_on == SLOT_ON && crate_status.pwrsrc_status[i].now_present) ) {
			crate_status.pwrsrc_status[i].max_power =  0.0;
			continue;
		}

		send_data.dev_name = pwmng_get_pwrsrc_name(i);
		pwr_msg.msg = GET_SHELF_POWER;
		pwr_msg.data = &send_data;
		send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
		dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
		if ( dc_message == NULL ) {
			alrm_data.dev_name = send_data.dev_name;
			alrm_data.failure_code = FAIL_POWER_DONT_READING;
			pwr_msg.msg = FAILURE_SIGNALING;
			pwr_msg.data = &alrm_data;
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
			crate_status.pwrsrc_status[i].max_power 	=  0.0;

			continue;
		}

                if ( dc_message->msg == GET_SHELF_POWER ) {
		    recv2_data = dc_message->data;
		    crate_status.pwrsrc_status[i].max_power 	=  recv2_data->max_power;
                } else {
                    _set_inauto_signal(dc_message->msg);
                    return;                  
                }
	}
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_reserve_calc	(int state, int signal)
{
	uint32_t		i;
	uint32_t		maximum = 0;

	crate_status.powerLimitSource  = 0;

	switch( val_VPowerLimitSource )
	{
	case PSU: {
				/* ищем максимум */
				for ( i = 0; i < PWR_MAX_NUM; i++ ) {
					if (   crate_status.pwrsrc_status[i].now_present
					       && crate_status.pwrsrc_status[i].now_on == SLOT_ON ) {
						maximum = (crate_status.pwrsrc_status[i].max_power > maximum) ? crate_status.pwrsrc_status[i].max_power : maximum;
					}
				}
				crate_status.powerLimitSource  = maximum;

				/* ищем минимум  */
				for ( i = 0; i < PWR_MAX_NUM; i++ ) {
					if (   crate_status.pwrsrc_status[i].now_present
							&& crate_status.pwrsrc_status[i].now_on == SLOT_ON
							&& crate_status.pwrsrc_status[i].max_power != 0 ) {
						crate_status.powerLimitSource  = (crate_status.pwrsrc_status[i].max_power < crate_status.powerLimitSource )
								? crate_status.pwrsrc_status[i].max_power : crate_status.powerLimitSource ;
					}
				}

				if ( crate_status.powerLimitSource  == 0 ) {
					crate_status.powerLimitSource  = val_VPowerMinReserve;
				}
			}
			break;


	case ProfileValue :
			crate_status.powerLimitSource  = val_VPowerLimit;
			break;
	}

}



/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_fan_read	(int state, int signal)
{
	msg_inttask_t			pwr_msg;
	msg_inttask_t			*dc_message;
	msg_pwr_data_t			send_data;
	msg_pwr_data_t			alrm_data;
	msg_dc_state_data_t		*recv_data;
	msg_dc_power_data_t		*recv2_data;

	send_data.dev_name = FAN;
	pwr_msg.msg = GET_SHELF_STATE;
	pwr_msg.data = &send_data;
	send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
	dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);

	if ( dc_message == NULL ) {
		alrm_data.dev_name = send_data.dev_name;
		alrm_data.failure_code = FAIL_POWER_DONT_READING;
		pwr_msg.msg = FAILURE_SIGNALING;
		pwr_msg.data = &alrm_data;
		send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
		crate_status.fan_status.max_power 	=  FAN_MAX_POWER;

		return;
	}

        if ( dc_message->msg == GET_SHELF_STATE ) {
          recv_data = dc_message->data;
	  crate_status.fan_status.now_on        =  (recv_data->power_state == SLOT_OFF) ? SLOT_OFF : SLOT_ON;
	  crate_status.fan_status.now_present 	=  recv_data->present;
        } else {
          _set_current_signal(dc_message->msg);
          return;                  
        }

	if ( !(crate_status.fan_status.now_on == SLOT_ON && crate_status.fan_status.now_present) ) {
		crate_status.fan_status.max_power =  0;
		/* тут возможно авария - отсуствие платы вентиляции */
		return;
	}

	send_data.dev_name = FAN;
	pwr_msg.msg = GET_SHELF_POWER;
	pwr_msg.data = &send_data;
	send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
	dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);

	if ( dc_message == NULL ) {
		alrm_data.dev_name = send_data.dev_name;
		alrm_data.failure_code = FAIL_POWER_DONT_READING;
		pwr_msg.msg = FAILURE_SIGNALING;
		pwr_msg.data = &alrm_data;
		send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
		crate_status.fan_status.max_power 	=  FAN_MAX_POWER;

		return;
	}

        if ( dc_message->msg == GET_SHELF_POWER ) {
	  recv2_data = dc_message->data;
	  crate_status.fan_status.max_power 	=  recv2_data->max_power;
        } else {
          _set_inauto_signal(dc_message->msg);
          return;                  
        }
}




/*=============================================================================================================*/
/*!  \brief Считать последовательно мощность с каждого слотового устройства.
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_slots_read	(int state, int signal)
{
	uint32_t				i;
	msg_inttask_t			pwr_msg;
	msg_inttask_t			*dc_message;
	msg_pwr_data_t			send_data;
	msg_dc_power_data_t		*recv2_data;
	msg_dc_state_data_t		*recv_data;

	for ( i = 0; i < SLOT_MAX_NUM; i++ ) {
		if ( !(crate_status.slot_status[i].now_present) ) {
			crate_status.slot_status[i].cons_power =  0.0;
			continue;
		}

		send_data.dev_name = pwmng_get_slot_name(i);
		pwr_msg.msg = GET_SHELF_POWER;
		pwr_msg.data = &send_data;
		send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
		dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
		if ( dc_message == NULL ) {
		    crate_status.slot_status[i].cons_power = 0.0;
		    continue;
		}

                if ( dc_message->msg == GET_SHELF_POWER ) {                
  		    recv2_data = dc_message->data;
		    crate_status.slot_status[i].cons_power = recv2_data->max_power;
                } else {
                    _set_inauto_signal(dc_message->msg);
                    return;                   
                }
                
                if ( crate_status.slot_status[i].health_status == ERROR ) {
			send_data.dev_name = pwmng_get_slot_name(i);
			pwr_msg.msg = GET_SHELF_STATE;
			pwr_msg.data = &send_data;

			/* определяем включено ли устройство */
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT, &pwr_msg);
			dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
			if ( dc_message == NULL ) {
				crate_status.slot_status[i].cons_power = 0.0;
				continue;
			}
                        if ( dc_message->msg == GET_SHELF_STATE ) {
  			  recv_data = dc_message->data;
			  crate_status.slot_status[i].now_present = recv_data->present;
			  crate_status.slot_status[i].now_on = (recv_data->power_state == SLOT_ON) ? SLOT_ON : SLOT_OFF;
      			  crate_status.slot_status[i].health_status = ((recv_data->hotswap_status == CHIP_OK) && (recv_data->eeprom_status == CHIP_OK)) ? OK : ERROR;
                          crate_status.slot_status[i].passive_flag = FALSE;
                        } else {
                          _set_inauto_signal(dc_message->msg);
                          return;                          
                        }
                }
	}
}


/*=============================================================================================================*/
/*!  \brief Определяем пассивные слотовые устройства.
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_check_passive	(int state, int signal)
{
	uint32_t				i;
	msg_inttask_t			pwr_msg;
	msg_inttask_t			*dc_message;
	msg_pwr_data_t			send_data;
	msg_pwr_data_t			alrm_data;
	msg_dc_state_data_t		*recv_data;


	for ( i = 0; i < SLOT_MAX_NUM; i++ ) {
		if ( !(crate_status.slot_status[i].now_on == SLOT_ON && crate_status.slot_status[i].now_present) ) {
			continue;
		}

		if ( crate_status.slot_status[i].cons_power == 0 ) {
			send_data.dev_name = pwmng_get_slot_name(i);
			pwr_msg.msg = GET_SHELF_STATE;
			pwr_msg.data = &send_data;
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
			dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);

			if ( dc_message == NULL ) {
				/* сбой управления */
				alrm_data.dev_name = send_data.dev_name;
				alrm_data.failure_code = FAIL_POWER_DONT_READING;
				pwr_msg.msg = FAILURE_SIGNALING;
				pwr_msg.data = &alrm_data;
				send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
				crate_status.slot_status[i].cons_power = SLOT_MAX_POWER;
				crate_status.slot_status[i].passive_flag = FALSE;
				crate_status.slot_status[i].health_status = ERROR;
				continue;
			}

                        if ( dc_message->msg == GET_SHELF_STATE ) {                
			  recv_data = dc_message->data;
			  crate_status.slot_status[i].passive_flag = TRUE;
			  crate_status.slot_status[i].health_status = OK;
                        } else {
                          _set_inauto_signal(dc_message->msg);
                          return;                          
                        }

			if ( (recv_data->hotswap_status == CHIP_OK ) && (recv_data->eeprom_status == CHIP_FAIL ) ) {
				/* сбой управления */
				alrm_data.dev_name = send_data.dev_name;
				alrm_data.failure_code = FAIL_POWER_DONT_READING;
				pwr_msg.msg = FAILURE_SIGNALING;
				pwr_msg.data = &alrm_data;
				send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
				crate_status.slot_status[i].cons_power = SLOT_MAX_POWER;
				crate_status.slot_status[i].passive_flag = FALSE;
				crate_status.slot_status[i].health_status = ERROR;
			}
		} /* if ( crate_status.pwrsrc_status[i].max_power == 0 ) */
	} /* for ( i = 0; i < SLOT_MAX_NUM; i++ ) */
}



/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_fan_on	(int state, int signal)
{
	;
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_calc_prohib	(int state, int signal)
{
	int						i;
	float			        power_slots_now = 0;
	msg_inttask_t			pwr_msg;
	msg_pwr_data_t			send_data;
	msg_inttask_t			*dc_message;
	msg_dc_state_data_t		*recv_data;
	_BOOL				is_prohibited_only = FALSE;

	crate_status.recuredPower = 0;
	power_slots_now = 0;
        is_prohibited_only = FALSE;

	        for (i = 0; i < SLOT_MAX_NUM; i++) {
		      if ( !crate_status.slot_status[i].now_present ) {
			  continue;
		      }

		      if ( crate_status.slot_status[i].now_on == SLOT_ON ) {
			    power_slots_now += crate_status.slot_status[i].cons_power;
		      } else {
			    if ( get_slotconf(i) == IgnorantStart) {
				crate_status.recuredPower += crate_status.slot_status[i].cons_power;
			    }
		      } /* if ( crate_status.pwrsrc_status[i].now_on ) */
	        } /* for (i = 0; i < SLOT_MAX_NUM; i++) */

	do {
        
		crate_status.actualAvailablePowerLimit = crate_status.powerLimitSource - crate_status.fan_status.max_power -
												POWER_CU - (float)val_VPowerMinReserve - power_slots_now;
	
		if ( crate_status.actualAvailablePowerLimit >= crate_status.recuredPower || is_prohibited_only == TRUE ) {
		      for (i = 0; i < SLOT_MAX_NUM; i++) {
			    if ( !crate_status.slot_status[i].now_present ) {
			      continue;
			      }
			  if (   (crate_status.slot_status[i].passive_flag == FALSE)
				  && (crate_status.slot_status[i].health_status == OK)
				  && (get_slotconf(i) == IgnorantStart)
				  && (crate_status.slot_status[i].now_on == SLOT_OFF)
			      ) {
				  break;
			      }
		      }

    		        /* нет невключеннных активных устройств c уверенным пуском */
		        if ( i == SLOT_MAX_NUM ) {
			  break;
		        } 
                  
						send_data.dev_name = pwmng_get_slot_name(i);
						send_data.dev_state = SLOT_ON;
						pwr_msg.msg = SET_SHELF_STATE;
						pwr_msg.data = &send_data;
						send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
			                        dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
			                        if ( dc_message == NULL ) {
				                     crate_status.slot_status[i].health_status = ERROR;
				                      continue;
			                        }
                                                if ( dc_message->msg == SET_SHELF_STATE ) {                                                
  			                            recv_data = dc_message->data;
  			                            crate_status.slot_status[i].now_on = (recv_data->power_state == SLOT_OFF) ? SLOT_OFF : SLOT_ON;
      						    if ( crate_status.slot_status[i].now_on == SLOT_ON ) {
                                                        power_slots_now += crate_status.slot_status[i].cons_power;
                                                    }
                                                } else {
                                                    _set_inauto_signal(dc_message->msg);
                                                    return;
                                                }
		} /* if ( crate_status.actualAvailablePowerLimit >= crate_status.recuredPower ) */
		else /* crate_status.actualAvailablePowerLimit < crate_status.recuredPower */
		{
			/* проверяем наличие включенных устройств c аккуратным пуском */
			for (i = SLOT_MAX_NUM - 1 ; i >= 0; i--) {
				if ( !crate_status.slot_status[i].now_present ) {
					continue;
				}

				if ( (crate_status.slot_status[i].passive_flag == FALSE)
				  && (crate_status.slot_status[i].health_status == OK)
				  && (get_slotconf(i) == SmartlyStart)
				  && (crate_status.slot_status[i].now_on == SLOT_ON)  
                                      ) {
					send_data.dev_name = pwmng_get_slot_name(i);
					send_data.dev_state = SLOT_OFF;
					pwr_msg.msg = SET_SHELF_STATE;
					pwr_msg.data = &send_data;
					send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
	                                dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);                                        
		                        if ( dc_message == NULL ) {
			                     crate_status.slot_status[i].health_status = ERROR;
			                      continue;
	                                }
                                        if ( dc_message->msg == SET_SHELF_STATE ) {                                                
  			                    recv_data = dc_message->data;
  			                    crate_status.slot_status[i].now_on = (recv_data->power_state == SLOT_OFF) ? SLOT_OFF : SLOT_ON;
				            if ( crate_status.slot_status[i].now_on == SLOT_OFF ) {
                                                power_slots_now -= crate_status.slot_status[i].cons_power;
                                            }
                                            break;
                                       } else {
                                            _set_inauto_signal(dc_message->msg);
                                            return;
                                       }
				}
			} /* for (i = SLOT_MAX_NUM - 1 ; i <= 0; i--) */
			if (i == -1) {
				is_prohibited_only = TRUE;
			}
                        
		}/* crate_status.actualAvailablePowerLimit < crate_status.recuredPower */
	} while (TRUE);

}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
static void _pwmng_power_calc_smartly	(int state, int signal)
{
	int						i;
	msg_inttask_t			pwr_msg;
	msg_pwr_data_t			send_data;
	msg_inttask_t			*dc_message;
	msg_dc_state_data_t		*recv_data;

	crate_status.recuredPower = 0;

	for (i = 0; i < SLOT_MAX_NUM; i++) {

		if ( crate_status.actualAvailablePowerLimit <= 0 ) {
			return;
		}

		if ( !crate_status.slot_status[i].now_present ) {
			continue;
		}

		if (   (crate_status.slot_status[i].passive_flag == TRUE)
			|| (crate_status.slot_status[i].health_status != OK)
			|| (get_slotconf(i) != SmartlyStart)
			|| (crate_status.slot_status[i].now_on == SLOT_ON)
		   ) {
			continue;
		}
		crate_status.recuredPower = crate_status.slot_status[i].cons_power;

		if ( crate_status.recuredPower > crate_status.actualAvailablePowerLimit ) {
			continue;
		} else { /* crate_status.recuredPower <= crate_status.actualAvailablePowerLimit */
			/* включаем устройтсво */
			send_data.dev_name = pwmng_get_slot_name(i);
			send_data.dev_state = SLOT_ON;
			pwr_msg.msg = SET_SHELF_STATE;
			pwr_msg.data = &send_data;
			send_inttask_message(DEVICE_CONTROLLER, POWER_MANAGMENT,  &pwr_msg);
			dc_message = recv_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, NULL);
			if ( dc_message == NULL ) {
			    crate_status.slot_status[i].health_status = ERROR;
		            continue;
			}
                        if ( dc_message->msg == SET_SHELF_STATE ) {                                                
  			    recv_data = dc_message->data;
  			    crate_status.slot_status[i].now_on = (recv_data->power_state == SLOT_OFF) ? SLOT_OFF : SLOT_ON;
                            if ( crate_status.slot_status[i].now_on == SLOT_ON ) {
			        crate_status.actualAvailablePowerLimit -= crate_status.recuredPower;
                            }
                        } else {
                           _set_inauto_signal(dc_message->msg);
                           return;
                        }
		}/* crate_status.recuredPower <= crate_status.actualAvailablePowerLimit */
	} /* for (i = 0; i < SLOT_MAX_NUM; i++) */
}


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
int   pwmng_is_slot_ready(int  slot_index)
{
    if ( crate_status.slot_status[slot_index].now_present )
    {
      if ( crate_status.slot_status[slot_index].passive_flag == TRUE ) {
          return 1;
      }
      
      if ( crate_status.slot_status[slot_index].now_on == SLOT_ON && crate_status.slot_status[slot_index].health_status == OK ) {
          return 1;
      }
    }
   
    return 0;
}       


/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
int   pwmng_is_slot_passive(int  slot_index)
{
    if ( crate_status.slot_status[slot_index].now_present )
    {
      if ( crate_status.slot_status[slot_index].passive_flag == TRUE ) {
          return 1;
      }
    }
   
    return 0;
}       

/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
int   pwmng_get_power_status(int  slot_index)
{
/*    
"Недоступно" -  при отсутствии вставленного устройства или при пассивном вставленном устройстве;
"Сбой управления" - при аппаратном сбое управления устройством;
"Работает" - при нормальной работе во включенном состоянии;

"ПределМощности" -  при выключенном состоянии из-за отсутствия запаса мощности;

"Выключено" - При выключенном состоянии и-за запрета запуска пользователем.

Недоступно@0|СбойУправления@1|Работает@2|ПределМощности@3|Выключено@4  
*/
    if ( crate_status.slot_status[slot_index].now_present || crate_status.slot_status[slot_index].passive_flag )
    {      
        if ( crate_status.slot_status[slot_index].health_status != OK ) {
          return 1;
        }
        
        if ( crate_status.slot_status[slot_index].now_on == SLOT_ON ) {
          return 2;
        } else {
          if ( get_slotconf(slot_index) == SmartlyStart ) {
            return 3;
          } else {            
            return 4;
          }
        }
    }
      
    return 0;      
}

/*=============================================================================================================*/
/*!  \brief
     \return
     \retval
     \sa
*/
/*=============================================================================================================*/
float   pwmng_get_power_limit(void)
{
    return crate_status.actualAvailablePowerLimit;
}

