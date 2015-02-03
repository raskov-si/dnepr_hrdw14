/*!
\file T8_Dnepr_Profile.h
\brief Код для чтения-записи параметров профиля.
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jan 2014
*/

#ifndef __PROFILESTORAGE_H_
#define __PROFILESTORAGE_H_

#include "support_common.h"
#include "sys.h"

//! Максимальная длина значения параметра
#define PROFILESTORAGE_VAL_MAXLEN	1024

void Dnepr_ProfileStorage_Init() ;

////////////////////////////////////////////////////////////////////////////////
// обёртки для записи параметра во флеш [и в eeprom]

u32 Dnepr_IPADDRValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 Dnepr_CHARLINEValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 Dnepr_REALValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 Dnepr_INTValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 Dnepr_BOOLValueUpdate(PARAM_INDEX* p_ix, void* buff);

u32 Dnepr_GetParamColors(u32 p_id, u8* failure_color, u8* degrade_color, u8* normal_color);
u32 Dnepr_WriteParamColors(u32 p_id, u8 failure_color, u8 degrade_color, u8 normal_color);
void  dnepr_wait_eeprom_contact 
(
    u32  time           /*!< [in] время в тесении которого должен появиться контакт */
);


#endif
