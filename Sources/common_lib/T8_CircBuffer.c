#include "common_lib\T8_CircBuffer.h"
#include "ctype.h"
#include "common_lib\memory.h"
#include "T8_Atomiccode.h"

//  TODO: измерить время выполнения на 75 МГц -- выполняется в прерывании
ReturnStatus CircBuffer_push_one_erasingdata( T8_CircBuffer* const desc_, 
				const CircBuffer_datatype data_ )
{
	STORAGE_ATOMIC() ;
	ReturnStatus	erase_data = OK;
	START_ATOMIC();
	desc_->data[desc_->iTail] = data_ ;
	// переполнение, надо протолкнуть iHead
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

// TODO: сравнить время выполнения с CircBuffer_push_one_erasingdata
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
	ReturnStatus ret = OK ; // если во время чтения затёрли наши данные -- ERROR

	if( (desc_->szActual == 0) || (szDest_ == 0) ){
		if( szActual_read_ )
			*szActual_read_ = 0 ;
		return OK ;
	}

	// чтобы парараллельно с этим копированием пушились данные 
	START_ATOMIC();
	iHead_store = iReadFrom = desc_->iHead ;
	iTail_store = desc_->iTail ;
	STOP_ATOMIC();

	if (iTail_store > iHead_store) {
		szActual_read = iTail_store - iReadFrom ; // сколько есть данных
		szActual_read = MIN(szDest_, szActual_read) ; // сколько есть места в приёмнике
		t8_memcopy( dest_, &(desc_->data[iReadFrom]),
				szActual_read );
		// сохраняем текущее состояние буфера
		START_ATOMIC();
		// пока читали наши данные затёрли
		if( iHead_store != desc_->iHead )
			ret = ERROR ;
		if ((iReadFrom + szActual_read) > desc_->iHead){
			desc_->iHead = iReadFrom + szActual_read ;
		}
		desc_->szActual -= szActual_read;
		STOP_ATOMIC();

	// данные проходят через конец и начало массива
	} else if (iTail_store <= iReadFrom){
		// копируем от головы до конца массива
		szActual_read = desc_->szTotal - iReadFrom ; // сколько копировать в первом заходе
		szActual_read = MIN(szDest_, szActual_read) ; // сколько есть места в приёмнике
		t8_memcopy( dest_, &(desc_->data[iReadFrom]),
						szActual_read );
		// копируем от начала массива до хвоста
		iReadFrom += szActual_read ;
		if( (iReadFrom >= desc_->szTotal) && (szActual_read < szDest_) ){
			iReadFrom = 0 ;
			// сколько надо прочитать во втором заходе
			szActual_read_2 = MIN(szDest_ - szActual_read, // сколько осталось места в приёмнике
								iTail_store ) ; // сколько ещё не прочитано (от начала массива)
			dest_i = szActual_read ;
			szActual_read += szActual_read_2; // сколько всего прочитали
			t8_memcopy( &dest_[dest_i], desc_->data,
					szActual_read_2 );
		}
		// сохраняем текущее состояние буфера
		START_ATOMIC();
		// пока читали наши данные затёрли
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
