/*!
\file T8_Dnepr_QSPI.c
\brief ������ ��� �������������� ������������ ������� � QSPI
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jul 2013
*/

#include "support_common.h"

#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/MCU/inc/T8_5282_spi.h"

#include "uCOS_II.H"
#include <intrinsics.h>
#include "io5282.h"
#include "prio.h"

/* In ColdfFire CS bita are inverted (i.e. "0"="enable"). */
#define SPI_CONVERT_CS(b) ((~b)&0x0F)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! �������, ��� ������������ �� ���������� � ���, ��� ���������� �����������
//! �� ���������� ����� ���������� ������� ������ ����������
static OS_EVENT *__spi_transaction_end_semaphore = NULL ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_QSPI_Init()
{
	// ������� ������� �������, ����������� ������ � ����������
	assert( __spi_transaction_end_semaphore = OSSemCreate( 0 ) );

	MCU_ConfigureIntr( INTR_ID_QSPI, 5, 1 );
	MCU_EnableIntr( INTR_ID_QSPI, TRUE );

	QSPI_Init() ;
}

_BOOL Dnepr_QSPI_ReadWriteArray( const SPI_CS_LINES cur_cs, const u8 *tx_data, const size_t tx_len,
						u8 *rx_data, const size_t rx_len,
						const u8 divider, _BOOL cpol, _BOOL cpha )
{
	INT8U return_code = OS_ERR_NONE;
	assert( __spi_transaction_end_semaphore );

	// ��������� �������, ��� ������������� �������
	T8_Dnepr_TS_SPI_Lock();

	Dnepr_GPIO_SPI_CS( cur_cs, TRUE );
	// ��������� ��������
	QSPI_ReadWriteArray( cur_cs, tx_data, tx_len, rx_data, rx_len, divider, cpol, cpha  );

	// ����������� �� �������� �� ����� ��������
	OSSemPend( __spi_transaction_end_semaphore, 0, &return_code );
	assert( return_code == OS_ERR_NONE );

	Dnepr_GPIO_SPI_CS( cur_cs, FALSE );

	// ����������� �������
	T8_Dnepr_TS_SPI_Unlock();

	return TRUE ;
}

void isr_QSPI()
{
	// ���� �������� ����������� -- ����������� �������
	if( QSPI_isr() ){
		assert( OSSemPost( __spi_transaction_end_semaphore ) == OS_ERR_NONE );
	}
}
