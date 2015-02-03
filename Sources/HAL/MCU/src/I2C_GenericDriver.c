/*!
\file I2C_GenericDriver.c
\brief Generic I2C driver.
\details Common peripherial driver for all I2C devices.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 30.05.2012
*/

#include "HAL\MCU\inc\T8_5282_i2c.h"
#include "HAL\MCU\inc\I2C_GenericDriver.h"

#define I2C_ADDR(addr) (addr>>1)

_BOOL I2C_InitPorts(){
	I2Cinit();
	return 1;
}

_BOOL I2C_DeInitPorts(void){
	return 1;
}

_BOOL I2C_SendCommand(unsigned char mAddr, unsigned char mCmd){	
	return (  I2CwriteByte(I2C_ADDR(mAddr), mCmd)==OK ) ? 1 : 0 ;
}

_BOOL I2C_ReadCommand(unsigned char mAddr, unsigned char *pbResult)
{
	unsigned char nData=0;
	_BOOL ret ;
	ret = (I2CreadByte(I2C_ADDR(mAddr), &nData) == OK) ? TRUE : FALSE ;
	if( pbResult ){
		*pbResult = nData ;
		return ret ;
	} else {
		return FALSE ;
	}
}

_BOOL I2C_WriteByte(unsigned char mAddr, unsigned char mCmd, unsigned char nData){
	
	return ( I2CsendByte(I2C_ADDR(mAddr), mCmd, nData)==OK ) ? 1 : 0 ;
}
_BOOL I2C_WriteWord(unsigned char mAddr, unsigned char mCmd, unsigned short int nData){
	
	return ( I2CWriteShortHL(I2C_ADDR(mAddr), mCmd, nData)==OK ) ? 1 : 0 ;
}

 _BOOL I2C_ReadByte(unsigned char mAddr, unsigned char mCmd, unsigned char *pbResult){
	unsigned char nData=0;
	_BOOL ret ;
	ret = (I2C_receiveByte(I2C_ADDR(mAddr), mCmd, &nData) == OK) ? TRUE : FALSE;
	if( pbResult ){
		*pbResult = nData ;
		return ret ;
	} else {
		return FALSE ;
	}
}

_BOOL I2C_ReadWord(unsigned char mAddr, unsigned char mCmd, unsigned short int *pwResult){
	unsigned short int nData=0;
	_BOOL ret ;
	ret = (I2CReceiveShortHL(I2C_ADDR(mAddr), mCmd, &nData, NORMAL_DATA_INTEGRITY) == OK) ? TRUE : FALSE ;
	if( pwResult ){
		*pwResult = nData ;
		return ret ;
	} else {
		return FALSE ;
	}
}

_BOOL I2C_ReadMultipleWords(unsigned char mAddr, unsigned char mCmd, unsigned short int* anData, unsigned char nWordsQuantity){
	return ( I2CReceiveShortHLArray(I2C_ADDR(mAddr), mCmd, anData, nWordsQuantity, NORMAL_DATA_INTEGRITY)==OK ) ? 1 : 0 ;
}

_BOOL I2C_WriteMultipleWords(unsigned char mAddr, unsigned char mCmd, unsigned short int* anData, unsigned char nWordsQuantity){	
	return ( I2CWriteShortHLArray(I2C_ADDR(mAddr), mCmd, anData, nWordsQuantity)==OK ) ? 1 : 0 ;
}

_BOOL I2C_ReadMultipleBytes(unsigned char mAddr, unsigned char mCmd, unsigned char* anData, unsigned char nBytesQuantity){
	return ( I2CReceiveByteArray(I2C_ADDR(mAddr), mCmd, anData, nBytesQuantity, NORMAL_DATA_INTEGRITY)==OK ) ? 1 : 0 ;	
}

_BOOL I2C_WriteMultipleBytes(unsigned char mAddr, unsigned char mCmd, unsigned char* anData, unsigned char nBytesQuantity){	
	return ( I2CWriteByteArray(I2C_ADDR(mAddr), mCmd, anData, nBytesQuantity)==OK ) ? 1 : 0 ;
}

_BOOL I2C_ReadMultipleBytes_16(u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity)
{
	return I2CReceiveByteArray_16( I2C_ADDR(mAddr), mCmd, anData, nBytesQuantity, NORMAL_DATA_INTEGRITY ) == OK ;
}

_BOOL I2C_WriteMultipleBytes_16(u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity)
{
	return I2CWriteByteArray_16( I2C_ADDR(mAddr), mCmd, anData, nBytesQuantity ) == OK ;
}
