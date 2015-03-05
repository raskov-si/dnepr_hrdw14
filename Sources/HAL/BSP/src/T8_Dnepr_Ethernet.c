/*!
\file T8_Dnepr_Ethernet.c
\brief Код для работы с сетью в Днепре
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "common_lib/memory.h"

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/IC/inc/MV_88E6095.h"
#include "HAL/MCU/inc/T8_5282_FEC.h"
#include "HAL/MCU/inc/T8_5282_interrupts.h"

////    // Семафор массива дескрипторов на посылку. В 2 раза меньше количества дескрипторов,
////    // потому что на каждый пакет приходиться по 2 дескриптора -- заголовок и тело отдельно.
////    __eth_tx_sem = OSSemCreate( (u8)(FEC_TX_BD_NUMBER / 2) );
//
//
//
//
//
//
//

/*=============================================================================================================*/

#define RX_PACKET_POOL_LEN	ETHERNET_RX_BD_NUMBER        
#define TX_PACKET_POOL_LEN	ETHERNET_TX_BD_NUMBER

#define RX_QUEUE_LEN	        ETHERNET_RX_BD_NUMBER        
#define TX_QUEUE_LEN	        ETHERNET_TX_BD_NUMBER

#define PAYLOAD2PBUF(p) (struct pbuf*)((u8*)(p) - LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))) 



#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static t_txrx_desc rx_bd[ ETHERNET_RX_BD_NUMBER ] ;  /*! дескрипторы на прием пакетов для DMA */

#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static t_txrx_desc tx_bd[ ETHERNET_TX_BD_NUMBER ] ;  /*! дескрипторы на передачу пакетов для DMA */

#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static struct pbuf *rx_pbuf_array[ETHERNET_RX_BD_NUMBER];


#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static u8     rx_arrays [ RX_PACKET_POOL_LEN ][ MAX_ETH_BUFF_SIZE ]; /*! массивы для приема пакетов для DMA */
_Pragma("location=\"sdram\"")
__no_init static _BOOL  rx_busy   [ RX_PACKET_POOL_LEN ];


#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static u8     tx_arrays [ TX_PACKET_POOL_LEN ][ MAX_ETH_BUFF_SIZE ]; /*! массивы для передачи пакетов для DMA */
_Pragma("location=\"sdram\"")
__no_init static _BOOL  tx_busy   [ TX_PACKET_POOL_LEN ];


static  t_eth                   default_eth;
static  u8                      last_rx_buf_descr = 0;

#if LWIP_TCPIP_CORE_LOCKING_INPUT
    static OS_EVENT             *eth0_rcv_queue = NULL;                     /*!   */    
    static void                 *rx_messages_array[RX_QUEUE_LEN] ;          /*! указатели для очереди сообщений  */
    
    static OS_EVENT             *eth0_tcm_queue = NULL;                     /*!   */    
    static void                 *tx_messages_array[TX_QUEUE_LEN] ;          /*! указатели для очереди сообщений  */
#endif




/*=============================================================================================================*/

_BOOL  dnepr_ethernet_open 
(
    t_eth    *out_descr
)
{
    u8      i;
    
    out_descr = out_descr;
    
    default_eth.data_descr.rx_pool.pocket_len      = MAX_ETH_BUFF_SIZE;
    default_eth.data_descr.rx_pool.pockets_number  = RX_PACKET_POOL_LEN;
    default_eth.data_descr.rx_pool.pockets_busy    = rx_busy;
    default_eth.data_descr.rx_pool.pockets_array   = (u8**)rx_arrays;
    
    default_eth.data_descr.tx_pool.pocket_len      = MAX_ETH_BUFF_SIZE;
    default_eth.data_descr.tx_pool.pockets_number  = TX_PACKET_POOL_LEN;
    default_eth.data_descr.tx_pool.pockets_busy    = tx_busy;
    default_eth.data_descr.tx_pool.pockets_array   = (u8**)tx_arrays;

    
    
    for ( i = 0; i < RX_PACKET_POOL_LEN; ++i )  {
        rx_bd[i].contr_status_inf = 0 ;
        rx_bd[i].data_length = 0 ;      
        rx_bd[i].starting_adress = &rx_arrays[i][0];    
    }
    
    for ( i = 0; i < TX_PACKET_POOL_LEN; ++i )  {
      
        tx_bd[i].contr_status_inf = 0 ;
        tx_bd[i].data_length = 0 ;      
        tx_bd[i].starting_adress = &tx_arrays[i][0];
    }
    
#if LWIP_TCPIP_CORE_LOCKING_INPUT    
    eth0_rcv_queue = OSQCreate( rx_messages_array, RX_QUEUE_LEN ) ;
#endif    
    
      
    out_descr = &default_eth;    
    return TRUE;      
}


/*!
\brief Инициализирует два MV88E6095 как тупой свитч, без RSTP, все порты включены
\param maddr массив 6ти байт мак-адреса, которым свитч должен слать пакеты flow control
\retval OK/ERROR
*/
u32 Dnepr_Ethernet_Init( const u8* maddr )
{
	u16	usBuffer = 0 ;
	s32 i ;

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

	// Инициализируем MAC
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

	return OK ; 

_err:
	return ERROR ;
}






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



/*=============================================================================================================*/
/*!  \brief Инициализация FEC для стека
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_fec_init
(
    const u8 *mac_adress
)
{
    t_fec_config      mii_config;
  
    mii_config.fec_mii_speed = FEC_MII_CLOCK_DEV_CALC(2500);
    memcpy(mii_config.mac_addr, mac_adress, 6);
    mii_config.fec_max_eth_pocket = MAX_ETH_PKT;
    
    mii_config.rxbd_ring = rx_bd;
    mii_config.rxbd_ring_len = ETHERNET_RX_BD_NUMBER;

    mii_config.txbd_ring = tx_bd;
    mii_config.txbd_ring_len = ETHERNET_TX_BD_NUMBER;
          
    t8_m5282_fec_init(&mii_config);
  
// инициализация прерывания о конце передаче фрейма
//    MCU_ConfigureIntr(INTR_ID_FEC_X_INTF, 6, 1 );
//    MCU_EnableIntr(INTR_ID_FEC_X_INTF,1);
    
// инициализация прерывания о конце передачи одного буфера
//    MCU_ConfigureIntr(INTR_ID_FEC_X_INTB, 6, 2 );
//    MCU_EnableIntr(INTR_ID_FEC_X_INTB,1);
    
// инициализация прерывания о приёме фрейма
    MCU_ConfigureIntr(INTR_ID_FEC_R_INTF, 6, 3 );
    MCU_EnableIntr(INTR_ID_FEC_R_INTF,1);

  
    return 0;
}

  
/*=============================================================================================================*/
/*!  \brief Инициализация PHY для стека
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_phy_init(void)
{
//  u32   i;

//  /* Configure EMDC clock, frequency = 2.50 MHz */
//  FEC_MSCR = MSCR_MII_SPEED;
//  PHY_EPHYCTL0 &= ~(EPHYCTL0_DIS10 | EPHYCTL0_DIS100);
//  PHY_EPHYCTL0 |= EPHYCTL0_LEDEN;
//  /* Enable EPHY */
//  PHY_EPHYCTL0 |= EPHYCTL0_EPHYEN;
//  /* Delay for startup time */
//  for (i = PHY_DELAY; i != 0; i--) 
//  {    
//    /*
//    * Put something here to make sure
//    * the compiler doesn't optimize the loop away
//    */
//  }
//  /* Link speed = 100 Mbps, full-duplex */
//  FEC_MMFR = MMFR_ST | MMFR_OP_WR | MMFR_TA | MMFR_RA_CONTROL |
//                      EPHY_DATARATE | EPHY_DPLX;
//  
//  /* Poll for MII Write Frame completion */
//  while ((FEC_EIR & EIR_MII) == 0) { }  
//  /* Request status register read (link status polling) */
//  FEC_MMFR = MMFR_ST | MMFR_OP_RD | MMFR_TA | MMFR_RA_STATUS;   
  
  return 0;
}

/*=============================================================================================================*/
/*!  \brief Инициализация switch, VLAN
*
*   \sa 
*/
/*=============================================================================================================*/
//int dnepr_ethernet_switch_init(void)


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


#if LWIP_TCPIP_CORE_LOCKING_INPUT
_BOOL   dnepr_ethernet_rx_check
(
    t_eth_data_descr    *data_descr
)
{   
    _BOOL    ret_code;
   
    data_descr->rx_cur_message = (net_meassage_t*)OSQAccept( eth0_rcv_queue, 0, &ret_code );
    
    return ret_code;
}
#endif


void isr_FEC_TxFrame_Handler()
{
//	MCF_FEC_EIR |= MCF_FEC_EIR_TXF ;
//	T8_Dnepr_FEC_TX_Frame_Sent() ;
//	__tx_cnt1-- ;
}


static size_t __tx_bd_isr_i = 0 ;
void isr_FEC_TxBuffer_Handler()
{
//	u8 i, err ;
//	Dnepr_Ethertnet_TX_Frame_t *q_mess ;
//
//	MCF_FEC_EIR |= MCF_FEC_EIR_TXB ;
//	__tx_cnt2-- ;
//
//
//	return ;
//	if( !__tx_queue ){
//		return ;
//	}
//	// Проверяем есть ли фреймы в очереди
//	// Сначала идёт сообщение с заголовком пакета
//	q_mess = (Dnepr_Ethertnet_TX_Frame_t*)OSQAccept( __tx_queue, &err );
//	assert( err == OS_ERR_NONE || err == OS_ERR_Q_EMPTY );
//	if( err == OS_ERR_Q_EMPTY ){
//		T8_Dnepr_TS_Ethernet_TX_Disactivate() ;
//		return ;
//	}
//
//	// Добавляем их в дескрипторы, если есть свободные. 
//	// Надо добавить весь пакет
//	i = __tx_bd_isr_i ;
//	do {
//		if( ++i >= FEC_TX_BD_NUMBER ){
//			i = 0 ;
//		}
//		// Если дескриптор свободен -- добавляем заголовок.
//		if( fec_tx_cstatus( i ) & MCF_FEC_TxBD_R == 0 ){
//			break ;
//		}
//
//	} while( i != __tx_bd_isr_i );
//	// Нет свободных дескрипторов, где-то ошибка.
//	assert( i != __tx_bd_isr_i );
//
//	++__tx_bd_isr_i ;
//	fec_add_tx_buffer( q_mess->data, i, q_mess->length );
//	if( ++i >= FEC_TX_BD_NUMBER ){
//		i = 0 ;
//	}
//	// Следующий дескриптор тоже должен быть пустым, потому что посылаем по-пакетно,
//	// а в каждом пакете два дескриптора.
//	assert( fec_tx_cstatus( i ) & MCF_FEC_TxBD_R == 0 );
//
//	// Теперь сообщение с телом пакета
//	q_mess = (Dnepr_Ethertnet_TX_Frame_t*)OSQAccept( __tx_queue, &err );
//	assert( err == OS_ERR_NONE );
//	fec_add_tx_buffer( q_mess->data, i, q_mess->length );
}

void isr_FEC_FIFOUnderrun_Handler()
{}

void isr_FEC_CollisionRetryLimit_Handler()
{}


void isr_FEC_ReceiveFrame_Handler()
{
    u8    ret;
        
    t8_m5282_fec_reset_rx_isr();
        
//        pocket_pools.pockets_array[last_rx_buf_descr][0]
#if LWIP_TCPIP_CORE_LOCKING_INPUT
        
    OSQPost( __NetRcvQueue, (void*)&__change_mac_struct );                                              /* сообщение собственному потоку */
    
#else        
    // if (rx_pbuf_array[last_rx_buf_descr] == NULL)                                           /* пихаем сообщения в tcp/ip поток lwip */
    rx_pbuf_array[last_rx_buf_descr]->payload = &default_eth.data_descr.rx_pool.pockets_array[last_rx_buf_descr][0];
    ret = (*default_eth.netif.input)(rx_pbuf_array[last_rx_buf_descr], &default_eth.netif);
    if ( ret == ERR_OK )    
#endif                                                                                                    /* начинаем прием в следующий буфер если его уже обработали (очередь увеличилась) */
    {          
        if ( ++last_rx_buf_descr == ETHERNET_RX_BD_NUMBER ) {
            last_rx_buf_descr = 0;
        }
    }
        
    t8_m5282_fec_set_empty_status(&rx_bd[last_rx_buf_descr].contr_status_inf);        
    t8_m5282_fec_start_rx();
}


void isr_FEC_ReceiveBuffer_Handler()
{}

void isr_FEC_MII_Handler()
{}

void isr_FEC_LateCollision_Handler()
{}

void isr_FEC_HeartbeatError_Handler()
{}

void isr_FEC_GracefulStopComplete_Handler()
{}

void isr_FEC_EthernetBusError_Handler()
{}

void isr_FEC_BabblingTransmitError_Handler()
{}

void isr_FEC_BabblingReceiveError_Handler()
{}

