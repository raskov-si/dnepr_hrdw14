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
//! ��������� ���������� ������� ����� ������ �������
static T8_Dnepr_PsStatusTypedef __ps_status ;

static PSU_UnitInfoTypedef 	__psu_info[ I2C_DNEPR_NUMBER_OF_PSU ];
static PSU_UnitMeasurements __psu_measues[ I2C_DNEPR_NUMBER_OF_PSU ];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Dnepr_Backplane_Init()
{

	MAX31785_InitPeripherialInterface( Dnepr_I2C_Get_PMBUS_EXT_Driver( I2C_DNEPR_FAN_SLOT_NUM ), PMB_MAX31785_ADDR_00 );
	PSU_InitPeripherialInterface( Dnepr_I2C_Get_PMBUS_PSU_Driver() );

	TCA9539_SetDirectionRead( Dnepr_I2C_Get_PMBUS_INT_Driver(), TCA9539_PSU_ADDR, 0xFC3F );
	Dnepr_Reload_PSU_Status_Pins() ; // ������� ����������
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
	__ps_status.tPs1.bInOk = (value & 0x0800) > 0 ;
	__ps_status.tPs1.bPowerGood = (value & 0x1000) > 0 ;
	__ps_status.tPs1.bFanFail = (value & 0x2000) > 0 ;
	__ps_status.tPs1.bTempOk = (value & 0x4000) > 0 ;

	__ps_status.tPs2.bSeated = (value & 0x0020) > 0 ;
	__ps_status.tPs2.bInOk = (value & 0x0010) > 0 ;
	__ps_status.tPs2.bPowerGood = (value & 0x0008) > 0 ;
	__ps_status.tPs2.bFanFail = (value & 0x0002) > 0 ;
	__ps_status.tPs2.bTempOk = (value & 0x0004) > 0 ;

	return TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static _BOOL __ps1_seated = FALSE ;
static _BOOL __ps2_seated = FALSE ;

//! \brief ������������ ��������� ������� �� � ��������� �� PMBus \ SMI
//! \retval ���������� TRUE ���� ������������� �� ��������
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
	// �� ������� ��������� ��������� ������� ��
	if( i >= 3 ){
		return FALSE ;
	}

	// PSU1 ��������
	if( __ps_status.tPs1.bSeated == 0 ){
		// ������������ ����.-��
		btest = PSU_Setup( 1, &__psu_info[0] );
		ret = ret && btest ;
		__ps1_seated = TRUE ;
	// PSU1 ������
	} else if ( __ps_status.tPs1.bSeated == 1 ){
		// ���������� ����.-�� � ��.
		__ps1_seated = FALSE ;
		PSU_ClearUnit( 1 );
	}
	// PSU2 ��������
	if( __ps_status.tPs2.bSeated == 0 ){
		btest = PSU_Setup( 2, &__psu_info[1] );
		ret = ret && btest ;
		__ps2_seated = TRUE ;
	// PSU2 ������
	} else if ( __ps_status.tPs2.bSeated == 1 ){
		__ps2_seated = FALSE ;
		// ���������� ����.-�� � ��.
		PSU_ClearUnit( 2 );
	}
	return ret ;
}

//! \brief ��������� ��������� ������ �������
void Dnepr_Backplane_Reload_PSU_DynInfo()
{
	if( __ps1_seated )
		PSU_GetUnitMeasurements( 1, &__psu_measues[0] );
	if( __ps2_seated )
		PSU_GetUnitMeasurements( 2, &__psu_measues[1] );
}

//! \brief getter ��� __ps_status
const T8_Dnepr_PsStatusTypedef* Dnepr_Backplane_GetPSU_Status()
{
	return &__ps_status ;
}

//! \brief getter ��� __psu_measues, ����������� ������������� ��
//! \param num ����� �� 0 -- �������, 1 -- ������
const PSU_UnitMeasurements* Dnepr_Backplane_GetPSU_Measures(const u8 num)
{
	if((num == 0) && (__ps1_seated == TRUE)){
		return &__psu_measues[ num ];
	} else if((num == 1) && (__ps2_seated == TRUE)){
		return &__psu_measues[ num ];
	} else {
		return NULL ;
	}
}

const PSU_UnitInfoTypedef* Dnepr_Backplane_GetPSU_Info( const u32 num )
{
	// ���� ����������� �� �� __ps*_seated, ����� ���� ����������, ��� 
	// ��������� ������ ����������
	if((num == 0) && (__ps1_seated == TRUE)){
		return &__psu_info[ num ];
	} else if((num == 1) && (__ps2_seated == TRUE)){
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// �������� ����� �� ���������

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
