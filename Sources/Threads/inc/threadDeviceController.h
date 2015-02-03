/*!
\file threadDeviceController.h
\brief ����� ��������� ���������� ��������� � ��������� �� ALARM PMBus'�
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/
#ifndef __THREADDEVICECONTROLLER_H_
#define __THREADDEVICECONTROLLER_H_

#include "common_lib/CommonEEPROMFormat.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_SlotEEPROM.h"

//! ������ hotswap'� ��������� ����������
typedef enum {
	HSSLOT_UNAVAILABLE,		//!< ���������� ���������� �� PMBus
	HSSLOT_ON,				//!< ���������� ��������
	HSSLOT_REGULAR_OFF,		//!< ���������� ��������� �� ������� ���������
	HSSLOT_OVERLIMIT_OFF,	//!< �������� ���������� ��������� ��������� �����
	HSSLOT_WAITING			//!< ������� ���������� ���������
} SlotPowerState_t ;

//! \brief ������ ��� EEPROM � HotSwap Controller'� � ����������� ��������� ���, �������
//! ��� �� ����������
void DeviceController_Init(void);

//! \brief ������� ������
void taskDeviceController(void *pdata);

//! \brief ���������� ����� ����������� �������� ����� �.�.
void Dnepr_DControl_ResetMaxPower();

//! \brief ������ �������� ���� ����� �� �� ��� ������� ������������� ��������
void Dnepr_DControl_SetPowerLimitSource( const _BOOL psu_source, const f32 limit );

/// ������������ �� ������� �������� ������� �������� � ��� �������� � 
/// ������������� ����������. ����������, ����� ������� ��������� �� EEPROM 
/// Backplane'�
void Dnepr_DControl_ReinitPowerLimitSource();

//! ���������� �� ���������� EDGEPORT, ����������� � alarm �� pmbus
void Dnepr_DControl_PMBusAlarm();
//! ���������� �� ���������� EDGEPORT, ����������� � ���������� �� ����
void Dnepr_DControl_Present_Interrupt();
//! ���������� �� ���������� EDGEPORT, ����������� � ���������� �� SFP
void Dnepr_DControl_SFP_Interrupt();
// ������ � ���, ��� � ������� ���������� ���������� ������� �����
void Dnepr_DControl_RereadPowerOnOff( const u32 slot_num );
//! ���������� ������� �������� ��������
f32 Dnepr_DControl_PowerReserv() ;

// ��������� ������ ��� ������� � ��������� ������������ �� ����� PMBus Alarm'��
size_t Dnepr_DControl_NextPMBusAlarm( s8* sResult, const size_t wResultLen );

// ������ � ������������� ��������� ����������� ��������� ���������,
// ��������, ���� ���������� ����� �� �������
void Dnepr_DControl_ReinitHotswaps();

//! ������ hotswapa ����� slot_num
SlotPowerState_t Dnepr_DControl_PowerStatus( const u32 slot_num ) ;

//! \brief ��������� ������, � ������ ��������� ��������� ���������.
I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotPresent() ;

//! \brief ��������� ����� ��������� ��� ��������� ������.
I2C_DNEPR_PresentDevicesTypedef* Dnepr_DControl_SlotRawPresent() ;

//! �������� ��������� ��������� � SFP.
void Dnepr_DControl_sfp_process_present() ;

//! ����� ������� ������ ������������ ���������� ������� ����������
void Dnepr_SlotEEPROM_RereadPowerOnOff( const u32 slot_num );
//! \brief ����� �������� ������ ������� ������������������� Alarm'� �� PMBus
//! \details ���������� � ������� ��������� � ���������� ������, �����. ���������
//! \details ����� ����� �������� ���������, ����� ������ ������.
//! \param sResult ��������� �� ������, � ������� ���� �������� ��������
//! \param wResultLen ����� ������� sResult
//! \retval ������� ������ ���� ��������
size_t Dnepr_SlotEEPROM_NextPMBusAlarm( s8* sResult, const size_t wResultLen ) ;

//! ��������������� �����������.
void Dnepr_DControl_SetFans() ;

f32 Dnepr_DControl_MaxPower() ;

#endif
