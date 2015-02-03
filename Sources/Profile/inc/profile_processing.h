/*!
\file profile_processing.h
\brief Static parameters load & store
\author <a href="mailto:baranovm@t8.ru">Mikhail Baranov</a>
\date june 2012
*/

#include "support_common.h"
#include "common_lib/T8_CircBuffer.h"

void Profile_Init();

//! \brief Основная процедура обработки сообщений профиля. Не возвращает ошибки
//! \param cBuffer 	буффер с полным сообщением команды профиля
//! \retval	указатель на массив с ответным сообщением (кот. хранится локально)
ReturnStatus Profile_message_processing(const s8 * recv_buff_,
			const size_t recv_command_len_,  s8* answer_ );
