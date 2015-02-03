/*!
\file AD_AD7921.c
\brief Драйвер AD7921
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date nov 2013
*/

#include "HAL/IC/inc/AD_AD7921.h"

static SPI_PeriphInterfaceTypedef* tSpiFlashInterfaceStructure = NULL ;

void AD_AD7921_Init( SPI_PeriphInterfaceTypedef* tSpiPeriphInterface )
{
	assert( (tSpiFlashInterfaceStructure = tSpiPeriphInterface) != NULL );
}

void AD_AD7921_ReadBoth( u16* first_ch, u16* second_ch )
{
	u16 tx ;
	assert( tSpiFlashInterfaceStructure );

	// измеряем 0й канал
	tx = 0x0000 ;
	tSpiFlashInterfaceStructure->SPI_GenericByteTransaction( (u8*)&tx, 2, NULL, 0 );
	tx = 0xFFFF ;
	tSpiFlashInterfaceStructure->SPI_GenericByteTransaction( (u8*)&tx, 2, (u8*)first_ch, 2 );
	*first_ch &= 0x0FFF ;
	tSpiFlashInterfaceStructure->SPI_GenericByteTransaction( (u8*)&tx, 2, (u8*)second_ch, 2 );
	*second_ch &= 0x0FFF ;
}
