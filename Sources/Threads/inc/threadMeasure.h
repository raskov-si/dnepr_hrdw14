/*!
\file threadMeasure.h
\brief поток дл€ измерени€ динамических параметров, данные забираютс€ в T8_Dnepr_Profile_params.*
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
//! ƒелает работу с двойным буфером динамических параметров атомарной.
void Dnepr_Measure_DynBuff_Lock();
//! ¬ыходим из атомарного участка работы с буфером динамических параметров.
void Dnepr_Measure_DynBuff_Unlock();

//! возвращает копию хранимую в ќ«” данных из EEPROM платы вентил€ции
Dnepr_Fans_CalData_t* Dnepr_Measure_Fans_Calib() ;
//! пишет копию из озу в EEPROM платы вентил€ции
void Dnepr_Measure_Fans_Calib_Update() ;

//! \brief устанавливает значение светодиода дл€ CPU, не вли€€ на ALARM[1|2]
void Dnepr_Measure_SetCPULed( const T8_Dnepr_LedTypedef led );
//! \brief устанавливает значение светодиода POWER
void Dnepr_Measure_SetPowerLed( const T8_Dnepr_LedTypedef led );
//! \brief устанавливает цвет светодиода ALARM
void Dnepr_Measure_SetAlarmLed( const T8_Dnepr_LedTypedef led );
//! \brief ѕрошить EEPROM Ѕѕ.
void Dnepr_UpdatePSUEEPROM( const u8 ps_num, const s8* vendor, const s8* ptnumber,
													const u16 power, const s8* srnumber );

//! \brief команда на запись в EEPROM слотового устройства
void Dnepr_Measure_SlotEEPROM_Write( const u32 slot_num, SlotHSEEPROMParams_t *pSlotParam );
//! \brief команда на запись в дополнительную секцию EEPROM слотового устройства
void Dnepr_Measure_SlotOptionalEEPROM_Write( const u32 slot_num, Dnepr_SlotEEPROM_Optional_t* pData );
//! \brief ”далить запись о пассивном устройстве в EEPROM.
void Dnepr_Measure_SlotOptionalEEPROM_Clear( const u32 slot_num );
//! \brief отдаЄт указатель на данные серийного номера или ноль, если CRC не совпал или не прочиталось
MAX_DS28CM00_ContentsTypedef *Dnepr_Measure_SerialNumber();
//! \brief то же, что и Dnepr_Measure_SerialNumber(), только серийный номер backplane'а в крейте
MAX_DS28CM00_ContentsTypedef *Dnepr_Measure_Backplane_SerialNumber();
//! \brief отдаЄт все измеренные данные о SFP 1 или 0, если модуль не вставлен или
//! не успели произвести измерени€ после включени€
const T8_SFP_OPTICAL_CHANNEL* Dnepr_Measure_SFP_L_Info() ;
//! \brief отдаЄт все измеренные данные о SFP 2 или 0, если модуль не вставлен или
//! не успели произвести измерени€ после включени€
const T8_SFP_OPTICAL_CHANNEL* Dnepr_Measure_SFP_U_Info() ;
//! \brief измен€ет состо€ние выводов лазеров обоих SFP
void Dnepr_Measure_SFP_ChangeState( const _BOOL sfp_1_on_, const _BOOL sfp_2_on_ );

//! определ€ет значение информационный параметр SFP?DDMState
typedef enum {
	DDM_COMPLIANCE_NOT_COMPLY_SFF8472 = 1,
	DDM_COMPLIANCE_NO_DDM = 2,
	DDM_COMPLIANCE_INTERNAL_CALIBRATION = 3,
	DDM_COMPLIANCE_EXTERNAL_CALIBRATION = 4,
	DDM_COMPLIANCE_NO_ANSWER = 5
} SFP_DDM_COMPLIANCE ;

//! \brief ¬озвращает соответствие SFP 1 SFF-8472 (рег. 92d по адресу A0h)
SFP_DDM_COMPLIANCE Dnepr_Measure_SFP1_DDM_Compliance();
//! \brief ¬озвращает соответствие SFP 2 SFF-8472 (рег. 92d по адресу A0h)
SFP_DDM_COMPLIANCE Dnepr_Measure_SFP2_DDM_Compliance();

//! \brief устанавливает скорость вращени€ вентил€торов
void Dnepr_Measure_UpdateFansRPM( const u32 fan_power );

//! \brief статус flash-пам€ти power sequnecer'а с настройками 
typedef enum __PowerSequencerFlashState {
	PSEQ_UNKNOWN = 0,
	PSEQ_OK,
	PSEQ_WROTE,
	PSEQ_FAIL
} PowerSequencerFlashState_t;
PowerSequencerFlashState_t Dnepr_Measure_PSequencer_State() ;

//! \brief перезагружает power sequencer и следовательно всю плату
void Dnepr_Measure_PSequencer_Reset() ;

#endif
