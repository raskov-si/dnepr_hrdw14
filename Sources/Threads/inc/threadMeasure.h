/*!
\file threadMeasure.h
\brief ����� ��� ��������� ������������ ����������, ������ ���������� � T8_Dnepr_Profile_params.*
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jul 2012
*/

#ifndef __THREADMEASURE_H_
#define __THREADMEASURE_H_

#include "uCOS_II.H"
#include "HAL\IC\inc\T8_SFP.h"
#include "HAL/IC/inc/MAX_DS28CM00.h"
#include "HAL/BSP/inc/T8_Dnepr_FPGA.h"
#include "HAL/BSP/inc/T8_Dnepr_LED.h"
#include "HAL/BSP/inc/T8_Dnepr_Fans.h"
#include "HAL/BSP/inc/T8_Dnepr_SlotEEPROM.h"

void taskMeasure(void *pdata);
//! ������ ������ � ������� ������� ������������ ���������� ���������.
void Dnepr_Measure_DynBuff_Lock();
//! ������� �� ���������� ������� ������ � ������� ������������ ����������.
void Dnepr_Measure_DynBuff_Unlock();

//! ���������� ����� �������� � ��� ������ �� EEPROM ����� ����������
Dnepr_Fans_CalData_t* Dnepr_Measure_Fans_Calib() ;
//! ����� ����� �� ��� � EEPROM ����� ����������
void Dnepr_Measure_Fans_Calib_Update() ;

//! \brief ������������� �������� ���������� ��� CPU, �� ����� �� ALARM[1|2]
void Dnepr_Measure_SetCPULed( const T8_Dnepr_LedTypedef led );
//! \brief ������������� �������� ���������� POWER
void Dnepr_Measure_SetPowerLed( const T8_Dnepr_LedTypedef led );
//! \brief ������������� ���� ���������� ALARM
void Dnepr_Measure_SetAlarmLed( const T8_Dnepr_LedTypedef led );
//! \brief ������� EEPROM ��.
void Dnepr_UpdatePSUEEPROM( const u8 ps_num, const s8* vendor, const s8* ptnumber,
													const u16 power, const s8* srnumber );

//! \brief ������� �� ������ � EEPROM ��������� ����������
void Dnepr_Measure_SlotEEPROM_Write( const u32 slot_num, SlotHSEEPROMParams_t *pSlotParam );
//! \brief ������� �� ������ � �������������� ������ EEPROM ��������� ����������
void Dnepr_Measure_SlotOptionalEEPROM_Write( const u32 slot_num, Dnepr_SlotEEPROM_Optional_t* pData );
//! \brief ������� ������ � ��������� ���������� � EEPROM.
void Dnepr_Measure_SlotOptionalEEPROM_Clear( const u32 slot_num );
//! \brief ����� ��������� �� ������ ��������� ������ ��� ����, ���� CRC �� ������ ��� �� �����������
MAX_DS28CM00_ContentsTypedef *Dnepr_Measure_SerialNumber();
//! \brief �� ��, ��� � Dnepr_Measure_SerialNumber(), ������ �������� ����� backplane'� � ������
MAX_DS28CM00_ContentsTypedef *Dnepr_Measure_Backplane_SerialNumber();
//! \brief ����� ��� ���������� ������ � SFP 1 ��� 0, ���� ������ �� �������� ���
//! �� ������ ���������� ��������� ����� ���������
const T8_SFP_OPTICAL_CHANNEL* Dnepr_Measure_SFP_L_Info() ;
//! \brief ����� ��� ���������� ������ � SFP 2 ��� 0, ���� ������ �� �������� ���
//! �� ������ ���������� ��������� ����� ���������
const T8_SFP_OPTICAL_CHANNEL* Dnepr_Measure_SFP_U_Info() ;
//! \brief �������� ��������� ������� ������� ����� SFP
void Dnepr_Measure_SFP_ChangeState( const _BOOL sfp_1_on_, const _BOOL sfp_2_on_ );

void dnepr_measure_SFP_change_autoneg_mode
(
    u8 sfp_num,                         /*!< [in] ����� sfp �� �������                      */
    u8 autoneg_state                    /*!< [in] ��������� � ������� ����� �������������   */
);


//! ���������� �������� �������������� �������� SFP?DDMState
typedef enum {
	DDM_COMPLIANCE_NOT_COMPLY_SFF8472 = 1,
	DDM_COMPLIANCE_NO_DDM = 2,
	DDM_COMPLIANCE_INTERNAL_CALIBRATION = 3,
	DDM_COMPLIANCE_EXTERNAL_CALIBRATION = 4,
	DDM_COMPLIANCE_NO_ANSWER = 5
} SFP_DDM_COMPLIANCE ;

//! \brief ���������� ������������ SFP 1 SFF-8472 (���. 92d �� ������ A0h)
SFP_DDM_COMPLIANCE Dnepr_Measure_SFP1_DDM_Compliance();
//! \brief ���������� ������������ SFP 2 SFF-8472 (���. 92d �� ������ A0h)
SFP_DDM_COMPLIANCE Dnepr_Measure_SFP2_DDM_Compliance();

//! \brief ������������� �������� �������� ������������
void Dnepr_Measure_UpdateFansRPM( const u32 fan_power );

//! \brief ������ flash-������ power sequnecer'� � ����������� 
typedef enum __PowerSequencerFlashState {
	PSEQ_UNKNOWN = 0,
	PSEQ_OK,
	PSEQ_WROTE,
	PSEQ_FAIL
} PowerSequencerFlashState_t;
PowerSequencerFlashState_t Dnepr_Measure_PSequencer_State() ;

//! \brief ������������� power sequencer � ������������� ��� �����
void Dnepr_Measure_PSequencer_Reset() ;

#endif
