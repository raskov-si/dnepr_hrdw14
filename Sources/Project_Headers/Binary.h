#ifndef __BINARY_H
#define __BINARY_H

//Макросы для двоичной записи констант.
#include <stdint.h>

#define HEX__(n) 0x##n##LU

// 8-bit conversion function
#define B8__(x) ((x&0x0000000FLU)?1:0) \
		+((x&0x000000F0LU)?2:0) \
		+((x&0x00000F00LU)?4:0) \
		+((x&0x0000F000LU)?8:0) \
		+((x&0x000F0000LU)?16:0) \
		+((x&0x00F00000LU)?32:0) \
		+((x&0x0F000000LU)?64:0) \
		+((x&0xF0000000LU)?128:0)


// For upto 8-bit binary constants
#define B8(d) ((uint8_t)B8__(HEX__(d)))

// For upto 16-bit binary constants, MSB first
#define B16(dmsb, dlsb) (((uint16_t)B8(dmsb)<<8) \
		+ B8(dlsb))

// For upto 32-bit binary constants, MSB first
#define B32(dmsb,db2,db3,dlsb) (((uint32_t)B8(dmsb)<<24) \
		+ ((uint32_t)B8(db2)<<16) \
		+ ((uint32_t)B8(db3)<<8) \
		+ B8(dlsb))

// Sample usage:
// B8(01010101) = 85
// B16(10101010,01010101) = 43605
// B32(10000000,11111111,10101010,01010101) = 2164238933

//Макросы для выделения старшего/младшего байтов.
#define _LSB(n) ( (unsigned char)( (n)&0x00FF ) )
#define _MSB(n) ( (unsigned char)( ( (n)&0xFF00 )>>8 ) )

//n=0 - LSB, n=3 - MSB.
#define _BYTE(x, n) ((unsigned char)((x&(0xFF<<(8*(n))))>>(8*(n))))

//Макрос для объединения двух и четырёх байтов
#define _WORD(msb, lsb) ( (unsigned short int)(((msb)<<8)|(lsb)) )
#define _DWORD(b1, b2, b3, b4) ((unsigned int)(((b1)<<24)|((b2)<<16)|((b3)<<8)|(b4)))


//x - указатель на первый элемент массива из 4 элементов (unsigned char)
//Пример: unsigned char x[4]; unsigned int y = __DWORD(&x);
#define __DWORD(x) (*(unsigned int*)(x))
//В обратную сторону - аналогично (это указатель на начало массива)
#define __BYTE(x) ((unsigned char*)&(x))

#define SWAP_BYTES_16W(x)   x = (_WORD(_LSB(x), _MSB(x) ))



#endif
