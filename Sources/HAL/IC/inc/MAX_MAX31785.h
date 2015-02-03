/*!
\file FanControl_Driver.h
\brief Header for MAX31785 driver.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 26.03.2012
*/

#ifndef __FANCONTROL_DRIVER_H
#define __FANCONTROL_DRIVER_H

#include <math.h>
#include "support_common.h"
#include "HAL/IC/inc/PMB_interface.h"
#include "HAL/IC/inc/PMBus_Commands.h"

/*!
\brief Fan controller PMBus address.
\details First digit is A1 conection, second one - A0;
\details "1"=100kOhm to Vdd (power), "0"=100kOhm to Vss (ground)
*/
typedef enum {
	PMB_MAX31785_ADDR_00 = 0xA4,  /*!< Fan controller address if A1=Low, A0=Low */
	PMB_MAX31785_ADDR_01 = 0xA6,  /*!< Fan controller address if A1=Low, A0=High */
	PMB_MAX31785_ADDR_10 = 0xA8,  /*!< Fan controller address if A1=High, A0=Low */
	PMB_MAX31785_ADDR_11 = 0xAA   /*!< Fan controller address if A1=High, A0=High */
} PMB_MAX31785_ADDR ;


//ch - number of channel (0-3 for I2C temperature sensors, 0-5 for the rest)
#define MAX31785_FAN_PWM_PAGE(ch)           (ch)
#define MAX31785_TEMP_DIODE_SENSOR_PAGE(ch) (ch+6)
#define MAX31785_TEMP_INTERNAL_SENSOR_PAGE  12u
#define MAX31785_TEMP_I2C_SENSOR_PAGE(ch)   (ch+13)
#define MAX31785_VOLTAGE_SENSOR_PAGE(ch)    (ch+17)
#define MAX31785_ALL_PAGES                  255u

//WRITE_PROTECT Command Byte
#define MAX31785_WP_ENABLE_ALL     0x00
#define MAX31785_WP_ENABLE_WP_PAGE 0x40
#define MAX31785_WP_ENABLE_WP      0x80

//FAN_CONFIG_1_2 Command Byte
#define MAX31785_FAN_ENABLED     B8(10000000)
#define MAX31785_FAN_DISABLED    B8(00000000)
#define MAX31785_FAN_RPM_CONTROL B8(01000000)
#define MAX31785_FAN_PWM_CONTROL B8(00000000)
#define MAX31785_FAN_1_TACH_P    B8(00000000)
#define MAX31785_FAN_2_TACH_P    B8(00010000)
#define MAX31785_FAN_3_TACH_P    B8(00100000)
#define MAX31785_FAN_4_TACH_P    B8(00110000)

#define MAX31785_IS_FAN_ENABLED(b)        (b & MAX31785_FAN_ENABLED)
#define MAX31785_IS_FAN_DISABLED(b)       (!MAX31785_IS_FAN_ENABLED(b))
#define MAX31785_IS_FAN_RPM_CONTROLLED(b) (b & MAX31785_FAN_RPM_CONTROL)
#define MAX31785_IS_FAN_PWM_CONTROLLED(b) (!MAX31785_IS_FAN_RPM_CONTROLLED(b))
#define MAX31785_FAN_GET_TACH_P(b)        ((b>>4)+1)

#define MAX31785_FAN_SET_ENABLE(b)   ( b | MAX31785_FAN_ENABLED )
#define MAX31785_FAN_RESET_ENABLE(b) (b & (~MAX31785_FAN_ENABLED))

#define MAX31785_FAN_SET_RPM_CONTROL(b) (b | MAX31785_FAN_RPM_CONTROL)
#define MAX31785_FAN_SET_PWM_CONTROL(b) (b & (~MAX31785_FAN_RPM_CONTROL))

#define MAX31785_FAN_SET_RPM_SPEED(b, speed) ( (b & 0x8000) | (speed & 0x7FFF) )
#define MAX31785_FAN_SET_PWM_SPEED(b, speed) ( (b & 0x8000) | (speed & 0x7FFF) )

//Exrtacts PWM/RPM speed (both for auto and manual control)
#define MAX31785_FAN_GET_RPM_SPEED(b) (b & 0x7FFF)
#define MAX31785_FAN_GET_PWM_SPEED(b) (b & 0x7FFF)

//These macro work so that initial manual speed is preserved
//Refer to datasheet
#define MAX31785_SET_AUTO_RPM_CONTROL(b)   (b | 0x8000) 
#define MAX31785_RESET_AUTO_RPM_CONTROL(b) (b & (~0x8000))
#define MAX31785_SET_AUTO_PWM_CONTROL(b)   (b | 0x8000) 
#define MAX31785_RESET_AUTO_PWM_CONTROL(b) (b & (~0x8000))

//FAN_COMMAND_1
#define MAX31785_IS_FAN_AUTO_CONTROLLED(b) (b&0x8000)

//STATUS_BYTE
#define MAX31785_SB_VOUT_OV     B8(00100000)
#define MAX31785_SB_TEMPERATURE B8(00000100)
#define MAX31785_SB_CML         B8(00000010)
#define MAX31785_SB_OTHER       B8(00000001)

#define MAX31785_SB_IS_VOUT_OV_FAULT(b)     ( b&MAX31785_SB_VOUT_OV )
#define MAX31785_SB_IS_TEMPERATURE_FAULT(b) ( b&MAX31785_SB_TEMPERATURE )
#define MAX31785_SB_IS_CML_FAULT(b)   ( b&MAX31785_SB_CML )
#define MAX31785_SB_IS_OTHER_FAULT(b) ( b&MAX31785_SB_OTHER )

//STATUS_WORD
#define MAX31785_SW_VOUT        B16(10000000, 00000000)
#define MAX31785_SW_MFR         B16(00010000, 00000000)
#define MAX31785_SW_FANS        B16(00000100, 00000000)
#define MAX31785_SW_VOUT_OV     B16(00000000, 00100000)
#define MAX31785_SW_TEMPERATURE B16(00000000, 00000100)
#define MAX31785_SW_CML         B16(00000000, 00000010)
#define MAX31785_SW_OTHER       B16(00000000, 00000001)

//STATUS_VOUT
#define MAX31785_VOUT_OV_FAULT B8(10000000)
#define MAX31785_VOUT_OV_WARN  B8(01000000)
#define MAX31785_VOUT_UV_WARN  B8(00100000)
#define MAX31785_VOUT_UV_FAULT B8(00010000)

//STATUS_CML
#define MAX31785_CML_COMM_FAULT     B8(10000000)
#define MAX31785_CML_DATA_FAULT     B8(01000000)
#define MAX31785_CML_FAULT_LOG_FULL B8(00000000)

//STATUS_MFR_SPECIFIC
#define MAX31785_SPECIFIC_OT_WARN  B8(01000000)
#define MAX31785_SPECIFIC_OT_FAULT B8(00100000)
#define MAX31785_SPECIFIC_WATCHDOG B8(00010000)

//STATUS_FANS_1_2
#define MAX31785_FAN_1_FAULT B8(10000000)
#define MAX31785_FAN_1_WARN  B8(00000000)
#define MAX31785_FAN_RED     B8(00001000)
#define MAX31785_FAN_ORANGE  B8(00000100)
#define MAX31785_FAN_YELLOW  B8(00000010)
#define MAX31785_FAN_GREEN   B8(00000001)

#define MAX31785_EXTRACT_FAN_COLOUR(b) ( b&0x0F )

#define MAX31785_IS_FAN_1_FAULT(b) ( b&MAX31785_FAN_1_FAULT )
#define MAX31785_IS_FAN_1_WARN(b) ( b&MAX31785_FAN_1_WARN )
#define MAX31785_IS_FAN_RED(b) ( b&MAX31785_FAN_RED )
#define MAX31785_IS_FAN_ORANGE(b) ( b&MAX31785_FAN_ORANGE )
#define MAX31785_IS_FAN_YELLOW(b) ( b&MAX31785_FAN_YELLOW )
#define MAX31785_IS_FAN_GREEN(b) ( b&MAX31785_FAN_GREEN )
#define MAX31785_IS_FAN_HEALTH_UNKNOWN(b) (!MAX31785_IS_FAN_RED(b))&&(!MAX31785_IS_FAN_ORANGE(b))&&(!MAX31785_IS_FAN_YELLOW(b))&&(!MAX31785_IS_FAN_GREEN(b))

#define MAX31785_FAN_STATUS_CHAR_RED     'r'
#define MAX31785_FAN_STATUS_CHAR_GREEN   'g'
#define MAX31785_FAN_STATUS_CHAR_ORANGE  'o'
#define MAX31785_FAN_STATUS_CHAR_YELLOW  'y'
#define MAX31785_FAN_STATUS_CHAR_UNKNOWN 'u'

#define MAX31785_IS_FAN_STATUS_CHAR_RED(s)     ( s==MAX31785_FAN_STATUS_CHAR_RED )
#define MAX31785_IS_FAN_STATUS_CHAR_GREEN(s)   ( s==MAX31785_FAN_STATUS_CHAR_GREEN )
#define MAX31785_IS_FAN_STATUS_CHAR_ORANGE(s)  ( s==MAX31785_FAN_STATUS_CHAR_ORANGE )
#define MAX31785_IS_FAN_STATUS_CHAR_YELLOW(s)  ( s==MAX31785_FAN_STATUS_CHAR_YELLOW )
#define MAX31785_IS_FAN_STATUS_CHAR_UNKNOWN(s) ( s==MAX31785_FAN_STATUS_CHAR_UNKNOWN )


//MFR_MODE
#define MAX31785_MFR_FORCE_NV_FAULT_LOG B16(10000000, 00000000)
#define MAX31785_MFR_CLEAR_NV_FAULT_LOG B16(01000000, 00000000)
#define MAX31785_MFR_ALERT_ENABLED  B16(00100000, 00000000)
#define MAX31785_MFR_ALERT_DISABLED B16(00000000, 00000000)
#define MAX31785_MFR_SOFT_RESET_1   B16(00001000, 00000000)
#define MAX31785_MFR_SOFT_RESET_0   B16(00000000, 00000000)
//Refer to datasheet for FAN_HEALTH_CRITERIA meaning
#define MAX31785_MFR_FAN_HEALTH_CRITERIA_1 B16(00000000, 00000000)
#define MAX31785_MFR_FAN_HEALTH_CRITERIA_2 B16(00000000, 01000000)
#define MAX31785_MFR_FAN_HEALTH_CRITERIA_3 B16(00000000, 10000000)
#define MAX31785_MFR_FAN_HEALTH_CRITERIA_4 B16(00000000, 11000000)
#define MAX31785_MFR_ADC_ENABLE(ch) (1<<ch)

//MFR_FAULT_RESPONSE
/**/
//MFR_NV_FAULT_LOG
/**/

//MFR_FAN_CONFIG

/*!
\brief Available PWM frequencies.
*/
typedef enum __MAX31785_PWMFrequency{
	MAX31785_FREQ_30Hz =  B16(00000000, 00000000),
	MAX31785_FREQ_50Hz =  B16(00100000, 00000000),
	MAX31785_FREQ_100Hz = B16(01000000, 00000000),
	MAX31785_FREQ_150Hz = B16(01100000, 00000000),
	MAX31785_FREQ_25kHz = B16(11100000, 00000000)
} MAX31785_PWMFrequency ;

#define MAX31785_FAN_DUAL_TACH B16(00010000, 00000000)
#define MAX31785_FAN_HYS_2C B16(00000000, 00000000)
#define MAX31785_FAN_HYS_4C B16(00000100, 00000000)
#define MAX31785_FAN_HYS_6C B16(00001000, 00000000)
#define MAX31785_FAN_HYS_8C B16(00001100, 00000000)
#define MAX31785_FAN_TSFO   B16(00000010, 00000000)
#define MAX31785_FAN_TACHO  B16(00000001, 00000000)

#define MAX31785_FAN_SET_TACHO(w)  ( w | MAX31785_FAN_TACHO )
#define MAX31785_FAN_RESET_TACHO(w)  ( w & (~MAX31785_FAN_TACHO) )

#define MAX31785_FAN_SET_TSFO(w)  ( w | MAX31785_FAN_TSFO )
#define MAX31785_FAN_RESET_TSFO(w)  ( w & (~MAX31785_FAN_TSFO) )

#define MAX31785_FAN_SET_TACHO_TSFO(w)  ( w | MAX31785_FAN_TSFO | MAX31785_FAN_TACHO )
#define MAX31785_FAN_RESET_TACHO_TSFO(w)  ( w & (~ ( MAX31785_FAN_TSFO | MAX31785_FAN_TACHO) ) )

//Refer to datasheet; valid s values are 0 through 7
#define MAX31785_FAN_RAMP_SPEED(s) (s<<5)
#define MAX31785_FAN_HEALTH B16(00000000, 00010000)
#define MAX31785_FAN_ROTOR_LOCK_HIGH   B16(00000000, 00001000)
#define MAX31785_FAN_ROTOR_LOCK_LOW    B16(00000000, 00000000)
#define MAX31785_FAN_ROTOR_TACH        B16(00000000, 00000000)
#define MAX31785_FAN_ROTOR_LOCK_DETECT B16(00000000, 00000100)
/*!
\brief Spin-up revolutions numbers. 
*/
typedef enum __MAX31785_SpinUpRevolutionsNumber{
	MAX31785_FAN_SPINUP_NONE  = B16(00000000, 00000000), /*!< No spin-up */
	MAX31785_FAN_SPINUP_2_REV = B16(00000000, 00000001), /*!< 2 revolutions */
	MAX31785_FAN_SPINUP_4_REV = B16(00000000, 00000010), /*!< 4 revolutions */
	MAX31785_FAN_SPINUP_8_REV = B16(00000000, 00000011)  /*!< 8 revolutions */
}MAX31785_SpinUpRevolutionsNumber;

#define MAX31785_SET_SPINUP_REVOLUTIONS(w, rev) ( ( w & 0xFFFC ) | ( rev & 0x0003) )
#define MAX31785_GET_SPINUP_REVOLUTIONS(w) ( w  & 0x0003 )
#define MAX31785_FAN_EXTRACT_PWM_FREQUENCY(w) ( w & 0xE000 )
#define MAX31785_FAN_SET_PWM_FREQUENCY(w, f) ( (w & 0x1FFF) | f )
#define MAX31785_FAN_HEALT_MON				0x0010


//MFR_TEMP_SENSOR_CONFIG
#define MAX31785_TSENS_ENABLED (1<<15)
//Temperature in C from 0 to 30 witn 1 C step is valid
#define MAX31785_TSENS_OFFSET(t) (t<<10)
//Refer to datasheet
#define MAX31785_TSENS_OFFSET_TEST (0x1F<<10)
#define MAX31785_TSENS_CONTROL_FAN(ch) (1<<ch)
#define MAX31785_TSENS_DO_NOT_CONTROL_FAN(ch) ( ~(MAX31785_TSENS_CONTROL_FAN(ch)) )
#define MAX31785_TSENS_DOES_CONTROL_FAN(b, ch) ( b & MAX31785_TSENS_CONTROL_FAN(ch) )
#define MAX31785_TSENS_CONTROL_ALL_FANS 0x003F

#define MAX31785_TSENS_EXTRACT_FANS(w) ( w & 0x003F )
#define MAX31785_TSENS_SET_FANS(w, fans)   ( w | fans )
#define MAX31785_TSENS_RESET_FANS(w, fans) ( w & (~fans) )
#define MAX31785_TSENS_EXTRACT_OFFSET(w) (w>>10)

#define MAX31785_TSENS_ENABLE(w) ( w | MAX31785_TSENS_ENABLED )
#define MAX31785_TSENS_DISABLE(w) ( w & (~MAX31785_TSENS_ENABLED) )
#define MAX31785_IS_TSENS_ENABLED(w) ( w & MAX31785_TSENS_ENABLED )



//PMBus data formats conversion constants
#define MAX31785_VOLTAGE_UNIT "mV"
#define MAX31785_VOLTAGE_RES  1
#define MAX31785_VOLTAGE_MAX  32767u
#define MAX31785_VOLTAGE_m    1
#define MAX31785_VOLTAGE_b    0
#define MAX31785_VOLTAGE_R    0

#define MAX31785_VOLTAGE_SCALING_UNIT "\0"
#define MAX31785_VOLTAGE_SCALING_RES  (1./32767)
#define MAX31785_VOLTAGE_SCALING_MAX  1
#define MAX31785_VOLTAGE_SCALING_m    32767u
#define MAX31785_VOLTAGE_SCALING_b    0
#define MAX31785_VOLTAGE_SCALING_R    0

#define MAX31785_TEMPERATURE_UNIT "C"
#define MAX31785_TEMPERATURE_RES  0.01
#define MAX31785_TEMPERATURE_MAX  327.67
#define MAX31785_TEMPERATURE_m    1
#define MAX31785_TEMPERATURE_b    0
#define MAX31785_TEMPERATURE_R    2

#define MAX31785_FAN_SPEED_RPM_UNIT "RPM"
#define MAX31785_FAN_SPEED_RPM_RES  1
#define MAX31785_FAN_SPEED_RPM_MAX  32767u
#define MAX31785_FAN_SPEED_RPM_m    1
#define MAX31785_FAN_SPEED_RPM_b    0
#define MAX31785_FAN_SPEED_RPM_R    0

#define MAX31785_FAN_SPEED_PERCENT_UNIT "%"
#define MAX31785_FAN_SPEED_PERCENT_RES  0.01
#define MAX31785_FAN_SPEED_PERCENT_MAX  327.67
#define MAX31785_FAN_SPEED_PERCENT_m    1
#define MAX31785_FAN_SPEED_PERCENT_b    0
#define MAX31785_FAN_SPEED_PERCENT_R    2

/*! General macro for code-into-value conversion. */
#define __MAX31785_CODE_TO_VALUE(code, m, b, R) (f32)((1./m)*( code*pow( 10., (s32)(-1*R) )-b ))
/*! General macro for value-to-code conversion. */
#define __MAX31785_VALUE_TO_CODE(value, m, b, R) ((value*m+b)*pow(10., (s32)R))
/*! Number of PWM-to-RPM table entries. */
#define MAX31785_PWM2RPM_POINTS 4
/*! Number of LUT table entries. */
#define MAX31785_LUT_POINTS 8
/*! Array of PWM-to-RPM table PWM values. */
#define MAX31785_PWM2RPM_PWM_VALUES {40.0, 60.0, 80.0, 100.0}

#define MAX31785_NO_FAULT_LIMIT 0.
#define MAX31785_NO_TEMPERATURE_FAULT_LIMIT 327.67
#define MAX31785_NO_FAN_FAULT_LIMIT 0.

#ifndef _BOOL
#define _BOOL u8
#endif

#define MAX31785_RETURN_SUCCESSFUL 1
#define MAX31785_RETURN_ERROR      0
#define MAX31785_RETURN_UNCHANGED  1 //for example, if one tries to turn off the fan which is already turned off

/*****************/
/*Public typedefs*/
/*****************/
/*!
\brief LUT Table structure
*/
typedef struct{
	f32 fTemperature[MAX31785_LUT_POINTS]; /*!< Temperature value (in &deg; C)*/
	f32 fPwm[MAX31785_LUT_POINTS]; /*!< PWM duty cycle (in %)*/
}MAX31785_LUTStructure;


/*!
\brief PWM to RPM table structure
\todo RPM is actually an integer value
*/
typedef struct{
	f32 fPwm[MAX31785_PWM2RPM_POINTS]; /*!< PWM duty cycle (in %)*/
	f32 fRpm[MAX31785_PWM2RPM_POINTS]; /*!< Speed value (in RPM)*/
}MAX31785_PWM2RPMStructure;

/*!
\brief Кол-во периодов сигнала тахометра на один оборот ротора вентилятора
*/
typedef enum MAX31785_TACHPULSES_{
	MAX31785_TACH1 = 0x00,
	MAX31785_TACH2 = 0x10,
	MAX31785_TACH3 = 0x20,
	MAX31785_TACH4 = 0x30
} MAX31785_TACHPULSES ;

s32 __round(f32 fValue); 

/*******************/
/*Private functions*/
/*******************/

//PMBus data formats conversion functions
//[Ready]
f32 __MAX31785_CodeToVoltage(s32 nCode); 
f32 __MAX31785_CodeToVoltageScaling(s32 nCode);  
f32 __MAX31785_CodeToTemperature(s32 nCode); 
f32 __MAX31785_CodeToFanSpeedRPM(s32 nCode); 
f32 __MAX31785_CodeToFanSpeedPercent(s32 nCode); 
u16 __MAX31785_VoltageToCode(f32 fValue);
u16 __MAX31785_VoltageScalingToCode(f32 fValue); 
u16 __MAX31785_TemperatureToCode(f32 fValue);
u16 __MAX31785_FanSpeedRPMToCode(f32 fValue);
u16 __MAX31785_FanSpeedPercentToCode(f32 fValue);


//Private functions for fans
u8 __MAX31785_GetFanConfig12(u8 nPage); //FAN_CONFIG_1_2
u16 __MAX31785_GetFanCommand1(u8 nPage); //FAN_COMMAND_1
u8 __MAX31785_GetStatusFans12(u8 nPage); //STATUS_FANS_1_2
_BOOL __MAX31785_SetFanConfig12(u8 nPage, u8 mCmd); //FAN_CONFIG_1_2
_BOOL __MAX31785_SetFanCommand1(u8 nPage, u16 mCmd); //FAN_COMMAND_1
u16 __MAX31785_GetReadFanSpeed1(u8 nPage); //READ_FAN_SPEED_1
u16 __MAX31785_GetMfrReadFanPwm(u8 nPage); //MFR_READ_FAN_PWM
u16 __MAX31785_GetMfrFanRuntime(u8 nPage); //MFR_FAN_RUN_TIME
u16 __MAX31785_GetMfrFanPwmAvg(u8 nPage); //MFR_FAN_PWM_AVG
u16 __MAX31785_GetMfrFanConfig(u8 nPage); //MFR_FAN_CONFIG
_BOOL __MAX31785_SetMfrFanConfig(u8 nPage, u16 mCmd); //MFR_FAN_CONFIG

u16 __MAX31785_GetMfrTempSensorConfig(u8 nPage); //MFR_TEMP_SENSOR_CONFIG
_BOOL __MAX31785_SetMfrTempSensorConfig(u8 nPage, u16 mCmd); //MFR_TEMP_SENSOR_CONFIG

u16 __MAX31785_GetReadTemperature1(u8 nPage); //READ_TEMPERATURE_1
_BOOL __MAX31785_GetMfrFanPwm2Rpm(u8 nPage, u16 *anValues); //MFR_FAN_PWM2RPM
_BOOL __MAX31785_SetMfrFanPwm2Rpm(u8 nPage, u16 *anValues); //MFR_FAN_PWM2RPM
_BOOL __MAX31785_GetMfrFanLut(u8 nPage, u16 *anValues); //MFR_FAN_PWM2RPM
_BOOL __MAX31785_SetMfrFanLut(u8 nPage, u16 *anValues); //MFR_FAN_PWM2RPM
u16 __MAX31785_GetMfrFanFaultLimit(u8 nPage); //MFR_FAN_FAULT_LIMIT
_BOOL __MAX31785_SetMfrFanFaultLimit(u8 nPage, u16 mCmd); //MFR_FAN_FAULT_LIMIT
u16 __MAX31785_GetOtFaultLimit(u8 nPage); //OT_FAULT_LIMIT
_BOOL __MAX31785_SetOtFaultLimit(u8 nPage, u16 mCmd); //OT_FAULT_LIMIT


/******************/
/*Public functions*/
/******************/

/*I2C/PMBus peripherial interfaces*/
void MAX31785_InitPeripherialInterface(PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, PMB_MAX31785_ADDR addr );

//Low-level general functions
_BOOL __MAX31785_StoreDefaultAll(); //STORE_DEFAULT_ALL
_BOOL __MAX31785_RestoreDefaultAll(); //RESTORE_DEFAULT_ALL
_BOOL __MAX31785_ClearFaults(); //CLEAR_FAULTS
//High-level functions for fans

_BOOL MAX31785_SetPage(u8 mPage);
_BOOL MAX31785_GetPage(void);

void MAX31785_LUTStructureInit(MAX31785_LUTStructure* fanLutStructure);
void MAX31785_PWM2RPMStructureInit(MAX31785_PWM2RPMStructure* fanPwm2RpmStructure);
_BOOL MAX31785_GetFanLUT(u8 nChannel, MAX31785_LUTStructure* fanLutStructure);
_BOOL MAX31785_SetFanLUT(u8 nChannel, MAX31785_LUTStructure* fanLutStructure);
_BOOL MAX31785_GetFanPWM2RPM(u8 nChannel, MAX31785_PWM2RPMStructure* fanPwm2RpmStructure);
_BOOL MAX31785_SetFanPWM2RPM(u8 nChannel, MAX31785_PWM2RPMStructure* fanPwm2RpmStructure);

_BOOL MAX31785_EnableFan(u8 nChannel);
_BOOL MAX31785_DisableFan(u8 nChannel);
_BOOL MAX31785_SetManualRPM(u8 nChannel); 
_BOOL MAX31785_SetManualPWM(u8 nChannel);
_BOOL MAX31785_SetAutomaticRPM(u8 nChannel);
_BOOL MAX31785_SetAutomaticPWM(u8 nChannel);
_BOOL MAX31785_SetRPMSpeed(u8 nChannel, f32 fValue);
_BOOL MAX31785_SetPWMSpeed(u8 nChannel, f32 fValue);

//! устанавливает количество импульсов тахометра на один оборот вентилятора
_BOOL MAX31785_SetTachPulses( const u8 nChannel, const MAX31785_TACHPULSES pulses );

f32 MAX31785_GetFanRPM(u8 nChannel);
f32 MAX31785_GetFanPWM(u8 nChannel);
_BOOL MAX31785_ReadFanRPM( u16 *res );
f32 MAX31785_ReadFanPWM(u8 nChannel);
_BOOL MAX31785_IsFanEnabled(u8 nChannel);
_BOOL MAX31785_IsFanRPMControlled(u8 nChannel);
_BOOL MAX31785_IsFanPWMControlled(u8 nChannel);
_BOOL MAX31785_IsFanAutoControlled(u8 nChannel);
s8 MAX31785_GetFanStatus(u8 nChannel);
u16 MAX31785_GetFanRuntime(u8 nChannel);
f32 MAX31785_GetFanAveragePWM(u8 nChannel);
_BOOL MAX31785_SetFanHealthMonitor(u8 nChannel, _BOOL bState);
_BOOL MAX31785_GetFanHealthMonitor(u8 nChannel);
_BOOL MAX31785_SetFanSpinUp(u8 nChannel, MAX31785_SpinUpRevolutionsNumber nRevolutions);
u8 MAX31785_GetFanSpinUp(u8 nChannel);
_BOOL MAX31785_SetFanPWMFrequency(u8 nChannel, MAX31785_PWMFrequency mFrequency);
u16 MAX31785_GetFanPWMFrequency(u8 nChannel);
_BOOL MAX31785_FanRampOnFault(u8 nChannel, _BOOL bState);
_BOOL MAX31785_SetSpeedFaultLimitRPM(u8 nChannel, f32 fValue);
_BOOL MAX31785_SetSpeedFaultLimitPWM(u8 nChannel, f32 fValue);
f32 MAX31785_GetSpeedFaultLimitRPM(u8 nChannel);
f32 MAX31785_GetSpeedFaultLimitPWM(u8 nChannel);

//High-level functions for temperarure sensors
_BOOL MAX31785_EnableTempSensor(u8 nChannel);
_BOOL MAX31785_SetTempOffset(u8 nChannel, u8 nValue);
f32 MAX31785_GetTempOffset(u8 nChannel);
u8 MAX31785_TempSensorGetFans(u8 nChannel);
_BOOL MAX31785_TempSensorSetFans(u8 nChannel, u8 mFans);
_BOOL MAX31785_TempSensorResetFans(u8 nChannel, u8 mFans);

_BOOL MAX31785_IsTempSensorEnabled(u8 nChannel); 
f32 MAX31785_ReadTempSensorTemperature(u8 nChannel);
_BOOL MAX31785_SetTemperatureFaultLimit(u8 nChannel, f32 fValue);
f32 MAX31785_GetTemperatureFaultLimit(u8 nChannel);

_BOOL MAX31785_SetMfrMode( const u8 mfr_mode_val );
_BOOL MAX31785_GetStatusByte( u8 *res );
_BOOL MAX31785_GetStatusWord( u16 *res );
_BOOL MAX31785_GetStatusCML( u8* res );
_BOOL MAX31785_GetMfrSpecific( u8* res );

#endif
