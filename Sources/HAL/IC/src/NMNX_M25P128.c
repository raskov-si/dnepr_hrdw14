/*!
\file NMNX_M25P128.c
\brief Driver for Numonix (Micron) M25P128 SPI NAND flash memory.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 05.06.2012
*/

#include "HAL\IC\inc\NMNX_M25P128.h"
#include "HAL\IC\inc\SPI_Interface.h"
#include <string.h> /* memmove(), etc. */

/* Pointer to peripherial access stricture and a \c define assigned to it */
static SPI_PeriphInterfaceTypedef* tSpiFlashInterfaceStructure = NULL;
#define SPI_PERIPH_INTERFACE_STRUCTURE tSpiFlashInterfaceStructure

#define __SPI_GenericByteTransaction ( *SPI_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction )

/*!
\addtogroup NMNX_M25P128_Driver
\{
*/

/*!
\defgroup NMNX_M25P128_Driver_Exported_Functions
\{
*/

void NMNX_M25P128_InitPeripherialInterface(SPI_PeriphInterfaceTypedef* tSpiPeriphInterface){
	/*!
	\brief Inits driver's peripherial interface.
	\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
	\param tSpiPeriphInterface Pointer to SPI_FpgaPeriphInterfaceTypedef structure.
	\warning This driver does not copy tSpiPeriphInterface fields, it stores only this pointer.
	\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
	*/	
	assert( tSpiPeriphInterface != NULL );
	SPI_PERIPH_INTERFACE_STRUCTURE = tSpiPeriphInterface;
	assert(SPI_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction!=NULL);
}

_BOOL NMNX_M25P128_ReadMfrId( NMNX_M25P128_IdentificationTypedef* tMfrIdStructure ){
	/*!
	\brief Gets general manufacturer and device data.
	\param tMfrIdStructure Target structure. See \ref NMNX_M25P128_IdentificationTypedef for field assignments.
	\retval 1.
	*/
	u8 anSendBuffer[4]={0}, anReceiveBuffer[4]={0};
	
	anSendBuffer[0]=NMNX_M25P128_RDID;	
	__SPI_GenericByteTransaction( anSendBuffer, anReceiveBuffer, 4 );
	
    /* TODO: Check actual byte order! */
	tMfrIdStructure->nManufacturerId = anReceiveBuffer[1];
	tMfrIdStructure->nMemoryCapacity = anReceiveBuffer[3];
	tMfrIdStructure->nMemoryType     = anReceiveBuffer[2];
	return 1;
}


u8 NMNX_M25P128_ReadStatusRegister(){
	/*!
	\brief Reads Status register.
	\retval Status register value.
	 */
	u8 anSendBuffer[2]={0}, anReceiveBuffer[2]={0};
	
	anSendBuffer[0]=NMNX_M25P128_RDSR;
	__SPI_GenericByteTransaction( anSendBuffer, anReceiveBuffer, 2);
	return anReceiveBuffer[1];
	
}

_BOOL NMNX_M25P128_WaitForWriteCompletion(){
	/*
	\brief Polls Status register \c WIP bit, stops when it is zero.
	\details Useful for PageProgram, Sector Erase and Bulk Erase operations.
	\retval 1.
	*/
	while( NMNX_M25P128_GET_WIP(NMNX_M25P128_ReadStatusRegister()) );
	return 1;
}

_BOOL NMNX_M25P128_WaitForWriteEnable(){
	/*
	\brief Polls Status register \c WEL bit, stops when it is "1".
	\details Useful for PageProgram, Sector Erase and Bulk Erase operations.
	\retval 1.
	*/
	while( !( NMNX_M25P128_GET_WEL(NMNX_M25P128_ReadStatusRegister()) ) );
	return 1;
}

u8 NMNX_M25P128_WriteStatusRegister( u8 nStatusRegisterValue){
	/*!
	\brief Writes Status register.
	\details Use this procedure to change the values of the Block Protect (BP2, BP1, BP0) bits. Other bits are not effected.
	\param nStatusRegisterValue Status Register value to write.
	\retval 1 if the transaction was successful, 0 otherwise.
	\note Write Status Register execution process may take up to 16 milliseconds. It's a good idea to use call \c NMNX_M25P128_WaitForWriteCompletion() after each Write Status Register procedure.
	
	 */
	
	u8 anSendBuffer[2]={0};
	
	/* Write enable */
	anSendBuffer[0]=NMNX_M25P128_WREN;
	__SPI_GenericByteTransaction( anSendBuffer, NULL, 1 );
	
	anSendBuffer[0]=NMNX_M25P128_RDSR;
	anSendBuffer[1]=nStatusRegisterValue;
	
	return __SPI_GenericByteTransaction( anSendBuffer, NULL, 2 );
}

_BOOL NMNX_M25P128_ReadBytes( u32 nStartingAddress, u8* anRecievedBytes, u16 nBytesNumber){
	/*!
	\brief Reads \p nBytesNumber bytes to a \p anRecievedBytes array starting from \p nStartingAddress address.
	\param nStartingAddress Starting address pointer in memory.
	\param anRecievedBytes Buffer for received bytes storage.
	\param nBytesNumber Number of bytes to read.
	\todo Implement and debug arbitrary byte number routine.
	\retval 1 if the transaction was successful, 0 otherwise.
	*/
	// FIXME: мы здесь килобайт стэка выкидываем -- правим драйвер spi
	u8 anBytesToSend[NMNX_M25P128_PAGE_SIZE+4]={0};
	u8 anBytesToGet [NMNX_M25P128_PAGE_SIZE+4]={0};
	_BOOL bTransactionResult=0;
	
	anBytesToSend[0]=NMNX_M25P128_READ;
	anBytesToSend[1]=_BYTE(nStartingAddress, 2);
	anBytesToSend[2]=_BYTE(nStartingAddress, 1);
	anBytesToSend[3]=_BYTE(nStartingAddress, 0);
	
	bTransactionResult = __SPI_GenericByteTransaction( anBytesToSend, anBytesToGet, nBytesNumber+4 );

	/* Erase first 4 junk bytes */
	memmove(anRecievedBytes, anBytesToGet+4, nBytesNumber);
	return bTransactionResult;
}


_BOOL NMNX_M25P128_PageProgram( u32 nStartingAddress, u8* anBytesToWrite, u16 nBytesNumber){
	/*!
	\brief Writes \p nBytesNumber bytes from an \p anRecievedBytes array starting from \p nStartingAddress address.
	\param nStartingAddress Starting address pointer in memory.
	\param anBytesToWrite Buffer with bytes to store.
	\param nBytesNumber Number of bytes to write.
	\retval 1.
	*/

	u8 nBytesToSendTmp[NMNX_M25P128_PAGE_SIZE+4]={0};
	u16 nActualByteNumber;

	/* Write enable */
	nBytesToSendTmp[0]=NMNX_M25P128_WREN;
	__SPI_GenericByteTransaction( nBytesToSendTmp, NULL, 1);
	NMNX_M25P128_WaitForWriteEnable();
	
	/* Command and address */
	nBytesToSendTmp[0]=NMNX_M25P128_PP; /* Page Program */
	nBytesToSendTmp[1]=_BYTE(nStartingAddress, 2);
	nBytesToSendTmp[2]=_BYTE(nStartingAddress, 1);
	nBytesToSendTmp[3]=_BYTE(nStartingAddress, 0);
	
	/* This function writes only first 256 bytes of anBytesToWrite or less.
	 * However, M25P128 is able to overlap memory if more than 156 bytes are sent to page. */
	nActualByteNumber = ( nBytesNumber > NMNX_M25P128_PAGE_SIZE ) ? NMNX_M25P128_PAGE_SIZE : nBytesNumber ;
	
	/* Append data to command and address array */
	memmove(nBytesToSendTmp+4, anBytesToWrite, nActualByteNumber);
	
	/* Write bytes to flash */
	__SPI_GenericByteTransaction( nBytesToSendTmp, NULL, nActualByteNumber+4 );
	
	/* Insert something to wait for write completion... or wait outside this function */
	
	/* Write disable */
	/* Not really necessary, as it disables itself */
	/*
	nBytesToSendTmp[0]=NMNX_M25P128_WRDI;
	__SPI_GenericByteTransaction( nBytesToSendTmp, NULL, 1);
	*/
	return 1;
}


_BOOL NMNX_M25P128_SectorErase( u32 nAddress){
	/*!
	\brief Sets all bits to 1 (\c FFh) in any chosen sector.
	\param nAddress Address of any byte inside a chosen sector.
	\note Sector Erase execution process may take up to 3 seconds. It's a good idea to use call \c NMNX_M25P128_WaitForWriteCompletion() after each Sector Erase procedure.
	\retval 1 if the transaction was successful, 0 otherwise.
	*/
	u8 nBytesToSendTmp[4]={0};
	
	/* Write enable */
	nBytesToSendTmp[0]=NMNX_M25P128_WREN;
	__SPI_GenericByteTransaction( nBytesToSendTmp, NULL, 1 );
	NMNX_M25P128_WaitForWriteEnable();
	
	/* Command and address */
	nBytesToSendTmp[0]=NMNX_M25P128_SE; /* Sector Erase */
	nBytesToSendTmp[1]=_BYTE(nAddress, 2);
	nBytesToSendTmp[2]=_BYTE(nAddress, 1);
	nBytesToSendTmp[3]=_BYTE(nAddress, 0);

	/* Write command and address */
	return __SPI_GenericByteTransaction( nBytesToSendTmp, NULL, 4 );
}

_BOOL NMNX_M25P128_BulkErase(){
	/*!
	\brief Sets all bits to 1 (\c FFh) in any chosen sector.
	\retval 1 if the transaction was successful, 0 otherwise.
	\warning The Bulk Erase (BE) instruction is executed only if all Block Protect (BP2, BP1, BP0) bits in Status Register are 0. The Bulk Erase (BE) instruction is ignored if one, or more, sectors are protected.
	\note Bulk Erase execution process may take up to 250 (!) seconds. It's a good idea to use call \c NMNX_M25P128_WaitForWriteCompletion() after each Bulk Erase procedure.
	*/
	u8 nByteToSendTmp;
	
	/* Write enable */
	nByteToSendTmp=NMNX_M25P128_WREN;
	__SPI_GenericByteTransaction( &nByteToSendTmp, NULL, 1);
	
	/* Command and address */
	nByteToSendTmp=NMNX_M25P128_BE; /* Bulk Erase */

	/* Write command and address */
	return __SPI_GenericByteTransaction( &nByteToSendTmp, NULL, 1 );
}

_BOOL NMNX_M25P128_FastRead( u32 nStartingAddress, u8* anRecievedBytes, u16 nBytesNumber){
	/*!
	\brief Reads \p nBytesNumber bytes at Higher Speed to a \p anRecievedBytes array starting from \p nStartingAddress address.
	\param nStartingAddress Starting address pointer in memory.
	\param anRecievedBytes Buffer for received bytes storage.
	\param nBytesNumber Number of bytes to read.
	\retval 1 if the transaction was successful, 0 otherwise.
	\todo Figure out the difference between \c READ and \c FAST_READ procedures.
	*/
	u8 anBytesToSend[NMNX_M25P128_PAGE_SIZE+5]={0};
	u8 anBytesToGet [NMNX_M25P128_PAGE_SIZE+5]={0};
	_BOOL bTransactionResult=0;
	
	/* Command, address and a dummy byte */
	anBytesToSend[0]=NMNX_M25P128_READ;
	anBytesToSend[1]=_BYTE(nStartingAddress, 2);
	anBytesToSend[2]=_BYTE(nStartingAddress, 1);
	anBytesToSend[3]=_BYTE(nStartingAddress, 0);
	anBytesToSend[4]=0x00; /* Dummy byte */
	
	/* Read data */
	bTransactionResult = __SPI_GenericByteTransaction( anBytesToSend, anBytesToGet, nBytesNumber+5);

	/* Erase first 5 junk bytes */
	memmove(anRecievedBytes, anBytesToGet+5, nBytesNumber);
	
	return bTransactionResult;
}

/*!
\}
*/ //NMNX_M25P128_Driver_Exported_Functions

/*!
\}
*/ //NMNX_M25P128_Driver

