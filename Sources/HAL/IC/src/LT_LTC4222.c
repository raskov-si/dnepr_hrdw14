/*!
\file PMBus_HotSwap_Driver.h
\brief Driver for Linear Technology LTC4222 Hot-Swap Controller.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 12.05.2012
*/

#include "HAL\IC\inc\LT_LTC4222.h"
#include "uCOS_II.H"


/* Для упрощения написания соотв. драйверов */
#define __PMB_WriteByte          (*tPmbusPeriphInterface->PMB_WriteByte)
#define __PMB_ReadByte           (*tPmbusPeriphInterface->PMB_ReadByte)
#define __PMB_ReadWord           (*tPmbusPeriphInterface->PMB_ReadWord)
#define __PMB_WriteWord          (*tPmbusPeriphInterface->PMB_WriteWord)
#define __PMB_SendCommand        (*tPmbusPeriphInterface->PMB_SendCommand)
#define __PMB_ReadMultipleBytes  (*tPmbusPeriphInterface->PMB_ReadMultipleBytes)
#define __PMB_WriteMultipleBytes (*tPmbusPeriphInterface->PMB_WriteMultipleBytes)
#define __PMB_ReadMultipleWords  (*tPmbusPeriphInterface->PMB_ReadMultipleWords)
#define __PMB_WriteMultipleWords (*tPmbusPeriphInterface->PMB_WriteMultipleWords)


/*!
\addtogroup PMBus_HotSwap_Driver
\{
*/

/*!
\defgroup PMBus_HotSwap_Driver_Exported_Functions
\{
*/

_BOOL LT_LTC4222_GetAdcVoltages( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcVoltagesStructure* tAdcVoltagesStructure){
	/*!
	\brief Gets all ADC voltage values.
	\details This function is an alternative to \c LT_LTC4222_GetSourceVoltage(), \c LT_LTC4222_GetAdinVoltage()
	\details and \c LT_LTC4222_GetSenseVoltage() as it's optimized for bulk data readings.
	\details Use it only if you need to read all voltage values at once.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param tAdcVoltagesStructure Target structure.
	\note All returned values (SENSEx, ADINx and SOURCEx) are represented in \em volts.
	\retval 1.
	*/
	
	u8 nAdcCodeMsb, nAdcCodeLsb;
	u8 nAdcControl;
	_BOOL ret = TRUE ;
	
	assert(tPmbusPeriphInterface!=NULL);
	
	/*
	There are several ways to achieve up-to date voltage values with this controller.
	Here the ADC runs conversions continuously (15 times per second).
	Whenever we need to read voltages, we halt it, read all needed registers and restart it again.
	*/
	
	/*Halting ADC before reading registers*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADC_CONTROL, &nAdcControl );
      	OSTimeDly(1);
	ret &= __PMB_WriteByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADC_CONTROL, LT_LTC4222_ADC_SET_HALT(nAdcControl));
	
	/*SOURCE1*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SOURCE1_MSB, &nAdcCodeMsb );
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SOURCE1_LSB, &nAdcCodeLsb );
	tAdcVoltagesStructure->fSource1 = (f32)PMB_ADC_RAW_SOURCE_TO_VOLTS( _WORD(nAdcCodeMsb, nAdcCodeLsb) );
	
	/*SOURCE2*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SOURCE2_MSB, &nAdcCodeMsb );
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SOURCE2_LSB, &nAdcCodeLsb );
	tAdcVoltagesStructure->fSource2 = (f32)PMB_ADC_RAW_SOURCE_TO_VOLTS( _WORD(nAdcCodeMsb, nAdcCodeLsb) );
	
	/*ADIN1*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADIN1_MSB, &nAdcCodeMsb );
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADIN1_LSB, &nAdcCodeLsb );
	tAdcVoltagesStructure->fAdin1 = (f32)PMB_ADC_RAW_ADIN_TO_VOLTS( _WORD(nAdcCodeMsb, nAdcCodeLsb) );
	
	/*ADIN2*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADIN2_MSB, &nAdcCodeMsb );
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADIN2_LSB, &nAdcCodeLsb );
	tAdcVoltagesStructure->fAdin2 = (f32)PMB_ADC_RAW_ADIN_TO_VOLTS( _WORD(nAdcCodeMsb, nAdcCodeLsb) );
		
	/*SENSE1*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SENSE1_MSB, &nAdcCodeMsb );
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SENSE1_LSB, &nAdcCodeLsb );
	tAdcVoltagesStructure->fSense1 = (f32)PMB_ADC_RAW_SENSE_TO_VOLTS( _WORD(nAdcCodeMsb, nAdcCodeLsb) );
	
	/*SENSE2*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SENSE2_MSB, &nAdcCodeMsb );
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_SENSE2_LSB, &nAdcCodeLsb );
	tAdcVoltagesStructure->fSense2 = (f32)PMB_ADC_RAW_SENSE_TO_VOLTS( _WORD(nAdcCodeMsb, nAdcCodeLsb) );
	
	/*Restarting ADC*/
      	OSTimeDly(1);
	ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADC_CONTROL, &nAdcControl );
      	OSTimeDly(1);
	ret &= __PMB_WriteByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADC_CONTROL, LT_LTC4222_ADC_RESET_HALT(nAdcControl));
	
	return ret;
}

_BOOL LT_LTC4222_SetConfig( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel, LT_LTC4222_ControlRegisterTypedef* tControlRegisterStructure, LT_LTC4222_FaultAlertRegisterTypedef* tAlertRegisterStructure){
	/*!
	\brief Sets Control and Alert registers.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param tControlRegisterStructure Control register values. Set to \ref NULL if you do not wish to reconfigure this register.
	\param tAlertRegisterStructure Alert register values. Set to \ref NULL if you do not wish to reconfigure this register.
	\retval 1 if the transactions were successful, 0 otherwise.
	\todo Find out if Fet on control bit is read-only.
	*/
	u8 nControl=0x00, nAlert=0x00;
	u8 nControlAddress=0x00, nAlertAddress=0x00;
	_BOOL bWriteResult=1;	

	assert(tPmbusPeriphInterface!=NULL);
	
	/* 	Setting register addresses. This is for 2-channel controller. */
	if( mChannel==LT_LTC4222_CHANNEL1 ){
		nControlAddress = LT_LTC4222_CONTROL1;
		nAlertAddress   = LT_LTC4222_ALERT1;
	}
	if( mChannel==LT_LTC4222_CHANNEL2 ){
		nControlAddress = LT_LTC4222_CONTROL2;
		nAlertAddress   = LT_LTC4222_ALERT2;
	}

	//Control register
	if(tControlRegisterStructure!=NULL){
		
		LT_LTC4222_SET_GPIO_CONFIG(nControl,tControlRegisterStructure->tGpioConfig);
		
		if(tControlRegisterStructure->bFetOnControl){
			LT_LTC4222_SET_FET_ON_CONTROL(nControl); //This bit may be read-only!
		}
		if( (tControlRegisterStructure->bMassWriteEnable)&&(mChannel==LT_LTC4222_CHANNEL2) ){
			LT_LTC4222_SET_MASS_WRITE_ENABLE(nControl);
		}
		if(tControlRegisterStructure->bGpioOutputState){
			LT_LTC4222_SET_GPO_HIGH(nControl);
		}
		if(tControlRegisterStructure->bOcAutoRetry){
			LT_LTC4222_SET_OC_AUTO_RETRY(nControl);
		}
		if(tControlRegisterStructure->bUvAutoRetry){
			LT_LTC4222_SET_UV_AUTO_RETRY(nControl);
		}
		if(tControlRegisterStructure->bOvAutoRetry){
			LT_LTC4222_SET_OV_AUTO_RETRY(nControl);
		}
			
		//Writing Control register to controller 
		bWriteResult &= __PMB_WriteByte(tPmbusPeriphInterface, nAddress, nControlAddress, nControl);
	}
	//Alert register
	if(tAlertRegisterStructure!=NULL){
		
		if(tAlertRegisterStructure->bFetShort){
			LT_LTC4222_SET_FET_SHORT_ALERT(nAlert);
		}
		if(tAlertRegisterStructure->bEnStateChange){
			LT_LTC4222_SET_nEN_STATE_CHANGE_ALERT(nAlert);
		}
		if(tAlertRegisterStructure->bPowerBad){
			LT_LTC4222_SET_POWER_BAD_ALERT(nAlert);
		}
		if(tAlertRegisterStructure->bOverCurrent){
			LT_LTC4222_SET_OC_ALERT(nAlert);
		}
		if(tAlertRegisterStructure->bUnderVoltage){
			LT_LTC4222_SET_UV_ALERT(nAlert);
		}
		if(tAlertRegisterStructure->bOverVoltage){
			LT_LTC4222_SET_OV_ALERT(nAlert);
		}

		//Writing Alert register to controller 
		bWriteResult &= __PMB_WriteByte(tPmbusPeriphInterface, nAddress, nAlertAddress, nAlert);
	}
	
	return bWriteResult;
}

_BOOL LT_LTC4222_GetConfig( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel, LT_LTC4222_ControlRegisterTypedef* tControlRegisterStructure, LT_LTC4222_FaultAlertRegisterTypedef* tAlertRegisterStructure){
	/*!
	\brief Gets Control and Alert register values.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param mChannel Hot Swap controller channel. See \ref LT_LTC4222_HotswapChannelTypedef.
	\param tControlRegisterStructure Target Control register structure. Set to \ref NULL if you do not wish to read this register. 
	\param tAlertRegisterStructure Target Alert register structure. Set to \ref NULL if you do not wish to read this register. 
	\retval 1.
	*/

	u8 nControl=0x00, nAlert=0x00;
	u8 nControlAddress=0x00, nAlertAddress=0x00;
	_BOOL ret = TRUE ;

	assert(tPmbusPeriphInterface!=NULL);
	
	/* 	Setting register addresses. This is for 2-channel controller. */
	if( mChannel==LT_LTC4222_CHANNEL1 ){
		nControlAddress = LT_LTC4222_CONTROL1;
		nAlertAddress   = LT_LTC4222_ALERT1;
	}
	if( mChannel==LT_LTC4222_CHANNEL2 ){
		nControlAddress = LT_LTC4222_CONTROL2;
		nAlertAddress   = LT_LTC4222_ALERT2;
	}
	
	/* Control register. */
	if(tControlRegisterStructure!=NULL){		
		/* Reading control register. */
		ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, nControlAddress, &nControl );
		tControlRegisterStructure->tGpioConfig      = (LT_LTC4222_GpioConfigTypedef) LT_LTC4222_GET_GPIO_CONFIG(nControl);
		tControlRegisterStructure->bGpioOutputState = LT_LTC4222_GET_GPO_STATE(nControl);
		tControlRegisterStructure->bMassWriteEnable = LT_LTC4222_GET_MASS_WRITE_STATE(nControl);
		tControlRegisterStructure->bFetOnControl    = LT_LTC4222_GET_FET_ON_CONTROL(nControl);
		tControlRegisterStructure->bOcAutoRetry     = LT_LTC4222_GET_OC_AUTO_RETRY(nControl);
		tControlRegisterStructure->bUvAutoRetry     = LT_LTC4222_GET_UV_AUTO_RETRY(nControl);
		tControlRegisterStructure->bOvAutoRetry     = LT_LTC4222_GET_OV_AUTO_RETRY(nControl);		
	}
	
	/* Alert register. */
	if(tAlertRegisterStructure!=NULL){		
		/* Reading alert register. */
		ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, nAlertAddress, &nAlert );
		
		tAlertRegisterStructure->bFetShort      = LT_LTC4222_GET_FET_SHORT_ALERT(nAlert);
		tAlertRegisterStructure->bEnStateChange = LT_LTC4222_GET_nEN_STATE_CHANGE_ALERT(nAlert);
		tAlertRegisterStructure->bPowerBad      = LT_LTC4222_GET_POWER_BAD_ALERT(nAlert);
		tAlertRegisterStructure->bOverCurrent   = LT_LTC4222_GET_OC_ALERT(nAlert);
		tAlertRegisterStructure->bUnderVoltage  = LT_LTC4222_GET_UV_ALERT(nAlert);
		tAlertRegisterStructure->bOverVoltage   = LT_LTC4222_GET_OV_ALERT(nAlert);		
	}	
	
	return ret ;
}

_BOOL LT_LTC4222_ResetFaults( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface,  u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel )
{
	/*!
	\brief Сбрасывает все биты в регистре FAULT в заданном канале
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param mChannel Hot Swap controller channel. See \ref LT_LTC4222_HotswapChannelTypedef.
	\retval 1.
	*/
	u8 nAlertAddress = 0x00 ;

	assert(tPmbusPeriphInterface!=NULL);
	
	/* 	Setting register addresses. This is for 2-channel controller. */
	if( mChannel==LT_LTC4222_CHANNEL1 ){
		nAlertAddress   = LT_LTC4222_FAULT1 ;
	}
	if( mChannel==LT_LTC4222_CHANNEL2 ){
		nAlertAddress   = LT_LTC4222_FAULT2 ;
	}
	__PMB_WriteByte(tPmbusPeriphInterface, nAddress, nAlertAddress, 0x00 );
	
	return 1 ;
}

_BOOL LT_LTC4222_GetStates( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_HotswapChannelTypedef mChannel, LT_LTC4222_StatusRegisterTypedef* tStatusRegisterStructure, LT_LTC4222_FaultAlertRegisterTypedef* tFaultRegisterStructure){
	/*!
	\brief Gets State register values.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param mChannel Hot Swap controller channel. See \ref LT_LTC4222_HotswapChannelTypedef.
	\param tStatusRegisterStructure Target Status register structure. Set to \ref NULL if you do not wish to read this register. 
	\param tFaultRegisterStructure Target Fault register structure. Set to \ref NULL if you do not wish to read this register. 
	
	\retval 1.	
	*/
	u8 nStatus, nFault;
	u8 nStatusAddress, nFaultAddress;
	_BOOL ret = TRUE ;

	assert(tPmbusPeriphInterface!=NULL);
	
	/* 	Setting register addresses. This is for 2-channel controller. */
	if( mChannel==LT_LTC4222_CHANNEL1 ){
		nStatusAddress = LT_LTC4222_STATUS1;
		nFaultAddress  = LT_LTC4222_FAULT1;
	}
	if( mChannel==LT_LTC4222_CHANNEL2 ){
		nStatusAddress = LT_LTC4222_STATUS2;
		nFaultAddress  = LT_LTC4222_FAULT2;
	}
	
	if(tStatusRegisterStructure!=NULL){
		/* Reading status register. */
          	OSTimeDly(1);   
		ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, nStatusAddress, &nStatus );
		
		tStatusRegisterStructure->bFetOn          = LT_LTC4222_GET_FET_ON(nStatus);
		tStatusRegisterStructure->bGpioInputState = LT_LTC4222_GET_GPI_STATE(nStatus);
		tStatusRegisterStructure->bFetShort       = LT_LTC4222_GET_FET_SHORT(nStatus);
		tStatusRegisterStructure->bEnState        = LT_LTC4222_GET_nEN_STATE(nStatus);
		tStatusRegisterStructure->bPowerBadState  = LT_LTC4222_GET_POWER_BAD(nStatus);
		tStatusRegisterStructure->bOverCurrent    = LT_LTC4222_GET_OC(nStatus);
		tStatusRegisterStructure->bUnderVoltage   = LT_LTC4222_GET_UV(nStatus);
		tStatusRegisterStructure->bOverVoltage    = LT_LTC4222_GET_OV(nStatus);
	}
	
	if(tFaultRegisterStructure!=NULL){
		/* Reading status register. */
          	OSTimeDly(1);   
		ret &= __PMB_ReadByte(tPmbusPeriphInterface, nAddress, nFaultAddress, &nFault );
		
		tFaultRegisterStructure->bFetShort      = LT_LTC4222_GET_FET_SHORT_FAULT(nFault);
		tFaultRegisterStructure->bEnStateChange = LT_LTC4222_GET_nEN_STATE_CHANGE_FAULT(nFault);
		tFaultRegisterStructure->bPowerBad      = LT_LTC4222_GET_POWER_BAD_FAULT(nFault);
		tFaultRegisterStructure->bOverCurrent   = LT_LTC4222_GET_OC_FAULT(nFault);
		tFaultRegisterStructure->bUnderVoltage  = LT_LTC4222_GET_UV_FAULT(nFault);
		tFaultRegisterStructure->bOverVoltage   = LT_LTC4222_GET_OV_FAULT(nFault);
	}
	
	return ret ;
}

u16 LT_LTC4222_GetRawAdcData( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel){
	/*!
	\brief Gets raw ADC data. 10-bit, left justified.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param mAdcChannel ADC channel to read from. See \ref LT_LTC4222_AdcChannelTypedef.
	\retval ADC code from given channel. LSB's 0..5 = 0.
	*/
	u8 nChannelMsb, nChannelLsb;
	u8 nAdcDataMsb, nAdcDataLsb;

	assert(tPmbusPeriphInterface!=NULL);
	
	//Setting registers' addresses
	switch(mAdcChannel){
	case LT_LTC4222_ADC_SOURCE1:
		nChannelMsb=LT_LTC4222_SOURCE1_MSB;
		nChannelLsb=LT_LTC4222_SOURCE1_LSB;
		break;
	case LT_LTC4222_ADC_SOURCE2:
		nChannelMsb=LT_LTC4222_SOURCE2_MSB;
		nChannelLsb=LT_LTC4222_SOURCE2_LSB;
		break;
	case LT_LTC4222_ADC_ADIN1:
		nChannelMsb=LT_LTC4222_ADIN1_MSB;
		nChannelLsb=LT_LTC4222_ADIN1_LSB;
		break;
	case LT_LTC4222_ADC_ADIN2:
		nChannelMsb=LT_LTC4222_ADIN2_MSB;
		nChannelLsb=LT_LTC4222_ADIN2_LSB;
		break;
	case LT_LTC4222_ADC_SENSE1:
		nChannelMsb=LT_LTC4222_SENSE1_MSB;
		nChannelLsb=LT_LTC4222_SENSE1_LSB;
		break;
	case LT_LTC4222_ADC_SENSE2:
		nChannelMsb=LT_LTC4222_SENSE2_MSB;
		nChannelLsb=LT_LTC4222_SENSE2_LSB;
		break;
	default:
		return 0;
	}
	
	//Reading data
	__PMB_ReadByte(tPmbusPeriphInterface, nAddress, nChannelMsb, &nAdcDataMsb );
	__PMB_ReadByte(tPmbusPeriphInterface, nAddress, nChannelLsb, &nAdcDataLsb );
	
	//Return word
	return _WORD(nAdcDataMsb, nAdcDataLsb);
}

_BOOL LT_LTC4222_SetAdcControl( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcControlRegisterTypedef* tAdcControlRegisterStructure){
	/*!
	\brief Sets ADC Control registeer values.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param tAdcControlRegisterStructure ADC Control register values structure.
	\retval 1 if the transaction was successful, 0 otherwise.
	*/
	u8 nAdcControl=0x00;

	assert(tPmbusPeriphInterface!=NULL);
	
	if(tAdcControlRegisterStructure==NULL) return 0;
	
	if(tAdcControlRegisterStructure->bAdcAlert){
		LT_LTC4222_ADC_SET_ALERT(nAdcControl);
	}
	if(tAdcControlRegisterStructure->bHalt){
		LT_LTC4222_ADC_SET_HALT(nAdcControl);
	}
	LT_LTC4222_ADC_SET_CHANNEL(nAdcControl, tAdcControlRegisterStructure->tAdcChannel);
	
	return __PMB_WriteByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADC_CONTROL, nAdcControl);
}

_BOOL LT_LTC4222_GetAdcControl( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcControlRegisterTypedef* tAdcControlRegisterStructure){
	/*!
	\brief Gets ADC Control register values.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param tAdcControlRegisterStructure Target ADC Control register values structure.
	\retval 1.
	*/
	u8 nAdcControl;
	_BOOL ret ;

	assert(tPmbusPeriphInterface!=NULL);
	
	if(tAdcControlRegisterStructure==NULL) return 0;

	ret = __PMB_ReadByte(tPmbusPeriphInterface, nAddress, LT_LTC4222_ADC_CONTROL, &nAdcControl );
	
	tAdcControlRegisterStructure->bAdcBusy    = LT_LTC4222_ADC_GET_BUSY(nAdcControl);
	tAdcControlRegisterStructure->bAdcAlert   = LT_LTC4222_ADC_GET_ALERT(nAdcControl);
	tAdcControlRegisterStructure->tAdcChannel = (LT_LTC4222_AdcChannelTypedef)LT_LTC4222_ADC_GET_CHANNEL(nAdcControl);
	tAdcControlRegisterStructure->bHalt       = LT_LTC4222_ADC_GET_HALT(nAdcControl);
	
	return ret ;
}

f32 LT_LTC4222_GetSourceVoltage( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel){
	/*!
	\brief Gets Source voltage.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param mAdcChannel ADC channel to read from. Should be \p LT_LTC4222_ADC_SOURCE1 or \p LT_LTC4222_ADC_SOURCE1.
	\retval Measured voltage in volts or \p PMB_ADC_ERROR in case of wrong parameter.
	*/
	u16 nAdcCode;

	assert(tPmbusPeriphInterface!=NULL);
	
	//If channel parameter is wrong...
	if( (mAdcChannel!=LT_LTC4222_ADC_SOURCE1)&&(mAdcChannel!=LT_LTC4222_ADC_SOURCE2) ){
		return PMB_ADC_ERROR;
	}
	
	//Getting RAW ADC code
	nAdcCode = LT_LTC4222_GetRawAdcData( tPmbusPeriphInterface, nAddress, mAdcChannel );
	//Turning it into a real-world value.
	return (f32)PMB_ADC_RAW_SOURCE_TO_VOLTS(nAdcCode);
}

f32 LT_LTC4222_GetAdinVoltage( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel){
	/*!
	\brief Gets ADIN voltage.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param mAdcChannel ADC channel to read from. Should be \p LT_LTC4222_ADC_ADIN1 or \p LT_LTC4222_ADC_ADIN2.
	\retval Measured voltage in volts or \p PMB_ADC_ERROR in case of wrong parameter.
	*/
	u16 nAdcCode;

	assert(tPmbusPeriphInterface!=NULL);
	
	//If channel parameter is wrong...
	if( (mAdcChannel!=LT_LTC4222_ADC_ADIN1)&&(mAdcChannel!=LT_LTC4222_ADC_ADIN2) ){
		return PMB_ADC_ERROR;
	}
	
	//Getting RAW ADC code
	nAdcCode = LT_LTC4222_GetRawAdcData( tPmbusPeriphInterface, nAddress, mAdcChannel);
	//Turning it into a real-world value.
	return (f32)PMB_ADC_RAW_ADIN_TO_VOLTS(nAdcCode);
}
f32 LT_LTC4222_GetSenseVoltage( PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface, u8 nAddress, LT_LTC4222_AdcChannelTypedef mAdcChannel){
	/*!
	\brief Gets Sense voltage.
	\param nAddress Device PMBus/I2C address. Left-justified.
	\param mAdcChannel ADC channel to read from. Should be \p LT_LTC4222_ADC_SENSE1 or \p LT_LTC4222_ADC_SENSE2.
	\retval Measured voltage in volts or \p PMB_ADC_ERROR in case of wrong parameter.
	\warning Returned value is represented in volts, not millivolts!
	*/
	u16 nAdcCode;

	assert(tPmbusPeriphInterface!=NULL);
	
	//If channel parameter is wrong...
	if( (mAdcChannel!=LT_LTC4222_ADC_SENSE1)&&(mAdcChannel!=LT_LTC4222_ADC_SENSE2) ){
		return PMB_ADC_ERROR;
	}
	
	//Getting RAW ADC code
	nAdcCode = LT_LTC4222_GetRawAdcData( tPmbusPeriphInterface, nAddress, mAdcChannel);
	//Turning it into a real-world value.
	return (f32)PMB_ADC_RAW_SENSE_TO_VOLTS(nAdcCode);	
}
/*!
\}
*/ //PMBus_HotSwap_Driver_Exported_Functions

/*!
\}
*/ //PMBus_HotSwap_Driver

