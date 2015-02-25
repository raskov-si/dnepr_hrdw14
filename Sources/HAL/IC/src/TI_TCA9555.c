/*!
\file TI_TCA9555.c
\brief Driver for TI TCA9555 I/O Expander.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 02.05.2012
*/

#include "HAL\IC\inc\TI_TCA9555.h"
#include "support_common.h"

#define TI_TCA9555_I2C_TIMOUTMS  (50)

/* Pointer to peripherial access stricture and a \c define assigned to it */
static I2C_PeriphInterfaceTypedef* tIoPeriphInterface = NULL;
#define I2C_PERIPH_INTERFACE_STRUCT_PTR tIoPeriphInterface

/*!
\addtogroup TI_TCA9555_Driver
\{
*/

/*!
\defgroup TI_TCA9555_Driver_Exported_Functions
\{
*/


void TI_TCA9555_InitPeripherialInterface(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface){
	/*!
	\brief Inits driver's peripherial interface.
	\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
	\param tI2cPeriphInterface Pointer to I2C_PeriphInterfaceTypedef structure.
	\warning This driver does not copy tI2cPeriphInterface fields, it stores only this pointer.
	\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
	*/	
	assert( tI2cPeriphInterface != NULL );
	I2C_PERIPH_INTERFACE_STRUCT_PTR = tI2cPeriphInterface;
}

void TI_TCA9555_InitPortsStructure(TI_TCA9555_PortsTypedef* tPorts){
	/*!
	\brief Fills \c TI_TCA9555_PortsTypedef structure with its default values.
	\param tPorts Target structure.
	\warning Not all default values equal zero!
	*/
	tPorts->nInputP0  = 0x00;
	tPorts->nInputP1  = 0x00;
	tPorts->nOutputP0 = 0xFF;
	tPorts->nOutputP1 = 0xFF;
	tPorts->nInvertP0 = 0x00;
	tPorts->nInvertP1 = 0x00;
	tPorts->nConfigP0 = 0xFF;
	tPorts->nConfigP1 = 0xFF;
}

_BOOL TI_TCA9555_WriteConfig(u8 nAddress, TI_TCA9555_PortsTypedef* tPorts){
	/*!
	\brief Updates I/O expander configuration (outputs, inversion and I/O configuration) with values from \c TI_TCA9555_PortsTypedef structure.
	\param nAddress I2C device address (LSB="don't care").
	\param tPorts Target structure.
	\retval 1.
	*/
	_BOOL ret = TRUE ;
	assert(I2C_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
//	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_CONFIG_P0, tPorts->nConfigP0);
//	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_INVERT_P0, tPorts->nInvertP0);
//	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_OUTPUT_P0, tPorts->nOutputP0);
//	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_CONFIG_P1, tPorts->nConfigP1);
//	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_INVERT_P1, tPorts->nInvertP1);
//	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_OUTPUT_P1, tPorts->nOutputP1);
        
	ret = ret && I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_CONFIG_P0, tPorts->nConfigP0, TI_TCA9555_I2C_TIMOUTMS, 2);
	ret = ret && I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_INVERT_P0, tPorts->nInvertP0, TI_TCA9555_I2C_TIMOUTMS, 2);
	ret = ret && I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_OUTPUT_P0, tPorts->nOutputP0, TI_TCA9555_I2C_TIMOUTMS, 2);
	ret = ret && I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_CONFIG_P1, tPorts->nConfigP1, TI_TCA9555_I2C_TIMOUTMS, 2);
	ret = ret && I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_INVERT_P1, tPorts->nInvertP1, TI_TCA9555_I2C_TIMOUTMS, 2);
	ret = ret && I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_WriteByte( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_OUTPUT_P1, tPorts->nOutputP1, TI_TCA9555_I2C_TIMOUTMS, 2);
	
	return ret;
}

_BOOL TI_TCA9555_ReadInputs(u8 nAddress, TI_TCA9555_PortsTypedef* tPorts){
	/*!
	\brief Reads input ports status to \c TI_TCA9555_PortsTypedef structure.
	\details Configuration fields remain unchanged.
	\note The Input Port registers reflect the incoming logic levels of the pins, regardless of whether the pin is defined as an input or an output by the Configuration register.
	\param nAddress I2C device address (LSB="don't care").
	\param tPorts Target structure.
	\retval 1.
	*/
	u16 nTmp;
	_BOOL ret ;
	
	assert(I2C_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	//Data registers could be (and are) read in couples. See datasheet. This saves up some bus performance.
	ret = I2C_PERIPH_INTERFACE_STRUCT_PTR->I2C_ReadWord( I2C_PERIPH_INTERFACE_STRUCT_PTR, nAddress, TI_TCA9555_INPUT_P0,
																&nTmp, TI_TCA9555_I2C_TIMOUTMS, 2);
	tPorts->nInputP0=_MSB(nTmp);
	tPorts->nInputP1=_LSB(nTmp);
	return ret;
}

/*!
\}
*/ //TI_TCA9555_Driver_Exported_Functions

/*!
\}
*/ //TI_TCA9555_Driver
