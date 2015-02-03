/*!
\file T8_Dnieper_GPIO.h
\brief Code for configuring and working with GPIO for Dnieper Board
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#ifndef T8_DNIEPER_GPIO_H_
#define T8_DNIEPER_GPIO_H_

#include "support_common.h"
#include "HAL/BSP/inc/T8_Dnepr_QSPI.h"

/// номер канала хабов i2c
typedef enum {
	GPIO_I2C_CHANNEL_1 = 0x00,
	GPIO_I2C_CHANNEL_2 = 0x01,
	GPIO_I2C_CHANNEL_3 = 0x02,
	GPIO_I2C_CHANNEL_4 = 0x03,
	GPIO_I2C_CHANNEL_5 = 0x04,
	GPIO_I2C_CHANNEL_6 = 0x05,
	GPIO_I2C_CHANNEL_7 = 0x06,
	GPIO_I2C_CHANNEL_8 = 0x07
} GPIO_I2C_CHANNEL_ENUM ;

void DNEPR_PinsInit(void);
void Dnepr_GPIO_timepin1_on() ;
void Dnepr_GPIO_timepin1_off() ;
void Dnepr_GPIO_timepin2_on() ;
void Dnepr_GPIO_timepin2_off() ;

//! включает соотв. конфигурацию сигналов на gpio, чтобы хабы i2c включили
//! соотв. канал
void Dnepr_GPIO_select_i2c_channel( const GPIO_I2C_CHANNEL_ENUM channel );

//! Устанавливает код на выводах для нужного CS и включает SPI_CS_EN
void Dnepr_GPIO_SPI_CS( const SPI_CS_LINES cs_num, _BOOL on );

//! Возвращает TRUE, если кнопка на лицевой панели нажата.
_BOOL Dnepr_GPIO_FrontPanel_Button_Pressed() ;

#endif /* T8_DNIEPER_GPIO_H_ */
