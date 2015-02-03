/*!
\file T8_Dnepr_QSPI.h
\brief ������ ��� �������������� ������������ ������� � QSPI
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jul 2013
*/

#ifndef _DNEPR_QSPI_H__
#define _DNEPR_QSPI_H__

#include "support_common.h"

typedef enum __SPI_CS_LINES{
	SPI_CS_0=0x00,
	SPI_CS_1=0x01,
	SPI_CS_2=0x02,
	SPI_CS_3=0x03,
	SPI_CS_4=0x04,
	SPI_CS_5=0x05,
	SPI_CS_6=0x06,
	SPI_CS_7=0x07
} SPI_CS_LINES ;

void Dnepr_QSPI_Init();

// \brief �������� ���������� SPI, ����������� �� � ����������
// \param cur_cs ������������ �������� CS'�� (0x01 --  1� CS ��������(������), 0�02 -- 2� ��������)
// \param tx_data ������ ������ ��� ��������
// \param tx_len ������� ������ �� tx_data ��������
// \param rx_data ������ ������ ��� �����
// \param rx_len ������� ������ ������� (���������� ������ MAX(rx_len, tx_len))
// \param divider �������� ������� �������� �� CLK, bitrate = Fclk / 2 /divider
// \retval ������ TRUE
_BOOL Dnepr_QSPI_ReadWriteArray( const SPI_CS_LINES cur_cs, const u8 *tx_data, const size_t tx_len,
						u8 *rx_data, const size_t rx_len,
						const u8 divider, _BOOL cpol, _BOOL cpha );

#endif
