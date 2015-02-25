/*!
\file TI_UCD9080.c
\brief Driver for TI UCD9080 Power-supply sequencer and monitor.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 13.08.2012
*/

#include "HAL\IC\inc\TI_UCD9080.h"
#include "support_common.h"
#include <ucos_ii.h>
#include <string.h>

#define  TI_UCD9080_I2C_TIMEOUTMS       (100)

/* Pointer to peripherial access stricture and a \c define assigned to it */
static I2C_PeriphInterfaceTypedef* tSequencerPeriphInterface = NULL;
#define I2C_PERIPH_INTERFACE_STRUCT_PTR tSequencerPeriphInterface

static TI_UCD9080_FlashlockValue TI_UCD9080_GetFlashlock();
static _BOOL TI_UCD9080_FlashlockCmd( TI_UCD9080_FlashlockValue mFlashlockValue );

static u8 _mAddress ;

/*!
\addtogroup TI_UCD9080_Driver
\{
*/

/*!
\defgroup TI_UCD9080_Driver_Exported_Functions
\{
*/

void TI_UCD9080_InitPeripherialInterface(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, const u8 mAddress ){
	/*!
	\brief Inits driver's peripherial interface.
	\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
	\param tI2cPeriphInterface Pointer to I2C_PeriphInterfaceTypedef structure.
	\warning This driver does not copy tI2cPeriphInterface fields, it stores only this pointer.
	\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
	*/	
	assert( tI2cPeriphInterface != NULL );
	I2C_PERIPH_INTERFACE_STRUCT_PTR = tI2cPeriphInterface;
    _mAddress = mAddress ;
}

f32 TI_UCD9080_ReadRailVoltage( TI_UCD9080_Rail mRail, TI_UCD9080_RailVoltageDividers* tDividers, TI_UCD9080_ReferenceVoltageType mVrefType){
	/*!
	\brief Reads a single rail voltage.
	\param nAddress I2C device address (left-justified).
	\param mRail Rail number (see \ref TI_UCD9080_Rail).
	\param tDividers Voltage divider resistro values. See \ref TI_UCD9080_RailVoltageDividers. Set to \c NULL if there is no external voltage divider.
	\param mVrefType Reference voltage type. See \ref TI_UCD9080_ReferenceVoltageType.
	\retval Voltage in V.
	*/
	f32 fDivisionCoefficient = 0.;
	f32 fVref;
	u8 nRailIndex;
	u16 nAdcRawData;
	
    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );

	nRailIndex = (u8)mRail - 1;
	
	if( (tDividers == NULL) || (tDividers->afRPulldown[nRailIndex] == 0) || (tDividers->afRPullup[nRailIndex] == 0) ){
		fDivisionCoefficient = 1.;
	}
	else{
		/* Because division by zero sucks */
		//assert( (tDividers->afRPulldown[nRailIndex] != 0) && (tDividers->afRPullup[nRailIndex] != 0) );
		
		fDivisionCoefficient = TI_UCD9080_DIVISION_COEFFICIENT(
			tDividers->afRPulldown[nRailIndex],
			tDividers->afRPullup[nRailIndex] );
	}
	
	I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_RAIL_HIGH_BASE((u8)mRail),
                            &nAdcRawData, TI_UCD9080_I2C_TIMEOUTMS, 2 );
	
	fVref = TI_UCD9080_GET_VREF(mVrefType);
	
	return (TI_UCD9080_CONVERT_VOLTAGE(nAdcRawData, fVref) * fDivisionCoefficient);
}

_BOOL TI_UCD9080_ReadRailVoltages( f32* afRailVoltages, TI_UCD9080_RailVoltageDividers* tDividers, TI_UCD9080_ReferenceVoltageType mVrefType){
	/*!
	\brief Reads a single rail voltage.
	\param nAddress I2C device address (left-justified).
	\param mRail Rail number (see \ref TI_UCD9080_Rail).
	\param tDividers Voltage divider resistro values. See \ref TI_UCD9080_RailVoltageDividers. Set to \c NULL if there is no external voltage divider.
	\param mVrefType Reference voltage type. See \ref TI_UCD9080_ReferenceVoltageType.
	\retval 1.
	*/
	TI_UCD9080_Rail mRail;
	u8 nRailIndex;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
	
	for(nRailIndex=0; nRailIndex<TI_UCD9080_RAILS_NUMBER; nRailIndex++){
		mRail = (TI_UCD9080_Rail)(nRailIndex + 1);
		afRailVoltages[nRailIndex] = TI_UCD9080_ReadRailVoltage( mRail, tDividers, mVrefType );
	}	

	return 1;
}

_BOOL TI_UCD9080_FetchError( TI_UCD9080_ErrorData* tError){
    /*!
    \brief Fetches one error from error log queue.
	\param nAddress I2C device address (left-justified).
    \param tError Target structure. See \ref TI_UCD9080_ErrorData.
	\retval 1 if transaction was successful, 0 otherwise.
    */
    u8 nErrorRegisterTmp[TI_UCD9080_ERROR_REGISTER_LENGTH];

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );

    if(!I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadMultipleBytes( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_ERROR_BASE(1), nErrorRegisterTmp, TI_UCD9080_ERROR_REGISTER_LENGTH, TI_UCD9080_I2C_TIMEOUTMS, 2)) return 0;
        
    tError->mErrorCode = (TI_UCD9080_ErrorCode)TI_UCD9080_ERROR_GET_ERROR_CODE(nErrorRegisterTmp);
    tError->mRail = (TI_UCD9080_Rail)TI_UCD9080_ERROR_GET_RAIL(nErrorRegisterTmp);
    tError->nData = TI_UCD9080_ERROR_GET_DATA(nErrorRegisterTmp);
    tError->nHour = TI_UCD9080_ERROR_GET_HOUR(nErrorRegisterTmp);
    tError->nMinutes = TI_UCD9080_ERROR_GET_MINUTES(nErrorRegisterTmp);
    tError->nSeconds = TI_UCD9080_ERROR_GET_SECONDS(nErrorRegisterTmp);
    tError->nMilliseconds = TI_UCD9080_ERROR_GET_MILLISECONDS(nErrorRegisterTmp);
        
    return 1;   
}


_BOOL TI_UCD9080_GetStatus( TI_UCD9080_Status* tStatus){
    /*!
    \brief Reads status register.
	\param nAddress I2C device address (left-justified).
    \param tStatus Target structure. See \ref TI_UCD9080_Status.
	\retval 1.
    */
    u8 nStatusTmp;
    _BOOL ret ;
    
    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );

    ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_STATUS, &nStatusTmp, TI_UCD9080_I2C_TIMEOUTMS, 2 );
    
    tStatus->bIicError = (_BOOL)TI_UCD9080_STATUS_GET_IICERROR(nStatusTmp);
    tStatus->bRail = TI_UCD9080_STATUS_GET_RAIL(nStatusTmp);
    tStatus->bNverrlog = TI_UCD9080_STATUS_GET_NVERRLOG(nStatusTmp);
    tStatus->mRegisterStatus = (TI_UCD9080_RegisterStatus)TI_UCD9080_STATUS_GET_REGISTER_STATUS(nStatusTmp);
        
    return ret ;
}


u8 TI_UCD9080_GetVersion(){
    /*!
    \brief Reads version register (raw value).
	\param nAddress I2C device address (left-justified).
	\retval Version register value.
    */
    u8 nVersionTmp;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_VERSION, &nVersionTmp, TI_UCD9080_I2C_TIMEOUTMS, 2 );
    
    return nVersionTmp;
}

u16 TI_UCD9080_GetRailStatus(){
    /*!
    \brief Reads rail status register (raw value).
	\param nAddress I2C device address (left-justified).
	\retval Rail status register value.
    */
    u16 nRailStatusTmp;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_RAILSTATUS_1,
                                &nRailStatusTmp, TI_UCD9080_I2C_TIMEOUTMS, 2 );
        
    return nRailStatusTmp;
}

_BOOL TI_UCD9080_Restart(){
    /*!
    \brief Initiates system restart.
	\param nAddress I2C device address (left-justified).
	\retval 1 if transaction was successful, 0 otherwise.
    */

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    return I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_RESTART, TI_UCD9080_RESTART_COMMAND, TI_UCD9080_I2C_TIMEOUTMS, 2);    
}


_BOOL TI_UCD9080_ReadUserData( u8* anData, u32 nSize){
    /*
    \brief Reads user data.
	\param nAddress I2C device address (left-justified).
    \param anData Pointer to a buffer to read to.
    \param nSize Number of bytes to read.
	\retval 1 if transaction was successful, 0 otherwise.
    */
    u32 i;
    u16 nWordTmp;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    for(i=0; i<nSize; i+=2){
		I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, TI_UCD9080_USER_DATA_ADDR(i), TI_UCD9080_I2C_TIMEOUTMS, 2);
                
		I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1,
                                &nWordTmp, TI_UCD9080_I2C_TIMEOUTMS, 2);
        
		anData[i]   = _MSB(nWordTmp) ;
		anData[i+1] = _LSB(nWordTmp) ;
	}
    
    return 1;
}

_BOOL TI_UCD9080_WriteUserData( u8 const* anData, u32 nSize){
    /*
    \brief Writes user data.
	\param nAddress I2C device address (left-justified).
    \param anData Pointer to a buffer to write from.
    \param nSize Number of bytes to write.
	\retval 1.
    */
    u32 i;
    u16 nWordTmp;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    TI_UCD9080_FlashlockCmd( TI_UCD9080_FlashlockUnlock );
	I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, TI_UCD9080_USER_DATA_ADDR(0), TI_UCD9080_I2C_TIMEOUTMS, 2);
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1, TI_UCD9080_MEMORY_UPDATE_CMD, TI_UCD9080_I2C_TIMEOUTMS, 2);    
    
    for(i=0; i<nSize; i+=2){       
		nWordTmp = _WORD(anData[i], anData[i+1]);
		I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, TI_UCD9080_USER_DATA_ADDR(i), TI_UCD9080_I2C_TIMEOUTMS, 2);    
        I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1, nWordTmp, TI_UCD9080_I2C_TIMEOUTMS, 2);          
	}
        
    TI_UCD9080_FlashlockCmd( TI_UCD9080_FlashlockLock );
    
    return 1;
}

u16 swap_word(u16 word) {
    u16 swap = ( (word >> 8) & 0x00FF ) | ( (word << 8) & 0xFF00 );
    return swap;
}

_BOOL TI_UCD9080_WriteConfig( u8 const* anData, u32 nSize){
    /*
    \brief Writes config data.
	\param nAddress I2C device address (left-justified).
    \param anData Pointer to a buffer to write from.
    \param nSize Number of bytes to write.
	\retval 1.
    */
    u32 i;
    u16 nWordTmp;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    TI_UCD9080_FlashlockCmd( TI_UCD9080_FlashlockUnlock );
	I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, TI_UCD9080_CONFIG_ADDR(0), TI_UCD9080_I2C_TIMEOUTMS, 2);
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1, TI_UCD9080_MEMORY_UPDATE_CMD, TI_UCD9080_I2C_TIMEOUTMS, 2);    
    
    for(i=0; i<nSize; i+=2){
        nWordTmp = _WORD(anData[i], anData[i+1]);
        I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, swap_word(swap_word(TI_UCD9080_CONFIG_ADDR_BASE) + i), TI_UCD9080_I2C_TIMEOUTMS, 2);
        I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1, nWordTmp, TI_UCD9080_I2C_TIMEOUTMS, 2);
    }
        
    TI_UCD9080_FlashlockCmd( TI_UCD9080_FlashlockLock );
    
    return 1;
}

_BOOL TI_UCD9080_ReadConfig( u8* anData, u32 nSize){
    /*
    \brief Reads config data.
	\param nAddress I2C device address (left-justified).
    \param anData Pointer to a buffer to read to.
    \param nSize Number of bytes to read.
	\retval 1 if transaction was successful, 0 otherwise.
    */
    u32 i;
    u16 nWordTmp;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    for(i=0; i<nSize; i+=2){
        I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, swap_word(swap_word(TI_UCD9080_CONFIG_ADDR_BASE) + i), TI_UCD9080_I2C_TIMEOUTMS, 2);
                
		I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1,
                                    &nWordTmp, TI_UCD9080_I2C_TIMEOUTMS, 2 );
        
		anData[i]   = _MSB(nWordTmp) ;
		anData[i+1] = _LSB(nWordTmp) ;
	}
    
    return 1;
}

/*!
\}
*/ //TI_UCD9080_Driver_Exported_Functions

/*!
\}
*/ //TI_UCD9080_Driver

static TI_UCD9080_FlashlockValue TI_UCD9080_GetFlashlock(){
    /*!
    \brief Reads version register (raw value).
	\param nAddress I2C device address (left-justified).
	\retval Current flashslock status. See \ref TI_UCD9080_FlashlockValue.
    */
    u8 nFlashlockTmp;

    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );
    
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_FLASHLOCK,
        &nFlashlockTmp, TI_UCD9080_I2C_TIMEOUTMS, 2 );
    
    return (TI_UCD9080_FlashlockValue)nFlashlockTmp;
}

static _BOOL TI_UCD9080_FlashlockCmd( TI_UCD9080_FlashlockValue mFlashlockValue){
    /*!
    \brief  A command to update flashlock.
	\param nAddress I2C device address (left-justified).
    \param mFlashlockValue Flashlock command. See \ref TI_UCD9080_FlashlockValue. Use TI_UCD9080_FlashlockWait to wait while flash is being updated and the rest to update flashlock register.
	\retval 1 if transaction was successful, 0 otherwise.
    */
    
    assert( I2C_PERIPH_INTERFACE_STRUCT_PTR != NULL );

    // Wait option
    if(mFlashlockValue == TI_UCD9080_FlashlockWait){
        while(TI_UCD9080_GetFlashlock() == TI_UCD9080_FlashlockWait);
        return 1;
    }
    
    return I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_FLASHLOCK, (u8)mFlashlockValue, TI_UCD9080_I2C_TIMEOUTMS, 2);    
}


_BOOL TI_UCD9080_ClearErrorLog(){
    /*!
    \brief Clears error log so that UCD9080 could monitor rails properly after firmware update.
    \param nAddress I2C device address (left-justified).
    \retval 1.
    */

//    TI_UCD9080_FlashlockCmd(mAddress, TI_UCD9080_FlashlockUnlock);
//  __I2C_WriteWord(mAddress, TI_UCD9080_WADDR1, 0x0010);
//    __I2C_WriteWord(mAddress, TI_UCD9080_WDATA1, TI_UCD9080_MEMORY_UPDATE_CMD);
//  __I2C_WriteWord(mAddress, TI_UCD9080_WADDR1, 0x7E10);
//    __I2C_WriteWord(mAddress, TI_UCD9080_WDATA1, TI_UCD9080_MEMORY_UPDATE_CMD);
//    TI_UCD9080_FlashlockCmd(mAddress, TI_UCD9080_FlashlockLock);

    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_FLASHLOCK, 0x02, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );

    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, 0x00, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR2, 0x00, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1, 0xdc, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA2, 0xba, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );

    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR1, 0x01, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WADDR2, 0x00, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA1, 0xdc, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );
    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_WDATA2, 0xba, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );

    I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, _mAddress, TI_UCD9080_FLASHLOCK, 0x00, TI_UCD9080_I2C_TIMEOUTMS, 2);
    OSTimeDly( 3 );

    return 1;
}
