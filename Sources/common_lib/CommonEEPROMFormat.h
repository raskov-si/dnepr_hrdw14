/*!
\file CommonEEPROMFormat.h
\brief высокоуровневое управление и взаимодействие со слотовыми устройствами (Hotswap, EEPROM etc)
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#ifndef __COMMONEEPROMFORMAT_H_
#define __COMMONEEPROMFORMAT_H_

//! адрес в EEPROM начиная с которого записаны данные
#define 	SLOTEEPROM_ADDRESS	0

//! номер версии формата данных в EEPROM
#define 	SLOTEEPROM_DATA_VERSION		0x0010
//! длина строки с именем устройства
#define 	SLOTEEPROM_SLOTNAME_LEN	16

#pragma pack( push )
#pragma pack( 1 )
//! структура данных в EEPROM для алгоритма включения слотовых устройств Control Unit'а
typedef struct {
	//! версия формата
	unsigned short version ;
	//! максимальная потребляемая мощность через 200 мс после включения в Вт по линии 12 В
	unsigned int dMaxPower ;
	//! название устройства
	char sName[ SLOTEEPROM_SLOTNAME_LEN ];

	//! номинал токоизмерительного шунта по линии 12 В
	float fHS_12_bypass ;
	//! номинал токоизмерительного шунта по линии 3.3 В
	float fHS_33_bypass ;
	// Over-Current Auto-Retry Channel 1 (12 В), 0 -- нет, 1 -- да
	unsigned char oc_ar1 ;
	// Over-Current Auto-Retry Channel 2 (3.3 В), 0 -- нет, 1 -- да
	unsigned char oc_ar2 ;
	// Undervoltage Auto-Retry Channel 1 (12 В), 0 -- нет, 1 -- да
	unsigned char uv_ar1 ;
	// Undervoltage Auto-Retry Channel 2 (3.3 В), 0 -- нет, 1 -- да
	unsigned char uv_ar2 ;
	// Overvoltage Auto-Retry Channel 1 (12 В), 0 -- нет, 1 -- да
	unsigned char ov_ar1 ;
	// Overvoltage Auto-Retry Channel 2 (3.3 В), 0 -- нет, 1 -- да
	unsigned char ov_ar2 ;

	//! контрольная сумма
	u32 CRC ;
} SlotHSEEPROMParams_t ;
#pragma pack( pop )

#endif
