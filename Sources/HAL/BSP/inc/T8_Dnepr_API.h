/*!
\file T8_Dnepr_API.h
\brief Driver for Dnepr internal peripherials (i2c switches, SFP, FPGA etc) .
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 05.05.2012
*/


#ifndef __I2C_BOARD_DRIVER_H
#define __I2C_BOARD_DRIVER_H

#include "HAL\IC\inc\TI_TCA9555.h"
#include "HAL\IC\inc\MAX_DS28CM00.h"
#include "HAL\IC\inc\TI_PCA9544A.h"
#include "HAL\IC\inc\LT_LTC4222.h"
#include "HAL\IC\inc\PSU_Driver.h"
#include "HAL\IC\inc\NMNX_M25P128.h"
#include "HAL\IC\inc\T8_SFP.h"
#include "HAL\BSP\inc\T8_Dnepr_FPGA.h"

#include "HAL\MCU\inc\I2C_GenericDriver.h"
#include "HAL/MCU/inc/T8_5282_FEC.h"
#include "HAL\MCU\inc\SPI_GenericDriver.h"

//! јдрес датчика температуры
#define I2C_DNEPR_TMP112_ADDR	0x90

//! јдрес PowerSequencer
#define I2C_DNEPR_UCD9080_ADDR	0xC0

//Switch 1 (DD30)
#define I2C_DNEPR_I2C_CH       TI_PCA9544A_Channel0
#define I2C_DNEPR_PMBUS_EXT_CH TI_PCA9544A_Channel1
#define I2C_DNEPR_PMBUS_INT_CH TI_PCA9544A_Channel2


//!Numbers of devices
#define I2C_DNEPR_NUMBER_OF_SLOTS 	14
#define I2C_DNEPR_FAN_SLOT_NUM 		13
#define I2C_DNEPR_NUMBER_OF_PSU   	2
#define I2C_DNEPR_NUMBER_OF_SFP   	2

/*! Outputs state config */
#define TI_TCA9555_SFP_DEFAULT_STATE_CONFIG_0	(0x38)	// L/U Disable и L RATE RATE SEL L
#define TI_TCA9555_SFP_DEFAULT_STATE_CONFIG_1	(0x01)	// RATE SEL U
/*! I/O direction config. 1=input, 0=output */
#define TI_TCA9555_SFP_DEFAULT_IO_CONFIG_0		(TI_TCA9555_ALL_INPUTS ^ 0x38) // L/U Disable и L RATE
#define TI_TCA9555_SFP_DEFAULT_IO_CONFIG_1		(TI_TCA9555_ALL_INPUTS ^ 0x01) // U RATE

/* We can use invert in SFP config */
#define TI_TCA9555_IS_SFP_U_PRESENT(b) ( ( (b.nInputP0)&0x40) == 0 )
#define TI_TCA9555_IS_SFP_L_PRESENT(b) ( ( (b.nInputP0)&0x02) == 0 )
#define I2C_DNEPR_SFP_L_INDEX 0
#define I2C_DNEPR_SFP_U_INDEX 1


/*!
\defgroup I2C_Board_Driver
\{
*/

/*!
\defgroup I2C_Board_Driver_Exported_Types
\{
*/

/*! Present devices */
typedef struct __I2C_DNEPR_PresentDevicesTypedef{
	_BOOL bSlotPresent[I2C_DNEPR_NUMBER_OF_SLOTS]; /*!< Whether device in slot [x] present or not. */
	_BOOL bSfpPresent[I2C_DNEPR_NUMBER_OF_SFP];
}I2C_DNEPR_PresentDevicesTypedef;

/*!
\}
*/ //I2C_Board_Driver_Exported_Types

/*!
\}
*/ //I2C_Board_Driver

/*!
 *\brief »нициализирует всЄ железо
 *\details »нициализаци€ интерфейсов переферии: spi, i2c mdio etc, и микросхем на этих интерфейсах
 */
void DNEPR_InitPeripherials(void);

//! »дентификатор SFP дл€ внутренних нужд
typedef enum __I2C_DNEPR_SFP_NUMBER
{
	SFP_1,		//!< нижн€€ SFP
	SFP_2 		//!< верхн€€ SFP
} DNEPR_SFP_NUMBER ;

_BOOL I2C_DNEPR_ClearPresentDevices(I2C_DNEPR_PresentDevicesTypedef* tPresentDevices);
_BOOL Dnepr_Refresh_SFP_Presents();
_BOOL Dnepr_Refresh_Presents();
I2C_DNEPR_PresentDevicesTypedef* I2C_DNEPR_GetPresentDevices(void);
_BOOL I2C_Dnepr_SFP_OnOff( const _BOOL sfp_1_on_, const _BOOL sfp_2_on_ );
//! перечитывает статические и динамические параметры SFP с номером sfp_num
//! \param sfp_params структура с данными результата
//! \param renew_static_info перечитывать неизмен€емые параметры или нет
//! \param sfp_num номер sfp модул€ (0 нижний, 1 верхний)
_BOOL I2C_Dnepr_SFP_Renew( T8_SFP_OPTICAL_CHANNEL* sfp_params,
					const _BOOL renew_static_info, const u32 sfp_num );

void isr_Watchdog();

//! переключаетс€ на внутренний HotSwap, т.ч. следующее обращение к LTC4222 будет к HotSwap'у на плате ƒнепра
void I2C_Dnepr_SelectDneprHotSwap() ;
// возвращает FALSE, если на текущий момент активен драйвер HotSwap'а дл€ слотовых
// устройств
_BOOL I2C_Dnepr_CheckDneprHotSwapSelection();

//! производит чтение серийного номера на плате днепра
_BOOL Dnepr_ReadDS28CM00_Internal(MAX_DS28CM00_ContentsTypedef* tContents);


#endif //__I2C_BOARD_DRIVER_H
