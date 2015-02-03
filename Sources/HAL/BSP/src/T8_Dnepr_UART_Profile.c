/*!
\file T8_Dnepr_UART_Profile.c
\brief ���������� ������� �� UART � ������
\author <a href="mailto:baranovm@gmail.com">Baranov Mikhail</a>
\date june 2012
*/

#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/BSP/inc/T8_Dnepr_UART_Profile.h"
#include "HAL\MCU\inc\T8_5282_uart.h"
#include "HAL/MCU/inc/T8_5282_interrupts.h"

void backplane_uart_isr_hook(void) ;

#define UART1_IRQ_LEVEL					5
#define UART1_IRQ_PRIOTITY				1

#define UART1_SPEED			  			115200 //!< �������� ����� �� kontron'�
uart_desc_t uart1_desc = {
 /*Kontron*/
 1,
 UART1_SPEED,
 0,
 NULL, // ��������� �� callback �� ����
 NULL, // ��������� �� callback �� ��������
 0
};


////////////////////////////////////////////////////////////////////////////////

void UART_Profile_Init( void( *rcv_callback)(const unsigned int),
	    unsigned int (*snd_callback)( unsigned int*, const unsigned int) )
{
	uart1_desc.rcv_callback = rcv_callback ;
	uart1_desc.snd_callback = snd_callback ;
	uart_init( &uart1_desc, SYSTEM_CLOCK_KHZ );
}

////////////////////////////////////////////////////////////////////////////////

u8 Dnepr_Profile_Address(void)
{
	return 125 ;
}

////////////////////////////////////////////////////////////////////////////////
// ���������� �� UART1 (kontron)

void UART1_Handler(void) {
	uart_isr( &uart1_desc );
}

void uart_backplane_start_tx()
{
	uart_start_tx( &uart1_desc );
}

void uart_backplane_stop_tx()
{
	uart_stop_tx( &uart1_desc );
}


