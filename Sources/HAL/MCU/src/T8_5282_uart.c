/*!
\file T8_Dnepr_UART_Profile.h
\brief Механизмы реализации профиля по UART в Днепре
\author <a href="mailto:baranovm@gmail.com">Baranov Mikhail</a>
\date june 2012
\todo Сделать по общему примеру: дескриптор -- структуры с параметрами, без хардкода
*/


/*
* 13.10.2011 UAV
* 1. correct uart1_inbuff()
* 2. correct ISR
* 09.11.2011
* 3. uartBP_inbuff()
*    uartTR_inbuff()
* */

/* FILENAME: iuart.c 
*
* Copyright  2005 By InterNiche Technologies Inc. All rights reserved
*
* Code for serfacing with UARTs
*
* FUNCTIONS: uart_init(), uart_getc(), uart_putc(), 
*          uart_flush(), uart_ready(), uart_present(),
*          uart_close(), uart_stats(), uart_check(),
*          uart_isr(), uart0_isr(), uart1_isr(),
* PPP FUNCTIONS: ln_uinit(), ln_uconnect(), ln_udisconnect(),
*          ln_uputc(), uart_baud()
*            
* If POLLED_UART is defined, then UART I/O is done via polling, else
* UART I/O is done using serrups.
*
* When POLLED_UART is defined
* 1. Console works, PPP doesn't work (we loose data)
* 2. uart_check() calls uart_isr() to process Tx and Rx FIFOs.
*
* When POLLED_UART is not defined
* 1. UART I/O is done using serrupts
* 2. In uart_init(), serrupts are enabled for the UART0, UART1
* 3. When serrupt occurs, from the vector table (vectors.s),
*    uartN_isr() is called. uartN_isr() calls uart_isr(N)
*    It checks Tx and Rx status to determine I/O processing.
*    If new data is received, get it and place it in the Rx buffer.
*    If there is data in the Tx buffer, transmit it. 
*
* To use serrupts on EVB, we need to do the following
* 1. UART0 and UART1 have unique serrupt vector numbers.
* 2. Configure the port bits (done in device initialization).
* 3. Enable the serrupts for the UARTs (done in uart_init())
*    and enable the recievers. Set the transmitters to "idle".
* 4. When the serrupt occurs, check the Rx status for new
*    characters and the Tx status for room to transmit more
*    characters. Continue until Rx and Tx clear.
*
*/

#include "HAL/MCU/inc/T8_5282_uart.h"


#define MAX_UARTS    3     //!< кол-во UART'ов в камне

#define UART0_IRQ_LEVEL            5
#define UART0_IRQ_PRIOTITY          0
#define UART1_IRQ_LEVEL          5
#define UART1_IRQ_PRIOTITY          1
#define UART2_IRQ_LEVEL          5
#define UART2_IRQ_PRIOTITY          2


/***********************************************************************
*  FUNCTION: uart_init()
*
* Sets up UART device
*
* PARAM1: unit;
*
* RETURNS: 0 if successful, otherwise -1
***********************************************************************/

ReturnStatus uart_init( uart_desc_t *const uart, const u32 i_sysclk_kHz_ )
{

	s32 dev;
	volatile s32 divisor;

	assert( uart != NULL );
	dev  = uart->unit;
	uart->uimr = 0 ;
	if ((dev < 0) || (dev >= MAX_UARTS))
	{
		return ERROR;
	}

	/* reset the UART */
	/* disable serrupts, reset the device, and disable Rx and Tx */
	MCF_UART_UIMR(dev) = 0;
	MCF_UART_UCR(dev) = MCF_UART_UCR_RESET_RX;
	MCF_UART_UCR(dev) = MCF_UART_UCR_RESET_TX;
	MCF_UART_UCR(dev) = MCF_UART_UCR_RESET_ERROR;
	MCF_UART_UCR(dev) = MCF_UART_UCR_RESET_BKCHGINT;
	MCF_UART_UCR(dev) = ( MCF_UART_UCR_RESET_MR |
		MCF_UART_UCR_TX_DISABLED |
		MCF_UART_UCR_RX_DISABLED );

	   /* configure for 8,N,1 with no flow control */
	MCF_UART_UMR(dev) = ( 	MCF_UART_UMR_PM_NONE |
		MCF_UART_UMR_BC_8 );
	MCF_UART_UMR(dev) = ( 	MCF_UART_UMR_CM_NORMAL |
		MCF_UART_UMR_SB_STOP_BITS_1 );

	/* baud rate is set by calculating a clock divider:
	*    divider = (f_sys/2) / (32 * baudrate)
	*/
	divisor = (s32)((((i_sysclk_kHz_ * 1000) / uart->speed) + 16) / 32);
	MCF_UART_UBG1(dev) = (u8)(divisor >> 8) & 0xff;
	MCF_UART_UBG2(dev) = (u8)divisor & 0xff;
	MCF_UART_UCSR(dev) = (MCF_UART_UCSR_RCS_SYS_CLK | MCF_UART_UCSR_TCS_SYS_CLK);
   
	/* initialize the UART serrupts */
	/*configure the UART ICR and enable associtate IMR*/
	if (dev == 0) {
	MCF_INTC0_ICR13 = MCF_INTC_ICR_IL(UART0_IRQ_LEVEL) | MCF_INTC_ICR_IP(UART0_IRQ_PRIOTITY);
	MCF_INTC0_IMRL &= ~( MCF_INTC_IMRL_INT_MASK13);
	}
	else if (dev == 1)
	{
	MCF_INTC0_ICR14 = MCF_INTC_ICR_IL(UART2_IRQ_LEVEL) | MCF_INTC_ICR_IP(UART2_IRQ_PRIOTITY);
	MCF_INTC0_IMRL &= ~( MCF_INTC_IMRL_INT_MASK14);
	}
	else if (dev == 2)
	{
	MCF_INTC0_ICR15 = MCF_INTC_ICR_IL(UART1_IRQ_LEVEL) | MCF_INTC_ICR_IP(UART1_IRQ_PRIOTITY);
	MCF_INTC0_IMRL &= ~( MCF_INTC_IMRL_INT_MASK15);
	}

	// включаем передатчик, прерывание по TXRDY замаскировано
	MCF_UART_UCR(dev) = MCF_UART_UCR_TX_ENABLED ;
	/* enable receiver and Rx & Tx serrupts */
	MCF_UART_UCR(dev) = (MCF_UART_UCR_RX_ENABLED);
	uart->uimr = MCF_UART_UIMR_FFULL_RXRDY ;
	MCF_UART_UIMR(uart->unit) = uart->uimr ;
	uart->initdone = 1;
	return OK;
}


/********************************************************************
* FUNCTION: uart_isr()
********************************************************************
* UART serrupt service request handler
*
* PARAM1: unit;        UART device number
*
* RETURN: none
*
* Copy characters from the UART Rx buffer to rx_buf.
* Copy characters from tx_buf to the UART Tx buffer.
* Turn off the Tx buffer if it is idle.
*
* The 'unit' parameter is guarenteed to be valid, so we
* don't do any error checking.
*********************************************************************/

#define SND_BUFF_LEN  1 //!< длина буфера для отсылки
static unsigned int snd_buff[SND_BUFF_LEN]; //!< буфер куда складываем байты для отсылки
static unsigned int snd_buff_curr_ind ; //!< номер первого непосланного байта из буфера

void uart_isr(uart_desc_t * const uart_)
{
	s32 dev = uart_->unit;
	volatile u8 ch;			//FSL made volatile for debugging help
	u8 usr;
	u32   sndmax ;    // сколько сейчас на отсылку байт передано в локальный буфер
	usr = MCF_UART_USR(dev);		//FSL get copy of uart status register
	assert( uart_ != NULL );
	/************************ UART receiver *****************************/
	//UART has FIFO for 3 bytes + shift register,
	//So if we read more than 4 bytes from UART,
	//something WRONG!
	while (usr & MCF_UART_USR_RXRDY) {
		ch = MCF_UART_URB(dev);	//FSL get a character
		// discard character if there was an error
		if ( !(usr & (MCF_UART_USR_FE | MCF_UART_USR_PE | MCF_UART_USR_OE )) ) {
			if(uart_->rcv_callback)
				(*uart_->rcv_callback)(ch) ;
		}
		else {//have errors
			MCF_UART_UCR(dev) = MCF_UART_UCR_RESET_ERROR;
		    uart_->phy_errors++;
		}  
        usr = MCF_UART_USR(dev);
    }
	/* UART transmitter */
	if ((usr & MCF_UART_USR_TXRDY) && (uart_->snd_callback) && (uart_->uimr & MCF_UART_UIMR_TXRDY)) {						//FSL if transmitter holding register is empty (i.e. _TXRDY=1)
            sndmax = (*uart_->snd_callback)( snd_buff, SND_BUFF_LEN ) ;
            assert( sndmax <= SND_BUFF_LEN );
            snd_buff_curr_ind = 0;
            for( snd_buff_curr_ind = 0; snd_buff_curr_ind < sndmax; snd_buff_curr_ind++) {
			MCF_UART_UTB(dev) = snd_buff[snd_buff_curr_ind];           //FSL output character
            }
	}
}


//! \brief разрешаем прерывание о готовности передатчика
void uart_start_tx(uart_desc_t * const uart_)
{
	uart_->uimr |= MCF_UART_UIMR_TXRDY ;
	MCF_UART_UIMR(uart_->unit) = uart_->uimr ;
}

//! \brief запрещает прерывания о готовности передатчика
void uart_stop_tx(uart_desc_t * const uart_)
{
    // маскируем прерывание о готовности передатчика
	uart_->uimr &= 0xFF ^ MCF_UART_UIMR_TXRDY ;
	MCF_UART_UIMR(uart_->unit) = uart_->uimr ;
}
