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

#define RX_PACKET_POOL_LEN	ETHERNET_RX_BD_NUMBER        
#define TX_PACKET_POOL_LEN	ETHERNET_TX_BD_NUMBER

#define RX_QUEUE_LEN	        ETHERNET_RX_BD_NUMBER        
#define TX_QUEUE_LEN	        ETHERNET_TX_BD_NUMBER

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
/*!  \brief Инициализация PHY для стека
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_phy_init(void)
{
    u16	usBuffer = 0 ;

    // Настраиваем порт MCU
    usBuffer = DSA_TAG | FORWARD_UNKNOWN | PORT_STATE(MV88E6095_PORT_FORWARDING);
    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PORT_CTRL_REG, usBuffer );
    usBuffer = MAP_DA | DEFAULT_FORWARD | CPU_PORT(MV88E6095_PORT9);
    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PORT_CTRL2_REG, usBuffer );
    usBuffer = NOT_FORCE_SPD | FORCE_LINK | LINK_FORCED_VALUE(1);
    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PCS_CTRL_REG, usBuffer);

    MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL, MV88E6095_GLOBAL_2, MV88E6095_CASCADE_PORT(0x8) | MV88E6095_DEV_NUM(1)  );
    MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL, MV88E6095_GLOBAL_2, MV88E6095_CASCADE_PORT(0x8) | MV88E6095_DEV_NUM(0x10)  );    
    
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
//        sys_thread_new(mcf5282_ethernet_timers_thread, NULL, DEFAULT_THREAD_PRIO);
      
    return TRUE;      
}



/*--------------------------------потоки приема и передачи-------------------------------------------------------*/


/** \brief rx thread
 *  \param *arg 
 */
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

/** \brief transmit thread
 * \param *arg 
 */
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
//void isr_FEC_TxFrame_Handler()
//{
//    /* clear interrupt status for tx interrupt */
//    MCF_FEC_EIR = MCF_FEC_EIR_TXF;
//    sys_sem_signal(&tx_sem);
//}


/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/
//void isr_FEC_ReceiveFrame_Handler()
//{
//    MCF_FEC_EIMR &= ~MCF_FEC_EIMR_RXF;  // disable rx int
//    MCF_FEC_EIR = MCF_FEC_EIR_RXF;          // clear int flag
//    
//    sys_sem_signal(&rx_sem);        
//}
//    t8_m5282_fec_reset_rx_isr();        



//void isr_FEC_TxBuffer_Handler() 
//{return;}

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



























#define PAYLOAD2PBUF(p) (struct pbuf*)((u8*)(p) - LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))) 


#define MCF_FEC_EIR_ALL_EVENTS   \
   ( MCF_FEC_EIR_HBERR | MCF_FEC_EIR_BABR | MCF_FEC_EIR_BABT | \
     MCF_FEC_EIR_GRA   | MCF_FEC_EIR_TXF  | MCF_FEC_EIR_TXB  | \
     MCF_FEC_EIR_RXF   | MCF_FEC_EIR_RXB  | MCF_FEC_EIR_MII  | \
     MCF_FEC_EIR_EBERR | MCF_FEC_EIR_LC   | MCF_FEC_EIR_RL   | \
     MCF_FEC_EIR_UN )

#define MCF_FEC_EIMR_ALL_MASKS   \
   ( MCF_FEC_EIMR_HBERR | MCF_FEC_EIMR_BABR | MCF_FEC_EIMR_BABT | \
     MCF_FEC_EIMR_GRA   | MCF_FEC_EIMR_TXF  | 0                 | \
     MCF_FEC_EIMR_RXF   | 0                 | MCF_FEC_EIMR_MII  | \
     MCF_FEC_EIMR_EBERR | MCF_FEC_EIMR_LC   | MCF_FEC_EIMR_RL   | \
     MCF_FEC_EIMR_UN )




static fec_mib_t *pMIB = (fec_mib_t*)&MCF_FEC_RMON_T_DROP ;


static u32 FECMulticastAddressSet(u16 mac_upper2, u32 mac_lower4)
{
    u8 mac[8];
    u8 hash_pos;
    mac[0] = (u8)(mac_upper2>>8);
    mac[1] = (u8)(mac_upper2);
    mac[2] = (u8)(mac_lower4>>24);
    mac[3] = (u8)(mac_lower4>>16);
    mac[4] = (u8)(mac_lower4>>8);
    mac[5] = (u8)(mac_lower4>>0);
    hash_pos = (~Crc32(mac, 6) >> 26);
    if (hash_pos & 0x20)
        MCF_FEC_GAUR |= 1 << (0x1F & hash_pos);
    else
        MCF_FEC_GALR |= 1 << (0x1F & hash_pos);
    return OK;
}




#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static t_txrx_desc rx_bd[ ETHERNET_RX_BD_NUMBER ] ;  /*! дескрипторы на прием пакетов для DMA */

#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static t_txrx_desc tx_bd[ ETHERNET_TX_BD_NUMBER ] ;  /*! дескрипторы на передачу пакетов для DMA */




/*=============================================================================================================*/


    
    


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! длина отдельного буфера в приёмнике, д.б. не меньше MAX_ETH_PKT и делиться на 16
#define FEC_MAX_RCV_BUFF_SIZE 	1536
//! количество дескрипторов приёмных буферов
#define FEC_RX_BD_NUMBER	8
//! количество дескрипторов буферов для отсылки
#define FEC_TX_BD_NUMBER	4

#define _RX_PACKET_POOL_LEN	16
#define _TX_PACKET_POOL_LEN	16


typedef struct net_meassage_t_
{
	enum {
		CHANGE_MAC,
		RX_PACKET,
		TX_PACKET,
		TIMER
	} message_type ;

	u8 new_local_mac[ 6 ];

	size_t packet_index ;
	size_t packet_len ;
} net_meassage_t ;


#pragma pack(push)
#pragma pack(1)
typedef struct BufferDescriptor 
{
   volatile u16  bd_cstatus;     //!< control and status
   volatile u16  bd_length;      //!< transfer length
   volatile u8 * bd_addr;        //!< buffer address
} BD;
#pragma pack(pop)


typedef struct {
	u8 mac_addr[6] ;		//!< мак-адрес
	u8 rcv_broadcast ;		//!< RX должен реагировать на broadcast пакеты или нет
} FEC_Config_t ;


#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
__no_init static BD __rx_bd[ FEC_RX_BD_NUMBER ] ;
#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
__no_init static BD __tx_bd[ FEC_TX_BD_NUMBER ] ;

Packet_Pool_t __rx_packet_pool ;
Packet_Pool_t __tx_packet_pool ;

static size_t __rx_packet_mess_ind = 0 ;
static net_meassage_t __rx_packet_mess[ RX_PACKET_POOL_LEN ] ;
static OS_EVENT  *__NetRcvQueue = 0;
static rx_cb_t *__rx_handler = NULL ;
#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
__no_init static u8 __rx_arrays[ RX_PACKET_POOL_LEN ][ FEC_MAX_RCV_BUFF_SIZE ];
static _BOOL __rx_busy[ RX_PACKET_POOL_LEN ];

volatile s32 __tx_cnt1 = 0;
volatile s32 __tx_cnt2 = 0;

static unsigned char __last_rx_bd = FEC_RX_BD_NUMBER-1 ; // индекс последнего рассмотренного буфера
//! Семафор для защиты дескрипторов на посылку от переполнения.
static OS_EVENT *__eth_tx_sem = NULL ;


static u32 __next_txbd = 0 ;
_BOOL fec_Transmit_2_BD( u8* hdr, const size_t hdr_len, u8* body, const size_t body_len )
{
    INT8U err ;
    u32 hdr_i, body_i ;
    STORAGE_ATOMIC() ;

    assert( ((u32)hdr & 0x0F) == 0 );
    assert( ((u32)body & 0x0F) == 0 );
    assert( hdr_len >= 12 );
    assert( body_len > 0 );

    OSSemPend( __eth_tx_sem, 0, &err );
    assert( err == OS_ERR_NONE );

    hdr_i = __next_txbd ;
    if( ++__next_txbd >= FEC_TX_BD_NUMBER ){
        __next_txbd = 0 ;
    }
    body_i = __next_txbd ;
    if( ++__next_txbd >= FEC_TX_BD_NUMBER ){
        __next_txbd = 0 ;
    }

    if( (__tx_bd[ hdr_i ].bd_cstatus & MCF_FEC_TxBD_R) ||
        (__tx_bd[ body_i ].bd_cstatus & MCF_FEC_TxBD_R) ){
        return FALSE ;
    }
    __tx_bd[ hdr_i ].bd_addr = hdr ;
    __tx_bd[ hdr_i ].bd_length = hdr_len ;
    __tx_bd[ body_i ].bd_addr = body ;
    __tx_bd[ body_i ].bd_length = body_len ;

    START_ATOMIC() ;
    __tx_bd[ body_i ].bd_cstatus |= MCF_FEC_TxBD_R | MCF_FEC_TxBD_L | MCF_FEC_TxBD_TC ;
    __tx_bd[ hdr_i ].bd_cstatus |= MCF_FEC_TxBD_R | MCF_FEC_TxBD_TC ;

    __tx_cnt1++ ;
    __tx_cnt2++ ;
    STOP_ATOMIC() ;

    fec_Start_TX();
    return TRUE ;
}


_BOOL Packet_Pool_Init(void)
{
	size_t i ;
	
	__rx_packet_pool.packet_len = FEC_MAX_RCV_BUFF_SIZE ;
	__rx_packet_pool.packets_number = RX_PACKET_POOL_LEN ;
	__rx_packet_pool.packets_array = (u8**)npalloc( RX_PACKET_POOL_LEN*sizeof(u8*) );
	if( !__rx_packet_pool.packets_array ){
		return FALSE ;
	}
	__rx_packet_pool.packets_busy = __rx_busy ;
	for( i = 0; i < RX_PACKET_POOL_LEN; ++i ){
		__rx_packet_pool.packets_array[ i ] = &__rx_arrays[ i ][ 0 ];
		__rx_packet_pool.packets_busy[ i ] = FALSE ;
	}
	return TRUE ;
}


size_t pool_getfree( const Packet_Pool_t* pool )
{
	size_t i ;
	assert( pool );
	assert( pool->packets_busy );

	// TODO: придумать способ поэффективнее, например кольцевой счетчик.

	for( i = 0; i < pool->packets_number; ++i ){
		if( !pool->packets_busy[ i ] ){
			pool->packets_busy[ i ] = TRUE ;
			return i ;
		}
	}
	return UINT_MAX ;
}


void fec_init(const u32 sys_clk_MHz_, FEC_Config_t * conf_ )
{
//    s32 i ;

//    // Семафор массива дескрипторов на посылку. В 2 раза меньше количества дескрипторов,
//    // потому что на каждый пакет приходиться по 2 дескриптора -- заголовок и тело отдельно.
//    __eth_tx_sem = OSSemCreate( (u8)(FEC_TX_BD_NUMBER / 2) );

    // Сброс FEC
	MCF_FEC_ECR |=	MCF_FEC_ECR_RESET ;
	while( MCF_FEC_ECR & MCF_FEC_ECR_RESET )
	{};

    // маска прерываний
    MCF_FEC_EIMR = MCF_FEC_EIMR_ALL_MASKS ;
    // чистим все флаги прерываний
    MCF_FEC_EIR |= MCF_FEC_EIR_ALL_EVENTS ;
    
    // Должно быть: sys_clk_MHz_ / (MII_SPEED*2) <=  2.5 МГц
    MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED((u32)( sys_clk_MHz_/5 )) ;


    MCF_FEC_GAUR = 0;
    MCF_FEC_GALR = 0;
    MCF_FEC_IAUR = 0;
    MCF_FEC_IALR = 0;

    // локальный MAC-адрес
    MCF_FEC_PALR = (u32)((0
        | (conf_->mac_addr[0] <<24)
        | (conf_->mac_addr[1] <<16)
        | (conf_->mac_addr[2] <<8)
        | (conf_->mac_addr[3] <<0)));
    MCF_FEC_PAUR = (u32)((0
        | (conf_->mac_addr[4] <<24)
        | (conf_->mac_addr[5] <<16)
        | MCF_FEC_PAUR_TYPE(0x00008808)));

    // FIXME: MCF_FEC_RCR_LOOP и MCF_FEC_RCR_PROM !!!
//    MCF_FEC_RCR = MCF_FEC_RCR_MAX_FL(MAX_ETH_PKT) | MCF_FEC_RCR_MII_MODE | MCF_FEC_RCR_FCE |
//		/*MCF_FEC_RCR_PROM |*/ (conf_->rcv_broadcast > 0 ? 0: MCF_FEC_RCR_BC_REJ) ;//| MCF_FEC_RCR_LOOP ;
//    MCF_FEC_RCR = MCF_FEC_RCR_MAX_FL(MAX_ETH_PKT) | MCF_FEC_RCR_MII_MODE | MCF_FEC_RCR_FCE |
//		MCF_FEC_RCR_PROM | MCF_FEC_RCR_LOOP ;

    MCF_FEC_RCR = MCF_FEC_RCR_MAX_FL(MAX_ETH_PKT) | MCF_FEC_RCR_MII_MODE | MCF_FEC_RCR_FCE |
		MCF_FEC_RCR_PROM;
    
    // полный дуплекс
    MCF_FEC_TCR = MCF_FEC_TCR_FDEN ;

    //Program receive buffer size
    MCF_FEC_EMRBR = FEC_MAX_RCV_BUFF_SIZE ;
//    // Configure Rx BD ring
//    MCF_FEC_ERDSR = (u32)&__rx_bd[0];
//    for( i = 0; i < FEC_RX_BD_NUMBER; i++ ){
//        __rx_bd[ i ].bd_addr = 0 ;
//        __rx_bd[ i ].bd_cstatus = 0 ;
//        __rx_bd[ i ].bd_length = 0 ;
//    }
//    __rx_bd[ i-1 ].bd_cstatus |= MCF_FEC_RxBD_W ;
//
//    // Configure Tx BD ring
//    MCF_FEC_ETDSR =  (u32)&__tx_bd[0];
//    for( i = 0; i < FEC_TX_BD_NUMBER; i++ ){
//        __tx_bd[ i ].bd_addr = 0 ;
//        __tx_bd[ i ].bd_cstatus = 0 ;
//        __tx_bd[ i ].bd_length = 0 ;
//    }
//    __tx_bd[ i-1 ].bd_cstatus |= MCF_FEC_TxBD_W ;

    FECMulticastAddressSet(0x0180, 0xC2000000);

    MCF_FEC_ECR = MCF_FEC_ECR_ETHER_EN ;

    // disable MIB
    MCF_FEC_MIBC |= MCF_FEC_MIBC_MIB_DISABLE ;
    // чистим память для MIB
    t8_memzero( (u8*)pMIB, sizeof(fec_mib_t) ) ;
    // включаем MIB
    MCF_FEC_MIBC &= ~MCF_FEC_MIBC_MIB_DISABLE ;
}



//void Dnepr_Ethernet_Enable_Mgmt( const _BOOL tx_bpdu_2_mcu )
//{
//	u16	usBuffer = 0 ;
//
//	// Включаем или не включаем приём BDPU пакетов
//	// Accept 01:80:C2:00:00:00
//
//	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_ENABLE_REG, 0x0001  );	
//	MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_ENABLE_REG, 0x0001  );
//
//	if( tx_bpdu_2_mcu )
//	{
//		// 1й свичт
//		MV88E6095_multichip_smi_read( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, &usBuffer  );
//		usBuffer |= RSVD2CPU ;
//		MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, usBuffer  );
//
//		// 2й свичт
//		MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, &usBuffer  );
//		usBuffer |= RSVD2CPU ;
//		MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, usBuffer  );
//	} else {
//		// 1й свичт
//		MV88E6095_multichip_smi_read( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, &usBuffer  );
//		usBuffer = usBuffer & (0xFFFF ^ RSVD2CPU) ;
//		MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, usBuffer  );
//
//		// 2й свичт
//		MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, &usBuffer  );
//		usBuffer = usBuffer & (0xFFFF ^ RSVD2CPU) ;
//		MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL_2, MV88E6095_MGMT_REG, usBuffer  );
//	}
//}


u8* Dnepr_net_rx_callback( const size_t rx_packet_index, size_t *len )
{
	// Сначала ищем замену буферу пришедшего пакета.
	size_t free_packet_ind = pool_getfree( &__rx_packet_pool );

	__rx_packet_mess[ __rx_packet_mess_ind ].message_type = RX_PACKET ;
	__rx_packet_mess[ __rx_packet_mess_ind ].packet_index = rx_packet_index ;
	__rx_packet_mess[ __rx_packet_mess_ind ].packet_len = *len ;

	if( __NetRcvQueue ){
		OSQPost( __NetRcvQueue, (void*)&__rx_packet_mess[ __rx_packet_mess_ind ] );
	}

	++__rx_packet_mess_ind ;
	if( __rx_packet_mess_ind >= RX_PACKET_POOL_LEN ){
		__rx_packet_mess_ind = 0 ;
	}

	if( free_packet_ind >= UINT_MAX ){
		return NULL ;
	} else {
		*len = __rx_packet_pool.packet_len ;
		return __rx_packet_pool.packets_array[ free_packet_ind ];
	}
}


void Dnepr_Ethernet_Register_rx_calrx_cb_tlback( rx_cb_t rx_packet_handler )
{
	__rx_handler = rx_packet_handler ;
}

void fec_add_rx_buffer( volatile u8* rxb_, const u8 n, const size_t len )
{
    // assert(n < FEC_RX_BD_NUMBER) ;
    assert( rxb_ != NULL ) ;
    // проверка на выравнивание по границе в 16 байт

    __rx_bd[n].bd_addr = rxb_ ;
	__rx_bd[n].bd_cstatus = MCF_FEC_RxBD_E ;
	__rx_bd[n].bd_length = len ;
    // показываем, что буфер последний
    if(n == FEC_RX_BD_NUMBER-1)
        __rx_bd[n].bd_cstatus |= MCF_FEC_RxBD_W ;
}


static size_t __rx_bd_i = 0 ;
_BOOL Dnepr_Ethernet_Init_RX_BD( u8* buff, const size_t len )
{
	if( __rx_bd_i >= FEC_RX_BD_NUMBER ){
		return TRUE ;
	}

	fec_add_rx_buffer( buff, __rx_bd_i, len );

	__rx_bd_i++ ;
	if( __rx_bd_i == FEC_RX_BD_NUMBER ){
		return TRUE ;
	} else {
		return FALSE ;
	}
}


void fec_add_tx_buffer( volatile u8* txb_, const u8 n, const size_t len )
{
    // проверка на выравнивание по границе в 16 байт
    // assert(((u32)(*(u8*)&__tx_bd) & 0x0000000F) == 0);
    __tx_bd[n].bd_addr = txb_ ;
    __tx_bd[n].bd_cstatus = 0 ;
    __tx_bd[n].bd_length = len ;
    // показываем, что буфер последний
    if(n == FEC_TX_BD_NUMBER-1)
        __tx_bd[n].bd_cstatus |= MCF_FEC_TxBD_W ;
}

static size_t __tx_bd_i = 0 ;
_BOOL Dnepr_Ethernet_Init_TX_BD( u8* buff, const size_t len )
{
	if( __tx_bd_i >= FEC_TX_BD_NUMBER ){
		return TRUE ;
	}

	fec_add_tx_buffer( buff, __tx_bd_i, len );

	__tx_bd_i++ ;
	if( __tx_bd_i == FEC_TX_BD_NUMBER ){
		return TRUE ;
	} else {
		return FALSE ;
	}
}

void fec_Start_RX(void)
{
    MCF_FEC_RDAR = MCF_FEC_RDAR_R_DES_ACTIVE ;
}

void fec_Start_TX(void)
{
    MCF_FEC_TDAR |= MCF_FEC_TDAR_X_DES_ACTIVE ;
}

void T8_Dnepr_FEC_TX_Frame_Sent()
{
    assert( __eth_tx_sem );
    assert( OSSemPost( __eth_tx_sem ) == OS_ERR_NONE );
}


static size_t __tx_hdr_i ;
#define TX_HDR_NUM	16
#define TX_HDR_LEN	(6+6+sizeof(FROM_CPU_TAG)) // длина заголовка: 2 мак-адреса и FROM_CPU_TAG
#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
__no_init static u8 __tx_hdr_array[TX_HDR_NUM][TX_HDR_LEN] ;



static const u8 devs[] = { 0x01, 0x10 };
static u8 cur_dev = 0 ;
static u8 portn = 0 ;

void Dnepr_net_Fill_From_CPU_Tag( FROM_CPU_TAG* tag, const u8 port_num )
{
//	u8 dev ;
//	u8 port ;

	FROM_CPU_FIXED_FIELDS( *tag );
	FROM_CPU_T_set( *tag, 0 );
	FROM_CPU_TRG_DEV_set( *tag, devs[ cur_dev ] );
	FROM_CPU_TRG_PORT_set( *tag, portn++ );
	if( portn >= 12 ){
		portn = 0 ;
		cur_dev++ ;
		if( cur_dev >= 2 ){
			cur_dev = 0 ;
		}
	}
	// FROM_CPU_TRG_DEV_set( *tag, 0x10 );
	// FROM_CPU_TRG_PORT_set( *tag, port_num );
	FROM_CPU_C_set( *tag, 0 );
	FROM_CPU_PRI_set( *tag, 0x0 );
	FROM_CPU_VID_set( *tag, 0 );

}


//! Посылает пакет ethernet.
//! \param DA 		Массив 6ти байт с адресом назначения.
//! \param port 	Номер виртуального порта, с которого будет послан пакет (__virt2hw_portnum).
//! \param data 	Массив содержимого пакета длинной len.
//! \param len 		Длина содержимого пакета.
//! \retval Возвращает успешность размещения в выходных буферах (fec_Transmit_2_BD).
_BOOL Dnepr_net_transmit( const u8* SA, const u8* DA, const u8 port, const u8* data, const size_t len )
{
	size_t i_hdr ;
	size_t i_body ;
	size_t hdr_len = 0 ;
	OS_EVENT  * pQueue ;
	FROM_CPU_TAG from_cpu_tag ;
	_BOOL ret ;

	// Данные должны быть выровнены по 16 байт. Ещё они должны лежать во внутренней памяти.
	assert( ((u32)data & 0x0F) == 0 );

	t8_memcopy( &__tx_hdr_array[ __tx_hdr_i ][ hdr_len ], (u8*)DA, 6 );
	hdr_len += 6 ;
	t8_memcopy( &__tx_hdr_array[ __tx_hdr_i ][ hdr_len ], (u8*)SA, 6 );
	hdr_len += 6 ;

//	Dnepr_net_Fill_From_CPU_Tag( &from_cpu_tag, 0 );
//	t8_memcopy( &__tx_hdr_array[ __tx_hdr_i ][ hdr_len ], (u8*)&from_cpu_tag, sizeof( from_cpu_tag ) );
//	hdr_len += 4 ;
//
//	t8_memcopy( &__tx_hdr_array[ __tx_hdr_i ][ hdr_len ], (u8*)&from_cpu_tag, sizeof( from_cpu_tag ) );
//	hdr_len += 4 ;

	ret = fec_Transmit_2_BD( &__tx_hdr_array[ __tx_hdr_i ][ 0 ], hdr_len, (u8*)data, len );

	if( ++__tx_hdr_i >= TX_HDR_NUM ){
		__tx_hdr_i = 0 ;
	}

	return ret ;
}



u16 fec_rx_cstatus( const u8 n )
{
    return __rx_bd[ n ].bd_cstatus ;
}

size_t fec_rx_data( const u8 n, u8 volatile  ** p )
{
    if( p ){
        *p = __rx_bd[ n ].bd_addr ;
    }
    return  __rx_bd[ n ].bd_length ;
}


void isr_FEC_TxFrame_Handler()
{
	MCF_FEC_EIR |= MCF_FEC_EIR_TXF ;
	T8_Dnepr_FEC_TX_Frame_Sent() ;
	__tx_cnt1-- ;
}

void isr_FEC_ReceiveFrame_Handler()
{
	u16 c ;
	MCF_FEC_EIR |= MCF_FEC_EIR_RXF ;
	// смотрим все непустые буферы
	for( __last_rx_bd = 0; __last_rx_bd < FEC_RX_BD_NUMBER; ++__last_rx_bd ){
		c = fec_rx_cstatus( __last_rx_bd );
		if( (c & MCF_FEC_RxBD_E) == 0 ){
			u8* p ;
			size_t len = fec_rx_data( __last_rx_bd, (u8 volatile **)&p );
			if( __rx_handler ){
				p = __rx_handler( __last_rx_bd, &len );
				if( p ){
					fec_add_rx_buffer( p, __last_rx_bd, len );
				}
			}
		}
	}
	fec_Start_RX();
}



void isr_FEC_TxBuffer_Handler()
{

	MCF_FEC_EIR |= MCF_FEC_EIR_TXB ;
	__tx_cnt2-- ;
	return ;
}


/*!
\brief Инициализирует два MV88E6095 как тупой свитч, без RSTP, все порты включены
\param maddr массив 6ти байт мак-адреса, которым свитч должен слать пакеты flow control
\retval OK/ERROR
*/
u32 Dnepr_Ethernet_Init( const u8* maddr )
{
	u16	usBuffer = 0 ;
	FEC_Config_t fec_conf ;

//	u8 dest_mac_addr[8] = { 0x5C, 0xD9, 0x98, 0xF5, 0xE3, 0x14, 0} ;
//	u8 switch_mac_addr_default[8] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x00, 0} ;
	u8 uc_mac_addr_default[8] = { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0} ;
	
	MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED((u32)( SYSTEM_CLOCK_KHZ/1000/5 )) ;

        
	// Настраиваем порт MCU
	usBuffer = DSA_TAG | FORWARD_UNKNOWN | PORT_STATE(MV88E6095_PORT_FORWARDING);
	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PORT_CTRL_REG, usBuffer );
	usBuffer = MAP_DA | DEFAULT_FORWARD | CPU_PORT(MV88E6095_PORT9);
	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PORT_CTRL2_REG, usBuffer );
	usBuffer = NOT_FORCE_SPD | FORCE_LINK | LINK_FORCED_VALUE(1);
	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT9, MV88E6095_PCS_CTRL_REG, usBuffer);

	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_GLOBAL, MV88E6095_GLOBAL_2, MV88E6095_CASCADE_PORT(0x8) | MV88E6095_DEV_NUM(1)  );
	MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_GLOBAL, MV88E6095_GLOBAL_2, MV88E6095_CASCADE_PORT(0x8) | MV88E6095_DEV_NUM(0x10)  );

        
//	memcpy( fec_conf.mac_addr, uc_mac_addr_default, 6 );
	memcpy( fec_conf.mac_addr, maddr, 6 );
	fec_conf.rcv_broadcast = 1 ;


	fec_init(SYSTEM_CLOCK_KHZ/1000, &fec_conf ) ;
        
        

	// инициализация прерывания о конце передаче фрейма
	MCU_ConfigureIntr(INTR_ID_FEC_X_INTF, 6, 1 );
	MCU_EnableIntr(INTR_ID_FEC_X_INTF,1);
	// инициализация прерывания о конце передачи одного буфера
	MCU_ConfigureIntr(INTR_ID_FEC_X_INTB, 6, 2 );
	MCU_EnableIntr(INTR_ID_FEC_X_INTB,1);
	// инициализация прерывания о приёме фрейма
	MCU_ConfigureIntr(INTR_ID_FEC_R_INTF, 6, 3 );
	MCU_EnableIntr(INTR_ID_FEC_R_INTF,1);

	return TRUE ;

// Switch Management Register

	// interswitch порты

//	// PCS Control Register -- ForcedLink (88E6095 datasheet page 61 Note) и LinkValue 
//	MV88E6095_multichip_smi_read( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
//	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
//	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );
//	MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
//	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
//	MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );
//
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
//
//	return OK ; 
//
//_err:
//	return ERROR ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

















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
        s32 i ;  
//	u16	usBuffer = 0 ;
	FEC_Config_t fec_conf ;
        t_fec_config      mii_config;


//	u8 dest_mac_addr[8] = { 0x5C, 0xD9, 0x98, 0xF5, 0xE3, 0x14, 0} ;
//	u8 switch_mac_addr_default[8] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x00, 0} ;
///	u8 uc_mac_addr_default[8] = { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0} ;
	        
//	MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED((u32)( SYSTEM_CLOCK_KHZ/1000/5 )) ;       // mdio

        
	// Настраиваем порт MCU
	memcpy( fec_conf.mac_addr, mac_adress, 6 );
	fec_conf.rcv_broadcast = 1 ;


        
        // Семафор массива дескрипторов на посылку. В 2 раза меньше количества дескрипторов,
        // потому что на каждый пакет приходиться по 2 дескриптора -- заголовок и тело отдельно.
        __eth_tx_sem = OSSemCreate( (u8)(FEC_TX_BD_NUMBER / 2) );
    // Configure Rx BD ring
    MCF_FEC_ERDSR = (u32)&__rx_bd[0];
    for( i = 0; i < FEC_RX_BD_NUMBER; i++ ){
        __rx_bd[ i ].bd_addr = 0 ;
        __rx_bd[ i ].bd_cstatus = 0 ;
        __rx_bd[ i ].bd_length = 0 ;
    }
    __rx_bd[ i-1 ].bd_cstatus |= MCF_FEC_RxBD_W ;

    // Configure Tx BD ring
    MCF_FEC_ETDSR =  (u32)&__tx_bd[0];
    for( i = 0; i < FEC_TX_BD_NUMBER; i++ ){
        __tx_bd[ i ].bd_addr = 0 ;
        __tx_bd[ i ].bd_cstatus = 0 ;
        __tx_bd[ i ].bd_length = 0 ;
    }
    __tx_bd[ i-1 ].bd_cstatus |= MCF_FEC_TxBD_W ;
        
//    fec_init(SYSTEM_CLOCK_KHZ/1000, &fec_conf ) ;
        
  
    mii_config.fec_mii_speed = FEC_MII_CLOCK_DEV_CALC(2500);
    memcpy(mii_config.mac_addr, mac_adress, 6);
    mii_config.max_eth_frame = MAX_ETH_PKT;
    mii_config.max_rcv_buf = MAX_ETH_BUFF_SIZE;
    
    
    mii_config.rxbd_ring = (t_txrx_desc*)__rx_bd;
    mii_config.rxbd_ring_len = FEC_RX_BD_NUMBER;

    mii_config.txbd_ring = (t_txrx_desc*)__tx_bd;
    mii_config.txbd_ring_len = FEC_TX_BD_NUMBER;
    
    mii_config.fec_mode= FEC_MODE_MII; 
    mii_config.ignore_mac_adress_when_recv = TRUE;
    mii_config.rcv_broadcast = TRUE;    
          
    m5282_fec_init(&mii_config);
        
        
        
    // install int handlers
//    SetInterruptVector(MCF52XX_FEC_X_INTF_VECTOR + MCF52XX_INTC0_VECTOR_BASE, &mcf5282_fec_int_txf);
//    SetInterruptVector(MCF52XX_FEC_R_INTF_VECTOR + MCF52XX_INTC0_VECTOR_BASE, &mcf5282_fec_int_rxf);
//
//    sim->intc[0].icrn[MCF52XX_FEC_X_INTF_VECTOR] = MCF52XX_FEC_X_INTF_IACKLPR;              // set int level and priority
//    sim->intc[0].icrn[MCF52XX_FEC_R_INTF_VECTOR] = MCF52XX_FEC_R_INTF_IACKLPR;              // set int level and priority
//
//    EnableInterrupt(MCF52XX_FEC_X_INTF_VECTOR);                             
//                                              // enable ints
//    EnableInterrupt(MCF52XX_FEC_R_INTF_VECTOR);                             
//                                                // enable ints
        
                
        

	// инициализация прерывания о конце передаче фрейма
	MCU_ConfigureIntr(INTR_ID_FEC_X_INTF, 6, 1 );
	MCU_EnableIntr(INTR_ID_FEC_X_INTF,1);
	// инициализация прерывания о конце передачи одного буфера
	MCU_ConfigureIntr(INTR_ID_FEC_X_INTB, 6, 2 );
	MCU_EnableIntr(INTR_ID_FEC_X_INTB,1);
	// инициализация прерывания о приёме фрейма
	MCU_ConfigureIntr(INTR_ID_FEC_R_INTF, 6, 3 );
	MCU_EnableIntr(INTR_ID_FEC_R_INTF,1);

	return 0 ;
}

  








//static size_t __tx_bd_isr_i = 0 ;
//void isr_FEC_TxBuffer_Handler()
//{
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
//}



//void isr_FEC_ReceiveFrame_Handler()
//{
//    u8    ret;
//        
//    t8_m5282_fec_reset_rx_isr();
//        
////        pocket_pools.pockets_array[last_rx_buf_descr][0]
//#if LWIP_TCPIP_CORE_LOCKING_INPUT
//        
//    OSQPost( __NetRcvQueue, (void*)&__change_mac_struct );                                              /* сообщение собственному потоку */
//    
//#else        
//    // if (rx_pbuf_array[last_rx_buf_descr] == NULL)                                           /* пихаем сообщения в tcp/ip поток lwip */
//    rx_pbuf_array[last_rx_buf_descr]->payload = &default_eth.data_descr.rx_pool.pockets_array[last_rx_buf_descr][0];
//    ret = (*default_eth.netif.input)(rx_pbuf_array[last_rx_buf_descr], &default_eth.netif);
//    if ( ret == ERR_OK )    
//#endif                                                                                                    /* начинаем прием в следующий буфер если его уже обработали (очередь увеличилась) */
//    {          
//        if ( ++last_rx_buf_descr == ETHERNET_RX_BD_NUMBER ) {
//            last_rx_buf_descr = 0;
//        }
//    }
//        
//    t8_m5282_fec_set_empty_status(&rx_bd[last_rx_buf_descr].contr_status_flags);        
//    t8_m5282_fec_start_rx();
//}



