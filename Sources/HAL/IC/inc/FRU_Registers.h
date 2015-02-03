/*!
\file FRU_Registers.h
\brief FRU registers and coefficients (Intel IPMI v1.0).
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 16.04.2012
*/

#ifndef __FRU_REGISTERS_H
#define __FRU_REGISTERS_H

#define FRU_MFR_NAME_BASE   0x2C
#define FRU_MFR_NAME_LENGTH 5

#define FRU_PRODUCT_NAME_BASE   0x32
#define FRU_PRODUCT_NAME_LENGTH 12

#define FRU_MODEL_ID_BASE   0x4F
#define FRU_MODEL_ID_LENGTH 4

#define	FRU_UNIQUE_SERIAL_NUMBER_BASE	0x55
#define	FRU_UNIQUE_SERIAL_NUMBER_LENGTH	4

#define FRU_POWER_CAPACITY_BASE  0x65
#define FRU_NOMINAL_VOLTAGE_BASE 0x83
#define FRU_STANDBY_VOLTAGE_BASE 0x95

#endif
