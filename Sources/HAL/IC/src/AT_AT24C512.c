/*!
\file AT_AT24C512.c
\brief ѕроцедуры дл€ работы со стандартными EEPROM на шине i2c
\author <a href="mailto:leonov@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#include "HAL/IC/inc/AT_AT24C512.h"

#define PAGESIZE	32 // размер страницы в EEPROM, 32 чтобы быть совместимыми с microchip 24xx64
#define PAGEMASK	0xFFFFFFE0 // биты адреса, кратного PAGESIZE

#define __read_i2c(p, devAddr,addr,data,len) (p->I2C_ReadMultipleBytes_16( p, devAddr,addr,data,len))
#define __write_i2c(p, devAddr,addr,data,len) (p->I2C_WriteMultipleBytes_16( p, devAddr,addr,data,len))

_BOOL AT_AT24C512_ReadArray( I2C_PeriphInterfaceTypedef *tI2CPeriphInterface, const u8 devAddr, size_t addr, u8* pData, size_t len )
{
	assert( tI2CPeriphInterface );

	return __read_i2c( tI2CPeriphInterface, devAddr, addr , pData , len ) ;
}

_BOOL AT_AT24C512_WriteArray( I2C_PeriphInterfaceTypedef *tI2CPeriphInterface, const u8 devAddr, size_t addr, u8* pData, size_t len )
{
	size_t addr_, len_, btcnt ;
	_BOOL result = TRUE ;
	u32 i ;
	assert( tI2CPeriphInterface );

	btcnt = 0 ;
	addr_ = addr ;
	do{
		len_ = ((addr_ & PAGEMASK) + PAGESIZE) - addr_ ;
		if( len_ > len ){
			len_ = len ;
		}
		result = __write_i2c( tI2CPeriphInterface, devAddr, addr_ , pData + btcnt, len_ ) && result ;
		for( i = 0; i < 3000000; i++ ){
			i = i ;
		}
		btcnt += len_ ;
		len -= len_ ;
		addr_ += len_ ;
	} while( len );
	return result ;
}
