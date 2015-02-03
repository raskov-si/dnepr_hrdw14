/*!
\file TI_TCA9555.h
\brief Driver for TI TCA9555 I/O Expander.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 02.05.2012
*/

#ifndef __TI_TCA9555_DRIVER_H
#define __TI_TCA9555_DRIVER_H

#include "support_common.h"
#include "HAL/IC/inc/I2C_interface.h"

/*!
\brief I2C I/O Expander address.
\param hw Hardware configurable pins state (e.g. \c 02h for A2=0, A1=1, A0=0).
*/
#define TI_TCA9555_ADDRESS(hw) ( 0x40 & ( hw<<1 ) )

//Control Register
#define TI_TCA9555_INPUT_P0  0x00
#define TI_TCA9555_INPUT_P1  0x01
#define TI_TCA9555_OUTPUT_P0 0x02
#define TI_TCA9555_OUTPUT_P1 0x03
#define TI_TCA9555_INVERT_P0 0x04
#define TI_TCA9555_INVERT_P1 0x05
#define TI_TCA9555_CONFIG_P0 0x06
#define TI_TCA9555_CONFIG_P1 0x07

//I/O direction config
#define TI_TCA9555_ALL_OUTPUTS 0x00
#define TI_TCA9555_ALL_INPUTS  0xFF

/*!
\defgroup TI_TCA9555_Driver
\{
*/

/*!
\defgroup TI_TCA9555_Driver_Exported_Types
\{
*/
typedef struct __TI_TCA9555_PortsTypedef{
	u8 nInputP0;  /*!< Port P0x input states */
	u8 nInputP1;  /*!< Port P1x input states */
	u8 nOutputP0; /*!< Port P0x output states */
	u8 nOutputP1; /*!< Port P1x output states */
	u8 nInvertP0; /*!< Port P0x polarity inversion states */
	u8 nInvertP1; /*!< Port P1x polarity inversion states */
	u8 nConfigP0; /*!< Port P0x I/O configuration (0=output, 1=input)*/
	u8 nConfigP1; /*!< Port P1x I/O configuration (0=output, 1=input)*/
}
TI_TCA9555_PortsTypedef;

/*!
\}
*/ //TI_TCA9555_Driver_Exported_Types

/*!
\}
*/ //TI_TCA9555_Driver

void  TI_TCA9555_InitPeripherialInterface(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface);
void  TI_TCA9555_InitPortsStructure(TI_TCA9555_PortsTypedef* tPorts);
_BOOL TI_TCA9555_WriteConfig(u8 nAddress, TI_TCA9555_PortsTypedef* tPorts);
_BOOL TI_TCA9555_ReadInputs(u8 nAddress, TI_TCA9555_PortsTypedef* tPorts);

#endif
