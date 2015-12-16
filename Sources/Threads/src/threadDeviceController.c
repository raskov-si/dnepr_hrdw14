/*!
\file threadDeviceController.c
\brief ����� ��������� ���������� ���������, ������������ ���������� ���������� � present'�� � ��������� �� ALARM PMBus'�
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/

#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_EdgePort.h"
#include "HAL/IC/inc/AT_AT24C512.h"
#include "T8_Atomiccode.h"
#include "Threads/inc/inttsk_mgs.h"
#include "Threads/inc/threadDeviceController.h"
#include "Threads/inc/threadMeasure.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "Application/inc/power_managment.h"
#include "common_lib/crc.h"
#include <string.h>
#include <stdio.h>
#include <float.h>
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"
#include "T8_atomic_heap.h"

#include "support_common.h"

extern u32 val_VPowerLimit ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ������ hotswap'�� ������ +1 ����� ���������� + ���������� ������������
static const u8 __hs_slot_addresses[ I2C_DNEPR_NUMBER_OF_SLOTS + 1 ] = 
{ 0x9C, 0xCA, 0xBC, 0xDC, 0x8E, 0xBA, 0xAE, 0xCE, 0x9E, 0xDA, 0xBE, 0xDE, 0xE8, 0x8C, 0xA4 };
#define FAN_CONTROLLER_ADDR_NUM	(I2C_DNEPR_NUMBER_OF_SLOTS) // ������� ������ ���������� ������������
#define FAN_HS_ADDR_NUM			(I2C_DNEPR_NUMBER_OF_SLOTS-1) // ������� ������ �������� ����� ����������

//! ��� �������� ���������� ����� ������� ����� �������� (llUptime), ����� ������� 
// ������� ����� ��� ��� ������ �������������� ������ hotswapa
static long long __slot_plugin_time[ I2C_DNEPR_NUMBER_OF_SLOTS ];
#define PMBUS_ALARM_TIMEOUT		2500 //!< ������� �� ������������� �� PMBUS_alarm
#define SLOT_PLUGIN_TIMEOUT		7000 //!< ������� ��������� � ����� ��, ������ ���� ������ PMBUS_ALARM_TIMEOUT

static long long __last_pmbus_alarm = 0 ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ���������� ������ � ��������� �������� ���������

// ��������� ��������� ���������� � ������������ � ���������� ���������
static SlotPowerState_t __aPwrStates[ I2C_DNEPR_NUMBER_OF_SLOTS ];
static _BOOL _hs_available[I2C_DNEPR_NUMBER_OF_SLOTS];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! ��� ��������� ���������� � taskDeviceController
typedef enum mess_type_ {
        NOMSGS,
//	PMBUS_ALARM,				//!< alarm �� pmbus
//	
//
//	PRESENT_INTERRUPT,			//!< ���������� �� ����
//	SFP_INTERRUPT, 				//!< ���������� �� SFP
//
//	PSU_VALUES,					//!< �������� �������� ������ �������
//	INIT_HOTSWAPS,				//!< �������� ��� ����������, ������� �����
//	REINIT_ONOFF,				//!< �������� �������� ����������, ������� ����� ����������
////	POWER_ALLOW,				//!< ���� �������� ��� ��������� ����������
//	FAN_SETTINGS_CHANGED		//!< �������� ��������� ��������������� ���������� �������������.
} mess_type_t ;


//! ��� ��������� ��� ������� ������
typedef struct task_message_ {
	mess_type_t message_type ; //! ��� ������

	u32	slot_num_2_write ; //! � ����� ���� ����� ��������� eeprom
} task_message_t ;

#define task_messages_len 24 	// ����� ������� ��������� ������, ������ ��������

//! �������� ���������, ������� ������ ����������.
static I2C_DNEPR_PresentDevicesTypedef __processed_slots_present ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//static OS_EVENT  *__qRcvQueue = 0;

static f32 __fPowerLimit1, __fPowerLimit2 ;	// �������� ������ �������
static f32 __fPowerLimit ; // ������� ����� �������� ����� ���������
static f32 __fSlotPower[ I2C_DNEPR_NUMBER_OF_SLOTS ]; //!< ������� �������� ���������� ������ ����
extern u32 val_VPowerLimitSource;
static _BOOL __bPSULimitSource = TRUE ; //!< TRUE -- ���� ������ �������� �� ��


static void __pmbus_alarm_handler() ;
static void __slot_power_dev_plug( const u8 slot_num, const _BOOL pluginout );
void __slot_power_onoff_init();
//static void __slot_power_onoff_init();
//static void __slot_power_onoff();

static f32 __curPowerLimit();

//! �������� ��������� SlotOnOff
typedef enum {
	PERMITTED_START = 0,	//!< ������ ����� ��������
	SMARTLY_START = 1,		//!< ������ ���������� � ������� ������ (�.�. EEPROM
							//! � ������� ������ ���� �������� ��������, ������� 
							//! ������ �������)
	IGNORANT_START = 2 		//!< ������ ��������� � ����� ������
} SLOT_ONOFF_TYPE ;
static SLOT_ONOFF_TYPE __slot_OnOff( const u32 slot_num );

static const u32 __present_react_timeout = 100 ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ���� �������������� ������� �� PMBus
typedef enum {
	NONE_ALARM			= 0,
	CH1_FETSHORT		= 0x0001,
	CH1_POWERBAD		= 0x0002,
	CH1_OVERCURRENT		= 0x0004,
	CH1_UNDERVOLTAGE	= 0x0008,
	CH1_OVERVOLTAGE		= 0x0010,
	CH2_FETSHORT		= 0x0020,
	CH2_POWERBAD		= 0x0040,
	CH2_OVERCURRENT		= 0x0080,
	CH2_UNDERVOLTAGE	= 0x0100,
	CH2_OVERVOLTAGE		= 0x0200,

	// ������ ����������� ����������
	FAN_COMM_FAULT		= 0x0400,
	FAN_DATA_FAULT		= 0x0800,
	FAN_LOG_FULL_FAULT	= 0x1000,
	FAN_WATCHDOG		= 0x4000,
	FAN_OT_WARN			= 0x8000,
	FAN_OT_FAULT		= 0x00010000,
	FAN_VOUT_OV			= 0x00020000,
	FAN_TEMP			= 0x00040000,
	FAN_CML				= 0x00080000,
	FAN_VOUT			= 0x00100000,
	FAN_FANS			= 0x00200000
}  AlarmType_t ;

//! ��� ��������� � ��������� ������
typedef struct {
	u8 slot_num ; // ����� �����, ������� � ����, 13 -- ����� ����������
	long long cur_tick ;	// ������� ��� ��
	AlarmType_t alarm ; // ��� ������
} PMBAlarmMess_t ;

#define __pmbalarms_arr_len 32  //!< ����� ������� � ��������
static void* __pmbalarms_arr[ __pmbalarms_arr_len ] ; //!< ������� � ��������
//! ������� � ���������� �������, ���������� �� PMBus
static OS_EVENT  *__qPMBAlarms = 0;

static void __set_fans() ;

static void _send_shelf_state         ( uint8_t *data );
static void _set_state_and_send_state ( uint8_t *data );
static void _send_shelf_power         ( uint8_t *data );
static void _slot_power_on            ( uint8_t dev_index );
static void _slot_power_off           ( uint8_t dev_index );

// ������� Alert ����� ��� ���� ������� ���� ���������
static LT_LTC4222_FaultAlertRegisterTypedef	hs_alert_reg = {
											TRUE,	// FET short condition
											FALSE,	// EN# changes state
											TRUE,	// output power is bad
											TRUE,	// overcurrent condition
											TRUE,	// undervoltage condition
											TRUE	// overvoltage condition
											};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DeviceController_Init(void)
{
	u32 i ;
	// ���� �� ������� �������� ����������� �������� ������������ ��������
	__bPSULimitSource = (val_VPowerLimitSource == 0) ; // TRUE -- PSU, ����� �������� �� �������

	I2C_DNEPR_ClearPresentDevices( &__processed_slots_present );
	// �������� � ������ �����
	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
		__fSlotPower[ i ] = 0. ;
		__slot_plugin_time[ i ] = llUptime ; // �� ��������� �������� ����� ��������� ������
 	}

 	Dnepr_SlotEEPROM_Init() ;

 	// ������ ��� EEPROM � HotSwap Controller'� � ����������� ��������� ���, �������
 	// ��� �� ����������
 	//__slot_power_onoff_init() ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//void taskDeviceController(void *pdata)
//{
//	INT8U return_code = OS_ERR_NONE;
//	u32 i ;
//	I2C_DNEPR_PresentDevicesTypedef		prev_presents ;
//	T8_Dnepr_PsStatusTypedef cur_psu_status ;
//	T8_Dnepr_PsStatusTypedef prev_psu_status ;
//	void *messages_array[task_messages_len] ; // ��� ������� ���������
//	task_message_t *qCurMessage ;
//	const PSU_UnitInfoTypedef* pPSUInfo1 = NULL ;
//	const PSU_UnitInfoTypedef* pPSUInfo2 = NULL ;
//	
//	pdata = pdata;
//	// ������� ��������� ��� ������
//	__qRcvQueue = OSQCreate( messages_array, task_messages_len ) ;
//	
//	// ������� �������� �������
//	__qPMBAlarms = OSQCreate( __pmbalarms_arr, __pmbalarms_arr_len ) ;
//
//	Dnepr_Refresh_Presents() ; // ������������ �������� � 
//	
//        // ���������� ������ ����
//	for( i = 0; i < 3; i++ ){
//		if( Dnepr_Reload_PSU_Status_Pins() ){
//			break ;
//		}
//	}
//	memcpy( &cur_psu_status, Dnepr_Backplane_GetPSU_Status(), sizeof(T8_Dnepr_PsStatusTypedef) );
//	memcpy( &prev_psu_status, &cur_psu_status, sizeof(T8_Dnepr_PsStatusTypedef) );
//	Dnepr_DControl_ResetMaxPower(); // ����� ��� ��������� ��������� ������� �������
//	
//	__last_pmbus_alarm = llUptime ;        
//
//	while(TRUE){
//          qCurMessage = (task_message_t*)OSQPend( __qRcvQueue, PMBUS_ALARM_TIMEOUT, &return_code );
//          assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_TIMEOUT) );
//          // ���� ������ ������� � ���������� ���������� PSU -- �������� ���� ���� �������
//          if( return_code == OS_ERR_TIMEOUT ){
//        	// ��� � 5 ������ ��������� �� alarm
//          	if( Dnepr_EdgePort_is_IRQ4_active() &&
//        		((llUptime - __last_pmbus_alarm) > PMBUS_ALARM_TIMEOUT) ){
//        		__last_pmbus_alarm = llUptime ;
//        		__pmbus_alarm_handler() ;
//        	}
//        	continue ;
//        } /* while(TRUE) */
//
//        switch( qCurMessage->message_type ){
//        	case PSU_VALUES:
//        		// ������������ �� ����������� ���������
//				Dnepr_Backplane_Reload_PSU_Info() ;
//				pPSUInfo1 = Dnepr_Backplane_GetPSU_Info( 0 );
//				pPSUInfo2 = Dnepr_Backplane_GetPSU_Info( 1 );
//				__fPowerLimit1 = pPSUInfo1 ? pPSUInfo1->fPower : 0.0 ;
//				__fPowerLimit2 = pPSUInfo2 ? pPSUInfo2->fPower : 0.0 ;
//
//                              // ���� �� ������ ��������� �������� -- ������������ ����� �����                                
//                                /* ���� ������������ ��������� �� ����������� �� ������ ������ ������������� !! */
//				if( (__fPowerLimit1 < 10.) || (__fPowerLimit1 > 3000.) ||
//					(!Dnepr_Backplane_GetPSU_Status()->tPs1.bPowerGood) ){
//					__fPowerLimit1 = FLT_MAX ;
//				}
//                		if( (__fPowerLimit2 < 10.) || (__fPowerLimit2 > 3000.) ||
//        			(!Dnepr_Backplane_GetPSU_Status()->tPs2.bPowerGood) ){
//					__fPowerLimit2= FLT_MAX ;
//				}
//
//				// ��������� ���������� POWER
//				// ���� ��� ������ �� �� -- ����� ������
//				if( !Dnepr_Backplane_PS1_Present() || !Dnepr_Backplane_PS2_Present() ){
//					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){YELLOW, FALSE} );
//				// ���� � ���� �� ������ �� �� PowerGood -- ������ ������
//				} else if( 	(!Dnepr_Backplane_GetPSU_Status()->tPs1.bPowerGood	) ||
//							(!Dnepr_Backplane_GetPSU_Status()->tPs2.bPowerGood	) ){
//					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){YELLOW, TRUE} );
//				// �� ������
//				} else {
//					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){GREEN, FALSE} );
//				}
//        		
//        		// ��������� ���������� ������ ��������
//        		if( __bPSULimitSource == TRUE ){
//        			__fPowerLimit = MIN( __fPowerLimit1, __fPowerLimit2 );
//				// �������� ����������� -- �������
//        		} else {
//        			if( val_VPowerLimit > 1 ){
//						__fPowerLimit = val_VPowerLimit ;
//					} else {
//						__fPowerLimit = FLT_MAX ;
//					}
//				}
//
//				// ��������� ���������� � ����������� SFP
//				Dnepr_Refresh_SFP_Presents() ;
//				// ���������� ��������� ����������� ��� �������� (DeviceController_Init())
//	        	__slot_power_onoff();
//	        	// ����������� �����������.
//	        	__set_fans() ;
//        	break ;
//                
//        	// ���������� ��� ������������� �������� ���������
//        	case INIT_HOTSWAPS:
//	        	__slot_power_onoff();
//	        	__set_fans() ;
//        	break ;
//                
//        	// ���������� ��� ������������� �������� ����������, ������� �� ���� ��������
//        	// ��-�� ���������� ��������
//        	case REINIT_ONOFF:
//        		__slot_power_onoff_init() ;
//        		__set_fans() ;
//        	break ;
//                
//        	// ���������� �� PMBus alarm
//        	// ���� � ��������� PMBUS_ALARM_TIMEOUT, ��� �� ���� -- ���������.
//        	case PMBUS_ALARM :
//        		if( llUptime > __last_pmbus_alarm ){
//        			if( (llUptime - __last_pmbus_alarm) > PMBUS_ALARM_TIMEOUT ){
//        				__last_pmbus_alarm = llUptime ;
//			        	__pmbus_alarm_handler() ;
//        			}
//        		// ���� ���� ������������ llUptime
//        		// ��������� �� PMBus_alarm
//        		} else {
//        			__last_pmbus_alarm = llUptime ;
//        			__pmbus_alarm_handler() ;
//        		}
//        	break ;
//                
//        	case PRESENT_INTERRUPT :
//				// ���� ���������� present'�
//				memcpy( &prev_presents, I2C_DNEPR_GetPresentDevices(), sizeof(I2C_DNEPR_PresentDevicesTypedef) );
//				OSTimeDly( __present_react_timeout );
//				// ������������ �������� �� ����
//				Dnepr_Refresh_Presents() ;
//
//	        	// ���� ��������
//				for(i=0; i<I2C_DNEPR_NUMBER_OF_SLOTS; i++){
//					if( I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] != 
//										prev_presents.bSlotPresent[i]){
//		        		// �������� ��������� ��������� ��� ��������� ���������
//		        		__slot_power_dev_plug( i, I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] );
//		        		// ����������� �����������.
//			        	__set_fans() ;
//					}
//				}
//				for( i = 0; i < 3; i++ ){
//					if( Dnepr_Reload_PSU_Status_Pins() ){
//						break ;
//					}
//				}
//				// �� ������� ��������� �������� ��������� ������� ��
//				if( i >= 3 ){
//					break ;
//				}
//				memcpy( &cur_psu_status, Dnepr_Backplane_GetPSU_Status(), sizeof(T8_Dnepr_PsStatusTypedef) );
//				if( (cur_psu_status.tPs1.bSeated != prev_psu_status.tPs1.bSeated) ||
//					(cur_psu_status.tPs2.bSeated != prev_psu_status.tPs2.bSeated) ||
//					(cur_psu_status.tPs1.bPowerGood != prev_psu_status.tPs1.bPowerGood) ||
//					(cur_psu_status.tPs2.bPowerGood != prev_psu_status.tPs2.bPowerGood) ){
//					// ������ ��������/������ ��
//					Dnepr_DControl_ResetMaxPower();
//				}
//				memcpy( &prev_psu_status, &cur_psu_status, sizeof(T8_Dnepr_PsStatusTypedef) );
//        	break ;
//                
//        	case SFP_INTERRUPT :
//		    	Dnepr_Refresh_SFP_Presents() ;
//        	break ;
//                
//        	// � ������� ���������� ���������� ������� �����
////        	case POWER_ALLOW :
////        		if( I2C_DNEPR_GetPresentDevices()->bSlotPresent[qCurMessage->slot_num_2_write] ){
////	        		__slot_power_dev_plug( qCurMessage->slot_num_2_write, TRUE );
////	        	}
////        	break ;
//                
//                
//                
//        	// � ������� �������� ��������� �������������� ������������.
//        	case FAN_SETTINGS_CHANGED :
//        		__set_fans() ;
//        	break ;
//        }
//	}
//}






void taskDeviceController(void *pdata)
{
       	msg_inttask_t	                      *dc_incom_message = NULL;
	u32                                   i ;
	I2C_DNEPR_PresentDevicesTypedef	      prev_presents ;
	T8_Dnepr_PsStatusTypedef              cur_psu_status ;
	T8_Dnepr_PsStatusTypedef              prev_psu_status ;
	const PSU_UnitInfoTypedef*            pPSUInfo1 = NULL ;
	const PSU_UnitInfoTypedef*            pPSUInfo2 = NULL ;
	
	pdata = pdata;
	
	// ������� �������� �������
	__qPMBAlarms = OSQCreate( __pmbalarms_arr, __pmbalarms_arr_len ) ;

	Dnepr_Refresh_Presents() ; // ������������ �������� � 
	
        // ���������� ������ ����
	for( i = 0; i < 3; i++ ){
		if( Dnepr_Reload_PSU_Status_Pins() ){
			break ;
		}
	}
	memcpy( &cur_psu_status, Dnepr_Backplane_GetPSU_Status(), sizeof(T8_Dnepr_PsStatusTypedef) );
	memcpy( &prev_psu_status, &cur_psu_status, sizeof(T8_Dnepr_PsStatusTypedef) );
        
	Dnepr_DControl_ReinitPowerSource(); // ����� ��� ��������� ��������� ������� �������
        Dnepr_DControl_SetFans();           // ��������� ���������� � ����������� SFP
        Dnepr_DControl_SFP_Interrupt();     // ����������� �����������.
        
	__last_pmbus_alarm = llUptime ;        

	while (TRUE) {
          dc_incom_message = recv_inttask_message(DEVICE_CONTROLLER, ANY_MODULE, NULL);
                    
          // ���� ������ ������� � ���������� ���������� PSU -- �������� ���� ���� �������
          if( dc_incom_message == NULL ){
        	// ��� � 5 ������ ��������� �� alarm
          	if( Dnepr_EdgePort_is_IRQ4_active() &&
        		((llUptime - __last_pmbus_alarm) > PMBUS_ALARM_TIMEOUT) ){
        		__last_pmbus_alarm = llUptime ;
        		__pmbus_alarm_handler() ;
        	}
        	continue ;
        } /* while(TRUE) */
        
        
        switch( dc_incom_message->msg ){
          
                case GET_SHELF_STATE:                  
                  _send_shelf_state(dc_incom_message->data);
                break;
                
                case SET_SHELF_STATE:
                  _set_state_and_send_state(dc_incom_message->data);
                break;

                case GET_SHELF_POWER:
                  _send_shelf_power(dc_incom_message->data);                  
                break;
          
                case FAILURE_SIGNALING:
                  
                break;
                
        	case PSU_VALUES:
        		// ������������ �� ����������� ���������
				Dnepr_Backplane_Reload_PSU_Info() ;
				pPSUInfo1 = Dnepr_Backplane_GetPSU_Info( 0 );
				pPSUInfo2 = Dnepr_Backplane_GetPSU_Info( 1 );

				// ��������� ���������� POWER
				// ���� ��� ������ �� �� -- ����� ������
				if( !Dnepr_Backplane_PS1_Present() || !Dnepr_Backplane_PS2_Present() ){
					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){YELLOW, FALSE} );
				// ���� � ���� �� ������ �� �� PowerGood -- ������ ������
				} else if( 	(!Dnepr_Backplane_GetPSU_Status()->tPs1.bPowerGood	) ||
						(!Dnepr_Backplane_GetPSU_Status()->tPs2.bPowerGood	) ){
					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){YELLOW, TRUE} );
				// �� ������
				} else {
					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){GREEN, FALSE} );
				}
                                topwmng_msg_crate_change();                                
        	break ;
                
                
        	// ���������� �� PMBus alarm
        	// ���� � ��������� PMBUS_ALARM_TIMEOUT, ��� �� ���� -- ���������.
        	case PMBUS_ALARM :
        		if( llUptime > __last_pmbus_alarm ){
        			if( (llUptime - __last_pmbus_alarm) > PMBUS_ALARM_TIMEOUT ){
        				__last_pmbus_alarm = llUptime ;
			        	__pmbus_alarm_handler() ;
        			}
        		// ���� ���� ������������ llUptime
        		// ��������� �� PMBus_alarm
        		} else {
        			__last_pmbus_alarm = llUptime ;
        			__pmbus_alarm_handler() ;
        		}
        	break ;
                
        	case PRESENT_INTERRUPT :
				// ���� ���������� present'�
				memcpy( &prev_presents, I2C_DNEPR_GetPresentDevices(), sizeof(I2C_DNEPR_PresentDevicesTypedef) );
				OSTimeDly( __present_react_timeout );
				// ������������ �������� �� ����
				Dnepr_Refresh_Presents() ;

	        	// ���� ��������
				for(i=0; i<I2C_DNEPR_NUMBER_OF_SLOTS; i++){
					if( I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] != 
										prev_presents.bSlotPresent[i]){
		        		// �������� ��������� ��������� ��� ��������� ���������
		        		__slot_power_dev_plug( i, I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] );
		        		// ����������� �����������.
			        	__set_fans() ;
					}
				}
				for( i = 0; i < 3; i++ ){
					if( Dnepr_Reload_PSU_Status_Pins() ){
						break ;
					}
				}
				// �� ������� ��������� �������� ��������� ������� ��
				if( i >= 3 ){
					break ;
				}
				memcpy( &cur_psu_status, Dnepr_Backplane_GetPSU_Status(), sizeof(T8_Dnepr_PsStatusTypedef) );
				if( (cur_psu_status.tPs1.bSeated != prev_psu_status.tPs1.bSeated) ||
					(cur_psu_status.tPs2.bSeated != prev_psu_status.tPs2.bSeated) ||
					(cur_psu_status.tPs1.bPowerGood != prev_psu_status.tPs1.bPowerGood) ||
					(cur_psu_status.tPs2.bPowerGood != prev_psu_status.tPs2.bPowerGood) ){
					// ������ ��������/������ ��
//					Dnepr_DControl_ResetMaxPower();
                                          Dnepr_DControl_ReinitPowerSource();
                                        } else {
                                          topwmng_msg_crate_change();
                                        }
				memcpy( &prev_psu_status, &cur_psu_status, sizeof(T8_Dnepr_PsStatusTypedef) );
        	break ;
                
        	case SFP_INTERRUPT :
		    Dnepr_Refresh_SFP_Presents() ;
        	break ;
                                
                
        	// � ������� �������� ��������� �������������� ������������.
        	case FAN_SETTINGS_CHANGED :
        	  __set_fans() ;
        	break ;
        }
	}
}


static void _send_shelf_state(uint8_t *data)
{   
    msg_pwr_data_t	  *recv_data;
    msg_inttask_t         topwr_msg;    
    msg_dc_state_data_t	  send_data;
    
    recv_data = (msg_pwr_data_t*)data;
    switch ( recv_data->dev_name ) {            
    case SLOT_1:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[0]; 
          send_data.power_state     = ( __aPwrStates[0] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[0] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(0) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    case SLOT_2:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[1]; 
          send_data.power_state     = ( __aPwrStates[1] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[1] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(1) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_3:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[2]; 
          send_data.power_state     = ( __aPwrStates[2] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[2] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(2) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_4:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[3]; 
          send_data.power_state     = ( __aPwrStates[3] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[3] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(3) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_5:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[4]; 
          send_data.power_state     = ( __aPwrStates[4] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[4] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(4) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_6:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[5]; 
          send_data.power_state     = ( __aPwrStates[5] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[5] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(5) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_7:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[6]; 
          send_data.power_state     = ( __aPwrStates[6] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[6] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(6) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_8:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[7]; 
          send_data.power_state     = ( __aPwrStates[7] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[7] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(7) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;      
    case SLOT_9:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[8]; 
          send_data.power_state     = ( __aPwrStates[8] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[8] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(8) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_10:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[9]; 
          send_data.power_state     = ( __aPwrStates[9] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[9] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(9) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_11:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[10]; 
          send_data.power_state     = ( __aPwrStates[10] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[10] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(10) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_12:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[11]; 
          send_data.power_state     = ( __aPwrStates[11] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[11] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(11) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    case SLOT_13:  {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[12]; 
          send_data.power_state     = ( __aPwrStates[12] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = ( _hs_available[12] == TRUE ) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(12) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    case POWER1:    {
          send_data.present         = (Dnepr_Backplane_GetPSU_Status()->tPs1.bSeated) ? 0 : 1;
          send_data.power_state     = (Dnepr_Backplane_GetPSU_Status()->tPs1.bInOk) ?  SLOT_OFF : SLOT_ON;
          send_data.hotswap_status  = CHIP_OK;
          send_data.eeprom_status   = (strlen(Dnepr_Backplane_GetPSU_Info(0)->sManufacturer) != 0) ? CHIP_OK : CHIP_FAIL;
    } break;    
    case POWER2:    {
          send_data.present         = (Dnepr_Backplane_GetPSU_Status()->tPs2.bSeated) ? 0 : 1;
          send_data.power_state     = (Dnepr_Backplane_GetPSU_Status()->tPs2.bInOk) ? SLOT_OFF : SLOT_ON;
          send_data.hotswap_status  = CHIP_OK;
          send_data.eeprom_status   = (strlen(Dnepr_Backplane_GetPSU_Info(1)->sManufacturer) != 0) ? CHIP_OK : CHIP_FAIL;          
    } break;    
    case FAN: {
          send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM];
          send_data.power_state     = (send_data.present) ? SLOT_ON : SLOT_OFF;
          send_data.hotswap_status  = (Dnepr_Fans_Calibrated()) ? CHIP_OK : CHIP_FAIL;
          send_data.eeprom_status   = (Dnepr_Fans_Calibrated()) ? CHIP_OK : CHIP_FAIL;          
    }break;
  } /*  switch ( recv_data->dev_name ) */
            
    topwr_msg.msg = GET_SHELF_STATE;
    topwr_msg.data = &send_data;     
    send_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, &topwr_msg);
}



static void _set_state_and_send_state (uint8_t *data)
{
    msg_pwr_data_t	  *recv_data;
    msg_inttask_t         topwr_msg;    
    msg_dc_state_data_t	  send_data;

    recv_data = (msg_pwr_data_t*)data;
    switch ( recv_data->dev_name ) { 
      
    case SLOT_1:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[0]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(0);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(0);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[0]; 
      send_data.power_state     = ( __aPwrStates[0] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[0] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(0) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_2:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[1]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(1);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(1);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[1]; 
      send_data.power_state     = ( __aPwrStates[1] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[1] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(1) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_3:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[2]) {
        if (recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(2);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(2);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[2]; 
      send_data.power_state     = ( __aPwrStates[2] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[2] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(2) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    

    case SLOT_4:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[3]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(3);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(3);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[3]; 
      send_data.power_state     = ( __aPwrStates[3] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[3] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(3) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_5:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[4]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(4);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(4);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[4]; 
      send_data.power_state     = ( __aPwrStates[4] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[4] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(4) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_6:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[5]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(5);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(5);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[5]; 
      send_data.power_state     = ( __aPwrStates[5] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[5] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(5) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_7:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[6]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(6);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(6);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[6]; 
      send_data.power_state     = ( __aPwrStates[6] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[6] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(6) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    

    case SLOT_8:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[7]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(7);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(7);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[7]; 
      send_data.power_state     = ( __aPwrStates[7] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[7] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(7) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_9:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[8]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(8);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(8);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[8]; 
      send_data.power_state     = ( __aPwrStates[8] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[8] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(8) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_10:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[9]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(9);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(9);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[9]; 
      send_data.power_state     = ( __aPwrStates[9] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[9] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(9) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    
    
    case SLOT_11:  {
      if (Dnepr_DControl_SlotPresent()->bSlotPresent[10]) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(10);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(10);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[10]; 
      send_data.power_state     = ( __aPwrStates[10] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[10] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(10) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    

    case SLOT_12:  {
      if ( Dnepr_DControl_SlotPresent()->bSlotPresent[11] ) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(11);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(11);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[11]; 
      send_data.power_state     = ( __aPwrStates[11] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[11] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(11) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;    

    case SLOT_13:  {
      if ( Dnepr_DControl_SlotPresent()->bSlotPresent[12] ) {
        if ( recv_data->dev_state == SLOT_WAITING || recv_data->dev_state == SLOT_OFF ) {
            _slot_power_off(12);
        } else if ( recv_data->dev_state == SLOT_ON ) {
            _slot_power_on(12);        
        }
      }
      send_data.present         = Dnepr_DControl_SlotPresent()->bSlotPresent[12]; 
      send_data.power_state     = ( __aPwrStates[12] == HSSLOT_ON ) ? SLOT_ON : SLOT_OFF;
      send_data.hotswap_status  = ( _hs_available[12] == TRUE ) ? CHIP_OK : CHIP_FAIL;
      send_data.eeprom_status   = ( Dnepr_SlotEEPROM_Available(12) == TRUE ) ? CHIP_OK : CHIP_FAIL;
    } break;
    
    } /* switch ( recv_data->dev_name ) */    
  
    topwr_msg.msg = SET_SHELF_STATE;
    topwr_msg.data = &send_data;     
    send_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, &topwr_msg);
}



static void _send_shelf_power         (uint8_t *data)
{
    msg_pwr_data_t	  *recv_data;
    msg_inttask_t         topwr_msg;    
    msg_dc_power_data_t	  send_data;

    recv_data = (msg_pwr_data_t*)data;
    send_data.max_power = 0.0;
    switch ( recv_data->dev_name ) { 
    case SLOT_1:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[0]  )  { send_data.max_power = __fSlotPower[0];  }  } break;    
    case SLOT_2:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[1]  )  { send_data.max_power = __fSlotPower[1];  }  } break;      
    case SLOT_3:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[2]  )  { send_data.max_power = __fSlotPower[2];  }  } break;  
    case SLOT_4:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[3]  )  { send_data.max_power = __fSlotPower[3];  }  } break;  
    case SLOT_5:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[4]  )  { send_data.max_power = __fSlotPower[4];  }  } break;  
    case SLOT_6:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[5]  )  { send_data.max_power = __fSlotPower[5];  }  } break;  
    case SLOT_7:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[6]  )  { send_data.max_power = __fSlotPower[6];  }  } break;  
    case SLOT_8:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[7]  )  { send_data.max_power = __fSlotPower[7];  }  } break;  
    case SLOT_9:    { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[8]  )  { send_data.max_power = __fSlotPower[8];  }  } break;  
    case SLOT_10:   { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[9]  )  { send_data.max_power = __fSlotPower[9];  }  } break;  
    case SLOT_11:   { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[10] )  { send_data.max_power = __fSlotPower[10]; }  } break;  
    case SLOT_12:   { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[11] )  { send_data.max_power = __fSlotPower[11]; }  } break;  
    case SLOT_13:   { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[12] )  { send_data.max_power = __fSlotPower[12]; }  } break;  
    case POWER1:    { if ( Dnepr_Backplane_GetPSU_Status()->tPs1.bSeated == 0 )  { send_data.max_power = Dnepr_Backplane_GetPSU_Info(0)->fPower; }  } break;
    case POWER2:    { if ( Dnepr_Backplane_GetPSU_Status()->tPs2.bSeated == 0 )  { send_data.max_power = Dnepr_Backplane_GetPSU_Info(1)->fPower; }  } break; 
    case POWER3:    { send_data.max_power = 0.0; }               break;
    case POWER4:    { send_data.max_power = 0.0; }               break;
    case FAN:       { if ( Dnepr_DControl_SlotPresent()->bSlotPresent[13] )  { send_data.max_power = __fSlotPower[13]; }  } break; 
    }
    
    topwr_msg.msg = GET_SHELF_POWER;
    topwr_msg.data = &send_data;     
    send_inttask_message(POWER_MANAGMENT, DEVICE_CONTROLLER, &topwr_msg);    
}



//// ������� �� ��������� present'�� � ����:
//// ������� ��������� EEPROM, ����� ��������� / �� ��������� ������
//static void __slot_power_dev_plug( const u8 slot_num, const _BOOL pluginout )
//{
//	size_t j ;
//	LT_LTC4222_AdcVoltagesStructure     tAdcVoltagesStructure;
//	LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
//	LT_LTC4222_StatusRegisterTypedef 	tStateRegisterStructure_ch1 ;
//	LT_LTC4222_AdcControlRegisterTypedef tAdcControlRegisterStructure ;
//
//	// ���������� ������ ��� ��������
//	if( pluginout ){
//		// ������ � ����������� HotSwap
//		// ��������� ������� hotswap'� � ��������� ���������� (������ PMBus �� ���������� ������)
//		for( j = 0; j < 10; j++ ){
//			_hs_available[slot_num] = LT_LTC4222_GetAdcVoltages( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcVoltagesStructure );
//			_hs_available[slot_num] = _hs_available[slot_num] && (tAdcVoltagesStructure.fSource2 > 2.7) && 
//							                      (tAdcVoltagesStructure.fSource2 < 4.0) ;
//			if( _hs_available[slot_num] ){
//				break  ;
//			} else {
//				OSTimeDly( 20 );
//			}
//		}
//		Dnepr_SlotEEPROM_Read( slot_num );
//		
//		// ����������� hotswap
//		if( _hs_available[slot_num] ){
//			// ������ ������� �������� ��������
//			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
//			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
//
//			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ],  LT_LTC4222_CHANNEL1, &tStateRegisterStructure_ch1, NULL );
//			
//                        
//                        
//                        
//                        
//			// ���� ���������� �������� ������ -- �� �������� ���
//			if( __slot_OnOff( slot_num ) == PERMITTED_START ){
//				__fSlotPower[ slot_num ] = 0 ;
//				hs_control_reg_ch1.bFetOnControl = 0 ;
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
//				hs_control_reg_ch2.bGpioOutputState = 0 ;
//				__aPwrStates[ slot_num ] = HSSLOT_REGULAR_OFF ;
//			// ���������� �������� �� ����������
//			// ���� ���������� �������� -- �� ����� ������
//			} else if( tStateRegisterStructure_ch1.bFetOn ){
//				__aPwrStates[ slot_num ] = HSSLOT_ON ;
//				__fSlotPower[ slot_num ] = 0.0 ;
//			// ���� EEPROM �������� � �������� ���������� ��� ����� ��� ������� -- ��������
//			} else if(	Dnepr_SlotEEPROM_Available( slot_num ) && 
//					((f32)Dnepr_SlotEEPROM_SlotPower( slot_num ) < __curPowerLimit()) ||
//					(__slot_OnOff( slot_num ) == IGNORANT_START) ){
//						__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num );
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_PGOOD ;
//				hs_control_reg_ch2.bGpioOutputState = 1 ;
//				hs_control_reg_ch1.bFetOnControl = 1 ;
//				__aPwrStates[ slot_num ] = HSSLOT_ON ;
//			} else {
//				hs_control_reg_ch1.bFetOnControl = 1 ;
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
//				hs_control_reg_ch2.bGpioOutputState = 0 ;
//				__fSlotPower[ slot_num ] = 0 ;
//				__aPwrStates[ slot_num ] = HSSLOT_OVERLIMIT_OFF ;
//			}
//
//			if( Dnepr_SlotEEPROM_Available( slot_num )){
//				if( __aPwrStates[ slot_num ] == HSSLOT_ON ){
//					__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num ) ;
//				}
//				// 1� ����� 12 �
//				hs_control_reg_ch1.bOcAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->oc_ar1 ;
//				hs_control_reg_ch1.bUvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->uv_ar1 ;
//				hs_control_reg_ch1.bOvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->ov_ar1 ;
//				// 2� ����� 3.3 �
//				hs_control_reg_ch2.bOcAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->oc_ar2 ;
//				hs_control_reg_ch2.bUvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->uv_ar2 ;
//				hs_control_reg_ch2.bOvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->ov_ar2 ;
//			}
//
//			// ������������� ������ �� ALERT ��� ���������� �������������� ���
//			LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcControlRegisterStructure );
//			tAdcControlRegisterStructure.bAdcAlert = FALSE ;
//			LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcControlRegisterStructure );
//
//			// �������� ������ � ������
//			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, &hs_alert_reg );
//			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, &hs_alert_reg );
//			OSTimeDly( 250 );
//                        
//			// ���������� �����
//			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1 );
//			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2 );
//
//			// ��������� ����� �������� ��� ��������� ��������
//			__slot_plugin_time[ slot_num ] = llUptime ;
//                        
//		} else {
//			// ���� hotswap ����������, �� ���� ������ �� EEPROM -- �����������,
//			// ��� ���������� ���������� ��� ��������
//			if( Dnepr_SlotEEPROM_Available( slot_num )){
//				__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num );
//			}
//			__aPwrStates[ slot_num ] = HSSLOT_UNAVAILABLE ;
//		}
//		__processed_slots_present.bSlotPresent[ slot_num ] = 1 ;
//                
//	// ���������� ������
//	} else {
//		__processed_slots_present.bSlotPresent[ slot_num ] = 0 ;
//		__aPwrStates[ slot_num ] = HSSLOT_UNAVAILABLE ;
//		__fSlotPower[ slot_num ] = 0 ;
//	}
//}

//static void __slot_power_on  ( const u8 slot_num, const _BOOL pluginout )
//static void __slot_power_off ( const u8 slot_num, const _BOOL pluginout )
//
//			// ���� ���������� �������� ������ -- �� �������� ���
//			if( __slot_OnOff( slot_num ) == PERMITTED_START ){
//				__fSlotPower[ slot_num ] = 0 ;
//				hs_control_reg_ch1.bFetOnControl = 0 ;
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
//				hs_control_reg_ch2.bGpioOutputState = 0 ;
//				__aPwrStates[ slot_num ] = HSSLOT_REGULAR_OFF ;
//			// ���������� �������� �� ����������
//			// ���� ���������� �������� -- �� ����� ������
//			} else if( tStateRegisterStructure_ch1.bFetOn ){
//				__aPwrStates[ slot_num ] = HSSLOT_ON ;
//				__fSlotPower[ slot_num ] = 0.0 ;
//			// ���� EEPROM �������� � �������� ���������� ��� ����� ��� ������� -- ��������
//			} else if(	Dnepr_SlotEEPROM_Available( slot_num ) && 
//					((f32)Dnepr_SlotEEPROM_SlotPower( slot_num ) < __curPowerLimit()) ||
//					(__slot_OnOff( slot_num ) == IGNORANT_START) ){
//						__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num );
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_PGOOD ;
//				hs_control_reg_ch2.bGpioOutputState = 1 ;
//				hs_control_reg_ch1.bFetOnControl = 1 ;
//				__aPwrStates[ slot_num ] = HSSLOT_ON ;
//			} else {
//				hs_control_reg_ch1.bFetOnControl = 1 ;
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
//				hs_control_reg_ch2.bGpioOutputState = 0 ;
//				__fSlotPower[ slot_num ] = 0 ;
//				__aPwrStates[ slot_num ] = HSSLOT_OVERLIMIT_OFF ;
//			}
//			if( Dnepr_SlotEEPROM_Available( slot_num )){
//				if( __aPwrStates[ slot_num ] == HSSLOT_ON ){
//					__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num ) ;
//				}
//				// 1� ����� 12 �
//				hs_control_reg_ch1.bOcAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->oc_ar1 ;
//				hs_control_reg_ch1.bUvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->uv_ar1 ;
//				hs_control_reg_ch1.bOvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->ov_ar1 ;
//				// 2� ����� 3.3 �
//				hs_control_reg_ch2.bOcAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->oc_ar2 ;
//				hs_control_reg_ch2.bUvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->uv_ar2 ;
//				hs_control_reg_ch2.bOvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->ov_ar2 ;
//			}
//			// �������� ������ � ������
//			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, &hs_alert_reg );
//			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, &hs_alert_reg );
//			OSTimeDly( 250 );



// ������� �� ��������� present'�� � ����:
// ������� ��������� EEPROM, ����� ��������� / �� ��������� ������
static void __slot_power_dev_plug( const u8 slot_num, const _BOOL pluginout )
{
	size_t                                j;
	LT_LTC4222_AdcVoltagesStructure       tAdcVoltagesStructure;
	LT_LTC4222_ControlRegisterTypedef     hs_control_reg_ch1, hs_control_reg_ch2 ;
	LT_LTC4222_StatusRegisterTypedef      tStateRegisterStructure_ch1 ;
	LT_LTC4222_AdcControlRegisterTypedef  tAdcControlRegisterStructure ;

	// ���������� ������ ��� ��������
	if( pluginout ){
		// ������ � ����������� HotSwap
		// ��������� ������� hotswap'� � ��������� ���������� (������ PMBus �� ���������� ������)
		for( j = 0; j < 3; j++ ){
			_hs_available[slot_num] = LT_LTC4222_GetAdcVoltages( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcVoltagesStructure );
			_hs_available[slot_num] = _hs_available[slot_num] && (tAdcVoltagesStructure.fSource2 > 2.7) && 
							                      (tAdcVoltagesStructure.fSource2 < 4.0) ;
			if( _hs_available[slot_num] ){
			    break  ;
			} else {
			    OSTimeDly( 20 );
			}
		}
		Dnepr_SlotEEPROM_Read( slot_num );
                if( Dnepr_SlotEEPROM_Available( slot_num )) {                              
                     __fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num ) ;
                } else {
                    __fSlotPower[ slot_num ] = 0.0;
                }
		
		// ����������� hotswap
		if( _hs_available[slot_num] ){
			// ������ ������� �������� ��������
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1, &tStateRegisterStructure_ch1, NULL );
			
                        if ( tStateRegisterStructure_ch1.bFetOn ) {
                            __aPwrStates[ slot_num ] = HSSLOT_ON ;
                        } else {
                            __aPwrStates[ slot_num ] = HSSLOT_WAITING ;
                        }
                        
                       
			// ������������� ������ �� ALERT ��� ���������� �������������� ���
			LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcControlRegisterStructure );
			tAdcControlRegisterStructure.bAdcAlert = FALSE ;
			LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcControlRegisterStructure );
                        
			// ���������� �����
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1 );
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2 );

			// ��������� ����� �������� ��� ��������� ��������
			__slot_plugin_time[ slot_num ] = llUptime ;
                        
		} else {
			// ���� hotswap ����������, �� ���� ������ �� EEPROM -- �����������,
			// ��� ���������� ���������� ��� ��������
			if( Dnepr_SlotEEPROM_Available(slot_num) ){
			    __fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num );
			}
			__aPwrStates[ slot_num ] = HSSLOT_UNAVAILABLE ;
		}
		__processed_slots_present.bSlotPresent[ slot_num ] = 1 ;
                
	// ���������� ������
	} else {
		__processed_slots_present.bSlotPresent[ slot_num ] = 0 ;
		__aPwrStates[ slot_num ] = HSSLOT_UNAVAILABLE ;
		__fSlotPower[ slot_num ] = 0 ;
	}
}


// ��� ��������� ������ �������� EEPROM � HotSwap'� ���� �������� ���������,
// ��������� �� ���������, ������������������ HotSwap�, ��������� ���������
// ��������� ���������
void __slot_power_onoff_init()
//static void __slot_power_onoff_init()
{
	size_t i, j ;
	LT_LTC4222_AdcVoltagesStructure     tAdcVoltagesStructure;
	LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
	LT_LTC4222_StatusRegisterTypedef 	tStateRegisterStructure_ch1 ;
	LT_LTC4222_AdcControlRegisterTypedef tAdcControlRegisterStructure ;
//	_BOOL hs_available ;

	Dnepr_Refresh_Presents() ;

	// ������ �� ������� ��� EEPROM � ��������� HotSwap'�� (+ 1 ���� ��� �����
	// ����������)
	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
		if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] ){
			__aPwrStates[ i ] = HSSLOT_UNAVAILABLE ;
			continue ;
		}
		__processed_slots_present.bSlotPresent[ i ] = 1 ;
		Dnepr_SlotEEPROM_Read( i );
                if( Dnepr_SlotEEPROM_Available( i )) {                              
                     __fSlotPower[ i ] = (f32)Dnepr_SlotEEPROM_SlotPower( i ) ;
                } else {
                    __fSlotPower[ i ] = 0.0;
                }
                
		// ������ � ����������� HotSwap
		// ��������� ������� hotswap'� �������� ����������
		for( j = 0; j < 5; j++ ){
			LT_LTC4222_GetAdcVoltages( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcVoltagesStructure );
			_hs_available[i] = 	(tAdcVoltagesStructure.fSource2 > 2.0) && 
							(tAdcVoltagesStructure.fSource2 < 4.0) ;
			if( _hs_available[i] ){
				break ;
			} else {
				OSTimeDly( 1 );
			}
		}
                


		// ����������� hotswap
		if( _hs_available[i] ){
			// ������ ������� �������� ��������
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );

			// ���������� �������� �� ����������
			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i],  LT_LTC4222_CHANNEL1, &tStateRegisterStructure_ch1, NULL );
			// ���� ���������� �������� -- �� ����� ������
			if( (tStateRegisterStructure_ch1.bFetOn) || 
				(i == I2C_DNEPR_FAN_SLOT_NUM - 1)){ // ��� ��� ����������� -- ���� �� �������
				__aPwrStates[ i ] = HSSLOT_ON ;
			// ���������� �� �������� -- � ��������� ������ ����� ����, ��� ��������� ��������� �� �����
			// � eeprom
			} else {
				__aPwrStates[ i ] = HSSLOT_WAITING ;
			}
			// ���� �������� EEPROM -- ����������� hotswap � ��������� ������������ ��������
			if( Dnepr_SlotEEPROM_Available( i )){
				// 1� ����� 12 �
				hs_control_reg_ch1.bOcAutoRetry = Dnepr_SlotEEPROM_val( i )->oc_ar1 ;
				hs_control_reg_ch1.bUvAutoRetry = Dnepr_SlotEEPROM_val( i )->uv_ar1 ;
				hs_control_reg_ch1.bOvAutoRetry = Dnepr_SlotEEPROM_val( i )->ov_ar1 ;
				// 2� ����� 3.3 �
				hs_control_reg_ch2.bOcAutoRetry = Dnepr_SlotEEPROM_val( i )->oc_ar2 ;
				hs_control_reg_ch2.bUvAutoRetry = Dnepr_SlotEEPROM_val( i )->uv_ar2 ;
				hs_control_reg_ch2.bOvAutoRetry = Dnepr_SlotEEPROM_val( i )->ov_ar2 ;
			}
			// ����������� ��������� ����������
			if( __aPwrStates[ i ] == HSSLOT_WAITING ){
				hs_control_reg_ch1.bFetOnControl = 1 ;
				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
				hs_control_reg_ch2.bGpioOutputState = 0 ;
			}
			//������������� ������ �� ALERT ��� ���������� �������������� ���
			LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );
			tAdcControlRegisterStructure.bAdcAlert = FALSE ;
			LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );

			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, &hs_alert_reg );
			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, &hs_alert_reg );
			// ���������� �����
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2 );
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1 );
		} else {
			// ���� hotswap ����������, �� ���� ������ �� EEPROM -- �����������,
			// ��� ���������� ���������� ��� ��������
			if( Dnepr_SlotEEPROM_Available( i )){
				__fSlotPower[ i ] = (f32)Dnepr_SlotEEPROM_SlotPower( i );
			}
			__aPwrStates[ i ] = HSSLOT_UNAVAILABLE ;
		}
	}
}


static void _slot_power_on( uint8_t dev_index )
{
  LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
  LT_LTC4222_AdcControlRegisterTypedef  tAdcControlRegisterStructure ;
  LT_LTC4222_StatusRegisterTypedef      __hs_slot_statuses1, __hs_slot_statuses2 ;
  LT_LTC4222_FaultAlertRegisterTypedef  __hs_faults1, __hs_faults2 ;

  
  LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
  LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
  hs_control_reg_ch2.tGpioConfig = LT_LTC4222_PGOOD ;
  hs_control_reg_ch2.bGpioOutputState = 1 ;
  hs_control_reg_ch1.bFetOnControl = 1 ;
  LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
  LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
  
  
// ������������� ������ �� ALERT ��� ���������� �������������� ���
  LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], &tAdcControlRegisterStructure );
  tAdcControlRegisterStructure.bAdcAlert = FALSE ;
  LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], &tAdcControlRegisterStructure );

  OSTimeDly( 200 );
  
// ���������� �����
  LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL2 );
  LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL1 );
// ��������� ������� ����� hotswap'�
  LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL1, &__hs_slot_statuses1, &__hs_faults1 );
  LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL2, &__hs_slot_statuses2, &__hs_faults2 );

  // ��������� ����� �������� ��� ��������� ��������
  __aPwrStates[ dev_index ] = HSSLOT_ON ;
  __slot_plugin_time[ dev_index ] = llUptime ;
}




static void _slot_power_off(  uint8_t  dev_index )
{
  LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
  
  __aPwrStates[ dev_index ] = HSSLOT_OFF ;

  LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
  LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );

  hs_control_reg_ch1.bFetOnControl = 0 ;
  hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
  hs_control_reg_ch2.bGpioOutputState = 0 ;

  LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
  LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( dev_index ), __hs_slot_addresses[dev_index], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
}



// �������� ��� ����������, ������� �� �������� � ��������, ���� ���� ������
// �������� � ���������� �� �������� ������
//static void __slot_power_onoff()
//{
//	LT_LTC4222_StatusRegisterTypedef __hs_slot_statuses1, __hs_slot_statuses2 ;
//	LT_LTC4222_FaultAlertRegisterTypedef __hs_faults1, __hs_faults2 ;
//	u32 i ;
//	LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
//	LT_LTC4222_AdcControlRegisterTypedef tAdcControlRegisterStructure ;
//
//
//	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
//		// ���� ���������� �� ��������� -- ������ �� ������
//		if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] ){
//
//
//		// ���� ���������� �������� ������ -- ��������� ��� ����� ���������������
//		} else if( __slot_OnOff( i ) == PERMITTED_START ){
//				__aPwrStates[ i ] = HSSLOT_REGULAR_OFF ;
//				__fSlotPower[ i ] = 0 ;
//
//				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
//				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
//
//				hs_control_reg_ch1.bFetOnControl = 0 ;
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
//				hs_control_reg_ch2.bGpioOutputState = 0 ;
//
//				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
//				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
//
//		// ���� � ���������� �� �������� EEPROM � ����� ����� -- �� ��������
//		} else if( (__slot_OnOff( i ) == SMARTLY_START)  && 
//				!Dnepr_SlotEEPROM_Available(i) ){
//					__aPwrStates[ i ] = HSSLOT_OVERLIMIT_OFF ;
//					__fSlotPower[ i ] = 0 ;
//		}else if( (__aPwrStates[ i ] == HSSLOT_WAITING) ||
//			(__aPwrStates[ i ] == HSSLOT_OVERLIMIT_OFF) ){
//			// ���-�� ����������� � ������ ��������
//			if( ((f32)Dnepr_SlotEEPROM_SlotPower( i ) < __curPowerLimit()) ||
//				// ��� ������� ��������� ����
//				( __slot_OnOff( i ) == IGNORANT_START ) ){
//				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
//				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
//				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_PGOOD ;
//				hs_control_reg_ch2.bGpioOutputState = 1 ;
//				hs_control_reg_ch1.bFetOnControl = 1 ;
//				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
//				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
//				__aPwrStates[ i ] = HSSLOT_ON ;
//				__fSlotPower[ i ] = (f32)Dnepr_SlotEEPROM_SlotPower( i ) ;
//
//
//				// ������������� ������ �� ALERT ��� ���������� �������������� ���
//				LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );
//				tAdcControlRegisterStructure.bAdcAlert = FALSE ;
//				LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );
//
//				OSTimeDly( 200 );
//				// ���������� �����
//				LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2 );
//				LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1 );
//				// ��������� ������� ����� hotswap'�
//				LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &__hs_slot_statuses1, &__hs_faults1 );
//				LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &__hs_slot_statuses2, &__hs_faults2 );
//
//				// ��������� ����� �������� ��� ��������� ��������
//				__slot_plugin_time[ i ] = llUptime ;
//				__processed_slots_present.bSlotPresent[ i ] = 1 ;
//			} else {
//				__aPwrStates[ i ] = HSSLOT_OVERLIMIT_OFF ;
//				__processed_slots_present.bSlotPresent[ i ] = 0 ;
//			}
//		}
//	}
//}













void task_shelf_manager(void *pdata)
{
    enum _POWER_STATES *ptr_state;

    ptr_state = &curnt_state;
    
    pdata = pdata;
    while (TRUE) 
    {
      WORK_OUT_STATIC_FSM_TABLE(enum _POWER_STATES, &curnt_state, pwmng_get_current_signal, powermang_trans_table)
//        { 
//        						int                         cur_signal;
//                                                        transition_worker           worker; 
//                                                        
//							cur_signal = pwmng_get_current_signal();
//                                                        worker = powermang_trans_table[(int)*ptr_state][cur_signal].worker; 
//                                                        if (worker != NULL) { 
//                                                            worker((int)*ptr_state, cur_signal); 
//                                                        } 
//                                                        *ptr_state = (enum _POWER_STATES)powermang_trans_table[(int)*ptr_state][cur_signal].new_state; 
//                                                      }        
        
    }  
}





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ������������ ���������, ���������� �� ������ ������� � ����������

//void Dnepr_DControl_ResetMaxPower()
//{
//  	msg_inttask_t             msg;
//        
//        msg.msg = PSU_VALUES;
//	msg.data = NULL;
//	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
//}


void topwmng_msg_profile_change ( void )
{
  msg_inttask_t             msg;
        
  msg.msg = PROF_POWER_RECALC;
  msg.data = NULL;
  send_inttask_message(POWER_MANAGMENT, ANY_MODULE,  &msg);  
}

void topwmng_msg_crate_change ( void )
{
  msg_inttask_t             msg;
        
  msg.msg = SHELF_POWER_RECALC;
  msg.data = NULL;
  send_inttask_message(POWER_MANAGMENT, ANY_MODULE,  &msg);  
}



// ������ � ���, ��� � ������� ���������� ���������� ������� �����
//void Dnepr_DControl_RereadPowerOnOff( const u32 slot_num )
//{
//  	msg_inttask_t             msg;
//        
//        msg.msg = POWER_RECALC;
//	msg.data = NULL;
//	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
//}

// ������ � ������������� ��������� ����������� ��������� ���������,
// ��������, ���� ���������� ����� �� �������
//void Dnepr_DControl_ReinitHotswaps()
//{
//  	msg_inttask_t             msg;
//        
//        msg.msg = INIT_HOTSWAPS;
//	msg.data = NULL;
//	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
//}

// ������ � ������������� ����������� �����������
void Dnepr_DControl_SetFans()
{
  	msg_inttask_t             msg;
        
        msg.msg = FAN_SETTINGS_CHANGED;
	msg.data = NULL;
	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
}

//f32 Dnepr_DControl_PowerReserv()
//{
//	f32 lim = __curPowerLimit() ;
//	if( lim > (FLT_MAX - 1000) ){
//		return 0 ;
//	} else {
//		return lim ;
//	}
//}

//SlotPowerState_t Dnepr_DControl_PowerStatus( const u32 slot_num )
//{
//	if( slot_num >= I2C_DNEPR_NUMBER_OF_SLOTS ){
//		return HSSLOT_UNAVAILABLE ;
//	}
//	return __aPwrStates[ slot_num ] ;
//}

//void Dnepr_DControl_SetPowerLimitSource( const _BOOL psu_source, const f32 limit )
//{
//	__bPSULimitSource = psu_source ;
//	if( !__bPSULimitSource ){
//		// �������� ����������� -- �������
//		if( limit > 1 ){
//			__fPowerLimit = limit ;
//		} else {
//			__fPowerLimit = FLT_MAX ;
//		}
//
//		// �������� ����������
//		Dnepr_DControl_ReinitHotswaps() ;
//	// �������� �������� �� ��
//	} else {
//		Dnepr_DControl_ResetMaxPower() ;
//	}
//}

I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotPresent()
{
	return &__processed_slots_present ;
}

I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotRawPresent()
{
	return I2C_DNEPR_GetPresentDevices() ;
}


void Dnepr_DControl_ReinitPowerSource(void)
{
  	msg_inttask_t             msg;
        
        msg.msg = PSU_VALUES;
	msg.data = NULL;
	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
}


size_t Dnepr_DControl_NextPMBusAlarm( s8* sResult, const size_t wResultLen )
{
	INT8U return_code = OS_ERR_NONE;
	long long tick ;
	_BOOL known_alarm = FALSE ;
	PMBAlarmMess_t* pLastMess = (PMBAlarmMess_t*)OSQAccept( __qPMBAlarms, &return_code );
#define wBuffLen	 64
	s8 sBuff[ wBuffLen ];
	size_t icurResult = 0 ;
	_BOOL notOneAlarm = FALSE ;
	assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_Q_EMPTY) );

	// ���� ��� ��������� �� �������
	if( !pLastMess ){
		return 0 ;
	}

	// ������� ����� ����� � ������ ������ � ��������, 
	tick = (u32)round( ((f32)pLastMess->cur_tick)/(f32)OS_TICKS_PER_SEC );
	sBuff[ 0 ] = '\0' ;
	if( pLastMess->slot_num < I2C_DNEPR_FAN_SLOT_NUM ){
		sprintf( sBuff, "%lli:%d:", tick, pLastMess->slot_num+1 );
	} else if( pLastMess->slot_num == I2C_DNEPR_FAN_SLOT_NUM ) {
		sprintf( sBuff, "%lli:FAN:", tick );
	}
	strncat( sResult, sBuff, wResultLen );
	icurResult += strlen( sBuff );
	icurResult = MIN( icurResult, wResultLen );

	sBuff[ 0 ] = '\0' ;

	#define DCONTROL_ADD_ALARM_DESC( alarm_enum, text )	if( (pLastMess->alarm & alarm_enum) != 0 ){ \
															known_alarm = TRUE ;					\
															if( notOneAlarm ){						\
																strncat( sBuff, "&", wBuffLen );	\
															} else {								\
																notOneAlarm = TRUE ;				\
															}										\
															strncat( sBuff, text, wBuffLen );		\
														}											\

	if( (pLastMess->alarm & CH1_FETSHORT) != 0 ){
		known_alarm = TRUE ;
		notOneAlarm = TRUE ;
		strncat( sBuff, "C1FS", wBuffLen );
	}
	DCONTROL_ADD_ALARM_DESC( CH1_POWERBAD, "C1PB" );
	DCONTROL_ADD_ALARM_DESC( CH1_OVERCURRENT, "C1OC" );
	DCONTROL_ADD_ALARM_DESC( CH1_UNDERVOLTAGE, "C1UV" );
	DCONTROL_ADD_ALARM_DESC( CH1_OVERVOLTAGE, "C1OV" );
	DCONTROL_ADD_ALARM_DESC( CH2_FETSHORT, "C2FS" );
	DCONTROL_ADD_ALARM_DESC( CH2_POWERBAD, "C2PB" );
	DCONTROL_ADD_ALARM_DESC( CH2_OVERCURRENT, "C2OC" );
	DCONTROL_ADD_ALARM_DESC( CH2_UNDERVOLTAGE, "C2UV" );
	DCONTROL_ADD_ALARM_DESC( CH2_OVERVOLTAGE, "C2OV" );

	DCONTROL_ADD_ALARM_DESC( FAN_COMM_FAULT, "FCOM" );
	DCONTROL_ADD_ALARM_DESC( FAN_COMM_FAULT, "FDAT" );
	DCONTROL_ADD_ALARM_DESC( FAN_LOG_FULL_FAULT, "FLOGF" );
	DCONTROL_ADD_ALARM_DESC( FAN_WATCHDOG, "FWTCH" );
	DCONTROL_ADD_ALARM_DESC( FAN_OT_WARN, "FOVW" );
	DCONTROL_ADD_ALARM_DESC( FAN_OT_FAULT, "FOT" );
	DCONTROL_ADD_ALARM_DESC( FAN_VOUT_OV, "FVOV" );
	DCONTROL_ADD_ALARM_DESC( FAN_TEMP, "FTEMP" );
	DCONTROL_ADD_ALARM_DESC( FAN_CML, "FCML" );
	DCONTROL_ADD_ALARM_DESC( FAN_VOUT, "FV" );
	DCONTROL_ADD_ALARM_DESC( FAN_FANS, "FFANS" );

	if( !known_alarm ){
		sprintf( sBuff, "unknown=%i", (u32)pLastMess->alarm );
	}
	strncat( sBuff, "|", wBuffLen );

	strncat( &sResult[icurResult], sBuff, wResultLen-icurResult );
	icurResult += strlen( sBuff );
	icurResult = MIN( icurResult, wResultLen );

	return icurResult ;
#undef wBuffLen	 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ���������� �� ����������

//static const task_message_t __pmbusalarmmess = { PMBUS_ALARM, 0 };
void Dnepr_DControl_PMBusAlarm()
{
  	msg_inttask_t             msg;
        
        msg.msg = PMBUS_ALARM;
	msg.data = NULL;
	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
}

static long long __pres_uptime = 0 ;
void Dnepr_DControl_Present_Interrupt()
{
	u32                       diff ;
  	msg_inttask_t             msg;
        
	if( llUptime ){
		if( llUptime >= __pres_uptime ){
			diff = llUptime - __pres_uptime ;
		} else {
			diff = llUptime ;
		}
		if( diff >= __present_react_timeout ){
			__pres_uptime = __present_react_timeout ;
                        msg.msg = PRESENT_INTERRUPT;
                  	msg.data = NULL;
                  	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
		}
	}
}

void Dnepr_DControl_SFP_Interrupt()
{
  	msg_inttask_t             msg;
        
        msg.msg = SFP_INTERRUPT;
	msg.data = NULL;
	send_inttask_message(DEVICE_CONTROLLER, ANY_MODULE,  &msg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PMBAlarmMess_t __alarm_mess[ __pmbalarms_arr_len ] ;
static size_t __curAlarmMess_ind = 0 ;

// ������� �� ����� �� pmbus
static void __pmbus_alarm_handler()
{
	LT_LTC4222_StatusRegisterTypedef __hs_slot_statuses1, __hs_slot_statuses2 ;
	LT_LTC4222_FaultAlertRegisterTypedef __hs_faults1, __hs_faults2 ;
#define max_pmbus_addr_cnt 16
	u8 pmbus_addr_cnt = 0 ; 	// ���-�� ������� ���������� �� ARA
	u8 pmbus_addr[ max_pmbus_addr_cnt ] ;	// ������ �� ���� pmbus, ���������� �� ARA
	size_t i, j ;

	// ���� ��������� ��������� ��������� alarm ������������ (�������� ���
	// �������� ���������� ������������ ��������), ������� ��� � �����
	while( Dnepr_EdgePort_is_IRQ4_active() && (pmbus_addr_cnt < max_pmbus_addr_cnt) ){
		// �������� ��� ������
		pmbus_addr[ pmbus_addr_cnt++ ] = Dnepr_I2C_Read_ARA() ;
		// ���������� �������� ���� ���������� ��� �������
	}


	for( j = 0; j < pmbus_addr_cnt; j++ ){
		for( i = 0;
				(pmbus_addr[ j ] != __hs_slot_addresses[i]) && (i < I2C_DNEPR_NUMBER_OF_SLOTS+1); i++ );
		// ����������� ����� -- �������
		if( i >= ( I2C_DNEPR_NUMBER_OF_SLOTS + 1 ))
			return ;

		// hot swap ��������� ���������� ��� ����� ����������
		if( i <= FAN_HS_ADDR_NUM){
			// ���� �� ������ ������� -- �� ���������
			if( __slot_plugin_time[ i ] + SLOT_PLUGIN_TIMEOUT > llUptime ){
				return ;
			}
			// ������������ ��� ������ � ������
			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL1,
										&__hs_slot_statuses1, &__hs_faults1 );
			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL2,
										&__hs_slot_statuses2, &__hs_faults2 );
			// ���������� �����
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL1 );
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL2 );

			// ��������� ��������� ���������
			__alarm_mess[ __curAlarmMess_ind ].slot_num = i ;
			__alarm_mess[ __curAlarmMess_ind ].cur_tick = llUptime ;
			__alarm_mess[ __curAlarmMess_ind ].alarm = NONE_ALARM ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults1.bFetShort ? CH1_FETSHORT : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults1.bPowerBad ? CH1_POWERBAD : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults1.bOverCurrent ? CH1_OVERCURRENT : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults1.bUnderVoltage ? CH1_UNDERVOLTAGE : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults1.bOverVoltage ? CH1_OVERVOLTAGE : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults2.bFetShort ? CH2_FETSHORT : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults2.bPowerBad ? CH2_POWERBAD : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults2.bOverCurrent ? CH2_OVERCURRENT : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults2.bUnderVoltage ? CH2_UNDERVOLTAGE : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |=
					__hs_faults2.bOverVoltage ? CH2_OVERVOLTAGE : 0 ;
			if( __qPMBAlarms ){
				// ����� � ������ ������� ��������� �� ������ � �� �������� �������
				// ��� �������������
				OSQPostOpt( __qPMBAlarms, (void*)&__alarm_mess[ __curAlarmMess_ind ],
							OS_POST_OPT_FRONT | OS_POST_OPT_NO_SCHED ) ;
				if( ++__curAlarmMess_ind >= __pmbalarms_arr_len ){
					__curAlarmMess_ind = 0 ;
				}
			}
		// ���������� ������������
		} else if( i ==  FAN_CONTROLLER_ADDR_NUM ){
			u8 cStatus, cmlStatus, mfrSpec ;
			u16 wStatus ;
			_BOOL pmb_error = TRUE ;

			pmb_error &= MAX31785_GetStatusByte( &cStatus );
			pmb_error &= MAX31785_GetStatusWord( &wStatus );
			pmb_error &= MAX31785_GetStatusCML( &cmlStatus );
			pmb_error &= MAX31785_GetMfrSpecific( &mfrSpec );

			__alarm_mess[ __curAlarmMess_ind ].slot_num = I2C_DNEPR_FAN_SLOT_NUM ;
			__alarm_mess[ __curAlarmMess_ind ].cur_tick = llUptime ;
			__alarm_mess[ __curAlarmMess_ind ].alarm = NONE_ALARM ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= cmlStatus & MAX31785_CML_COMM_FAULT ?
														FAN_COMM_FAULT : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= cmlStatus & MAX31785_CML_DATA_FAULT ?
														FAN_DATA_FAULT : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= cmlStatus & MAX31785_CML_FAULT_LOG_FULL ?
														FAN_LOG_FULL_FAULT : 0 ;

			__alarm_mess[ __curAlarmMess_ind ].alarm |= mfrSpec & MAX31785_SPECIFIC_WATCHDOG ?
														FAN_WATCHDOG : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= mfrSpec & MAX31785_SPECIFIC_OT_WARN ?
														FAN_OT_WARN : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= mfrSpec & MAX31785_SPECIFIC_OT_FAULT ?
														FAN_OT_FAULT : 0 ;

			__alarm_mess[ __curAlarmMess_ind ].alarm |= cStatus & MAX31785_SB_VOUT_OV ?
														FAN_VOUT_OV : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= cStatus & MAX31785_SB_TEMPERATURE ?
														FAN_TEMP : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= cStatus & MAX31785_SB_CML ?
														FAN_CML : 0 ;

			__alarm_mess[ __curAlarmMess_ind ].alarm |= wStatus & MAX31785_SW_VOUT ?
														FAN_VOUT : 0 ;
			__alarm_mess[ __curAlarmMess_ind ].alarm |= wStatus & MAX31785_SW_FANS ?
														FAN_FANS : 0 ;

			__MAX31785_ClearFaults();
			if( __qPMBAlarms ){
				// ����� � ������ ������� ��������� �� ������ � �� �������� �������
				// ��� �������������
				OSQPostOpt( __qPMBAlarms, (void*)&__alarm_mess[ __curAlarmMess_ind ],
							OS_POST_OPT_FRONT | OS_POST_OPT_NO_SCHED ) ;
				if( ++__curAlarmMess_ind >= __pmbalarms_arr_len ){
					__curAlarmMess_ind = 0 ;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////








////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ����������� � ���������� ������� ��������� ����� �����������
static f32 __curPowerLimit()
{
	size_t i ;
	f32 pwr_lim = __fPowerLimit ;
	if( pwr_lim < 0.1 )
		return pwr_lim ;
	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
		pwr_lim -= __fSlotPower[ i ];
	}
	return pwr_lim ;
}

f32 Dnepr_DControl_MaxPower()
{
	size_t i ;
	f32 cur_max = 0 ;
	
	// �������� ��� �������� ���������� ����� ����� ������������.
	for( i = 0; i < I2C_DNEPR_FAN_SLOT_NUM; i++ ){
		if( cur_max < __fSlotPower[ i ] ){
			cur_max = __fSlotPower[ i ] ;
		}
	}
	return cur_max ;	
}

extern u32 val_Slot1Enable ;
extern u32 val_Slot2Enable ;
extern u32 val_Slot3Enable ;
extern u32 val_Slot4Enable ;
extern u32 val_Slot5Enable ;
extern u32 val_Slot6Enable ;
extern u32 val_Slot7Enable ;
extern u32 val_Slot8Enable ;
extern u32 val_Slot9Enable ;
extern u32 val_Slot10Enable ;
extern u32 val_Slot11Enable ;
extern u32 val_Slot12Enable ;
extern u32 val_Slot13Enable ;

// ���������� ������� ��� ��� ���� ������ �� ���������� �������
static SLOT_ONOFF_TYPE __slot_OnOff( const u32 slot_num )
{
	if( slot_num >= I2C_DNEPR_NUMBER_OF_SLOTS){
		return SMARTLY_START ;
	}
	switch(slot_num){
		case 0 :
		return (SLOT_ONOFF_TYPE)val_Slot1Enable ;
		case 1 :
		return (SLOT_ONOFF_TYPE)val_Slot2Enable ;
		case 2 :
		return (SLOT_ONOFF_TYPE)val_Slot3Enable ;
		case 3 :
		return (SLOT_ONOFF_TYPE)val_Slot4Enable ;
		case 4 :
		return (SLOT_ONOFF_TYPE)val_Slot5Enable ;
		case 5 :
		return (SLOT_ONOFF_TYPE)val_Slot6Enable ;
		case 6 :
		return (SLOT_ONOFF_TYPE)val_Slot7Enable ;
		case 7 :
		return (SLOT_ONOFF_TYPE)val_Slot8Enable ;
		case 8 :
		return (SLOT_ONOFF_TYPE)val_Slot9Enable ;
		case 9 :
		return (SLOT_ONOFF_TYPE)val_Slot10Enable ;
		case 10 :
		return (SLOT_ONOFF_TYPE)val_Slot11Enable ;
		case 11 :
		return (SLOT_ONOFF_TYPE)val_Slot12Enable ;
		case 12 :
		return (SLOT_ONOFF_TYPE)val_Slot13Enable ;
		case I2C_DNEPR_FAN_SLOT_NUM :
		return IGNORANT_START ;
	}
	return SMARTLY_START ;
}

void Dnepr_DControl_sfp_process_present()
{
	// ��������� ������� sfp
 	Dnepr_Refresh_SFP_Presents() ;

	__processed_slots_present.bSfpPresent[ 0 ] =  I2C_DNEPR_GetPresentDevices()->bSfpPresent[ 0 ];
	__processed_slots_present.bSfpPresent[ 1 ] =  I2C_DNEPR_GetPresentDevices()->bSfpPresent[ 1 ];
}


void Dnepr_DControl_fun_process_present(void)
{
    Dnepr_Refresh_Presents();
    
    __processed_slots_present.bSlotPresent[ 13 ] =  I2C_DNEPR_GetPresentDevices()->bSlotPresent[ 13 ];
}


extern u32 val_FUFanMode ;
extern u32 val_FUFanThreshold75 ;
extern u32 val_FUFanThreshold90 ;
extern u32 val_FUFanSpeedSet ;

static void __set_fans()
{
        u32 need_fan_power ;
	// ���� �������������� ����� ���������� �������������.
	if( !val_FUFanMode ){
		u32 m = (u32)Dnepr_DControl_MaxPower() ;
		if( m <= val_FUFanThreshold75 ){
			need_fan_power = 50 ;
		} else if( m <= val_FUFanThreshold90 ){
			need_fan_power = 75 ;
		} else {
			need_fan_power = 90 ;
		}
	} else {
		need_fan_power = val_FUFanSpeedSet ;
	}
	Dnepr_Measure_UpdateFansRPM( need_fan_power );
}
