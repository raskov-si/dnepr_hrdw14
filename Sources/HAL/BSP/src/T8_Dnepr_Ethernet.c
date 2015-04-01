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
/*!  \brief Инициализация PHY для стека
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_phy_init(void)
{
    u16	usBuffer = 0 ;
    
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
    
    
    

//	// Инициализируем MAC
//	if( !maddr )
//		goto _err ;
//	for(i=0;i<3;i++){
//		usBuffer = ((u16)(maddr[i*2]) << 8) | (u16)maddr[i*2+1];
//		MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer );
//		if(i==2)
//			MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer+1 );
//		else
//			MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer );
//	}
    
    
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

