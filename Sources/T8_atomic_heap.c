#include "T8_atomic_heap.h"

void* npalloc(size_t size_){
	STORAGE_ATOMIC() ;
	
	void *p = NULL ;
	START_ATOMIC() ;
	p = calloc(size_,1); 
	STOP_ATOMIC() ;
	return p;
}
void npfree(void * ptr){
	STORAGE_ATOMIC() ;
	
	//mcu_irq_disable();
	START_ATOMIC() ;
	free(ptr);
	//mcu_irq_restore();
	STOP_ATOMIC() ;
}
