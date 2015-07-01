/*!
\file T8_Dnepr_filesystem.c
\brief  од дл€ св€зи yaffs с железом днепра и профилем
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/	

#include "yaffs_guts.h"
#include "yaffsfs.h"
#include "yaffs_ecc.h"
#include "yaffs_trace.h"
#include "yaffscfg.h"
#include "yportenv.h"
#include "uCOS_II.H"

#include <stdlib.h>

#include <errno.h>

#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "T8_Atomic_heap.h"

#include "HAL/IC/inc/at45db321d.h"
#include "HAL/BSP/inc/T8_Dnepr_filesystem.h"

#include "common_lib/memory.h"

void yaffsfs_LockInit(void);

unsigned yaffs_trace_mask = 0 ;

static struct yaffs_dev __dnepr_flash_dev ;

static int __WriteChunk(struct yaffs_dev *dev, int nand_chunk,const u8 *data,
									int data_len, const u8 *oob, int oob_len);
static int __ReadChunk(struct yaffs_dev *dev, int nand_chunk,u8 *data,
		int data_len, u8 *oob, int oob_len, enum yaffs_ecc_result *ecc_result);
static int __EraseBlock(struct yaffs_dev *dev, int block_no);
static int __MarkBad(struct yaffs_dev *dev, int block_no);
static int __CheckBad(struct yaffs_dev *dev, int block_no);
static int __Initialise(struct yaffs_dev *dev);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PAGES_PER_BLOCK 4 		// в блоке 4 страницы YAFFS
#define PAGE_SIZE 		1056	// 1 страница YAFFS -- 2 страницы flash
#define TAG_SIZE		16 		// в YAFFS тэги занимают 16 байт + ECC на них 9 байт + выравнивыние
#define PAGE_ECC_LEN	15 		// 3 байта ECC на 256 байт, данных ceil((1024 - PAGE_ECC_LEN)/256)*3 = 12 + 3 байта на tags
#define TOTAL_BLOCKS	1024

//! —амый последний байт, в котором храним сигнатуру битого сектора.
#define PAGE_FAILURE_BYTE_POS (PAGE_SIZE - AT_AT45DB321D_SECTOR_LEN - 1)
//! — какого места храним тэги страницы.
#define PAGE_TAG_POS	(PAGE_SIZE - AT_AT45DB321D_SECTOR_LEN - PAGE_ECC_LEN - TAG_SIZE - 1)
//! — какого места храним ECC.
#define PAGE_ECC_POS	(PAGE_SIZE - AT_AT45DB321D_SECTOR_LEN - PAGE_ECC_LEN - 1)

#define PAGE_DATA_SIZE 	(PAGE_SIZE -  PAGE_ECC_LEN - 1 - TAG_SIZE)

// буфер нужен дл€ вычислени€ ECC на массивах, не кратных 256
static u8 __cbuff256[ 256 ];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_filesystem_Init(void)
{
	// enum yaffs_ecc_result ecc_result ;
	// _BOOL ret ;
	// s32 hw_file ;
	// size_t i, j ;
	// size_t iterations ;
	// size_t start_time, end_time ;
	// size_t total_time = 0 ;
	t8_memzero( (u8*)__cbuff256, sizeof(__cbuff256) );

	t8_memzero( (u8*)&__dnepr_flash_dev, sizeof(__dnepr_flash_dev) );

	__dnepr_flash_dev.param.name = DNEPR_FLASH_ADDR ;

	__dnepr_flash_dev.param.inband_tags = 0 ;
	__dnepr_flash_dev.param.total_bytes_per_chunk = PAGE_DATA_SIZE ;
	__dnepr_flash_dev.param.chunks_per_block = PAGES_PER_BLOCK ;
	__dnepr_flash_dev.param.start_block = 0 ;
	__dnepr_flash_dev.param.end_block = TOTAL_BLOCKS-1 ;
	__dnepr_flash_dev.param.is_yaffs2 = 1 ;
	__dnepr_flash_dev.param.use_nand_ecc = 0 ;
	__dnepr_flash_dev.param.no_tags_ecc = 1 ;
	__dnepr_flash_dev.param.n_reserved_blocks = 5 ;
	__dnepr_flash_dev.param.wide_tnodes_disabled = 0 ;
	__dnepr_flash_dev.param.refresh_period = 1000 ;
	__dnepr_flash_dev.param.n_caches = 0 ;
	__dnepr_flash_dev.param.enable_xattr = 0 ;

	__dnepr_flash_dev.drv.drv_write_chunk_fn = 	__WriteChunk ;
	__dnepr_flash_dev.drv.drv_read_chunk_fn = 	__ReadChunk ;
	__dnepr_flash_dev.drv.drv_erase_fn = 		__EraseBlock ;
	__dnepr_flash_dev.drv.drv_mark_bad_fn = 	__MarkBad ;
	__dnepr_flash_dev.drv.drv_check_bad_fn = 	__CheckBad ;
	__dnepr_flash_dev.drv.drv_initialise_fn = 	__Initialise ;	

	yaffs_add_device( &__dnepr_flash_dev );

	yaffs_start_up() ;

	// // стираем
	// __EraseBlock( NULL, 0 + BASE/PAGES_PER_BLOCK );
	// memset( test_array, 0xAA, sizeof(test_array) );
	// memset( test_oob_ar, 0xAA, sizeof(test_oob_ar) );
	// for( i = 0; i < 4; ++i ){
	// 	__ReadChunk( NULL, i + BASE, test_array, sizeof(test_array),
	// 							test_oob_ar, sizeof(test_oob_ar),
	// 							&ecc_result );
	// }
	// пишем
	// for( i = 0; i < 1024; i++ ){
	// 	test_array[ i ] = i ;
	// }
	// test_array[ 1 ] = 0xB5 ;
	// test_array[ 2 ] = 0xA5 ;
	// test_array[ 3 ] = 0xC5 ;
	// test_array[ 4 ] = 0xA5 ;
	// for( i = 0; i < 16; i++ ){
	// 	test_oob_ar[ i ] = i ^ 0xFF ;
	// }
	// // sec 1
	// __WriteChunk( NULL, 0 + BASE, test_array, sizeof(test_array),	
	// 						test_oob_ar, sizeof(test_oob_ar) );
	// for( i = 0; i < 4; ++i ){
	// 	u8 cmp_res ;
	// 	__ReadChunk( NULL, i + BASE, test_array_2, sizeof(test_array_2),
	// 							test_oob_ar_2, sizeof(test_oob_ar_2),
	// 							&ecc_result );
	// 	cmp_res = memcmp( test_array, test_array_2, sizeof(test_array_2) );
	// 	cmp_res = memcmp( test_oob_ar, test_oob_ar_2, sizeof(test_oob_ar_2) );
	// 	memset( test_array_2, 0xAA, sizeof(test_array_2) );
	// 	memset( test_oob_ar_2, 0xAA, sizeof(test_oob_ar_2) );
	// }
	// // sec 2
	// __WriteChunk( NULL, 2 + BASE, test_array, sizeof(test_array),	
	// 						test_oob_ar, sizeof(test_oob_ar) );
	// for( i = 0; i < 4; ++i ){
	// 	u8 cmp_res ;
	// 	__ReadChunk( NULL, i + BASE, test_array_2, sizeof(test_array_2),
	// 							test_oob_ar_2, sizeof(test_oob_ar_2),
	// 							&ecc_result );
	// 	cmp_res = memcmp( test_array, test_array_2, sizeof(test_array_2) );
	// 	cmp_res = memcmp( test_oob_ar_2, test_oob_ar_2, sizeof(test_oob_ar_2) );
	// 	memset( test_array_2, 0xAA, sizeof(test_array_2) );
	// 	memset( test_oob_ar_2, 0xAA, sizeof(test_oob_ar_2) );
	// }

	// while(1){}
	// for( i = 0; i < 4; ++i ){
	// 	__ReadChunk( NULL, i+9*4, test_array, sizeof(test_array),
	// 							test_oob_ar, sizeof(test_oob_ar),
	// 							NULL );
	// 	memset( test_array, 0xAA, sizeof(test_array) );
	// 	memset( test_oob_ar, 0xAA, sizeof(test_oob_ar) );
	// }
	// while(TRUE);

	// yaffs_format( DNEPR_FLASH_ROOT, 0, 0, 1 );
        
	if( yaffs_mount( DNEPR_FLASH_ROOT ) < 0 )
        {
		yaffs_format( DNEPR_FLASH_ROOT, 0, 0, 1 );
	}
	// i = 0 ;
	// j = 0 ;
	// if( (hw_file = yaffs_open(DNEPR_FLASH_ROOT "/hw1.txt", O_RDWR | O_CREAT, S_IREAD|S_IWRITE )) >= 0 ){
	// 	ret = yaffs_read( hw_file, test_array_2, sizeof(test_array_2) );
	// 	yaffs_lseek( hw_file, 0, SEEK_SET );
	// 	if( ret == 0 )
	// 		i = memcmp( test_array, test_array_2, sizeof(test_array_2) ) ;

	// 	iterations = 0 ;
	// 	while(TRUE){
	// 		start_time = llUptime ;
	// 		iterations++ ;
	// 		// for( i = 0; i < 1024; i += 4 ){
	// 		// 	test_array[ i ] = rand() & 0xFF ;
	// 		// 	test_array[ i + 1 ] = rand() & 0xFF  ;
	// 		// 	test_array[ i + 2 ] = rand() & 0xFF  ;
	// 		// 	test_array[ i + 3 ] = rand() & 0xFF  ;
	// 		// 	j++ ;
	// 		// }
	// 		ret = yaffs_write( hw_file, test_array, sizeof(test_array) );
	// 		yaffs_lseek( hw_file, 0, SEEK_SET );
	// 		memset( test_array_2, 0xAA, sizeof(test_array_2) );
	// 		ret = yaffs_read( hw_file, test_array_2, sizeof(test_array_2) );
	// 		yaffs_lseek( hw_file, 0, SEEK_SET );
	// 		end_time = llUptime ;
	// 		total_time += end_time - start_time ;
	// 		i = memcmp( test_array, test_array_2, sizeof(test_array_2) ) ;
	// 		assert( i == 0 );
	// 	}
	// }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL Dnepr_filesystem_Write( u8* sName, u8* pbData, const size_t actual_len )
{
	u8 filename[ DNEPR_FLASH_NAMELEN*2 ];
	s32 hw_file ;
	_BOOL success = TRUE ;

	filename[ 0 ] = 0 ;
	// к имени параметра добавл€ем путь к корню
	sprintf( (s8*)filename, "%s%s", DNEPR_FLASH_ROOT, sName );

	// открываем файл и, если он существует, переписывем всЄ содержимое
	if( (hw_file = yaffs_open( (const s8*)filename, O_RDWR | O_CREAT | O_TRUNC, S_IWRITE | S_IREAD )) < 0 ){
		success = FALSE ;
		goto _ERROR ;
	}

	if( yaffs_write( hw_file, pbData, actual_len ) < 0 ){
		success = FALSE ;
		goto _ERROR ;
	}

	success = TRUE ;

_ERROR:
	yaffs_close( hw_file ) ;
	return success ;
}

_BOOL Dnepr_filesystem_Read( u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen )
{
	u8 filename[ DNEPR_FLASH_NAMELEN*2 ];
	s32 hw_file ;
	_BOOL success = TRUE ;
	s32 total_read ;

	filename[ 0 ] = 0 ;
	// к имени параметра добавл€ем путь к корню
	sprintf( (s8*)filename, "%s%s", DNEPR_FLASH_ROOT, sName );

	*actual_len = 0 ;

	if( (hw_file = yaffs_open( (const s8*)filename, O_RDWR, S_IWRITE | S_IREAD )) < 0 ){
		success = FALSE ;
		goto _ERROR ;
	}

	if( (total_read = yaffs_read( hw_file, pbData, maxlen )) < 0 ){
		success = FALSE ;
		goto _ERROR ;
	}

	success = TRUE ;
	*actual_len = (size_t)total_read ;

_ERROR:
	yaffs_close( hw_file ) ;
	return success ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// драйвер флеш дл€ YAFFS

int yaffs_start_up(void)
{
	/* Call the OS initialisation (eg. set up lock semaphore */
	yaffsfs_OSInitialisation();

	return 0;
}

void yaffsfs_OSInitialisation(void)
{
	yaffsfs_LockInit();
}

/*
 * yaffsfs_CurrentTime() retrns a 32-bit timestamp.
 * 
 * Can return 0 if your system does not care about time.
 */
 
u32 yaffsfs_CurrentTime(void)
{
	return 0 ;
}


void yaffsfs_Lock(void)
{
	T8_Dnepr_TS_FFS_Lock() ;
}

void yaffsfs_Unlock(void)
{
	T8_Dnepr_TS_FFS_Unlock() ;
}

void yaffsfs_LockInit(void)
{
}

/*
 * yaffsfs_CheckMemRegion()
 * Check that access to an address is valid.
 * This can check memory is in bounds and is writable etc.
 *
 * Returns 0 if ok, negative if not.
 */
int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
	if(!addr)
		return -1;
	return 0;
}

/*
 * yaffsfs_SetError() and yaffsfs_GetError()
 * Do whatever to set the system error.
 * yaffsfs_GetError() just fetches the last error.
 */

static int yaffsfs_lastError;

void yaffsfs_SetError(int err)
{
	//Do whatever to set error
	yaffsfs_lastError = err;
	errno = err;
}

int yaffsfs_GetLastError(void)
{
	return yaffsfs_lastError;
}

//! “екущий размер очереди.
static size_t __yaffs_heap_allocated = 0 ;
//! ћаксимальный размер очереди.
static size_t __yaffs_max_heap = 0 ;

void *yaffsfs_malloc(size_t size)
{
	// перед данными отводим 32 бита на размер этих данных
	void * p = npalloc( size );
	if( p ){
		__yaffs_heap_allocated += size + 4 ; // 32 бита использует allocator чтобы хранить размер блока
		if( __yaffs_max_heap < __yaffs_heap_allocated ){
			__yaffs_max_heap = __yaffs_heap_allocated ;
		}
	}
	return p ;
}

void yaffsfs_free(void *ptr)
{
	void *p = (void*)((u8*)ptr - 4) ;
	size_t size = *(size_t*)p ;
	__yaffs_heap_allocated -= size ;

	npfree( ptr );
}

/*
 * yaffs_bug_fn()
 * Function to report a bug.
 */
 
void yaffs_bug_fn(const char *file_name, int line_no)
{
	assert(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// стирание длитс€ 35.4 мс
// запись буфера в флеш длитс€ 3.5 мс
// чтение страницы (запись страницы в буфер) длитс€ 1.18 мс

static void __wait_2_write_msecs( const u8 _1st_pause, const u8 other_pauses )
{
	if( _1st_pause ){
		OSTimeDly( _1st_pause );
	}
	while( !AT_AT45DB321D_ReadyToWrite() ){
		OSTimeDly( other_pauses );
	}
}

size_t __write_cnt = 0 ;

// ѕишет 1 сектор ‘— -- это 2 сектора flash-пам€ти. » spare-область
static int __WriteChunk(struct yaffs_dev *dev, int nand_chunk,
				   const u8 *data, int data_len,
				   const u8 *oob, int oob_len)
{
	size_t to_write1, to_write2 ;
	size_t i ;
	u8 ecc[ 15 ]; // 3 байта на 256 данных (12 итого) + 3 байта на tag'и
	(void) dev;

	assert( data_len <= PAGE_DATA_SIZE );
	assert( oob_len <= TAG_SIZE );

	if (!data || !data_len) {
		return YAFFS_OK ;
	}

	__write_cnt++ ;
	// ждЄм окончани€ предыдущей записи
	__wait_2_write_msecs( 0, 1 );
	
	to_write1 = MIN( data_len, AT_AT45DB321D_SECTOR_LEN );
	AT_AT45DB321D_WriteBuffer1( 0, (u8*)data, to_write1 );
	to_write2 = data_len - to_write1 ;
	if( to_write2 ){
		AT_AT45DB321D_WriteBuffer2( 0, (u8*)data+to_write1, to_write2 );
	}

	// spare область -- 16 байт сразу после данных
	if( oob && oob_len ){
		AT_AT45DB321D_WriteBuffer2( PAGE_TAG_POS, (u8*)oob, oob_len );
	}

	// вычисл€ем ECC на данных
	for( i = 0 ; i < data_len; i += 256 ){
		yaffs_ecc_calc( data + i, &ecc[(i >> 8)*3] );
	}

	// вычисл€ем ecc на tag'и
	// __cbuff256 все старшие незадействованные байты вплоть до 256го зан€ты нул€ми
	t8_memcopy( __cbuff256, (u8*)oob, oob_len );
	yaffs_ecc_calc( __cbuff256, &ecc[(i >> 8)*3] );

	// пишем ECC -- 15 байт после OOB
	AT_AT45DB321D_WriteBuffer2( PAGE_ECC_POS, ecc, PAGE_ECC_LEN );

	AT_AT45DB321D_ProgrammBuffer1( nand_chunk << 11 );
	__wait_2_write_msecs( 3, 1 );
	AT_AT45DB321D_ProgrammBuffer2( (nand_chunk << 11) + (1 << 10) );

	return YAFFS_OK ;
}

size_t __ecc_no_errs = 0 ;
size_t __ecc_fixed_errs = 0 ;
size_t __ecc_unfixed_errs = 0 ;

// ждЄт окончани€ записи или стирани€ и читает данные из флеш
// TODO: надо бы позволить читать данные из блока, в который сейчас не пишем
static int __ReadChunk(struct yaffs_dev *dev, int nand_chunk,
				   u8 *data, int data_len,
				   u8 *oob, int oob_len,
				   enum yaffs_ecc_result *ecc_result)
{
	size_t to_read ;
	size_t data_index = 0;
	u8 ecc_read[ 15 ];
	size_t i ;
	u8 ecc_test[ 3 ];
	s32 ecc_result_calc = 0 ;

	(void) dev;

	assert( data_len <= PAGE_DATA_SIZE );
	assert( oob_len <= TAG_SIZE );

	// ждЄм окончани€ записи
	__wait_2_write_msecs( 0, 1 );

	// читаем ECC
	AT_AT45DB321D_DirectReadBytes( (nand_chunk << 11)  + (1 << 10) + PAGE_ECC_POS, ecc_read, PAGE_ECC_LEN );
	
	if (data && data_len) {

		to_read = MIN( data_len, AT_AT45DB321D_SECTOR_LEN );
		AT_AT45DB321D_DirectReadBytes( nand_chunk << 11, data + data_index, to_read );
		to_read = MIN( data_len - to_read, AT_AT45DB321D_SECTOR_LEN-32 );
		AT_AT45DB321D_DirectReadBytes( (nand_chunk << 11) + (1 << 10), data + AT_AT45DB321D_SECTOR_LEN, to_read );
		//!!!!!!!!
		#ifdef CHECK_ECC
		data[500] |= 0x40 ;
		#endif
		//!!!!!!!!
		// провер€ем
		for( i = 0 ; i < data_len; i += 256 ){
			yaffs_ecc_calc( data + i, ecc_test );
			ecc_result_calc |= yaffs_ecc_correct( data + i, &ecc_read[(i >> 8)*3], ecc_test );
		}
	}
	if( oob ){
		i = 1024 ;
		AT_AT45DB321D_DirectReadBytes( (nand_chunk << 11)  + (1 << 10) + PAGE_TAG_POS, oob, oob_len );
		// //!!!!!!!!
		#ifdef CHECK_ECC
		oob[5] ^= 0x80 ;
		#endif
		// //!!!!!!!!
		t8_memcopy( __cbuff256, oob, oob_len );
		yaffs_ecc_calc( __cbuff256, ecc_test );
		ecc_result_calc |= yaffs_ecc_correct( __cbuff256, &ecc_read[(i >> 8)*3], ecc_test );
		t8_memcopy( oob, __cbuff256, oob_len );
	}

	if( !ecc_result ){
	} else if( ecc_result_calc == 0 ){
		__ecc_no_errs++ ;
		*ecc_result = YAFFS_ECC_RESULT_NO_ERROR ;
	} else if( ecc_result_calc == 1 ){
		__ecc_fixed_errs++ ;
		*ecc_result = YAFFS_ECC_RESULT_FIXED ;
	} else if( ecc_result_calc == -1 ){
		__ecc_unfixed_errs++ ;
		*ecc_result = YAFFS_ECC_RESULT_UNFIXED ;
	}

	if( ecc_result_calc != 0 ){
		i++ ;
		i-- ;
	}

	return YAFFS_OK ;
}

// стирает блок в flash
static int __EraseBlock(struct yaffs_dev *dev, int block_no)
{
	assert((block_no >= 0) && (block_no < TOTAL_BLOCKS));

	// ждЄм окончани€ записи
	__wait_2_write_msecs( 0, 1 );
	AT_AT45DB321D_ErraseBlock( block_no ) ;

	return YAFFS_OK ;
}

// помечает весь блок плохим, записыва€ в spare область первого сектора Y 
// в последнем байте
static int __MarkBad(struct yaffs_dev *dev, int block_no)
{
	u8 bad_mark = 'Y' ;
	
	__EraseBlock( dev, block_no );
	__wait_2_write_msecs( 20, 5 );

	AT_AT45DB321D_WriteBuffer1( PAGE_FAILURE_BYTE_POS, &bad_mark, 1 );
	__wait_2_write_msecs( 2, 1 );
	AT_AT45DB321D_ProgrammBuffer1( (block_no*PAGES_PER_BLOCK + 1) << 10 );

	return YAFFS_OK ;
}

static int __CheckBad(struct yaffs_dev *dev, int block_no)
{
	u8 block_status1 ;

	AT_AT45DB321D_DirectReadBytes( (u32)((u32)(block_no*dev->param.chunks_per_block + 1) << 10) + (u32)PAGE_FAILURE_BYTE_POS,
													&block_status1, 1 );

	if( block_status1 == 'Y' ){
		return YAFFS_FAIL ;
	} else {
		return YAFFS_OK ;
	}
}

static int __Initialise(struct yaffs_dev *dev)
{
	return YAFFS_OK ;
}
