/*!
\file AT_AT45DB321D.с
\brief Драйвер AT45DB321D
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date nov 2012
*/

#include "HAL/IC/inc/at45db321d.h"
#include "common_lib\memory.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define AT_AT45DB321D_STATUS_ADDR	0xD7
#define AT_AT45DB321D_RDY 			0x80

#define AT_AT45DB321D_MANUFID_ADDR	0x9F

#define AT_AT45DB321D_DIRECT_READ	0xD2

#define AT_AT45DB321D_BUFF1_WRT		0x84
#define AT_AT45DB321D_BUFF2_WRT		0x87

#define AT_AT45DB321D_BUFF1_TO_MEM_WITH_ERRASE	0x83
#define AT_AT45DB321D_BUFF2_TO_MEM_WITH_ERRASE	0x86

#define AT_AT45DB321D_BUFF1_TO_MEM_WITHOUT_ERRASE	0x88
#define AT_AT45DB321D_BUFF2_TO_MEM_WITHOUT_ERRASE	0x89

#define AT_AT45DB321D_BUFF1_READ	0xD4
#define AT_AT45DB321D_BUFF2_READ	0xD6

#define AT_AT45DB321D_BLOCK_ERASE	0x50

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Pointer to peripherial access stricture and a \c define assigned to it */
static SPI_PeriphInterfaceTypedef* tSpiFlashInterfaceStructure = NULL;
#define SPI_PERIPH_INTERFACE_STRUCTURE tSpiFlashInterfaceStructure
#define __SPI_GenericByteTransaction ( *SPI_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AT_AT45DB321D_InitPeripherialInterface(SPI_PeriphInterfaceTypedef* tSpiPeriphInterface)
{
	assert( tSpiPeriphInterface != NULL );
	SPI_PERIPH_INTERFACE_STRUCTURE = tSpiPeriphInterface;
	assert(SPI_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction!=NULL);
}

u16 AT_AT45DB321D_ReadStatus()
{
	u8 anSendBuffer[2]={0}, anReceiveBuffer[2]={0};
	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	anSendBuffer[0] = AT_AT45DB321D_STATUS_ADDR ;	
	__SPI_GenericByteTransaction( anSendBuffer, 2, anReceiveBuffer, 2 );

	return anReceiveBuffer[1] ;
}

void AT_AT45DB321D_ReadJEDEC( AT_AT45DB321D_JEDEC *pJEDEC )
{
	u8 anSendBuffer[4]={0}, anReceiveBuffer[4]={0};
	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	if( !pJEDEC )
		return;

	anSendBuffer[0] = AT_AT45DB321D_MANUFID_ADDR ;	
	__SPI_GenericByteTransaction( anSendBuffer, 4, anReceiveBuffer, 4 );

	pJEDEC->manufacturer = anReceiveBuffer[1] ;
	pJEDEC->family_code = (anReceiveBuffer[2] >> 5) & 0x03 ;
	pJEDEC->density_code = anReceiveBuffer[2] & 0x1F ;
	pJEDEC->mlc_code = (anReceiveBuffer[3] >> 5) & 0x03 ;
	pJEDEC->prod_version = anReceiveBuffer[3] & 0x1F ;
}

_BOOL AT_AT45DB321D_ReadyToWrite()
{
	return (AT_AT45DB321D_ReadStatus() & AT_AT45DB321D_RDY) > 0;	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define __AT45DB321D_BUFF_SZ	536

void AT_AT45DB321D_DirectReadBytes( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber )
{
	u8 anReceiveBuffer[ __AT45DB321D_BUFF_SZ ];
	u8 anSendBuffer[ 8 ];

	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	anSendBuffer[ 0 ] = AT_AT45DB321D_DIRECT_READ ;	
	anSendBuffer[ 1 ] = (wAddr >> 16) & 0x7F ;	
	anSendBuffer[ 2 ] = (wAddr >> 8) & 0xFF ;	
	anSendBuffer[ 3 ] = wAddr & 0xFF ;	

	anSendBuffer[ 4 ] = anSendBuffer[ 5 ] = anSendBuffer[ 6 ] = anSendBuffer[ 7 ]
		= 0 ;

	__SPI_GenericByteTransaction( anSendBuffer, 8, anReceiveBuffer,
		MIN(nBytesNumber+8, __AT45DB321D_BUFF_SZ) );

	t8_memcopy( anRecievedBytes, anReceiveBuffer + 8, nBytesNumber );
}

void AT_AT45DB321D_WriteBuffer1( const u32 wAddress, u8* anBytesToWrite, u16 nBytesNumber )
{
	u8 anSendBuffer[ __AT45DB321D_BUFF_SZ ];

	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	anSendBuffer[ 0 ] = AT_AT45DB321D_BUFF1_WRT ;	
	anSendBuffer[ 1 ] = 0 ;	
	anSendBuffer[ 2 ] = (wAddress >> 8) & 0x3 ;
	anSendBuffer[ 3 ] = wAddress & 0xFF ;

	t8_memcopy( anSendBuffer+4, anBytesToWrite,
		MIN(nBytesNumber, __AT45DB321D_BUFF_SZ-4) );	
	__SPI_GenericByteTransaction( anSendBuffer, MIN(nBytesNumber+4, __AT45DB321D_BUFF_SZ),
			NULL, 0 );
}

void AT_AT45DB321D_WriteBuffer2( const u32 wAddress, u8* anBytesToWrite, u16 nBytesNumber )
{
	u8 anSendBuffer[ __AT45DB321D_BUFF_SZ ];

	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	anSendBuffer[ 0 ] = AT_AT45DB321D_BUFF2_WRT ;
	anSendBuffer[ 1 ] = 0 ;
	anSendBuffer[ 2 ] = (wAddress >> 8) & 0x3 ;
	anSendBuffer[ 3 ] = wAddress & 0xFF ;

	t8_memcopy( anSendBuffer+4, anBytesToWrite,
		MIN(nBytesNumber, __AT45DB321D_BUFF_SZ-4) );	
	__SPI_GenericByteTransaction( anSendBuffer, MIN(nBytesNumber+4, __AT45DB321D_BUFF_SZ),
		NULL, 0 );

}

void AT_AT45DB321D_ProgrammBuffer1( const u32 wAddr )
{
	u8 anSendBuffer[4]={0} ;
	assert( SPI_PERIPH_INTERFACE_STRUCTURE );
	assert( (wAddr & 0x1FF) == 0 ); // нет смещения внутри сектора

	anSendBuffer[0] = AT_AT45DB321D_BUFF1_TO_MEM_WITHOUT_ERRASE ;	
	anSendBuffer[ 1 ] = (wAddr >> 16) & 0x7F ;
	anSendBuffer[ 2 ] = (wAddr >> 8) & 0xFF ;
	anSendBuffer[ 3 ] = wAddr & 0xFF ;	
	__SPI_GenericByteTransaction( anSendBuffer, 4, NULL, 0 );
}

void AT_AT45DB321D_ProgrammBuffer2( const u32 wAddr )
{
	u8 anSendBuffer[4]={0} ;
	assert( SPI_PERIPH_INTERFACE_STRUCTURE );
	assert( (wAddr & 0x1FF) == 0 ); // нет смещения внутри сектора

	anSendBuffer[0] = AT_AT45DB321D_BUFF2_TO_MEM_WITHOUT_ERRASE ;
	anSendBuffer[ 1 ] = (wAddr >> 16) & 0x7F ;
	anSendBuffer[ 2 ] = (wAddr >> 8) & 0xFF ;
	anSendBuffer[ 3 ] = wAddr & 0xFF ;	
	__SPI_GenericByteTransaction( anSendBuffer, 4, NULL, 0 );
}

void AT_AT45DB321D_ReadBuff1( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber )
{
	u8 anSendBuffer[ 5 ];
	u8 anReceiveBuffer[ __AT45DB321D_BUFF_SZ ];

	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	anSendBuffer[ 0 ] = AT_AT45DB321D_BUFF1_READ ;	
	anSendBuffer[ 1 ] = (wAddr >> 16) & 0x3F ;	
	anSendBuffer[ 2 ] = (wAddr >> 8) & 0xFF ;	
	anSendBuffer[ 3 ] = wAddr & 0xFF ;	

	anSendBuffer[ 4 ] = 0 ;

	__SPI_GenericByteTransaction( anSendBuffer, 5, anReceiveBuffer,
		MIN(nBytesNumber+5, __AT45DB321D_BUFF_SZ) );

	t8_memcopy( anRecievedBytes, anReceiveBuffer + 5, nBytesNumber );
}

void AT_AT45DB321D_ReadBuff2( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber )
{
	u8 anSendBuffer[ 5 ];
	u8 anReceiveBuffer[ __AT45DB321D_BUFF_SZ ];

	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	anSendBuffer[ 0 ] = AT_AT45DB321D_BUFF2_READ ;	
	anSendBuffer[ 1 ] = (wAddr >> 16) & 0x3F ;	
	anSendBuffer[ 2 ] = (wAddr >> 8) & 0xFF ;	
	anSendBuffer[ 3 ] = wAddr & 0xFF ;	

	anSendBuffer[ 4 ] = 0 ;

	__SPI_GenericByteTransaction( anSendBuffer, 5, anReceiveBuffer,
		MIN(nBytesNumber+5, __AT45DB321D_BUFF_SZ) );

	t8_memcopy( anRecievedBytes, anReceiveBuffer + 5, nBytesNumber );
}

void AT_AT45DB321D_ErraseBlock( const u32 nBlock )
{
	u8 anSendBuffer[ 5 ];

	assert( SPI_PERIPH_INTERFACE_STRUCTURE );

	anSendBuffer[ 0 ] = AT_AT45DB321D_BLOCK_ERASE ;	
	anSendBuffer[ 1 ] = (nBlock >> 3) & 0x7F ;	
	anSendBuffer[ 2 ] = (nBlock << 5) & 0xFF ;	
	anSendBuffer[ 3 ] = 0 ;	

	__SPI_GenericByteTransaction( anSendBuffer, 4, NULL, 0 );
}
