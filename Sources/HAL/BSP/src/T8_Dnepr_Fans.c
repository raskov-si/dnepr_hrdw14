/*!
\file T8_Dnepr_Fans.c
\brief Процедуры для работы с платой вентиляции
\author <a href="mailto:leonov@t8.ru">baranovm@t8.ru</a>
\details Настройка интерфейса к MAX_31785 происходит в T8_Dnepr_Backplane
\date dec 2012
*/

/*******************************************************************************
| # вентилятора | # разъёма | # канала контроллера |
	1 				x9 			3
	2 				x10			5
	3 				x8			4
	4 				x7			1
	5 				x2			2
	6 				x1 			0

Разъёмы на плате (вид со стороны разъёмов):
|											|
| x10		_разъём_backplane'а_		x1 	|
|											|
| x9     x8              			x7  x2	|
| 											|

Физическое расположение вентиляторов:

|	разъём  |
| 1 | 3 | 5 |
| 2 | 4 | 6 |


// таймыр V6
| # вентилятора | # разъёма | # канала контроллера |
    1               x1          1
    2               x2          2
    3               x3          3
    4               x4          4

|   разъём  |
|  1     3  |
|  2     4  |

// таймыр мини 
| # вентилятора | # разъёма | # канала контроллера |
	1 				x1 			1
	2 				x2			2

|	разъём  |
|     1     |
|     2     |

*******************************************************************************/

#include "support_common.h"
#include "HAL/BSP/inc/T8_Dnepr_Fans.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/IC/inc/AT_AT24C512.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include <string.h>
#include <float.h>
#include "common_lib/crc.h"
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

static _BOOL	__fancal_read = FALSE ;
static Dnepr_Fans_CalData_t __fancal_data ;
static _BOOL __write_fan_eeprom( Dnepr_Fans_CalData_t *pFancal );
static void __wait_4_rpm_stab( const u8 fans_num );
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL	Dnepr_Fans_ReadCalibration()
{
	_BOOL result ;
	size_t i ;
	for( i = 0; i < 100; i++ ){
		result = AT_AT24C512_ReadArray( Dnepr_I2C_Get_I2C_BP_Driver( I2C_DNEPR_FAN_SLOT_NUM ), DNEPR_BACKPLANE_EEPROM_ADDR,
						Dnepr_Fans_CalData_t_STARTADDRESS,	(u8*)&__fancal_data, sizeof(Dnepr_Fans_CalData_t) );
		if( result ){
			break ;
		} else {
			OSTimeDly( 5 );
		}
	}
    // недоступна EEPROM
    if( !result ){
    	__fancal_read = FALSE ;
    	goto readcal_ret ;
    }

    __fancal_read = Crc32_mem( (u8*)&__fancal_data, sizeof(Dnepr_Fans_CalData_t) - sizeof(u32), 0 ) == __fancal_data.crc ;
readcal_ret:
	if( __fancal_read ){
		__fancal_data.vendor[ DNEPR_FANS_VENDOR_LEN-1 ] = 0 ;
		__fancal_data.hw_number[ DNEPR_FANS_HW_LEN-1 ] = 0 ;
		__fancal_data.pt_number[ DNEPR_FANS_PT_LEN-1 ] = 0 ;
		__fancal_data.sr_number[ DNEPR_FANS_SR_LEN-1 ] = 0 ;
		__fancal_data.fans_partnumber[ DNEPR_FANS_PARTNUM_LEN-1 ] = 0 ;
	    return __fancal_read ;
	} else {
		return FALSE ;
	}
}

_BOOL	Dnepr_Fans_Calibrate( const Dnepr_Fans_CalData_t* calib_data )
{
	assert( calib_data );
	
	__fancal_data.version = DNEPR_FANS_DATA_VERSION ;
	__fancal_data.max_work_hrs = calib_data->max_work_hrs ;
	__fancal_data.fans_num = calib_data->fans_num ;
	strncpy( (s8*)__fancal_data.vendor, (const s8*)calib_data->vendor, DNEPR_FANS_VENDOR_LEN );
	strncpy( (s8*)__fancal_data.hw_number, (const s8*)calib_data->hw_number, DNEPR_FANS_HW_LEN );
	strncpy( (s8*)__fancal_data.pt_number, (const s8*)calib_data->pt_number, DNEPR_FANS_PT_LEN );
	strncpy( (s8*)__fancal_data.sr_number, (const s8*)calib_data->sr_number, DNEPR_FANS_SR_LEN );
	strncpy( (s8*)__fancal_data.fans_partnumber, (const s8*)calib_data->fans_partnumber, DNEPR_FANS_PARTNUM_LEN-1 );
	__fancal_data.fans_partnumber[ DNEPR_FANS_PARTNUM_LEN-1 ] = '\0' ;
	__fancal_data.min_rpm = calib_data->min_rpm ;
	__fancal_data.max_rpm = calib_data->max_rpm ;

	__write_fan_eeprom( &__fancal_data );
	Dnepr_Fans_ReadCalibration();
	return TRUE ;
}

_BOOL	Dnepr_Fans_SetRPM( const u32 rpm )
{	
	size_t i ;
	f32 fRPM ;	
	if( __fancal_read ){
		if( rpm > __fancal_data.max_rpm ){
			fRPM = (f32)__fancal_data.max_rpm ;
		} else if( rpm < __fancal_data.min_rpm ){
			fRPM = (f32)__fancal_data.min_rpm ;
		} else {
			fRPM = (f32) rpm ;
		}
		if( __fancal_data.fans_num == 2 ){
			for( i = 1; i <= 2; i++ ){
				MAX31785_SetSpeedFaultLimitRPM( i, __fancal_data.min_rpm );
				MAX31785_SetManualRPM( i ); 
				MAX31785_SetRPMSpeed( i, fRPM );
				MAX31785_EnableFan( i );
			}
		} else if( __fancal_data.fans_num == 4 ){
			for( i = 1; i <= 4; i++ ){
				MAX31785_SetSpeedFaultLimitRPM( i, __fancal_data.min_rpm );
				MAX31785_SetManualRPM( i ); 
				MAX31785_SetRPMSpeed( i, fRPM );
				MAX31785_EnableFan( i );
			}
		} else if( __fancal_data.fans_num == 6 ){
			for( i = 0; i < 6; i++ ){
				MAX31785_SetSpeedFaultLimitRPM( i, __fancal_data.min_rpm );
				MAX31785_SetManualRPM( i ); 
				MAX31785_SetRPMSpeed( i, fRPM );
				MAX31785_EnableFan( i );
			}
		}
	// иначе считаем, что у нас 6 вентиляторов и не делаем проверку
	} else {
		fRPM = (f32)rpm ;
		for( i = 0; i < 6; i++ ){
				MAX31785_SetSpeedFaultLimitRPM( i, __fancal_data.min_rpm );
				MAX31785_SetManualRPM( i ); 
				MAX31785_SetRPMSpeed( i, fRPM );
				MAX31785_EnableFan( i );
			}
	}
	__MAX31785_StoreDefaultAll() ;
	return TRUE ;
}

u16 Dnepr_Fans_GetRPM( const u32 fan_num )
{
	_BOOL success ;
	u16 ret ;
	if( __fancal_read ){
		if(__fancal_data.fans_num == 2){
			MAX31785_SetPage( fan_num+1 );
			OSTimeDly( 5 );
			success = MAX31785_ReadFanRPM( &ret );
			return ret ; 
		} else if(__fancal_data.fans_num == 4){
			if( fan_num > 3 ){
				return 0xFFFF ;
			}
			MAX31785_SetPage( fan_num+1 );
			OSTimeDly( 5 );
			success = MAX31785_ReadFanRPM( &ret );
			return ret ; 
		} else if(__fancal_data.fans_num == 6){
			switch(fan_num){
				case 0:
					MAX31785_SetPage( 3 );
					break ;
				case 1:
					MAX31785_SetPage( 5 );
					break ;
				case 2:
					MAX31785_SetPage( 4 );
					break ;
				case 3:
					MAX31785_SetPage( 1 );
					break ;
				case 4:
					MAX31785_SetPage( 2 );
					break ;
				case 5:
					MAX31785_SetPage( 0 );
					break ;
			}
			OSTimeDly( 5 );
			success = MAX31785_ReadFanRPM( &ret );
			OSTimeDly( 5 );
			if( !success ){
				Dnepr_GPIO_timepin1_on() ;
				Dnepr_GPIO_timepin1_off() ;
			}
			return ret ; 
		}
	}
	return 0xFFFF;
}

_BOOL Dnepr_Fans_Calibrated()
{
	return __fancal_read ;
}

Dnepr_Fans_CalData_t* Dnepr_Fans_CalData()
{
	if( !__fancal_read ){
		return NULL ;
	} else {
		return &__fancal_data ;
	}
}

void Dnepr_Fans_Init( )
{
	size_t i ;
	_BOOL retry = TRUE ;
	u32 retry_times = 10 ;
	
	MAX31785_SetMfrMode( MAX31785_MFR_ALERT_ENABLED );
	do {
		for( i = 0; i < 6; i++ ){
			retry &= MAX31785_FanRampOnFault( i, 0 );
			retry &= MAX31785_SetFanPWMFrequency( i, MAX31785_FREQ_25kHz );
			retry &= MAX31785_SetFanSpinUp( i, MAX31785_FAN_SPINUP_8_REV );	
			retry &= MAX31785_SetTachPulses( i, MAX31785_TACH2 );
		}
	} while( (!retry) && (--retry_times > 0) );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// вычисляет CRC и пишет в EEPROM устройства в заданном слоте
static _BOOL __write_fan_eeprom( Dnepr_Fans_CalData_t *pFancal )
{
	_BOOL result ;
	assert( pFancal );

	pFancal->crc = Crc32_mem( (u8*)pFancal, sizeof(Dnepr_Fans_CalData_t) - sizeof(u32), 0 );

	result = AT_AT24C512_WriteArray( Dnepr_I2C_Get_I2C_BP_Driver( I2C_DNEPR_FAN_SLOT_NUM ),
						DNEPR_BACKPLANE_EEPROM_ADDR, Dnepr_Fans_CalData_t_STARTADDRESS,
											(u8*)pFancal, sizeof(Dnepr_Fans_CalData_t) );
	return result ;
}
