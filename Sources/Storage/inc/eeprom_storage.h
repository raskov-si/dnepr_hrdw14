/*!
\file eeprom_storage.h
\brief Нежурналируемое хранилище в EEPROM с адресацией по строке
\details Отдельно хранит данные в секторах фиксированного размера и индексные
\details структуры с именами записей и адресами секторов. Журналирование д.б.
\details реализовано выше уровнем за счет дублирования раздела.
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

//! Версия структуры данных в EEPROM. При добавлении новых полей с сохраненнием 
//! обратной совместимости, добавляем биты, сохраняя текущие.
#define		STEEPROM_VERSION_BASE	0x00001000

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Структура данных в разделе
// *********************************************************************************************************************
//	Head (всего STEEPROM_HEAD_SIZE байт):
//		1. STEEPROM_PartDescriptor
//		2. Filling Zeros
//		3. CRC32
// *********************************************************************************************************************
//	HashTable (всего STEEPROM_HASHTABLE_SIZE байт):
//		1. HashTable
//		2. CRC32
// *********************************************************************************************************************
//	DataSectors (всего sizeof(SectorHead)+ STEEPROM_MAXNAMELEN + STEEPROM_SECTOR_DATA_SIZE + 4 байта CRC):
//		1. SectorHead
//		2. Data
//		3. CRC32 -- проверяется при включении, чтобы не держать список всех параметров
// *********************************************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! длина заголовка раздела, в сумме с HASHTABLE_SIZE хорошо бы что бы была 
//! степень 2м, влияет на STEEPROM_VERSION_BASE
#define 	STEEPROM_HEAD_SIZE					252
//! размер хэш-таблицы с CRC32, влияет на STEEPROM_VERSION_BASE
#define 	STEEPROM_HASHTABLE_SIZE				1028

//! сколько данных вмещает один сектор, влияет на STEEPROM_VERSION_BASE
#define 	STEEPROM_SECTOR_DATA_SIZE			16
//! максимальное число секторов, это число не влияет на версию STEEPROM_VERSION_BASE,
//! но влияет на объём ОЗУ, занимаемый STEEPROM_PartitionHandler
#define		STEEPROM_MAX_SECTORS				1024 // битмап займёт 128 байт

//! сколько выделять памяти под массив с именем записи, не влияет на STEEPROM_VERSION_BASE
#define 	STEEPROM_MAXNAMELEN					128
//! размер максимально возможной транзакции по SPI с EEPROM, не влияет на STEEPROM_VERSION_BASE
#define		STEEPROM_MAX_EEPROMTRANSACTION		256

//! Дескриптор раздела, находится в самом начале раздела EEPROM (с 0го байта),
//! описывает сам раздел
#pragma pack( push )
#pragma pack( 1 )
typedef struct __STEEPROM_PartDescriptor
{
	u32 version ;			//!< версия структур данных

	u32 partition_begin ; 	//!< адрес первого байта раздела
	u32 partition_end ;		//!< адрес последнего байта раздела

	//! кол-во байт в имени записи (STEEPROM_SectorHead::name)
	u8	record_namelen ;

	u32 total_datasectors ; //!< общее количество секторов с данными в разделе
	u32 sector_len ;		//!< длина сектора
} STEEPROM_PartDescriptor ;
#pragma pack( pop )

//! Заголовок сектора
#pragma pack( push )
#pragma pack( 1 )
typedef struct __STEEPROM_SectorHead {
	//! строка с именем записи (в EEPROM вместо этого указателя
	//! выделяются PartDescriptor::record_namelen байт)
	u8* name ;

	u32 sect_status ; //!< статус сектора см. enum STEEPROM_SectorStatus
	u32 data_crc ; //!< контрольная сумма на все данные
	//! адрес 1го сектора записи с таким же значением _hash( name )
	u32 next_hash_sect ;
	//! адрес 1го сектора предыдущей (или последней в связном списке) записи
	//! с таким же хэшем от name
	u32 prev_hash_sect ;

	u16 record_len ; 		//!< кол-во байт данных во всей записи
	u16 sector_datalen ;	//!< кол-во байт данных из этого сектора актуальны
	u32 next_sect ; 		//!< адрес следующего дескриптора сектора для этой записи 
} STEEPROM_SectorHead ;
#pragma pack( pop )

//! значение поля sect_status в 
typedef enum {
	STEEPROM_SECTSTAT_FREE 	= 0x00000000 ,	//!< сектор свободен
	STEEPROM_SECTSTAT_HEAD 	= 0x00000010 ,	//!< первый сектор записи
	STEEPROM_SECTSTAT_BODY 	= 0x00000020 ,	//!< не первый сектор записи
	STEEPROM_SECTSTAT_INVALID = 0xFFFFFFFF	//!< заголовок сектора неверный
} STEEPROM_SectorStatus ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! Интерфейс для ввода-вывода в EEPROM. Макисмальный размер массива не должен быть меньше STEEPROM_MAX_EEPROMTRANSACTION
typedef struct __STEEPROM_HW_Interface
{
	void (*ReadArray)( u8* dest, const size_t address, const size_t length ); //!< читает соотв. массив
	void (*WriteArray)( const u8* src, const size_t address, const size_t length ); //!< пишет соотв. массив

	//! процедура вычисляет CRC и возвращает 32 бита, здесь -- на случай если
	//! в другом проекте будет другой полином
	u32 (*CRC)(u8* buf, u32 len, u32 prev_crc );
} STEEPROM_HW_Interface;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! Хендлер раздела в EEPROM описывает раздел и хранит в себе указатель на
//! STEEPROM_HW_Interface и пр.
typedef struct __STEEPROM_PartitionHandler
{
	STEEPROM_HW_Interface *pHWI ;
	_BOOL	opened ;

	struct __STEEPROM_PartDescriptor part_params ; // читаем при открытии раздела

	// битмап занятости секторов, чтобы не проходить каждый раз сектора в EEPROM
	// заполняется при открытии, потом поддерживается актуальным
	u8 freesector_bitmap[ STEEPROM_MAX_SECTORS/8 ];
} STEEPROM_PartitionHandler ;

/*!
\brief Читает дескриптор раздела из EEPROM
\details Вызывается перед началом работы с разделом. EEPROM должен быть достпуен иначе процедура не вернёт управление.
\param pPHandler указатель на структуру, при успешном открытии возвращаются параметры открытого раздела
\param begin_address номер первого байта открываемого раздела
\param tHWInterface указатель на заполненную структуру интерфейса
\retval FALSE если раздел не отформатирован
*/
_BOOL STEEPROM_OpenPartition( 	STEEPROM_PartitionHandler *pPHandler,
								const size_t begin_address,
								STEEPROM_HW_Interface* tHWInterface );
_BOOL STEEPROM_IsPartitionOpened( const STEEPROM_PartitionHandler *pPHandler );

//! Параметры форматирования хранилища
typedef struct __STEEPROM_FormatParams
{
	size_t	begin_address ;		//!< адрес начала раздела
	size_t	end_address;		//!< адрес конца раздела

	size_t	record_namelen	; 	//!< максимальная длина имени записи
	size_t	total_datasectors ;	//!< максимальное количество секторов с данными, д.б. кратно 8
} STEEPROM_FormatParams ;

//! Производит форматирование eeprom. После форматирования раздел необходимо открыть.
void STEEPROM_Format( const STEEPROM_HW_Interface *pHWI, const STEEPROM_FormatParams* formatParams ) ;

// TODO: проверять так же все данные
//! Проверяет crc всех секторов
//! \retval TRUE все секторы верные
_BOOL STEEPROM_CheckPartition( const STEEPROM_PartitionHandler *pPHandler );

/*!
\brief Производит добавление/перезапись соотв. записи в EEPROM
\param pPHandler указатель на открытый хендлер раздела
\param sName строка с именем записи, не больше STEEPROM_FormatParams.record_namelen
\param pbData бинарные данные с записью. Могут быть длиннее сектора
\param len Длина массива sData.
\retval FALSE если не проходит верификация или раздел не открыт
*/
_BOOL STEEPROM_WriteRecord( STEEPROM_PartitionHandler *pPHandler, 
							u8* sName, u8* pbData, const size_t len );

/*!
\brief Ищет запись с соотв. именем и отдаёт её содержимое.
\param pPHandler указатель на структуру хендлера открытого раздела
\param sName имя записи
\param pbData куда пишутся данные
\param actual_len сколько байт записано в параметре, 0 если параметр не найден
\param maxlen длина массива pbData
\retval FALSE если данные не целостны или раздел не открыт
*/
_BOOL STEEPROM_ReadRecord( 	const STEEPROM_PartitionHandler *pPHandler,
							u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen );

//! \brief находит параметр, проверяет все CRC и возвращает CRC на данные
_BOOL STEEPROM_CheckValue( 	const STEEPROM_PartitionHandler *pPHandler,
							u8* sName, u32* CRC );

void STEEPROM_SpoilPartition( const STEEPROM_PartitionHandler *pPHandler );

/*!
\}
*/ // EEPROM_Storage

// внутренние процедуры, светят наружу только для тестов
#ifdef __STEEPROM_INTERNALS

// возвращает __bitmap_next_free когда не осталось свободных секторов
#define STEEPROM_INVALID_ADDRESS		0xFFFFFFFF

u32		__hashtable_seek( const STEEPROM_PartitionHandler *pPHandler, u8* sName );
void 	__hashtable_change( STEEPROM_PartitionHandler *pPHandler,
												u8* sName, const u32 sect );

void 	__bitmap_set( u8* bitmap, const size_t sect_num, const _BOOL val );
_BOOL 	__bitmap_get( const u8* bitmap, const size_t sect_num );
size_t __bitmap_next_free( const u8* bitmap, const size_t total_sectors );
size_t 	__bitmap_count_free( const u8* bitmap, const size_t total_sectors );


//! структура для всего сектора целиком
typedef struct ___STEEPROM_SECTOR {
	STEEPROM_SectorHead head ;
	u8 data[ STEEPROM_SECTOR_DATA_SIZE ];
	u32 crc ;
} __STEEPROM_Sector ;

//! ищем первый сектор параметра
//! \param pPHandler хендлер раздела
//! \param sect куда пишется сам первый сектор, имеет смысл если sect != STEEPROM_INVALID_ADDRESS
//! \param sName имя параметра
//! \param sect_n куда кладётся номер сектора, если параметр не найден пишется STEEPROM_INVALID_ADDRESS
//! \retval TRUE если не выявлено неконсистентности структуры данных
_BOOL __seek_1st_sector( const STEEPROM_PartitionHandler *pPHandler, __STEEPROM_Sector *sect,
					 u8* sName, size_t *sect_n );

void __write_sector( const STEEPROM_PartitionHandler *pPHandler, 
				const size_t sect_num, __STEEPROM_Sector* buff );

#endif // __STEEPROM_INTERNALS

#endif
