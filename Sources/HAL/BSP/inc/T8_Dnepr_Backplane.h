/*!
\file T8_Dnepr_Backplane.h
\brief Driver for crate PMBus nodes (2 PSU, slot devices hotswap controllers) and SPI EEPROM
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date sep 2012
*/

#ifndef __PMBUS_CRATE_DRIVER_
#define __PMBUS_CRATE_DRIVER_ 

#include "HAL/BSP/inc/T8_Dnepr_FPGA.h"
#include "HAL/IC/inc/PSU_Driver.h"
#include "HAL/IC/inc/MAX_MAX31785.h"
#include "HAL/IC/inc/MAX_DS28CM00.h"

#define DNEPR_FAN_SELECT			13
#define DNEPR_BACKPLANE_EEPROM_ADDR	0xA6
#define DNEPR_BACKPLANE_FAN_HS		0x8C

/*! Single PSU Status (see Emerson PSU datasheets). */
typedef struct __PsTypedef{
	u8 bSeated    :1;
	u8 bInOk      :1;
	u8 bPowerGood :1;
	u8 bFanFail   :1;
	u8 bTempOk    :1;
} T8_Dnepr_PsTypedef;

/*! States of PSU 1 and 2. */
typedef struct __PsStatusTypedef{
	T8_Dnepr_PsTypedef tPs1; /*!< PSU 1 status */
	T8_Dnepr_PsTypedef tPs2; /*!< PSU 2 status */
} T8_Dnepr_PsStatusTypedef;

/*! States of PSU */
typedef struct __PsStateTypedef{
	u8 bPs1On :1; /*!< PSU 1 state */
	u8 bPs2On :1; /*!< PSU 2 state */
} T8_Dnepr_PsStateTypedef;


//! \brief настраивает интерфейс шин PMBus у драйверов микросхем, блоков питания и пр.
void Dnepr_Backplane_Init();

//! \brief перечитывает из ПЛИС состояния выводов блоков питания
_BOOL Dnepr_Reload_PSU_Status_Pins();

//! \brief перечитывает состояние выводов БП и параметры по PMBus \ SMI
//! \note должно происходить из потока threadMeasure, 
_BOOL Dnepr_Backplane_Reload_PSU_Info();

//! \brief getter для __ps_status
const T8_Dnepr_PsStatusTypedef* Dnepr_Backplane_GetPSU_Status();
const PSU_UnitMeasurements* Dnepr_Backplane_GetPSU_Measures(const u8 num);
const PSU_UnitInfoTypedef* Dnepr_Backplane_GetPSU_Info( const u32 num );
void Dnepr_Backplane_Reload_PSU_DynInfo();

_BOOL Dnepr_Backplane_PS1_Present() ;
_BOOL Dnepr_Backplane_PS2_Present() ;

//! переключается на HotSwap PMB_Ext, т.ч. следующее обращение к LTC4222 будет к HotSwap'ам на стороне Backplane'а
void Dnepr_Backplane_SelectHotSwap() ;

//! читает массив байт из eeprom из указанного слота
_BOOL Dnepr_Backplane_EEPROM_Read( I2C_PeriphInterfaceTypedef *tI2CPeriphInterface, const u8 devAddr, const u16 data_addr, u8* pbData, const size_t len );
//! пишет массив байт в eeprom из указанного слота
_BOOL Dnepr_Backplane_EEPROM_Write( I2C_PeriphInterfaceTypedef *tI2CPeriphInterface,  const u8 devAddr, const u16 data_addr, u8* pbData, const size_t len );

//! производит чтение серийного номера на бекплейне
_BOOL Dnepr_ReadDS28CM00_Backplane(MAX_DS28CM00_ContentsTypedef* tContents);

I2C_PeriphInterfaceTypedef *Dnepr_BP_DS28CM00_I2C_handle();

#endif
