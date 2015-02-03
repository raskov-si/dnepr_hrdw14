/*!
\file T8_Dnepr_I2C.h
\brief Функции I2C, абстрагирующие шину I2C Днепра для драйверов микросхем
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#ifndef __DNEPR_I2C_H_
#define __DNEPR_I2C_H_

#include "HAL/IC/inc/PMB_interface.h"
#include "HAL/IC/inc/I2C_interface.h"

#define DNEPR_I2C_NOSLOT	SPI_FPGA_ALL_SLOTS-1 //!< Индекс слота, обозначающий индиферентность транзакции к SELECT'у.

/*! I2C lines (separated by switches and buffers) */
typedef enum __I2C_DNEPR_BusTypedef{
	I2C_DNEPR_PMBUS_EXT		= 0x01, /*!< PMBUS_EXT bus */
	I2C_DNEPR_PMBUS_INT		= 0x02, /*!< PMBUS_INT bus */
	I2C_DNEPR_BP_SERNUM		= 0x03, /*!< Backplane S/N bus */
	I2C_DNEPR_SFP_U			= 0x04, /*!< SFP_U bus */
	I2C_DNEPR_SFP_L			= 0x05, /*!< SFP_L bus */
	I2C_DNEPR_IO_EXPANDER	= 0x06, /*!< I/O Expander bus */
	I2C_DNEPR_PSU			= 0x07,	//< канал I2C для блоков питания
	I2C_DNEPR_NONE			= 0xFF
}I2C_DNEPR_BusTypedef;

void Dnepr_I2C_init() ;

//! включает соответствующие каналы на свитчах I2C
I2C_DNEPR_BusTypedef I2C_DNEPR_SelectBus(I2C_DNEPR_BusTypedef mBusName) ;
//! возвращает номер канала, который сейчас включен
I2C_DNEPR_BusTypedef I2C_Dnepr_CurrentBus();

I2C_PeriphInterfaceTypedef *Dnepr_Internal_DS28CM00_I2C_handle();

//! читает из Backplane'а адрес устройства, выставившего ALARM
u8 Dnepr_I2C_Read_ARA() ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// функции, возвращают указатели на заполненные структуры специально для доступа
// в указанные в их названиях каналы

PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_EXT_Driver( const u8 nSlot ) ;
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_PSU_Driver() ;
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_PSU_Driver() ;
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_INT_Driver() ;
// драйвер i2c для расширителя портов для select -- не блокирует мьютекс i2c
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver() ;

I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_BP_SerNum_Driver() ;
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_IO_Expander_Driver() ;
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_SFP_U_Driver() ;
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_SFP_L_Driver() ;
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_INT_Driver() ;
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_BP_Driver( const u8 nSlot ) ;
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_9544A_Driver() ;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define I2C_DNEPR_SWITCH_2_ADDRESS    0xEE
#define I2C_DNEPR_IO_EXPANDER_ADDRESS ((0x27)<<1) 
#define I2C_DNEPR_HSW_ADDRESS         0x98

#define I2C_DNEPR_IO_EXP_CH    TI_PCA9544A_Channel0
#define I2C_DNEPR_SFP_U_CH     TI_PCA9544A_Channel1
#define I2C_DNEPR_SFP_L_CH     TI_PCA9544A_Channel2


#endif
