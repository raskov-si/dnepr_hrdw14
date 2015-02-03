/*!
\file T8_Dnepr_filesystem.h
\brief Код для связи yaffs с железом днепра и профилем
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#ifndef __DNEPR_FILESYSTEM_H_
#define __DNEPR_FILESYSTEM_H_

#define DNEPR_FLASH_NAMELEN	32  //!< Максимальный размер длины названия файла.
#define DNEPR_FLASH_ADDR	"DneprFlash"
#define DNEPR_FLASH_ROOT	"/" DNEPR_FLASH_ADDR "/"

void Dnepr_filesystem_Init(void);

//! \brief Пишет во флеш в файл с именем sName данные, если необходимо, создаёт.
//! \param sName имя параметра.
//! \param pbData данные.
//! \param actual_len длина данных.
//! \retval TRUE если запись удалась.
_BOOL Dnepr_filesystem_Write( u8* sName, u8* pbData, const size_t actual_len );

//! \brief Читает параметр из файла, если файла нет возвращает FALSE.
//! \param sName указатель на строку с именем параметра
//! \param pbData указатель на массив, куда будут скопированы данные
//! \param actual_len длина прочитанных данных
//! \param maxlen длина массива pbData
//! \retval TRUE в случае успеха, иначе FALSE
_BOOL Dnepr_filesystem_Read( u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen );

#endif
