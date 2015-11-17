/*!
\file T8_Dnepr_SlotEEPROM.c
\brief Код записи и чтения структур в EEPROM слотовых устройств.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date mar 2014
*/


#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_SlotEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/IC/inc/AT_AT24C512.h"

#include "Threads/inc/threadDeviceController.h"
#include "common_lib/crc.h"

#include <string.h>
#include <stdio.h>
#include <float.h>
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "support_common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// данные из eeprom слотовых устрройств
#pragma data_alignment=4
static SlotHSEEPROMParams_t __slot_eeprom_data[ I2C_DNEPR_NUMBER_OF_SLOTS ];
// доступность основной записи в eeprom
#pragma data_alignment=4
static DControl_EEPROM_Status_t __slot_eeprom_status[ I2C_DNEPR_NUMBER_OF_SLOTS ];

//! доступность дополнительной записи в eeprom
static Dnepr_SlotEEPROM_Optional_Status_t __opt_eeprom_status[ I2C_DNEPR_NUMBER_OF_SLOTS ];
//! данные дополнительной секции в eeprom
static Dnepr_SlotEEPROM_Optional_t __opt_eeprom_data[ I2C_DNEPR_NUMBER_OF_SLOTS ];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static _BOOL __read_slot_eeprom( const u8 slot_num, SlotHSEEPROMParams_t *pParams );
static _BOOL __write_slot_eeprom( const u8 slot_num, SlotHSEEPROMParams_t *pParams );

static _BOOL __secondary_eeprom_write( const u32 slot_num,
							Dnepr_SlotEEPROM_Optional_t* pData );
static _BOOL __secondary_eeprom_clear( const u32 slot_num );
static _BOOL __secondary_eeprom_read( const u32 slot_num,
							Dnepr_SlotEEPROM_Optional_t* pData );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_SlotEEPROM_Init()
{
	u32 i ;

	for( i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; ++i ){
		__opt_eeprom_status[ i ] = EEPROMSLOT_OPTIONAL_UNAVAILABLE ;
	}
}

void Dnepr_SlotEEPROMWrite( const u32 slot_num, SlotHSEEPROMParams_t *pSlotParam )
{
	assert( pSlotParam );
	if( __write_slot_eeprom( slot_num, pSlotParam )){
		if( Dnepr_SlotEEPROM_Read( slot_num )){
//			Dnepr_DControl_ReinitHotswaps() ;
                    topwmng_msg_crate_change();
		}
	}
}




_BOOL Dnepr_SlotEEPROM_Read( const u32 slot_num )
{
	u32 j ;

	if( slot_num >= I2C_DNEPR_NUMBER_OF_SLOTS ){
		return FALSE ;
	}

	// читаем EEPROM
	// несколько раз для надёжности
	for( j = 0; j < 5; j++ ){
		__slot_eeprom_status[slot_num] = ( __read_slot_eeprom( slot_num, &__slot_eeprom_data[slot_num] ) 
												? EEPROMSLOT_OK : EEPROMSLOT_UNAVAILABLE );
		if( __slot_eeprom_status[slot_num] == EEPROMSLOT_OK ){
			__opt_eeprom_status[ slot_num ] = (__secondary_eeprom_read( slot_num, &__opt_eeprom_data[slot_num] )
						? EEPROMSLOT_OPTIONAL_AVAILABLE : EEPROMSLOT_OPTIONAL_UNAVAILABLE) ;
			break ;
		} else {
			OSTimeDly( 1 );
		}
	}
	return __slot_eeprom_status[slot_num] == EEPROMSLOT_OK ;
}

SlotHSEEPROMParams_t* Dnepr_SlotEEPROM_val( const u32 slot_num )
{
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			if(  __slot_eeprom_status[ slot_num ] == EEPROMSLOT_OK ){
				return &__slot_eeprom_data[ slot_num ] ;
			}
		}
	}
	return NULL ;
}

_BOOL Dnepr_SlotEEPROM_Available( const u32 slot_num )
{
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			return __slot_eeprom_status[ slot_num ] == EEPROMSLOT_OK ;
		}
	}
	return FALSE ;
}

const s8* Dnepr_SlotEEPROM_DeviceName( const u32 slot_num )
{
	assert( slot_num < (I2C_DNEPR_NUMBER_OF_SLOTS) );
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			if(  __slot_eeprom_status[ slot_num ] == EEPROMSLOT_OK ){
				return __slot_eeprom_data[ slot_num ].sName ;
			}
		}
	}
//	return "Unavailable" ;
	return "" ;
}

u32 Dnepr_SlotEEPROM_SlotPower( const u32 slot_num )
{
	assert( slot_num < (I2C_DNEPR_NUMBER_OF_SLOTS) );
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			if(  __slot_eeprom_status[ slot_num ] == EEPROMSLOT_OK ){
				return __slot_eeprom_data[ slot_num ].dMaxPower ;
			}
		}
	}
	return 0 ;
}

f32 Dnepr_SlotEEPROM_3v3_Bypass( const u32 slot_num )
{
	assert( slot_num < (I2C_DNEPR_NUMBER_OF_SLOTS) );
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			if(  __slot_eeprom_status[ slot_num ] == EEPROMSLOT_OK ){
				return __slot_eeprom_data[ slot_num ].fHS_33_bypass ;
			}
		}
	}
	return FLT_MAX ;
}

f32 Dnepr_SlotEEPROM_12v0_Bypass( const u32 slot_num )
{
	assert( slot_num < (I2C_DNEPR_NUMBER_OF_SLOTS) );
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			if(  __slot_eeprom_status[ slot_num ] == EEPROMSLOT_OK ){
				return __slot_eeprom_data[ slot_num ].fHS_12_bypass ;
			}
		}
	}
	return FLT_MAX ;
}

_BOOL Dnepr_SlotOptionalEEPROM_Write( const u32 slot_num, Dnepr_SlotEEPROM_Optional_t* pData )
{
	assert( pData );
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			return __secondary_eeprom_write( slot_num, pData );
		}
	}
	return FALSE ;
}

_BOOL Dnepr_SlotOptionalEEPROM_Clear( const u32 slot_num )
{
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			__opt_eeprom_status[ slot_num ] = EEPROMSLOT_OPTIONAL_UNAVAILABLE ;
			return __secondary_eeprom_clear( slot_num );
		}
	}
	return FALSE ;
}


_BOOL Dnepr_SlotOptionalEEPROM_Available( const u32 slot_num )
{
	assert( slot_num < (I2C_DNEPR_NUMBER_OF_SLOTS) );
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			return __opt_eeprom_status[ slot_num ] == EEPROMSLOT_OPTIONAL_AVAILABLE ;
		}
	}
	return FALSE ;
}

Dnepr_SlotEEPROM_Optional_t* Dnepr_SlotOptionalEEPROM_val( const u32 slot_num )
{
	assert( slot_num < (I2C_DNEPR_NUMBER_OF_SLOTS) );
	if( slot_num < I2C_DNEPR_NUMBER_OF_SLOTS ){
		if( Dnepr_DControl_SlotRawPresent()->bSlotPresent[ slot_num ] ){
			return &__opt_eeprom_data[ slot_num ];
		}
	}
	return NULL ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// читает параметры устройства из EEPROM и проверяет CRC, селект слота делается 
// в коде выше уровнем
static _BOOL __read_slot_eeprom( const u8 slot_num, SlotHSEEPROMParams_t *pParams )
{
	_BOOL result ;
	assert( pParams );
	if( slot_num > I2C_DNEPR_NUMBER_OF_SLOTS )
		return FALSE ;
	result = AT_AT24C512_ReadArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				SLOTEEPROM_ADDRESS,	(u8*)pParams, sizeof(SlotHSEEPROMParams_t) );
    pParams->sName[ SLOTEEPROM_SLOTNAME_LEN-1 ] = 0 ;

    // недоступна EEPROM
    if( !result )
    	goto __read_eeprom_exit ;

    // неверная контрольная сумма
    result = Crc32_mem( (u8*)pParams, sizeof(SlotHSEEPROMParams_t) - sizeof(u32), 0 ) == pParams->CRC ;

__read_eeprom_exit:    
    if( !result ){
    	pParams->sName[0] = 0 ;
    	pParams->dMaxPower = 0 ;
    }
	return result ;
}

// вычисляет CRC и пишет в EEPROM устройства в заданном слоте
static _BOOL __write_slot_eeprom( const u8 slot_num, SlotHSEEPROMParams_t *pParams )
{
	_BOOL result ;

	assert( pParams );
	if( slot_num > I2C_DNEPR_NUMBER_OF_SLOTS )
		return FALSE ;

	pParams->CRC = Crc32_mem( (u8*)pParams, sizeof(SlotHSEEPROMParams_t) - sizeof(u32), 0 );

	result = AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ),
					DNEPR_BACKPLANE_EEPROM_ADDR, SLOTEEPROM_ADDRESS, (u8*)pParams, sizeof(SlotHSEEPROMParams_t) );

	return result ;
}

static _BOOL __secondary_eeprom_read( const u32 slot_num,
							Dnepr_SlotEEPROM_Optional_t* pData )
{
	_BOOL result ;
	u32 crc ;
	assert( pData );
	assert( pData->pData );
	if( slot_num > I2C_DNEPR_NUMBER_OF_SLOTS )
		return FALSE ;

	result = AT_AT24C512_ReadArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR,	(u8*)(&pData->length), sizeof(u32) );

    // недоступна EEPROM
    if( !result ){
    	goto __read_passive_exit ;
    }
    // прочитаная длина больше допустимой
    if( pData->length > DNEPR_PASSIVE_STRUCT_MAXLEN ){
    	result = FALSE ;
    	goto __read_passive_exit ;
    }

    // читаем данные
	result = AT_AT24C512_ReadArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR+sizeof(u32),	(u8*)(pData->pData), pData->length );
	if( !result ){
    	goto __read_passive_exit ;
	}
	// читаем CRC 
	result = AT_AT24C512_ReadArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR+sizeof(u32)+pData->length, (u8*)(&pData->crc), sizeof(u32) );
	if( !result ){
    	goto __read_passive_exit ;
	}

    // проверяем контрольную сумму
    crc = Crc32_mem( (u8*)&pData->length, sizeof(u32), 0 );
    result = ( Crc32_mem( (u8*)pData->pData, pData->length, crc ) == pData->crc );
	if( !result ){
    	goto __read_passive_exit ;
	}

__read_passive_exit:    
	return result ;
}

static _BOOL __secondary_eeprom_write( const u32 slot_num,
							Dnepr_SlotEEPROM_Optional_t* pData )
{
	_BOOL result ;
	u32 crc ;
	assert( pData );
	assert( pData->pData );
	if( slot_num > I2C_DNEPR_NUMBER_OF_SLOTS )
		return FALSE ;

	result = AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR, (u8*)(&pData->length), sizeof(u32) );
    // недоступна EEPROM
    if( !result ){
    	goto __read_passive_exit ;
    }

    // пишем данные
	result = AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR+sizeof(u32),	(u8*)(pData->pData), pData->length );
	if( !result ){
    	goto __read_passive_exit ;
	}
	// вычисляем CRC
	crc = Crc32_mem( (u8*)&pData->length, sizeof(u32), 0 );
    pData->crc = Crc32_mem( (u8*)pData->pData, pData->length, crc ) ;
	// пишем CRC 
	result = AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR+sizeof(u32)+pData->length, (u8*)(&pData->crc), sizeof(u32) );

__read_passive_exit:    
	return result ;
}

static _BOOL __secondary_eeprom_clear( const u32 slot_num )
{
	_BOOL result ;
	u32 len = 0xFFFFFFFF ;
	if( slot_num > I2C_DNEPR_NUMBER_OF_SLOTS )
		return FALSE ;

	result = AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR, (u8*)(&len), sizeof(u32) );
    // недоступна EEPROM
    if( !result ){
    	goto __read_passive_exit ;
    }

    // портим данные
	result = AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR+sizeof(u32),	(u8*)(&len), sizeof(u32) );
	result = result & AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( slot_num ), DNEPR_BACKPLANE_EEPROM_ADDR,
				DNEPR_PASSIVE_STRUCT_ADDR+sizeof(u32)+sizeof(u32),	(u8*)(&len), sizeof(u32) );
	__read_passive_exit:    
	return result ;
}
