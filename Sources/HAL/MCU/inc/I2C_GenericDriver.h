/*!
\file I2C_GenericDriver.h
\brief Header for I2C driver.
\details Common peripherial driver for all I2C devices.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 30.05.2012
*/

#ifndef __I2C_GENERIC_DRIVER_H
#define __I2C_GENERIC_DRIVER_H
//#include "PMBus_Commands.h"
#include "support_common.h"

/*!
\addtogroup Peripherial_Driver
\{
*/

/*!
\defgroup I2C_Hardware_Level_Functions
\{
*/


/*!
\brief Read a single byte via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\retval Received byte.
*/
_BOOL I2C_ReadCommand(unsigned char mAddr, unsigned char *pbResult);

/*!
\brief Read a single word (16 bits) via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param pwResult Pointer to store the result
\retval 1 if transaction was successful, 0 otherwise.
\warning Data byte high is sent first.
*/
_BOOL I2C_ReadWord(unsigned char mAddr, unsigned char mCmd, unsigned short int *pwResult);

/*!
\brief Send a single byte via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param nData Data byte to be sent.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL I2C_WriteByte(u8 mAddr, u8 mCmd, u8 nData);

/*!
\brief Send a single word (16 bits) via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param nData Data word to be sent.
\retval 1 if transaction was successful, 0 otherwise.
\warning Data byte high is sent first.
*/
_BOOL I2C_WriteWord(u8 mAddr, u8 mCmd, u16 nData);

/*!
\brief Read a single byte via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param pbResult Pointer to byte for result
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL I2C_ReadByte(unsigned char mAddr, unsigned char mCmd, unsigned char *pbResult);

/*!
\brief Send a single command via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL I2C_SendCommand(u8 mAddr, u8 mCmd);


/*!
\brief Reads a single byte via I2C (with no register access command). 
\param mAddr I2C device address. 
\param pbResult pointer to store the result
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL I2C_ReadCommand(unsigned char mAddr, unsigned char *pbResult);

/*!
\brief Read multiple bytes via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param anData Pointer to a data bytes array.
\param nBytesQuantity Number of bytes to be read.
\retval 1 if transaction was successful, 0 otherwise.
*/


_BOOL I2C_ReadMultipleBytes(u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity);

/*!
\brief Send multiple bytes via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param anData Pointer to a data bytes array.
\param nBytesQuantity Number of bytes to be sent from an array.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL I2C_WriteMultipleBytes(u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity);

/*!
\brief Read multiple bytes via I2C with 16 bit command length. 
\param mAddr I2C device address.
\param mCmd I2C command. 
\param anData Pointer to a data bytes array.
\param nBytesQuantity Number of bytes to be read.
\retval 1 if transaction was successful, 0 otherwise.
*/


_BOOL I2C_ReadMultipleBytes_16(u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity);

/*!
\brief Send multiple bytes via I2C with 16 bit command length. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param anData Pointer to a data bytes array.
\param nBytesQuantity Number of bytes to be sent from an array.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL I2C_WriteMultipleBytes_16(u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity);

/*!
\brief Read multiple words (16 bits) via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param anData Pointer to a data words array.
\param nBytesQuantity Number of words to be read.
\retval 1 if transaction was successful, 0 otherwise.
\warning Data byte high is sent first.
*/
_BOOL I2C_ReadMultipleWords(u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity);

/*!
\brief Send multiple words via I2C. 
\param mAddr I2C device address. 
\param mCmd I2C command. 
\param anData Pointer to a data words array.
\param nBytesQuantity Number of words to be sent from an array.
\retval 1 if transaction was successful, 0 otherwise.
*/
_BOOL I2C_WriteMultipleWords(u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity);

/*!
\brief Sets output pin connected to fan controller \em Control pin.
\param bState New state of Control pin: 1 for high, 0 for low.
\retval 1.
*/


/*!
\brief Inits I2C controller.
\retval 1.
*/
_BOOL I2C_InitPorts(void);
/*!
\brief De-inits I2C controller.
\retval 1.
*/
_BOOL I2C_DeInitPorts(void);

/*!
\}
*/ //I2C_Hardware_Level_Functions

/*!
\}
*/ //Peripherial_Driver

#endif //__I2C_GENERIC_DRIVER_H
