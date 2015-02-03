//--------------------------------------------------------------------------
//-| FILENAME: cfm.c
//-|
//-| Created on: 17.06.2010
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#include "HAL/MCU/inc/T8_5282_cfm.h"
#include "ErrorManagement/assert.h"
#include "ErrorManagement/status_codes.h"

#include "T8_atomic_heap.h"
#include "common_lib/memory.h"
#include <intrinsics.h>
#include "io5282.h"
#include <string.h>
#include <stdlib.h> 

u32 mcuFlashPageErase(void*);
u32 mcuFlashWrite(void*,void*, u32);

u32 cfm_flash_page_write(void* base, void* data) {
	assert(mcuFlashPageErase(base) == OK);
	assert(mcuFlashWrite(base,data,2048) == OK);
	return 0 ;
}

u32 CFM_WriteData(void* addr, void* data, u32 len) {
	u32 flag=ERROR;
	u32* base;
	u32 offset,  temp;
	u8* byte2048;
	base = (u32*)((u32)addr & 0xFFFFF800);
	offset = (u32)addr & 0x7FF;
	if (offset + len > 0x800) {
		if ((byte2048 = (u8*)npalloc(2048))) {
			t8_memcopy(byte2048,(u8*)base,2048);
			t8_memcopy(byte2048+offset,data,temp = 0x800-offset);
			cfm_flash_page_write(base,byte2048);
			base+=0x200;
			t8_memcopy(byte2048,(u8*)base,2048);
			t8_memcopy(byte2048,INCVOID(data,temp),len-temp);
			cfm_flash_page_write(base,byte2048);
			npfree(byte2048);
			flag = OK;
		}
	}
	else {
		if ((byte2048 = (u8*)npalloc(2048))) {
			t8_memcopy(byte2048,(u8*)base,2048);
			t8_memcopy(byte2048+offset,data,len);
			cfm_flash_page_write(base,byte2048);
			npfree(byte2048);
			flag = OK;
		}
	}
	return flag;
}


u32 CFM_WriteData2048(void* addr, void* data) {
	u32 flag=ERROR;
	if (!((u32)addr & 0x7FF)) {
		cfm_flash_page_write(addr,data);
		flag = OK;
	}
	return flag;
}

/********************************************************************/

//TODO: Write protect from while (1)
u32 mcu_flash_longword_write(u32* addr, u32 data) {
	while (!(MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_CBEIF));
#if (SYSTEM_CLOCK_KHZ <= 12800)
	MCF_CFM_CFMCLKD = SYSTEM_CLOCK_KHZ/(2*200);
#if (SYSTEM_CLOCK_KHZ/(2*(SYSTEM_CLOCK_KHZ/(2*200)+1)) >= 200)
	#error Wrong Flash Module frequency
#endif
#if (SYSTEM_CLOCK_KHZ/(2*(SYSTEM_CLOCK_KHZ/(2*200)+1)) <= 150)
	#error Wrong Flash Module frequency
#endif
#else
	MCF_CFM_CFMCLKD = 0x40 | SYSTEM_CLOCK_KHZ/(2*200*8);
#if (SYSTEM_CLOCK_KHZ/(16*(SYSTEM_CLOCK_KHZ/(2*200*8)+1)) >= 200)
	#error Wrong Flash Module frequency
#endif
#if (SYSTEM_CLOCK_KHZ/(16*(SYSTEM_CLOCK_KHZ/(2*200*8)+1)) <= 150)
	#error Wrong Flash Module frequency
#endif
#endif

	*addr = data;
	MCF_CFM_CFMCMD = MCF_CFM_CFMCMD_WORD_PROGRAM;
	MCF_CFM_CFMUSTAT = MCF_CFM_CFMUSTAT_CBEIF;
	while (!(MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_CCIF));
	if (MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_PVIOL) {
		MCF_CFM_CFMUSTAT = MCF_CFM_CFMUSTAT_PVIOL;
		return ERROR;
	}
	if (MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_ACCERR) {
			MCF_CFM_CFMUSTAT = MCF_CFM_CFMUSTAT_ACCERR;
			return ERROR;
	}
	if (!(MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_CBEIF)) {
		return ERROR;
	}
	return 0;
}

/********************************************************************/
//TODO: Write protect from while (1)
u32 mcu_flash_page_erase(u32* start_addr) {
	while (!(MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_CBEIF));
#if (SYSTEM_CLOCK_KHZ <= 12800)
	MCF_CFM_CFMCLKD = SYSTEM_CLOCK_KHZ/(2*200);
#if (SYSTEM_CLOCK_KHZ/(2*(SYSTEM_CLOCK_KHZ/(2*200)+1)) >= 200)
	#error Wrong Flash Module frequency
#endif
#if (SYSTEM_CLOCK_KHZ/(2*(SYSTEM_CLOCK_KHZ/(2*200)+1)) <= 150)
	#error Wrong Flash Module frequency
#endif
#else
	MCF_CFM_CFMCLKD = 0x40 | SYSTEM_CLOCK_KHZ/(2*200*8);
#if (SYSTEM_CLOCK_KHZ/(16*(SYSTEM_CLOCK_KHZ/(2*200*8)+1)) >= 200)
	#error Wrong Flash Module frequency
#endif
#if (SYSTEM_CLOCK_KHZ/(16*(SYSTEM_CLOCK_KHZ/(2*200*8)+1)) <= 150)
	#error Wrong Flash Module frequency
#endif
#endif
	*start_addr = 0x0;
	MCF_CFM_CFMCMD = MCF_CFM_CFMCMD_PAGE_ERASE;
	MCF_CFM_CFMUSTAT = MCF_CFM_CFMUSTAT_CBEIF;
	while (!(MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_CCIF));
	if (MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_PVIOL) {
		MCF_CFM_CFMUSTAT = MCF_CFM_CFMUSTAT_PVIOL;
		return ERROR;
	}
	if (MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_ACCERR) {
		MCF_CFM_CFMUSTAT = MCF_CFM_CFMUSTAT_ACCERR;
		return ERROR;
	}
	if (!(MCF_CFM_CFMUSTAT & MCF_CFM_CFMUSTAT_CBEIF)) {
		return ERROR;
	}
	return OK;
}


/********************************************************************/

u32 mcu_flash_write_ram(u32* dst, u32* src, u32 length) {
	u32 i;
	u32 (*lword_write)(u32*, u32);
	if (!(lword_write = (u32 (*)(u32*,u32))npalloc(0xFC)))
		return ERROR;
	t8_memcopy((u8*)lword_write,(u8*)mcu_flash_longword_write,0xFC);
	for (i=0;i<length;i++) {
		if (lword_write(dst++,*src++)) {
			npfree((void*)lword_write);
			return ERROR;
		}
	}
	npfree((void*)lword_write);
	return OK;
}

/********************************************************************/
//length in bytes
u32 mcuFlashWrite(void* vdst, void* vsrc, u32 length) {
	STORAGE_ATOMIC();
	
	u32 key;
	u32 (*write_func)(u32*, u32*, u32);
	u32* dst = (u32*)vdst;
	u32* src = (u32*)vsrc;
	length = (length+3)/4;
	dst = (u32*)((u32)dst | (u32)(__IPSBAR) | 0x44000000);
	if (!(write_func = (u32 (*)(u32*,u32*,u32))npalloc(0xFC)))
		return ERROR;
	t8_memcopy((u8*)write_func,(u8*)mcu_flash_write_ram,0xFC);
	START_ATOMIC() ;
	key = write_func(dst,src,length);
	STOP_ATOMIC() ;
	npfree((void*)write_func);
	return key;
}

/********************************************************************/

u32 mcuFlashPageErase(void* vaddr) {
	STORAGE_ATOMIC() ;
	
	u32 key;
	u32 (*erase_func)(u32* start_addr);
	u32* addr = (u32*)vaddr;
	if ((u32)addr & 0x7FF)
		return ERROR;
	addr = (u32*)((u32)addr | (u32)(__IPSBAR) | 0x04000000);
	if (!(erase_func = (u32 (*)(u32*))npalloc(0xFC)))
		return ERROR;
	t8_memcopy((u8*)erase_func, (u8*)mcu_flash_page_erase,0xFC);
	START_ATOMIC() ;
	key = erase_func(addr);
	STOP_ATOMIC() ;
	npfree((void*)erase_func);
	return key ;
}
