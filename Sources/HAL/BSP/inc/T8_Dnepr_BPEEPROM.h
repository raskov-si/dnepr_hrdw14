/*!
\file T8_Dnepr_BPEEPROM.h
\brief Код для работы с параметрами в EEPROM на backplane'е
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#ifndef __DNEPR_BPEEPROM_H_
#define __DNEPR_BPEEPROM_H_

#include "support_common.h"
#include "sys.h"

#define BPEEPROM_PRIMARY_PART_START 	0
#define BPEEPROM_SECONDARY_PART_START 	65536
#define BPEEPROM_END		131071 // последний байт в EEPROM

//! ограничение на длину имени параметра -- сектор 76 байт
#define BPEEPROM_NAMELEN				32 
//! общее кол-во секторов, (65536 - 1280) / 76 = 845
#define BPEEPROM_TOTALSECTORS	840

typedef enum {
	BPEEPROM_NONE,			//!< отсутствует
	BPEEPROM_OK,			//!< всё хорошо
	BPEEPROM_ERRORNEOUS		//!< ошибки
} BPEEPROM_State ;

//! \brief Пытается открыть оба раздела, проверяет целостность обоих, копирует целостный
//! \brief в нецелостный, копирует параметры в наши локальные параметры, в случае
//! \brief неудачи говорит фиксирует во внутренних переменных, что eeprom сломалась.
//! \brief Вызывается до всех потоков.
_BOOL Dnepr_BPEEPROM_Init() ;

//! \brief читает параметр из первичного раздела, в случае неудачи читает из
//! \brief читает из вторичного и переписывает в первичный
//! \param sName указатель на строку с именем параметра
//! \param pbData указатель на массив, куда будут скопированы данные
//! \param actual_len длина прочитанных данных
//! \param maxlen длина массива pbData
//! \retval TRUE в случае успеха, иначе FALSE
_BOOL Dnepr_BPEEPROM_Read(u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen  );

//! \brief пишет параметр в EEPROM в оба раздела по очереди
//! \param sName имя параметра
//! \param pbData данные
//! \param actual_len длина данных
//! \retval TRUE если запись удалась хотя бы в один раздел
_BOOL Dnepr_BPEEPROM_Write(u8* sName, u8* pbData, const size_t actual_len );

//! переформатирует eeprom, делает пустым, прибл. 8 секунда на раздел -- 16 всего
_BOOL Dnepr_eeprom_format(); 
//! проходит по параметрам с флагом EEPROM_PARAM, и переписывает значение из EEPROM, если нужно
_BOOL Dnepr_eeprom_sync();
//! возвращает текущее состояние EEPROM
BPEEPROM_State Dnepr_eeprom_GetState() ;

_BOOL Dnepr_BPEEPROM_CheckPresent();
void Dnepr_BPEEPROM_WriteEnable(void);
void Dnepr_BPEEPROM_WriteDisable(void);


#endif
