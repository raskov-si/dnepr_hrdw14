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
#include "support_common.h" // ��� ����� ������

typedef		u8	CircBuffer_datatype ;

////////////////////////////////////////////////////////////////////////////////
//! ���������� ���������� ������
typedef struct 
{
	//! ��������� �� ������ � �������
	CircBuffer_datatype*	data ; 
	//! ��������� ������ ��������� ������, ����� ������ �� ������ ������� 4
	const size_t			szTotal ;	
	//! ������� ���������� ���������� ��������� (CircBuffer_datatype)
	size_t					szActual ;
	//! ���-�� ������� ��������� � ���-�� ������������
	size_t					szLostBytes ; 
	//! ����� ������� ��������� �������� � ���������
	size_t					iHead ;
	//! ����� ���������� �� ��������� �������� ��������� � ���������
	size_t					iTail ;
} T8_CircBuffer ;

//! ���������� ��������� ����� � ���� (�.�. � ����� ��� ���������)
// ������� ��� � ��������������� ����� ������ ��� ����, ����� ��������� t8_memcopy32
#define		CIRCBUFFER(name, size) u8 name##_array_[size];\
		T8_CircBuffer name = { (CircBuffer_datatype*)name##_array_, \
		size, 0, 0, 0, 0}

////////////////////////////////////////////////////////////////////////////////

//! \brief	�������� ���������� ������� � ��������� ������, ��� ������ ������,
//! 		��������� ����� ������
//! \param	desc_	���������� ���������� ������
//! \param	data_	���� ������
//! \retval	OK ���� ������ �� ������, ����� ERROR
ReturnStatus CircBuffer_push_one_erasingdata( T8_CircBuffer* const desc_, 
				const CircBuffer_datatype data_ );
//! \brief	�������� ���������� ������� � ��������� ������, �� ��� ��������  
//! 	������ ������, ��������� ������ ������
//! \param	desc_	���������� ���������� ������
//! \param	data_	������
//! \retval	OK ���� ����������� � ��������, ����� ERROR
ReturnStatus CircBuffer_push_one_tender( T8_CircBuffer * const desc_, 
				const CircBuffer_datatype data_ );

//! \brief ��������� ��� ������ �� ������, ���������� � �� � ������ ������,
//! ����� ����������� ����������� � �����-������ push
//! \param desc_ ���������� ���������� ������
//! \param dest_ ���� ������� ������
//! \param szDest_ ������������ ������ ������������ ������
//! \param szActual_read_ ������� �������� � dest_
//! \retval	OK � �������� ������ ����� �� ������������,
//! 	ERROR -- ����� ������������, ����������� ������ ���������
ReturnStatus CircBuffer_read( T8_CircBuffer * const desc_,
				CircBuffer_datatype* const dest_, const size_t szDest_,
				 size_t* const szActual_read_ );

//! \brief ������ ��������� ������
size_t	CircBuffer_total_size( const T8_CircBuffer* const desc_ );
//! \brief	������ �������� ������
size_t	CircBuffer_actual_size( const T8_CircBuffer* const desc_ );
//! \brief	���������� ���������� ������
size_t	CircBuffer_lost_size( const T8_CircBuffer* const desc_ );

ReturnStatus circbuffer_read_byte
( 
    T8_CircBuffer           *const circbuf_desc,
    CircBuffer_datatype     *dest
);

size_t	circbuffer_get_storage_data_size ( const T8_CircBuffer* const circbuf_desc );
size_t	circbuffer_get_space_size( const T8_CircBuffer* const circbuf_desc );

ReturnStatus circbuffer_read_block
( 
    T8_CircBuffer                   *circbuf_desc,
    CircBuffer_datatype             *dest, 
    size_t                          read_size,
    size_t                          *actual_read_size 
);





#endif	// 	__CIRCBUFFER_H
