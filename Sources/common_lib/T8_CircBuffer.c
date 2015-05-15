#include "common_lib\T8_CircBuffer.h"
#include "ctype.h"
#include "common_lib\memory.h"
#include "T8_Atomiccode.h"

//  TODO: �������� ����� ���������� �� 75 ��� -- ����������� � ����������
ReturnStatus CircBuffer_push_one_erasingdata(T8_CircBuffer *const desc_,
	const CircBuffer_datatype data_)
{
	STORAGE_ATOMIC();
	ReturnStatus	erase_data = OK;
	START_ATOMIC();
	desc_->data[desc_->iTail] = data_;
	// ������������, ���� ����������� iHead
	if (desc_->szActual == desc_->szTotal) {
		if (desc_->iHead + 1 == desc_->szTotal)
			desc_->iHead = 0;
		else
			desc_->iHead++;
		desc_->szLostBytes++;
		erase_data = ERROR;
	} else
		desc_->szActual++;
	if (desc_->iTail + 1 == desc_->szTotal)
		desc_->iTail = 0;
	else
		desc_->iTail++;

	STOP_ATOMIC();
	return erase_data;
}

// TODO: �������� ����� ���������� � CircBuffer_push_one_erasingdata
ReturnStatus CircBuffer_push_one_tender(T8_CircBuffer *const desc_,
	const CircBuffer_datatype data_)
{
	STORAGE_ATOMIC();
	ReturnStatus	erase_data = OK;
	if (desc_->szActual == desc_->szTotal) {
		desc_->szLostBytes++;
		return ERROR;
	}

	START_ATOMIC();
	desc_->data[desc_->iTail] = data_;
	desc_->szActual++;
	if (desc_->iTail + 1 == desc_->szTotal)
		desc_->iTail = 0;
	else
		desc_->iTail++;
	STOP_ATOMIC();
	return erase_data;
}




ReturnStatus CircBuffer_read(T8_CircBuffer *const desc_,
	CircBuffer_datatype *const  dest_, const size_t szDest_,
	size_t *const szActual_read_)
{
	STORAGE_ATOMIC();
	size_t		iTail_store, iHead_store, iReadFrom;
	size_t		szActual_read = 0;
	size_t		szActual_read_2 = 0;
	size_t		dest_i = 0;
	ReturnStatus ret = OK; // ���� �� ����� ������ ������ ���� ������ -- ERROR

	if ((desc_->szActual == 0) || (szDest_ == 0)) {
		if (szActual_read_)
			*szActual_read_ = 0;
		return OK;
	}

	// ����� ������������� � ���� ������������ �������� ������
	START_ATOMIC();
	iHead_store = iReadFrom = desc_->iHead;
	iTail_store = desc_->iTail;
	STOP_ATOMIC();

	if (iTail_store > iHead_store) {
		szActual_read = iTail_store - iReadFrom; // ������� ���� ������
		szActual_read = MIN(szDest_, szActual_read); // ������� ���� ����� � ��������
		t8_memcopy(dest_, &(desc_->data[iReadFrom]),
			szActual_read);
		// ��������� ������� ��������� ������
		START_ATOMIC();
		// ���� ������ ���� ������ ������
		if (iHead_store != desc_->iHead)
			ret = ERROR;
		if ((iReadFrom + szActual_read) > desc_->iHead) {
			desc_->iHead = iReadFrom + szActual_read;
		}
		desc_->szActual -= szActual_read;
		STOP_ATOMIC();

		// ������ �������� ����� ����� � ������ �������
	} else if (iTail_store <= iReadFrom) {
		// �������� �� ������ �� ����� �������
		szActual_read = desc_->szTotal - iReadFrom; // ������� ���������� � ������ ������
		szActual_read = MIN(szDest_, szActual_read); // ������� ���� ����� � ��������
		t8_memcopy(dest_, &(desc_->data[iReadFrom]),
			szActual_read);
		// �������� �� ������ ������� �� ������
		iReadFrom += szActual_read;
		if ((iReadFrom >= desc_->szTotal) && (szActual_read < szDest_)) {
			iReadFrom = 0;
			// ������� ���� ��������� �� ������ ������
			szActual_read_2 = MIN(szDest_ - szActual_read, // ������� �������� ����� � ��������
				iTail_store); // ������� ��� �� ��������� (�� ������ �������)
			dest_i = szActual_read;
			szActual_read += szActual_read_2; // ������� ����� ���������
			t8_memcopy(&dest_[dest_i], desc_->data,
				szActual_read_2);
		}
		// ��������� ������� ��������� ������
		START_ATOMIC();
		// ���� ������ ���� ������ ������
		if (iHead_store != desc_->iHead)
			ret = ERROR;
		desc_->iHead = szActual_read_2;
		desc_->szActual -= szActual_read;
		STOP_ATOMIC();
	}

	if (szActual_read_)
		*szActual_read_ = szActual_read;
	return ret;
}


size_t	CircBuffer_total_size(const T8_CircBuffer *const desc_)
{
	return desc_->szTotal;
}
size_t	CircBuffer_actual_size(const T8_CircBuffer *const desc_)
{
	return desc_->szActual;
}



size_t	CircBuffer_lost_size(const T8_CircBuffer *const desc_)
{
	return desc_->szLostBytes;
}


/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
ReturnStatus circbuffer_push_byte_erasing
(
	T8_CircBuffer           *const circbuf_desc,
	CircBuffer_datatype     source
)
{
	STORAGE_ATOMIC();
	size_t	tail_store, head_store;
        uint8_t overflow_flag = 0;

	tail_store = circbuf_desc->iTail;
        head_store = circbuf_desc->iHead;

	circbuf_desc->data[tail_store] = source;
	tail_store++;
	if (tail_store == circbuf_desc->szTotal) {
		tail_store = 0;
	}
        if (tail_store == head_store) {
          overflow_flag = 1;
          head_store = tail_store + 1;
          if (head_store == circbuf_desc->szTotal) {
            head_store = 0;
          }
        }

	START_ATOMIC();
	circbuf_desc->iTail =  tail_store;
        if (overflow_flag) {
            circbuf_desc->iHead = head_store;
        }
	circbuf_desc->szActual = (overflow_flag) ? circbuf_desc->szTotal : circbuf_desc->szActual+1;
	STOP_ATOMIC();

	return OK;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/

ReturnStatus circbuffer_pop_byte
(
	T8_CircBuffer           *const circbuf_desc,
	CircBuffer_datatype     *dest
	)
{
	STORAGE_ATOMIC();
	size_t	head_store;

	if ((circbuf_desc->szTotal == 0) || (dest == NULL)) {
		return ERROR;
	}

	head_store = circbuf_desc->iHead;

	*dest = circbuf_desc->data[head_store];
	head_store++;
	if (head_store == circbuf_desc->szTotal) {
		head_store = 0;
	}

	START_ATOMIC();
	circbuf_desc->iHead =  head_store;
	circbuf_desc->szActual--;
	STOP_ATOMIC();

	return OK;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
ReturnStatus circbuffer_pop_block
(
	T8_CircBuffer                   *circbuf_desc,
	CircBuffer_datatype             *dest,
	size_t                          read_size,
	size_t                          *actual_read_size
	)
{
	STORAGE_ATOMIC();
	size_t          head_store, tail_store;
	size_t          readed_from_buf_data = read_size;   /* ���  ������ � �������� ����� ���� ������������ ������� */
	size_t          readed_data_size;                   /* ��� �������� ���������� ���������� ������ */


//    assert ( circbuf_desc != NULL );

	if ((circbuf_desc->szActual == 0) || (read_size == 0))  {
		if (actual_read_size != NULL)  {
			*actual_read_size = 0;
		}
		return ERROR;
	}

	head_store = circbuf_desc->iHead;
	tail_store = circbuf_desc->iTail;

	if (read_size > circbuf_desc->szActual) {
		read_size = circbuf_desc->szActual;
	}

	/* ���������� ������� ����� �������� �� ������ */
	readed_from_buf_data = (tail_store > head_store) ?  (tail_store - head_store) : (circbuf_desc->szTotal - head_store);


	/* ���� ���������� ������������� ������ ������, ������ ������ �� ��� ��������� */
	if (readed_from_buf_data > read_size) {readed_from_buf_data = 	read_size; }


	t8_memcopy(dest, &(circbuf_desc->data[head_store]), readed_from_buf_data * sizeof circbuf_desc->data[0]);
	head_store += readed_from_buf_data;
	readed_data_size = readed_from_buf_data;

	/* ���� ���������� ������������� ������ ������ ��� �� ����� �������� �� ��� �� ������ ��� ����  */
	if (readed_from_buf_data < read_size)  {
		if (head_store >= circbuf_desc->szTotal) {
			head_store = 0;
		}

		if (head_store != tail_store)  {
			if ((read_size - readed_data_size) >  tail_store)   {
				readed_from_buf_data = 	tail_store;
			}  else  {
				readed_from_buf_data = 	(read_size - readed_data_size);
			}

			t8_memcopy(&dest[readed_data_size], circbuf_desc->data, readed_from_buf_data * sizeof circbuf_desc->data[0]);
			readed_data_size += readed_from_buf_data;
			head_store += readed_from_buf_data;
		}
	}

	*actual_read_size = readed_data_size;

	START_ATOMIC();
	circbuf_desc->iHead = head_store;
	circbuf_desc->szActual -= readed_data_size;
	STOP_ATOMIC();

	return OK;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
ReturnStatus circbuffer_push_block_erasing
(
	T8_CircBuffer                   *circbuf_desc,
	size_t                          *actual_write_size,
	CircBuffer_datatype             *source,
	size_t                          write_size
)
{
	STORAGE_ATOMIC();
	size_t          head_store, tail_store;
	size_t          index               = 0;
	size_t          writed_to_buf_data  = write_size;
        uint8_t         overflow_flag = 0;


	if ((circbuf_desc->szTotal == 0) || (write_size == 0))  {
		if (actual_write_size != NULL)  {
			*actual_write_size = 0;
		}
		return ERROR;
	}

	head_store = circbuf_desc->iHead;
	tail_store = circbuf_desc->iTail;

	if ( write_size > circbuf_desc->szTotal ) {
		writed_to_buf_data = write_size = circbuf_desc->szTotal;
	}

        if ( write_size > circbuffer_get_space_size(circbuf_desc) ) {
           overflow_flag = 1;
        }          
        
	if ((tail_store + write_size) >= circbuf_desc->szTotal) {
            writed_to_buf_data = (circbuf_desc->szTotal - tail_store);
            t8_memcopy(&circbuf_desc->data[tail_store], source, writed_to_buf_data * sizeof circbuf_desc->data[0]);
	    tail_store = 0;
	    index += writed_to_buf_data;
	    writed_to_buf_data = write_size - writed_to_buf_data;
	}
        
	t8_memcopy(&circbuf_desc->data[tail_store], &source[index], writed_to_buf_data * sizeof circbuf_desc->data[0]);
	tail_store += writed_to_buf_data;
        
        if ( overflow_flag == 1 ) {          
            head_store = tail_store + 1;
            if (head_store == circbuf_desc->szTotal) {
                head_store = 0;
            }
        }

	*actual_write_size = write_size;

	START_ATOMIC();
	circbuf_desc->iTail = tail_store;
        if (overflow_flag) {
          circbuf_desc->iHead = head_store;
        }
	circbuf_desc->szActual = (overflow_flag) ? circbuf_desc->szTotal : (circbuf_desc->szActual + write_size) ;
	STOP_ATOMIC();

	return OK;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
ReturnStatus circbuffer_push_block
(
	T8_CircBuffer                   *circbuf_desc,
	size_t                          *actual_write_size,
	CircBuffer_datatype             *source,
	size_t                          write_size
	)
{
	STORAGE_ATOMIC();
	size_t          head_store, tail_store;
	size_t          index               = 0;
	size_t          writed_to_buf_data  = write_size;

        
	if ((circbuf_desc->szTotal == 0) || (write_size == 0))  {
		if (actual_write_size != NULL)  {
			*actual_write_size = 0;
		}
		return ERROR;
	}

	head_store = circbuf_desc->iHead;
	tail_store = circbuf_desc->iTail;
        

	if ( write_size > circbuf_desc->szTotal ) {
		writed_to_buf_data = write_size = circbuf_desc->szTotal;
	}

        if ( write_size > circbuffer_get_space_size(circbuf_desc) ) {
               write_size = writed_to_buf_data = circbuffer_get_space_size(circbuf_desc);              
        }          
        
	if ((tail_store + write_size) >= circbuf_desc->szTotal) {
            writed_to_buf_data = (circbuf_desc->szTotal - tail_store);
            t8_memcopy(&circbuf_desc->data[tail_store], source, writed_to_buf_data * sizeof circbuf_desc->data[0]);
	    tail_store = 0;
	    index += writed_to_buf_data;
	    writed_to_buf_data = write_size - writed_to_buf_data;
	}
        
	t8_memcopy(&circbuf_desc->data[tail_store], &source[index], writed_to_buf_data * sizeof circbuf_desc->data[0]);
	tail_store += writed_to_buf_data;


	*actual_write_size = write_size;

	START_ATOMIC();
	circbuf_desc->iTail = tail_store;
	circbuf_desc->szActual = circbuf_desc->szActual + write_size;
	STOP_ATOMIC();

    return OK;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
size_t	circbuffer_get_storage_data_size(const T8_CircBuffer *const circbuf_desc)
{
    return circbuf_desc->szActual;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
size_t	circbuffer_get_space_size(const T8_CircBuffer *const circbuf_desc)
{
    return circbuf_desc->szTotal - circbuffer_get_storage_data_size(circbuf_desc);
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void    circbuffer_set_empty( T8_CircBuffer* circbuf_desc ){

    STORAGE_ATOMIC();
    START_ATOMIC();
    circbuf_desc->iHead = circbuf_desc->iTail;
    circbuf_desc->szActual = 0;
    STOP_ATOMIC();
}



