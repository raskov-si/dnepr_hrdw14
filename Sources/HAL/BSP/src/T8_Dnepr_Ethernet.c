/*!
\file T8_Dnepr_Ethernet.c
\brief Код для работы с сетью в Днепре
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>


#include "common_lib/memory.h"
#include "common_lib/crc.h"
#include "Project_Headers/T8_atomic_heap.h"

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/IC/inc/MV_88E6095.h"
#include "HAL/MCU/inc/T8_5282_interrupts.h"

#include "lwip/include/arch/sys_arch.h"
#include "lwip/include/netif/etharp.h"
#include "lwip/include/lwip/opt.h"
#include "lwip/include/lwip/sys.h"
#include "lwip/include/lwip/stats.h"
#include "lwip/include/lwip/dhcp.h"
#include "lwip/tcpip.h"


/*=============================================================================================================*/

//! адрес  первого MV88E6095 по SMI в multichip mode
#define MV88E6095_1_CHIPADDR			0x01
//! адрес  второго MV88E6095 по SMI в multichip mode
#define MV88E6095_2_CHIPADDR			0x10

/*=============================================================================================================*/

void                            low_level_input(struct netif *netif);
void                            dnepr_if_tx_cleanup(void);
void                            dnepr_ethernet_rx_thread(void *arg);
void                            dnepr_ethernet_tx_thread(void *arg);

/*=============================================================================================================*/

static  sys_sem_t               tx_sem;
static  sys_sem_t               rx_sem;
struct  netif                   *eth0;

/*=============================================================================================================*/



/*--------------------------------инициализация-------------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief Инициализация FEC для стека
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_fec_init
(
    const u8        *mac_adress,    
    t_txrx_desc*    rx_desc_buf,
    size_t          rx_buf_len,
    t_txrx_desc*    tx_desc_buf,
    size_t          tx_buf_len
)
{
    t_fec_config      mii_config;


    mii_config.fec_mii_speed = FEC_MII_CLOCK_DEV_CALC(2500);
    memcpy(mii_config.mac_addr, mac_adress, 6);
    mii_config.max_eth_frame = MAX_ETH_PKT;
    mii_config.max_rcv_buf = MAX_ETH_BUFF_SIZE;
    
    mii_config.rxbd_ring        = rx_desc_buf;
    mii_config.rxbd_ring_len    = rx_buf_len;

    mii_config.txbd_ring        = tx_desc_buf;
    mii_config.txbd_ring_len    = tx_buf_len;
    
    mii_config.fec_mode= FEC_MODE_MII; 
    mii_config.ignore_mac_adress_when_recv = FALSE;
    mii_config.rcv_broadcast = TRUE;    
          
    m5282_fec_init(&mii_config);
        

	// инициализация прерывания о конце передаче фрейма
    MCU_ConfigureIntr(INTR_ID_FEC_X_INTF, 6, 1 );
    MCU_EnableIntr(INTR_ID_FEC_X_INTF,1);
	// инициализация прерывания о приёме фрейма
    MCU_ConfigureIntr(INTR_ID_FEC_R_INTF, 6, 3 );
    MCU_EnableIntr(INTR_ID_FEC_R_INTF,1);

    return 0 ;
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

	//	Параметры 					vid		dbnum	MV88E6095_Ports_VLAN_Status_t*
	MV88E6095_AddVTUEntry( pcbDevAddr,	vid,	0, 	&stats 	);
}

void VLAN_PortDefVIDSet( const u8 pcbDevAddr, const VLAN_ID_t vid, const size_t port_num )
{
	//										port_index	force_def_vid	VID
	MV88E6095_PortDefaultVID( 	pcbDevAddr, 	port_num, 	0, 				vid );
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

u32 VLAN_PortModeSet( 	const u8 pcbDevAddr, const size_t port_num,
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
			// если порт ACCESS -- все VLAN'ы его транка становятся недоступны
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
			MV88E6095_AddVTUEntry( 	pcbDevAddr, vids_arr[i], 	0,		&stats 	);
		}
	}
	if( mode == VLAN_PORTMODE_ACCESS ){
		if( MV88E6095_ReadVTUEntry( pcbDevAddr, defvid, NULL, &stats ) == OK ){
			__change_port_state( &stats, port_num,
						VTU_PORT_UNTAGGED, VTU_PORT_FORWARDING );
			MV88E6095_AddVTUEntry( pcbDevAddr,	defvid, 	0,		&stats 	);
		}  else
			return ERROR ;
	}

	return OK ;
}


void __set_up_port(const u8 pcbDevAddr, const u8 port, const u8 port_state,
					const u8 cpu_port, const _BOOL dsa_port )
{
	u16 usBuffer ;

	usBuffer = TRANSMIT_FRAMES_UNTAGGED | FORWARD_UNKNOWN
          | PORT_STATE(port_state) | (dsa_port ? DSA_TAG : 0);
	MV88E6095_multichip_smi_write( pcbDevAddr, port, MV88E6095_PORT_CTRL_REG, usBuffer);
	usBuffer = MAP_DA | DEFAULT_FORWARD | CPU_PORT(cpu_port);
	MV88E6095_multichip_smi_write( pcbDevAddr, port, MV88E6095_PORT_CTRL2_REG, usBuffer);	
}



/*=============================================================================================================*/


/*=============================================================================================================*/
/*!  \brief Инициализация PHY для стека
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_phy_init(void)
{
    u16	usBuffer = 0 ;
    VLAN_ID_t mcu_port_vids[] = { 20 };
//    VLAN_ID_t cpu_port_vids[] = { 1, 20 };
    VLAN_ID_t cpu_port_vids[] = { 20 };
    
    
        
// PCS Control Register -- ForcedLink (88E6095 datasheet page 61 Note) и LinkValue 
    MV88E6095_multichip_smi_read( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
    usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );
    MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
    usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
    MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );
    

    // Настраиваем порт MCU
//    usBuffer = DSA_TAG | FORWARD_UNKNOWN | PORT_STATE(MV88E6095_PORT_FORWARDING);
//    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PORT_CTRL_REG, usBuffer );
//    usBuffer = MAP_DA | DEFAULT_FORWARD | CPU_PORT(MV88E6095_PORT9);
//    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PORT_CTRL2_REG, usBuffer );
    
    usBuffer = NOT_FORCE_SPD | FORCE_LINK | LINK_FORCED_VALUE(1);
    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PCS_CTRL_REG, usBuffer);

//    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL, MV88E6095_GLOBAL_2, MV88E6095_CASCADE_PORT(0x8) | MV88E6095_DEV_NUM(1)  );
//    MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL, MV88E6095_GLOBAL_2, MV88E6095_CASCADE_PORT(0x8) | MV88E6095_DEV_NUM(0x10)  );  
        
    /* настраиваем VLAN 20 */
//    VLAN_AddVID( MV88E6095_1_CHIPADDR, 1 );
//    VLAN_AddVID( MV88E6095_1_CHIPADDR, 20 );
//    
//    // порт CU
//    VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, 1, 10 );
//    VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 10, 20, cpu_port_vids, 2, VLAN_PORTMODE_TRUNK, 0);
//    
//    //порт MCU
//    VLAN_PortDefVIDSet( MV88E6095_1_CHIPADDR, 20, 9 );
//    VLAN_PortModeSet( MV88E6095_1_CHIPADDR, 9, 20, mcu_port_vids, 1, VLAN_PORTMODE_ACCESS, 1);
//    
////	// Настраиваем порт Kontron'а
////    __set_up_port( MV88E6095_1_CHIPADDR, MV88E6095_PORT10, MV88E6095_PORT_FORWARDING, MV88E6095_PORT9, FALSE );
//    __set_up_port( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PORT_FORWARDING, MV88E6095_PORT10, FALSE );
    
	// Порты лицевой панели принимают нетэгированный трафик, становятся членами VLAN'а 100, участвуют в RSTP
//	__set_up_port( &hSwitch2, MV88E6095_PORT0, MV88E6095_PORT_LEARNING, MV88E6095_PORT8, FALSE ); // 1я медь UR
    
       
    
  return 0;
}

/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/

_BOOL  dnepr_ethernet_lwip_open 
(
    struct  netif   *atcual_netif
)
{
    eth0 = atcual_netif;
    
    sys_sem_new(&rx_sem, 0);                // create receive semaphore
    sys_sem_new(&tx_sem, 0);                // create transmit semaphore

        /* start new threads for tx, rx and timers */
    sys_thread_new("rx_eth", dnepr_ethernet_rx_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
    sys_thread_new("tx_eth", dnepr_ethernet_tx_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
      
    return TRUE;      
}



/*--------------------------------потоки приема и передачи-------------------------------------------------------*/

/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/

void dnepr_ethernet_rx_thread(void *arg)
{
    LWIP_UNUSED_ARG(arg);

    while (1)
    {
        sys_sem_wait(&rx_sem);

        low_level_input(eth0);
    }
}

/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/

void dnepr_ethernet_tx_thread(void *arg)
{
    LWIP_UNUSED_ARG(arg);
    
    while (1)
    {
        sys_sem_wait(&tx_sem);

        dnepr_if_tx_cleanup();
    }
}






/*-----------------------------------------обработчики прерываний-------------------------------------------------------*/

/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/
void isr_FEC_TxFrame_Handler()
{
    /* clear interrupt status for tx interrupt */
    MCF_FEC_EIR = MCF_FEC_EIR_TXF;
    sys_sem_signal(&tx_sem);
}


/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/
void isr_FEC_ReceiveFrame_Handler()
{
    MCF_FEC_EIMR &= ~MCF_FEC_EIMR_RXF;  // disable rx int
    MCF_FEC_EIR = MCF_FEC_EIR_RXF;          // clear int flag
    
    sys_sem_signal(&rx_sem);        
}
//    t8_m5282_fec_reset_rx_isr();        



void isr_FEC_TxBuffer_Handler() 
{return;}

void isr_FEC_ReceiveBuffer_Handler()
{return;}

void isr_FEC_MII_Handler()
{return;}


void isr_FEC_FIFOUnderrun_Handler()
{return;}

void isr_FEC_CollisionRetryLimit_Handler()
{return;}

void isr_FEC_LateCollision_Handler()
{return;}

void isr_FEC_HeartbeatError_Handler()
{return;}

void isr_FEC_GracefulStopComplete_Handler()
{return;}

void isr_FEC_EthernetBusError_Handler()
{return;}

void isr_FEC_BabblingTransmitError_Handler()
{return;}

void isr_FEC_BabblingReceiveError_Handler()
{return;}



/*--------------------------------функции работы с switch-------------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief Управляем режимом автоопределения типа сети auto-negotiation для SFP
     \details Отключаем или включаем режим в зависимости от флага mode_flag для конкретного порта switch'а 
     \details MV88E6095 PORT9 для SFP1, PORT10 для SFP2
     \details По умолчанию после сброса данный режим включен

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
    case 1:     port = MV88E6095_PORT10;     break;  /* sfp1 - нижняя  L port10  */
    case 2:     port = MV88E6095_PORT9;      break;  /* sfp2 - верхняя U port9   */
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



/*--------------------------------всякая фигня-------------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief Переводим MAC-адрес из строкового в числовое выражение
*/
/*=============================================================================================================*/
void dnepr_ethernet_str_2_mac
( 
    u8*         out,            /*!< [out] получаемое числовое значение (массив из 6 элементов)   */
    const char* str             /*!< [in]  входящая строка с IP-адресом                           */
)
{
	u32 flag=FALSE;
	s8 mac_byte[3];
	u8 new_mac_addr[6];
	u8 mac_byte_num=0;
	u8 i,k, data_len;
	flag = TRUE;

	if( !out || !str ){
		return ;
	}

	data_len = strnlen( str, 18 );

	for (i=0, k=0; (i<data_len) && (flag == TRUE); i++) {
		if ((str[i] != ':') && (str[i] != '-') && (str[i] != '.')) {
			if (k<2)
				mac_byte[k++] = str[i];
			else
				flag = FALSE;
		}
		else if (k && (k<3)) {
			mac_byte[k] = 0;
			k = 0;
			errno = 0;
			if (mac_byte_num < 6) {
				new_mac_addr[mac_byte_num++] = strtol(mac_byte, NULL, 16);
				if (errno == ERANGE)
					flag = FALSE;
			}
			else
				flag = FALSE;
		}
	}
//Last byte. We don't find delimiter, but find 0
	if ((mac_byte_num == 5) && (flag == TRUE))
		if (k && (k<3)) {
			mac_byte[k] = 0;
			errno = 0;
			new_mac_addr[mac_byte_num++] = strtol(mac_byte, NULL, 16);
			if (errno == ERANGE)
				flag = FALSE;
	}
	if (flag == TRUE){
		memcpy( out, new_mac_addr, sizeof(new_mac_addr) );
	}
}



/*=============================================================================================================*/
/*!  \brief Переводим MAC-адрес из числового в строковое выражение
*/
/*=============================================================================================================*/
void dnepr_ethernet_mac_2_str
( 
    char        *str,           /*!< [out]  выходящая строка с IP-адресом                           */
    const u8    *mac            /*!< [in]   числовое значение MAC-адреса (массив из 6 элементов)    */    
)
{
	size_t i ;
	size_t istr = 0 ;
	static const char alf[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	assert( str != NULL );

	for( i = 0; i < 6; ++i ){
		str[istr++] = alf[ (mac[ i ] & 0xF0) >> 4 ];
		str[istr++] = alf[mac[ i ] & 0x0F ];
		if( i < 5 ){
			str[istr++] = ':' ;
		}
	}
}

