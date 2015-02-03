/*!
\file Dnepr_I2C_Interface_Funcs_templates.h
\brief Шаблоны для функций I2C
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date aug 2013
*/

#define I2C_FUNC_DECL( name, args ) static _BOOL name( args );
#define I2C_FUNC_DEF( name, args, func_2_call, channel ) static _BOOL 	\
			name( args ){												\
				_BOOL ret ;												\
				T8_Dnepr_TS_I2C_Lock();									\
				if( I2C_Dnepr_CurrentBus() != channel )					\
					I2C_DNEPR_SelectBus( channel );						\
				ret = func_2_call ;										\
				T8_Dnepr_TS_I2C_Unlock() ;								\
				return ret ;											\
			}
