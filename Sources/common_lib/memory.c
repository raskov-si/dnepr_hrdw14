/*!
\file memory.c
\brief Fast procedures for memory copying and memory set (idea brought from startf.c)  
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date may 2012
*/

#include "common_lib\memory.h"
#include <string.h>

s32 strnlen( const s8* str, size_t len )
{
	return (s32)memchr( ((const void*)str), 0, (len) ) - (s32)str ;
}

void t8_memzero( u8 *dest_, size_t sz_ )
{
	u32 i;
	s32 *lptr;

	if (sz_ >= 32)
	{
		/* выравниваем начальный адрес по 4 байта */
		i = (- (u32) dest_) & 3;

		if (i)
		{
			sz_ -= i;
			do
				*dest_++ = 0 ;
			while (--i);
		}

		/* разворачиваем цикл и идём по 32 бита */
		i = sz_ >> 5;
		if (i)
		{
			lptr = (s32 *)dest_ ;
			dest_ += i * 32 ;
			do
			{
				*lptr++ = 0 ;
				*lptr++ = 0 ;
				*lptr++ = 0 ;
				*lptr++ = 0 ;
				*lptr++ = 0 ;
				*lptr++ = 0 ;
				*lptr++ = 0 ;
				*lptr++ = 0 ;
			}
			while (--i);
		}
		i = (sz_ & 31) >> 2;

		/* handle any 4 byte blocks left */
		if (i)
		{
			lptr = (s32 *)dest_ ;
			dest_ += i * 4;
			do
				*lptr++ = 0;
			while (--i);
		}
		sz_ &= 3;
	}

	/* все оставшиеся блоки побайтно */
	if (sz_)
		do
			*dest_++ = 0;
		while (--sz_);
}

// скопировано из memzero с небольшими изменениями. Копипаст для скорости -- чтобы
// не делать ещё один вызов
// !!! Важно: выравнивайте dest_ и src_ по границе 4 байт
void t8_memcopy( u8 *dest_, u8 *src_, size_t sz_ )
{
	u32 i;
	s32 *lptr, *lpSrc ;

	if (sz_ >= 32)
	{
		/* выравниваем начальный адрес по 4 байта */
		i = (- (u32) dest_) & 3;

		if (i)
		{
			sz_ -= i;
			do
				*dest_++ = *src_++ ;
			while (--i);
		}

		/* разворачиваем цикл и идём по 32 бита */
		i = sz_ >> 5;
		if (i)
		{
			lptr = (s32 *)dest_ ;
			lpSrc = (s32 *)src_ ;
			dest_ += i * 32 ;
			src_ += i * 32 ;
			do
			{
				*lptr++ = *lpSrc++ ;
				*lptr++ = *lpSrc++ ;
				*lptr++ = *lpSrc++ ;
				*lptr++ = *lpSrc++ ;
				*lptr++ = *lpSrc++ ;
				*lptr++ = *lpSrc++ ;
				*lptr++ = *lpSrc++ ;
				*lptr++ = *lpSrc++ ;
			}
			while (--i);
		}
		i = (sz_ & 31) >> 2;

		/* handle any 4 byte blocks left */
		if (i)
		{
			lptr = (s32 *)dest_ ;
			lpSrc = (s32 *)src_ ;
			dest_ += i * 4;
			src_ +=  i * 4;
			do
				*lptr++ = *lpSrc++ ;
			while (--i);
		}
		sz_ &= 3;
	}

	/* все оставшиеся блоки побайтно */
	if (sz_)
		do
			*dest_++ = *src_++ ;
		while (--sz_);
}
