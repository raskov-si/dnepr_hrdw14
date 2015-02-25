/*!
\file MAX_MAX31785.c
\brief Driver for MAX31785 fan controller.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 26.03.2012
\version 0.12 (26 mar 2012)
\todo Implement all fault handling routines.
*/

#include "HAL/IC/inc/MAX_MAX31785.h"

#define     MAX31785_PMBUS_TIMEOUTMS    (100)

/* Pointer to peripherial access stricture and a \c define assigned to it */
static PMB_PeriphInterfaceTypedef* tPsuPeriphInterface = NULL;
#define PMB_PERIPH_INTERFACE_STRUCT_PTR tPsuPeriphInterface

static PMB_MAX31785_ADDR __pmbus_addr ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MAX31785_InitPeripherialInterface(PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, PMB_MAX31785_ADDR addr )
{
	/*!
	\brief Inits driver's peripherial interface.
	\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
	\param tPmbusPeriphInterface Pointer to PMB_PeriphInterfaceTypedef structure.
	\warning This driver does not copy tPmbusPeriphInterface fields, it stores only this pointer.
	\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
	*/	
	assert( tPmbusPeriphInterface != NULL );
	PMB_PERIPH_INTERFACE_STRUCT_PTR = tPmbusPeriphInterface;
	__pmbus_addr = addr ;
}

s32 __round(f32 fValue){
/*!
\brief Rounds closest value to closest integer.
\retval Closest integer value to the input parameter.
\todo Correct negative number support.
*/
	s32 nValue;

	nValue = ((fValue - floor(fValue)) >= 0.5) ? (s32)ceil(fValue) : (s32)floor(fValue);
	return nValue;
}

/*!
\defgroup MAX31785_Driver
\{
*/

/*!
\defgroup MAX31785_Private_Functions
\{
*/

/*!
\defgroup MAX31785_Data_Conversion_Functions
\{
*/

f32 __MAX31785_CodeToVoltage(s32 nCode){
/*!
\brief Converts DIRECT format code to a proper floating-point voltage value (in mV).
\warning All voltage-related parameters of PMBus devices are reported as positive values.
*/
	return __MAX31785_CODE_TO_VALUE(nCode,
		MAX31785_VOLTAGE_m,
		MAX31785_VOLTAGE_b,
		MAX31785_VOLTAGE_R);
}
f32 __MAX31785_CodeToVoltageScaling(s32 nCode){
/*!
\brief Converts DIRECT format code to a floating-point voltage scaling value.
*/
	return __MAX31785_CODE_TO_VALUE(nCode,
		MAX31785_VOLTAGE_SCALING_m,
		MAX31785_VOLTAGE_SCALING_b,
		MAX31785_VOLTAGE_SCALING_R);
}
f32 __MAX31785_CodeToTemperature(s32 nCode){
/*!
\brief Converts DIRECT format code to a floating-point temperature value (in @htmlonly &deg; @endhtmlonly C).
*/
	return __MAX31785_CODE_TO_VALUE(nCode,
		MAX31785_TEMPERATURE_m,
		MAX31785_TEMPERATURE_b,
		MAX31785_TEMPERATURE_R);
}
f32 __MAX31785_CodeToFanSpeedRPM(s32 nCode){
/*!
\brief Converts DIRECT format code to a floating-point RPM value.
*/
	return __MAX31785_CODE_TO_VALUE(nCode,
		MAX31785_FAN_SPEED_RPM_m,
		MAX31785_FAN_SPEED_RPM_b,
		MAX31785_FAN_SPEED_RPM_R);
}
f32 __MAX31785_CodeToFanSpeedPercent(s32 nCode){
/*!
\brief Converts DIRECT format code to a floating-point PWM duty cycle value (in \%).
*/
		return __MAX31785_CODE_TO_VALUE(nCode,
		MAX31785_FAN_SPEED_PERCENT_m,
		MAX31785_FAN_SPEED_PERCENT_b,
		MAX31785_FAN_SPEED_PERCENT_R);
}

u16 __MAX31785_VoltageToCode(f32 fValue){
/*!
\brief Converts floating-point voltage value (in mV) to a PMBus DIRECT code.
\details Valid input values: 0 to 32767 mV with 1 mV step.
*/
	return __round(__MAX31785_VALUE_TO_CODE(fValue,
		MAX31785_VOLTAGE_m,
		MAX31785_VOLTAGE_b,
		MAX31785_VOLTAGE_R));
}
u16 __MAX31785_VoltageScalingToCode(f32 fValue){
/*!
\brief Converts floating-point voltage voltage scaling value to a PMBus DIRECT code.
\details Valid input values: 0 to 1 with 1/32767 step.
*/
	return __round(__MAX31785_VALUE_TO_CODE(fValue,
		MAX31785_VOLTAGE_SCALING_m,
		MAX31785_VOLTAGE_SCALING_b,
		MAX31785_VOLTAGE_SCALING_R));
}
u16 __MAX31785_TemperatureToCode(f32 fValue){
/*!
\brief Converts floating-point temperature value (in C) to a PMBus DIRECT code.
\details Valid input values: 0 to 327.67 C with 0.01 C step.
*/
	return __round(__MAX31785_VALUE_TO_CODE(fValue,
		MAX31785_TEMPERATURE_m,
		MAX31785_TEMPERATURE_b,
		MAX31785_TEMPERATURE_R));
}
u16 __MAX31785_FanSpeedRPMToCode(f32 fValue){/*!
\brief Converts floating-point fan speed value (in RPM) to a PMBus DIRECT code.
\details Valid input values: 0 to 32767 RPM with 1 RPM step.
*/
	return __round(__MAX31785_VALUE_TO_CODE(fValue,
		MAX31785_FAN_SPEED_RPM_m,
		MAX31785_FAN_SPEED_RPM_b,
		MAX31785_FAN_SPEED_RPM_R));
}
u16 __MAX31785_FanSpeedPercentToCode(f32 fValue){
/*!
\brief Converts floating-point PWM duty cycle value (in \%) to a PMBus DIRECT code.
\details Valid input values: 0 to 327.67 \% with 0.01 \% step.
*/
	return __round(__MAX31785_VALUE_TO_CODE(fValue,
		MAX31785_FAN_SPEED_PERCENT_m,
		MAX31785_FAN_SPEED_PERCENT_b,
		MAX31785_FAN_SPEED_PERCENT_R));
}

/*!
\}
*/ //MAX31785_Data_Conversion_Functions

/*!
\defgroup MAX31785_Register_Access_Functions
\details These functions access internal device registers directly.
\details However, they could also be changed to access these registers
\details stored elsewhere (for example, RAM) to keep bus load low.
\{
*/

_BOOL MAX31785_SetPage(u8 mPage){
	/*
	\brief Sets number of PMBus page to read/write. 
	\details Use before every read/write command (or section of commands). 
	\param mPage Number of page: 0..22 or 255. 
	\retval 1. 
	*/
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_PAGE, mPage, MAX31785_PMBUS_TIMEOUTMS, 3);
	return 1;
}
_BOOL MAX31785_GetPage(void){
	u8 res ;
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_PAGE, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}

//Private functions for fans
//(either read directly from device of preloaded data from RAM or something else)
u8 __MAX31785_GetFanConfig12(u8 nPage){
	_BOOL res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(MAX31785_FAN_PWM_PAGE(nPage));
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_FAN_CONFIG_1_2, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
u16 __MAX31785_GetFanCommand1(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(MAX31785_FAN_PWM_PAGE(nPage));
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_FAN_COMMAND_1, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
u8 __MAX31785_GetStatusFans12(u8 nPage){
	u8 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(MAX31785_FAN_PWM_PAGE(nPage));
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_STATUS_FANS_1_2, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
_BOOL __MAX31785_SetFanConfig12(u8 nPage, u8 mCmd){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(MAX31785_FAN_PWM_PAGE(nPage));
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_FAN_CONFIG_1_2, mCmd, MAX31785_PMBUS_TIMEOUTMS, 3);
}
_BOOL __MAX31785_SetFanCommand1(u8 nPage, u16 mCmd){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(MAX31785_FAN_PWM_PAGE(nPage));
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_FAN_COMMAND_1, mCmd, MAX31785_PMBUS_TIMEOUTMS, 3);
}
u16 __MAX31785_GetMfrTempSensorConfig(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_TEMP_SENSOR_CONFIG, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
_BOOL __MAX31785_SetMfrTempSensorConfig(u8 nPage, u16 mCmd){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_TEMP_SENSOR_CONFIG, mCmd, MAX31785_PMBUS_TIMEOUTMS, 3);
}
u16 __MAX31785_GetReadTemperature1(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_READ_TEMPERATURE_1, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
_BOOL __MAX31785_GetMfrFanPwm2Rpm(u8 nPage, u16 *anValues){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadMultipleWords(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_PWM2RPM, anValues, MAX31785_PWM2RPM_POINTS, MAX31785_PMBUS_TIMEOUTMS, 3);
	return 1;
}
_BOOL __MAX31785_GetMfrFanLut(u8 nPage, u16 *anValues){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadMultipleWords(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_LUT, anValues, MAX31785_LUT_POINTS*2, MAX31785_PMBUS_TIMEOUTMS, 3);
	return 1;
}
_BOOL __MAX31785_SetMfrFanLut(u8 nPage, u16 *anValues){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteMultipleWords(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_LUT, anValues, MAX31785_LUT_POINTS*2, MAX31785_PMBUS_TIMEOUTMS, 3);
}
_BOOL __MAX31785_SetMfrFanPwm2Rpm(u8 nPage, u16 *anValues){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteMultipleWords(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_PWM2RPM, anValues, MAX31785_PWM2RPM_POINTS, MAX31785_PMBUS_TIMEOUTMS, 3);
}
u16 __MAX31785_GetReadFanSpeed1(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_READ_FAN_SPEED_1, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
u16 __MAX31785_GetMfrReadFanPwm(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_READ_FAN_PWM, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
u16 __MAX31785_GetMfrFanRuntime(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_RUN_TIME, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
u16 __MAX31785_GetMfrFanPwmAvg(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_PWM_AVG, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
u16 __MAX31785_GetMfrFanConfig(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_CONFIG, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}
_BOOL __MAX31785_SetMfrFanConfig(u8 nPage, u16 mCmd){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_CONFIG, mCmd, MAX31785_PMBUS_TIMEOUTMS, 3);
}
_BOOL __MAX31785_SetMfrFanFaultLimit(u8 nPage, u16 mCmd){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_FAULT_LIMIT, mCmd, MAX31785_PMBUS_TIMEOUTMS, 3);
}

u16 __MAX31785_GetMfrFanFaultLimit(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_FAN_FAULT_LIMIT, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}

_BOOL __MAX31785_SetOtFaultLimit(u8 nPage, u16 mCmd){
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_OT_FAULT_LIMIT, mCmd, MAX31785_PMBUS_TIMEOUTMS, 3);
}

u16 __MAX31785_GetOtFaultLimit(u8 nPage){
	u16 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage(nPage);
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_OT_FAULT_LIMIT, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}

u8 __MAX31785_GetCapability(u8 nPage)
{
	u8 res ;
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage( nPage );
	PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_CAPABILITY, &res, MAX31785_PMBUS_TIMEOUTMS, 3 );
	return res ;
}

/*!
\}
*/ //MAX31785_Register_Access_Functions

/*!
\}
*/ //MAX31785_Private_Functions

/*!
\defgroup MAX31785_Exported_Functions
\{
*/

_BOOL MAX31785_SetMfrMode( const u8 mfr_mode_val )
{
	//! \brief ”станавливает значение регистра MFR_MODE
	//! \param mfr_mode_val значение регистра
	//! \retval TRUE в случае удачи
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	MAX31785_SetPage( 22 );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_MFR_MODE, mfr_mode_val, MAX31785_PMBUS_TIMEOUTMS, 3);
}

_BOOL MAX31785_GetStatusByte( u8 *res )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	assert( res != NULL );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_STATUS_BYTE, res, MAX31785_PMBUS_TIMEOUTMS, 3 );
}

_BOOL MAX31785_GetStatusWord( u16 *res )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	assert( res != NULL );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_STATUS_WORD, res, MAX31785_PMBUS_TIMEOUTMS, 3 );
}

_BOOL MAX31785_GetStatusCML( u8* res )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	assert( res != NULL );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_STATUS_CML, res, MAX31785_PMBUS_TIMEOUTMS, 3 );
}

_BOOL MAX31785_GetMfrSpecific( u8* res )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	assert( res != NULL );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_STATUS_MFR_SPECIFIC, res, MAX31785_PMBUS_TIMEOUTMS, 3);	
}


_BOOL __MAX31785_StoreDefaultAll(){
	/*!
	\brief Instructs the device to transfer the device configuration information to the internal flash memory array.
	\details Not all information is stored. Only configuration data is stored, not any status or operational data.
	\warning It is NOT recommended to use this command while the device is operating fans. The device is unresponsive to PMBus commands and does not monitor fans while transferring the configuration.
	\warning VDD must be above 2.9V for the device to perform this command.
	\retval 1 if transmission was successful, 0 otherwise.
	*/
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_SendCommand(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_STORE_DEFAULT_ALL, MAX31785_PMBUS_TIMEOUTMS, 3);
}
_BOOL __MAX31785_RestoreDefaultAll(){
	/*!
	\brief Transfers the default configuration information from the internal flash memory array to the user memory registers in the device.
	\warning This command should only be executed when the device is not operating the fans.
	*/
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_SendCommand(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_RESTORE_DEFAULT_ALL, MAX31785_PMBUS_TIMEOUTMS, 3);
}

_BOOL __MAX31785_ClearFaults(){
	/*!
	\brief Clears any fault or warning bits in the status registers that have been set.
	\retval 1 if transmission was successful, 0 otherwise.
	*/
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	return PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_SendCommand( PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_CLEAR_FAULTS, MAX31785_PMBUS_TIMEOUTMS, 3);
}


_BOOL MAX31785_EnableFan(u8 nChannel){
	/*!
	\brief Enables fan.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nPage;

	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	if ( MAX31785_IS_FAN_ENABLED(nConfig12) ){
		return MAX31785_RETURN_UNCHANGED;
	}
	else{
		return __MAX31785_SetFanConfig12(nPage, MAX31785_FAN_SET_ENABLE(nConfig12) );
	}
}
_BOOL MAX31785_DisableFan(u8 nChannel){
	/*!
	\brief Disables fan.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nPage;
	u8 nConfig12;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	if ( MAX31785_IS_FAN_DISABLED(nConfig12) ){
		return MAX31785_RETURN_UNCHANGED;
	}
	else{
		return __MAX31785_SetFanConfig12(nPage, MAX31785_FAN_RESET_ENABLE(nConfig12) );
	}
}

_BOOL MAX31785_SetTachPulses( const u8 nChannel, const MAX31785_TACHPULSES pulses )
{
	u8 nConfig12, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12 = __MAX31785_GetFanConfig12(nPage);
	nConfig12 |= pulses ;
	return __MAX31785_SetFanConfig12( nPage, nConfig12 );
}

_BOOL MAX31785_SetAutomaticRPM(u8 nChannel){
	/*!
	\brief Sets automatic RPM control mode.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nCommand1, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	nCommand1=__MAX31785_GetFanCommand1(nPage);
	__MAX31785_SetFanConfig12(nPage, MAX31785_FAN_SET_RPM_CONTROL(nConfig12));
	return __MAX31785_SetFanCommand1(nPage, MAX31785_SET_AUTO_RPM_CONTROL(nCommand1));
}
_BOOL MAX31785_SetAutomaticPWM(u8 nChannel){
	/*!
	\brief Sets automatic PWM control mode.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nCommand1, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	nCommand1=__MAX31785_GetFanCommand1(nPage);
	__MAX31785_SetFanConfig12(nPage, MAX31785_FAN_SET_PWM_CONTROL(nConfig12));
	return __MAX31785_SetFanConfig12(nPage, MAX31785_SET_AUTO_PWM_CONTROL(nCommand1));
}

_BOOL MAX31785_SetManualRPM(u8 nChannel){
	/*!
	\brief Sets manual RPM control mode.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nCommand1, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	nCommand1=__MAX31785_GetFanCommand1(nPage);
	__MAX31785_SetFanConfig12(nPage, MAX31785_FAN_SET_RPM_CONTROL(nConfig12));
	return __MAX31785_SetFanCommand1(nPage, MAX31785_RESET_AUTO_RPM_CONTROL(nCommand1));
}
_BOOL MAX31785_SetManualPWM(u8 nChannel){
	/*!
	\brief Sets manual PWM control mode.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nCommand1, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	nCommand1=__MAX31785_GetFanCommand1(nPage);
	__MAX31785_SetFanConfig12(nPage, MAX31785_FAN_SET_PWM_CONTROL(nConfig12));
	return __MAX31785_SetFanCommand1(nPage, MAX31785_RESET_AUTO_PWM_CONTROL(nCommand1));
}

_BOOL MAX31785_SetRPMSpeed(u8 nChannel, f32 fValue){
	/*!
	\brief Sets fan speed (in RPM).
	\param nChannel Number of fan (0 through 5).
	\param fValue Fan speed (in RPM).
	\warning Overrides value set by MAX31785_SetPWMSpeed().
	*/
	u8 nCommand1, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nCommand1=__MAX31785_GetFanCommand1(nPage);
	return __MAX31785_SetFanCommand1( nPage, MAX31785_FAN_SET_RPM_SPEED( nCommand1, __MAX31785_FanSpeedRPMToCode(fValue) ) );
}
_BOOL MAX31785_SetPWMSpeed(u8 nChannel, f32 fValue){
	/*!
	\brief Sets fan PWM duty cycle (in \%).
	\param nChannel Number of fan (0 through 5).
	\param fValue PWM duty cycle (in \%).
	\warning Overrides value set by MAX31785_SetRPMSpeed().
	*/
	u8 nCommand1, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nCommand1=__MAX31785_GetFanCommand1(nPage);
	return __MAX31785_SetFanCommand1( nPage, MAX31785_FAN_SET_PWM_SPEED( nCommand1, __MAX31785_FanSpeedPercentToCode(fValue) ) );
}

f32 MAX31785_GetFanRPM(u8 nChannel){
	/*!
	\brief Gets fan speed.
	\param nChannel Number of fan (0 through 5).
	\retval Fan speed in RPM set by user.
	\warning This is speed set by user and not a measured value.
	\warning To get a measured current speed value, use \ref MAX31785_ReadFanRPM()
	*/
	u8 nPage;
	u16 nCommand1;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nCommand1=__MAX31785_GetFanCommand1(nPage);
	return __MAX31785_CodeToFanSpeedRPM(MAX31785_FAN_GET_RPM_SPEED(nCommand1));
}
f32 MAX31785_GetFanPWM(u8 nChannel){
	/*!
	\brief Gets fan PWM duty cycle.
	\param nChannel Number of fan (0 through 5).
	\retval Fan PWM duty cycle (in \%) set by user.
	\warning This is PWM duty cycle value set by user and not a measured value.
	\warning To get current PWM value, use \ref MAX31785_ReadFanPWM()
	*/
	u8 nPage;
	u16 nCommand1;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nCommand1=__MAX31785_GetFanCommand1(nPage);	
	return __MAX31785_CodeToFanSpeedPercent(MAX31785_FAN_GET_PWM_SPEED(nCommand1));
}

_BOOL MAX31785_IsFanEnabled(u8 nChannel){
	/*!
	\brief Returns 1 if fan is enabled, 0 otherwise.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	return ((MAX31785_IS_FAN_ENABLED(nConfig12)) ? 1 : 0 );
}
_BOOL MAX31785_IsFanRPMControlled(u8 nChannel){
	/*!
	\brief Returns 1 if fan is RPM controlled, 0 otherwise.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	
	return ((MAX31785_IS_FAN_RPM_CONTROLLED(nConfig12)) ? 1 : 0 );
}
_BOOL MAX31785_IsFanPWMControlled(u8 nChannel){
	/*!
	\brief Returns 1 if fan is PWM controlled, 0 otherwise.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nConfig12, nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nConfig12=__MAX31785_GetFanConfig12(nPage);
	return MAX31785_IS_FAN_PWM_CONTROLLED(nConfig12);
}
_BOOL MAX31785_IsFanAutoControlled(u8 nChannel){
	/*!
	\brief Returns 1 if fan is automatically controlled, 0 otherwise.
	\param nChannel Number of fan (0 through 5).
	*/
	u8 nCommand1, nPage;

	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nCommand1=__MAX31785_GetFanCommand1(nPage);	
	return MAX31785_IS_FAN_AUTO_CONTROLLED(nCommand1);

}
s8 MAX31785_GetFanStatus(u8 nChannel){
	/*!
	\brief Returns current fan health status.
	\retval 'r' for red, 'o' for orange, 'g' for green, 'y' for yellow, 'u' for unknown.
	\param nChannel Number of fan (0 through 5).
	\bug Does not always work in 40-60% PWM range.
	*/
	u8 nStatusFans12, nPage, nColour;

	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nStatusFans12=__MAX31785_GetStatusFans12(nPage);
	nColour=MAX31785_EXTRACT_FAN_COLOUR(nStatusFans12);

	//Returns colour in more user-friendly format
	switch(nColour){
	case MAX31785_FAN_RED:    return MAX31785_FAN_STATUS_CHAR_RED;
	case MAX31785_FAN_GREEN:  return MAX31785_FAN_STATUS_CHAR_GREEN;
	case MAX31785_FAN_ORANGE: return MAX31785_FAN_STATUS_CHAR_ORANGE;
	case MAX31785_FAN_YELLOW: return MAX31785_FAN_STATUS_CHAR_YELLOW;
	default: return MAX31785_FAN_STATUS_CHAR_UNKNOWN;
	}
}

_BOOL MAX31785_FanRampOnFault(u8 nChannel, _BOOL bState){
	/*!
	\brief Sets Fan response to fan fault.
	\details Whether ramp fan to 100% PWM duty cycle if fan or temp sensor fault is occured or not. \em Note: clears (or sets) both TACHO and TSFO bits.
	\param nChannel Number of fan (0 through 5).
	\param bState State of fault response (1 to ramp fan to 100% PWM duty cycle if fan fault is detected, 0 to leave normal operation).
	\retval 1 if the transaction was successful, 0 otherwise.
	\warning bState is not TACHO value, actually it is opposite to it.
	*/	
	u8 nPage;
	u16 nMfrFanConfig, nNewMfrFanConfig;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanConfig=__MAX31785_GetMfrFanConfig(nPage);

	nNewMfrFanConfig = ( bState ? MAX31785_FAN_RESET_TACHO_TSFO(nMfrFanConfig) : MAX31785_FAN_SET_TACHO_TSFO(nMfrFanConfig) );	
	return __MAX31785_SetMfrFanConfig(nPage, nNewMfrFanConfig);
}

_BOOL MAX31785_EnableTempSensor(u8 nChannel){
		/*!
	\brief Returns 1 if fan is RPM controlled, 0 otherwise.
	\param nChannel Number of fan (0 through 5).
	*/
	u16 nMfrTempSensorConfig=0;

	nMfrTempSensorConfig=__MAX31785_GetMfrTempSensorConfig(MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel));
	
	if( MAX31785_IS_TSENS_ENABLED(nMfrTempSensorConfig) ) return 0;
	else{
		__MAX31785_SetMfrTempSensorConfig(MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel), MAX31785_TSENS_ENABLE(nMfrTempSensorConfig));
		return 1;
	}
}
f32 MAX31785_ReadTempSensorTemperature(u8 nChannel){
	/*!
	\brief Gets temperarure sensor measured value.
	\param nChannel Number of temperature sensor (0 through 10).
	\retval Temperarure in C.
	*/
	u16 nReadTemperature1=0;
	nReadTemperature1=__MAX31785_GetReadTemperature1(MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel));
	return __MAX31785_CodeToTemperature(nReadTemperature1);
}
u8 MAX31785_TempSensorGetFans(u8 nChannel){
	/*!
	\brief Gets list of fans assigned to a certain temperature sensor.
	\param nChannel Number of temperature sensor (0 through 10).
	\retval Bitmask containing list of assigned fans. Use \ref MAX31785_TSENS_DOES_CONTROL_FAN() macro to decode. 
	*/
	u16 nMfrTempSensorConfig=0;
	nMfrTempSensorConfig=__MAX31785_GetMfrTempSensorConfig(MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel));
	return MAX31785_TSENS_EXTRACT_FANS(nMfrTempSensorConfig);
}

_BOOL MAX31785_TempSensorSetFans(u8 nChannel, u8 mFans){
	/*!
	\brief Binds fans to be controlled by a temperature sensor.
	\param nChannel Number of temperature sensor (0 through 10).
	\param mFans Fans to be controlled. See \ref MAX31785_TSENS_CONTROL_FAN() and \ref MAX31785_TSENS_CONTROL_ALL_FANS macros. (See following example.)
	\code
		//Example
		MAX31785_TempSensorSetFans(6, MAX31785_TSENS_CONTROL_ALL_FANS);
		MAX31785_TempSensorSetFans(5, MAX31785_TSENS_CONTROL_FAN(1) | MAX31785_TSENS_CONTROL_FAN(4) );
	\endcode
	\retval 1 if transmission was successful, 0 otherwise.
	\warning Does not override previously bound fans, i. e. this function adds fan to sensor's fan list. To remove fan from control list use \ref MAX31785_TempSensorResetFans().

	*/
	u16 nMfrTempSensorConfig=0;
	u8 nPage;

	nPage=MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel);
	nMfrTempSensorConfig=__MAX31785_GetMfrTempSensorConfig(nPage);

	return __MAX31785_SetMfrTempSensorConfig( nPage, MAX31785_TSENS_SET_FANS(nMfrTempSensorConfig, mFans) );

}
_BOOL MAX31785_TempSensorResetFans(u8 nChannel, u8 mFans){
	/*!
	\brief Unbinds fans to not be controlled by a temperature sensor.
	\details Usage is completely similar to \ref MAX31785_TempSensorSetFans().
	\param nChannel Number of temperature sensor (0 through 10).
	\param mFans Fans to \em not be controlled. See \ref MAX31785_TSENS_CONTROL_FAN() and \ref MAX31785_TSENS_CONTROL_ALL_FANS macros.
	\retval 1 if transmission was successful, 0 otherwise.
	*/
	u16 nMfrTempSensorConfig=0;
	u8 nPage;

	nPage=MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel);
	nMfrTempSensorConfig=__MAX31785_GetMfrTempSensorConfig(nPage);

	return __MAX31785_SetMfrTempSensorConfig( nPage, MAX31785_TSENS_RESET_FANS(nMfrTempSensorConfig, mFans) );

}


f32 MAX31785_GetTempOffset(u8 nChannel){	/*!
	\brief Gets temperature offset of a temperature sensor.
	\param nChannel Number of temperature sensor (0 through 10).
	\retval Temperature offset value in C. 
	*/
	u16 nMfrTempSensorConfig=0;
	nMfrTempSensorConfig=__MAX31785_GetMfrTempSensorConfig(MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel));
	return (f32)MAX31785_TSENS_EXTRACT_OFFSET(nMfrTempSensorConfig);
}
_BOOL MAX31785_IsTempSensorEnabled(u8 nChannel){	/*!
	\brief Gets state of a certain temperature sensor.
	\param nChannel Number of temperature sensor (0 through 10).
	\retval 1 if sensor is enabled, 0 otherwise. 
	*/
	u16 nMfrTempSensorConfig=0;
	nMfrTempSensorConfig=__MAX31785_GetMfrTempSensorConfig(MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel));
	return ( MAX31785_IS_TSENS_ENABLED(nMfrTempSensorConfig) ? 1 : 0 );
}

void MAX31785_LUTStructureInit(MAX31785_LUTStructure* fanLutStructure){
	/*!
	\brief Fills each MAX31785_LUTStructure field with its default value. 
	\param fanLutStructure A pointer to a MAX31785_LUTStructure structure which will be initialized.
	\retval None.
	*/
	u8 i;
	for(i=0; i<MAX31785_LUT_POINTS; i++){
		fanLutStructure->fPwm[i]=0.;
		fanLutStructure->fTemperature[i]=0.;
	}
}
void MAX31785_PWM2RPMStructureInit(MAX31785_PWM2RPMStructure* fanPwm2RpmStructure){
	/*!
	\brief Fills each MAX31785_PWM2RPMStructure field with its default value. 
	\param fanPwm2RpmStructure A pointer to a MAX31785_PWM2RPMStructure structure which will be initialized.
	\retval None.
	*/
	u8 i;
	f32 fPwmValues[]=MAX31785_PWM2RPM_PWM_VALUES;
	for(i=0; i<MAX31785_PWM2RPM_POINTS; i++){
		fanPwm2RpmStructure->fPwm[i]=fPwmValues[i];
		fanPwm2RpmStructure->fRpm[i]=0.;
	}
}
_BOOL MAX31785_GetFanLUT(u8 nChannel, MAX31785_LUTStructure* fanLutStructure){
	/*!
	\brief Fills each MAX31785_LUTStructure field with its defined value read from the device. 
	\param fanLutStructure A pointer to a MAX31785_LUTStructure structure which will be used.
	\retval 1.
	*/
	u16 anLut[MAX31785_LUT_POINTS*2];
	u8 i, j=0;
	MAX31785_LUTStructureInit(fanLutStructure);	
	__MAX31785_GetMfrFanLut(nChannel, anLut);
	for(i=0; i<MAX31785_LUT_POINTS*2; i+=2, j++){
		fanLutStructure->fTemperature[j]=__MAX31785_CodeToTemperature(anLut[i]);
		fanLutStructure->fPwm[j]=__MAX31785_CodeToFanSpeedPercent(anLut[i+1]);
	}
	return 1;
}
_BOOL MAX31785_GetFanPWM2RPM(u8 nChannel, MAX31785_PWM2RPMStructure* fanPwm2RpmStructure){
	/*!
	\brief Fills each MAX31785_PWM2RPMStructure field with its defined value read from the device. 
	\param fanPwm2RpmStructure A pointer to a MAX31785_PWM2RPMStructure structure which will be used.
	\retval 1.
	*/
	u16 anRpmList[MAX31785_PWM2RPM_POINTS];
	u8 i;
	MAX31785_PWM2RPMStructureInit(fanPwm2RpmStructure);
	__MAX31785_GetMfrFanPwm2Rpm(nChannel, anRpmList);
	for(i=0; i<MAX31785_PWM2RPM_POINTS; i++){
		fanPwm2RpmStructure->fRpm[i]= __MAX31785_CodeToFanSpeedRPM(anRpmList[i]);
	}
	return 1;
}
_BOOL MAX31785_SetFanLUT(u8 nChannel, MAX31785_LUTStructure* fanLutStructure){
	/*!
	\brief Fills LUT in the device memory according to MAX31785_LUTStructure related field values. 
	\param fanLutStructure A pointer to a MAX31785_LUTStructure structure which will be used.
	\retval 1.
	*/
	u16 anLut[MAX31785_LUT_POINTS*2];
	u8 i, j=0;
	for(i=0; i<MAX31785_LUT_POINTS*2; i+=2, j++){
		anLut[i]=__MAX31785_TemperatureToCode(fanLutStructure->fTemperature[j]);
		anLut[i+1]=__MAX31785_FanSpeedPercentToCode(fanLutStructure->fPwm[j]);
	}
	return __MAX31785_SetMfrFanLut(nChannel, anLut);
}
_BOOL MAX31785_SetFanPWM2RPM(u8 nChannel, MAX31785_PWM2RPMStructure* fanPwm2RpmStructure){
	/*!
	\brief Fills PWM2RPM table in the device memory according to MAX31785_PWM2RPMStructure related field values. 
	\param fanPwm2RpmStructure A pointer to a MAX31785_PWM2RPMStructure structure which will be used.
	\retval 1.
	*/
	u16 anRpmList[MAX31785_PWM2RPM_POINTS];
	u8 i;
	for(i=0; i<MAX31785_PWM2RPM_POINTS; i++){
		anRpmList[i]=__MAX31785_FanSpeedRPMToCode(fanPwm2RpmStructure->fRpm[i]);
	}
	return __MAX31785_SetMfrFanPwm2Rpm(nChannel, anRpmList);
}

_BOOL MAX31785_ReadFanRPM( u16 *res )
{
	/*!
	\brief Gets current fan speed.
	\param res куда пишутс€ оборты
	\retval ”спешность чтени€
	\warning Ќеобходимо заранее установить страницу
	*/
	_BOOL ret ;
	u16 rpm ;
	assert( res );

	ret = PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, __pmbus_addr, PMB_READ_FAN_SPEED_1, &rpm, MAX31785_PMBUS_TIMEOUTMS, 3 );
	*res = (u16)__MAX31785_CodeToFanSpeedRPM( rpm );

	return ret ;
}
f32 MAX31785_ReadFanPWM(u8 nChannel){
	/*!
	\brief Gets current fan PWM duty cycle.
	\param nChannel Number of fan (0 through 5).
	\retval Current fan PWM duty cycle (in \%).
	\warning This function gets a recently measured value, i. e. actual PWM duty cycle.
	\warning To get a duty cycle value set by user, use \ref MAX31785_GetFanPWM()
	*/
	u8 nPage;
	u16 nMfrReadFanPwm;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrReadFanPwm=__MAX31785_GetMfrReadFanPwm(nPage);
	return __MAX31785_CodeToFanSpeedPercent(nMfrReadFanPwm);
}
u16 MAX31785_GetFanRuntime(u8 nChannel){
	/*!
	\brief Gets current fan runtime.
	\param nChannel Number of fan (0 through 5).
	\retval Current fan runtime in hours.
	*/
	u8 nPage;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	return __MAX31785_GetMfrFanRuntime(nPage);
}
f32 MAX31785_GetFanAveragePWM(u8 nChannel){
	/*!
	\brief Gets the lifetime average of the fan PWM duty cycle.
	\details When combined with the runtime information, this duty cycle helps predict the remaining lifetime of the fan.
	\param nChannel Number of fan (0 through 5).
	\retval Average fan PWM duty cycle (in \%).
	*/
	u8 nPage;
	u16 nMfrFanPwmAvg;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanPwmAvg=__MAX31785_GetMfrFanPwmAvg(nPage);
	return __MAX31785_CodeToFanSpeedPercent(nMfrFanPwmAvg);
}
u16 MAX31785_GetFanPWMFrequency(u8 nChannel){
	/*!
	\brief Gets current PWM frequency applied to the fan.
	\param nChannel Number of fan (0 through 5).
	\retval PWM frequency in Hz.
	*/
	u8 nPage;
	u16 nMfrFanConfig;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanConfig=__MAX31785_GetMfrFanConfig(nPage);

	switch(MAX31785_FAN_EXTRACT_PWM_FREQUENCY(nMfrFanConfig)){
	case MAX31785_FREQ_30Hz:  return 30;
	case MAX31785_FREQ_50Hz:  return 50;
	case MAX31785_FREQ_100Hz: return 100;
	case MAX31785_FREQ_150Hz: return 150;
	case MAX31785_FREQ_25kHz: return 25000;
	default: return 0;
	}
}

_BOOL MAX31785_SetFanPWMFrequency(u8 nChannel, MAX31785_PWMFrequency mFrequency){
	/*!
	\brief Sets current PWM frequency applied to the fan.
	\param nChannel Number of fan (0 through 5).
	\param mFrequency New PWM frequency value.
	\retval 1.
	*/
	u8 nPage;
	u16 nMfrFanConfig;
	
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanConfig=__MAX31785_GetMfrFanConfig(nPage);

	return __MAX31785_SetMfrFanConfig(nPage, MAX31785_FAN_SET_PWM_FREQUENCY(nMfrFanConfig, mFrequency));

}

_BOOL MAX31785_SetSpeedFaultLimitRPM(u8 nChannel, f32 fValue){
	/*!
	\brief Sets RPM fan speed fault limit.
	\details Sets the value of the fan speed (in RPM mode) that causes a fan speed warning. Fans operating \b below these limits for over 10s continuous trip the fault.
	\param nChannel Number of fan (0 through 5).
	\param fValue Fan speed value (RPM); \em fValue set to \ref MAX31785_NO_FAULT_LIMIT disables speed limit control.
	\retval 1 if the transaction was successful, 0 otherwise.
	\warning Overrides value set by MAX31785_SetSpeedFaultLimitPWM().
	\bug Does not seem to cause fault condition if current speed happens to be below this limit (unlike MAX31785_SetSpeedFaultLimitPWM()).
	*/
	u16 nMfrFanFaultLimit;
	u8 nPage;
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanFaultLimit=__MAX31785_FanSpeedRPMToCode(fValue);
	return __MAX31785_SetMfrFanFaultLimit(nPage, nMfrFanFaultLimit);
}
_BOOL MAX31785_SetSpeedFaultLimitPWM(u8 nChannel, f32 fValue){
	/*!
	\brief Sets PWM fan speed fault limit.
	\details Sets the percentage of target fan speed (in PWM mode) that causes a fan speed warning.
	\param nChannel Number of fan (0 through 5).
	\param fValue Fan speed value (PWM duty cycle, \%); \em fValue set to \ref MAX31785_NO_FAULT_LIMIT disables speed limit control. Fans operating \b below these limits for over 10s continuous trip the fault.
	\retval 1 if the transaction was successful, 0 otherwise.
	\warning Overrides value set by MAX31785_SetSpeedFaultLimitRPM().
	*/
	u16 nMfrFanFaultLimit;
	u8 nPage;
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanFaultLimit=__MAX31785_FanSpeedPercentToCode(fValue);
	return __MAX31785_SetMfrFanFaultLimit(nPage, nMfrFanFaultLimit);
}
f32 MAX31785_GetSpeedFaultLimitRPM(u8 nChannel){
	/*!
	\brief Gets RPM fan speed fault limit.
	\details Gets the value of the fan speed (in RPM mode) that causes a fan speed warning.
	\param nChannel Number of fan (0 through 5).
	\retval Fan speed value (RPM); or \ref MAX31785_NO_FAULT_LIMIT if speed limit control disabled.
	*/	
	u16 nMfrFanFaultLimit;
	u8 nPage;
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanFaultLimit=__MAX31785_GetMfrFanFaultLimit(nPage);
	return __MAX31785_CodeToFanSpeedRPM(nMfrFanFaultLimit);
}
f32 MAX31785_GetSpeedFaultLimitPWM(u8 nChannel){
	/*!
	\brief Gets PWM fan speed fault limit.
	\details Gets the value of the fan speed (in RPM mode) that causes a fan speed warning.
	\param nChannel Number of fan (0 through 5).
	\retval Fan speed value (PWM duty cycle, \%); or \ref MAX31785_NO_FAULT_LIMIT if speed limit control disabled.
	*/	
	u16 nMfrFanFaultLimit;
	u8 nPage;
	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanFaultLimit=__MAX31785_GetMfrFanFaultLimit(nPage);
	return __MAX31785_CodeToFanSpeedPercent(nMfrFanFaultLimit);
}

_BOOL MAX31785_SetFanSpinUp(u8 nChannel, MAX31785_SpinUpRevolutionsNumber mRevolutions){
	/*!
	\brief Sets number fan spin-up revolutions.
	\param nChannel Number of fan (0 through 5).
	\param mRevolutions See \ref MAX31785_SpinUpRevolutionsNumber. 
	\retval 1 if the transaction was successful, 0 otherwise.
	*/
	u8 nPage;
	u16 nMfrFanConfig;

	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanConfig=__MAX31785_GetMfrFanConfig(nPage);

	__MAX31785_SetMfrFanConfig(nPage, MAX31785_SET_SPINUP_REVOLUTIONS(nMfrFanConfig, mRevolutions));
	return 1;
}
_BOOL MAX31785_SetFanHealthMonitor(u8 nChannel, _BOOL bState)
{
	u8 nPage;
	u16 nMfrFanConfig;

	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanConfig=__MAX31785_GetMfrFanConfig(nPage);

	__MAX31785_SetMfrFanConfig(nPage, nMfrFanConfig | MAX31785_FAN_HEALT_MON );
	return 1;	
}
_BOOL MAX31785_GetFanHealthMonitor(u8 nChannel)
{
	u8 nPage;
	u16 nMfrFanConfig;

	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanConfig=__MAX31785_GetMfrFanConfig(nPage);

	return (nMfrFanConfig & MAX31785_FAN_HEALT_MON) > 0 ;
}
u8 MAX31785_GetFanSpinUp(u8 nChannel){
	/*!
	\brief Gets number of fan spin-up revolutions.
	\param nChannel Number of fan (0 through 5).
	\retval Number of spin-up revolutions.
	*/
	u8 nPage;
	u16 nMfrFanConfig;

	nPage=MAX31785_FAN_PWM_PAGE(nChannel);
	nMfrFanConfig=__MAX31785_GetMfrFanConfig(nPage);

	switch(MAX31785_GET_SPINUP_REVOLUTIONS(nMfrFanConfig)){
	case MAX31785_FAN_SPINUP_NONE:  return 0;
	case MAX31785_FAN_SPINUP_2_REV: return 2;
	case MAX31785_FAN_SPINUP_4_REV: return 4;
	case MAX31785_FAN_SPINUP_8_REV: return 8;
	default: return 0;
	}
}

_BOOL MAX31785_SetTemperatureFaultLimit(u8 nChannel, f32 fValue){
	/*!
	\brief Sets temperature sensor OT fault limit.
	\param nChannel Number of temp sensor (0 through 10).
	\param fValue Temperature value (in C) or \ref MAX31785_NO_TEMPERATURE_FAULT_LIMIT.
	\retval 1 if the transaction was successful, 0 otherwise.
	*/
	u16 nOtFaultLimit;
	u8 nPage;
	nPage=MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel);
	nOtFaultLimit=__MAX31785_FanSpeedPercentToCode(fValue);
	return __MAX31785_SetOtFaultLimit(nPage, nOtFaultLimit);
}

f32 MAX31785_GetTemperatureFaultLimit(u8 nChannel){
	/*!
	\brief Gets temperature sensor OT fault limit.
	\details Gets the value of the temperature that causes an overtemperature fault.
	\param nChannel Number of temp sensor (0 through 10).
	\retval Temperature value (in C).
	*/	
	u16 nOtFaultLimit;
	u8 nPage;
	nPage=MAX31785_TEMP_DIODE_SENSOR_PAGE(nChannel);
	nOtFaultLimit=__MAX31785_GetOtFaultLimit(nPage);
	return __MAX31785_CodeToTemperature(nOtFaultLimit);
}

/*!
\}
*/ //MAX31785_Exported_Functions

/*!
\}
*/ //MAX31785_Driver

