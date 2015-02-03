/*!
\file T8_Dnepr_BP_CurrentMeasure.h
\brief Измеряет ток в двух каналах.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#ifndef T8_DNEPR_BP_CURRENTMEASURE_H__
#define T8_DNEPR_BP_CURRENTMEASURE_H__

#include "HAL\IC\inc\SPI_Interface.h"

void Dnepr_BP_Current_Init( SPI_PeriphInterfaceTypedef* tSpiPeriphInterface );
void Dnepr_BP_Current_Measure( f32* first_ch_amp, const f32 k1, const f32 b1,
										f32* second_ch_amp, const f32 k2, const f32 b2 );

#endif
