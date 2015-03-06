/*!
\file cc.h
\brief адаптер lwip для архитектуры 
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jul 2014
*/
#ifndef __CC_H__
#define __CC_H__

#ifdef	__cplusplus
    extern "C" {
#endif
      
#include "support_common.h"

#define u8_t	u8
#define u16_t	u16
#define u32_t	u32
#define s8_t	s8
#define s16_t	s16
#define s32_t	s32

typedef u32 mem_ptr_t ;

#define U16_F "hu"
#define S16_F "d"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "uz"

#define BYTE_ORDER                      BIG_ENDIAN

#define LWIP_CHKSUM_ALGORITHM           (3)

#define SYS_ARCH_PROTECT(x)		OS_ENTER_CRITICAL()
#define SYS_ARCH_UNPROTECT(x)		OS_EXIT_CRITICAL()
#define SYS_ARCH_DECL_PROTECT(x) 	STORAGE_ATOMIC()

#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

#define LWIP_COMPAT_MUTEX                   (1)
#define LWIP_PROVIDE_ERRNO

#define LINK_SPEED_OF_YOUR_NETIF_IN_BPS     100000000
#define LWIP_PLATFORM_ASSERT(x)		    

#ifdef	__cplusplus
    }
#endif

#endif	/* __CC_H__ */
