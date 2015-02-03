/*!
\file TI_UCD9080.h
\brief Driver for TI UCD9080 Power-supply sequencer and monitor.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 13.08.2012
*/

#ifndef __TI_UCD9080_DRIVER_H
#define __TI_UCD9080_DRIVER_H

#include "support_common.h"
#include "HAL/IC/inc/I2C_interface.h"

// * General macros
#define TI_UCD9080_RAILS_NUMBER 8
#define TI_UCD9080_GPO_NUMBER 4
#define TI_UCD9080_USER_DATA_SIZE 128
#define TI_UCD9080_CONFIG_SIZE 512
#define TI_UCD9080_ERROR_FIFO_DEPTH 8
#define TI_UCD9080_ERROR_REGISTER_LENGTH 6

#define TI_UCD9080_INTERNAL_VREF 2.5 /*!< Internal reference voltage (V) */
#define TI_UCD9080_EXTERNAL_VREF 3.3 /*!< External (Vcc) reference voltage (V) */
#define TI_UCD9080_GET_VREF(vreftype) ((vreftype == TI_UCD9080_InternalVref) ? TI_UCD9080_INTERNAL_VREF : TI_UCD9080_EXTERNAL_VREF)

/*!
\brief I2C I/O Expander address (left-fistified).
\param hw Hardware configurable pins state (e.g. \c 02h for A3=0, A2=0, A1=1, A0=0).
*/
#define TI_UCD9080_ADDRESS(hw) ( 0xC0 & ( hw<<1 ) )

/*!
\brief Converts ADC code to real-world voltage value (in V).
\param code ADC code value.
\param vref Reference voltage value (in V).
\retval Voltage value in V.
*/
#define TI_UCD9080_CONVERT_VOLTAGE(code, vref) (((f32)code / 1024) * vref)
/*!
\brief Converts ADC code to real-world voltage value (in V) with respect to the external voltage divider.
\param code ADC code value.
\param vref Reference voltage value (in V).
\param rpullup Pull-up resistor value in Ohms.
\param rpullup Pull-down resistor value in Ohms.
\retval Voltage value in V.
*/
#define TI_UCD9080_CONVERT_VOLTAGE_WITH_DIVIDER(code, vref, rpulldown, rpullup) \
	(TI_UCD9080_CONVERT_VOLTAGE(code, vref) * ((rpulldown + rpullup) / rpulldown))
	
#define TI_UCD9080_DIVISION_COEFFICIENT(rpulldown, rpullup) ((rpulldown + rpullup) / rpulldown)

// * Register map
// ** Rail registers, data bytes high and low base. ch - number of rail, 1 through 8.
#define TI_UCD9080_RAIL_HIGH_BASE(ch) (0x02 * (ch-1))
#define TI_UCD9080_RAIL_LOW_BASE(ch) (TI_UCD9080_RAIL_HIGH_BASE(ch) + 0x01)
// ** Error registers and its fields. n - number of byte, 1 through 6.
// ** bytes - six-byte error register array. bytes[0] - register 0x20, bytes [5] - register 0x25.
#define TI_UCD9080_ERROR_BASE(n) (0x20 + (n-1))
#define TI_UCD9080_ERROR_GET_ERROR_CODE(bytes) (bytes[0] >> 5)
#define TI_UCD9080_ERROR_GET_RAIL(bytes) ((bytes[0]&0x1C) >> 2)
#define TI_UCD9080_ERROR_GET_DATA(bytes) (((u16)(bytes[0] & 0x03) << 8) & (bytes[1]))
#define TI_UCD9080_ERROR_GET_HOUR(bytes) (bytes[3])
#define TI_UCD9080_ERROR_GET_MINUTES(bytes) (bytes[2])
#define TI_UCD9080_ERROR_GET_SECONDS(bytes) (bytes[5] >> 2)
#define TI_UCD9080_ERROR_GET_MILLISECONDS(bytes) (((u16)(bytes[5] & 0x03) << 8) & (bytes[4]))
// ** Status register
#define TI_UCD9080_STATUS 0x26
#define TI_UCD9080_STATUS_GET_IICERROR(b) ((b>>7) & 0x01)
#define TI_UCD9080_STATUS_GET_RAIL(b) ((b>>6) & 0x01)
#define TI_UCD9080_STATUS_GET_NVERRLOG(b) ((b>>5) & 0x01)
#define TI_UCD9080_STATUS_GET_REGISTER_STATUS(b) (b & 0x03)	
// ** Version register
#define TI_UCD9080_VERSION 0x27
// ** Rail error status register
#define TI_UCD9080_RAILSTATUS_1 0x28
#define TI_UCD9080_RAILSTATUS_2 0x29
// ** Configuration memory lock/unlock register and its value options.
#define TI_UCD9080_FLASHLOCK          0x2E
//#define TI_UCD9080_FLASHLOCK_LOCK     0x00
//#define TI_UCD9080_FLASHLOCK_UPDATING 0x01
//#define TI_UCD9080_FLASHLOCK_UNLOCK   0x02
// ** Restart register and byte to write it to reset the system.
#define TI_UCD9080_RESTART 0x2F
#define TI_UCD9080_RESTART_COMMAND 0x00
// ** Address registers
#define TI_UCD9080_WADDR1 0x30
#define TI_UCD9080_WADDR2 0x31
// ** Data registers
#define TI_UCD9080_WDATA1 0x32
#define TI_UCD9080_WDATA2 0x33

// * ROM map
/*   Due to reverse byte order in I2C communication with IC,
     following values may differ from ones given in the datasheet. */
// ** User data
#define TI_UCD9080_USER_DATA_ADDR_BASE 0x8010
#define TI_UCD9080_USER_DATA_ADDR(n) (TI_UCD9080_USER_DATA_ADDR_BASE + (n<<8))
// ** Configuration space
#define TI_UCD9080_CONFIG_ADDR_BASE 0x00E0
#define TI_UCD9080_CONFIG_ADDR(n) (TI_UCD9080_CONFIG_ADDR_BASE + (n<<8))
// ** General
#define TI_UCD9080_MEMORY_UPDATE_CMD 0xDCBA

#define TI_UCD9080_USER_DATA_ADDR_START 0x1080
#define TI_UCD9080_CONFIG_ADDR_START    0xE000
/*!
\defgroup TI_UCD9080_Driver
\{
*/

/*!
\defgroup TI_UCD9080_Driver_Exported_Types
\{
*/
/*! Rails */
typedef enum __TI_UCD9080_Rail{
	TI_UCD9080_Rail1 = 1, /*!< Rail 1 */
	TI_UCD9080_Rail2 = 2, /*!< Rail 2 */
	TI_UCD9080_Rail3 = 3, /*!< Rail 3 */
	TI_UCD9080_Rail4 = 4, /*!< Rail 4 */
	TI_UCD9080_Rail5 = 5, /*!< Rail 5 */
	TI_UCD9080_Rail6 = 6, /*!< Rail 6 */
	TI_UCD9080_Rail7 = 7, /*!< Rail 7 */
	TI_UCD9080_Rail8 = 8  /*!< Rail 8 */
}TI_UCD9080_Rail;

/*! Reference voltage typedef. */
typedef enum __TI_UCD9080_ReferenceVoltageType{
	TI_UCD9080_InternalVref = 0, /*!< Internal reference voltage (2.5 V) */
	TI_UCD9080_ExternalVref = 1  /*!< External reference voltage (3.3 V) */
}TI_UCD9080_ReferenceVoltageType;

/*! Resistor divider values */
typedef struct __TI_UCD9080_RailVoltageDividers{
	f32 afRPullup[TI_UCD9080_RAILS_NUMBER];   /*!< Pull-up resistor values for each rail */
	f32 afRPulldown[TI_UCD9080_RAILS_NUMBER]; /*!< Pull-down resistor values for each rail */
}TI_UCD9080_RailVoltageDividers;

/*! Error codes */
typedef enum __TI_UCD9080_ErrorCode{
	TI_UCD9080_NullAlarm = 0x00,             /*!< Null alarm */
	TI_UCD9080_SupplyDidNotStart = 0x01,     /*!< Supply did not start */
	TI_UCD9080_SustainedOvervoltage = 0x02,  /*!< Sustained overvoltage detected */
	TI_UCD9080_SustainedUndervoltage = 0x03, /*!< Sustained undervoltage detected */
	TI_UCD9080_OvervoltageGlitch = 0x04,     /*!< Sustained overvoltage detected */
	TI_UCD9080_UndervoltageGlitch = 0x05,    /*!< Sustained undervoltage detected */
	TI_UCD9080_Reserved1 = 0x06,             /*!< Reserved */
	TI_UCD9080_Reserved2 = 0x07              /*!< Reserved */
}TI_UCD9080_ErrorCode;

/*! Error register contents */
/* It could be a good idea to use bit fields to save some memory
   (see UCD9080 datasheet), but also may be not. */
typedef struct __TI_UCD9080_ErrorData{
	TI_UCD9080_ErrorCode mErrorCode; /*!< Error code */
	TI_UCD9080_Rail mRail; /*!< Number of rail */
	u16 nData; /*!< Data (dependent on error code) */
	u8 nHour; /*!< Hours */
	u8 nMinutes; /*!< Minutes */
	u8 nSeconds; /*!< Seconds */
	u16 nMilliseconds; /*!< Milliseconds */
}TI_UCD9080_ErrorData;

/*! I2C error options */
typedef enum __TI_UCD9080_RegisterStatus{
	TI_UCD9080_NoError = 0, /*!< No error */
	TI_UCD9080_InvalidAddress = 1, /*!< Invalid address */
	TI_UCD9080_ReadAccessError = 2, /*!< Read access error */
	TI_UCD9080_WriteAccessError = 3, /*!< Write access error */
}TI_UCD9080_RegisterStatus;

/*! Status register structure */
typedef struct __TI_UCD9080_Status{
	u8 bIicError: 1; /*! I2C PHY layer error bit */
	u8 bRail: 1;     /*! Rail error */
	u8 bNverrlog: 1; /*! Reserved */
	TI_UCD9080_RegisterStatus mRegisterStatus; /*! I2C error code */
}TI_UCD9080_Status;

/*! Flashlock register values */
typedef enum __TI_UCD9080_FlashlockValue{
	TI_UCD9080_FlashlockLock = 0x00,  /*!< Lock flash (default) */
	TI_UCD9080_FlashlockWait = 0x01,  /*!< Flash is being updated */
	TI_UCD9080_FlashlockUnlock = 0x02 /*!< Unlock flash (before configuration) */
}TI_UCD9080_FlashlockValue;

/*!
\brief Structure that accumulates all significant static and dynamic fields.
\details It's up to user how to use it and which fields to read.
*/
typedef struct __TI_UCD9080_Contents{
	f32 afRailVoltages[TI_UCD9080_RAILS_NUMBER]; /*!< Rail voltages in V */
	TI_UCD9080_ErrorData tErrorLog[TI_UCD9080_ERROR_FIFO_DEPTH]; /*! Error messages log */
	TI_UCD9080_Status tStatus; /*!< Status register */
	u8 nVersion; /*!< Version register */
	u16 nRailStatus; /*!< Rail error status */
	u8* anConfig; /*!< Config bytes pointer */
	u8* anUserData; /*!< User data pointer */
}TI_UCD9080_Contents;

/*!
\}
*/ //TI_UCD9080_Driver_Exported_Types

void TI_UCD9080_InitPeripherialInterface(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, const u8 mAddress );

f32 TI_UCD9080_ReadRailVoltage( TI_UCD9080_Rail mRail, TI_UCD9080_RailVoltageDividers* tDividers, TI_UCD9080_ReferenceVoltageType mVrefType);
_BOOL TI_UCD9080_ReadRailVoltages( f32* afRailVoltages, TI_UCD9080_RailVoltageDividers* tDividers, TI_UCD9080_ReferenceVoltageType mVrefType);
_BOOL TI_UCD9080_FetchError( TI_UCD9080_ErrorData* tError);
_BOOL TI_UCD9080_GetStatus( TI_UCD9080_Status* tStatus);
u8 TI_UCD9080_GetVersion();
u16 TI_UCD9080_GetRailStatus();
_BOOL TI_UCD9080_Restart();
_BOOL TI_UCD9080_ClearErrorLog();

_BOOL TI_UCD9080_WriteConfig( u8 const* anData, u32 nSize);
_BOOL TI_UCD9080_ReadConfig( u8* anData, u32 nSize);
_BOOL TI_UCD9080_WriteUserData( u8 const* anData, u32 nSize);
_BOOL TI_UCD9080_ReadUserData( u8* anData, u32 nSize);

/*!
\}
*/ //TI_UCD9080_Driver

#endif
