/*!
\file NMNX_M25P128.h
\brief Driver for Numonix (Micron) M25P128 SPI NAND flash memory.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 05.06.2012
*/

#ifndef __NMNX_M25P128_DRIVER_H
#define __NMNX_M25P128_DRIVER_H

#include "support_common.h"
#include "HAL\IC\inc\SPI_Interface.h"

/* Protected area sizes */
#define NMNX_M25P128_PROTECT_NONE          0x00
#define NMNX_M25P128_PROTECT_UPPER_64TH    0x01
#define NMNX_M25P128_PROTECT_UPPER_32ND    0x02
#define NMNX_M25P128_PROTECT_UPPER_16ND    0x03
#define NMNX_M25P128_PROTECT_UPPER_8ND     0x04
#define NMNX_M25P128_PROTECT_UPPER_QUARTER 0x05
#define NMNX_M25P128_PROTECT_UPPER_HALF    0x06
#define NMNX_M25P128_PROTECT_ALL           0x07

/* Memory organization */
#define NMNX_M25P128_SECTORS_NUMBER 64
#define NMNX_M25P128_PAGE_SIZE 256
#define NMNX_M25P128_MEMORY_BASE 0x000000
#define NMNX_M25P128_SECTOR_SIZE 0x040000 /* 256 bytes * 1024 pages */
#define NMNX_M25P128_SECTOR_BASE(s) ( NMNX_M25P128_MEMORY_BASE + NMNX_M25P128_SECTOR_SIZE*(s) ) /*! \param s Sector number. 0 through ( \ref NMNX_M25P128_SECTORS_NUMBER - 1 )*/
#define NMNX_M25P128_SECTOR_CEIL(s) ( NMNX_M25P128_SECTOR_BASE(s) + (NMNX_M25P128_SECTOR_SIZE-1) ) /*! \param s Sector number. 0 through ( \ref NMNX_M25P128_SECTORS_NUMBER - 1 )*/
#define NMNX_M25P128_PAGE_BASE(p) ( NMNX_M25P128_MEMORY_BASE + (p)*NMNX_M25P128_PAGE_SIZE )
/*! Base address of a given block. 1 block = 512 bytes = 2 pages. */
//#define NMNX_M25P128_BLOCK_BASE(b) ( NMNX_M25P128_PAGE_BASE((b)*2) )
/*!
\brief Base address of a page in a given block.
\param b Number of a block.
\param p Number of page in a block. Should be 0 or 1 since one block equals two pages.
*/
//#define NMNX_M25P128_BLOCK_PAGE_BASE(b, p) ( NMNX_M25P128_BLOCK_BASE(b)+(NMNX_M25P128_PAGE_SIZE*(p)) )
/*! \note Following macro assumes that 1 Erase sector = 512 bytes. */
#define NMNX_M25P128_BLOCK_PAGE_BASE(b, p) ( NMNX_M25P128_SECTOR_BASE(b)+(NMNX_M25P128_PAGE_SIZE*(p)) )

/* Instructions */
#define NMNX_M25P128_WREN      0x06
#define NMNX_M25P128_WRDI      0x04
#define NMNX_M25P128_RDID      0x9F
#define NMNX_M25P128_RDSR      0x05
#define NMNX_M25P128_WRSR      0x01
#define NMNX_M25P128_READ      0x03
#define NMNX_M25P128_FAST_READ 0x0B
#define NMNX_M25P128_PP        0x02
#define NMNX_M25P128_SE        0xD8
#define NMNX_M25P128_BE        0xC7

/* Status Register */
#define NMNX_M25P128_GET_SRWD(b) ( (b&0x80) >> 7 )
#define NMNX_M25P128_GET_BP(b)   ( (b&0x1C) >> 2 )
#define NMNX_M25P128_GET_WEL(b)  ( (b&0x02) >> 1 )
#define NMNX_M25P128_GET_WIP(b)  ( (b&0x01) )

#define NMNX_M25P128_SET_SRWD(b) ( (b&0x01) << 7 )
#define NMNX_M25P128_SET_BP(b)   ( (b&0x07) << 2 )
#define NMNX_M25P128_SET_WEL(b)  ( (b&0x01) << 1 )
#define NMNX_M25P128_SET_WIP(b)  ( (b&0x01) )

/*!
\defgroup NMNX_M25P128_Driver
\{
*/

/*!
\}
*/ //NMNX_M25P128_Driver_Exported_Types
/*! Manufacturer and device general info */
typedef struct __NMNX_M25P128_IdentificationTypedef{
	u8 nManufacturerId;  /*!< Manufacturer identification assigned by JEDEC (\c 20h for Numonyx). */
	u8 nMemoryType;      /*!< Memory type (20h). */
	u8 nMemoryCapacity;  /*!< Memory capacity (18h). */
}NMNX_M25P128_IdentificationTypedef;

/*!
\}
*/ //NMNX_M25P128_Driver
void NMNX_M25P128_InitPeripherialInterface(SPI_PeriphInterfaceTypedef* tSpiPeriphInterface);
_BOOL NMNX_M25P128_ReadMfrId( NMNX_M25P128_IdentificationTypedef* tMfrIdStructure );
_BOOL NMNX_M25P128_ReadBytes( u32 nStartingAddress, u8* anRecievedBytes, u16 nBytesNumber );
_BOOL NMNX_M25P128_PageProgram( u32 nStartingAddress, u8* anBytesToWrite, u16 nBytesNumber );
_BOOL NMNX_M25P128_WaitForWriteCompletion();
_BOOL NMNX_M25P128_WaitForWriteEnable();
u8 NMNX_M25P128_ReadStatusRegister();
/* Yet to be modified or implemented */
_BOOL NMNX_M25P128_SectorErase( u32 nAddress );
_BOOL NMNX_M25P128_BulkErase();
_BOOL NMNX_M25P128_FastRead( u32 nStartingAddress, u8* anRecievedBytes, u16 nBytesNumber );
u8 NMNX_M25P128_WriteStatusRegister( u8 nStatusRegisterValue );

#endif //__NMNX_M25P128_DRIVER_H
