/*!
\file T8_Dnepr_EdgePort.h
\brief Модуль с обработчиками внешних прерываний (ПЛИС, PMBus, свитчи Marvell)
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date nov 2012
*/

#ifndef __DNEPR_EDGEPORT_H_
#define __DNEPR_EDGEPORT_H_

//! инициализирует внешние прерывания
void Dnepr_EdgePortInit() ;
/// TRUE если вывод прерывания по прежнему в низком уровне
_BOOL Dnepr_EdgePort_is_IRQ4_active();

#endif
