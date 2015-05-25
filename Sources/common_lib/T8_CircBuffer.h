/*!
\file T8_CircBuffer.h
\brief circular buffer with static memory allocation first/last packet
 prioritisation. Copypasted from Neva's circ_buffer 
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date may 2012
*/

#ifndef		__CIRCBUFFER_H
#define		__CIRCBUFFER_H

#include "ErrorManagement\status_codes.h"
#include "support_common.h" // для типов данных

typedef		u8	CircBuffer_datatype ;

////////////////////////////////////////////////////////////////////////////////
//! Дескриптор кольцевого буфера
typedef struct 
{
	//! указатель на память с данными
	CircBuffer_datatype*	data ; 
	//! суммарный размер хранилища данных, очень хорошо бы делать кратным 4
	const size_t			szTotal ;	
	//! текущее количество хранящихся элементов (CircBuffer_datatype)
	size_t					szActual ;
	//! кол-во затёртых элементов в рез-те переполнения
	size_t					szLostBytes ; 
	//! номер первого принятого элемента в хранилище
	size_t					iHead ;
	//! номер следующего за последним принятым элементом в хранилище
	size_t					iTail ;
        uint8_t                                 push_lock;
        uint8_t                                 pop_lock;
} T8_CircBuffer ;

//! определяет кольцевой буфер в коде (м.б. в стеке или глобально)
// грязный хак с преобразованием типов сделан для того, чтобы применять t8_memcopy32
#define		CIRCBUFFER(name, size) u8 name##_array_[size];\
		T8_CircBuffer name = { (CircBuffer_datatype*)name##_array_, \
		size, 0, 0, 0, 0, 0, 0}

////////////////////////////////////////////////////////////////////////////////

//! \brief	атомарно записывает элемент в кольцевой буффер, трёт старые данные,
//! 		приоритет новых данных
//! \param	desc_	дескриптор кольцевого буфера
//! \param	data_	сами данные
//! \retval	OK если ничего не затёрли, иначе ERROR
ReturnStatus CircBuffer_push_one_erasingdata( T8_CircBuffer* const desc_, 
				const CircBuffer_datatype data_ );
//! \brief	атомарно записывает элемент в кольцевой буффер, не даёт затереть  
//! 	старые данные, приоритет старых данных
//! \param	desc_	дескриптор кольцевого буфера
//! \param	data_	данные
//! \retval	OK если поместилось и записано, иначе ERROR
ReturnStatus CircBuffer_push_one_tender( T8_CircBuffer * const desc_, 
				const CircBuffer_datatype data_ );

//! \brief считывает все данные из буфера, хранящиеся в нём в момент вызова,
//! может выполняться параллельно с каким-нибудь push
//! \param desc_ дескриптор кольцевого буфера
//! \param dest_ куда пишутся данные
//! \param szDest_ максимальный размер записываемых данных
//! \param szActual_read_ сколько записали в dest_
//! \retval	OK в процессе чтения буфер не переполнился,
//! 	ERROR -- буфер переполнился, прочитанные данные испорчены
ReturnStatus CircBuffer_read( T8_CircBuffer * const desc_,
				CircBuffer_datatype* const dest_, const size_t szDest_,
				 size_t* const szActual_read_ );

//! \brief размер хранилища буфера
size_t	CircBuffer_total_size( const T8_CircBuffer* const desc_ );
//! \brief	размер хранимых данных
size_t	CircBuffer_actual_size( const T8_CircBuffer* const desc_ );
//! \brief	количество потерянных данных
size_t	CircBuffer_lost_size( const T8_CircBuffer* const desc_ );

/*=============================================================================================================*/

ReturnStatus circbuffer_pop_byte
( 
    T8_CircBuffer           *const circbuf_desc,
    CircBuffer_datatype     *dest
);

ReturnStatus circbuffer_push_byte_erasing
(
	T8_CircBuffer           *const circbuf_desc,
	CircBuffer_datatype     source
);

ReturnStatus circbuffer_pop_block
( 
    T8_CircBuffer                   *circbuf_desc,
    CircBuffer_datatype             *dest, 
    size_t                          read_size,
    size_t                          *actual_read_size 
);

ReturnStatus circbuffer_push_block
(
	T8_CircBuffer                   *circbuf_desc,
        size_t                          *actual_write_size,
	CircBuffer_datatype             *source,
	size_t                          write_size
);

ReturnStatus circbuffer_push_block_erasing
(
	T8_CircBuffer                   *circbuf_desc,
	size_t                          *actual_write_size,
	CircBuffer_datatype             *source,
	size_t                          write_size
);

size_t	circbuffer_get_storage_data_size ( const T8_CircBuffer* const circbuf_desc );
size_t	circbuffer_get_space_size( const T8_CircBuffer* const circbuf_desc );
void    circbuffer_set_empty( T8_CircBuffer* circbuf_desc );



#endif	// 	__CIRCBUFFER_H
