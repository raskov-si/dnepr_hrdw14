/*!
\file T8_5282_uart.h
\brief Драйвер UART MCF5282. Provide common ColdFire UART routines for polled serial IO
\details FIFO из драйвера убран, потому что его тяжело использовать в более высокоуровневом слое без polling'а, а об ОС и синхронизации здесь мы ничего не знаем
\author FSL, <a href="mailto:baranovm@gmail.com">Baranov Mikhail</a>
*/

#ifndef __IUART_H__
#define __IUART_H__

#include "support_common.h"
#include "ErrorManagement\status_codes.h"

#define TERMINAL_UART  0
#define BACKPLANE_UART 	2 // TODO: в PUSK4 д.б. 1


/********************************************************************/
//!  Дескриптор UART
typedef struct uart_desc
{
    int unit;                  //!< unit number
    unsigned long speed;       //!< baud rate
    unsigned long phy_errors;  //!< сколько насчитано ошибок в uart_isr
    //! Указатель на callback, который будет вызываться при приёме очередного слова (байта)
    void (*rcv_callback)(const unsigned int);
    //! \brief Указатель на callback, который будет вызываться из uart_isr при появлении возможности послать
    //! param snd_buff указатель на массив длинной len_max
    //! param len_max длина массива snd_buff
    //! retval количество действительно записанных байтов
    unsigned int (*snd_callback)( unsigned int * snd_buff, const unsigned int len_max );
    int initdone;              //!< была ли произведена инициализация
    //! текущее значение регистра uimr (его нельзя прочитать напрямую)
    u8 uimr ;
}   uart_desc_t ;

/********************************************************************/

//! Инициализация UART
ReturnStatus uart_init( uart_desc_t *const uart, 
                        const u32 i_sysclk_kHz_ );

//! Обработчик всех прерываний от UART
//! \param  uart_    Дескриптор соотв. UART'а
void uart_isr( uart_desc_t *const uart_ ) ;

/********************************************************************/

//! \brief включает передатчик -- тут же генерируется прерывание о передаче
void uart_start_tx( uart_desc_t * const );
//! \brief останавливает передатчик
void uart_stop_tx( uart_desc_t * const );

/********************************************************************/

#endif /* __IUART_H__ */
