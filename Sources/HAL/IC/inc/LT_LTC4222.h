/*!
\file LT_LTC4222.h
\brief Driver for Linear Technology LTC4222 Hot-Swap Controller.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\note This device is PMBus-compatible. All registers are in MFR_SPECIFIC area.
\todo Check actual register field meanings. Datasheet descriptions are quite confusing (regarding "0" and "1" states).
\date 10.05.2012
*/

#ifndef __LT_LTC4222_DRIVER_H
#define __LT_LTC4222_DRIVER_H

#include "support_common.h"
#include "HAL/IC/inc/PMB_interface.h"

//Hot Swap controller register addresses
#define LT_LTC4222_CONTROL1    0xD0
#define LT_LTC4222_ALERT1      0xD1
#define LT_LTC4222_STATUS1     0xD2
#define LT_LTC4222_FAULT1      0xD3
#define LT_LTC4222_CONTROL2    0xD4
#define LT_LTC4222_ALERT2      0xD5
#define LT_LTC4222_STATUS2     0xD6
#define LT_LTC4222_FAULT2      0xD7
#define LT_LTC4222_SOURCE1_MSB 0xD8
#define LT_LTC4222_SOURCE1_LSB 0xD9
#define LT_LTC4222_SOURCE2_MSB 0xDA
#define LT_LTC4222_SOURCE2_LSB 0xDB
#define LT_LTC4222_ADIN1_MSB   0xDC
#define LT_LTC4222_ADIN1_LSB   0xDD
#define LT_LTC4222_ADIN2_MSB   0xDE
#define LT_LTC4222_ADIN2_LSB   0xDF
#define LT_LTC4222_SENSE1_MSB  0xE0
#define LT_LTC4222_SENSE1_LSB  0xE1
#define LT_LTC4222_SENSE2_MSB  0xE2
#define LT_LTC4222_SENSE2_LSB  0xE3
#define LT_LTC4222_ADC_CONTROL 0xE4

//Control registers (0xD0 and 0xD4)
#define LT_LTC4222_GPIO_PGOOD         B8(00000000)
#define LT_LTC4222_GPIO_nPGOOD        B8(01000000)
#define LT_LTC4222_GPIO_OUTPUT        B8(10000000)
#define LT_LTC4222_GPIO_INPUT         B8(11000000)
#define LT_LTC4222_GPO_HIGH           B8(00100000)
#define LT_LTC4222_GPO_LOW            B8(00000000)
#define LT_LTC4222_MASS_WRITE_ENABLE  B8(00010000)
#define LT_LTC4222_MASS_WRITE_DISABLE B8(00000000)
#define LT_LTC4222_FET_ON_CONTROL     B8(00001000)
#define LT_LTC4222_OC_AUTO_RETRY      B8(00000100)
#define LT_LTC4222_UV_AUTO_RETRY      B8(00000010)
#define LT_LTC4222_OV_AUTO_RETRY      B8(00000001)

#define LT_LTC4222_SET_GPIO_CONFIG(b, cfg)  ( b = b | cfg ) 
#define LT_LTC4222_SET_GPO_HIGH(b)          ( b = b |LT_LTC4222_GPO_HIGH )
#define LT_LTC4222_SET_MASS_WRITE_ENABLE(b) ( b = b |LT_LTC4222_MASS_WRITE_ENABLE )
#define LT_LTC4222_SET_FET_ON_CONTROL(b)    ( b = b |LT_LTC4222_FET_ON_CONTROL )
#define LT_LTC4222_SET_OC_AUTO_RETRY(b)     ( b = b |LT_LTC4222_OC_AUTO_RETRY )
#define LT_LTC4222_SET_UV_AUTO_RETRY(b)     ( b = b |LT_LTC4222_UV_AUTO_RETRY )
#define LT_LTC4222_SET_OV_AUTO_RETRY(b)     ( b = b |LT_LTC4222_OV_AUTO_RETRY )

#define LT_LTC4222_GET_GPIO_CONFIG(b)       ( ((b)&0xC0) )
#define LT_LTC4222_GET_GPO_STATE(b)         ( ((b)&LT_LTC4222_GPO_HIGH)>>5 )
#define LT_LTC4222_GET_MASS_WRITE_STATE(b)  ( ((b)&LT_LTC4222_MASS_WRITE_ENABLE)>>4 )
#define LT_LTC4222_GET_FET_ON_CONTROL(b)    ( ((b)&LT_LTC4222_FET_ON_CONTROL)>>3 )
#define LT_LTC4222_GET_OC_AUTO_RETRY(b)     ( ((b)&LT_LTC4222_OC_AUTO_RETRY)>>2 )
#define LT_LTC4222_GET_UV_AUTO_RETRY(b)     ( ((b)&LT_LTC4222_UV_AUTO_RETRY)>>1 )
#define LT_LTC4222_GET_OV_AUTO_RETRY(b)     ( ((b)&LT_LTC4222_OV_AUTO_RETRY) )

//Alert registers (0xD1 and 0xD5)
#define LT_LTC4222_FET_SHORT_ALERT        B8(00100000)
#define LT_LTC4222_nEN_STATE_CHANGE_ALERT B8(00010000)
#define LT_LTC4222_POWER_BAD_ALERT        B8(00001000)
#define LT_LTC4222_OC_ALERT               B8(00000100)
#define LT_LTC4222_UV_ALERT               B8(00000010)
#define LT_LTC4222_OV_ALERT               B8(00000001)

#define LT_LTC4222_SET_FET_SHORT_ALERT(b)        ( (b)|=LT_LTC4222_FET_SHORT_ALERT )
#define LT_LTC4222_SET_nEN_STATE_CHANGE_ALERT(b) ( (b)|=LT_LTC4222_nEN_STATE_CHANGE_ALERT )
#define LT_LTC4222_SET_POWER_BAD_ALERT(b)        ( (b)|=LT_LTC4222_POWER_BAD_ALERT )
#define LT_LTC4222_SET_OC_ALERT(b)               ( (b)|=LT_LTC4222_OC_ALERT )
#define LT_LTC4222_SET_UV_ALERT(b)               ( (b)|=LT_LTC4222_UV_ALERT )
#define LT_LTC4222_SET_OV_ALERT(b)               ( (b)|=LT_LTC4222_OV_ALERT )

#define LT_LTC4222_GET_FET_SHORT_ALERT(b)        ( ((b)&LT_LTC4222_FET_SHORT_ALERT)>>5 )
#define LT_LTC4222_GET_nEN_STATE_CHANGE_ALERT(b) ( ((b)&LT_LTC4222_nEN_STATE_CHANGE_ALERT)>>4 )
#define LT_LTC4222_GET_POWER_BAD_ALERT(b)        ( ((b)&LT_LTC4222_POWER_BAD_ALERT)>>3 )
#define LT_LTC4222_GET_OC_ALERT(b)               ( ((b)&LT_LTC4222_OC_ALERT)>>2 )
#define LT_LTC4222_GET_UV_ALERT(b)               ( ((b)&LT_LTC4222_UV_ALERT)>>1 )
#define LT_LTC4222_GET_OV_ALERT(b)               ( ((b)&LT_LTC4222_OV_ALERT) )


//Status registers (0xD2 and 0xD6)
#define LT_LTC4222_GET_FET_ON(b)    ( ((b)&0x80)>>7 )
#define LT_LTC4222_GET_GPI_STATE(b) ( ((b)&0x40)>>6 )
#define LT_LTC4222_GET_FET_SHORT(b) ( ((b)&0x20)>>5 )
#define LT_LTC4222_GET_nEN_STATE(b) ( ((b)&0x10)>>4 )
#define LT_LTC4222_GET_POWER_BAD(b) ( ((b)&0x08)>>3 )
#define LT_LTC4222_GET_OC(b)        ( ((b)&0x04)>>2 )
#define LT_LTC4222_GET_UV(b)        ( ((b)&0x02)>>1 )
#define LT_LTC4222_GET_OV(b)        ( ((b)&0x01) )

//Fault registers (0xD3 and 0xD7)
#define LT_LTC4222_FET_SHORT_FAULT        B8(00100000)
#define LT_LTC4222_nEN_STATE_CHANGE_FAULT B8(00010000)
#define LT_LTC4222_POWER_BAD_FAULT        B8(00001000)
#define LT_LTC4222_OC_FAULT               B8(00000100)
#define LT_LTC4222_UV_FAULT               B8(00000010)
#define LT_LTC4222_OV_FAULT               B8(00000001)

#define LT_LTC4222_GET_FET_SHORT_FAULT(b)        ( LT_LTC4222_GET_FET_SHORT_ALERT(b) )
#define LT_LTC4222_GET_nEN_STATE_CHANGE_FAULT(b) ( LT_LTC4222_GET_nEN_STATE_CHANGE_ALERT(b) )
#define LT_LTC4222_GET_POWER_BAD_FAULT(b)        ( LT_LTC4222_GET_POWER_BAD_ALERT(b) )
#define LT_LTC4222_GET_OC_FAULT(b)               ( LT_LTC4222_GET_OC_ALERT(b) )
#define LT_LTC4222_GET_UV_FAULT(b)               ( LT_LTC4222_GET_UV_ALERT(b) )
#define LT_LTC4222_GET_OV_FAULT(b)               ( LT_LTC4222_GET_OV_ALERT(b) )

/* ADC Control register */
#define LT_LTC4222_ADC_BUSY       B8(00100000)
#define LT_LTC4222_ADC_ALERT      B8(00010000)
/* ADC Channels are in respective typedef below */
#define LT_LTC4222_ADC_HALT       B8(00000001) 

#define LT_LTC4222_ADC_SET_ALERT(b)       ( (b) |= LT_LTC4222_ADC_ALERT )
#define LT_LTC4222_ADC_SET_CHANNEL(b, ch) ( (b) |= ((ch)<<1) )
#define LT_LTC4222_ADC_SET_HALT(b)        ( (b) |= LT_LTC4222_ADC_HALT )
#define LT_LTC4222_ADC_RESET_HALT(b)      ( (b) &= (~LT_LTC4222_ADC_HALT) )

#define LT_LTC4222_ADC_GET_BUSY(b)    ( ((b)&LT_LTC4222_ADC_BUSY) >> 5 )
#define LT_LTC4222_ADC_GET_ALERT(b)   ( ((b)&LT_LTC4222_ADC_ALERT) >> 4 )
#define LT_LTC4222_ADC_GET_CHANNEL(b) ( ((b)&0x0E)>>1 )
#define LT_LTC4222_ADC_GET_HALT(b)    ( ((b)&LT_LTC4222_ADC_HALT) )

//ADC LSB Steps (resolution)
#define PMB_ADC_SENSE_STEP  62.5  /*!< VDD Sense LSB resolution (uV)*/
#define PMB_ADC_SOURCE_STEP 31.25 /*!< Source LSB resolution (mV)*/
#define PMB_ADC_ADIN_STEP   1.25  /*!< ADIN LSB resolution (mV) */

//ADC resolution. Use these constants with RAW (left-justified) ADC data.
#define PMB_ADC_RAW_SENSE_STEP  (PMB_ADC_SENSE_STEP/64.0)
#define PMB_ADC_RAW_SOURCE_STEP (PMB_ADC_SOURCE_STEP/64.0)
#define PMB_ADC_RAW_ADIN_STEP   (PMB_ADC_ADIN_STEP/64.0)

//Macros for ADC code conversion to real-world values. Use with RAW data (bits 0 through 5 = 0).
#define PMB_ADC_RAW_SENSE_TO_MILLIVOLTS(v) ( (f32)(v)*PMB_ADC_RAW_SENSE_STEP/1000. )
#define PMB_ADC_RAW_SOURCE_TO_VOLTS(v)     ( (f32)(v)*PMB_ADC_RAW_SOURCE_STEP/1000. )
#define PMB_ADC_RAW_ADIN_TO_VOLTS(v)       ( (f32)(v)*PMB_ADC_RAW_ADIN_STEP/1000. )
#define PMB_ADC_RAW_SENSE_TO_VOLTS(v)      ( PMB_ADC_RAW_SENSE_TO_MILLIVOLTS(v) / 1000. )

/*! ADC Error value */
#define PMB_ADC_ERROR -1.0

/*!
\defgroup LT_LTC4222_Driver
\{
*/

/*!
\defgroup LT_LTC4222_Driver_Exported_Types
\{
*/

/*! GPIO pin configuration structure */
typedef enum __LT_LTC4222_GpioConfigTypedef{
	LT_LTC4222_PGOOD  = LT_LTC4222_GPIO_PGOOD,  /*! Power Good  */
	LT_LTC4222_nPGOOD = LT_LTC4222_GPIO_nPGOOD, /*! Inverted Power Good */
	LT_LTC4222_OUTPUT = LT_LTC4222_GPIO_OUTPUT, /*! General Purpose Output */
	LT_LTC4222_INPUT  = LT_LTC4222_GPIO_INPUT   /*! General Purpose Input */
}LT_LTC4222_GpioConfigTypedef;

/*! Control register settings structure */
typedef struct __LT_LTC4222_ControlRegisterTypedef{
	LT_LTC4222_GpioConfigTypedef tGpioConfig; /*!< GPIO pin configuration structure */
	u8 bGpioOutputState : 1; /*!< GPIO pin state when gonfigured as output */
	u8 bMassWriteEnable : 1; /*!< Allows Mass wrte Addressing (channel 2 only!) */
	u8 bFetOnControl    : 1; /*!< On Control Bit (Read only ?) */
	u8 bOcAutoRetry     : 1; /*!< Overcurrent Auto-Retry Bit */
	u8 bUvAutoRetry     : 1;  /*!< Undervoltage Auto-Retry Bit */
	u8 bOvAutoRetry     : 1; /*!< Overvoltage Auto-Retry Bit */
}LT_LTC4222_ControlRegisterTypedef;

/*! Fault/Alert register settings structure. Could be used both for Alert and Fault registers. */
typedef struct __LT_LTC4222_FaultAlertRegisterTypedef{
	u8 bFetShort      : 1; /*!< Fault/alert for FET short condition */
	u8 bEnStateChange : 1; /*!< Fault/alert when EN# changes state */ 
	u8 bPowerBad      : 1; /*!< Fault/alert when output power is bad */ 
	u8 bOverCurrent   : 1; /*!< Fault/alert for overcurrent condition */ 
	u8 bUnderVoltage  : 1; /*!< Fault/alert for undervoltage condition */ 
	u8 bOverVoltage   : 1; /*!< Fault/alert for overvoltage condition */ 
}LT_LTC4222_FaultAlertRegisterTypedef;

/*! Status register state structure */
typedef struct __LT_LTC4222_StatusRegisterTypedef{
	u8 bFetOn          : 1; /*!< FET on state */
	u8 bGpioInputState : 1; /*!< GPIO input state */
	u8 bFetShort       : 1; /*!< FET short Status */
	u8 bEnState        : 1; /*!< Indicates if channel is enabled when EN# is low (1=EN# pin low, 0=EN# pin high) */
	u8 bPowerBadState  : 1; /*!< Indicates Power is bad when FB is low (1=FB low, 0=FB high) */
	u8 bOverCurrent    : 1; /*!< Indicates overcurrent condition (1=overcurrent, 0=no overcurrent) */
	u8 bUnderVoltage   : 1; /*!< Indicates input undervoltage when UV is low (1=UV low, 0=UV high) */
	u8 bOverVoltage    : 1; /*!< Indicates input overvoltage when OV is high (1=OV high, 0=OV low) */
}LT_LTC4222_StatusRegisterTypedef;

/*! ADC channels (see LTC4222 datasheet for detals) */
typedef enum __LT_LTC4222_AdcChannelTypedef{
	LT_LTC4222_ADC_SOURCE1 = 0x00, /*!< SOURCE, channel 1 */
	LT_LTC4222_ADC_SOURCE2 = 0x01, /*!< SOURCE, channel 2 */
	LT_LTC4222_ADC_ADIN1   = 0x02, /*!< ADIN, channel 1 */
	LT_LTC4222_ADC_ADIN2   = 0x03, /*!< ADIN, channel 2 */
	LT_LTC4222_ADC_SENSE1  = 0x04, /*!< SENSE, channel 1 */
	LT_LTC4222_ADC_SENSE2  = 0x05  /*!< SENSE, channel 2 */
}LT_LTC4222_AdcChannelTypedef;

/*! ADC control register structure */
typedef struct __LT_LTC4222_AdcControlRegisterTypedef{
	u8 bAdcBusy  : 1; /*!< Status Bit That is High When the ADC is Converting. See datasheet for details. */
	u8 bAdcAlert : 1; /*!< Enables the ALERT# Pin to Pull Low When the ADC Finishes a Measurement. */
	LT_LTC4222_AdcChannelTypedef tAdcChannel; /*! ADC Channel Address. See datasheet for details. */
	u8 bHalt     : 1; /*! Stops the Data Converter and Enables Point and Shoot Mode */
}LT_LTC4222_AdcControlRegisterTypedef;

/*! Hot Swap controller power handling channels */
typedef enum __LT_LTC4222_HotswapChannelTypedef{
	LT_LTC4222_CHANNEL1 = 0x00, /*!< Channel (power path) 1 */
	LT_LTC4222_CHANNEL2 = 0x01  /*!< Channel (power path) 2 */
}LT_LTC4222_HotswapChannelTypedef;

/*!
\brief ADC Voltages structure.
\details Real-world voltage values on all ADC pins.
\note ADINx voltage represents input ADC voltage that should not exceed ADC reference voltage,
\note so take into consideration external prescaler (if one exists) to get actual measured value.  
*/
typedef struct __LT_LTC4222_AdcVoltagesStructure{
	f32 fSource1; /*!< SOURCE1 voltage in volts */
	f32 fSource2; /*!< SOURCE2 voltage in volts */
	f32 fAdin1;   /*!< ADIN1 voltage in volts */
	f32 fAdin2;   /*!< ADIN2 voltage in volts */
	f32 fSense1;  /*!< SENSE1 voltage in volts */
	f32 fSense2;  /*!< SENSE1 voltage in volts */
}LT_LTC4222_AdcVoltagesStructure;

/*!
\}
*/ //LT_LTC4222_Driver_Exported_Types

/*!
\}
*/ //LT_LTC4222_Driver

_BOOL LT_LTC4222_SetConfig( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel, LT_LTC4222_ControlRegisterTypedef* tControlRegisterStructure, LT_LTC4222_FaultAlertRegisterTypedef* tAlertRegisterStructure);
_BOOL LT_LTC4222_GetConfig( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel, LT_LTC4222_ControlRegisterTypedef* tControlRegisterStructure, LT_LTC4222_FaultAlertRegisterTypedef* tAlertRegisterStructure);
_BOOL LT_LTC4222_ResetFaults( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface,  u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel );
_BOOL LT_LTC4222_GetStates( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel, LT_LTC4222_StatusRegisterTypedef* tStateRegisterStructure, LT_LTC4222_FaultAlertRegisterTypedef* tFaultRegisterStructure);
_BOOL LT_LTC4222_SetAdcControl( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcControlRegisterTypedef* tAdcControlRegisterStructure);
_BOOL LT_LTC4222_GetAdcControl( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcControlRegisterTypedef* tAdcControlRegisterStructure);
u16  LT_LTC4222_GetRawAdcData( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel);
f32  LT_LTC4222_GetSourceVoltage( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel);
f32  LT_LTC4222_GetAdinVoltage( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel);
f32  LT_LTC4222_GetSenseVoltage( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel);
_BOOL LT_LTC4222_GetAdcVoltages( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcVoltagesStructure* tAdcVoltagesStructure);

#endif //__LT_LTC4222_DRIVER_H
