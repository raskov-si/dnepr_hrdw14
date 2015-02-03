/*!
\file T8_Dnepr_SDRAM.h
\brief Код инициализации внешней ОЗУ
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#ifndef __DNEPR_SDRAM_H_
#define __DNEPR_SDRAM_H_

#include "support_common.h"

void Dnepr_SDRAM_init( u32 sdram_address, u32 system_clock_khz );
_BOOL Dnepr_SDRAM_Check( size_t sdram_address, size_t sdram_len );
_BOOL Dnepr_SDRAM_GetStatus() ;

#endif
