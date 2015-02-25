/*!
\file T8_Dnepr_UART_Profile.h
\brief ��������� ���������� ������� �� UART � ������
\author <a href="mailto:baranovm@gmail.com">Baranov Mikhail</a>
\date june 2012
*/

#ifndef __UART_PROFILE_DRIVER_H
#define __UART_PROFILE_DRIVER_H

#include "support_common.h"
#include "HAL\MCU\inc\T8_5282_uart.h"
#include "HAL\MCU\inc\T8_5282_interrupts.h"


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


/*------------------------------������ �  UART0 (���������� �������� ��� �������������)-----------------------*/

static inline void uart_terminal_init( uart_desc_t *const uart )
{
    uart_init( uart, SYSTEM_CLOCK_KHZ );
}

static inline void uart_terminal_start_tx( uart_desc_t *const uart )
{
    uart_start_tx( uart );  
}

static inline void uart_terminal_stop_tx( uart_desc_t *const uart )
{
    uart_stop_tx( uart );
}


static inline void uart_sync_init( uart_desc_t *const uart )
{
    uart_init( uart, SYSTEM_CLOCK_KHZ );
}

static inline void uart_sync_start_tx( uart_desc_t *const uart )
{
    uart_start_tx( uart );  
}

static inline void uart_sync_stop_tx( uart_desc_t *const uart )
{
    uart_stop_tx( uart );
}


#endif // __UART_PROFILE_DRIVER_H
