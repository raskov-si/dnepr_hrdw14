/*!
\file T8_Dnepr_Select.h
\brief ��� ��� ������ � ������������� ������ ��� SELECT � PRESENT.
\author <a href="mailto:baranovm@t8.ru">������� �. �.</a>
\date oct 2013
*/

#ifndef __T8_DNEPR_SELECTPRESENT_H_
#define __T8_DNEPR_SELECTPRESENT_H_

#include "support_common.h"
#include "HAL/IC/inc/TI_TCA9539.h"

typedef enum {
	SELECT_SLOT_0 ,
	SELECT_SLOT_1 ,
	SELECT_SLOT_2 ,
	SELECT_SLOT_3 ,
	SELECT_SLOT_4 ,
	SELECT_SLOT_5 ,
	SELECT_SLOT_6 ,
	SELECT_SLOT_7 ,
	SELECT_SLOT_8 ,
	SELECT_SLOT_9 ,
	SELECT_SLOT_10 ,
	SELECT_SLOT_11 ,
	SELECT_SLOT_12 ,
	SELECT_SLOT_FAN ,
	SELECT_SLOT_ALL ,
	SELECT_SLOT_NONE
} Dnepr_Select_t ;



//! �������������� ����������� ������ � ������� ����������.
void Dnepr_InitSelect();
//! �������� �������� ����, ���������� ���������� ���������� �� I2C.
_BOOL Dnepr_Select( const Dnepr_Select_t select, _BOOL *swtchd );

_BOOL dnepr_select_slot_read
(
  const Dnepr_Select_t signal_index
);


#endif
