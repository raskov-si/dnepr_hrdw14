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

//#define MV88E6095_PCS_CTRL_REG			0x01
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


/* global conrol register */
#define MV88E6095_GLOBAL_CONTROL            0x04
  #define GLOBAL_CONTROL_SWRESET                (1 << 15)

//! VTU VID Register
#define MV88E6095_VTU_VID                   0x06
#define VTU_VID_VALID                                   0x1000
#define VTU_VID(x)                                      (x & 0x0FFF)
//#define VTU_OP_LOAD                                     0x3000
//#define VTU_BUSY                                        0x8000
#define MV88E6095_VTU_OPERATION                         0x05
#define VTU_DBNUM(x)                                    (((x & 0xF0) << 4) | (x & 0x0F))


//! VTU Data Register Ports 0 to 3
#define MV88E6095_VTU_DATA03                    0x07
#define VTU_PORT0(tag, state)                   (tag | (state << 2))
#define VTU_PORT1(tag, state)                   ((tag << 4) | (state << 6))
#define VTU_PORT2(tag, state)                   ((tag << 8) | (state << 10))
#define VTU_PORT3(tag, state)                   ((tag << 12) | (state << 14))

//! VTU Data Register Ports 4 to 7
#define MV88E6095_VTU_DATA47                    0x08
#define VTU_PORT4(tag, state)                   (tag | (state << 2))
#define VTU_PORT5(tag, state)                   ((tag << 4) | (state << 6))
#define VTU_PORT6(tag, state)                   ((tag << 8) | (state << 10))
#define VTU_PORT7(tag, state)                   ((tag << 12) | (state << 14))

//! VTU Data Register Ports 8 to 11
#define MV88E6095_VTU_DATA810                   0x09
#define VTU_PORT8(tag, state)                   (tag | (state << 2))
#define VTU_PORT9(tag, state)                   ((tag << 4) | (state << 6))
#define VTU_PORT10(tag, state)                  ((tag << 8) | (state << 10))
#define VTU_VIDPRI(x)                                   ((x & 3) << 12)
#define VTU_VIDPRI_Override                             ((x & 1) << 15)

//! Default Port VLAN ID & Priority
#define MV88E6095_PORT_DEFVLANID                0x07
#define DEFPRI(x)                               ((x & 0x07) << 13)
#define FORCE_DEF_VID                           0x1000
#define DEF_VID(x)                              (x & 0x0FFF)
#define VTU_BUSY                                0x8000

#define VTU_NO_OP                               0x0000
#define VTU_OP_FLUSH                            0x1000
#define VTU_OP_LOAD_OR_PURGE                    0x3000
#define VTU_OP_NEXT                             0x4000
#define VTU_OP_GET_CLEAR_VIOLDATA               0x7000

#define VLAN_MODE(x)                            ((x & 3) << 10)

//! Режимы работы порта
enum __VLAN_PortMode_t {
        VLAN_PORTMODE_ACCESS            = 0, //!< нетегированный трафик для одной VLAN
        VLAN_PORTMODE_GENERAL_ALL       = 1, //!< принимать все фреймы
        VLAN_PORTMODE_TRUNK             = 2  //!< принимать только тегированный трафик с нужными VLAN ID
} ;

typedef enum  __VLAN_PortMode_t VLAN_PortMode_t; 

typedef u16 VLAN_ID_t; //!< Тип для VLAN ID

//! Значения tag для MV88E6095_VTU_PORT*
typedef enum {  
        //! Port is a member of this VLAN and frames are to egress unmodified
        VTU_PORT_UNMODIFIED     = 0,
        //! Port is a member of this VLAN and frames are to egress Tagged
        VTU_PORT_UNTAGGED               = 1,
        //! Port is a member of this VLAN and frames are to egress Untagged
        VTU_PORT_TAGGED                 = 2,
        //! Port is not a member of this VLAN. Any frames with this VID are discarded at ingress and are not allowed to egress this port.       VTU_PORT_UNMODIFIED     = 0,
        VTU_PORT_NOTMEMBER              = 3
} MV88E6095_Port_Tagging;

//! Значения state для MV88E6095_VTU_PORT*
typedef enum {
        //! 802.1S Disabled. Use non-VLAN Port States for this port from frames with this VID.
        VTU_PORT_8021S_DISABLED                 = 0,
        //! Blocking/Listening Port State for this port for frames with this VID.
        VTU_PORT_BLOCKING_LISTENING             = 1,
        //! Learning Port State for this port for frames with this VID.
        VTU_PORT_LEARNING                               = 2,
        //! Forwarding Port State for this port for frames with this VID.
        VTU_PORT_FORWARDING                             = 3
} MV88E6095_Port_State;


//! определяются состояния портов в терминах 802.1Q и 802.1S
typedef struct {
        MV88E6095_Port_Tagging port0_tag;
        MV88E6095_Port_State port0_state;

        MV88E6095_Port_Tagging port1_tag;
        MV88E6095_Port_State port1_state;

        MV88E6095_Port_Tagging port2_tag;
        MV88E6095_Port_State port2_state;

        MV88E6095_Port_Tagging port3_tag;
        MV88E6095_Port_State port3_state;

        MV88E6095_Port_Tagging port4_tag;
        MV88E6095_Port_State port4_state;

        MV88E6095_Port_Tagging port5_tag;
        MV88E6095_Port_State port5_state;

        MV88E6095_Port_Tagging port6_tag;
        MV88E6095_Port_State port6_state;

        MV88E6095_Port_Tagging port7_tag;
        MV88E6095_Port_State port7_state;

        MV88E6095_Port_Tagging port8_tag;
        MV88E6095_Port_State port8_state;

        MV88E6095_Port_Tagging port9_tag;
        MV88E6095_Port_State port9_state;

        MV88E6095_Port_Tagging port10_tag;
        MV88E6095_Port_State port10_state;
} MV88E6095_Ports_VLAN_Status_t ;


typedef enum __Port8021QState {  
        //! Use Port Based VLANs only. The VLANTable bits and the DefaultVID
        //! assigned to the frame during ingress determine which Egress ports this
        //! Ingress port is allowed to switch frames to for all frames.
        PORT_8021Q_DISABLED = 0,
        //! Enable 802.1Q for this Ingress port. Do not discard Ingreess Membership 
        //! violations and use the VLANTable bits below if the frame's VID is not
        //! contained in the VTU (both errors are logged)
        PORT_8021Q_FALLBACK = 1,
        //! Enable 802.1Q for this Ingress port. Do not discard Ingress Membership
        //! violations but discard the frame if its VID is not contained in the VTU
        //! (both errors are logged).
        PORT_8021Q_CHECK = 2,
        //! Enable 802.1Q for this Ingress port. Discard Ingress Membership
        //! violations and discard frames whose VID is not contained in the VTU
        //! (both errors are logged).
        PORT_8021Q_SECURE = 3
} Port8021QState;


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


void MV88E6095_AddVTUEntry(   const u8 pcbDevAddr, const u16 vid, const u8 dbnum, const MV88E6095_Ports_VLAN_Status_t* stats );
void mv88E6095_purge_vtu_entry(  const u8 pcbDevAddr, const u16 vid, const u8 dbnum, const MV88E6095_Ports_VLAN_Status_t* stats );
void mv88E6095_flush_all_entrys(  const u8 pcbDevAddr );
void MV88E6095_PortResetDefaultVID( const u8 pcbDevAddr,  const u8 port_index );


u32 MV88E6095_ReadVTUEntry(   const u8 pcbDevAddr, const u16 vid, u8 *dbnum, MV88E6095_Ports_VLAN_Status_t* stats );
u8 MV88E6095_NextVTUEntry( const u8 pcbDevAddr, u16 prevvid, u16 *nextvid);
void MV88E6095_PortDefaultVID( const u8 pcbDevAddr,  const u8 port_index, const u8 force_def_vid, const u16 VID );
void MV88E6095_Change_Port8021Q_state( const u8 pcbDevAddr,  const u8 port_index, const Port8021QState state );
void MV88E6095_Reset_Port8021Q_state( const u8 pcbDevAddr,  const u8 port_index );

#endif /* MARAVELL_88E6095_H_ */
