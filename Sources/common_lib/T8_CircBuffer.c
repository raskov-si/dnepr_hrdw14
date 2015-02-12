#include "common_lib\T8_CircBuffer.h"
#include "ctype.h"
#include "common_lib\memory.h"
#include "T8_Atomiccode.h"

//  TODO: �������� ����� ���������� �� 75 ��� -- ����������� � ����������
ReturnStatus CircBuffer_push_one_erasingdata( T8_CircBuffer* const desc_, 
				const CircBuffer_datatype data_ )
{
	STORAGE_ATOMIC() ;
	ReturnStatus	erase_data = OK;
	START_ATOMIC();
	desc_->data[desc_->iTail] = data_ ;
	// ������������, ���� ����������� iHead
	if( desc_->szActual == desc_->szTotal ){
		if(desc_->iHead+1 == desc_->szTotal)
			desc_->iHead = 0 ;
		else
			desc_->iHead++;
		desc_->szLostBytes++;
		erase_data = ERROR;
	} else
		desc_->szActual++;
	if(desc_->iTail+1 == desc_->szTotal)
		desc_->iTail = 0 ;
	else
		desc_->iTail++;

	STOP_ATOMIC();
	return erase_data;
}

// TODO: �������� ����� ���������� � CircBuffer_push_one_erasingdata
ReturnStatus CircBuffer_push_one_tender( T8_CircBuffer* const desc_, 
				const CircBuffer_datatype data_ )
{
	STORAGE_ATOMIC() ;
	ReturnStatus	erase_data = OK;
	if( desc_->szActual == desc_->szTotal ){
		desc_->szLostBytes++;
		return ERROR ;
	}

	START_ATOMIC();
	desc_->data[desc_->iTail] = data_ ;
	desc_->szActual++;
	if(desc_->iTail+1 == desc_->szTotal)
		desc_->iTail = 0 ;
	else
		desc_->iTail++;
	STOP_ATOMIC();
	return erase_data;
}

ReturnStatus CircBuffer_write
( 
    T8_CircBuffer*          const descriptor,
    CircBuffer_datatype*    const data, 
    const size_t            datalen
)
{
	STORAGE_ATOMIC();
        
//	ReturnStatus	erase_data = OK;
//	if( descriptor->szActual == descriptor->szTotal ){
//		descriptor->szLostBytes+= datalen;
//		return ERROR ;
//	}
//
//	START_ATOMIC();
//	desc_->data[desc_->iTail] = data_ ;
//	desc_->szActual++;
//	if(desc_->iTail+1 == desc_->szTotal)
//		desc_->iTail = 0 ;
//	else
//		desc_->iTail++;
//	STOP_ATOMIC();
//        
//	return erase_data;
}



ReturnStatus CircBuffer_read( T8_CircBuffer* const desc_,
				CircBuffer_datatype* const  dest_, const size_t szDest_,
				 size_t* const szActual_read_ )
{
	STORAGE_ATOMIC() ;
	size_t		iTail_store, iHead_store, iReadFrom ;
	size_t		szActual_read = 0 ;
	size_t		szActual_read_2 = 0 ;
	size_t		dest_i = 0 ;
	ReturnStatus ret = OK ; // ���� �� ����� ������ ������ ���� ������ -- ERROR

	if( (desc_->szActual == 0) || (szDest_ == 0) ){
		if( szActual_read_ )
			*szActual_read_ = 0 ;
		return OK ;
	}

	// ����� ������������� � ���� ������������ �������� ������ 
	START_ATOMIC();
	iHead_store = iReadFrom = desc_->iHead ;
	iTail_store = desc_->iTail ;
	STOP_ATOMIC();

	if (iTail_store > iHead_store) {
		szActual_read = iTail_store - iReadFrom ; // ������� ���� ������
		szActual_read = MIN(szDest_, szActual_read) ; // ������� ���� ����� � ��������
		t8_memcopy( dest_, &(desc_->data[iReadFrom]),
				szActual_read );
		// ��������� ������� ��������� ������
		START_ATOMIC();
		// ���� ������ ���� ������ ������
		if( iHead_store != desc_->iHead )
			ret = ERROR ;
		if ((iReadFrom + szActual_read) > desc_->iHead){
			desc_->iHead = iReadFrom + szActual_read ;
		}
		desc_->szActual -= szActual_read;
		STOP_ATOMIC();

	// ������ �������� ����� ����� � ������ �������
	} else if (iTail_store <= iReadFrom){
		// �������� �� ������ �� ����� �������
		szActual_read = desc_->szTotal - iReadFrom ; // ������� ���������� � ������ ������
		szActual_read = MIN(szDest_, szActual_read) ; // ������� ���� ����� � ��������
		t8_memcopy( dest_, &(desc_->data[iReadFrom]),
						szActual_read );
		// �������� �� ������ ������� �� ������
		iReadFrom += szActual_read ;
		if( (iReadFrom >= desc_->szTotal) && (szActual_read < szDest_) ){
			iReadFrom = 0 ;
			// ������� ���� ��������� �� ������ ������
			szActual_read_2 = MIN(szDest_ - szActual_read, // ������� �������� ����� � ��������
								iTail_store ) ; // ������� ��� �� ��������� (�� ������ �������)
			dest_i = szActual_read ;
			szActual_read += szActual_read_2; // ������� ����� ���������
			t8_memcopy( &dest_[dest_i], desc_->data,
					szActual_read_2 );
		}
		// ��������� ������� ��������� ������
		START_ATOMIC();
		// ���� ������ ���� ������ ������
		if( iHead_store != desc_->iHead )
			ret = ERROR ;
		desc_->iHead = szActual_read_2 ;
		desc_->szActual -= szActual_read ;
		STOP_ATOMIC();
	}

	if(szActual_read_)
		*szActual_read_ = szActual_read ;
	return ret ;
}


size_t	CircBuffer_total_size( const T8_CircBuffer* const desc_ ) 
{
	return desc_->szTotal ;
}
size_t	CircBuffer_actual_size( const T8_CircBuffer* const desc_ )
{
	return desc_->szActual ;
}
size_t	CircBuffer_lost_size( const T8_CircBuffer* const desc_ )
{
	return desc_->szLostBytes ;
}
