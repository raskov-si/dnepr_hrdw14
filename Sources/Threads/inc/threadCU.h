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

//! ��� ������, ���������� � taskCU ����� �������
typedef struct taskCU_message_ {
	enum mess_type_t {
		PROFILE_STRING,		//!< ������ ������� �������
		RENEW_SFP_PARAMS,	//!< ���� �������� ��������� SFP
		FORMAT_BPEEPROM,	//!< ����������������� EEPROM �� backplane'�
		RESET2DEFAULT		//!< �������� ��������� � ����������� ��������
	} message_type ;
    size_t rcvmsg_len ;         //!< ����� ���������� ��������� � ��������� ������
    T8_CircBuffer * rcv_cbuff ; //!< ��������� ����� � �����������
} taskCU_message_t ;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void taskCU (void *pdata);
//! \brief �������� ��������� � ����� CU
void CU_send_queue_message( const taskCU_message_t *mess_ );
//! \brief ������������ ����� ��������� ������ ������� � �� � 1/8192 �� ������� ��������� ����������
u32 CU_get_max_delay();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __THREADCU_H_
