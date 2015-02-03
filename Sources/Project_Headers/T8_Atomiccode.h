/*!
\file critsection.h
\brief Macroses for making atomic code. Changed 
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date may 2012
*/

#ifndef CRITSECTION_H_
#define CRITSECTION_H_

#include "HAL/MCU/inc/T8_5282_interrupts.h"
#include "OS_CPU.H"

// Предполагается к использованию на уровне application,
// в uC/OS-II свои макросы, взятые из примера

#if OS_CRITICAL_METHOD == 3
#define	STORAGE_ATOMIC()	 OS_CPU_SR  cpu_sr = 0u;  // TODO: надо бы перейти на хранение sr в стеке
#endif

#define	START_ATOMIC()		OS_ENTER_CRITICAL()
#define	STOP_ATOMIC()		OS_EXIT_CRITICAL()

#endif /* CRITSECTION_H_ */
