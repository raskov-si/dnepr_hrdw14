/*!
\file AT_AT45DB321D.h
\brief ������� AT45DB321D
\author <a href="mailto:baranovm@t8.ru">������� �. �.</a>
\date nov 2012
*/

#ifndef __AT_45DB321D
#define __AT_45DB321D

#include "support_common.h"
#include "HAL\IC\inc\SPI_Interface.h"

/*!
\defgroup AT_45DB321D_Driver
\{
*/

//! ����� ������� � �������� � �������, ������������ ��� ������� wAddr
#define AT_AT45DB321D_ADDRESS(SECTOR,SHIFT)	(((SECTOR << 10) & 0x7FFC00) | (SHIFT & 0x3F))

void AT_AT45DB321D_InitPeripherialInterface(SPI_PeriphInterfaceTypedef* tSpiPeriphInterface);

//! ��� �������������, ���������� � �.�., �������� �� ����� ����������
typedef struct AT_AT45DB321D_JEDEC__ {
	u8 manufacturer ;	//!< 0x1F ��� Atmel, �.�. ������ 1�� �����, �� � ���� �������� �� ���������������
	u8 family_code ; 	//!< 1 -- Data Flash
	u8 density_code ;	//!< ���-�� ���
	u8 mlc_code ;		//!< 
	u8 prod_version ;	//!< 
} AT_AT45DB321D_JEDEC ;

#define AT_AT45DB321D_SECTOR_LEN	528
#define AT_AT45DB321D_DATA_LEN		512

//! ���������� ������� �������
u16 AT_AT45DB321D_ReadStatus();
//! ��������� ��������� JEDEC ������� �� ����������
void AT_AT45DB321D_ReadJEDEC( AT_AT45DB321D_JEDEC *pJEDEC );

//! ������ ������ �������� �� ����-������ � ����� ����� �������, �������� 528 ���� (�� 512 -- 1.16 ��)
void AT_AT45DB321D_DirectReadBytes( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber );
//! ��������� ������ ����� (�� 512 ���� 1.16 ��)
void AT_AT45DB321D_WriteBuffer1( const u32 wAddress, u8* anBytesToWrite, u16 nBytesNumber );
//! ��������� ������ �����
void AT_AT45DB321D_WriteBuffer2( const u32 wAddress, u8* anBytesToWrite, u16 nBytesNumber );
//! ����� ���������� ������� ������ �� ������ wAddr (10.49 ��)
void AT_AT45DB321D_ProgrammBuffer1( const u32 wAddr );
//! ����� ���������� ������� ������ �� ������ wAddr
void AT_AT45DB321D_ProgrammBuffer2( const u32 wAddr );
//! ������ ���������� 1�� ������ 
void AT_AT45DB321D_ReadBuff1( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber );
//! ������ ���������� 2�� ������ 
void AT_AT45DB321D_ReadBuff2( const u32 wAddr, u8* anRecievedBytes, u16 nBytesNumber );
//! ������� ���� (8 �������) ����� nBlock
void AT_AT45DB321D_ErraseBlock( const u32 nBlock );

//! ��������� ���������� ����-������
_BOOL AT_AT45DB321D_ReadyToWrite();

/*!
\}
*/

#endif
