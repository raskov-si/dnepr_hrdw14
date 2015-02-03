/*!
\file 88E6095.c
\brief Code for configuring and working with Marvell 88E6095 in single and multichip modes, based on Neva's Maravell_88E6095.c
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#include "HAL\IC\inc\MV_88E6095.h"
#include "HAL/MCU/inc/T8_5282_timers.h"
#include "support_common.h"
#include "ctype.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pointer to peripherial access structure and a \c define assigned to it 
static SMI_PeriphInterfaceTypedef* tSMI_PeriphInterface = NULL ;
#define SMI_PERIPH_INTERFACE_STRUCT_PTR tSMI_PeriphInterface

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief Инициализазирует интерфейс SMI
//! \param _smi_iface указатель на структуру с указателями на функции SMI
void MV_88E6095_InitPeripheralInterface( SMI_PeriphInterfaceTypedef* _smi_iface )
{
	assert( _smi_iface != NULL ) ;
	SMI_PERIPH_INTERFACE_STRUCT_PTR = _smi_iface ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*!
\brief Пишет в smi MV88E6095 в multichip addressing mode
\param pcbDevAddr адрес smi нужной МК (определяется подтяжкой соотв. выводов порта 10)
\param virtDevAddr адрес внутри МК в singlechip mode
\param regAddr адрес регистра
\param data сами данные
*/
void MV88E6095_multichip_smi_write(	const u8 pcbDevAddr, const u8 virtDevAddr,
									const u8 regAddr, const u16 data )
{
	
	register u16 command_reg_val = (u16)MV88E6095_CMD_REG__REGADDR( regAddr );
	command_reg_val |= MV88E6095_CMD_REG__DEVADDR( virtDevAddr );
	command_reg_val |= MV88E6095_CMD_REG__SMIOP_WR ;
	command_reg_val |= MV88E6095_CMD_REG__SMIMODE_22 ;
	command_reg_val |= MV88E6095_CMD_REG__SMIBUSY ;

	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write( (u32)pcbDevAddr,  (u32)MV88E6095_DATA_REG, data );
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write( (u32)pcbDevAddr,  (u32)MV88E6095_CMD_REG, command_reg_val );
}

/*!
\brief Читает из smi MV88E6095 в multichip addressing mode
\param pcbDevAddr адрес smi нужной МК (определяется подтяжкой соотв. выводов порта 10)
\param virtDevAddr адрес внутри МК в singlechip mode
\param regAddr адрес регистра
\param data сами данные
\retval OK/ERROR
*/
u32 MV88E6095_multichip_smi_read(	const u8 pcbDevAddr, const u8 virtDevAddr,
									const u8 regAddr, u16* data )
{
	u16 	usBuffer;
	register u16 command_reg_val;

	// сочиняем SMI Command Register
	
	command_reg_val = (u16)MV88E6095_CMD_REG__REGADDR( regAddr );
	command_reg_val |= MV88E6095_CMD_REG__DEVADDR( virtDevAddr );
	command_reg_val |= MV88E6095_CMD_REG__SMIOP_RD ;
	command_reg_val |= MV88E6095_CMD_REG__SMIMODE_22 ;
	command_reg_val |= MV88E6095_CMD_REG__SMIBUSY ;

	// ждём пока прочитает (при чтении с другого чипа всё это займёт
	// не многим меньше 100 мкс, для рантайма надо придумать что-нибудь)
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write( (u32)pcbDevAddr,  (u32)MV88E6095_CMD_REG, command_reg_val );
	do {
		SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read((u32)pcbDevAddr, (u32)MV88E6095_CMD_REG, &usBuffer);
	}
	while ((usBuffer & MV88E6095_CMD_REG__SMIBUSY) > 0);

	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read( (u32)pcbDevAddr, (u32)MV88E6095_DATA_REG, data );
	return OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MV88E6095_InitPort(u8 port_index) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_CTRL_REG, &mv_data);
	mv_data = (mv_data & ~(u16)PORT_STATE(0x3)) | PORT_STATE(PORT_FORWARDING);
	mv_data |= FORWARD_UNKNOWN;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(dev_addr, MV88E6095_PORT_CTRL_REG, mv_data);

	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_CTRL2_REG, &mv_data);
	mv_data |= MAP_DA | DEFAULT_FORWARD;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(dev_addr, MV88E6095_PORT_CTRL2_REG, mv_data);
	//PHY: energy detect disable UAV
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(port_index,0x10,&mv_data);
	mv_data &= ~(1<<14);
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(port_index,0x10,mv_data);
}

void MV88E6095_EnablePort(u8 port_index) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_CTRL_REG, &mv_data);
	mv_data = (mv_data & ~(u16)PORT_STATE(0x3)) | PORT_STATE(PORT_FORWARDING);
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(dev_addr, MV88E6095_PORT_CTRL_REG, mv_data);
}

void MV88E6095_DisablePort(u8 port_index) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_CTRL_REG, &mv_data);
	mv_data = (mv_data & ~(u16)PORT_STATE(0x3)) | PORT_STATE(PORT_DISABLED);
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(dev_addr, MV88E6095_PORT_CTRL_REG, mv_data);
}


void MV88E6095_SetPortState(u8 port_index, u8 port_state) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_CTRL_REG, &mv_data);
	mv_data = (mv_data & ~(u16)PORT_STATE(0x3)) | PORT_STATE(port_state);
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(dev_addr, MV88E6095_PORT_CTRL_REG, mv_data);
}

u8 MV88E6095_GetPortState(u8 port_index) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_CTRL_REG, &mv_data);
	return (mv_data & (u16)PORT_STATE(0x3));
}


u8 MV88E6095_Link(u8 port_index) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_STATUS_REG, &mv_data);
	if (mv_data & MV88E6095_LINK)
		return 1;
	else
		return 0;
}

u32 MV88E6095_Speed(u8 port_index) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_STATUS_REG, &mv_data);
	switch (MV88E6095_SPEED(mv_data)) {
	case 0x0: return 10;
	case 0x1: return 100;
	case 0x2: return 1000;
	}
	return 0;
}

u32 MV88E6095_Duplex(u8 port_index) {
	u16 mv_data;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(dev_addr, MV88E6095_PORT_STATUS_REG, &mv_data);
	return MV88E6095_DUPLEX(mv_data);
}


u32 MV88E6095_FlushATUEntries() {
	u16 mv_data;
	u32 flag=ERROR;
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(MV88E6095_GLOBAL, MV88E6095_ATU_OPER_REG, &mv_data);
	if (mv_data & MV88E6095_ATUBusy) {
		HWait(20);
		SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(MV88E6095_GLOBAL, MV88E6095_ATU_OPER_REG, &mv_data);
		if (mv_data & MV88E6095_ATUBusy)
			flag = ERROR;
		else
			flag = OK;
	}
	else
		flag = OK;
	if (flag == OK) {
		mv_data = MV88E6095_ATUBusy | MV88E6095_ATUOp(MV88E6095_FLUSH_ALL_ENTRIES);
		SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(MV88E6095_GLOBAL, MV88E6095_ATU_OPER_REG, mv_data);
	}
	return flag;
}

/*
void MV88E6095_() {
	SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(MV88E6095_GLOBAL);
}
*/
/*============================= UAV added ============================================*/
void MV88E6095_SetMacAddr(u8* maddr){
	u16 i,w;
	for(i=0;i<3;i++){
		w = (u16)(maddr[i*2]<<8) | (u16)maddr[i*2+1];
//		if(i == 0) w |= (u16)0x0100;//each port will have unique mac_addr
		SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Write(MV88E6095_GLOBAL, (u32)(i+1), w);
	}
}
void MV88E6095_GetMacAddr(u8* maddr){
	u16 i,w;
	for(i=0;i<3;i++){
		SMI_PERIPH_INTERFACE_STRUCT_PTR->SMI_Read(MV88E6095_GLOBAL, (u32)(i+1), &w);
		if(i == 0) w &= (u16)0xFEFF;//clear "DiffAddr" bit
		maddr[i*2]= (u8)(w>>8);
		maddr[i*2+1] = (u8)w;
	}
}
