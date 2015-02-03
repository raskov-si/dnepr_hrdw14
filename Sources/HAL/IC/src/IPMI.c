/*!
\file IPMI.c
\brief –абота с IPMI v1.0 (дл€ чтени€ параметров блоков питани€).
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#include "HAL/IC/inc/PMB_interface.h"
#include "HAL/IC/inc/IPMI.h"
#include "uCOS_II.H"
#include "Binary.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
	IPMI_STR_TYPE_BIN = 0,
	IPMI_STR_TYPE_BCD = 1,
	IPMI_STR_TYPE_6bASCII = 2,
	IPMI_STR_TYPE_LANG = 3
} IPMI_StrType_t ;

static u8 __ipmi_len_2_type_len( IPMI_StrType_t type, const size_t len );
static size_t __ipmi_len_type_2_len( u8 type_n_len, IPMI_StrType_t* type );

static _BOOL __write_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 len, u8* str );
static _BOOL __read_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 *len, u8* str );

static _BOOL __PMB_WriteMultipleBytes(PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u8* anData, u8 len );
static _BOOL __PMB_ReadMultipleBytes(PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u8* anData, u8 len );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL IPMI_Read_CommonHeader( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_CommonHeader* header )
{
	OSTimeDly( 50 );
	return __PMB_ReadMultipleBytes( pmb_bus, mAddr, IPMI_COMMONHEADER_ADDR, (u8*)header, sizeof(IPMI_CommonHeader) );
}

_BOOL IPMI_Read_ProductInfo( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, IPMI_ProductInfo* product_info )
{
	size_t i_offset = offset ;
	size_t len_2_read ;
	_BOOL ret = TRUE ;
	
	// читаем IPMI_ProductInfo до первой строки (потому что длина строки становитс€ известна только когда прочитаем)
	len_2_read = (u8*)&product_info->manufacturer_name_length - (u8*)product_info ;
	OSTimeDly( 50 );
	ret = ret && __PMB_ReadMultipleBytes( pmb_bus, mAddr, i_offset, (u8*)product_info, len_2_read );
	
	i_offset += len_2_read ;
	ret = ret && __read_sz_n_str( pmb_bus, mAddr, i_offset, &product_info->manufacturer_name_length, (u8*)&product_info->manufacturer_name );
	i_offset += product_info->manufacturer_name_length + 1 ;
	ret = ret && __read_sz_n_str( pmb_bus, mAddr, i_offset, &product_info->name_length, (u8*)&product_info->name );
	i_offset += product_info->name_length + 1 ;
	ret = ret && __read_sz_n_str( pmb_bus, mAddr, i_offset, &product_info->model_length, (u8*)&product_info->model );
	i_offset += product_info->model_length + 1 ;
	ret = ret && __read_sz_n_str( pmb_bus, mAddr, i_offset, &product_info->serial_number_len, (u8*)&product_info->serial_number );
	i_offset += product_info->serial_number_len + 1 ;
	ret = ret && __read_sz_n_str( pmb_bus, mAddr, i_offset, &product_info->asset_tag_len, (u8*)&product_info->asset_tag );
	i_offset += product_info->asset_tag_len + 1 ;
	ret = ret && __read_sz_n_str( pmb_bus, mAddr, i_offset, &product_info->fru_file_id_len, (u8*)&product_info->fru_file_id );
	i_offset += product_info->fru_file_id_len + 1 ;

	return ret ;
}

//! находит в FRU MultiRecord про PSU и читает еЄ
_BOOL IPMI_Find_n_Read_PSU_MultiArea( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_FRU* fru,
				IPMI_MultiRecordHeader* multi_header, IPMI_MultiRecord_PowerSupplyInformation *psu_area )
{
	_BOOL ret = TRUE ;
	size_t offset ;
	size_t nAreas = 0 ;
	assert( pmb_bus );
	assert( fru );	
	assert( multi_header );
	assert( psu_area );

	offset = fru->header.multirecord_area_offset * 8 ;
	// ищем запись про блоки питани€
	do {
		OSTimeDly( 50 );
		ret = ret && __PMB_ReadMultipleBytes( pmb_bus, mAddr, offset, (u8*)multi_header, sizeof(IPMI_MultiRecordHeader) );
		offset += sizeof(IPMI_MultiRecordHeader) + multi_header->length ;
		++nAreas ;
	} while(((multi_header->end_n_format & 0x80) == 0) && (nAreas < 10) && (multi_header->type_id != IPMI_MRT_POWER_SUPPLY)) ;
	if(multi_header->type_id == IPMI_MRT_POWER_SUPPLY){
		offset -= multi_header->length ;
		OSTimeDly( 50 );
		ret = ret && __PMB_ReadMultipleBytes( pmb_bus, mAddr, offset, (u8*)psu_area, sizeof(IPMI_MultiRecord_PowerSupplyInformation) );
		offset += multi_header->length ;
		// переворачиваем все многобайтовые пол€
		#define SWAP_BYTES_16W(x)   x = (_WORD(_LSB(x), _MSB(x) ))
		SWAP_BYTES_16W(psu_area->overall_capacity );
		SWAP_BYTES_16W(psu_area->peak_va );
		// XXX: динамически определ€ем пор€док байт (если напр€жение больше 300 ¬ или меньше 0 -- мен€ем его)
		if((psu_area->low_end_input_voltage_range_1 > 30000) || (psu_area->low_end_input_voltage_range_1 < 0)){
			SWAP_BYTES_16W(psu_area->low_end_input_voltage_range_1 );
		}
		if((psu_area->high_end_input_voltage_range_1 > 30000) || (psu_area->high_end_input_voltage_range_1 < 0)){
			SWAP_BYTES_16W(psu_area->high_end_input_voltage_range_1 );
		}
		if((psu_area->low_end_input_voltage_range_2 > 300) || (psu_area->low_end_input_voltage_range_2 < 0)){
			SWAP_BYTES_16W(psu_area->low_end_input_voltage_range_2 );
		}
		if((psu_area->high_end_input_voltage_range_2 > 300) || (psu_area->high_end_input_voltage_range_2 < 0)){
			SWAP_BYTES_16W(psu_area->high_end_input_voltage_range_2 );
		}
		SWAP_BYTES_16W(psu_area->peak_watt );
	}
	return ret ;
}

_BOOL IPMI_Read_FRU_Headers( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_FRU* fru )
{
	_BOOL ret = TRUE ;
	assert( pmb_bus );
	assert( fru );
	// читаем Common Header
	OSTimeDly( 50 );
	ret = ret && __PMB_ReadMultipleBytes( pmb_bus, mAddr, IPMI_COMMONHEADER_ADDR, (u8*)&fru->header, sizeof(IPMI_CommonHeader) );
	// читаем Product Info
	if( fru->header.product_area_offset != 0 ){
		ret = ret && IPMI_Read_ProductInfo( pmb_bus, mAddr, fru->header.product_area_offset*8, &fru->product_info );
	}
	return ret ;
}

_BOOL IPMI_Write_FRU( PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, IPMI_FRU* fru,
					IPMI_MultiRecordHeader* multi_header, IPMI_MultiRecord_PowerSupplyInformation *psu_area )
{
	size_t i_offset = IPMI_COMMONHEADER_ADDR ;
	size_t i ;
	_BOOL ret = TRUE ;
	assert( pmb_bus );
	assert( fru );

	// ѕишем Common Header
	OSTimeDly( 5 );
	ret = ret && __PMB_WriteMultipleBytes( pmb_bus, mAddr, i_offset, (u8*)&fru->header, sizeof(IPMI_CommonHeader) );

	// ѕишем Product Info
	OSTimeDly( 5 );
	i_offset = fru->header.product_area_offset*8 ;
	// пишем до первой строки
	ret = ret && __PMB_WriteMultipleBytes( pmb_bus, mAddr, 
			i_offset, (u8*)&fru->product_info,
			(u8*)&fru->product_info.manufacturer_name_length - (u8*)&fru->product_info );
	OSTimeDly( 5 );

	i_offset += (u8*)&fru->product_info.manufacturer_name_length - (u8*)&fru->product_info ;

	ret = ret && __write_sz_n_str( pmb_bus, mAddr, i_offset, fru->product_info.manufacturer_name_length, fru->product_info.manufacturer_name );
	
	i_offset += fru->product_info.manufacturer_name_length + 1 ;
	ret = ret && __write_sz_n_str( pmb_bus, mAddr, i_offset, fru->product_info.name_length, fru->product_info.name );
	i_offset += fru->product_info.name_length + 1 ;
	ret = ret && __write_sz_n_str( pmb_bus, mAddr, i_offset, fru->product_info.model_length, fru->product_info.model );
	i_offset += fru->product_info.model_length + 1 ;
	ret = ret && __write_sz_n_str( pmb_bus, mAddr, i_offset, fru->product_info.serial_number_len, fru->product_info.serial_number );
	i_offset += fru->product_info.serial_number_len + 1 ;
	ret = ret && __write_sz_n_str( pmb_bus, mAddr, i_offset, fru->product_info.asset_tag_len, fru->product_info.asset_tag );
	i_offset += fru->product_info.asset_tag_len + 1 ;
	ret = ret && __write_sz_n_str( pmb_bus, mAddr, i_offset, fru->product_info.fru_file_id_len, fru->product_info.fru_file_id );
	i_offset += fru->product_info.fru_file_id_len + 1 ;

	i_offset = fru->header.multirecord_area_offset * 8 ;
	// пишем multi header area Ѕѕ
	ret = ret && __PMB_WriteMultipleBytes( pmb_bus, mAddr, 
			i_offset, (u8*)multi_header, sizeof(IPMI_MultiRecordHeader) );
	i_offset += sizeof( IPMI_MultiRecordHeader );
	// пишем собсно информацию о Ѕѕ
	ret = ret && __PMB_WriteMultipleBytes( pmb_bus, mAddr, 
			i_offset, (u8*)psu_area, sizeof(IPMI_MultiRecord_PowerSupplyInformation) );

	return ret ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static size_t __ipmi_len_type_2_len( u8 type_n_len, IPMI_StrType_t* type )
{
	if( type ){
		switch( (type_n_len & 0xC0) >> 6 ){
			case 0:
			*type = IPMI_STR_TYPE_BIN ;
			break ;
			case 1:
			*type = IPMI_STR_TYPE_BCD ;
			break ;
			case 2:
			*type = IPMI_STR_TYPE_6bASCII ;
			break ;
			case 3:
			*type = IPMI_STR_TYPE_LANG ;
			break ;
		}
	}
	return type_n_len & 0x3F ;
}

static u8 __ipmi_len_2_type_len( IPMI_StrType_t type, const size_t len )
{
	u8 res = 0 ;

	res = ((u8)type) << 6 ;
	res |= (len & 0x3F) ;

	return res ;
}

// читает побайтно из адреса mAddr и смещени€ offset сначала байт с длиной строки, сразу следом строку
static _BOOL __read_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 *len, u8* str)
{
	_BOOL ret ;
	IPMI_StrType_t type ;

	OSTimeDly( 50 );
	ret = pmb_bus->PMB_ReadByte( pmb_bus, mAddr, offset, len );
	*len = __ipmi_len_type_2_len( *len, &type );
	// если тип строки не 3 (см раздел 13 документа Platform Management FRU Information Storage Definition)
	// разбирать его не умеем
	if(type != IPMI_STR_TYPE_LANG ){
		str[0] = 0 ;
	} else {
		OSTimeDly( 50 );
		ret = ret && __PMB_ReadMultipleBytes( pmb_bus, mAddr, offset+1, str, *len );
		str[ *len ] = 0 ;
	}
	return ret ;
}

static _BOOL __write_sz_n_str(PMB_PeriphInterfaceTypedef *pmb_bus, u8 mAddr, size_t offset, u8 len, u8* str)
{
	_BOOL ret ;
	u8 type_n_len ;

	type_n_len = __ipmi_len_2_type_len( IPMI_STR_TYPE_LANG, len );
	ret = __PMB_WriteMultipleBytes( pmb_bus, mAddr, offset, &type_n_len, 1 );
	ret = ret && __PMB_WriteMultipleBytes( pmb_bus, mAddr, offset+1, str, len );

	return ret ;
}

static _BOOL __PMB_WriteMultipleBytes(PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 offset, u8* anData, u8 len )
{
	_BOOL ret = TRUE ;
	u8 cur_len = 0 ;
	size_t cur_offset = offset ;
	size_t i = 0 ;
	do {
		// сколько пишем в текущей транзакции
		cur_len = MIN( ((cur_offset + 8) & 0xF8) - cur_offset, len - cur_len );

		ret = ret && p->PMB_WriteMultipleBytes( p, mAddr, cur_offset, &anData[ i ], cur_len );
		OSTimeDly( 5 );
		if( !ret ){
			++i ;
			--i ;
		}
		i += cur_len ;
		cur_offset += cur_len ;
	} while( (cur_offset - offset) < len );

	return ret ;
}

static _BOOL __PMB_ReadMultipleBytes(PMB_PeriphInterfaceTypedef* p, u8 mAddr, u8 mCmd, u8* anData, u8 len )
{
	return p->PMB_ReadMultipleBytes( p, mAddr, mCmd, anData, len );
}
