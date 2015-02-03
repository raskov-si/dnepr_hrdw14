//--------------------------------------------------------------------------
//-| FILENAME: pr_ch.c
//-|
//-| Created on: 07.09.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------
#include <string.h>
#include "support_common.h"

#include "Profile/inc/sys.h"
#include "HAL/MCU/inc/T8_5282_cfm.h"
#include "common_lib/memory.h"

//TODO: вместо *_ATOMIC -- мьютексы
#define STRCPY(x,y)\
	START_ATOMIC();\
		strcpy(x,y);\
	STOP_ATOMIC();

#define MEMCPY(x,y,l)\
	START_ATOMIC();\
		t8_memcopy(x,y,l);\
	STOP_ATOMIC();
	

u32 PROFILE_CHARLINEValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	STORAGE_ATOMIC() ; // нужен для MEMCPY
	u32 flag=ERROR;
	s8* str;
	str = (s8*)SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
	if (str!=NULL) {
		if (strlen(str)+1 < buff_len) {
			STRCPY( buff, str );
			flag = OK;
		}
	}
	return flag;
}
u32 PROFILE_REALValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	STORAGE_ATOMIC() ; // нужен для MEMCPY
	u32 flag=ERROR;
	void* p_v;
	if (buff_len >= sizeof(f32)) {
		p_v = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
		if (p_v != NULL) {
			MEMCPY(buff, p_v, sizeof(f32));
			flag = OK;
		}
	}
	return flag;
}
u32 PROFILE_INTValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	STORAGE_ATOMIC() ;
	
	u32 flag=ERROR;
	void* p_v;
	if (buff_len >= sizeof(u32)) {
		p_v = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
		if (p_v != NULL) {
			MEMCPY(buff, p_v, sizeof(u32));
			flag = OK;
		}
	}
	return flag;
}
u32 PROFILE_BOOLValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	STORAGE_ATOMIC() ;
	u32 flag=ERROR;
	void* p_v;
	if (buff_len >= sizeof(u8)) {
		p_v = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
		if (p_v != NULL) {
			MEMCPY(buff, p_v, sizeof(u8));
			flag = OK;
		}
	}
	return flag;
}

//////////////////////////////////////////////////////////////////////////////////////////

u32 PROFILE_IPADDRValueUpdate(PARAM_INDEX* p_ix, void* buff) {
#define IPADDR_LEN 17
	STORAGE_ATOMIC() ; // нужен для MEMCPY
	u32 flag=ERROR;
	u32 i, str_len;
	s8 ipaddr_str[IPADDR_LEN];
	void* write_addr;
	if (p_ix != NULL) {
		str_len = strlen(buff) + 1;
		if (IPADDR_LEN >= str_len) {
			strcpy(ipaddr_str, buff);
			for (i=0; ipaddr_str[i]; i++) {
				if (ipaddr_str[i] == ';')
					ipaddr_str[i] = '.';
			}
			write_addr = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
			if (write_addr != NULL) {
				MEMCPY( write_addr, ipaddr_str, str_len );
				flag = OK;
			}
		}
	}
	return flag;
#undef IPADDR_LEN
}
u32 PROFILE_CHARLINEValueUpdate(PARAM_INDEX* p_ix, void* buff) {
	STORAGE_ATOMIC() ; // нужен для MEMCPY
	u32 flag=ERROR;
	u32 str_len;
	void* write_addr;
	if (p_ix != NULL) {
		str_len = strlen(buff) + 1;
		if (p_ix->parent->data_len >= str_len) {
			write_addr = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
			if (write_addr != NULL) {
				MEMCPY( write_addr, buff, str_len );
				flag = OK;
			}
		}
	}
	return flag;
}
u32 PROFILE_REALValueUpdate(PARAM_INDEX* p_ix, void* buff) {
	STORAGE_ATOMIC() ; // нужен для MEMCPY
	u32 flag=ERROR;
	void* write_addr;
	if (SYS_CheckValue(p_ix, buff) == OK) {
		write_addr = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
		if (write_addr != NULL) {
			MEMCPY( write_addr, buff, sizeof(u32) );
			flag = OK;
		}
	}
	return flag;
}
u32 PROFILE_INTValueUpdate(PARAM_INDEX* p_ix, void* buff) {
	STORAGE_ATOMIC() ; // нужен для MEMCPY
	u32 flag=ERROR;
	void* write_addr;
	if (SYS_CheckValue(p_ix, buff) == OK) {
		write_addr = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
		if (write_addr != NULL) {
			MEMCPY( write_addr, buff, sizeof(s32) );
			flag = OK;
		}
	}
	return flag;
}
u32 PROFILE_BOOLValueUpdate(PARAM_INDEX* p_ix, void* buff) {
	STORAGE_ATOMIC() ; // нужен для MEMCPY
	u32 flag=ERROR;
	void* write_addr;
	if (p_ix != NULL) {
		write_addr = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
		if (write_addr != NULL) {
			MEMCPY( write_addr, buff, sizeof(u8) );
			flag = OK;
		}
	}
	return flag;
}
