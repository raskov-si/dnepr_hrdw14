/*!
\file T8_atomic_heap.h
\brief Inline procedures for reentrant heap tools. 
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date June 2012
*/

#ifndef 	ATOMIC_HEAP_H__
#define		ATOMIC_HEAP_H__

#include "T8_Atomiccode.h"
#include <stdlib.h>

void* npalloc(size_t size_);
void npfree(void * ptr);

#endif // ATOMIC_HEAP_H__
