/*!
\file T8_Dnepr_UART_Profile.h
\brief ��������� ���������� ������� �� UART � ������
\author <a href="mailto:baranovm@gmail.com">Baranov Mikhail</a>
\date june 2012
*/

#ifndef __UART_PROFILE_DRIVER_H
#define __UART_PROFILE_DRIVER_H

#include "support_common.h"

#define BACKPLANE_UART					1

//! ������������� UART
// \param rcv_callback ��������� �� �������, ������� ����� ���������� ��� ������ �������� �����
void UART_Profile_Init( void (*rcv_callback)(const unsigned int),
			unsigned int (*snd_callback)( unsigned int*, const unsigned int)  ) ;

//! ���������� ����� �� ���� backplane'�
u8 Dnepr_Profile_Address(void);

//! �������� ���������� uart �� kontron'� -- ����� �������������� ���������� � ���������� backplane_uart_isr_snd_hook
void uart_backplane_start_tx();
//! ��������� ���������� uart �� kontron'�
void uart_backplane_stop_tx();

#endif // __UART_PROFILE_DRIVER_H
