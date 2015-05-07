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
///// функции для одного чипа
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


//! Добавляет или изменяет запись БД в VTU
//! \param vid VLAN ID
//! \param dbnum номер БД (0-255) для разделения MAC-адресов по разным VLAN'ам
//! \param stats состоянив портов в контексте этого VLAN'а
void MV88E6095_AddVTUEntry(   const u8 pcbDevAddr, const u16 vid, const u8 dbnum, const MV88E6095_Ports_VLAN_Status_t* stats )
{
	u16 mv_data;

	if( !stats )
		return ;

	mv_data = VTU_VID_VALID | VTU_VID(vid) ;
	MV88E6095_multichip_smi_write( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_VID, mv_data );

	mv_data = 	VTU_PORT0( stats->port0_tag, stats->port0_state ) | 
				VTU_PORT1( stats->port1_tag, stats->port1_state ) | 
				VTU_PORT2( stats->port2_tag, stats->port2_state ) | 
				VTU_PORT3( stats->port3_tag, stats->port3_state ) ;
	MV88E6095_multichip_smi_write( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_DATA03, mv_data );

	mv_data = 	VTU_PORT4( stats->port4_tag, stats->port4_state ) | 
				VTU_PORT5( stats->port5_tag, stats->port5_state ) | 
				VTU_PORT6( stats->port6_tag, stats->port6_state ) | 
				VTU_PORT7( stats->port7_tag, stats->port7_state ) ;
	MV88E6095_multichip_smi_write( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_DATA47, mv_data );

	mv_data = 	VTU_PORT8( stats->port8_tag, stats->port8_state ) | 
				VTU_PORT9( stats->port9_tag, stats->port9_state ) | 
				VTU_PORT10( stats->port10_tag, stats->port10_state ) ;
	MV88E6095_multichip_smi_write( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_DATA810, mv_data );


	mv_data = VTU_OP_LOAD | VTU_DBNUM(dbnum) | VTU_BUSY;
	MV88E6095_multichip_smi_write( pcbDevAddr, MV88E6095_GLOBAL, MV88E6095_VTU_OPERATION, mv_data );
	// ждём готовность
	do{ 
		HWait(20);
		MV88E6095_multichip_smi_read( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_OPERATION, &mv_data );
	} while( mv_data & VTU_BUSY );
}


//! Устанавливает умолчальный номер VLAN'а для порта
//! \param port_index номер порта
//! \param VID VLAN ID
void MV88E6095_PortDefaultVID( const u8 pcbDevAddr,  const u8 port_index, const u8 force_def_vid, 
								const u16 VID )
{
	u16 mv_data ;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	
	MV88E6095_multichip_smi_read( pcbDevAddr,  dev_addr, MV88E6095_PORT_DEFVLANID, &mv_data );
	mv_data &= ~( FORCE_DEF_VID | 0x0FFF );
	if( force_def_vid > 0 )
		mv_data |= FORCE_DEF_VID ;
	mv_data |= DEF_VID( VID );
	MV88E6095_multichip_smi_write( pcbDevAddr, dev_addr, MV88E6095_PORT_DEFVLANID, mv_data );	
}


void MV88E6095_Change_Port8021Q_state( const u8 pcbDevAddr,  const u8 port_index, const Port8021QState state )
{
	u16 mv_data ;
	u32 dev_addr;
	dev_addr = 0x10 | port_index;
	
	MV88E6095_multichip_smi_read( pcbDevAddr,  dev_addr, MV88E6095_PORT_CTRL2_REG, &mv_data );
	mv_data &= 0xF3FF ; // сбрасываем 10-11е биты
	mv_data |= VLAN_MODE( (u8)state );
	MV88E6095_multichip_smi_write( pcbDevAddr, dev_addr, MV88E6095_PORT_CTRL2_REG, mv_data );
}

//! Читает запись в VTU с соотв. VLAN ID
//! \param vid 		VLAN ID
//! \param dbnum 	сюда запишется dbnum
//! \param stats 	сюда запишутся состояния портов в этов VLAN'е
//! \retval OK или ERROR
u32 MV88E6095_ReadVTUEntry(   const u8 pcbDevAddr, const u16 vid, u8 *dbnum, MV88E6095_Ports_VLAN_Status_t* stats )
{
	u16 i, j, mv_data ;
	i = 0 ;
	// находим соотв. запись
	do {
		j = MV88E6095_NextVTUEntry( pcbDevAddr, i, &i );
	} while( ((i & 0x0FFF) != vid) && (i != 0x0FFF) ) ;
	// закончились записи в VTU -- выходим
	if( !(i & VTU_VID_VALID) || ((i & 0x0FFF) != vid) ){
		return ERROR ;
	}

	// в j dbnum
	if( dbnum ){
		*dbnum = j ;
	}
	if( !stats ){
		return OK ;
	}

	// читаем сами данные
	MV88E6095_multichip_smi_read( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_DATA03, &mv_data );
	stats->port0_tag = 		mv_data & 3 ;
	stats->port0_state = 	(mv_data >> 2) & 3;
	stats->port1_tag = 		(mv_data >> 4) & 3;
	stats->port1_state = 	(mv_data >> 6) & 3;
	stats->port2_tag = 		(mv_data >> 8) & 3;
	stats->port2_state = 	(mv_data >> 10) & 3;
	stats->port3_tag = 		(mv_data >> 12) & 3;
	stats->port3_state = 	(mv_data >> 14) & 3;

	MV88E6095_multichip_smi_read( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_DATA47, &mv_data );
	stats->port4_tag = 		mv_data & 3 ;
	stats->port4_state = 	(mv_data >> 2) & 3;
	stats->port5_tag = 		(mv_data >> 4) & 3;
	stats->port5_state = 	(mv_data >> 6) & 3;
	stats->port6_tag = 		(mv_data >> 8) & 3;
	stats->port6_state = 	(mv_data >> 10) & 3;
	stats->port7_tag = 		(mv_data >> 12) & 3;
	stats->port7_state = 	(mv_data >> 14) & 3;

	MV88E6095_multichip_smi_read( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_DATA810, &mv_data );
	stats->port8_tag = 		mv_data & 3 ;
	stats->port8_state = 	(mv_data >> 2) & 3;
	stats->port9_tag = 		(mv_data >> 4) & 3;
	stats->port9_state = 	(mv_data >> 6) & 3;
	stats->port10_tag = 	(mv_data >> 8) & 3;
	stats->port10_state = 	(mv_data >> 10) & 3;

	return OK ;
}


//! Ищет в бд VTU запись следующую за записью с VID==prevvid, возвращает его DBNum 
//! \param prevvid defines the starting VID to search. Use the last address to find the next address (there is no need to write to this register in this case).
//! \param nextvid если не NULL сюда запишется 
u8 MV88E6095_NextVTUEntry( const u8 pcbDevAddr, u16 prevvid, u16 *nextvid)
{
	u16 mv_data, dbnum;

	// пишем VID
	MV88E6095_multichip_smi_write( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_VID, VTU_VID( prevvid ));
	// команда
	MV88E6095_multichip_smi_write( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_OPERATION, VTU_OP_NEXT | VTU_BUSY );

	// ждём готовность
	do{ 
		HWait(20);
		MV88E6095_multichip_smi_read( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_OPERATION, &mv_data );
	} while( mv_data & VTU_BUSY );
	dbnum = (mv_data & 0x0F) | ((mv_data >> 4) & 0xF0 );

	if( nextvid )
		MV88E6095_multichip_smi_read( pcbDevAddr,  MV88E6095_GLOBAL, MV88E6095_VTU_VID, nextvid );

	return dbnum ;
}
