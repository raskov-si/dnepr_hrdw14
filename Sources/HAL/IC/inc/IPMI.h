/*!
\file IPMI.h
\brief Работа с IPMI v1.0 (для чтения параметров блоков питания).
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#ifndef IPMI_H__
#define IPMI_H__

#include "HAL/IC/inc/I2C_interface.h"

#define POWERUNIT_PMBUS_TIMEOUTMS   (100)


//! максимальная длина строк, если на самом деле строка больше -- не читаем и ограничиваемся означенной длиной
#define IPMI_STRINGS_LEN	32

#define IPMI_COMMONHEADER_ADDR 	0

typedef struct __IPMI_CommonHeader {
	u8 format_version ;
	u8 internal_area_offset ;
	u8 chassis_info_area_offset ;
	u8 board_area_offset ;
	u8 product_area_offset ;
	u8 multirecord_area_offset ;
	u8 pad ;
	u8 checksum ;
} IPMI_CommonHeader ;

typedef struct __IPMI_ProductInfo {
	u8 format_version ;
	u8 product_area_length ;		//!< длина этой записи в словах по 8 байт
	u8 language_code ;
	u8 manufacturer_name_length ;
	u8 manufacturer_name[IPMI_STRINGS_LEN+1] ;
	u8 name_length ;
	u8 name[IPMI_STRINGS_LEN+1] ;
	u8 model_length ;
	u8 model[IPMI_STRINGS_LEN+1] ;
	u8 serial_number_len ;
	u8 serial_number[IPMI_STRINGS_LEN+1] ;
	u8 asset_tag_len ;
	u8 asset_tag[IPMI_STRINGS_LEN+1] ;
	u8 fru_file_id_len ;
	u8 fru_file_id[IPMI_STRINGS_LEN+1] ;
	// контрольную сумму не реализуем
} IPMI_ProductInfo ;

typedef struct __IPMI_MultiRecordHeader {
	u8 type_id ;
	u8 end_n_format ;
	u8 length ;
	u8 record_checksum ;
	u8 header_checksum ;
} IPMI_MultiRecordHeader ;

typedef enum {
	IPMI_MRT_POWER_SUPPLY = 0 ,
	IPMI_MRT_DC_OUTPUT = 1,
	IPMI_MRT_DC_LOAD = 2,
	IPMI_MRT_MANAGEMENT_ACCESS_RECORD = 3,
	IPMI_MRT_BASE_COMPATIBILITY_RECORD = 4,
	IPMI_MRT_EXTENDED_COMPATIBILITY_RECORD = 5
} IPMI_MultiRecordHeader_types_t ;

typedef struct __IPMI_MultiRecord_PowerSupplyInformation {
	u16 overall_capacity ;
	u16 peak_va ;
	u8 inrush_current ;
	u8 inrush_interval_ms ;
	s16 low_end_input_voltage_range_1 ;
	s16 high_end_input_voltage_range_1 ;
	s16 low_end_input_voltage_range_2 ;
	s16 high_end_input_voltage_range_2 ;

	u8 low_end_input_frequency_range ;
	u8 high_end_input_frequency_range ;
	u8 input_dropout_tolerance_ms ;
	u8 binary_flags ;

	u16 peak_watt ;
	u8 combined_wattage[3] ;

	u8 predictive_fail_tach ;
} IPMI_MultiRecord_PowerSupplyInformation ;

#define IPMI_FRU_MULTIRECORDS_MAX 	4

typedef struct __IPMI_FRU {
	IPMI_CommonHeader 		header ;
	IPMI_ProductInfo 		product_info ;
	IPMI_MultiRecordHeader 	multirecord_header[IPMI_FRU_MULTIRECORDS_MAX] ;
} IPMI_FRU ;

//! Читает Common Header и затем Product Info заголовок раздела FRU.
_BOOL IPMI_Read_FRU_Headers( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_FRU* fru );
//! Читает Product Info Area.
_BOOL IPMI_Read_ProductInfo( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, IPMI_ProductInfo* product_info );
_BOOL IPMI_Read_CommonHeader( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_CommonHeader* header );
_BOOL IPMI_Find_n_Read_PSU_MultiArea( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, u8 m2Addr, IPMI_FRU* fru,
				IPMI_MultiRecordHeader* multi_header, IPMI_MultiRecord_PowerSupplyInformation *psu_area );

//! Прошивает EEPROM в PSU в соответствии с fru.
_BOOL IPMI_Write_FRU( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_FRU* fru,
					IPMI_MultiRecordHeader* multi_header, IPMI_MultiRecord_PowerSupplyInformation *psu_area );

_BOOL ipmi_read_common_header
(
  PMB_PeriphInterfaceTypedef  *pmb_bus,     /*!< [in]   таблица функций обратного вызова для транзакций по шине */
  u8                          mAddr,        /*!< [in]   адрес I2C EEPROM блока питания                          */
  IPMI_FRU                    *fru          /*!< [out]  структура для считываемых параметров                    */ 
);

_BOOL ipmi_read_product_info
(
  PMB_PeriphInterfaceTypedef  *pmb_bus,     /*!< [in]   таблица функций обратного вызова для транзакций по шине */
  u8                          mAddr,        /*!< [in]   адрес I2C EEPROM блока питания                          */
  u8                          m2Addr,       /*!< [in] адрес мастер-контроллера БП на i2c шине                     */  
  IPMI_FRU                    *fru          /*!< [out]  структура для считываемых параметров                    */ 
);

_BOOL ipmi_get_adress_acknowledge
(
  PMB_PeriphInterfaceTypedef  *pmb_bus,     /*!< [in]   таблица функций обратного вызова для транзакций по шине */
  u8                          mAddr         /*!< [in]   адрес I2C EEPROM блока питания                          */
);

#endif // IPMI_H__
