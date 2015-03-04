/*!
\file threadDeviceController.c
\brief поток управляет включением устройств, поддерживает актуальной информацию о present'ах и реагирует на ALARM PMBus'а
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/

#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_EdgePort.h"
#include "HAL/IC/inc/AT_AT24C512.h"
#include "T8_Atomiccode.h"
#include "Threads/inc/threadDeviceController.h"
#include "Threads/inc/threadMeasure.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
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

// адреса hotswap'ов слотов +1 платы вентиляции + контроллер вентиляторов
static const u8 __hs_slot_addresses[ I2C_DNEPR_NUMBER_OF_SLOTS + 1 ] = 
{ 0x9C, 0xCA, 0xBC, 0xDC, 0x8E, 0xBA, 0xAE, 0xCE, 0x9E, 0xDA, 0xBE, 0xDE, 0xE8, 0x8C, 0xA4 };
#define FAN_CONTROLLER_ADDR_NUM	(I2C_DNEPR_NUMBER_OF_SLOTS) // позиция адреса контоллера вентиляторов
#define FAN_HS_ADDR_NUM			(I2C_DNEPR_NUMBER_OF_SLOTS-1) // позиция адреса хотсвопа платы вентиляции

//! при втыкании устройства здесь пишется время втыкания (llUptime), чтобы выждать 
// таймаут перед тем как начать регистрировать ошибки hotswapa
static long long __slot_plugin_time[ I2C_DNEPR_NUMBER_OF_SLOTS ];
#define PMBUS_ALARM_TIMEOUT		2500 //!< таймаут на реагигорвание на PMBUS_alarm
#define SLOT_PLUGIN_TIMEOUT		7000 //!< таймаут включения в тиках ОС, должен быть больше PMBUS_ALARM_TIMEOUT

static long long __last_pmbus_alarm = 0 ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// внутренние данные о состоянии слотовых устройств

// состояние слотового устройства в соответствии с алгоритмом включения
static SlotPowerState_t __aPwrStates[ I2C_DNEPR_NUMBER_OF_SLOTS ];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! тип сообщений посылаемых в taskDeviceController
typedef enum mess_type_ {
	PMBUS_ALARM,				//!< alarm на pmbus
	

	PRESENT_INTERRUPT,			//!< прерывание от плис
	SFP_INTERRUPT, 				//!< прерывание от SFP

	PSU_VALUES,					//!< измерили мощности блоков питания
	INIT_HOTSWAPS,				//!< включить все устройства, которые можно
	REINIT_ONOFF,				//!< включить слотовые устройства, которым можно включиться
	POWER_ALLOW,				//!< надо включить или выключить устройство
	FAN_SETTINGS_CHANGED		//!< Изменили параметры автоматического управления вентиляторами.
} mess_type_t ;

//! тип сообщений для очереди потока
typedef struct task_message_ {
	mess_type_t message_type ; //! тип команд

	u32	slot_num_2_write ; //! в какой слот пишем настройки eeprom
} task_message_t ;

#define task_messages_len 24 	// длина очереди сообщений потока, должна уместить

//! Презенты устройств, которые успели обработать.
static I2C_DNEPR_PresentDevicesTypedef __processed_slots_present ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static OS_EVENT  *__qRcvQueue = 0;

static f32 __fPowerLimit1, __fPowerLimit2 ;	// мощность блоков питания
static f32 __fPowerLimit ; // сколько всего мощности можно потребить
static f32 __fSlotPower[ I2C_DNEPR_NUMBER_OF_SLOTS ]; //!< сколько мощности потребляет каждый слот
extern u32 val_VPowerLimitSource;
static _BOOL __bPSULimitSource = TRUE ; //!< TRUE -- берём предел мощности из БП


static void __pmbus_alarm_handler() ;
static void __slot_power_dev_plug( const u8 slot_num, const _BOOL pluginout );
void __slot_power_onoff_init();
//static void __slot_power_onoff_init();
static void __slot_power_onoff();

static f32 __curPowerLimit();

//! Значения параметра SlotOnOff
typedef enum {
	PERMITTED_START = 0,	//!< запуск слота запрещён
	SMARTLY_START = 1,		//!< запуск происходит в обычном режиме (д.б. EEPROM
							//! в котором должна быть написана мощность, которой 
							//! должно хватать)
	IGNORANT_START = 2 		//!< запуск произойдёт в любом случае
} SLOT_ONOFF_TYPE ;
static SLOT_ONOFF_TYPE __slot_OnOff( const u32 slot_num );

static const u32 __present_react_timeout = 100 ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Типы регистрируемых алармов по PMBus
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

	// аварии контроллера вентиляции
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

//! тип сообщения с описанием аларма
typedef struct {
	u8 slot_num ; // номер слота, начиная с нуля, 13 -- плата вентиляции
	long long cur_tick ;	// текущий тик ОС
	AlarmType_t alarm ; // тип аварии
} PMBAlarmMess_t ;

#define __pmbalarms_arr_len 32  //!< длина очереди с алармами
static void* __pmbalarms_arr[ __pmbalarms_arr_len ] ; //!< очередь с алармами
//! очередь с описаниями алармов, полученных по PMBus
static OS_EVENT  *__qPMBAlarms = 0;

static void __set_fans() ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DeviceController_Init(void)
{
	u32 i ;
	// берём из профиля источник предельного значение потребляемой мощности
	__bPSULimitSource = (val_VPowerLimitSource == 0) ; // TRUE -- PSU, иначе значение из профиля

	I2C_DNEPR_ClearPresentDevices( &__processed_slots_present );
	// мощности в каждом слоте
	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
		__fSlotPower[ i ] = 0. ;
		__slot_plugin_time[ i ] = llUptime ; // по умолчанию ставится время включения Днепра
 	}

 	Dnepr_SlotEEPROM_Init() ;

 	// читаем все EEPROM и HotSwap Controller'ы и задерживаем включение тех, которые
 	// ещё не включились
 	//__slot_power_onoff_init() ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void taskDeviceController(void *pdata)
{
	INT8U return_code = OS_ERR_NONE;
	u32 i ;
	I2C_DNEPR_PresentDevicesTypedef		prev_presents ;
	T8_Dnepr_PsStatusTypedef cur_psu_status ;
	T8_Dnepr_PsStatusTypedef prev_psu_status ;
	void *messages_array[task_messages_len] ; // для очереди сообщений
	task_message_t *qCurMessage ;
	const PSU_UnitInfoTypedef* pPSUInfo1 = NULL ;
	const PSU_UnitInfoTypedef* pPSUInfo2 = NULL ;
	
	pdata = pdata;
	// очередь сообщений для потока
	__qRcvQueue = OSQCreate( messages_array, task_messages_len ) ;
	
	// очередь описаний алармов
	__qPMBAlarms = OSQCreate( __pmbalarms_arr, __pmbalarms_arr_len ) ;

	Dnepr_Refresh_Presents() ; // перечитываем презенты и 
	// дискретные выводы ПЛИС
	for( i = 0; i < 3; i++ ){
		if( Dnepr_Reload_PSU_Status_Pins() ){
			break ;
		}
	}
	memcpy( &cur_psu_status, Dnepr_Backplane_GetPSU_Status(), sizeof(T8_Dnepr_PsStatusTypedef) );
	memcpy( &prev_psu_status, &cur_psu_status, sizeof(T8_Dnepr_PsStatusTypedef) );
	Dnepr_DControl_ResetMaxPower(); // сразу при включении обновляем статусы питания
	
	__last_pmbus_alarm = llUptime ;        

	while(TRUE){
          qCurMessage = (task_message_t*)OSQPend( __qRcvQueue, PMBUS_ALARM_TIMEOUT, &return_code );
          assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_TIMEOUT) );
          // если прошел таймаут и необходимо перечитать PSU -- посылаем сами себе команду
          if( return_code == OS_ERR_TIMEOUT ){
        	// раз в 2,5 секунд реагируем на alarm
          	if( Dnepr_EdgePort_is_IRQ4_active() &&
        		((llUptime - __last_pmbus_alarm) > PMBUS_ALARM_TIMEOUT) ){
        		__last_pmbus_alarm = llUptime ;
        		__pmbus_alarm_handler() ;
        	}
        	continue ;
        } /* while(TRUE) */

        switch( qCurMessage->message_type ){
        	case PSU_VALUES:
        		// перечитываем их статические параметры
				Dnepr_Backplane_Reload_PSU_Info() ;
				pPSUInfo1 = Dnepr_Backplane_GetPSU_Info( 0 );
				pPSUInfo2 = Dnepr_Backplane_GetPSU_Info( 1 );
				__fPowerLimit1 = pPSUInfo1 ? pPSUInfo1->fPower : 0.0 ;
				__fPowerLimit2 = pPSUInfo2 ? pPSUInfo2->fPower : 0.0 ;

                              // если не удаётся прочитать мощность -- перечитываем через паузу                                
                                /* если максимальная мощьность не считывается то лимиты делаем максимальными !! */
				if( (__fPowerLimit1 < 10.) || (__fPowerLimit1 > 3000.) ||
					(!Dnepr_Backplane_GetPSU_Status()->tPs1.bPowerGood) ){
					__fPowerLimit1 = FLT_MAX ;
				}
                		if( (__fPowerLimit2 < 10.) || (__fPowerLimit2 > 3000.) ||
        			(!Dnepr_Backplane_GetPSU_Status()->tPs2.bPowerGood) ){
					__fPowerLimit2= FLT_MAX ;
				}

				// обновляем светодиоды POWER
				// если нет одного из БП -- горим желтым
				if( !Dnepr_Backplane_PS1_Present() || !Dnepr_Backplane_PS2_Present() ){
					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){YELLOW, FALSE} );
				// если у хотя бы одного БП не PowerGood -- мигаем желтым
				} else if( 	(!Dnepr_Backplane_GetPSU_Status()->tPs1.bPowerGood	) ||
							(!Dnepr_Backplane_GetPSU_Status()->tPs2.bPowerGood	) ){
					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){YELLOW, TRUE} );
				// всё хорошо
				} else {
					Dnepr_Measure_SetPowerLed( (T8_Dnepr_LedTypedef){GREEN, FALSE} );
				}
        		
        		// обновляем оставшийся резерв мощности
        		if( __bPSULimitSource == TRUE ){
        			__fPowerLimit = MIN( __fPowerLimit1, __fPowerLimit2 );
				// источник ограничений -- профиль
        		} else {
        			if( val_VPowerLimit > 1 ){
						__fPowerLimit = val_VPowerLimit ;
					} else {
						__fPowerLimit = FLT_MAX ;
					}
				}

				// Обновляем информацию о вставленных SFP
				Dnepr_Refresh_SFP_Presents() ;
				// производим включение задержанных при загрузке (DeviceController_Init())
	        	__slot_power_onoff();
	        	// Настраиваем вентиляторы.
	        	__set_fans() ;
        	break ;
                
        	// вызывается при необходимости провести включение
        	case INIT_HOTSWAPS:
	        	__slot_power_onoff();
	        	__set_fans() ;
        	break ;
                
        	// вызывается при необходимости включить устройства, которые не были включены
        	// из-за недостатка мощности
        	case REINIT_ONOFF:
        		__slot_power_onoff_init() ;
        		__set_fans() ;
        	break ;
                
        	// прерывание от PMBus alarm
        	// если в последние PMBUS_ALARM_TIMEOUT, его не было -- реагируем.
        	case PMBUS_ALARM :
        		if( llUptime > __last_pmbus_alarm ){
        			if( (llUptime - __last_pmbus_alarm) > PMBUS_ALARM_TIMEOUT ){
        				__last_pmbus_alarm = llUptime ;
			        	__pmbus_alarm_handler() ;
        			}
        		// если было переполнение llUptime
        		// реагируем на PMBus_alarm
        		} else {
        			__last_pmbus_alarm = llUptime ;
        			__pmbus_alarm_handler() ;
        		}
        	break ;
                
        	case PRESENT_INTERRUPT :
				// берём предыдущие present'ы
				memcpy( &prev_presents, I2C_DNEPR_GetPresentDevices(), sizeof(I2C_DNEPR_PresentDevicesTypedef) );
				OSTimeDly( __present_react_timeout );
				// перечитываем презенты из ПЛИС
				Dnepr_Refresh_Presents() ;

	        	// ищем различия
				for(i=0; i<I2C_DNEPR_NUMBER_OF_SLOTS; i++){
					if( I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] != 
										prev_presents.bSlotPresent[i]){
		        		// проводим процедуру включения или фиксируем вынимание
		        		__slot_power_dev_plug( i, I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] );
		        		// Переключаем вентиляторы.
			        	__set_fans() ;
					}
				}
				for( i = 0; i < 3; i++ ){
					if( Dnepr_Reload_PSU_Status_Pins() ){
						break ;
					}
				}
				// не удалось прочитать значения статусных выводов БП
				if( i >= 3 ){
					break ;
				}
				memcpy( &cur_psu_status, Dnepr_Backplane_GetPSU_Status(), sizeof(T8_Dnepr_PsStatusTypedef) );
				if( (cur_psu_status.tPs1.bSeated != prev_psu_status.tPs1.bSeated) ||
					(cur_psu_status.tPs2.bSeated != prev_psu_status.tPs2.bSeated) ||
					(cur_psu_status.tPs1.bPowerGood != prev_psu_status.tPs1.bPowerGood) ||
					(cur_psu_status.tPs2.bPowerGood != prev_psu_status.tPs2.bPowerGood) ){
					// значит вставили/вынули БП
					Dnepr_DControl_ResetMaxPower();
				}
				memcpy( &prev_psu_status, &cur_psu_status, sizeof(T8_Dnepr_PsStatusTypedef) );
        	break ;
                
        	case SFP_INTERRUPT :
		    	Dnepr_Refresh_SFP_Presents() ;
        	break ;
                
        	// в профиле поменялось разрешение запуска слота
        	case POWER_ALLOW :
        		if( I2C_DNEPR_GetPresentDevices()->bSlotPresent[qCurMessage->slot_num_2_write] ){
	        		__slot_power_dev_plug( qCurMessage->slot_num_2_write, TRUE );
	        	}
        	break ;
                
        	// В профиле поменяли настройки регулированием вентиляторов.
        	case FAN_SETTINGS_CHANGED :
        		__set_fans() ;
        	break ;
        }
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// интерфейсные процедуры, вызываются из других потоков и прерываний

static task_message_t __power_mess = { PSU_VALUES, 0 };
void Dnepr_DControl_ResetMaxPower()
{
	if( __qRcvQueue  ){
		OSQPost( __qRcvQueue, (void*)&__power_mess ) ;
	}
}

// сигнал о том, что в профиле поменялось разрешение запуска слота
static task_message_t __onoffallow_mess = { POWER_ALLOW, 0 };
void Dnepr_DControl_RereadPowerOnOff( const u32 slot_num )
{
	__onoffallow_mess.slot_num_2_write = slot_num ;
	if( __qRcvQueue  ){
		OSQPost( __qRcvQueue, (void*)&__onoffallow_mess ) ;
	}	
}

// сигнал о необходимости проверить возможность включения устройств,
// например, если увеличился запас по питанию
static task_message_t __reiniths_mess = { INIT_HOTSWAPS, 0 };
void Dnepr_DControl_ReinitHotswaps()
{
	if( __qRcvQueue  ){
		OSQPost( __qRcvQueue, (void*)&__reiniths_mess ) ;
	}	
}

// сигнал о необходимости пересчитать вентиляторы
static task_message_t __fan_mess = { FAN_SETTINGS_CHANGED, 0 };
void Dnepr_DControl_SetFans()
{
	if( __qRcvQueue  ){
		OSQPost( __qRcvQueue, (void*)&__fan_mess ) ;
	}	
}

f32 Dnepr_DControl_PowerReserv()
{
	f32 lim = __curPowerLimit() ;
	if( lim > (FLT_MAX - 1000) ){
		return 0 ;
	} else {
		return lim ;
	}
}

SlotPowerState_t Dnepr_DControl_PowerStatus( const u32 slot_num )
{
	if( slot_num >= I2C_DNEPR_NUMBER_OF_SLOTS ){
		return HSSLOT_UNAVAILABLE ;
	}
	return __aPwrStates[ slot_num ] ;
}

void Dnepr_DControl_SetPowerLimitSource( const _BOOL psu_source, const f32 limit )
{
	__bPSULimitSource = psu_source ;
	if( !__bPSULimitSource ){
		// источник ограничений -- профиль
		if( limit > 1 ){
			__fPowerLimit = limit ;
		} else {
			__fPowerLimit = FLT_MAX ;
		}

		// включаем устройства
		Dnepr_DControl_ReinitHotswaps() ;
	// забираем мощность из бп
	} else {
		Dnepr_DControl_ResetMaxPower() ;
	}
}

I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotPresent()
{
	return &__processed_slots_present ;
}

I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotRawPresent()
{
	return I2C_DNEPR_GetPresentDevices() ;
}

void Dnepr_DControl_ReinitPowerLimitSource()
{
	__bPSULimitSource = (val_VPowerLimitSource == 0) ; // TRUE -- PSU, иначе значение из профиля
	Dnepr_DControl_SetPowerLimitSource( __bPSULimitSource, val_VPowerLimit );
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

	// если нет сообщений об алармах
	if( !pLastMess ){
		return 0 ;
	}

	// сначала пишем время с начала работы в секундах, 
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
// вызываются из прерываний

static const task_message_t __pmbusalarmmess = { PMBUS_ALARM, 0 };
void Dnepr_DControl_PMBusAlarm()
{
	if( __qRcvQueue  ){
//		OSQPost( __qRcvQueue, (void*)&__pmbusalarmmess ) ;
                OSQPostFront( __qRcvQueue, (void*)&__pmbusalarmmess ) ;                
	}
}

static const task_message_t __fpgamess = { PRESENT_INTERRUPT, 0 };
static long long __pres_uptime = 0 ;
void Dnepr_DControl_Present_Interrupt()
{
	u32 diff ;
	if( __qRcvQueue && (llUptime ) ){
		if( llUptime >= __pres_uptime ){
			diff = llUptime - __pres_uptime ;
		} else {
			diff = llUptime ;
		}
		if( diff >= __present_react_timeout ){
			__pres_uptime = __present_react_timeout ;
//			OSQPost( __qRcvQueue, (void*)&__fpgamess ) ;
                        OSQPostFront( __qRcvQueue, (void*)&__fpgamess ) ;                
                        
		}
	}
}

static const task_message_t __sfpmess = { SFP_INTERRUPT, 0 };
void Dnepr_DControl_SFP_Interrupt()
{
	if( __qRcvQueue  ){
//		OSQPost( __qRcvQueue, (void*)&__sfpmess ) ;
                OSQPostFront( __qRcvQueue, (void*)&__sfpmess ) ;                
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PMBAlarmMess_t __alarm_mess[ __pmbalarms_arr_len ] ;
static size_t __curAlarmMess_ind = 0 ;

// реакция на аларм по pmbus
static void __pmbus_alarm_handler()
{
	LT_LTC4222_StatusRegisterTypedef __hs_slot_statuses1, __hs_slot_statuses2 ;
	LT_LTC4222_FaultAlertRegisterTypedef __hs_faults1, __hs_faults2 ;
#define max_pmbus_addr_cnt 16
	u8 pmbus_addr_cnt = 0 ; 	// кол-во адресов ответивших на ARA
	u8 pmbus_addr[ max_pmbus_addr_cnt ] ;	// адреса на шине pmbus, ответившие на ARA
	size_t i, j ;

	// если несколько устройств возбудили alarm одновременно (например два
	// слотовых устройства одновременно вставили), обходим все в цикле
	while( Dnepr_EdgePort_is_IRQ4_active() && (pmbus_addr_cnt < max_pmbus_addr_cnt) ){
		// выясняем кто вызвал
		pmbus_addr[ pmbus_addr_cnt++ ] = Dnepr_I2C_Read_ARA() ;
		// продолжаем обходить если прерывание ещё активно
	}


	for( j = 0; j < pmbus_addr_cnt; j++ ){
		for( i = 0;
				(pmbus_addr[ j ] != __hs_slot_addresses[i]) && (i < I2C_DNEPR_NUMBER_OF_SLOTS+1); i++ );
		// неизвестный адрес -- выходим
		if( i >= ( I2C_DNEPR_NUMBER_OF_SLOTS + 1 ))
			return ;

		// hot swap слотового устройства или платы вентиляции
		if( i <= FAN_HS_ADDR_NUM){
			// если не прошел таймаут -- не реагируем
			if( __slot_plugin_time[ i ] + SLOT_PLUGIN_TIMEOUT > llUptime ){
				return ;
			}
			// перечитываем его статус и алармы
			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL1,
										&__hs_slot_statuses1, &__hs_faults1 );
			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL2,
										&__hs_slot_statuses2, &__hs_faults2 );
			// сбрасываем фолты
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL1 );
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), pmbus_addr[ j ], LT_LTC4222_CHANNEL2 );

			// Заполняем структуру сообщения
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
				// кладёт в начало очереди сообщение об аларме и не вызывает шедулер
				// для эффективности
				OSQPostOpt( __qPMBAlarms, (void*)&__alarm_mess[ __curAlarmMess_ind ],
							OS_POST_OPT_FRONT | OS_POST_OPT_NO_SCHED ) ;
				if( ++__curAlarmMess_ind >= __pmbalarms_arr_len ){
					__curAlarmMess_ind = 0 ;
				}
			}
		// контроллер вентиляторов
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
				// кладёт в начало очереди сообщение об аларме и не вызывает шедулер
				// для эффективности
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

// регистр Alert общий для всех каналов всех устройств
static LT_LTC4222_FaultAlertRegisterTypedef	hs_alert_reg = {
											TRUE,	// FET short condition
											FALSE,	// EN# changes state
											TRUE,	// output power is bad
											TRUE,	// overcurrent condition
											TRUE,	// undervoltage condition
											TRUE	// overvoltage condition
											};

// При включении Днепра проходит EEPROM и HotSwap'ы всех слотовых устройств,
// считывает их состояния, переинициализирует HotSwapы, запускает пересмотр
// включения устройств
void __slot_power_onoff_init()
//static void __slot_power_onoff_init()
{
	size_t i, j ;
	LT_LTC4222_AdcVoltagesStructure     tAdcVoltagesStructure;
	LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
	LT_LTC4222_StatusRegisterTypedef 	tStateRegisterStructure_ch1 ;
	LT_LTC4222_AdcControlRegisterTypedef tAdcControlRegisterStructure ;
	_BOOL hs_available ;

	Dnepr_Refresh_Presents() ;

	// читаем по очереди все EEPROM и состояния HotSwap'ов (+ 1 слот для платы
	// вентиляции)
	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
		if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] ){
			__aPwrStates[ i ] = HSSLOT_UNAVAILABLE ;
			continue ;
		}
		__processed_slots_present.bSlotPresent[ i ] = 1 ;
		Dnepr_SlotEEPROM_Read( i );
		// читаем и настраиваем HotSwap
		// проверяем наличие hotswap'а проверяя напряжения
		for( j = 0; j < 5; j++ ){
			LT_LTC4222_GetAdcVoltages( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcVoltagesStructure );
			hs_available = 	(tAdcVoltagesStructure.fSource2 > 2.0) && 
							(tAdcVoltagesStructure.fSource2 < 4.0) ;
			if( hs_available ){
				break ;
			} else {
				OSTimeDly( 1 );
			}
		}

		// настраиваем hotswap
		if( hs_available ){
			// читаем текущие значения настроек
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );

			// определяем включено ли устройство
			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i],  LT_LTC4222_CHANNEL1, &tStateRegisterStructure_ch1, NULL );
			// если устройство работает -- не важно почему
			if( (tStateRegisterStructure_ch1.bFetOn) || 
				(i == I2C_DNEPR_FAN_SLOT_NUM - 1)){ // или это вентиляторы -- тоже не трогаем
				__aPwrStates[ i ] = HSSLOT_ON ;
				__fSlotPower[ i ] = (f32)Dnepr_SlotEEPROM_SlotPower(i) ;
			// устройство не включено -- о включении решаем после того, как прочитаем параметры из флеши
			// и eeprom
			} else {
				__aPwrStates[ i ] = HSSLOT_WAITING ;
			}
			// если доступна EEPROM -- настраиваем hotswap и учитываем потребляемую мощность
			if( Dnepr_SlotEEPROM_Available( i )){
				// 1й канал 12 В
				hs_control_reg_ch1.bOcAutoRetry = Dnepr_SlotEEPROM_val( i )->oc_ar1 ;
				hs_control_reg_ch1.bUvAutoRetry = Dnepr_SlotEEPROM_val( i )->uv_ar1 ;
				hs_control_reg_ch1.bOvAutoRetry = Dnepr_SlotEEPROM_val( i )->ov_ar1 ;
				// 2й канал 3.3 В
				hs_control_reg_ch2.bOcAutoRetry = Dnepr_SlotEEPROM_val( i )->oc_ar2 ;
				hs_control_reg_ch2.bUvAutoRetry = Dnepr_SlotEEPROM_val( i )->uv_ar2 ;
				hs_control_reg_ch2.bOvAutoRetry = Dnepr_SlotEEPROM_val( i )->ov_ar2 ;
			}
			// задерживаем включение устройства
			if( __aPwrStates[ i ] == HSSLOT_WAITING ){
				hs_control_reg_ch1.bFetOnControl = 1 ;
				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
				hs_control_reg_ch2.bGpioOutputState = 0 ;
			}
			//устанавливаем запрет на ALERT при завершении преобразования АЦП
			LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );
			tAdcControlRegisterStructure.bAdcAlert = FALSE ;
			LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );

			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, &hs_alert_reg );
			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, &hs_alert_reg );
			// сбрасываем фолты
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2 );
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1 );
		} else {
			// если hotswap недоступен, но есть данные из EEPROM -- расчитываем,
			// что устройство потребляет эту мощность
			if( Dnepr_SlotEEPROM_Available( i )){
				__fSlotPower[ i ] = (f32)Dnepr_SlotEEPROM_SlotPower( i );
			}
			__aPwrStates[ i ] = HSSLOT_UNAVAILABLE ;
		}
	}
}

// проходит все устройства, которые не включены и включает, если есть резерв
// мощности и устройству не запрещен запуск
static void __slot_power_onoff()
{
	LT_LTC4222_StatusRegisterTypedef __hs_slot_statuses1, __hs_slot_statuses2 ;
	LT_LTC4222_FaultAlertRegisterTypedef __hs_faults1, __hs_faults2 ;
	u32 i ;
	LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
	LT_LTC4222_AdcControlRegisterTypedef tAdcControlRegisterStructure ;


	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
		// если устройство не вставлено -- ничего не делаем
		if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[i] ){


		// если устройству запрещён запуск -- выключаем при любых обстоятельствах
		} else if( __slot_OnOff( i ) == PERMITTED_START ){
				__aPwrStates[ i ] = HSSLOT_REGULAR_OFF ;
				__fSlotPower[ i ] = 0 ;

				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );

				hs_control_reg_ch1.bFetOnControl = 0 ;
				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
				hs_control_reg_ch2.bGpioOutputState = 0 ;

				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );

		// если у устройства не доступна EEPROM и старт умный -- не включаем
		} else if( (__slot_OnOff( i ) == SMARTLY_START)  && 
				!Dnepr_SlotEEPROM_Available(i) ){
					__aPwrStates[ i ] = HSSLOT_OVERLIMIT_OFF ;
					__fSlotPower[ i ] = 0 ;
		}else if( (__aPwrStates[ i ] == HSSLOT_WAITING) ||
			(__aPwrStates[ i ] == HSSLOT_OVERLIMIT_OFF) ){
			// уст-во вписывается в предел мощности
			if( ((f32)Dnepr_SlotEEPROM_SlotPower( i ) < __curPowerLimit()) ||
				// ИЛИ включен уверенный пуск
				( __slot_OnOff( i ) == IGNORANT_START ) ){
				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
				LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_PGOOD ;
				hs_control_reg_ch2.bGpioOutputState = 1 ;
				hs_control_reg_ch1.bFetOnControl = 1 ;
				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
				LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );
				__aPwrStates[ i ] = HSSLOT_ON ;
				__fSlotPower[ i ] = (f32)Dnepr_SlotEEPROM_SlotPower( i ) ;


				// устанавливаем запрет на ALERT при завершении преобразования АЦП
				LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );
				tAdcControlRegisterStructure.bAdcAlert = FALSE ;
				LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], &tAdcControlRegisterStructure );

				OSTimeDly( 200 );
				// сбрасываем фолты
				LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2 );
				LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1 );
				// считываем статусы этого hotswap'а
				LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL1, &__hs_slot_statuses1, &__hs_faults1 );
				LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( i ), __hs_slot_addresses[i], LT_LTC4222_CHANNEL2, &__hs_slot_statuses2, &__hs_faults2 );

				// фиксируем время включния для выжидания таймаута
				__slot_plugin_time[ i ] = llUptime ;
				__processed_slots_present.bSlotPresent[ i ] = 1 ;
			} else {
				__aPwrStates[ i ] = HSSLOT_OVERLIMIT_OFF ;
				__processed_slots_present.bSlotPresent[ i ] = 0 ;
			}
		}
	}
}

// реакция на изменение present'ов у плис:
// сначала считываем EEPROM, потом разрешаем / не разрешаем запуск
static void __slot_power_dev_plug( const u8 slot_num, const _BOOL pluginout )
{
	size_t j ;
	_BOOL hs_available ;
	LT_LTC4222_AdcVoltagesStructure     tAdcVoltagesStructure;
	LT_LTC4222_ControlRegisterTypedef 	hs_control_reg_ch1, hs_control_reg_ch2 ;
	LT_LTC4222_StatusRegisterTypedef 	tStateRegisterStructure_ch1 ;
	LT_LTC4222_AdcControlRegisterTypedef tAdcControlRegisterStructure ;

	// устройство только что вставили
	if( pluginout ){
		// читаем и настраиваем HotSwap
		// проверяем наличие hotswap'а и проверяем напряжения (модуль PMBus не возвращает ошибку)
		for( j = 0; j < 10; j++ ){
			hs_available = LT_LTC4222_GetAdcVoltages( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcVoltagesStructure );
			hs_available = hs_available && (tAdcVoltagesStructure.fSource2 > 2.7) && 
							(tAdcVoltagesStructure.fSource2 < 4.0) ;
			if( hs_available ){
				break  ;
			} else {
				OSTimeDly( 20 );
			}
		}
		Dnepr_SlotEEPROM_Read( slot_num );
		
		// настраиваем hotswap
		if( hs_available ){
			// читаем текущие значения настроек
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, NULL );
			LT_LTC4222_GetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, NULL );

			LT_LTC4222_GetStates( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ],  LT_LTC4222_CHANNEL1, &tStateRegisterStructure_ch1, NULL );
			
			// если устройству запрещен запуск -- не включаем его
			if( __slot_OnOff( slot_num ) == PERMITTED_START ){
				__fSlotPower[ slot_num ] = 0 ;
				hs_control_reg_ch1.bFetOnControl = 0 ;
				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
				hs_control_reg_ch2.bGpioOutputState = 0 ;
				__aPwrStates[ slot_num ] = HSSLOT_REGULAR_OFF ;
			// определяем включено ли устройство
			// если устройство работает -- не важно почему
			} else if( tStateRegisterStructure_ch1.bFetOn ){
				__aPwrStates[ slot_num ] = HSSLOT_ON ;
				__fSlotPower[ slot_num ] = 0.0 ;
			// если EEPROM доступна и мощности достаточно ИЛИ старт без разбора -- включаем
			} else if(	Dnepr_SlotEEPROM_Available( slot_num ) && 
					((f32)Dnepr_SlotEEPROM_SlotPower( slot_num ) < __curPowerLimit()) ||
					(__slot_OnOff( slot_num ) == IGNORANT_START) ){
						__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num );
				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_PGOOD ;
				hs_control_reg_ch2.bGpioOutputState = 1 ;
				hs_control_reg_ch1.bFetOnControl = 1 ;
				__aPwrStates[ slot_num ] = HSSLOT_ON ;
			} else {
				hs_control_reg_ch1.bFetOnControl = 1 ;
				hs_control_reg_ch2.tGpioConfig = LT_LTC4222_OUTPUT ;
				hs_control_reg_ch2.bGpioOutputState = 0 ;
				__fSlotPower[ slot_num ] = 0 ;
				__aPwrStates[ slot_num ] = HSSLOT_OVERLIMIT_OFF ;
			}

			if( Dnepr_SlotEEPROM_Available( slot_num )){
				if( __aPwrStates[ slot_num ] == HSSLOT_ON ){
					__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num ) ;
				}
				// 1й канал 12 В
				hs_control_reg_ch1.bOcAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->oc_ar1 ;
				hs_control_reg_ch1.bUvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->uv_ar1 ;
				hs_control_reg_ch1.bOvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->ov_ar1 ;
				// 2й канал 3.3 В
				hs_control_reg_ch2.bOcAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->oc_ar2 ;
				hs_control_reg_ch2.bUvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->uv_ar2 ;
				hs_control_reg_ch2.bOvAutoRetry = Dnepr_SlotEEPROM_val( slot_num )->ov_ar2 ;
			}

			// устанавливаем запрет на ALERT при завершении преобразования АЦП
			LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcControlRegisterStructure );
			tAdcControlRegisterStructure.bAdcAlert = FALSE ;
			LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], &tAdcControlRegisterStructure );

			// включаем каналы и алерты
			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1, &hs_control_reg_ch1, &hs_alert_reg );
			LT_LTC4222_SetConfig( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2, &hs_control_reg_ch2, &hs_alert_reg );
			OSTimeDly( 250 );
			// сбрасываем фолты
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL1 );
			LT_LTC4222_ResetFaults( Dnepr_I2C_Get_PMBUS_EXT_Driver( slot_num ), __hs_slot_addresses[ slot_num ], LT_LTC4222_CHANNEL2 );

			// фиксируем время включния для выжидания таймаута
			__slot_plugin_time[ slot_num ] = llUptime ;
		} else {
			// если hotswap недоступен, но есть данные из EEPROM -- расчитываем,
			// что устройство потребляет эту мощность
			if( Dnepr_SlotEEPROM_Available( slot_num )){
				__fSlotPower[ slot_num ] = (f32)Dnepr_SlotEEPROM_SlotPower( slot_num );
			}
			__aPwrStates[ slot_num ] = HSSLOT_UNAVAILABLE ;
		}
		__processed_slots_present.bSlotPresent[ slot_num ] = 1 ;
	// устройство вынули
	} else {
		__processed_slots_present.bSlotPresent[ slot_num ] = 0 ;
		__aPwrStates[ slot_num ] = HSSLOT_UNAVAILABLE ;
		__fSlotPower[ slot_num ] = 0 ;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// расчитывает и возвращает текущий доступный лимит потребления
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
	
	// Проходим все слотовые устройства кроме блока вентиляторов.
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

// возвращает включен или нет слот исходя из параметров профиля
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
	// Обновляем наличие sfp
 	Dnepr_Refresh_SFP_Presents() ;

	__processed_slots_present.bSfpPresent[ 0 ] =  I2C_DNEPR_GetPresentDevices()->bSfpPresent[ 0 ];
	__processed_slots_present.bSfpPresent[ 1 ] =  I2C_DNEPR_GetPresentDevices()->bSfpPresent[ 1 ];
}

extern u32 val_FUFanMode ;
extern u32 val_FUFanThreshold75 ;
extern u32 val_FUFanThreshold90 ;
extern u32 val_FUFanSpeedSet ;

static void __set_fans()
{
        u32 need_fan_power ;
	// Если автоматический режим управления вентиляторами.
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
