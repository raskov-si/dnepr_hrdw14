/*!
\file T8_5282_uart.h
\brief ������� UART MCF5282. Provide common ColdFire UART routines for polled serial IO
\details FIFO �� �������� �����, ������ ��� ��� ������ ������������ � ����� ��������������� ���� ��� polling'�, � �� �� � ������������� ����� �� ������ �� �����
\author FSL, <a href="mailto:baranovm@gmail.com">Baranov Mikhail</a>
*/

#ifndef __IUART_H__
#define __IUART_H__

#include "support_common.h"
#include "ErrorManagement\status_codes.h"

#define TERMINAL_UART  0
#define BACKPLANE_UART 	2 // TODO: � PUSK4 �.�. 1


/********************************************************************/
//!  ���������� UART
typedef struct uart_desc
{
    int unit;                  //!< unit number
    unsigned long speed;       //!< baud rate
    unsigned long phy_errors;  //!< ������� ��������� ������ � uart_isr
    //! ��������� �� callback, ������� ����� ���������� ��� ����� ���������� ����� (�����)
    void (*rcv_callback)(const unsigned int);
    //! \brief ��������� �� callback, ������� ����� ���������� �� uart_isr ��� ��������� ����������� �������
    //! param snd_buff ��������� �� ������ ������� len_max
    //! param len_max ����� ������� snd_buff
    //! retval ���������� ������������� ���������� ������
    unsigned int (*snd_callback)( unsigned int * snd_buff, const unsigned int len_max );
    int initdone;              //!< ���� �� ����������� �������������
    //! ������� �������� �������� uimr (��� ������ ��������� ��������)
    u8 uimr ;
}   uart_desc_t ;

/********************************************************************/

//! ������������� UART
ReturnStatus uart_init( uart_desc_t *const uart, 
                        const u32 i_sysclk_kHz_ );

//! ���������� ���� ���������� �� UART
//! \param  uart_    ���������� �����. UART'�
void uart_isr( uart_desc_t *const uart_ ) ;

/********************************************************************/

//! \brief �������� ���������� -- ��� �� ������������ ���������� � ��������
void uart_start_tx( uart_desc_t * const );
//! \brief ������������� ����������
void uart_stop_tx( uart_desc_t * const );

/********************************************************************/

#endif /* __IUART_H__ */
