/*!
\file MAX_DS28CM00.h
\brief Driver for Maxim DS28CM00 I2C silicon serial number.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 30.04.2012
*/

#ifndef __MAX_DS28CM00_DRIVER_H
#define __MAX_DS28CM00_DRIVER_H

#include "support_common.h"
#include "HAL/IC/inc/I2C_interface.h"

/* Address (non-programmable) */
#define MAX_DS28CM00_ADDRESS 0xA0

/* Memory map */
#define MAX_DS28CM00_DEVICE_FAMILY_BASE    0x00
#define MAX_DS28CM00_SERIAL_NUMBER_BASE    0x01
#define MAX_DS28CM00_CRC_BASE              0x07
#define MAX_DS28CM00_CONTROL_REGISTER_BASE 0x08

/* Control register */
#define MAX_DS28CM00_CM 0x01
#define MAX_DS28CM00_CM_SMB 0x01 /*!< SMBus mode */
#define MAX_DS28CM00_CM_I2C 0x00 /*!< I2C mode   */

/* Serial Number length in bytes. */
#define MAX_DS28CM00_LENGTH 6

/*!
\defgroup MAX_DS28CM00_Driver
\{
*/

/*!
\defgroup MAX_DS28CM00_Driver_Exported_Types
\{
*/

/*!
\brief Structure for DS28CM00 contents.
\todo Find a way to store 48-bit S/N in a single variable.
 */
typedef struct __MAX_DS28CM00_ContentsTypedef{
	u8 nFamilyCode;                        /*!< Device Family Code (70h) */
	u8 nSerialNumber[MAX_DS28CM00_LENGTH]; /*!< 48-bit serial number */
	u8 nCRC;                               /*!< CRC of Family Code and 48-bit Serial Number */
}
MAX_DS28CM00_ContentsTypedef;

/*! Mode of device operation (with or without bus timeout function). */
typedef enum __MAX_DS28CM00_ModeTypedef{
	MAX_DS28CM00_I2C = 0, /*!< I2C mode (no bus timeout) */
	MAX_DS28CM00_SMB = 1  /*!< SMBus mode (bus timeout function enabled)  */
}
MAX_DS28CM00_ModeTypedef;

/*!
\}
*/ //MAX_DS28CM00_Driver_Exported_Types

/*!
\}
*/ //MAX_DS28CM00_Driver

void MAX_DS28CM00_InitContentsStructure( MAX_DS28CM00_ContentsTypedef* tContents );
_BOOL MAX_DS28CM00_ReadContents(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface,
										MAX_DS28CM00_ContentsTypedef* tContents);
_BOOL MAX_DS28CM00_CheckCRC( MAX_DS28CM00_ContentsTypedef* tContents );
_BOOL MAX_DS28CM00_SetMode(	I2C_PeriphInterfaceTypedef* tI2cPeriphInterface,
												MAX_DS28CM00_ModeTypedef mMode);
MAX_DS28CM00_ModeTypedef MAX_DS28CM00_GetMode( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface );



#endif //__MAX_DS28CM00_DRIVER_H
