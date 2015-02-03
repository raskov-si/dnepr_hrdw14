/*!
\file T8_88E6095.h
\brief Code for configuring and working with Marvell 88E6095 in single and multichip modes, based on Neva's Maravell_88E6095.h
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/

#ifndef MARAVELL_88E6095_H_
#define MARAVELL_88E6095_H_

#include "support_common.h"
#include "HAL/IC/inc/SMI_interface.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Интерфейс ввода/вывода по SMI в MV88E6095

//! Таймаут ожидания доступности данных в SMI Data Register (см. MV88E6095_smi_read)
#define MV88E6095_TIMEOUT		10

/*!
\defgroup MV88E6095_SMI_defines
\brief Адреса чипов по SMI
\{
*/

//! адрес SMI Command Register
#define MV88E6095_CMD_REG					0x0
//! SMI Command Register -- SMI Unit Busy 
#define MV88E6095_CMD_REG__SMIBUSY			0x8000
//! SMI Command Register -- clause 22 (без включения -- clause 45)
#define MV88E6095_CMD_REG__SMIMODE_22		0x1000
//! SMI Command Register -- reading
#define MV88E6095_CMD_REG__SMIOP_RD			0x0800
//! SMI Command Register -- writing
#define MV88E6095_CMD_REG__SMIOP_WR			0x0400
//! SMI Command Register -- адрес устройства в singlechip addressing mode
#define MV88E6095_CMD_REG__DEVADDR(x)		(0x3E0 & (((u16)(x))<<5))
//! SMI Command Register -- адрес регистра в singlechip addressing mode
#define MV88E6095_CMD_REG__REGADDR(x)		((x)&0x1F)

//! адрес SMI Data Register
#define MV88E6095_DATA_REG				0x1

/*!
\}
*/ //MV88E6095_SMI_defines

#define MV88E6095_PORT_STATUS_REG		0x0
#define MV88E6095_PORT_STATUS_REG__PAUSE_EN		0x8000
#define MV88E6095_LINK			0x0800
#define MV88E6095_SPEED(x)		(((x)&0x300)>>8)
#define MV88E6095_DUPLEX(x)		(((x)&0x400)>>10)

#define MV88E6095_PCS_CTRL_REG		        (0x01)
    #define MV88E6095_PCS_CTRL_REG_NOT_FORCE_SPD            (0x03)
    #define MV88E6095_PCS_CTRL_REG_FORCE_LINK	            (0x10)
    #define MV88E6095_PCS_CTRL_REG_LINK_FORCED_VALUE(x)     ((1&(x))<<5)

    #define MV88E6095_PCS_CTRL_REG_FORCE_SPEED_MASK         (3u<<0)
    #define MV88E6095_PCS_CTRL_REG_FORCE_SPEED(x)           ((3u&(x))<<0)
         #define MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_10B       (0u)
         #define MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_100B      (1u)
         #define MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_1000B     (2u)
         #define MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_AUTO      (3u)

    #define MV88E6095_PCS_CTRL_REG_AUTONEG_ENABLE           (1u<<10)


#define MV88E6095_STATS_OP_REG	0x1D
#define MV88E6095_STATS_OP_REG__STATSBUSY	0x8000
#define MV88E6095_STATS_OP_REG__STATSOP(x) (((x)&0x7) << 12)
#define MV88E6095_STATS_OP_REG__STATSPTR(x) ((x)&0x3F)
#define CAPTURE_ALL_COUNTERS	0x5
#define READ_COUNTER			0x4
#define INGOODOCTETSLO	0x00
#define OUTOCTETSLO		0x0E

#define MV88E6095_STATS_CNT32_REG		0x1E
#define MV88E6095_STATS_CNT10_REG		0x1F

#define MV88E6095_PORT_CTRL_REG			0x4
#define TRANSMIT_FRAMES_UNTAGGED		0x1000
#define FORWARD_UNKNOWN					0x04
#define DSA_TAG 						0x0100
#define PORT_STATE(x)					((x)&0x3)
#define PORT_FORWARDING					0x3
#define PORT_DISABLED					0x0

#define MV88E6095_PORT_CTRL2_REG		0x8
#define CPU_PORT(x)						(((x)-0x10)&0xF)
#define DEFAULT_FORWARD					0x40
#define MAP_DA							0x80


#define MV88E6095_MGMT_ENABLE_REG		0x03

#define MV88E6095_MGMT_REG				0x05
#define RSVD2CPU						0x08

#define MV88E6095_PCS_CTRL_REG			0x01
#define NOT_FORCE_SPD	0x03
#define FORCE_LINK		0x10
#define LINK_FORCED_VALUE(x)	((1&(x))<<5)

#define MV88E6095_ATU_CTRL_REG	0x0A
#define AGE_TIME(x)	((((x)/15)&0xFF)<<4)

#define MV88E6095_ATU_OPER_REG	0x0B
#define MV88E6095_ATUBusy	0x8000
#define MV88E6095_ATUOp(x)	((x)&0x7)<<12
#define MV88E6095_FLUSH_ALL_ENTRIES 0x1

#define MV88E6095_PORT0		0x10
#define MV88E6095_PORT1		0x11
#define MV88E6095_PORT2		0x12
#define MV88E6095_PORT3		0x13
#define MV88E6095_PORT4		0x14
#define MV88E6095_PORT5		0x15
#define MV88E6095_PORT6		0x16
#define MV88E6095_PORT7		0x17
#define MV88E6095_PORT8		0x18
#define MV88E6095_PORT9		0x19
#define MV88E6095_PORT10	0x1A

#define MV88E6095_GLOBAL	0x1B
#define MV88E6095_GLOBAL2	0x1C

#define MV88E6095_PORT_DISABLED	0x0
#define MV88E6095_PORT_LISTENING	0x1
#define MV88E6095_PORT_LEARNING	0x2
#define MV88E6095_PORT_FORWARDING	0x3

//! \brief Инициализазирует интерфейс SMI
//! \param _smi_iface указатель на структуру с указателями на функции SMI
void MV_88E6095_InitPeripheralInterface( SMI_PeriphInterfaceTypedef* _smi_iface );

/*!
\brief Пишет в smi MV88E6095 в multichip addressing mode
\param pcbDevAddr адрес smi нужной МК (определяется подтяжкой соотв. выводов порта 10)
\param virtDevAddr адрес внутри МК в singlechip mode
\param regAddr адрес регистра
\param data сами данные
*/
void MV88E6095_multichip_smi_write(	const u8 pcbDevAddr, const u8 virtDevAddr,
									const u8 regAddr, const u16 data );
/*!
\brief Читает из smi MV88E6095 в multichip addressing mode
\param pcbDevAddr адрес smi нужной МК (определяется подтяжкой соотв. выводов порта 10)
\param virtDevAddr адрес внутри МК в singlechip mode
\param regAddr адрес регистра
\param data сами данные
\retval OK/ERROR
*/
u32 MV88E6095_multichip_smi_read(	const u8 pcbDevAddr, const u8 virtDevAddr,
									const u8 regAddr, u16* data );

void MV88E6095_EnablePort(u8 port_index);
void MV88E6095_InitPort(u8 port_index);
void MV88E6095_DisablePort(u8 port_index);
void MV88E6095_SetPortState(u8 port_index, u8 port_state);
u8 MV88E6095_GetPortState(u8 port_index);
u8 MV88E6095_Link(u8 port_index);
u32 MV88E6095_Speed(u8 port_index);
u32 MV88E6095_FlushATUEntries();
u32 MV88E6095_Duplex(u8 port_index);

void MV88E6095_SetMacAddr(u8* maddr);
void MV88E6095_GetMacAddr(u8* maddr);


typedef struct {
	u8 body[4];
} FROM_CPU_TAG;

typedef struct {
	u8 body[4];
} TO_CPU_TAG;

#define FROM_CPU_TRG_DEV_set(x,y) ((x).body[0] = ((x).body[0]&0xE0)|(0x1F&(y)));
#define FROM_CPU_T_set(x,y) ((x).body[0] =  ((x).body[0]&0xDF)|((u8)(0x1&(y))<<5));
#define FROM_CPU_TRG_PORT_set(x,y) ((x).body[1] = ((x).body[1]&0x7)|((u8)(0x1F&(y))<<3));
#define FROM_CPU_C_set(x,y) ((x).body[1] = ((x).body[1]&0xFE)|(0x1&(y)));
#define FROM_CPU_PRI_set(x,y) ((x).body[2] = (((x).body[2]&0x1F)|((u8)(0x7&(y))<<5)));
#define FROM_CPU_VID_set(x,y) ((x).body[2] = (((x).body[2]&0xF0)|((u8)(0xF&(y)>>12)))); \
		((x).body[3] = (((x).body[3]&0x0)|((u8)(0xFF&(y)))));
#define FROM_CPU_FIXED_FIELDS(x) ((x).body[0] = (((x).body[0] & 0x3F) | 0x40)); \
		((x).body[1] = ((x).body[1] & 0xF9)); ((x).body[2] = ((x).body[2] & 0xEF));

#define TO_CPU_T(x) (((x).body[0] & 0x20)>>5)
#define TO_CPU_SRC_DEV(x) ((x).body[0] & 0x3F)
#define TO_CPU_SRC_PORT(x) (((x).body[1] & 0xF8)>>3)
#define TO_CPU_CODE(x) (((x).body[1]&0x6)|(((x).body[2]&0x10)>>4))
#define TO_CPU_C(x) (((x).body[1] & 0x1))
#define TO_CPU_PRI (((x).body[2]&0xE0) >> 5)
#define TO_CPU_VID (((u16)((x).body[2])&0x0F)|(x).body[3])

#endif /* MARAVELL_88E6095_H_ */
