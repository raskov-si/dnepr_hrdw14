/*!
\file T8_Dnepr_Ethernet.h
\brief Код для работы с сетью в Днепре
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/


#include "support_common.h"
#include "lwip/netif.h"
#include "HAL/MCU/inc/T8_5282_FEC.h"


/*=============================================================================================================*/

//! адрес  первого MV88E6095 по SMI в multichip mode
#define MV88E6095_1_CHIPADDR			0x01
//! адрес  второго MV88E6095 по SMI в multichip mode
#define MV88E6095_2_CHIPADDR			0x10

/** Maximum packet size we can handle on a FEC. Since we don't
 * scatter/gather this is both max buffer and max frame size, and
 * applies to both transmit and receive.
 */
#define MAX_ETH_PKT                 1522    /*! The recommended default value to be programmed is 1518 or 1522 if VLAN tags are supported. */
#define MAX_ETH_BUFF_SIZE 	    1536    /*! длина отдельного буфера в приёмнике, д.б. не меньше MAX_ETH_PKT и делиться на 16 */


#define ETHERNET_RX_BD_NUMBER       (8)      /*!< количество дескрипторов приёмных буферов      */
#define ETHERNET_TX_BD_NUMBER       (4)      /*!< количество дескрипторов буферов для отсылки   */




/*=============================================================================================================*/


/*=============================================================================================================*/

_BOOL   dnepr_ethernet_lwip_open            (struct netif*);
int     dnepr_ethernet_fec_init             (
                                                const u8 *mac_adress
                                            );
int     dnepr_ethernet_phy_init             (void);
void    dnepr_ethernet_sfpport_autoneg_mode ( 
                                                u8      sfp_num,        /*!< [in] номер sfp 1,2                                   */
                                                u8      mode_flag       /*!< [in] включить или выключить режим  0 - выкл, 1 - вкл */
                                            );
void    dnepr_ethernet_str_2_mac            ( 
                                                u8*         out,        /*!< [out] получаемое числовое значение (массив из 6 элементов)   */
                                                const char* str         /*!< [in]  входящая строка с IP-адресом                           */
                                            );

/*=============================================================================================================*/


//! Пул массивов для пакетов.
#pragma pack(push)
#pragma pack(4)

struct POCKET_POOL
{
	size_t 	pocket_len ; 	        //!< Длина буфера одного пакета.
	size_t 	pockets_number ;        //!< Сколько всего буферов пакетов.
	_BOOL 	*pockets_busy;	        //!< Массив с булами, определяющими содержит ли конкретный пакет данные.
	u8	**pockets_array;	//!< Массив с буферами данных пакетов.
};

struct STR_NET_MESSAGE
{
//	enum {
////		CHANGE_MAC,
//		RX_PACKET,
//		TX_PACKET
////		TIMER
//	} message_type ;

	size_t packet_index ;
	size_t packet_len ;
//	u8 new_local_mac[ 6 ];
};


struct ETH_POOL_DESC {
  struct POCKET_POOL     rx_pool;
  struct POCKET_POOL     tx_pool;  
  
#if LWIP_TCPIP_CORE_LOCKING_INPUT  
  struct STR_NET_MESSAGE         *rx_cur_message;
  struct STR_NET_MESSAGE         *tx_cur_message;  
#endif  
};


struct ETH_DESC {
    struct netif            netif;
    struct ETH_POOL_DESC    data_descr;
};

#pragma pack(pop)


typedef struct POCKET_POOL      t_eth_pocket_pool;
typedef struct STR_NET_MESSAGE  t_net_message;
typedef struct ETH_POOL_DESC    t_eth_data_descr;
typedef struct ETH_DESC         t_eth;


//_BOOL  dnepr_ethernet_open 
//(
//    t_eth    *out_descr
//);

u32 Dnepr_Ethernet_Init( const u8* maddr );




typedef u8* (rx_cb_t)( const size_t rx_packet_index, size_t *len );

//! Пул массивов для пакетов.
typedef struct Packet_Pool_t_
{
	size_t 	packet_len ; 	//!< Длина буфера одного пакета.
	size_t 	packets_number ;//!< Сколько всего буферов пакетов.
	_BOOL* 	packets_busy ;	//!< Массив с булами, определяющими содержит ли конкретный пакет данные.
	u8**	packets_array ;	//!< Массив с буферами данных пакетов.
} Packet_Pool_t ;

void    Dnepr_Ethernet_Enable_Mgmt( const _BOOL tx_bpdu_2_mcu );
u8*     Dnepr_net_rx_callback( const size_t rx_packet_index, size_t *len );
void Dnepr_Ethernet_Register_rx_calrx_cb_tlback( rx_cb_t rx_packet_handler );
_BOOL Dnepr_Ethernet_Init_RX_BD( u8* buff, const size_t len );
_BOOL Dnepr_Ethernet_Init_TX_BD( u8* buff, const size_t len );
size_t pool_getfree( const Packet_Pool_t* pool );
void fec_Start_RX(void);
void fec_Start_TX(void);
_BOOL Packet_Pool_Init(void);
_BOOL Dnepr_net_transmit( const u8* SA, const u8* DA, const u8 port, const u8* data, const size_t len );

