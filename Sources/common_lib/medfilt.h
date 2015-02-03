/*!
\file medfilt.h
\brief Median filter realisation 
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date sep 2013
*/

#ifndef T8_MEDFILT_H__
#define T8_MEDFILT_H__

#include "support_common.h"

#define MEDFILT_LEN 3
typedef struct __t8_medfilt_t {
	u32 elements[ MEDFILT_LEN ] ;
	u8 last_elem ;
	u8 filled ;
} t8_medfilt_t ;

#define MEDFILT_CREATE( name ) t8_medfilt_t name = { {0,0,0}, 0, 0 };

f32 t8_medfilt_f32( t8_medfilt_t *filt, f32 new_elem );
u32 t8_medfilt_u32( t8_medfilt_t *filt, u32 new_elem );

#endif
