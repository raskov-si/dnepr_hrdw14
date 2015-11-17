/*
 * inttask_messages.h
 *
 *  Created on: 23 окт. 2015 г.
 *      Author: kiryanov
 */

#ifndef TEST_INTTSK_MGS_H_
#define TEST_INTTSK_MGS_H_

#include <stdint.h>


#define ANY_MODULE		        (0)
#define DEVICE_CONTROLLER		(1)
#define POWER_MANAGMENT			(2)


#define SIZEOF_INTTASK                  (8)



typedef enum MSG_TYPE {
	GET_SHELF_STATE,
	SET_SHELF_STATE,
	GET_SHELF_POWER,
	FAILURE_SIGNALING,
	PMBUS_ALARM,				//!< alarm на pmbus
	PRESENT_INTERRUPT,			//!< прерывание от плис
	SFP_INTERRUPT, 				//!< прерывание от SFP
	PSU_VALUES,					//!< измерили мощности блоков питания
//	INIT_HOTSWAPS,				//!< включить все устройства, которые можно
//	REINIT_ONOFF,				//!< включить слотовые устройства, которым можно включиться
//     	POWER_RECALC,				//!< изменились параметры шасси
	FAN_SETTINGS_CHANGED,		        //!< Изменили параметры автоматического управления вентиляторами.  
        PROF_POWER_RECALC,
        SHELF_POWER_RECALC,
} msg_type_t;


typedef struct MSG_STRUCT{
	msg_type_t			msg;
	void				*data;
} msg_inttask_t;


typedef enum MSG_UNIT_NAME {
	SLOT_1,
	SLOT_2,
	SLOT_3,
	SLOT_4,
	SLOT_5,
	SLOT_6,
	SLOT_7,
	SLOT_8,
	SLOT_9,
	SLOT_10,
	SLOT_11,
	SLOT_12,
	SLOT_13,
	POWER1,
	POWER2,
	POWER3,
	POWER4,
	FAN
} msg_unit_name_t;



typedef enum MSG_UNIT_STATE {
        UNKNOWN = 0,
	SLOT_ON,
	SLOT_OFF,
	SLOT_WAITING,
} msg_unit_state_t ;



typedef enum MSG_FAILURE_CODE {
        FAIL_NO                     = 0,
	FAIL_POWER_DONT_READING
}msg_failure_code_t;



typedef struct  MSG_PWR_DATA {
	msg_unit_name_t		dev_name;
	msg_unit_state_t	dev_state;
	msg_failure_code_t	failure_code;
} msg_pwr_data_t;



typedef enum MSG_CHIP_STATE {
	CHIP_OK,
	CHIP_FAIL,
	CHIP_SLEEP
} msg_chip_state_t ;

typedef struct  MSG_DC_STATE_DATA {
	msg_unit_state_t	power_state;
	uint8_t			present;
	msg_chip_state_t	hotswap_status;
	msg_chip_state_t	eeprom_status;
} msg_dc_state_data_t;


typedef struct  MSG_DC_POWER_DATA {
	float	       max_power;
} msg_dc_power_data_t;


typedef struct  MSG_PWR_EVENTS_DATA {
     msg_unit_name_t   unit_source_of_event;     
} msg_pwr_events_data_t;


int                     open_inttask_message (void);
int                     close_inttask_message (void);
int 			send_inttask_message (int, int, msg_inttask_t*);
msg_inttask_t* 	        recv_inttask_message (int, int, void*);
msg_inttask_t* 	        tryrecv_inttask_message (int, int, void*);

#endif /* TEST_INTTSK_MGS_H_ */
