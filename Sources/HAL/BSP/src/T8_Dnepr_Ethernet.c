/*!
\file T8_Dnepr_Ethernet.c
\brief  од дл€ работы с сетью в ƒнепре
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/IC/inc/MV_88E6095.h"
#include "HAL/MCU/inc/T8_5282_MII.h"


void VLAN_AddVID( const u8 pcbDevAddr, const VLAN_ID_t vid );
void VLAN_PortDefVIDSet( const u8 pcbDevAddr, const VLAN_ID_t vid, const size_t port_num );
u32  VLAN_PortModeSet(   const u8 pcbDevAddr, const size_t port_num,
                                                VLAN_ID_t defvid, VLAN_ID_t *vids_arr, const size_t vids_arr_len,
                                                const VLAN_PortMode_t mode, u8 secure_flag );


/*!
\brief »нициализирует два MV88E6095 как тупой свитч, без RSTP, все порты включены
\param maddr массив 6ти байт мак-адреса, которым свитч должен слать пакеты flow control
\retval OK/ERROR
*/

#define  EXTRN_VLAN   1
#define  SHELF_VLAN   10

u32 Dnepr_Ethernet_Init( const u8* maddr )
{
	u16	usBuffer = 0 ;
	s32 i ;
        VLAN_ID_t cpu_port_vids[] = { EXTRN_VLAN, SHELF_VLAN };
        

	MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED((u32)( SYSTEM_CLOCK_KHZ/1000/5 )) ;

// Switch Management Register

	// interswitch порты

	// PCS Control Register -- ForcedLink (88E6095 datasheet page 61 Note) и LinkValue 
	MV88E6095_multichip_smi_read( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );
	MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
	MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );

	// »нициализируем MAC
	if( !maddr )
		goto _err ;
	for(i=0;i<3;i++){
		usBuffer = ((u16)(maddr[i*2]) << 8) | (u16)maddr[i*2+1];
		MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer );
		if(i==2)
			MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer+1 );
		else
			MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer );
	}
        
        /* инициализируем VLAN  */
        VLAN_AddVID( MV88E6095_1_CHIPADDR, EXTRN_VLAN );
        VLAN_AddVID( MV88E6095_2_CHIPADDR, EXTRN_VLAN );
        VLAN_AddVID( MV88E6095_1_CHIPADDR, SHELF_VLAN );
        VLAN_AddVID( MV88E6095_2_CHIPADDR, SHELF_VLAN );

                                                        /* trunk порты между чипами */
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, EXTRN_VLAN, 8 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 8, EXTRN_VLAN, &cpu_port_vids[0], 2, VLAN_PORTMODE_TRUNK, 1);
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, EXTRN_VLAN, 8 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 8, EXTRN_VLAN, &cpu_port_vids[0], 2, VLAN_PORTMODE_TRUNK, 1);
        
        //MV88E6095_1_CHIPADDR, MV88E6095_PORT10    /*  CPU */
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, EXTRN_VLAN, 10 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 10, EXTRN_VLAN, &cpu_port_vids[0], 2, VLAN_PORTMODE_TRUNK, 1);
        
        /* VID 10  - между слотами и CPU */
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 0 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 0, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 1 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 1, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 2 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 2, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 3 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 3, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 4 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 4, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 5 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 5, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 6 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 6, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, SHELF_VLAN, 7 );
        VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 7, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
         
        
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, SHELF_VLAN, 3 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 3, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, SHELF_VLAN, 4 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 4, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, SHELF_VLAN, 5 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 5, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, SHELF_VLAN, 6 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 6, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
        
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, SHELF_VLAN, 7 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 7, SHELF_VLAN, &cpu_port_vids[1], 1, VLAN_PORTMODE_ACCESS, 1);
                
        
        /* VID 100 - c внешним миром и CPU */              
                                                        /*  8P8C совмещенный с USB   */
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, EXTRN_VLAN, 2 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 2, EXTRN_VLAN, cpu_port_vids, 1, VLAN_PORTMODE_ACCESS, 1);          
          
//        MV88E6095_2_CHIPADDR,    MV88E6095_PORT0  /*  8P8C    */
//        MV88E6095_2_CHIPADDR,    MV88E6095_PORT1  /*  8P8C    */
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, EXTRN_VLAN, 0 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 0, EXTRN_VLAN, cpu_port_vids, 1, VLAN_PORTMODE_ACCESS, 1);
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, EXTRN_VLAN, 1 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 1, EXTRN_VLAN, cpu_port_vids, 1, VLAN_PORTMODE_ACCESS, 1);
        
//        MV88E6095_2_CHIPADDR,    MV88E6095_PORT10 /* sfp1 - нижн€€  L port10  */
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, EXTRN_VLAN, 10 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 10, EXTRN_VLAN, cpu_port_vids, 1, VLAN_PORTMODE_ACCESS, 1);
                
//        MV88E6095_2_CHIPADDR,    MV88E6095_PORT9  /* sfp2 - верхн€€ U port9   */
        VLAN_PortDefVIDSet( MV88E6095_2_CHIPADDR, EXTRN_VLAN, 9 );
        VLAN_PortModeSet( MV88E6095_2_CHIPADDR, 9, EXTRN_VLAN, cpu_port_vids, 1, VLAN_PORTMODE_ACCESS, 1);
        

	return OK ; 

_err:
	return ERROR ;
}

/*=============================================================================================================*/
/*!  \brief ”правл€ем режимом автоопределени€ типа сети auto-negotiation дл€ SFP
     \details ќтключаем или включаем режим в зависимости от флага mode_flag дл€ конкретного порта switch'а 
     \details MV88E6095 PORT9 дл€ SFP1, PORT10 дл€ SFP2
     \details ѕо умолчанию после сброса данный режим включен

     \sa MV88E6095_multichip_smi_read, cmsfpautoneg_update 
*/
/*=============================================================================================================*/
void dnepr_ethernet_sfpport_autoneg_mode
( 
    u8      sfp_num,                        /*!< [in] номер sfp 1,2                                   */
    u8      mode_flag                       /*!< [in] включить или выключить режим  0 - выкл, 1 - вкл */
)
{  
    u8      port;
    u16     temp_reg;
    
    switch (sfp_num)
    {
    case 1:     port = MV88E6095_PORT10;     break;  /* sfp1 - нижн€€  L port10  */
    case 2:     port = MV88E6095_PORT9;      break;  /* sfp2 - верхн€€ U port9   */
    default:    return;
    }
  
    MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, port, MV88E6095_PCS_CTRL_REG, &temp_reg  );  
    if ( mode_flag == 0 )    {
        /* отключаем режим auto-negotiation */
        temp_reg &= ~(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_MASK | MV88E6095_PCS_CTRL_REG_AUTONEG_ENABLE);        
        temp_reg |= MV88E6095_PCS_CTRL_REG_FORCE_SPEED(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_1000B);
    } else {
        /* включаем режим auto-negotiation */
        temp_reg &= ~(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_MASK);        
        temp_reg |= MV88E6095_PCS_CTRL_REG_FORCE_SPEED(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_AUTO) | MV88E6095_PCS_CTRL_REG_AUTONEG_ENABLE ;
    }
    MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, port, MV88E6095_PCS_CTRL_REG, temp_reg  );
}


/*=============================================================================================================*/

void VLAN_AddVID( const u8 pcbDevAddr, const VLAN_ID_t vid )
{
        MV88E6095_Ports_VLAN_Status_t stats = (MV88E6095_Ports_VLAN_Status_t){
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING };

        if( (vid < 1) || (vid > 4095) )
                return ;

        //      ѕараметры                                       vid             dbnum   MV88E6095_Ports_VLAN_Status_t*
        MV88E6095_AddVTUEntry( pcbDevAddr,      vid,    0,      &stats  );
}

void VLAN_PortDefVIDSet( const u8 pcbDevAddr, const VLAN_ID_t vid, const size_t port_num )
{
        //                                                                              port_index      force_def_vid   VID
        MV88E6095_PortDefaultVID(       pcbDevAddr,     port_num,       0,                              vid );
}


void __change_port_state( MV88E6095_Ports_VLAN_Status_t* stats, const size_t port_num,
                                                        MV88E6095_Port_Tagging tag, MV88E6095_Port_State state )
{
        if( !stats )
                return ;
        switch( port_num ){
                case 0:
                        stats->port0_tag = tag ;
                        stats->port0_state = state ;
                        break ;
                case 1:
                        stats->port1_tag = tag ;
                        stats->port1_state = state ;
                        break ;
                case 2:
                        stats->port2_tag = tag ;
                        stats->port2_state = state ;
                        break ;
                case 3:
                        stats->port3_tag = tag ;
                        stats->port3_state = state ;
                        break ;
                case 4:
                        stats->port4_tag = tag ;
                        stats->port4_state = state ;
                        break ;
                case 5:
                        stats->port5_tag = tag ;
                        stats->port5_state = state ;
                        break ;
                case 6:
                        stats->port6_tag = tag ;
                        stats->port6_state = state ;
                        break ;
                case 7:
                        stats->port7_tag = tag ;
                        stats->port7_state = state ;
                        break ;
                case 8:
                        stats->port8_tag = tag ;
                        stats->port8_state = state ;
                        break ;
                case 9:
                        stats->port9_tag = tag ;
                        stats->port9_state = state ;
                        break ;
                case 10:
                        stats->port10_tag = tag ;
                        stats->port10_state = state ;
                        break ;
        }
}

u32 VLAN_PortModeSet(   const u8 pcbDevAddr, const size_t port_num,
                                                VLAN_ID_t defvid, VLAN_ID_t *vids_arr, const size_t vids_arr_len,
                                                const VLAN_PortMode_t mode, u8 secure_flag )
{
        size_t i ;
        MV88E6095_Ports_VLAN_Status_t stats;

        if( (mode == VLAN_PORTMODE_ACCESS) || (mode == VLAN_PORTMODE_TRUNK) ){
          if ( secure_flag ) {
                MV88E6095_Change_Port8021Q_state( pcbDevAddr, port_num, PORT_8021Q_SECURE );
          } else {
                MV88E6095_Change_Port8021Q_state( pcbDevAddr, port_num, PORT_8021Q_FALLBACK );
          }
        } else if( mode == VLAN_PORTMODE_GENERAL_ALL ){
                MV88E6095_Change_Port8021Q_state( pcbDevAddr, port_num, PORT_8021Q_FALLBACK );
        }

        for( i = 0; i < vids_arr_len; i++){
                if( MV88E6095_ReadVTUEntry( pcbDevAddr, vids_arr[i], NULL, &stats ) == OK ){
                        // если порт ACCESS -- все VLAN'ы его транка станов€тс€ недоступны
                        if( mode == VLAN_PORTMODE_ACCESS ){
                                __change_port_state( &stats, port_num,
                                                        VTU_PORT_NOTMEMBER, VTU_PORT_FORWARDING );
                        } else if( mode == VLAN_PORTMODE_GENERAL_ALL ){
                                __change_port_state( &stats, port_num,
                                                        VTU_PORT_UNMODIFIED, VTU_PORT_FORWARDING );
                        } else if( mode == VLAN_PORTMODE_TRUNK){
                                __change_port_state( &stats, port_num,
                                                        VTU_PORT_TAGGED, VTU_PORT_FORWARDING );
                        }
                        MV88E6095_AddVTUEntry(  pcbDevAddr, vids_arr[i],        0,              &stats  );
                }
        }
        if( mode == VLAN_PORTMODE_ACCESS ){
                if( MV88E6095_ReadVTUEntry( pcbDevAddr, defvid, NULL, &stats ) == OK ){
                        __change_port_state( &stats, port_num,
                                                VTU_PORT_UNTAGGED, VTU_PORT_FORWARDING );
                        MV88E6095_AddVTUEntry( pcbDevAddr,      defvid,         0,              &stats  );
                }  else
                        return ERROR ;
        }

        return OK ;
}
/*=============================================================================================================*/


