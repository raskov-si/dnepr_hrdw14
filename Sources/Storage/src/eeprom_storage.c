/*!
\file eeprom_storage.c
\brief ������������� ��������� � EEPROM � ���������� �� ������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/
#include "common_lib/memory.h"
#define __STEEPROM_INTERNALS
#include "Storage/inc/eeprom_storage.h"
#include <string.h> // strnlen
#include <stdio.h>

static u8 __sect_name_buff[ STEEPROM_MAXNAMELEN ] ;
static __STEEPROM_Sector __sect_buff ; // ����� ������ ������� �� ��� �����

static u32 __hashtable_crc( const STEEPROM_HW_Interface *pHWI, const u32 partition_begin );
static void __renew_hashtable_crc( const STEEPROM_HW_Interface *pHWI, const u32 partition_begin );
static u8 __hash(u8* str);

//! ������ ������ � ���������� ��� ���������� CRC
static _BOOL __read_sector( const STEEPROM_PartitionHandler *pPHandler, 
 					const size_t sect_num, __STEEPROM_Sector* buff );


//! ����� ������ ������� � eeprom
#define	__STEEPROM_SECT_BEGIN(pPHandler,sectn)	(pPHandler->part_params.partition_begin+				\
													STEEPROM_HEAD_SIZE+STEEPROM_HASHTABLE_SIZE+			\
 			 										sectn*pPHandler->part_params.sector_len)
//! ����� ����� ��������� � ���������� �������
#define __STEEPROM_NAMELEN(pPHandler)			(pPHandler->part_params.record_namelen)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define __READ_EEPROM(partd,dest,addr,len) 	((partd->pHWI->ReadArray)(dest,addr,len))
#define __WRITE_EEPROM(partd,src,addr,len) 	((partd->pHWI->WriteArray)(src,addr,len))
#define __CRC_EEPROM(partd,arr,len)			((partd->pHWI->CRC)(arr,len,0))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL STEEPROM_OpenPartition( 	STEEPROM_PartitionHandler *pPHandler,
								const size_t begin_address,
								STEEPROM_HW_Interface* tHWInterface )
{
	u8 buff[256] ;
	u32 crc ;
	size_t i ;
	assert( pPHandler );
	assert( tHWInterface );

	pPHandler->pHWI = tHWInterface ;

	// ������ ���������� �������
	__READ_EEPROM(pPHandler, buff, begin_address, STEEPROM_HEAD_SIZE );
	// ���� �� ������ CRC -- ������� � �������
	if( __CRC_EEPROM(pPHandler, buff, STEEPROM_HEAD_SIZE-4) != 
			*((u32*)&buff[STEEPROM_HEAD_SIZE-4]) ){
		pPHandler->opened = FALSE ;
		return FALSE ;
	}
	t8_memcopy( (u8*)&pPHandler->part_params, buff, sizeof(STEEPROM_PartDescriptor) );
	// ���� �� ������� ������ -- ���� ������� � �������
	if( (pPHandler->part_params.version & STEEPROM_VERSION_BASE) == 0){
		pPHandler->opened = FALSE ;
		return FALSE ;
	}
	// ��������� ����������� ���-�������
	crc = __hashtable_crc( pPHandler->pHWI,
										pPHandler->part_params.partition_begin );
	__READ_EEPROM( pPHandler, buff,
					begin_address + STEEPROM_HEAD_SIZE + STEEPROM_HASHTABLE_SIZE - 4,
					4 );
	if( crc != *((u32*)buff) ){
		pPHandler->opened = FALSE ;
		return FALSE ;
	}

	// ��������� freesector_bitmap
	t8_memzero( pPHandler->freesector_bitmap, pPHandler->part_params.total_datasectors >> 3 );
	// �������� ��� ������� � ������ 1 ���� ������ ��������
	for( i = 0; i < pPHandler->part_params.total_datasectors; i++ ){
		// ������ ������ status
		__READ_EEPROM( pPHandler, buff, 
			__STEEPROM_SECT_BEGIN(pPHandler, i)+__STEEPROM_NAMELEN(pPHandler),
																		4 );

		if( *((u32*)buff) == STEEPROM_SECTSTAT_FREE ){
			__bitmap_set( pPHandler->freesector_bitmap, i, TRUE );
		} else {
			__bitmap_set( pPHandler->freesector_bitmap, i, FALSE );
		}
	}

	pPHandler->opened = TRUE ;
	return TRUE ;
}

_BOOL STEEPROM_IsPartitionOpened( const STEEPROM_PartitionHandler *pPHandler )
{
	assert( pPHandler );

	return ( pPHandler->opened == TRUE );
}

void STEEPROM_Format( const STEEPROM_HW_Interface *pHWI, const STEEPROM_FormatParams* formatParams )
{
	u8 buff[256] ;
	STEEPROM_PartDescriptor part_params;
	u32 crc ;
	size_t total_size ;
	size_t actual_size ;
	size_t i, j ;
	size_t sector_address ;

	assert( pHWI );
	assert( formatParams );
	assert( formatParams-> begin_address < formatParams->end_address );
	assert( formatParams->record_namelen <= STEEPROM_MAXNAMELEN );

	////////////////////////////////////////////////////////////////////////////
	// ���������� �������
	part_params.version 			= STEEPROM_VERSION_BASE ;
	part_params.partition_begin 	= formatParams->begin_address ;
	part_params.partition_end 		= formatParams->end_address ;
	part_params.record_namelen 		= formatParams->record_namelen ;
	part_params.total_datasectors 	= formatParams->total_datasectors ;
	part_params.sector_len 			= sizeof(STEEPROM_SectorHead) - sizeof(u8*) +
										part_params.record_namelen +
						 				STEEPROM_SECTOR_DATA_SIZE + 4 ;

	////////////////////////////////////////////////////////////////////////////
	// ��������� ������ �� ������� �������
	total_size = formatParams->end_address - formatParams->begin_address ;
	// �������������� �����, 4 ����� ���������� ��-�� ��������� name
	actual_size  = 	STEEPROM_HEAD_SIZE + STEEPROM_HASHTABLE_SIZE +
						( part_params.sector_len )*part_params.total_datasectors ;
	// �� ������� ������� ������� - �������
	if( total_size < actual_size )
		return ;
	// �����, ����� �������� �� 8 -- ������� ��� ������ bitmap'�� ���������
	// �������� ������� �����
	if( part_params.total_datasectors & 0x7 )
		return ;
	if( part_params.total_datasectors > STEEPROM_MAX_SECTORS )
		return ;
	if( part_params.record_namelen > STEEPROM_MAXNAMELEN )
		return ;

	////////////////////////////////////////////////////////////////////////////
	// ������ ��������� ������� ����� ��� ��� ����� ������� ���������� ��������

	buff[ 0 ] = 0xBB ;
	buff[ 1 ] = 0xAA ;
	buff[ 2 ] = 0xDD ;
	buff[ 3 ] = 0xCC ;
	pHWI->WriteArray( buff, part_params.partition_begin, 4 );

	////////////////////////////////////////////////////////////////////////////
	// ��������� ��� �������

	__sect_buff.head.name 			= __sect_name_buff ;
	sprintf( (char *)__sect_buff.head.name, "INVALID_NAME" );

	__sect_buff.head.sect_status 	= STEEPROM_SECTSTAT_FREE ;
	__sect_buff.head.sect_status	= 0 ;
	__sect_buff.head.data_crc		= 0 ;
	__sect_buff.head.next_hash_sect	= 0 ;
	__sect_buff.head.prev_hash_sect = 0 ;
	__sect_buff.head.record_len 	= 0 ;
	__sect_buff.head.sector_datalen = 0 ;
	__sect_buff.head.next_sect 		= STEEPROM_INVALID_ADDRESS ;

	t8_memzero( __sect_buff.data, STEEPROM_SECTOR_DATA_SIZE );
	// ��������� CRC
	crc = pHWI->CRC( __sect_buff.head.name, part_params.record_namelen, 0 );
 	crc = pHWI->CRC( (u8*)&__sect_buff.head.sect_status, 
 		part_params.sector_len - part_params.record_namelen - sizeof(u32), crc );
 	__sect_buff.crc = crc ;

 	// pPHandler ��� ���, ������� ����� ������� ������
	for( j = 0; j < part_params.total_datasectors; j++ ){
		sector_address = part_params.partition_begin 
			+ STEEPROM_HEAD_SIZE+STEEPROM_HASHTABLE_SIZE + j*part_params.sector_len ;
		// ����� ���
		pHWI->WriteArray( __sect_buff.head.name, sector_address, 
												part_params.record_namelen );
		// � �� ���������
		pHWI->WriteArray( (u8*)&__sect_buff.head.sect_status,
			sector_address + part_params.record_namelen,
			part_params.sector_len - part_params.record_namelen );
	}

	////////////////////////////////////////////////////////////////////////////
	// ��������� Head

	t8_memzero( buff, sizeof(buff) );
	t8_memcopy( buff, (u8*)&part_params, sizeof(STEEPROM_PartDescriptor) );

	// ������� CRC
	*((u32*)&buff[STEEPROM_HEAD_SIZE-4]) =
								pHWI->CRC( buff, STEEPROM_HEAD_SIZE - 4, 0 );

	// ����� � Head
	pHWI->WriteArray( buff, part_params.partition_begin, STEEPROM_HEAD_SIZE );
	
	////////////////////////////////////////////////////////////////////////////
	// ��������� HashTable FF, ������ ���������� STEEPROM_INVALID_ADDRESS �� ���� �������
	memset( buff, 0xFF, sizeof(buff) );
	// �������� � ��������� ��� ������ ���-������� ������ � ������� ������ � EEPROM ������� buff � 256 ����
	for( i = 0; i < STEEPROM_HASHTABLE_SIZE - 4; i += sizeof(buff) ){
		// ���� ����� ������ �� ��� 256 ����
		if( (i + sizeof(buff)) < (STEEPROM_HASHTABLE_SIZE - 4) ){
			pHWI->WriteArray( buff, i + part_params.partition_begin + STEEPROM_HEAD_SIZE,
																sizeof(buff) );
		// ����� ���-������� ������ ��� ����� 256 ����
		} else {
			pHWI->WriteArray( buff, i + part_params.partition_begin + STEEPROM_HEAD_SIZE,
											STEEPROM_HASHTABLE_SIZE - 4 - i );
		}
	}
	__renew_hashtable_crc( pHWI, part_params.partition_begin );
}

_BOOL STEEPROM_CheckPartition( const STEEPROM_PartitionHandler *pPHandler )
{
	size_t i ;
	assert( pPHandler );

	if( !STEEPROM_IsPartitionOpened( pPHandler ) ){
		return FALSE ;
	}
	__sect_buff.head.name = __sect_name_buff ;
	__sect_buff.head.name[ 0 ] = 0 ;

	for( i = 0; i < pPHandler->part_params.total_datasectors; i++ ){
		if( !__read_sector( pPHandler, i, &__sect_buff ) )
			return FALSE ;
		
	}
	return TRUE ;
}

void STEEPROM_SpoilRecord( STEEPROM_PartitionHandler *pPHandler, u8* sName )
{

}

_BOOL STEEPROM_WriteRecord( STEEPROM_PartitionHandler *pPHandler, 
							u8* sName, u8* pbData, const size_t len )
{
	u32 sect_n ;
	u32 sect_n_next ;
	u32 sect_n_prev ;
	u32 data_crc ;
	size_t free_sectors ;
	size_t i ;
	size_t isects_new ;
	size_t isects_old ;

	assert( sName );
	assert( pbData );
	assert( pPHandler );
	assert( len > 0 ); // �� ������������ ������ ���������

	__sect_buff.head.name = __sect_name_buff ;
	__sect_buff.head.name[ 0 ] = 0 ;

	// ���� ������������ -- �������
	if( strnlen( (const char*)sName, __STEEPROM_NAMELEN(pPHandler) ) < 0 ){
		return FALSE ;
	}

	// ������� �������� ����� ����� �������� ���������
	isects_new = len / STEEPROM_SECTOR_DATA_SIZE ;
	if( len % STEEPROM_SECTOR_DATA_SIZE ){
		isects_new += 1 ;
	}

	// ��� crc
	data_crc = __CRC_EEPROM( pPHandler, pbData, len );

	// ������� ����� ������ ������ ���������
	if( !__seek_1st_sector( pPHandler, &__sect_buff, sName, &sect_n) ){
		return FALSE ;
	}

	// ������� ���������� ��������� ��������
	free_sectors = __bitmap_count_free( pPHandler->freesector_bitmap,
									pPHandler->part_params.total_datasectors );

	// �������� ������������
	if( sect_n != STEEPROM_INVALID_ADDRESS ){
		// ��������
		if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_HEAD ){
			return FALSE ;
		}
		// ������ � __sect_buff ������ ������ ����� ���������

		// ������� �������� �������� ������� �������� ���������
		isects_old = __sect_buff.head.record_len / STEEPROM_SECTOR_DATA_SIZE ;
		if( __sect_buff.head.record_len % STEEPROM_SECTOR_DATA_SIZE ){
			isects_old += 1 ;
		}

		// ���-�� �������� �������������
		if( isects_new > isects_old ){
			// ��������� ���� �� ����� ��� ����������� ��������
			if( (isects_new - isects_old) > free_sectors ){
				return FALSE ;
			}

			// ��������� ��� ����� ���������� ���������� �������
			for( i = 0; i < isects_old-1; i++ ){
				if( i > 0 ){
					__sect_buff.head.sect_status = STEEPROM_SECTSTAT_BODY ;
				}
				__sect_buff.head.record_len = len ;
				__sect_buff.head.data_crc = data_crc ;
				t8_memcopy( __sect_buff.data, pbData+i*STEEPROM_SECTOR_DATA_SIZE,
												 STEEPROM_SECTOR_DATA_SIZE );
				__write_sector( pPHandler, sect_n, &__sect_buff );

				sect_n = __sect_buff.head.next_sect  ;
				if( !sect_n ){
					return FALSE ;
				}
				// ������ ��������� ������, ���� ������ -- ��������� ������ ���������, ������� � �������
				if( !__read_sector( pPHandler, sect_n, &__sect_buff ) ){
					return FALSE ;
				}
				if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_BODY ){
					return FALSE ;
				}
			}
			// ������ � __sect_buff ��������� ������ ������� ��������

			// �������� ����� � ��������� ��������� ������
			for( ; i < isects_new; i++ ){
				// ���� ��� �� ��������� ������ -- �������� ��������� ������
				if( i < (isects_new-1) ){
					// ���� ��� ���� ������, ������� ��������� ������� ������
					sect_n_next = __bitmap_next_free( pPHandler->freesector_bitmap,
										pPHandler->part_params.total_datasectors );
					if( sect_n_next == STEEPROM_INVALID_ADDRESS ){
						return FALSE ;
					}
					// �������� ��� �������
					__bitmap_set( pPHandler->freesector_bitmap, sect_n_next, FALSE );
				} else {
					sect_n_next = STEEPROM_INVALID_ADDRESS ;
				}

				__sect_buff.head.record_len = len ;
				__sect_buff.head.data_crc = data_crc ;
				__sect_buff.head.next_sect = sect_n_next ;
				__sect_buff.head.sect_status = i == 0 ? STEEPROM_SECTSTAT_HEAD : STEEPROM_SECTSTAT_BODY ;
				// ���� ����� ���� ���� ������
				if( (i+1)*STEEPROM_SECTOR_DATA_SIZE < len ){
					__sect_buff.head.sector_datalen = STEEPROM_SECTOR_DATA_SIZE ;
					t8_memcopy( __sect_buff.data, pbData+i*STEEPROM_SECTOR_DATA_SIZE,
														 STEEPROM_SECTOR_DATA_SIZE );
				// �������� ����� ����� �������
				} else {
					__sect_buff.head.sector_datalen = len - i*STEEPROM_SECTOR_DATA_SIZE ;
					t8_memcopy( __sect_buff.data, pbData+i*STEEPROM_SECTOR_DATA_SIZE,
											 __sect_buff.head.sector_datalen );
				}
				__write_sector( pPHandler, sect_n, &__sect_buff );
				sect_n = sect_n_next ;
			}
		// ���-�� �������� ����������� ��� �� ��������
		} else {
			// ��������� ������ ������ ��������� � ��������� ��� �����
			// � sect_n_prev
			sect_n_prev = sect_n ;
			// � ������� ������ ������ ������� �������, ����� CRC �� ���� ��������
			// �� ������ � ������ ���������� ���������� ������� � ���� ������
			// ��� ����� ������� ������ ������ ��������
			for( i = 1; i < isects_new; i++ ){
				sect_n = __sect_buff.head.next_sect ;
				// ������ ��������� ������, ���� ������ -- ��������� ������ ���������, ������� � �������
				if( !__read_sector( pPHandler, sect_n, &__sect_buff ) ){
					return FALSE ;
				}
				if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_BODY ){
					return FALSE ;
				}
			}
			sect_n = __sect_buff.head.next_sect ;
			// ������ i == isects_new
			assert( i == isects_new );
			for( ; i < isects_old; i++ ){
				// ������ ��������� ������, ���� ������ -- ��������� ������ ���������, ������� � �������
				if( !__read_sector( pPHandler, sect_n, &__sect_buff ) ){
					return FALSE ;
				}
				if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_BODY ){
					return FALSE ;
				}
				sect_n_next = __sect_buff.head.next_sect ;
				__sect_buff.head.data_crc = 0 ;
				__sect_buff.head.next_sect = STEEPROM_INVALID_ADDRESS ;
				__sect_buff.head.sector_datalen = 0 ;
				__sect_buff.head.sect_status = STEEPROM_SECTSTAT_FREE ;
				__write_sector( pPHandler, sect_n, &__sect_buff );
				__bitmap_set( pPHandler->freesector_bitmap, sect_n, TRUE );
				sect_n = sect_n_next ;
			}
			// ������������ � ��������� ���������
			sect_n = sect_n_prev ;
			if( !__read_sector( pPHandler, sect_n, &__sect_buff ) ){
				return FALSE ;
			}
			if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_HEAD ){
				return FALSE ;
			}

			// ��������� ��� ����� ���������� ������� ������ ��������
			for( i = 0; i < isects_new-1; i++ ){
				if( i > 0 ){
					__sect_buff.head.sect_status = STEEPROM_SECTSTAT_BODY ;
				}
				__sect_buff.head.record_len = len ;
				__sect_buff.head.data_crc = data_crc ;
				t8_memcopy( __sect_buff.data, pbData+i*STEEPROM_SECTOR_DATA_SIZE,
												 STEEPROM_SECTOR_DATA_SIZE );
				__write_sector( pPHandler, sect_n, &__sect_buff );

				sect_n = __sect_buff.head.next_sect  ;
				if( !sect_n ){
					return FALSE ;
				}
				// ������ ��������� ������, ���� ������ -- ��������� ������ ���������, ������� � �������
				if( !__read_sector( pPHandler, sect_n, &__sect_buff ) ){
					return FALSE ;
				}
				if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_BODY ){
					return FALSE ;
				}
			}

			// ��������� ��������� ������
			__sect_buff.head.record_len = len ;
			__sect_buff.head.data_crc = data_crc ;
			__sect_buff.head.next_sect = STEEPROM_INVALID_ADDRESS ;
			__sect_buff.head.sector_datalen = len - i*STEEPROM_SECTOR_DATA_SIZE ;
			t8_memcopy( __sect_buff.data, pbData+i*STEEPROM_SECTOR_DATA_SIZE,
									 __sect_buff.head.sector_datalen );
			__write_sector( pPHandler, sect_n, &__sect_buff );
		}
	// �������� �����������, ��������� �����
	} else {
		// ���� ����� �������� ���-�������, �� ������� ������ �������� � ���-�������, ����� ��� ������ � �������, �����
		// crc ����� ���-�������
		_BOOL renew_hashtable_crc = FALSE ;
		// ��������� ���� �� ����� ��� ����������� ��������
		if( isects_new > free_sectors ){
			return FALSE ;
		}

		// �������� ����� ������
		sect_n = __bitmap_next_free( pPHandler->freesector_bitmap,
										pPHandler->part_params.total_datasectors );
		if(sect_n == STEEPROM_INVALID_ADDRESS){
			return FALSE ;
		}
		// �������� ��� �������
		__bitmap_set( pPHandler->freesector_bitmap, sect_n, FALSE );

		// ���� � ���-������� ���� ����� ��� -- ������ �������� 
		// � ����� �������� ������
		// ������ �
		// * sect_n 		-- ����� ������
		// * sect_n_next 	-- �������� ������, �� ������� ��������� ��� �������
		// * sect_n_prev	-- ������ ���������� ������
		sect_n_next = __hashtable_seek( pPHandler, sName );
		if( sect_n_next != STEEPROM_INVALID_ADDRESS ){
			if( !__read_sector( pPHandler, sect_n_next, &__sect_buff) ){
				return FALSE ;
			}
			if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_HEAD ){
				return FALSE ;
			}
			sect_n_prev = __sect_buff.head.prev_hash_sect ;
		
			__sect_buff.head.prev_hash_sect = sect_n ;
			__write_sector( pPHandler, sect_n_next, &__sect_buff );

			// �������� ��������� � ������ ���������� �������
			if( !__read_sector( pPHandler, sect_n_prev, &__sect_buff) ){
				return FALSE ;
			}
			if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_HEAD ){
				return FALSE ;
			}
			__sect_buff.head.next_hash_sect = sect_n ;
			__write_sector( pPHandler, sect_n_prev, &__sect_buff );
		// ���� � ���-������� ���  ������ ���� -- ���������
		} else {
			renew_hashtable_crc = TRUE ;
			sect_n_next = sect_n ;
			sect_n_prev = sect_n ;
			// ��������� ������ � ���-�������
			__hashtable_change( pPHandler, sName, sect_n );
		}

		// ����� ��� ������� ������ ���������
		t8_memzero( __sect_buff.head.name, pPHandler->part_params.record_namelen );
		strncpy( (char*)__sect_buff.head.name, (char*)sName, pPHandler->part_params.record_namelen - 1 );
		__sect_buff.head.name[pPHandler->part_params.record_namelen-1] = '\0';
		__sect_buff.head.sect_status = STEEPROM_SECTSTAT_HEAD ;
		__sect_buff.head.data_crc = data_crc ;
		__sect_buff.head.next_hash_sect = sect_n_next ;
		__sect_buff.head.prev_hash_sect = sect_n_prev ;
		__sect_buff.head.record_len = len ;
		// �������� ��� ������� ����� ����������
		for( i = 0; i < isects_new-1; i++ ){
			if( i > 0 ){
				__sect_buff.head.sect_status = STEEPROM_SECTSTAT_BODY ;
			}
			// �������� ��� ������
			t8_memcopy( __sect_buff.data, (u8*)pbData+i*STEEPROM_SECTOR_DATA_SIZE,
										 STEEPROM_SECTOR_DATA_SIZE );
			__sect_buff.head.sector_datalen = STEEPROM_SECTOR_DATA_SIZE ;
			// �������� ����� ������
			sect_n_next = __bitmap_next_free( pPHandler->freesector_bitmap,
											pPHandler->part_params.total_datasectors );
			// �������� ��� �������
			__bitmap_set( pPHandler->freesector_bitmap, sect_n_next, FALSE );

			__sect_buff.head.next_sect = sect_n_next ;

			__write_sector( pPHandler, sect_n, &__sect_buff );
			sect_n = sect_n_next ;
		}
		if( i > 0 ){
			__sect_buff.head.sect_status = STEEPROM_SECTSTAT_BODY ;
		}
		__sect_buff.head.next_sect = STEEPROM_INVALID_ADDRESS ;
		__sect_buff.head.sector_datalen = len - i*STEEPROM_SECTOR_DATA_SIZE ;
		t8_memcopy( __sect_buff.data, pbData+i*STEEPROM_SECTOR_DATA_SIZE,
								 __sect_buff.head.sector_datalen );
		__write_sector( pPHandler, sect_n, &__sect_buff );

		if( renew_hashtable_crc ){
			__renew_hashtable_crc( pPHandler->pHWI,
									pPHandler->part_params.partition_begin );
		}

	}
	return TRUE ;
}

_BOOL STEEPROM_ReadRecord( 	const STEEPROM_PartitionHandler *pPHandler,
							u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen )
{
	u32 sect_n ;
	size_t isects ;
	size_t i ;
	u32 data_crc, secthead_crc ;

	assert( sName );
	assert( pbData );
	assert( pPHandler );
	assert( actual_len );

	__sect_buff.head.name = __sect_name_buff ;
	__sect_buff.head.name[ 0 ] = 0 ;
	
	// ���� ������������
	if( strnlen( (const char*)sName, __STEEPROM_NAMELEN(pPHandler) ) < 0){
		return FALSE ;
	}

	if( !__seek_1st_sector( pPHandler, &__sect_buff, sName, &sect_n) ){
		return FALSE ;
	}
	if( sect_n == STEEPROM_INVALID_ADDRESS ){
		*actual_len = 0 ;
		return TRUE ;
	}
	*actual_len = __sect_buff.head.record_len ;
	secthead_crc = __sect_buff.head.data_crc ;
	if( maxlen < *actual_len ){
		return FALSE ;
	}
	if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_HEAD ){
		return FALSE ;
	}

	// ������� �������� �������� ��������
	isects = __sect_buff.head.record_len / STEEPROM_SECTOR_DATA_SIZE ;
	if( __sect_buff.head.record_len % STEEPROM_SECTOR_DATA_SIZE ){
		isects += 1 ;
	}
	// �������� ��� �������
	for( i = 0; i < isects; i++ ){
		t8_memcopy( pbData+i*STEEPROM_SECTOR_DATA_SIZE,
				__sect_buff.data,
								__sect_buff.head.sector_datalen );

		sect_n = __sect_buff.head.next_sect ;
		if( (sect_n != STEEPROM_INVALID_ADDRESS) && (i + 1 != isects) ){
			if( !__read_sector( pPHandler, sect_n, &__sect_buff ) ){
				return FALSE ;
			}
			if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_BODY ){
				return FALSE ;
			}
		}
	}
	// ��������� CRC �� ��� ������
	data_crc = __CRC_EEPROM( pPHandler, pbData, *actual_len );
	return secthead_crc == data_crc ;
}

_BOOL STEEPROM_CheckValue( 	const STEEPROM_PartitionHandler *pPHandler,
							u8* sName, u32* CRC )
{
	u32 sect_n ;
	size_t isects ;
	size_t i ;
	u32 secthead_crc ;

	assert( sName );
	assert( pPHandler );
	assert( CRC );

	__sect_buff.head.name = __sect_name_buff ;
	__sect_buff.head.name[ 0 ] = 0 ;
	
	// ���� ������������
	if( strnlen( (const char*)sName, __STEEPROM_NAMELEN(pPHandler) ) < 0 ){
		return FALSE ;
	}

	if( !__seek_1st_sector( pPHandler, &__sect_buff, sName, &sect_n) ){
		return FALSE ;
	}
	if( sect_n == STEEPROM_INVALID_ADDRESS ){
		return FALSE ;
	}
	secthead_crc = __sect_buff.head.data_crc ;
	if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_HEAD ){
		return FALSE ;
	}

	// ������� �������� �������� ��������
	isects = __sect_buff.head.record_len / STEEPROM_SECTOR_DATA_SIZE ;
	if( __sect_buff.head.record_len % STEEPROM_SECTOR_DATA_SIZE ){
		isects += 1 ;
	}
	*CRC = 0 ;
	// �������� ��� �������
	for( i = 0; i < isects; i++ ){
		*CRC = pPHandler->pHWI->CRC(__sect_buff.data,__sect_buff.head.sector_datalen, *CRC );
		sect_n = __sect_buff.head.next_sect ;
		if( (sect_n != STEEPROM_INVALID_ADDRESS) && (i + 1 != isects) ){
			if( !__read_sector( pPHandler, sect_n, &__sect_buff ) ){
				return FALSE ;
			}
			if( __sect_buff.head.sect_status != STEEPROM_SECTSTAT_BODY ){
				return FALSE ;
			}
		}
	}

	return secthead_crc == *CRC ;
}

void STEEPROM_SpoilPartition( const STEEPROM_PartitionHandler *pPHandler )
{
	const u8 buff[ 32 ] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	if( !STEEPROM_IsPartitionOpened( pPHandler ) ){
		return ;
	}
	
	__WRITE_EEPROM( pPHandler, buff,
 		pPHandler->part_params.partition_begin, sizeof(buff) );		
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ���������� ��������� ��� ���-�������

// ���� � ���-������� �������� �����. �����
u32	__hashtable_seek( const STEEPROM_PartitionHandler *pPHandler, u8* sName )
{
	u32 sect ;
	u8 hash ;
	assert( sName );

	hash = __hash( sName );
	__READ_EEPROM( pPHandler, (u8*)&sect,
		 pPHandler->part_params.partition_begin + STEEPROM_HEAD_SIZE + hash*4, 4 );
	return sect ;
}

// �������� �������� � ���-�������
void __hashtable_change( STEEPROM_PartitionHandler *pPHandler,
												u8* sName, const u32 sect )
{
	u32 hash ;

	hash = __hash( sName );
	__WRITE_EEPROM( pPHandler, (const u8*)&sect,
		pPHandler->part_params.partition_begin + STEEPROM_HEAD_SIZE + hash*4, 4 );
}

// ��������� crc ���-�������
static u32 __hashtable_crc( const STEEPROM_HW_Interface *pHWI, const u32 partition_begin )
{
	u8 buff[256] ;
	size_t i;
	u32 crc ;

	crc = 0 ;
	for( i = 0; i < STEEPROM_HASHTABLE_SIZE - 4; i += sizeof(buff) ){
		if( (i + sizeof(buff)) < (STEEPROM_HASHTABLE_SIZE - 4) ){
			pHWI->ReadArray( buff, partition_begin + STEEPROM_HEAD_SIZE + i,
																sizeof(buff) );
			crc = pHWI->CRC( buff, sizeof(buff), crc );
		} else {
			pHWI->ReadArray( buff, partition_begin + STEEPROM_HEAD_SIZE + i,
											STEEPROM_HASHTABLE_SIZE - 4 - i );
			crc = pHWI->CRC( buff, STEEPROM_HASHTABLE_SIZE - 4 - i, crc );
		}
	}
	return crc ;
}

// ��������� �������� crc ���-�������
static void __renew_hashtable_crc( const STEEPROM_HW_Interface *pHWI, const u32 partition_begin )
{
	u32 crc ;
	crc = __hashtable_crc( pHWI, partition_begin );
	pHWI->WriteArray( (u8*)&crc, partition_begin + STEEPROM_HEAD_SIZE +
									 	STEEPROM_HASHTABLE_SIZE - 4, 4 );	
}

// DJB -- Daniel J. Bernstein
// ��������� 32�-������ ���, �� ���������� ������ ������� 8
static u8 __hash(u8* str)
{
   u32 hash = 5381;
   u32 i    = 0;

   for(i = 0; *str != 0; str++, i++){
      hash = ((hash << 5) + hash) + (*str);
   }

   return hash & 0xFF ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ��� ������� ��������� ��������

// ������������� �����. ��� � �������� val, ������ ���������� � ����
// TRUE -- ��������, FALSE -- �����
void __bitmap_set( u8* bitmap, const size_t sect_num, const _BOOL val )
{
	u8 byte = bitmap[sect_num >> 3] ;
	// ������������� ���
	if( val ){
		byte = byte | (1 << (sect_num & 0x07)) ;
	// ������� �����. ���
	} else {
		byte = byte & ~(1 << (sect_num & 0x07)) ;
	}
	bitmap[sect_num >> 3] = byte ;
}

// ��������� �������� �����. ���� �������, ������� � ����
_BOOL __bitmap_get( const u8* bitmap, const size_t sect_num )
{
	u8 byte = bitmap[sect_num >> 3] ;

	return (byte & (1 << (sect_num & 0x07))) > 0 ;
}

// ���������� ����� ������� �� ������� �������, ����������� 1 (��������)
// ������� � 0
size_t __bitmap_next_free( const u8* bitmap, const size_t total_sectors )
{
	size_t i, j, max_i ;
	max_i = total_sectors >> 3 ;

	for( i = 0; (!bitmap[i]) & (i < max_i); i++ );
	for( j = 0; j < 8; j++ ){
		if( (bitmap[i] >> j) & 1 ){
			return (i << 3) | (j & 0x07) ;
		}
	}

	return STEEPROM_INVALID_ADDRESS ;
}

// ��������� ���-�� ������ ��������
size_t __bitmap_count_free( const u8* bitmap, const size_t total_sectors )
{
	size_t i ;
	size_t max_words, words_remainder ;
	u32 word ;
	size_t accum = 0 ;
	size_t bitmap_bytes = total_sectors >> 3 ;
	max_words = bitmap_bytes >> 2 ; // � ����� 4 �����
	words_remainder = bitmap_bytes & 0x03 ;

	for( i = 0; i < max_words; i++ ){
		word = ((u32*)bitmap)[i] ;
		// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
		for( ; word; accum++ ){
			word &= word - 1 ;
		}
	}
	// �������� ��������
	for( i = 0; i < words_remainder; i++ ){
		word = bitmap[ (bitmap_bytes & 0xFFFFFFFC) + i ] ;
		for( ; word; accum++ ){
			word &= word - 1 ;
		}
	}

	return accum ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static _BOOL	__read_sector( const STEEPROM_PartitionHandler *pPHandler, 
 					const size_t sect_num, __STEEPROM_Sector* buff )
 {
 	u32 crc ;
 	assert( pPHandler );
 	assert( buff );
 	assert( buff->head.name );
 	assert( sect_num < pPHandler->part_params.total_datasectors );

 	// �������� name -- ������ ���������� �� ������� � �������
 	__READ_EEPROM( pPHandler, buff->head.name,
 			__STEEPROM_SECT_BEGIN(pPHandler,sect_num), __STEEPROM_NAMELEN(pPHandler) );
 	// � �������� �� ���������
 	__READ_EEPROM( pPHandler, (u8*)&buff->head.sect_status, 
 		__STEEPROM_SECT_BEGIN(pPHandler,sect_num)+__STEEPROM_NAMELEN(pPHandler),
		pPHandler->part_params.sector_len - __STEEPROM_NAMELEN(pPHandler) );

 	// ��������� crc
 	crc = pPHandler->pHWI->CRC( buff->head.name, __STEEPROM_NAMELEN(pPHandler), 0 );
 	crc = pPHandler->pHWI->CRC( (u8*)&buff->head.sect_status, 
 		pPHandler->part_params.sector_len - __STEEPROM_NAMELEN(pPHandler) -sizeof(u32), crc );

 	return (crc == buff->crc) ;
}

// ������� crc ������� � ����� ���� ������ � EEPROM
void __write_sector( const STEEPROM_PartitionHandler *pPHandler, 
				const size_t sect_num, __STEEPROM_Sector* buff )
{
	u32 crc ;
	assert( buff );
	assert( pPHandler );
	assert( buff->head.name );

	// ��������� CRC
	crc = pPHandler->pHWI->CRC( buff->head.name, __STEEPROM_NAMELEN(pPHandler), 0 );
 	crc = pPHandler->pHWI->CRC( (u8*)&buff->head.sect_status, 
 		pPHandler->part_params.sector_len - __STEEPROM_NAMELEN(pPHandler)-sizeof(u32), crc );
	buff->crc = crc ;

 	__WRITE_EEPROM( pPHandler, buff->head.name,
 		__STEEPROM_SECT_BEGIN(pPHandler,sect_num), __STEEPROM_NAMELEN(pPHandler) );
 	__WRITE_EEPROM( pPHandler, (u8*)&(buff->head.sect_status),
 		__STEEPROM_SECT_BEGIN(pPHandler,sect_num) + __STEEPROM_NAMELEN(pPHandler),
 			pPHandler->part_params.sector_len - __STEEPROM_NAMELEN(pPHandler) );
}

// ���� ������ ������ ���������, ���������� TRUE ���� �� �������� �������, � sect_n ����� ����� ������� ��� 0 ���� �� �������
_BOOL __seek_1st_sector( const STEEPROM_PartitionHandler *pPHandler, __STEEPROM_Sector *sect,
					 u8* sName, size_t *sect_n )
{
	size_t first_hash_sect ;
	size_t sect_n_next ;
	u32 cmp_ret ;

	assert( sect_n );
	assert( sect );
	assert( sect->head.name );
	assert( sName );

	// ��������� ������� ������ ���������
	*sect_n = __hashtable_seek( pPHandler, sName );
	// ���� � ���-������� ��� ������ -- ������� �����
	if( *sect_n == STEEPROM_INVALID_ADDRESS ){
		*sect_n = STEEPROM_INVALID_ADDRESS ;
		return TRUE ;
	}
	// �������� �� �������� ������ ���� �� ����� ������ ���
	first_hash_sect = *sect_n ;
	sect_n_next = *sect_n ;
	do {
		*sect_n = sect_n_next ;
		// ���� ������ CRC ������� �����
		if( !__read_sector( pPHandler, *sect_n, sect )){
			return FALSE ;
		}
		if( sect->head.sect_status != STEEPROM_SECTSTAT_HEAD){
			return FALSE ;
		}
		sect_n_next = sect->head.next_hash_sect ;
	} while(			
		( (cmp_ret = strncmp( (const char*)sName, (const char*)sect->head.name, STEEPROM_MAXNAMELEN)) != 0 ) &&
		( sect->head.next_hash_sect != first_hash_sect ));

	// �� �������
	if( cmp_ret != 0 ){
		*sect_n = STEEPROM_INVALID_ADDRESS ;
	}
	// ���� cmp_ret == 0, �� *sect_n ��� ����� ������ ��������
	return TRUE ;
}
