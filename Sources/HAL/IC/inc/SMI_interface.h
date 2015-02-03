/*!
\file SMI_interface.h
\brief Базовый требуемый интерфейс драйверов микросхем с SMI/MDIO
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#ifndef __SMI_INTERFACE_H_
#define __SMI_INTERFACE_H_

typedef struct __SMI_PeriphInterfaceTypedef {
	u32 (*SMI_Read)		(u32 phy_addr, u32 reg_addr, u16* data) ;
	u32 (*SMI_Write)		(u32 phy_addr, u32 reg_addr, u16 data) ;
} SMI_PeriphInterfaceTypedef ;

#endif // __SMI_INTERFACE_H_
