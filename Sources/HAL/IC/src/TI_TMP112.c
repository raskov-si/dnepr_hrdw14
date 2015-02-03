/*!
\file TI_TMP112.c
\brief Driver for Texas Instruments TMP112 Temperature sensor.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 21.08.2012
*/

#include "HAL/IC/inc/TI_TMP112.h"

static u8 __nAddress = 0 ;

/* Pointer to peripherial access stricture and a \c define assigned to it */
static I2C_PeriphInterfaceTypedef* tTmp112PeriphInterface = NULL;
#define I2C_PERIPH_INTERFACE_STRUCT_PTR tTmp112PeriphInterface

void TI_TMP112_InitPeripherialInterface(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, u8 nAddress )
{
	/*!
	\brief Inits driver's peripherial interface.
	\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
	\param tI2cPeriphInterface Pointer to I2C_PeriphInterfaceTypedef structure.
	\warning This driver does not copy tI2cPeriphInterface fields, it stores only this pointer.
	\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
	*/	
	assert(tI2cPeriphInterface != NULL);
	__nAddress = nAddress ;
	I2C_PERIPH_INTERFACE_STRUCT_PTR = tI2cPeriphInterface;
}

_BOOL TI_TMP112_GetConfig( TI_TMP112_ConfigurationRegister* tConfig ){
	/*!
	\brief Gets TMP112 configuration register.
	\param tConfig Target structure.
	\retval 1.
	*/
	u16 nConfigTmp;
	_BOOL ret ;
    
    assert(tConfig != NULL);
	
	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, __nAddress, TI_TMP112_CONFIGURATION_REGISTER, &nConfigTmp);
	
	tConfig->bOneShotConversionReady = TI_TMP112_GET_OS(nConfigTmp);
    tConfig->mConverterReslution = (TI_TMP112_ConverterReslution)TI_TMP112_GET_R1_R0(nConfigTmp);
    tConfig->mConsecutiveFaults = (TI_TMP112_ConsecutiveFaults)TI_TMP112_GET_F1_F0(nConfigTmp);
    tConfig->bPolarity = TI_TMP112_GET_POL(nConfigTmp);
    tConfig->bThermostatMode = TI_TMP112_GET_TM(nConfigTmp);
    tConfig->bShutdownMode = TI_TMP112_GET_SD(nConfigTmp);
    tConfig->mConversionRate = (TI_TMP112_ConversionRate)TI_TMP112_GET_CR1_CR0(nConfigTmp);
    tConfig->bAlert = TI_TMP112_GET_AL(nConfigTmp);
    tConfig->bExtendedMode = TI_TMP112_GET_EM(nConfigTmp);
	
	return ret ;
}

_BOOL TI_TMP112_SetConfig( TI_TMP112_ConfigurationRegister* tConfig ){
	/*!
	\brief Sets TMP112 configuration register.
	\param tConfig Structure with config register fields values.
	\retval 1.
	*/
	u16 nConfigTmp = 0x0000;
    assert(tConfig != NULL);
    
	nConfigTmp |= TI_TMP112_SET_OS(tConfig->bOneShotConversionReady);
    nConfigTmp |= TI_TMP112_SET_R1_R0((u16)(tConfig->mConverterReslution));
    nConfigTmp |= TI_TMP112_SET_F1_F0((u16)(tConfig->mConsecutiveFaults));
    nConfigTmp |= TI_TMP112_SET_POL(tConfig->bPolarity);
    nConfigTmp |= TI_TMP112_SET_TM(tConfig->bThermostatMode);
    nConfigTmp |= TI_TMP112_SET_SD(tConfig->bShutdownMode);
    nConfigTmp |= TI_TMP112_SET_CR1_CR0((u16)(tConfig->mConversionRate));
    nConfigTmp |= TI_TMP112_SET_AL(tConfig->bAlert);
    nConfigTmp |= TI_TMP112_SET_EM(tConfig->bExtendedMode);
	
	return I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, __nAddress, TI_TMP112_CONFIGURATION_REGISTER, nConfigTmp );
}


_BOOL TI_TMP112_SetThresholds( TI_TMP112_AlarmThresholds* tThresholds ){
	/*!
	\brief Sets TMP112 alarm limit registers.
	\param tThresholds Temperarure thresholds to be set.
	\retval 1.
	*/
	TI_TMP112_ConfigurationRegister tConfig;
	u16 nAlarmHCode, nAlarmLCode;
	_BOOL ret ;
    
    assert(tThresholds != NULL);
	
	/* Reading configuration register to check whether it's a extended (13-bit) mode or not */
	TI_TMP112_GetConfig( &tConfig );
    
    if(tConfig.bExtendedMode){
        nAlarmHCode = TI_TMP112_CONVERT_C_TO_13_BIT_CODE(tThresholds->fTHigh);
        nAlarmLCode = TI_TMP112_CONVERT_C_TO_13_BIT_CODE(tThresholds->fTLow);
    }
    else{        
        nAlarmHCode = TI_TMP112_CONVERT_C_TO_12_BIT_CODE(tThresholds->fTHigh);
        nAlarmLCode = TI_TMP112_CONVERT_C_TO_12_BIT_CODE(tThresholds->fTLow);
    }
	
    ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, __nAddress, TI_TMP112_THIGH_REGISTER, nAlarmHCode);
    ret &= I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, __nAddress, TI_TMP112_TLOW_REGISTER, nAlarmLCode);
	
	return ret ;
	
}

_BOOL TI_TMP112_GetThresholds( TI_TMP112_AlarmThresholds* tThresholds ){
	/*!
	\brief Gets TMP112 alarm limit registers.
	\param tThresholds Temperarure thresholds structure to be written to.
	\retval 1.
	*/
	TI_TMP112_ConfigurationRegister tConfig;
	u16 nAlarmHCode, nAlarmLCode;
	_BOOL ret ;
    
    assert(tThresholds != NULL);
	
	/* Reading configuration register to check whether it's a extended (13-bit) mode or not */
	TI_TMP112_GetConfig( &tConfig );
    
    /* Reading integer-represented thresholds */
    ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, __nAddress, TI_TMP112_THIGH_REGISTER, &nAlarmHCode );
    ret &= I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, __nAddress, TI_TMP112_TLOW_REGISTER, &nAlarmLCode );
    
    if(tConfig.bExtendedMode){
		tThresholds->fTHigh = TI_TMP112_CONVERT_13_BIT_CODE_TO_C(nAlarmHCode);
		tThresholds->fTLow = TI_TMP112_CONVERT_13_BIT_CODE_TO_C(nAlarmLCode);
    }
    else{        
		tThresholds->fTHigh = TI_TMP112_CONVERT_12_BIT_CODE_TO_C(nAlarmHCode);
		tThresholds->fTLow = TI_TMP112_CONVERT_12_BIT_CODE_TO_C(nAlarmLCode);
    }
	
	return ret ;
	
}

f32 TI_TMP112_ReadTemperature(){
	/*!
	\brief Gets TMP112 temperature in C degrees (both for 12-bit and extended 13-bit mode).
	\retval Temperature in @htmlonly &deg;@endhtmlonly C.
	*/
    u16 nRawTempCode;
	f32 fTemperature;

	I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, __nAddress, TI_TMP112_TEMPERATURE_REGISTER, &nRawTempCode );
	
	/* LSB of temperature register is mode bit, so we use it to determine temperature resolution 
       instead of reading configuration register. */
	if(TI_TMP112_CHECK_EXTENDED_MODE(nRawTempCode)){
		fTemperature = TI_TMP112_CONVERT_13_BIT_CODE_TO_C(nRawTempCode);
	}
	else{
		fTemperature = TI_TMP112_CONVERT_12_BIT_CODE_TO_C(nRawTempCode);
	}
	
    return fTemperature;
}
