/*!
\file TI_PCA9544A.c
\brief Driver for TI PCA9544A I2C switch.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 02.05.2012
*/

#include "HAL\IC\inc\TI_PCA9544A.h"

#define  TI_PCA9544A_I2C_TIMEOUTMS  (50)

/* Pointer to peripherial access stricture and a \c define assigned to it */
//static I2C_PeriphInterfaceTypedef* tSwitchPeriphInterface = NULL;
//#define I2C_PERIPH_INTERFACE_STRUCT_PTR tSwitchPeriphInterface

/*!
\addtogroup TI_PCA9544A_Driver
\{
*/

/*!
\defgroup TI_PCA9544A_Driver_Exported_Functions
\{
*/

void TI_PCA9544A_InitChannelStateStructure(TI_PCA9544A_ChannelStateTypedef* tChannelStates){
	/*!
	\brief Fills \c TI_PCA9544A_ChannelStateTypedef structure with its default values.
	\param tChannelStates Target structure.
	*/
	tChannelStates->bCh0State=0;
	tChannelStates->bCh1State=0;
	tChannelStates->bCh2State=0;
	tChannelStates->bCh3State=0;
}

void TI_PCA9544A_InitChannelInterruptStructure( TI_PCA9544A_ChannelInterruptTypedef* tChannelInterrupts){
	/*!
	\brief Fills \c TI_PCA9544A_ChannelInterruptTypedef structure with its default values.
	\param tChannelInterrupts Target structure.
	*/
	tChannelInterrupts->bInt0State=0;
	tChannelInterrupts->bInt1State=0;
	tChannelInterrupts->bInt2State=0;
	tChannelInterrupts->bInt3State=0;
}

_BOOL TI_PCA9544A_GetInterruptStates( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface,
				u8 nAddress, TI_PCA9544A_ChannelInterruptTypedef* tChannelInterrupts){
	/*!
	\brief Gets interrupt states.
	\note The channel does not need to be active for detection of the interrupt.
	\param nAddress I2C device address (LSB="don't care").
	\param tChannelInterrupts Target structure.
	\retval успех чтения из i2c
	*/
	u8 nTmp;
	_BOOL ret ;

	assert(tI2cPeriphInterface!=NULL);
	
	ret = tI2cPeriphInterface->I2C_ReadCommand( tI2cPeriphInterface, nAddress, &nTmp, TI_PCA9544A_I2C_TIMEOUTMS, 0 );
	
	if( TI_PCA9544A_CR_IS_INT0(nTmp) ) tChannelInterrupts->bInt0State=1;
	if( TI_PCA9544A_CR_IS_INT1(nTmp) ) tChannelInterrupts->bInt1State=1;
	if( TI_PCA9544A_CR_IS_INT2(nTmp) ) tChannelInterrupts->bInt2State=1;
	if( TI_PCA9544A_CR_IS_INT3(nTmp) ) tChannelInterrupts->bInt3State=1;

	return ret ;	
}


_BOOL TI_PCA9544A_EnableChannel( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface,
						u8 nAddress, TI_PCA9544A_ChannelNumberTypedef mChannel){
	/*!
	\brief Enables certain channel.
	\note Only one channel can be enabled at a time.
	\param mChannel Channel to be enabled (see \c \ref TI_PCA9544A_ChannelNumberTypedef).
	\param nAddress I2C device address (LSB="don't care").
	\retval 1 if transaction was successful, 0 otherwise.
	*/
	u8 nTmp;

	assert(tI2cPeriphInterface!=NULL);

	nTmp = mChannel | TI_PCA9544A_CR_EN ;
	return tI2cPeriphInterface->I2C_SendCommand( tI2cPeriphInterface, nAddress, nTmp, TI_PCA9544A_I2C_TIMEOUTMS, 1);
}

/*!
\}
*/ //TI_PCA9544A_Driver_Exported_Functions

/*!
\}
*/ //I2C_SwitchDriver
