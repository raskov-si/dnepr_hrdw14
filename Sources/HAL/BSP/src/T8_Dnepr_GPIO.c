/*!
\file T8_Dnepr_GPIO.c
\brief Code for configuring and working with GPIO Edge Port (External Interrupts) for Dnieper Board
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/MCU/inc/T8_5282_interrupts.h"
#include <intrinsics.h>
#include "prio.h"
#include "io5282.h"

void Dnepr_GPIO_timepin1_off();
void Dnepr_GPIO_timepin1_on();
void Dnepr_GPIO_timepin2_off();
void Dnepr_GPIO_timepin2_on();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief настраиваем выводы микросхемы для работы на нашей плате 

void DNEPR_PinsInit(void)
{
#ifdef  DEBUG_TERMINAL  
	// UART1 и UART0  
	MCF_PAD_PUAPAR = MCF_PAD_PUAPAR_PUAPA3 | MCF_PAD_PUAPAR_PUAPA2 | MCF_PAD_PUAPAR_PUAPA1 | MCF_PAD_PUAPAR_PUAPA0 ;
#else        
	// UART 1 on
	MCF_PAD_PUAPAR = MCF_PAD_PUAPAR_PUAPA3 | MCF_PAD_PUAPAR_PUAPA2;
#endif        
	
	// MDII
	MCF_PAD_PASPAR |= MCF_PAD_PASPAR_PASPA5_EMDIO | MCF_PAD_PASPAR_PASPA4_EMDC ;
	// MII
	MCF_PAD_PEHLPAR = MCF_PAD_PEHLPAR_PELPA | MCF_PAD_PEHLPAR_PEHPA ;


	// CS0-CS2 -- GPIO
	MCF_PAD_PQSPAR &= 0xFF ^ 0x38 ;
	MCF_GPIO_DDRQS |= MCF_GPIO_DDRQS_DDRQS3 | MCF_GPIO_DDRQS_DDRQS4 | MCF_GPIO_DDRQS_DDRQS5 ;
	MCF_GPIO_PORTQS = MCF_GPIO_PORTQS_PORTQS3 | MCF_GPIO_PORTQS_PORTQS4 |
						MCF_GPIO_PORTQS_PORTQS5 ;

	// SPI_CS_EN -- GPIO
	MCF_PAD_PTCPAR = MCF_PAD_PTCPAR_PTCPA3(MCF_PAD_PTCPAR_PTCPA3_GPIO) ;
	MCF_GPIO_DDRTC = MCF_GPIO_DDRTC_DDRTC3 ;
	// SPI CS по умолчанию выключен
	MCF_GPIO_PORTTC &= 0xFF ^ MCF_GPIO_PORTTC_PORTTC3 ;
	Dnepr_GPIO_SPI_CS( SPI_CS_0, FALSE );
		
	MCF_PAD_PBCDPAR &= 0x3F ;
	// SDRAM
	MCF_PAD_PBCDPAR = (~MCF_PAD_PBCDPAR_PCDPA) | MCF_PAD_PBCDPAR_PBPA ;


	// кнопка сброса параметров
	MCF_GPIO_DDRC &= 0xFF ^ 0x08 ;
	
	Dnepr_GPIO_timepin1_off() ;
	Dnepr_GPIO_timepin2_off() ;

	// выводы для управления хабами i2c
	MCF_GPTA_GPTDDR = 	MCF_GPTA_GPTDDR_DDRT3 | MCF_GPTA_GPTDDR_DDRT1 |
						MCF_GPTA_GPTDDR_DDRT2 ; // ALERT_LOCK, SW_A, SW_B
	MCF_GPTB_GPTDDR = 	MCF_GPTB_GPTDDR_DDRT0 ; // SW_INH
	MCF_GPTA_GPTPORT &= 0xF1 ; // ALERT_LOCK = SW_A = SW_B = 0
	MCF_GPTB_GPTPORT &= 0xFE ; // SW_INH = 0

	// выводы внешних прерываний конфигурируются в Dnepr_EdgePort
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_GPIO_timepin1_on()
{}

void Dnepr_GPIO_timepin1_off()
{}

void Dnepr_GPIO_timepin2_on()
{}

void Dnepr_GPIO_timepin2_off()
{}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GPIO_I2C_CHANNEL_ENUM __cur_channel = GPIO_I2C_CHANNEL_1 ;
void Dnepr_GPIO_select_i2c_channel( const GPIO_I2C_CHANNEL_ENUM channel )
{
	if( channel == __cur_channel ){
		return ;
	}
	__cur_channel = channel ;
	
	MCF_GPTB_GPTPORT &= 0xFE ;
	MCF_GPTA_GPTPORT &= 0xF9 ;
	// переворачиваем биты (см. схему)
	MCF_GPTB_GPTPORT |= (channel & 0x04) ? 1 : 0 ;
	MCF_GPTA_GPTPORT |= (channel & 0x01) ? 4 : 0 ;
	MCF_GPTA_GPTPORT |= channel & 0x02 ;
}

void Dnepr_GPIO_SPI_CS( const SPI_CS_LINES cs_num, _BOOL on )
{
	if( on ){
		MCF_GPIO_PORTQS = ((u8)cs_num << 3) & 0x38 ;
		MCF_GPIO_PORTTC |= MCF_GPIO_PORTTC_PORTTC3 ;
	} else  {
		MCF_GPIO_PORTTC &= 0xFF ^ MCF_GPIO_PORTTC_PORTTC3 ;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL Dnepr_GPIO_FrontPanel_Button_Pressed()
{
	return (MCF_GPIO_SETC & MCF_GPIO_SETC_SETC3) == 0 ;
}
