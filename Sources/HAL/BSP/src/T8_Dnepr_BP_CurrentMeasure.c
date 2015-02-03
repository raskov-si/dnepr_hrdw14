/*!
\file T8_Dnepr_BP_CurrentMeasure.c
\brief Измеряет ток в двух каналах.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#include "HAL/IC/inc/AD_AD7921.h"

//////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_BP_Current_Init( SPI_PeriphInterfaceTypedef* tSpiPeriphInterface )
{
	AD_AD7921_Init( tSpiPeriphInterface );
}

void Dnepr_BP_Current_Measure( f32* first_ch_amp, const f32 k1, const f32 b1,
										f32* second_ch_amp, const f32 k2, const f32 b2 )
{
	u16 u1, u2 ;

	AD_AD7921_ReadBoth( &u1, &u2 );
	*first_ch_amp = ((f32)u1 - 2048.) * 3.3 / 4096. / 0.045 * k1 + b1 ;
	*second_ch_amp = ((f32)u2 - 2048.) * 3.3 / 4096. / 0.045 * k2 + b2 ;
}
