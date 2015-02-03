/*!
\file PMBus_GenericDriver.c
\brief Generic PMBus driver.
\details Common peripherial driver for all PMBus devices.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 30.05.2012
*/

#include "HAL\MCU\inc\T8_5282_i2c.h"
#include "HAL\MCU\inc\PMBus_GenericDriver.h"

#define PMB_ADDR(addr) (addr>>1)
//#define PMB_WORD(word) ((word>>8)|(word<<8))

_BOOL PMB_InitPorts(){
	I2Cinit();
	return 1;
}

_BOOL PMB_DeInitPorts(void){
	return 1;
}

_BOOL PMB_SendCommand(unsigned char mAddr, unsigned char mCmd){	
	return ( I2CwriteByte(PMB_ADDR(mAddr), mCmd)==OK ) ? 1 : 0 ;
}

_BOOL PMB_WriteByte(unsigned char mAddr, unsigned char mCmd, unsigned char nData){
	return ( I2CsendByte(PMB_ADDR(mAddr), mCmd, nData)==OK ) ? 1 : 0 ;
}
_BOOL PMB_WriteWord(unsigned char mAddr, unsigned char mCmd, unsigned short int nData){
	return ( I2CWriteShortLH(PMB_ADDR(mAddr), mCmd, nData)==OK ) ? 1 : 0 ;
}

_BOOL PMB_ReadByte(unsigned char mAddr, unsigned char mCmd, unsigned char *pbResult )
{
	unsigned char nData=0;
	_BOOL ret ;	
	ret = (I2C_receiveByte(PMB_ADDR(mAddr), mCmd, &nData) == OK) ? 1 : 0 ;
	if( pbResult ){
		*pbResult = nData ;
		return ret ;
	} else {
		return FALSE ;
	}
}

_BOOL PMB_ReadWord(unsigned char mAddr, unsigned char mCmd, unsigned short int *pwResult )
{
	unsigned short int nData=0;
	_BOOL ret ;
	ret = (I2CReceiveShortLH(PMB_ADDR(mAddr), mCmd, &nData, NORMAL_DATA_INTEGRITY) == OK) ? 1 : 0 ;
	if( pwResult ){
		*pwResult = nData ;
		return ret ;
	} else {
		return FALSE ;
	}
}

_BOOL PMB_ReadMultipleWords(unsigned char mAddr, unsigned char mCmd, unsigned short int* anData, unsigned char nWordsQuantity){
	return ( I2CReceiveShortLHArray(PMB_ADDR(mAddr), mCmd, anData, nWordsQuantity, NORMAL_DATA_INTEGRITY)==OK ) ? 1 : 0 ;
}

_BOOL PMB_ReadMultipleBytes(unsigned char mAddr, unsigned char mCmd, unsigned char* anData, unsigned char nBytesQuantity){
	return ( I2CReceiveByteArray(PMB_ADDR(mAddr), mCmd, anData, nBytesQuantity, NORMAL_DATA_INTEGRITY)==OK ) ? 1 : 0 ;	
}

_BOOL PMB_WriteMultipleBytes(unsigned char mAddr, unsigned char mCmd, unsigned char* anData, unsigned char nBytesQuantity){	
	return ( I2CWriteByteArray(PMB_ADDR(mAddr), mCmd, anData, nBytesQuantity)==OK ) ? 1 : 0 ;
}

_BOOL PMB_WriteMultipleWords(unsigned char mAddr, unsigned char mCmd, unsigned short int* anData, unsigned char nWordsQuantity){	
	return ( I2CWriteShortLHArray(PMB_ADDR(mAddr), mCmd, anData, nWordsQuantity)==OK ) ? 1 : 0 ;
}
