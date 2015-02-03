/*!
\file medfilt.c
\brief Median filter realisation 
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date sep 2013
*/

#include "support_common.h"
#include "common_lib/medfilt.h"

#define _element( obj, ind, type ) *(type*)&(obj->elements[ ind ])

f32 t8_medfilt_f32( t8_medfilt_t *filt, f32 new_elem )
{
	f32 middle;
	f32 a, b, c ;

	// добавляем элемент в окно
	_element( filt, filt->last_elem++, f32 ) = new_elem ;
	if( filt->last_elem >= MEDFILT_LEN ){
		filt->last_elem = 0 ;
	}
	// если ещё не заполнили окно
	if( filt->filled < MEDFILT_LEN ){
		++filt->filled ;
		return new_elem ;
	}

	a = _element( filt, 0, f32 ) ;
	b = _element( filt, 1, f32 ) ;
	c = _element( filt, 2, f32 ) ;

	if ((a <= b) && (a <= c)){
		middle = (b <= c) ? b : c;
	} else if ((b <= a) && (b <= c)) {
		middle = (a <= c) ? a : c;
	} else {
		middle = (a <= b) ? a : b;
	}
	return middle;
}

u32 t8_medfilt_u32( t8_medfilt_t *filt, u32 new_elem )
{
	u32 middle;
	u32 a, b, c ;

	// добавляем элемент в окно
	_element( filt, filt->last_elem++, u32 ) = new_elem ;
	if( filt->last_elem >= MEDFILT_LEN ){
		filt->last_elem = 0 ;
	}
	// если ещё не заполнили окно
	if( filt->filled < MEDFILT_LEN ){
		++filt->filled ;
		return new_elem ;
	}

	a = _element( filt, 0, u32 ) ;
	b = _element( filt, 1, u32 ) ;
	c = _element( filt, 2, u32 ) ;

	if ((a <= b) && (a <= c)){
		middle = (b <= c) ? b : c;
	} else if ((b <= a) && (b <= c)) {
		middle = (a <= c) ? a : c;
	} else {
		middle = (a <= b) ? a : b;
	}
	return middle;
}
