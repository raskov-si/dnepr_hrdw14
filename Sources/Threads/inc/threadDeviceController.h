/*!
\file threadDeviceController.h
\brief поток управляет включением устройств и реагирует на ALARM PMBus'а
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/
#ifndef __THREADDEVICECONTROLLER_H_
#define __THREADDEVICECONTROLLER_H_

#include "common_lib/CommonEEPROMFormat.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_SlotEEPROM.h"

//! статус hotswap'а слотового устройства
typedef enum {
	HSSLOT_UNAVAILABLE,		//!< устройство недоступно по PMBus
	HSSLOT_ON,				//!< устройство включено
	HSSLOT_REGULAR_OFF,		//!< устройство выключено по команде оператора
	HSSLOT_OVERLIMIT_OFF,	//!< мощность устройства превышает доступный порог
	HSSLOT_WAITING			//!< ожидает разрешения включения
} SlotPowerState_t ;

//! \brief читает все EEPROM и HotSwap Controller'ы и задерживаем включение тех, которые
//! ещё не включились
void DeviceController_Init(void);

//! \brief функция потока
void taskDeviceController(void *pdata);

//! \brief заставляет поток пересчитать мощности обоих б.п.
void Dnepr_DControl_ResetMaxPower();

//! \brief предел мощности надо брать из БП или задаётся фиксированное значение
void Dnepr_DControl_SetPowerLimitSource( const _BOOL psu_source, const f32 limit );

/// перечитывает из профиля источник предела мощности и его значение и 
/// перезапускает устройства. Вызывается, когда профиль перечитан из EEPROM 
/// Backplane'а
void Dnepr_DControl_ReinitPowerLimitSource();

//! вызывается из прерывания EDGEPORT, информирует о alarm на pmbus
void Dnepr_DControl_PMBusAlarm();
//! вызывается из прерывания EDGEPORT, информирует о прерывании от ПЛИС
void Dnepr_DControl_Present_Interrupt();
//! вызывается из прерывания EDGEPORT, информирует о прерывании от SFP
void Dnepr_DControl_SFP_Interrupt();
// сигнал о том, что в профиле поменялось разрешение запуска слота
void Dnepr_DControl_RereadPowerOnOff( const u32 slot_num );
//! возвращает сколько осталось мощности
f32 Dnepr_DControl_PowerReserv() ;

// заполняет строку для профиля с описанием произошедших на линии PMBus Alarm'ов
size_t Dnepr_DControl_NextPMBusAlarm( s8* sResult, const size_t wResultLen );

// сигнал о необходимости проверить возможность включения устройств,
// например, если увеличился запас по питанию
void Dnepr_DControl_ReinitHotswaps();

//! статус hotswapa слота slot_num
SlotPowerState_t Dnepr_DControl_PowerStatus( const u32 slot_num ) ;

//! \brief Занятость слотов, с учетом механизма пассивных устройств.
I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotPresent() ;

//! \brief Состояния сухих контактов про занятость слотов.
I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotRawPresent() ;

//! Обновить состояние презентов в SFP.
void Dnepr_DControl_sfp_process_present() ;
void Dnepr_DControl_fun_process_present(void);


//! отдаёт команду потоку пересмотреть разрешение запуска устройства
void Dnepr_SlotEEPROM_RereadPowerOnOff( const u32 slot_num );
//! \brief пишет описание самого старого зарегистрированного Alarm'а на PMBus
//! \details обращается к очереди сообщений с описаниями аварий, соотв. следующий
//! \details вызов вернёт описание следующей, более ранней аварии.
//! \param sResult указатель на строку, в которую надо записать описание
//! \param wResultLen длина массива sResult
//! \retval сколько байтов было записано
size_t Dnepr_SlotEEPROM_NextPMBusAlarm( s8* sResult, const size_t wResultLen ) ;

//! Перенастраивает вентиляторы.
void Dnepr_DControl_SetFans() ;

f32 Dnepr_DControl_MaxPower() ;

#endif
