/*!
\file T8_Dnepr_SlotEEPROM.h
\brief ��� ������ � ������ �������� � EEPROM �������� ���������.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date mar 2014
*/

#ifndef _T8_DNEPR_SLOTEEPROM__
#define _T8_DNEPR_SLOTEEPROM__

#include "common_lib\CommonEEPROMFormat.h"

//! ������ EEPROM ��������� ����������
typedef enum {
	EEPROMSLOT_UNAVAILABLE,	//!< ���� ����������, ���� ������� CRC
	EEPROMSLOT_OK			//!< EEPROM �����������
} DControl_EEPROM_Status_t ;

//! ����� � EEPROM �� �������� ��������� ��������� ������������ ��������� ����������.
#define DNEPR_PASSIVE_STRUCT_ADDR		41
#define DNEPR_PASSIVE_STRUCT_MAXLEN		256
#define DNEPR_PASSIVE_STRUCT_DATALEN	(DNEPR_PASSIVE_STRUCT_MAXLEN - sizeof(u32) - sizeof(u32))

//! ��������� ��� ������������� �������������� ������ � EEPROM ��������� ����������.
typedef struct {
	u32 length ;
	u8 	pData[DNEPR_PASSIVE_STRUCT_DATALEN] ;
	u32 crc ;
} Dnepr_SlotEEPROM_Optional_t ;

#define EEPROM_PASSIVE_DESC_LEN 32
#define EEPROM_PASSIVE_DEST_LEN 32
#define EEPROM_PASSIVE_SN_LEN	32
#define EEPROM_PASSIVE_CLASS_LEN	32

//! ��������� �������������� ���������� � EEPROM ���������� ����������. �������� ������ 
//! �������, �������� � ���������� Dnepr_SlotEEPROM_Optional_t.
typedef struct {
	_BOOL passive ;
	u8 sDescription[EEPROM_PASSIVE_DESC_LEN] ;
	u8 sDestination[EEPROM_PASSIVE_DEST_LEN] ;
	u8 sSerial[EEPROM_PASSIVE_SN_LEN] ;
	u8 sClass[EEPROM_PASSIVE_CLASS_LEN] ;
} Dnepr_SlotEEPROM_Passive_t ;

typedef enum {
	EEPROMSLOT_OPTIONAL_UNAVAILABLE ,
	EEPROMSLOT_OPTIONAL_AVAILABLE 
} Dnepr_SlotEEPROM_Optional_Status_t ;

void Dnepr_SlotEEPROM_Init();

//! \brief ����� ��������� � �������� eeprom
void Dnepr_SlotEEPROMWrite( const u32 slot_num, SlotHSEEPROMParams_t *pSlotParam );
_BOOL Dnepr_SlotEEPROM_Read( const u32 slot_num );
SlotHSEEPROMParams_t* Dnepr_SlotEEPROM_val( const u32 slot_num );
_BOOL Dnepr_SlotEEPROM_Available( const u32 slot_num );

//! ��� ���������� �� ��� eeprom
const s8* Dnepr_SlotEEPROM_DeviceName( const u32 slot_num );
//! �������� ������������ ������
u32 Dnepr_SlotEEPROM_SlotPower( const u32 slot_num );
//! ������� ����� ������ 3.3 �
f32 Dnepr_SlotEEPROM_3v3_Bypass( const u32 slot_num );
//! ������� ����� ������ 12.0 �
f32 Dnepr_SlotEEPROM_12v0_Bypass( const u32 slot_num );

//! \brief ����� �������������� ������ � eeprom ��������� ����������
_BOOL Dnepr_SlotOptionalEEPROM_Write( const u32 slot_num, Dnepr_SlotEEPROM_Optional_t* pData );
//! \brief ������ ������������ ������.
_BOOL Dnepr_SlotOptionalEEPROM_Clear( const u32 slot_num );

_BOOL Dnepr_SlotOptionalEEPROM_Available( const u32 slot_num );
Dnepr_SlotEEPROM_Optional_t* Dnepr_SlotOptionalEEPROM_val( const u32 slot_num );

#endif
