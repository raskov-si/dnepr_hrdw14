/*!
\file memory.h
\brief Fast procedures for memory copying and memory set (brought from startf.c)  
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date may 2012
*/

#ifndef MEMORY_H_
#define MEMORY_H_

#include "support_common.h"

s32 strnlen( const s8* str, size_t len );
void t8_memzero( u8 *dest_, size_t sz_ ) ;
void t8_memcopy( u8 *dest_, u8 *src_, size_t sz_ );

#endif /* MEMORY_H_ */
