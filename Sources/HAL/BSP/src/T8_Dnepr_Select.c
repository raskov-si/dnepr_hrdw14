/*!
\file T8_Dnepr_Select.c
\brief  од дл€ работы с расширител€ми портов дл€ SELECT и PRESENT.
\author <a href="mailto:baranovm@t8.ru">Ѕаранов ћ. ¬.</a>
\date oct 2013
*/


#include "HAL/BSP/inc/T8_Dnepr_Select.h"
#include "HAL/BSP/inc/T8_Dnepr_I2C.h"

#define TCA9539_SELECT_ADDR		0xEA

//////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_InitSelect()
{
	size_t i ;
	u16 dump_val ;
	// включаем все порты на запись
	// !!! здесь используетс€ Dnepr_I2C_Get_PMBUS_INT_Driver, чтобы мьютекс I2C блокировалс€
	// в обычной работе он блокироватьс€ не должен
	TCA9539_SetDirectionRead( Dnepr_I2C_Get_PMBUS_INT_Driver(), TCA9539_SELECT_ADDR, 0x0000 );
	// читаем содержимое порта, чтобы сн€ть возможные прерывани€
	for( i = 0; i < 3; i++ ){
		// здесь тоже надо использовать Dnepr_I2C_Get_PMBUS_INT_Driver
		if( TCA9539_ReadGPIO( Dnepr_I2C_Get_PMBUS_INT_Driver(), TCA9539_SELECT_ADDR, &dump_val ) ){
			break ;
		}
	}
	// по умолчанию выбираем таймыр
	Dnepr_Select( SELECT_SLOT_FAN, NULL );
}


static Dnepr_Select_t __cur_select = SELECT_SLOT_NONE ;
_BOOL Dnepr_Select( const Dnepr_Select_t select, _BOOL *swtchd )
{
	_BOOL ret ;
	if( select == __cur_select ){
		if( swtchd ){
			*swtchd = FALSE ;
		}
		return TRUE ;
	}
	switch( select ){
		case SELECT_SLOT_0 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8004 );
		break;
		case SELECT_SLOT_1 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8008 );
		break;
		case SELECT_SLOT_2 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8010 );
		break;
		case SELECT_SLOT_3 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8020 );
		break;
		case SELECT_SLOT_4 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8040 );
		break;
		case SELECT_SLOT_5 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8080 );
		break;
		case SELECT_SLOT_6 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8100 );
		break;
		case SELECT_SLOT_7 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8200 );
		break;
		case SELECT_SLOT_8 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0xC000 );
		break;
		case SELECT_SLOT_9 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0xA000 );
		break;
		case SELECT_SLOT_10 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x9000 );
		break;
		case SELECT_SLOT_11 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8800 );
		break;
		case SELECT_SLOT_12 :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8400 );
		break;
		case SELECT_SLOT_FAN :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x0000 );
		break;
		case SELECT_SLOT_ALL :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x7FFC );
		break;
		case SELECT_SLOT_NONE :
			ret = TCA9539_WriteGPIO( Dnepr_I2C_Get_PMBUS_INT_SELECT_Driver(), TCA9539_SELECT_ADDR, 0x8000 );
		break;
	}
	__cur_select = select ;
	if( swtchd ){
		*swtchd = TRUE ;
	}
	return ret ;
}
