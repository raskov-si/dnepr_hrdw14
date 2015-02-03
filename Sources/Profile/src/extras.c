/*
 * extras.c
 *
 *  Created on: 26.12.2011
 *      Author: urusov
 */
#include <stdio.h>

#include "Profile/inc/extras.h"

void t8_ultoa(unsigned int val,char * str, const size_t str_len, int radix )
{
	switch(radix){
	default:
	case 10:
		snprintf( str, str_len, "%u", val );
		break;
	case 16:
		snprintf( str, str_len, "%0X", val );
		break;
	}
}
void t8_itoa( int val,char * str, const size_t str_len, int radix )
{
	switch(radix){
	default:
	case 10:
		snprintf( str, str_len, "%d", val );
		break;
	case 16:
		snprintf( str, str_len, "%0X", val );
		break;
	}
}
