//--------------------------------------------------------------------------
//-| FILENAME: crc.h
//-|
//-| Created on: 05.10.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#ifndef CRC_H_
#define CRC_H_

u16 Crc16(u8*,size_t);
u16 Crc16_mem(u8*,size_t,u16);
u32 Crc32(u8* buf, u32 len);
u32 Crc32_mem(u8* buf, u32 len, u32 prev_crc);

#endif /* CRC_H_ */
