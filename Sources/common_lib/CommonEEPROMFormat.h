/*!
\file CommonEEPROMFormat.h
\brief ��������������� ���������� � �������������� �� ��������� ������������ (Hotswap, EEPROM etc)
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#ifndef __COMMONEEPROMFORMAT_H_
#define __COMMONEEPROMFORMAT_H_

//! ����� � EEPROM ������� � �������� �������� ������
#define 	SLOTEEPROM_ADDRESS	0

//! ����� ������ ������� ������ � EEPROM
#define 	SLOTEEPROM_DATA_VERSION		0x0010
//! ����� ������ � ������ ����������
#define 	SLOTEEPROM_SLOTNAME_LEN	16

#pragma pack( push )
#pragma pack( 1 )
//! ��������� ������ � EEPROM ��� ��������� ��������� �������� ��������� Control Unit'�
typedef struct {
	//! ������ �������
	unsigned short version ;
	//! ������������ ������������ �������� ����� 200 �� ����� ��������� � �� �� ����� 12 �
	unsigned int dMaxPower ;
	//! �������� ����������
	char sName[ SLOTEEPROM_SLOTNAME_LEN ];

	//! ������� ������������������ ����� �� ����� 12 �
	float fHS_12_bypass ;
	//! ������� ������������������ ����� �� ����� 3.3 �
	float fHS_33_bypass ;
	// Over-Current Auto-Retry Channel 1 (12 �), 0 -- ���, 1 -- ��
	unsigned char oc_ar1 ;
	// Over-Current Auto-Retry Channel 2 (3.3 �), 0 -- ���, 1 -- ��
	unsigned char oc_ar2 ;
	// Undervoltage Auto-Retry Channel 1 (12 �), 0 -- ���, 1 -- ��
	unsigned char uv_ar1 ;
	// Undervoltage Auto-Retry Channel 2 (3.3 �), 0 -- ���, 1 -- ��
	unsigned char uv_ar2 ;
	// Overvoltage Auto-Retry Channel 1 (12 �), 0 -- ���, 1 -- ��
	unsigned char ov_ar1 ;
	// Overvoltage Auto-Retry Channel 2 (3.3 �), 0 -- ���, 1 -- ��
	unsigned char ov_ar2 ;

	//! ����������� �����
	u32 CRC ;
} SlotHSEEPROMParams_t ;
#pragma pack( pop )

#endif
