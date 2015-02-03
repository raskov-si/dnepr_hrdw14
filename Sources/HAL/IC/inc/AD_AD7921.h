/*!
\file AD_AD7921.h
\brief Драйвер AD7921
\author <a href="mailto:baranovm@t8.ru">Баранов М. В.</a>
\date nov 2013
*/

#ifndef AD_AD7921_H__
#define AD_AD7921_H__

#include "HAL\IC\inc\SPI_Interface.h"

void AD_AD7921_Init( SPI_PeriphInterfaceTypedef* tSpiPeriphInterface );
void AD_AD7921_ReadBoth( u16* first_ch, u16* second_ch );

#endif
