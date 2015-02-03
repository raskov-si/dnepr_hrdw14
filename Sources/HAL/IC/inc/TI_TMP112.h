/*!
\file TI_TMP112.h
\brief Driver for Texas Instruments TMP112 Temperature sensor.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 21.08.2012
*/

#ifndef __TI_TMP112_H
#define __TI_TMP112_H

#include "support_common.h"
#include "HAL/IC/inc/I2C_interface.h"

/* Pointer register */
#define TI_TMP112_TEMPERATURE_REGISTER 0x00
#define TI_TMP112_CONFIGURATION_REGISTER 0x01
#define TI_TMP112_TLOW_REGISTER 0x02
#define TI_TMP112_THIGH_REGISTER 0x03

/* Configuration register */
#define TI_TMP112_OS  0x8000
#define TI_TMP112_R1  0x4000
#define TI_TMP112_R0  0x2000
#define TI_TMP112_F1  0x1000
#define TI_TMP112_F0  0x0800
#define TI_TMP112_POL 0x0400
#define TI_TMP112_TM  0x0200
#define TI_TMP112_SD  0x0100
#define TI_TMP112_CR1 0x0080
#define TI_TMP112_CR0 0x0040
#define TI_TMP112_AL  0x0020
#define TI_TMP112_EM  0x0010

#define TI_TMP112_GET_OS(w) ((w & TI_TMP112_OS) >> 15)
#define TI_TMP112_GET_R1_R0(w) ((w & (TI_TMP112_R1 | TI_TMP112_R0)) >> 13)
#define TI_TMP112_GET_F1_F0(w) ((w & (TI_TMP112_F1 | TI_TMP112_F0)) >> 11)
#define TI_TMP112_GET_POL(w) ((w & TI_TMP112_POL) >> 10)
#define TI_TMP112_GET_TM(w) ((w & TI_TMP112_TM) >> 9)
#define TI_TMP112_GET_SD(w) ((w & TI_TMP112_SD) >> 8)
#define TI_TMP112_GET_CR1_CR0(w) ((w & (TI_TMP112_CR1 | TI_TMP112_CR0)) >> 6)
#define TI_TMP112_GET_AL(w) ((w & TI_TMP112_AL) >> 5)
#define TI_TMP112_GET_EM(w) ((w & TI_TMP112_EM) >> 4)

#define TI_TMP112_SET_OS(w) ((w & 0x01) << 15)
#define TI_TMP112_SET_R1_R0(w) ((w & 0x03) << 13)
#define TI_TMP112_SET_F1_F0(w) ((w & 0x03) << 11)
#define TI_TMP112_SET_POL(w) ((w & 0x01) << 10)
#define TI_TMP112_SET_TM(w) ((w & 0x01) << 9)
#define TI_TMP112_SET_SD(w) ((w & 0x01) << 8)
#define TI_TMP112_SET_CR1_CR0(w) ((w & 0x03) << 6)
#define TI_TMP112_SET_AL(w) ((w & 0x01) << 5)
#define TI_TMP112_SET_EM(w) ((w & 0x01) << 4)

#define TI_TMP112_12_BIT_MODE 0x0000
#define TI_TMP112_13_BIT_EXTENDED_MODE 0x0001
#define TI_TMP112_CHECK_EXTENDED_MODE(w) (w & 0x0001)

/* Resolution */
#define TI_TMP112_LSB_STEP 0.0625 // in C
/* For reading temperature */
#define TI_TMP112_CONVERT_12_BIT_CODE_TO_C(w) ((w >> 4) * TI_TMP112_LSB_STEP)
#define TI_TMP112_CONVERT_13_BIT_CODE_TO_C(w) ((w >> 3) * TI_TMP112_LSB_STEP)
/* For setting alarm thresholds. "t" should be a floating-point value!  */
#define TI_TMP112_CONVERT_C_TO_12_BIT_CODE(t) ( ((u16)(t / TI_TMP112_LSB_STEP)) << 4 )
#define TI_TMP112_CONVERT_C_TO_13_BIT_CODE(t) ( ((u16)(t / TI_TMP112_LSB_STEP)) << 3 )

typedef enum __TI_TMP112_ConversionRate{
    TI_TMP112_025Hz = 0,
    TI_TMP112_1Hz = 1,
    TI_TMP112_4Hz = 2,
    TI_TMP112_8Hz = 3
}TI_TMP112_ConversionRate;

typedef enum __TI_TMP112_ConsecutiveFaults{
    TI_TMP112_1Fault = 0,
    TI_TMP112_2Faults = 1,
    TI_TMP112_4Faults = 2,
    TI_TMP112_6Faults = 3
}TI_TMP112_ConsecutiveFaults;

typedef enum __TI_TMP112_ConverterReslution{
    TI_TMP112_Unknown = 0,
    TI_TMP112_12Bits = 3
}TI_TMP112_ConverterReslution;

typedef struct __TI_TMP112_ConfigurationRegister{
    u8 bOneShotConversionReady: 1; /*!< One-shot / conversion ready (OS) */
    TI_TMP112_ConverterReslution mConverterReslution; /*!< Converter resolution (R1, R0) */
    TI_TMP112_ConsecutiveFaults mConsecutiveFaults; /*!< Fault queue (F1, F0) */
    u8 bPolarity: 1; /*!< Polarity (POL) */
    u8 bThermostatMode: 1;  /*!< Thermostat mode (TM) */
    u8 bShutdownMode: 1; /*!< Shutdown Mode (SD) */
    TI_TMP112_ConversionRate mConversionRate; /*!< Conversion rate (CR1, CR0) */
    u8 bAlert: 1; /*!< Alert (AL Bit) */
    u8 bExtendedMode: 1; /*! Extended mode (EM) */
}TI_TMP112_ConfigurationRegister;

typedef struct __TI_TMP112_AlarmThresholds{
    f32 fTHigh; /*!< High-limit threshold (in C degrees) */
    f32 fTLow; /*!< Low-limit threshold (in C degrees) */    
}TI_TMP112_AlarmThresholds;
    
void TI_TMP112_InitPeripherialInterface(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, u8 nAddress );
f32 TI_TMP112_ReadTemperature();
_BOOL TI_TMP112_SetConfig( TI_TMP112_ConfigurationRegister* tConfig );
_BOOL TI_TMP112_GetConfig( TI_TMP112_ConfigurationRegister* tConfig );
_BOOL TI_TMP112_SetThresholds( TI_TMP112_AlarmThresholds* tThresholds );
_BOOL TI_TMP112_GetThresholds( TI_TMP112_AlarmThresholds* tThresholds );

#endif //__TI_TMP112_H
