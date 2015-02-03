/*!
\file T8_Dnepr_I2C.c
\brief Функции I2C, абстрагирующие шину I2C Днепра для драйверов микросхем
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/BSP/inc/T8_Dnepr_FPGA.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Select.h"
#include "HAL/IC/inc/TI_PCA9544A.h"

#include "HAL/MCU/inc/PMBus_GenericDriver.h"
#include "HAL/MCU/inc/T8_5282_i2c.h"
#include "HAL/MCU/inc/I2C_GenericDriver.h"

#include "Application/inc/T8_Dnepr_TaskSynchronization.h"

#include "uCOS_II.H"

//! структура, указатель на которую кладётся в PMB_PeriphInterfaceTypedef::bus_info
//! и I2C_PeriphInterfaceTypedef::bus_info
typedef struct __Dnepr_I2C_driver_internal_info {
	I2C_DNEPR_BusTypedef bus_channel ; //!< номер канала I2C, который нужно включить перед транзакцией
	//! если TRUE, значит функцию драйвера шины вызвали из I2C_DNEPR_SelectBus,
	//! чтобы переключить шину, и значит мьютекс уже заблокирован и его блокировать
	//! не нужно
	_BOOL dont_block ;
	//! номер слота, SELECT которого надо включить в FPGA
	u8 nSelect ;
} Dnepr_I2C_driver_internal_info ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_I2C_init()
{
	I2C_InitPorts();
	I2CInit_Timer( (I2CWaitTimer_t){ (u32 (*)())OSTimeGet, 10 } ); // 10 тиков таймера -- 10 мс ждём повисвший i2c
}



static I2C_DNEPR_BusTypedef __Bus_Switch_curState ;

I2C_DNEPR_BusTypedef I2C_DNEPR_SelectBus(I2C_DNEPR_BusTypedef mBusName)
{
	/*!
	\brief Configures switch devices to connect certain bus to MCU.
	\param mBusName Bus to connect (see \ref I2C_DNEPR_BusTypedef).
	\retval Previous switched channel
	\todo Everything about Power sequencer and Hot-Swap controller.
	*/
	I2C_DNEPR_BusTypedef prevBusState = __Bus_Switch_curState ;
	__Bus_Switch_curState = mBusName ;

	switch(mBusName){
	
	case I2C_DNEPR_PMBUS_EXT:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_5 );
		break;
		
	case I2C_DNEPR_PMBUS_INT:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_3 );
		break;
	
	case I2C_DNEPR_BP_SERNUM:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_2 );
		break;	
		
	case I2C_DNEPR_SFP_U:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_1 );
		TI_PCA9544A_EnableChannel( Dnepr_I2C_Get_I2C_9544A_Driver(), I2C_DNEPR_SWITCH_2_ADDRESS, I2C_DNEPR_SFP_U_CH);
		break;
		
	case I2C_DNEPR_SFP_L:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_1 );
		TI_PCA9544A_EnableChannel( Dnepr_I2C_Get_I2C_9544A_Driver(), I2C_DNEPR_SWITCH_2_ADDRESS, I2C_DNEPR_SFP_L_CH);
		break;		
		
	case I2C_DNEPR_IO_EXPANDER:
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_1 );
		TI_PCA9544A_EnableChannel( Dnepr_I2C_Get_I2C_9544A_Driver(), I2C_DNEPR_SWITCH_2_ADDRESS, I2C_DNEPR_IO_EXP_CH);
		break;
		
	case I2C_DNEPR_PSU :
		Dnepr_GPIO_select_i2c_channel( GPIO_I2C_CHANNEL_7 );
		break ;
	}
    return prevBusState ;
}

/*!
\brief Returns current switched channel on i2c bus
\retval Current switched channel
*/
I2C_DNEPR_BusTypedef I2C_Dnepr_CurrentBus()
{
	return __Bus_Switch_curState ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u8 Dnepr_I2C_Read_ARA()
{
	u8 ret ;
	u8 i ;
	_BOOL swtch ;
	T8_Dnepr_TS_I2C_Lock() ;
	// 5 раза пытаемся включить нужный select
	for( i = 5 ; i < 3; i++ ){
		if( Dnepr_Select( SELECT_SLOT_ALL, &swtch ) ){
			break ;
		}
	}
	// если было переключение (до этого SELECT был на другом слоте) --
	// делаем задержку, чтобы свитч i2c успел переключиться
	if( swtch ){
		OSTimeDly( 1 );
	}
	if( I2C_Dnepr_CurrentBus() != I2C_DNEPR_PMBUS_EXT )
		I2C_DNEPR_SelectBus( I2C_DNEPR_PMBUS_EXT );

	I2C_ReadCommand( 0x18, &ret ) ;
	T8_Dnepr_TS_I2C_Unlock() ;
	
	return ret ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// функции обёртки над PMB_PeriphInterfaceTypedef, блокируют мьютекс и включают
// нужный канал i2c

#define PMBUS_TRANSACTION_BEGIN()	_BOOL   ret ;																				\
									_BOOL   swtch ;																				\
									u8		i ;																					\
									const u8 slot_num = ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->nSelect ;				\
									assert( p );																				\
									if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){						\
										T8_Dnepr_TS_I2C_Lock() ;																\
									}																							\
									/* надо переключить SELECT */ 																\
									if( slot_num != DNEPR_I2C_NOSLOT ){															\
										for( i = 0; i < 5; i++ ){																\
											if( Dnepr_Select( slot_num, &swtch ) ){												\
												break ;																			\
											}																					\
										}																						\
										/* если правда переключали -- вставляем паузу */										\
										if( swtch ){																			\
											OSTimeDly( 1 );																		\
										}																						\
									}																							\
									if( I2C_Dnepr_CurrentBus() != ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel )\
										I2C_DNEPR_SelectBus( ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel );


#define PMBUS_TRANSACTION_END()																									\
									if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){						\
										T8_Dnepr_TS_I2C_Unlock() ;																\
									}																							\
									return ret ;



static _BOOL
__PMB_GetAcknowledge(PMB_PeriphInterfaceTypedef *p, u8 mAddr )
{
	PMBUS_TRANSACTION_BEGIN();
	ret = PMB_GetAcknowledge( mAddr );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadByte(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *pbResult )
{
	PMBUS_TRANSACTION_BEGIN();
	ret = PMB_ReadByte( mAddr, mCmd, pbResult );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadWord(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 *pwResult )
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_ReadWord( mAddr, mCmd, pwResult );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteByte(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 nData)
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_WriteByte( mAddr, mCmd, nData );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteWord(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 nData)
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_WriteWord( mAddr, mCmd, nData );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_SendCommand(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd)
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_SendCommand( mAddr, mCmd);
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadMultipleBytes(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity)
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_ReadMultipleBytes( mAddr, mCmd, anData, nBytesQuantity);
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteMultipleBytes(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity)
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_WriteMultipleBytes( mAddr, mCmd, anData, nBytesQuantity );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_ReadMultipleWords(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity)
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_ReadMultipleWords( mAddr, mCmd, anData, nBytesQuantity );
	PMBUS_TRANSACTION_END() ;
}

static _BOOL
__PMB_WriteMultipleWords(PMB_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nBytesQuantity)
{
	PMBUS_TRANSACTION_BEGIN() ;
	ret = PMB_WriteMultipleWords( mAddr, mCmd, anData, nBytesQuantity );
	PMBUS_TRANSACTION_END() ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

#define I2C_TRANSACTION_BEGIN() _BOOL ret ; 																			\
	u8 i ;																												\
	_BOOL swtch ;																										\
	I2C_DNEPR_BusTypedef channel_switch_to	=	((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel ;			\
	u8 slot_num = ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->nSelect ;											\
	assert( p );																										\
	if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){												\
		T8_Dnepr_TS_I2C_Lock() ;																						\
	}																													\
	if( slot_num != DNEPR_I2C_NOSLOT ){																					\
		for( i = 0; i < 5; i++ ){																\
			if( Dnepr_Select( slot_num, &swtch ) ){												\
				break ;																			\
			}																					\
		}																						\
		/* если правда переключали -- вставляем паузу */										\
		if( swtch ){																			\
			OSTimeDly( 1 );																		\
		}																						\
	}																													\
	if( (channel_switch_to != 0) && (I2C_Dnepr_CurrentBus() != channel_switch_to) )										\
		I2C_DNEPR_SelectBus( ((Dnepr_I2C_driver_internal_info*)(p->bus_info))->bus_channel );
								
#define I2C_TRANSACTION_END()																							\
	if( !((Dnepr_I2C_driver_internal_info*)(p->bus_info))->dont_block ){												\
		T8_Dnepr_TS_I2C_Unlock() ;																						\
	}																													\
	return ret ;

static _BOOL
__I2C_ReadByte( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 *pbResult )
{
	I2C_TRANSACTION_BEGIN();
	ret = I2C_ReadByte( mAddr, mCmd, pbResult ) ;
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadWord( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 *pwResult )
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_ReadWord( mAddr, mCmd, pwResult );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteByte( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8 nData)
{
	I2C_TRANSACTION_BEGIN();
	ret =   I2C_WriteByte( mAddr, mCmd, nData);
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteWord( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16 nData)
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_WriteWord( mAddr, mCmd, nData );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_SendCommand( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd)
{
	I2C_TRANSACTION_BEGIN();
	ret =   I2C_SendCommand( mAddr, mCmd );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadCommand( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 *pbResult )
{
	I2C_TRANSACTION_BEGIN();
	ret = I2C_ReadCommand( mAddr, pbResult );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadMultipleBytes( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity)
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_ReadMultipleBytes( mAddr, mCmd, anData, nBytesQuantity );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteMultipleBytes( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u8* anData, u8 nBytesQuantity)
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_WriteMultipleBytes( mAddr, mCmd, anData, nBytesQuantity );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadMultipleBytes_16( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity)
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_ReadMultipleBytes_16( mAddr, mCmd, anData, nBytesQuantity );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteMultipleBytes_16( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u16 mCmd, u8* anData, u8 nBytesQuantity)
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_WriteMultipleBytes_16( mAddr, mCmd, anData, nBytesQuantity );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_ReadMultipleWords( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity)
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_ReadMultipleWords( mAddr, mCmd, anData, nWordsQuantity );
	I2C_TRANSACTION_END();
}

static _BOOL
__I2C_WriteMultipleWords( I2C_PeriphInterfaceTypedef *p, u8 mAddr, u8 mCmd, u16* anData, u8 nWordsQuantity)
{
	I2C_TRANSACTION_BEGIN();
	ret =  I2C_WriteMultipleWords( mAddr, mCmd, anData, nWordsQuantity );
	I2C_TRANSACTION_END();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// функции, возвращают указатели на заполненные структуры специально для доступа
// в указанные в их названиях каналы

//! конструирует I2C_PeriphInterfaceTypedef для устройств на PMBus
#define DEFINE_PMBus_Struct(name, bus_info) static		 						\
PMB_PeriphInterfaceTypedef name =												\
{ 	__PMB_ReadByte, __PMB_ReadWord, __PMB_WriteByte, __PMB_WriteWord, 			\
	__PMB_SendCommand, __PMB_ReadMultipleBytes, __PMB_WriteMultipleBytes,		\
	__PMB_ReadMultipleWords, __PMB_WriteMultipleWords, __PMB_GetAcknowledge, bus_info }

Dnepr_I2C_driver_internal_info __pmb_businfo_ext = { I2C_DNEPR_PMBUS_EXT, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_ext, &__pmb_businfo_ext );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_EXT_Driver( const u8 nSlot )
{
	assert( nSlot < I2C_DNEPR_NUMBER_OF_SLOTS );
	((Dnepr_I2C_driver_internal_info*)(__pmb_ext.bus_info))->nSelect = nSlot ;
	return &__pmb_ext ;
}

Dnepr_I2C_driver_internal_info __pmb_businfo_psu = { I2C_DNEPR_PSU, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_psu, &__pmb_businfo_psu );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_PSU_Driver()
{	return &__pmb_psu ; }

Dnepr_I2C_driver_internal_info __pmb_businfo_int = { I2C_DNEPR_PMBUS_INT, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_int, &__pmb_businfo_int );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_INT_Driver()
{	return &__pmb_int ; }

Dnepr_I2C_driver_internal_info __pmb_businfo_int_select = { I2C_DNEPR_PMBUS_INT, TRUE, DNEPR_I2C_NOSLOT };
DEFINE_PMBus_Struct( __pmb_int_select, &__pmb_businfo_int_select );
PMB_PeriphInterfaceTypedef *Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver()
{	return &__pmb_int_select ; }


//! конструирует I2C_PeriphInterfaceTypedef для устройств на I2C
#define DEFINE_I2C_Struct(name, bus_info) static		 						\
I2C_PeriphInterfaceTypedef name =												\
{ 	__I2C_ReadByte, __I2C_ReadWord, __I2C_WriteByte, __I2C_WriteWord, 			\
	__I2C_SendCommand, __I2C_ReadCommand, __I2C_ReadMultipleBytes, 				\
	__I2C_WriteMultipleBytes, __I2C_ReadMultipleWords, __I2C_WriteMultipleWords,\
	__I2C_ReadMultipleBytes_16, __I2C_WriteMultipleBytes_16, 					\
	bus_info }

Dnepr_I2C_driver_internal_info __i2c_businfo_bp_sernum = { I2C_DNEPR_BP_SERNUM, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_bp_sernum, &__i2c_businfo_bp_sernum );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_BP_SerNum_Driver()
{	return &__i2c_bp_sernum ; }

Dnepr_I2C_driver_internal_info __i2c_businfo_io_expander = { I2C_DNEPR_IO_EXPANDER, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_io_expander, &__i2c_businfo_io_expander );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_IO_Expander_Driver()
{	return &__i2c_io_expander ; }

Dnepr_I2C_driver_internal_info __i2c_businfo_sfp_u = { I2C_DNEPR_SFP_U, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_sfp_u, &__i2c_businfo_sfp_u );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_SFP_U_Driver()
{	return &__i2c_sfp_u ; }

Dnepr_I2C_driver_internal_info __i2c_businfo_sfp_l = { I2C_DNEPR_SFP_L, FALSE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_sfp_l, &__i2c_businfo_sfp_l );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_SFP_L_Driver()
{	return &__i2c_sfp_l ; }

DEFINE_I2C_Struct( __i2c_ext, &__pmb_businfo_int );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_INT_Driver()
{	return &__i2c_ext ; }

DEFINE_I2C_Struct( __i2c_bp, &__pmb_businfo_ext );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_BP_Driver( const u8 nSlot )
{
	assert( nSlot < I2C_DNEPR_NUMBER_OF_SLOTS );
	((Dnepr_I2C_driver_internal_info*)(__i2c_bp.bus_info))->nSelect = nSlot ;
	return &__i2c_bp ;
}

Dnepr_I2C_driver_internal_info __i2c_businfo_9544A = { 0, TRUE, DNEPR_I2C_NOSLOT };
DEFINE_I2C_Struct( __i2c_9544A, &__i2c_businfo_9544A );
I2C_PeriphInterfaceTypedef *Dnepr_I2C_Get_I2C_9544A_Driver()
{	return &__i2c_9544A ; }
