/*!
\file T8_Dnepr_UART_Profile.h
\brief Механизмы реализации профиля по UART в Днепре
\author <a href="mailto:baranovm@gmail.com">Baranov Mikhail</a>
\date june 2012
*/

#ifndef __UART_PROFILE_DRIVER_H
#define __UART_PROFILE_DRIVER_H

#include "support_common.h"

#define BACKPLANE_UART					1

//! Инициализация UART
// \param rcv_callback Указатель на функцию, которая будет вызываться при каждом принятом байте
void UART_Profile_Init( void (*rcv_callback)(const unsigned int),
			unsigned int (*snd_callback)( unsigned int*, const unsigned int)  ) ;

//! возвращает адрес на шине backplane'а
u8 Dnepr_Profile_Address(void);

//! включает передатчик uart до kontron'а -- будет генерироваться прерывание и вызываться backplane_uart_isr_snd_hook
void uart_backplane_start_tx();
//! выключает передатчик uart до kontron'а
void uart_backplane_stop_tx();

#endif // __UART_PROFILE_DRIVER_H
