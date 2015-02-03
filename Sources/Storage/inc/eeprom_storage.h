/*!
\file eeprom_storage.h
\brief ��������������� ��������� � EEPROM � ���������� �� ������
\details �������� ������ ������ � �������� �������������� ������� � ���������
\details ��������� � ������� ������� � �������� ��������. �������������� �.�.
\details ����������� ���� ������� �� ���� ������������ �������.
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/

#ifndef __EEPROM_STORAGE_
#define __EEPROM_STORAGE_

#include "support_common.h"

/*!
\defgroup EEPROM_Storage
\{
*/

//! ������ ��������� ������ � EEPROM. ��� ���������� ����� ����� � ������������ 
//! �������� �������������, ��������� ����, �������� �������.
#define		STEEPROM_VERSION_BASE	0x00001000

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ��������� ������ � �������
// *********************************************************************************************************************
//	Head (����� STEEPROM_HEAD_SIZE ����):
//		1. STEEPROM_PartDescriptor
//		2. Filling Zeros
//		3. CRC32
// *********************************************************************************************************************
//	HashTable (����� STEEPROM_HASHTABLE_SIZE ����):
//		1. HashTable
//		2. CRC32
// *********************************************************************************************************************
//	DataSectors (����� sizeof(SectorHead)+ STEEPROM_MAXNAMELEN + STEEPROM_SECTOR_DATA_SIZE + 4 ����� CRC):
//		1. SectorHead
//		2. Data
//		3. CRC32 -- ����������� ��� ���������, ����� �� ������� ������ ���� ����������
// *********************************************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! ����� ��������� �������, � ����� � HASHTABLE_SIZE ������ �� ��� �� ���� 
//! ������� 2�, ������ �� STEEPROM_VERSION_BASE
#define 	STEEPROM_HEAD_SIZE					252
//! ������ ���-������� � CRC32, ������ �� STEEPROM_VERSION_BASE
#define 	STEEPROM_HASHTABLE_SIZE				1028

//! ������� ������ ������� ���� ������, ������ �� STEEPROM_VERSION_BASE
#define 	STEEPROM_SECTOR_DATA_SIZE			16
//! ������������ ����� ��������, ��� ����� �� ������ �� ������ STEEPROM_VERSION_BASE,
//! �� ������ �� ����� ���, ���������� STEEPROM_PartitionHandler
#define		STEEPROM_MAX_SECTORS				1024 // ������ ����� 128 ����

//! ������� �������� ������ ��� ������ � ������ ������, �� ������ �� STEEPROM_VERSION_BASE
#define 	STEEPROM_MAXNAMELEN					128
//! ������ ����������� ��������� ���������� �� SPI � EEPROM, �� ������ �� STEEPROM_VERSION_BASE
#define		STEEPROM_MAX_EEPROMTRANSACTION		256

//! ���������� �������, ��������� � ����� ������ ������� EEPROM (� 0�� �����),
//! ��������� ��� ������
#pragma pack( push )
#pragma pack( 1 )
typedef struct __STEEPROM_PartDescriptor
{
	u32 version ;			//!< ������ �������� ������

	u32 partition_begin ; 	//!< ����� ������� ����� �������
	u32 partition_end ;		//!< ����� ���������� ����� �������

	//! ���-�� ���� � ����� ������ (STEEPROM_SectorHead::name)
	u8	record_namelen ;

	u32 total_datasectors ; //!< ����� ���������� �������� � ������� � �������
	u32 sector_len ;		//!< ����� �������
} STEEPROM_PartDescriptor ;
#pragma pack( pop )

//! ��������� �������
#pragma pack( push )
#pragma pack( 1 )
typedef struct __STEEPROM_SectorHead {
	//! ������ � ������ ������ (� EEPROM ������ ����� ���������
	//! ���������� PartDescriptor::record_namelen ����)
	u8* name ;

	u32 sect_status ; //!< ������ ������� ��. enum STEEPROM_SectorStatus
	u32 data_crc ; //!< ����������� ����� �� ��� ������
	//! ����� 1�� ������� ������ � ����� �� ��������� _hash( name )
	u32 next_hash_sect ;
	//! ����� 1�� ������� ���������� (��� ��������� � ������� ������) ������
	//! � ����� �� ����� �� name
	u32 prev_hash_sect ;

	u16 record_len ; 		//!< ���-�� ���� ������ �� ���� ������
	u16 sector_datalen ;	//!< ���-�� ���� ������ �� ����� ������� ���������
	u32 next_sect ; 		//!< ����� ���������� ����������� ������� ��� ���� ������ 
} STEEPROM_SectorHead ;
#pragma pack( pop )

//! �������� ���� sect_status � 
typedef enum {
	STEEPROM_SECTSTAT_FREE 	= 0x00000000 ,	//!< ������ ��������
	STEEPROM_SECTSTAT_HEAD 	= 0x00000010 ,	//!< ������ ������ ������
	STEEPROM_SECTSTAT_BODY 	= 0x00000020 ,	//!< �� ������ ������ ������
	STEEPROM_SECTSTAT_INVALID = 0xFFFFFFFF	//!< ��������� ������� ��������
} STEEPROM_SectorStatus ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! ��������� ��� �����-������ � EEPROM. ������������ ������ ������� �� ������ ���� ������ STEEPROM_MAX_EEPROMTRANSACTION
typedef struct __STEEPROM_HW_Interface
{
	void (*ReadArray)( u8* dest, const size_t address, const size_t length ); //!< ������ �����. ������
	void (*WriteArray)( const u8* src, const size_t address, const size_t length ); //!< ����� �����. ������

	//! ��������� ��������� CRC � ���������� 32 ����, ����� -- �� ������ ����
	//! � ������ ������� ����� ������ �������
	u32 (*CRC)(u8* buf, u32 len, u32 prev_crc );
} STEEPROM_HW_Interface;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! ������� ������� � EEPROM ��������� ������ � ������ � ���� ��������� ��
//! STEEPROM_HW_Interface � ��.
typedef struct __STEEPROM_PartitionHandler
{
	STEEPROM_HW_Interface *pHWI ;
	_BOOL	opened ;

	struct __STEEPROM_PartDescriptor part_params ; // ������ ��� �������� �������

	// ������ ��������� ��������, ����� �� ��������� ������ ��� ������� � EEPROM
	// ����������� ��� ��������, ����� �������������� ����������
	u8 freesector_bitmap[ STEEPROM_MAX_SECTORS/8 ];
} STEEPROM_PartitionHandler ;

/*!
\brief ������ ���������� ������� �� EEPROM
\details ���������� ����� ������� ������ � ��������. EEPROM ������ ���� �������� ����� ��������� �� ����� ����������.
\param pPHandler ��������� �� ���������, ��� �������� �������� ������������ ��������� ��������� �������
\param begin_address ����� ������� ����� ������������ �������
\param tHWInterface ��������� �� ����������� ��������� ����������
\retval FALSE ���� ������ �� ��������������
*/
_BOOL STEEPROM_OpenPartition( 	STEEPROM_PartitionHandler *pPHandler,
								const size_t begin_address,
								STEEPROM_HW_Interface* tHWInterface );
_BOOL STEEPROM_IsPartitionOpened( const STEEPROM_PartitionHandler *pPHandler );

//! ��������� �������������� ���������
typedef struct __STEEPROM_FormatParams
{
	size_t	begin_address ;		//!< ����� ������ �������
	size_t	end_address;		//!< ����� ����� �������

	size_t	record_namelen	; 	//!< ������������ ����� ����� ������
	size_t	total_datasectors ;	//!< ������������ ���������� �������� � �������, �.�. ������ 8
} STEEPROM_FormatParams ;

//! ���������� �������������� eeprom. ����� �������������� ������ ���������� �������.
void STEEPROM_Format( const STEEPROM_HW_Interface *pHWI, const STEEPROM_FormatParams* formatParams ) ;

// TODO: ��������� ��� �� ��� ������
//! ��������� crc ���� ��������
//! \retval TRUE ��� ������� ������
_BOOL STEEPROM_CheckPartition( const STEEPROM_PartitionHandler *pPHandler );

/*!
\brief ���������� ����������/���������� �����. ������ � EEPROM
\param pPHandler ��������� �� �������� ������� �������
\param sName ������ � ������ ������, �� ������ STEEPROM_FormatParams.record_namelen
\param pbData �������� ������ � �������. ����� ���� ������� �������
\param len ����� ������� sData.
\retval FALSE ���� �� �������� ����������� ��� ������ �� ������
*/
_BOOL STEEPROM_WriteRecord( STEEPROM_PartitionHandler *pPHandler, 
							u8* sName, u8* pbData, const size_t len );

/*!
\brief ���� ������ � �����. ������ � ����� � ����������.
\param pPHandler ��������� �� ��������� �������� ��������� �������
\param sName ��� ������
\param pbData ���� ������� ������
\param actual_len ������� ���� �������� � ���������, 0 ���� �������� �� ������
\param maxlen ����� ������� pbData
\retval FALSE ���� ������ �� �������� ��� ������ �� ������
*/
_BOOL STEEPROM_ReadRecord( 	const STEEPROM_PartitionHandler *pPHandler,
							u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen );

//! \brief ������� ��������, ��������� ��� CRC � ���������� CRC �� ������
_BOOL STEEPROM_CheckValue( 	const STEEPROM_PartitionHandler *pPHandler,
							u8* sName, u32* CRC );

void STEEPROM_SpoilPartition( const STEEPROM_PartitionHandler *pPHandler );

/*!
\}
*/ // EEPROM_Storage

// ���������� ���������, ������ ������ ������ ��� ������
#ifdef __STEEPROM_INTERNALS

// ���������� __bitmap_next_free ����� �� �������� ��������� ��������
#define STEEPROM_INVALID_ADDRESS		0xFFFFFFFF

u32		__hashtable_seek( const STEEPROM_PartitionHandler *pPHandler, u8* sName );
void 	__hashtable_change( STEEPROM_PartitionHandler *pPHandler,
												u8* sName, const u32 sect );

void 	__bitmap_set( u8* bitmap, const size_t sect_num, const _BOOL val );
_BOOL 	__bitmap_get( const u8* bitmap, const size_t sect_num );
size_t __bitmap_next_free( const u8* bitmap, const size_t total_sectors );
size_t 	__bitmap_count_free( const u8* bitmap, const size_t total_sectors );


//! ��������� ��� ����� ������� �������
typedef struct ___STEEPROM_SECTOR {
	STEEPROM_SectorHead head ;
	u8 data[ STEEPROM_SECTOR_DATA_SIZE ];
	u32 crc ;
} __STEEPROM_Sector ;

//! ���� ������ ������ ���������
//! \param pPHandler ������� �������
//! \param sect ���� ������� ��� ������ ������, ����� ����� ���� sect != STEEPROM_INVALID_ADDRESS
//! \param sName ��� ���������
//! \param sect_n ���� ������� ����� �������, ���� �������� �� ������ ������� STEEPROM_INVALID_ADDRESS
//! \retval TRUE ���� �� �������� ����������������� ��������� ������
_BOOL __seek_1st_sector( const STEEPROM_PartitionHandler *pPHandler, __STEEPROM_Sector *sect,
					 u8* sName, size_t *sect_n );

void __write_sector( const STEEPROM_PartitionHandler *pPHandler, 
				const size_t sect_num, __STEEPROM_Sector* buff );

#endif // __STEEPROM_INTERNALS

#endif
