/*!
\file T8_Dnepr_BPEEPROM.h
\brief ��� ��� ������ � ����������� � EEPROM �� backplane'�
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#ifndef __DNEPR_BPEEPROM_H_
#define __DNEPR_BPEEPROM_H_

#include "support_common.h"
#include "sys.h"

#define BPEEPROM_PRIMARY_PART_START 	0
#define BPEEPROM_SECONDARY_PART_START 	65536
#define BPEEPROM_END		131071 // ��������� ���� � EEPROM

//! ����������� �� ����� ����� ��������� -- ������ 76 ����
#define BPEEPROM_NAMELEN				32 
//! ����� ���-�� ��������, (65536 - 1280) / 76 = 845
#define BPEEPROM_TOTALSECTORS	840

typedef enum {
	BPEEPROM_NONE,			//!< �����������
	BPEEPROM_OK,			//!< �� ������
	BPEEPROM_ERRORNEOUS		//!< ������
} BPEEPROM_State ;

//! \brief �������� ������� ��� �������, ��������� ����������� �����, �������� ���������
//! \brief � �����������, �������� ��������� � ���� ��������� ���������, � ������
//! \brief ������� ������� ��������� �� ���������� ����������, ��� eeprom ���������.
//! \brief ���������� �� ���� �������.
_BOOL Dnepr_BPEEPROM_Init() ;

//! \brief ������ �������� �� ���������� �������, � ������ ������� ������ ��
//! \brief ������ �� ���������� � ������������ � ���������
//! \param sName ��������� �� ������ � ������ ���������
//! \param pbData ��������� �� ������, ���� ����� ����������� ������
//! \param actual_len ����� ����������� ������
//! \param maxlen ����� ������� pbData
//! \retval TRUE � ������ ������, ����� FALSE
_BOOL Dnepr_BPEEPROM_Read(u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen  );

//! \brief ����� �������� � EEPROM � ��� ������� �� �������
//! \param sName ��� ���������
//! \param pbData ������
//! \param actual_len ����� ������
//! \retval TRUE ���� ������ ������� ���� �� � ���� ������
_BOOL Dnepr_BPEEPROM_Write(u8* sName, u8* pbData, const size_t actual_len );

//! ��������������� eeprom, ������ ������, �����. 8 ������� �� ������ -- 16 �����
_BOOL Dnepr_eeprom_format(); 
//! �������� �� ���������� � ������ EEPROM_PARAM, � ������������ �������� �� EEPROM, ���� �����
_BOOL Dnepr_eeprom_sync();
//! ���������� ������� ��������� EEPROM
BPEEPROM_State Dnepr_eeprom_GetState() ;

_BOOL Dnepr_BPEEPROM_CheckPresent();
void Dnepr_BPEEPROM_WriteEnable(void);
void Dnepr_BPEEPROM_WriteDisable(void);


#endif
