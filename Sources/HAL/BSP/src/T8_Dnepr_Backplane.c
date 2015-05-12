#include "support_common.h"
#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/IC/inc/PSU_Driver.h"
#include "HAL/MCU/inc/PMBus_GenericDriver.h"
#include "HAL/MCU/inc/SPI_GenericDriver.h"
#include "HAL/IC/inc/ST_M95M01.h"
#include "HAL/IC/inc/PSMI_Registers.h"
#include "HAL/IC/inc/PSU_Driver.h"
#include "HAL/IC/inc/LT_LTC4222.h"
#include "HAL/IC/inc/AT_AT24C512.h"
#include "HAL/IC/inc/TI_TCA9539.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"

#define TCA9539_PSU_ADDR		0xEC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! состояние сигнальных выводов обоих блоков питания
static T8_Dnepr_PsStatusTypedef __ps_status ;

static PSU_UnitInfoTypedef 	__psu_info[ I2C_DNEPR_NUMBER_OF_PSU ];
static PSU_UnitMeasurements __psu_measues[ I2C_DNEPR_NUMBER_OF_PSU ];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Dnepr_Backplane_Init()
{

	MAX31785_InitPeripherialInterface( Dnepr_I2C_Get_PMBUS_EXT_Driver( I2C_DNEPR_FAN_SLOT_NUM ), PMB_MAX31785_ADDR_00 );
	PSU_InitPeripherialInterface( Dnepr_I2C_Get_PMBUS_PSU_Driver() );

	TCA9539_SetDirectionRead( Dnepr_I2C_Get_PMBUS_INT_Driver(), TCA9539_PSU_ADDR, 0xFC3F );
	Dnepr_Reload_PSU_Status_Pins() ; // снимаем прерывание
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL Dnepr_Reload_PSU_Status_Pins()
{
	size_t i ;
	u16 value ;
	for( i = 0; i < 3; i++ ){
		if( TCA9539_ReadGPIO( Dnepr_I2C_Get_PMBUS_INT_Driver(), TCA9539_PSU_ADDR, &value ) ){
			break ;
		}
	}
	if( i >= 3 ){
		return FALSE ;
	}
__ps_status.tPs1.bSeated = (value & 0x0400) > 0 ; 
__ps_status.tPs1.bPowerGood = __ps_status.tPs1.bTempOk = __ps_status.tPs1.bInOk   = (value & 0x0800) > 0 ;   
__ps_status.tPs2.bSeated = (value & 0x2000) > 0 ;
__ps_status.tPs2.bPowerGood = __ps_status.tPs2.bTempOk = __ps_status.tPs2.bInOk   = (value & 0x4000) > 0 ;

__ps_status.tPs3.bSeated = (value & 0x0020) > 0 ;
__ps_status.tPs3.bPowerGood = __ps_status.tPs3.bTempOk = __ps_status.tPs3.bInOk   = (value & 0x0010) > 0 ;
__ps_status.tPs4.bSeated = (value & 0x0002) > 0 ;
__ps_status.tPs4.bPowerGood = __ps_status.tPs4.bTempOk = __ps_status.tPs4.bInOk   = (value & 0x0004) > 0 ;
        

	return TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static _BOOL __ps1_seated = FALSE ;
static _BOOL __ps2_seated = FALSE ;
static _BOOL __ps3_seated = FALSE ;
static _BOOL __ps4_seated = FALSE ;

//! \brief перечитывает состояние выводов БП и параметры по PMBus \ SMI
//! \retval возвращает TRUE если установленные БП ответили
_BOOL Dnepr_Backplane_Reload_PSU_Info()
{
	_BOOL ret = TRUE ;
	_BOOL btest = TRUE ;
	size_t i ;
	for( i = 0; i < 3; i++ ){
		if( Dnepr_Reload_PSU_Status_Pins() ){
			break;
		}
	}
	// не удалось прочитать состояния выводов БП
	if( i >= 3 ){
		return FALSE ;
	}

	// PSU1 вставлен
	if( __ps_status.tPs1.bSeated == 0 ){
		// перечитываем коэф.-ты
		btest = PSU_Setup( 1, &__psu_info[0] );
		ret = ret && btest ;
		__ps1_seated = TRUE ;
	// PSU1 вынули
	} else if ( __ps_status.tPs1.bSeated == 1 ){
		// сбрасываем коэф.-ты и пр.
		__ps1_seated = FALSE ;
		PSU_ClearUnit( 1 );
	}
	// PSU2 вставлен
	if( __ps_status.tPs2.bSeated == 0 ){
		btest = PSU_Setup( 2, &__psu_info[1] );
		ret = ret && btest ;
		__ps2_seated = TRUE ;
	// PSU2 вынули
	} else if ( __ps_status.tPs2.bSeated == 1 ){
		__ps2_seated = FALSE ;
		// сбрасываем коэф.-ты и пр.
		PSU_ClearUnit( 2 );
	}
	if( __ps_status.tPs3.bSeated == 0 ){
		btest = PSU_Setup( 3, &__psu_info[2] );
		ret = ret && btest ;
		__ps3_seated = TRUE ;
	// PSU2 вынули
	} else if ( __ps_status.tPs2.bSeated == 1 ){
		__ps3_seated = FALSE ;
		// сбрасываем коэф.-ты и пр.
		PSU_ClearUnit( 3 );
	}
	if( __ps_status.tPs4.bSeated == 0 ){
		btest = PSU_Setup( 4, &__psu_info[3] );
		ret = ret && btest ;
		__ps4_seated = TRUE ;
	// PSU2 вынули
	} else if ( __ps_status.tPs2.bSeated == 1 ){
		__ps4_seated = FALSE ;
		// сбрасываем коэф.-ты и пр.
		PSU_ClearUnit( 4 );
	}
	return ret ;
}

//! \brief обновляет показания блоков питания
void Dnepr_Backplane_Reload_PSU_DynInfo()
{
	if( __ps1_seated )
		PSU_GetUnitMeasurements( 1, &__psu_measues[0] );
	if( __ps2_seated )
		PSU_GetUnitMeasurements( 2, &__psu_measues[1] );
	if( __ps3_seated )
		PSU_GetUnitMeasurements( 3, &__psu_measues[2] );
	if( __ps4_seated )
		PSU_GetUnitMeasurements( 4, &__psu_measues[3] );
}

//! \brief getter для __ps_status
const T8_Dnepr_PsStatusTypedef* Dnepr_Backplane_GetPSU_Status()
{
	return &__ps_status ;
}

//! \brief getter для __psu_measues, учитывающий вставленность бп
//! \param num номер БП 0 -- верхний, 1 -- нижний
const PSU_UnitMeasurements* Dnepr_Backplane_GetPSU_Measures(const u8 num)
{
	if((num == 0) && (__ps1_seated == TRUE)){
		return &__psu_measues[ num ];
	} else if((num == 1) && (__ps2_seated == TRUE)){
		return &__psu_measues[ num ];
	} else if((num == 2) && (__ps3_seated == TRUE)){
		return &__psu_measues[ num ];
	} else if((num == 3) && (__ps4_seated == TRUE)){
		return &__psu_measues[ num ];
	} else {
		return NULL ;
	}
}

const PSU_UnitInfoTypedef* Dnepr_Backplane_GetPSU_Info( const u32 num )
{
	// берём присутствие бп из __ps*_seated, чтобы быть уверенными, что 
	// параметры успели перечитать
	if((num == 0) && (__ps1_seated == TRUE)){
		return &__psu_info[ num ];
	} else if((num == 1) && (__ps2_seated == TRUE)){
		return &__psu_info[ num ];
	} else if((num == 2) && (__ps3_seated == TRUE)){
		return &__psu_info[ num ];
	} else if((num == 3) && (__ps4_seated == TRUE)){
		return &__psu_info[ num ];
	} else {
		return NULL ;
	}
}

_BOOL Dnepr_Backplane_PS1_Present()
{
	return __ps1_seated ;
}

_BOOL Dnepr_Backplane_PS2_Present()
{
	return __ps2_seated ;
}

_BOOL Dnepr_Backplane_PS3_Present()
{
	return __ps3_seated ;
}

_BOOL Dnepr_Backplane_PS4_Present()
{
	return __ps4_seated ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// серийный номер на бекплейне

_BOOL Dnepr_ReadDS28CM00_Backplane(MAX_DS28CM00_ContentsTypedef* tContents)
{
	_BOOL ret ;
	ret = MAX_DS28CM00_ReadContents( Dnepr_BP_DS28CM00_I2C_handle(), tContents );
	
	return ret ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

I2C_PeriphInterfaceTypedef *Dnepr_BP_DS28CM00_I2C_handle()
{
	return Dnepr_I2C_Get_I2C_BP_SerNum_Driver() ;
}
