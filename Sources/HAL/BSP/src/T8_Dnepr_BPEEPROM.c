/*!
\file T8_Dnepr_BPEEPROM.c
\brief Код для работы с параметрами в EEPROM на backplane'е
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#include <string.h>
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/IC/inc/ST_M95M01.h"
#include "HAL/MCU/inc/T8_5282_cfm.h"
#include "support_common.h"
#include "common_lib/crc.h"
#include "common_lib/memory.h"
#include "Storage/inc/eeprom_storage.h"
#include "Profile/inc/pr_ch.h"
#include "Profile/inc/profile_processing.h"
#include "Profile/inc/sys.h"
#include "Profile/inc/extras.h"

static void __copy_bytes( const size_t src_addr, const size_t dst_addr, const size_t len );

static _BOOL __eeprom_present = FALSE ; //!< EEPROM виден на шине
static _BOOL __eeprom_1_ok = FALSE ;	//!< 1й раздел аппаратно в порядке
static _BOOL __eeprom_2_ok = FALSE ;	//!< 2й раздел аппаратно в порядке

//! описание разделов для форматирования
static STEEPROM_FormatParams __prime_format = {BPEEPROM_PRIMARY_PART_START,
				BPEEPROM_SECONDARY_PART_START-1, BPEEPROM_NAMELEN, BPEEPROM_TOTALSECTORS
				};
static STEEPROM_FormatParams __secondary_format = {BPEEPROM_SECONDARY_PART_START,
				BPEEPROM_END-1, BPEEPROM_NAMELEN, BPEEPROM_TOTALSECTORS
				};

static STEEPROM_HW_Interface __eeprom_iface = { ST_M95M01_ReadArray, 
												ST_M95M01_WriteArray,
												Crc32_mem };

static STEEPROM_PartitionHandler	__primary_part ;
static STEEPROM_PartitionHandler	__secondary_part ;


_BOOL Dnepr_BPEEPROM_CheckPresent()
{
    if( !ST_M95M01_CheckPresence() ){
	__eeprom_present = FALSE ;
	return FALSE ;
    } else {
	__eeprom_present = TRUE ;
    }  
}


_BOOL Dnepr_BPEEPROM_Init()
{
	_BOOL prm_success = FALSE ;
	_BOOL scnd_success = FALSE ;
	// Драйвер ST_M95M01 установлен в DNEPR_InitPeripherials()

	#ifdef __DEBUG
	return TRUE ;
	#endif

	if( !ST_M95M01_CheckPresence() ){
		__eeprom_present = FALSE ;
		return FALSE ;
	} else {
		__eeprom_present = TRUE ;
	}

	// здесь проверяется только заголовок и хеш-таблица
	prm_success = STEEPROM_OpenPartition( &__primary_part,
										BPEEPROM_PRIMARY_PART_START, &__eeprom_iface );
	// Проверяем что заголовок раздела корректный. В предыдущих версиях была ошибка:
	// при копировании исправного раздела в неисправный так же копировался заголовок.
	if( (__primary_part.part_params.partition_begin != BPEEPROM_PRIMARY_PART_START) ||
		(__primary_part.part_params.partition_end != BPEEPROM_SECONDARY_PART_START-1)){
		prm_success = FALSE ;
	}
	// проверяем все сектора
	if( prm_success ){
		prm_success = STEEPROM_CheckPartition( &__primary_part );
	}
	// так же для второго раздела
	scnd_success = STEEPROM_OpenPartition( &__secondary_part,
										BPEEPROM_SECONDARY_PART_START, &__eeprom_iface );
	// Проверяем что заголовок раздела корректный. В предыдущих версиях была ошибка:
	// при копировании исправного раздела в неисправный так же копировался заголовок.
	if( (__secondary_part.part_params.partition_begin != BPEEPROM_SECONDARY_PART_START) ||
		(__secondary_part.part_params.partition_end != BPEEPROM_END-1)){
		scnd_success = FALSE ;
	}
	if( scnd_success ){
		scnd_success = STEEPROM_CheckPartition( &__secondary_part );
	}

	// всё хорошо -- выходим
	if( prm_success && scnd_success ){
		__eeprom_1_ok = __eeprom_2_ok = TRUE ;
		return __eeprom_present = TRUE ;
	}
	// если всё плохо -- форматируем оба раздела, значения наполнятся потом при чтении
	if( !(prm_success || scnd_success) ){
		STEEPROM_Format( &__eeprom_iface, &__prime_format );
		STEEPROM_Format( &__eeprom_iface, &__secondary_format );
		prm_success = STEEPROM_OpenPartition( &__primary_part,
										BPEEPROM_PRIMARY_PART_START, &__eeprom_iface );
		scnd_success = STEEPROM_OpenPartition( &__secondary_part, 
										BPEEPROM_SECONDARY_PART_START, &__eeprom_iface );

	// копируем первичный во вторичный
	} else if(prm_success) {
		STEEPROM_Format( &__eeprom_iface, &__secondary_format );
		// т.к. второй впритык к первому -- его адрес является размером секторв
		__copy_bytes( BPEEPROM_PRIMARY_PART_START+STEEPROM_HEAD_SIZE, BPEEPROM_SECONDARY_PART_START+STEEPROM_HEAD_SIZE, 
				BPEEPROM_SECONDARY_PART_START-STEEPROM_HEAD_SIZE );
		scnd_success = STEEPROM_OpenPartition( &__secondary_part, 
								BPEEPROM_SECONDARY_PART_START, &__eeprom_iface );
	// копируем вторичный в первичный
	} else if(scnd_success) {
		STEEPROM_Format( &__eeprom_iface, &__prime_format );
		// т.к. второй впритык к первому -- его адрес является размером секторв
		__copy_bytes( BPEEPROM_SECONDARY_PART_START+STEEPROM_HEAD_SIZE, BPEEPROM_PRIMARY_PART_START+STEEPROM_HEAD_SIZE,
				BPEEPROM_SECONDARY_PART_START-STEEPROM_HEAD_SIZE );
		prm_success = STEEPROM_OpenPartition( &__primary_part, 
								BPEEPROM_PRIMARY_PART_START, &__eeprom_iface );
	}


	__eeprom_1_ok = prm_success ;
	__eeprom_2_ok = scnd_success ;
	return __eeprom_present ;
}

_BOOL Dnepr_BPEEPROM_Read(u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen  )
{
	_BOOL prm_success = FALSE ;
	_BOOL scnd_success = FALSE ;

	assert( sName );
	assert( pbData );
	assert( actual_len );

	if( !__eeprom_present ){
		return FALSE ;
	}

	// читаем из первичного раздела
	if( STEEPROM_IsPartitionOpened( &__primary_part ) && __eeprom_1_ok ){
		prm_success = STEEPROM_ReadRecord( &__primary_part, sName, pbData, actual_len, maxlen );
		if( *actual_len == 0 ){
			prm_success = FALSE ;
		}
	}
	// если не получилось, читаем из вторичного и пишем в первичный
	if( !prm_success ){
		if( STEEPROM_IsPartitionOpened( &__secondary_part ) && __eeprom_2_ok ){
			scnd_success = STEEPROM_ReadRecord( &__secondary_part, sName, pbData, actual_len, maxlen );
			// пишем в первичный раздел
			if( scnd_success && __eeprom_1_ok && (*actual_len > 0) ){
				STEEPROM_WriteRecord( &__primary_part, sName, pbData, *actual_len );
			} else if ( *actual_len == 0 ){
				scnd_success = FALSE ;
			}
		}
	}

	return (prm_success || scnd_success) ;
}

_BOOL Dnepr_BPEEPROM_Write(u8* sName, u8* pbData, const size_t actual_len )
{
	_BOOL prm_success = FALSE ;
	_BOOL scnd_success = FALSE ;
	u32 prm_crc, scnd_crc ;

	assert( sName );
	assert( pbData );

	if( !__eeprom_present ){
		return FALSE ;
	}

	// если оба раздела в порядке, решаем в какой очерёдности пишем в какой раздел
	if( __eeprom_1_ok && __eeprom_2_ok ){
		prm_success = STEEPROM_CheckValue( &__primary_part, sName, &prm_crc );
		scnd_success = STEEPROM_CheckValue( &__secondary_part, sName, &scnd_crc );
		// Eсли первый целостный, а второй нет, сначала пишем во вторичный раздел.
		if( prm_success && !scnd_success){
			scnd_success = STEEPROM_WriteRecord( &__secondary_part, sName, pbData, actual_len );
			prm_success = STEEPROM_WriteRecord( &__primary_part, sName, pbData, actual_len );

		// если присутствует и он одинаковый в обоих разделах, или не присутствует 
		// ни в одном-- пишем в первичный раздел
		} else {
			prm_success = STEEPROM_WriteRecord( &__primary_part, sName, pbData, actual_len );
			scnd_success = STEEPROM_WriteRecord( &__secondary_part, sName, pbData, actual_len );
		}
	// если в порядке только первый раздел
	} else if( __eeprom_1_ok ){
		prm_success = STEEPROM_WriteRecord( &__primary_part, sName, pbData, actual_len );
	// если в порядке только второй раздел
	} else if( __eeprom_2_ok ){
		scnd_success = STEEPROM_WriteRecord( &__secondary_part, sName, pbData, actual_len );
	}
	// если неудалось записать в первичный раздел -- портим его
	if( __eeprom_1_ok && (!prm_success) ){
		STEEPROM_SpoilPartition( &__primary_part );
		__eeprom_1_ok = FALSE ;
	}
	if( __eeprom_2_ok && (!scnd_success) ){
		STEEPROM_SpoilPartition( &__secondary_part );
		__eeprom_2_ok = FALSE ;
	}

	return ( prm_success || scnd_success );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL Dnepr_eeprom_format()
{
	_BOOL prm_success = FALSE ;
	_BOOL scnd_success = FALSE ;

	if( !ST_M95M01_CheckPresence() ){
		__eeprom_present = FALSE ;
		return FALSE ;
	}

	STEEPROM_Format( &__eeprom_iface, &__prime_format );
	STEEPROM_Format( &__eeprom_iface, &__secondary_format );

	prm_success = STEEPROM_OpenPartition( &__primary_part, 
										BPEEPROM_PRIMARY_PART_START, &__eeprom_iface );
	scnd_success = STEEPROM_OpenPartition( &__secondary_part, 
										BPEEPROM_SECONDARY_PART_START, &__eeprom_iface );
	// всё хорошо -- выходим
	if( prm_success && scnd_success ){
		return __eeprom_present = TRUE ;
	}

	return prm_success || scnd_success ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BPEEPROM_State Dnepr_eeprom_GetState()
{
	if( !__eeprom_present ){
		return BPEEPROM_NONE ;
	} else if( __eeprom_1_ok && __eeprom_2_ok ){
		return BPEEPROM_OK ;
	} else {
		return BPEEPROM_ERRORNEOUS ;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// низкоуровневое копирование из одного участка eeprom в другой
static void __copy_bytes( const size_t src_addr, const size_t dst_addr, const size_t len )
{
	size_t i ;
	u8 buff[ 128 ];

	for( i = 0; i < len; i += 32 ){
		ST_M95M01_ReadArray( buff, i+src_addr, MIN(32,len-i) );
		ST_M95M01_WriteArray( buff, i+dst_addr, MIN(32,len-i) );
	}
}
