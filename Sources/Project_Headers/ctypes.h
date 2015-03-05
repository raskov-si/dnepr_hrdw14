//--------------------------------------------------------------------------
// FILENAME: ctypes.h
//
// Created on: 29.07.2009
// Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#ifndef CTYPES_H_
#define CTYPES_H_
/*
#define __CF_USE_FULL_LIBS 1
#define _MSL_FLOATING_POINT 1
#undef _MSL_NO_MATH_LIB
*/

#include    <stdint.h>
#include    <ysizet.h>

//#define OK 1
//#define ERROR 0xFFFFFFFF
typedef unsigned char	 		u8;
typedef volatile unsigned char	 	vu8;
typedef char				s8;
typedef volatile char			vs8;
typedef unsigned short	 		u16;
typedef volatile unsigned short	 	vu16;
typedef short				s16;
typedef volatile short			vs16;
typedef unsigned long	 		u32;
typedef volatile unsigned long	 	vu32;
typedef long				s32;
typedef volatile long			vs32;
typedef unsigned long long  u64;
typedef float				f32;
typedef double				f64;
//typedef	u32			        size_t;


#ifndef _BOOL
#define _BOOL unsigned char
#endif

//enum _RTOS_STATUS {OK=1,ERROR=0xFFFFFFFF};
#define CR 13
#define LF 10
#define SP 32

#define ASF32(x) (*(f32*)&x)
#define ASS32(x) (*(s32*)&x)
#define ASU32(x) (*(u32*)&x)
#define ASBOOL(x) (*(u8*)&x)


#define INCVOID(x,y)	((void*)((((u8*)(x))+(y))))

#ifndef FALSE
	#define FALSE 0
#endif

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef NULL
	#define NULL	0
#endif


#define ASF32(x) (*(f32*)&x)
#define ASS32(x) (*(s32*)&x)

#endif // CTYPES_H_

