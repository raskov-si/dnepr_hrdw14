/*!
\file T8_Dnepr_filesystem.h
\brief ��� ��� ����� yaffs � ������� ������ � ��������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#ifndef __DNEPR_FILESYSTEM_H_
#define __DNEPR_FILESYSTEM_H_

#define DNEPR_FLASH_NAMELEN	32  //!< ������������ ������ ����� �������� �����.
#define DNEPR_FLASH_ADDR	"DneprFlash"
#define DNEPR_FLASH_ROOT	"/" DNEPR_FLASH_ADDR "/"

void Dnepr_filesystem_Init(void);

//! \brief ����� �� ���� � ���� � ������ sName ������, ���� ����������, ������.
//! \param sName ��� ���������.
//! \param pbData ������.
//! \param actual_len ����� ������.
//! \retval TRUE ���� ������ �������.
_BOOL Dnepr_filesystem_Write( u8* sName, u8* pbData, const size_t actual_len );

//! \brief ������ �������� �� �����, ���� ����� ��� ���������� FALSE.
//! \param sName ��������� �� ������ � ������ ���������
//! \param pbData ��������� �� ������, ���� ����� ����������� ������
//! \param actual_len ����� ����������� ������
//! \param maxlen ����� ������� pbData
//! \retval TRUE � ������ ������, ����� FALSE
_BOOL Dnepr_filesystem_Read( u8* sName, u8* pbData, size_t *actual_len, const size_t maxlen );

#endif
