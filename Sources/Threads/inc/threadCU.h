/*!
\file threadCU.h
\brief CU uart interaction thread
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jul 2012
*/

#ifndef __THREADCU_H_
#define __THREADCU_H_

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "common_lib/T8_CircBuffer.h"

//! тип команд, послыаемых в taskCU через очередь
typedef struct taskCU_message_ {
	enum mess_type_t {
		PROFILE_STRING,		//!< пришла команда профиля
		RENEW_SFP_PARAMS,	//!< надо обновить параметры SFP
		FORMAT_BPEEPROM,	//!< переформатировать EEPROM на backplane'е
		RESET2DEFAULT		//!< сбросить параметры в умолчальные значения
	} message_type ;
    size_t rcvmsg_len ;         //!< длина пришедшего сообщения в кольцевом буфере
    T8_CircBuffer * rcv_cbuff ; //!< кольцевой буфер с сообщениями
} taskCU_message_t ;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void taskCU (void *pdata);
//! \brief отсылает сообщение в поток CU
void CU_send_queue_message( const taskCU_message_t *mess_ );
//! \brief максимальное время обработки строки профиля в МК в 1/8192 от периода тактового генератора
u32 CU_get_max_delay();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __THREADCU_H_
