/*!
\file PMBus_GenericDriver.h
\brief Header for PMBus driver.
\details Common peripherial driver for all PMBus devices.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 30.05.2012
*/

#ifndef __PMBUS_GENERIC_DRIVER_H
#define __PMBUS_GENERIC_DRIVER_H
//#include "PMBus_Commands.h"
#include "support_common.h"

/*!
\addtogroup Peripherial_Driver
\{
*/

/*!
\defgroup PMBus_Hardware_Level_Functions
\{
*/

/*!
\brief Read a single byte via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param pbResult pointer to store the result
\retval Received byte.
*/
_BOOL PMB_ReadByte(unsigned char mAddr, unsigned char mCmd, unsigned char *pbResult );

/*!
\brief Read a single word (16 bits) via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param pwResult pointer to store the result
\retval Received word.
\warning Data byte low is sent first.
*/
_BOOL PMB_ReadWord(unsigned char mAddr, unsigned char mCmd, unsigned short int *pwResult );

/*!
\brief Send a single byte via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param nData Data byte to be sent.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL PMB_WriteByte(u8 mAddr, u8 mCmd, u8 nData);

/*!
\brief Send a single word (16 bits) via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param nData Data word to be sent.
\retval 1 if transaction was successful, 0 otherwise.
\warning Data byte low is sent first.
*/
_BOOL PMB_WriteWord(u8 mAddr, u8 mCmd, u16 nData);

/*!
\brief Send a single command via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL PMB_SendCommand(u8 mAddr, u8 mCmd);

/*!
\brief Read multiple bytes via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param anData Pointer to a data bytes array.
\param nBytesQuantity Number of bytes to be read.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL PMB_ReadMultipleBytes(u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity);

/*!
\brief Send multiple bytes via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param anData Pointer to a data bytes array.
\param nBytesQuantity Number of bytes to be sent from an array.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL PMB_WriteMultipleBytes(u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity);

/*!
\brief Read multiple words (16 bits) via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param anData Pointer to a data words array.
\param nBytesQuantity Number of words to be read.
\retval 1 if transaction was successful, 0 otherwise.
\warning Data byte low is sent first.
*/
_BOOL PMB_ReadMultipleWords(u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity);

/*!
\brief Send multiple words via PMBus. 
\param mAddr PMBus device address. 
\param mCmd PMBus command. 
\param anData Pointer to a data words array.
\param nBytesQuantity Number of words to be sent from an array.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL PMB_WriteMultipleWords(u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity);

/*!
\}
*/ //PMBus_Hardware_Level_Functions


/*!
\defgroup Peripherial_General_Functions
\{
*/

/*!
\brief Inits the peripherials.
\retval 1.
*/
_BOOL PMB_InitPorts();
/*!
\brief De-inits the peripherials.
\retval 1.
*/
_BOOL PMB_DeInitPorts();

/*!
\}
*/ //Peripherial_General_Functions

/*!
\}
*/ //Peripherial_Driver

#endif //__PMBUS_GENERIC_DRIVER_H
