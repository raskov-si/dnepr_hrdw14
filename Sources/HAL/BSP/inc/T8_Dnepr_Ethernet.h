/*!
\file T8_Dnepr_Ethernet.h
\brief ��� ��� ������ � ����� � ������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/


#include "support_common.h"
#include "lwip/netif.h"
#include "HAL/MCU/inc/T8_5282_FEC.h"


/*=============================================================================================================*/

//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_1_CHIPADDR			0x01
//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_2_CHIPADDR			0x10

/** Maximum packet size we can handle on a FEC. Since we don't
 * scatter/gather this is both max buffer and max frame size, and
 * applies to both transmit and receive.
 */
#define MAX_ETH_PKT                 1522    /*! The recommended default value to be programmed is 1518 or 1522 if VLAN tags are supported. */
#define MAX_ETH_BUFF_SIZE 	    1536    /*! ����� ���������� ������ � ��������, �.�. �� ������ MAX_ETH_PKT � �������� �� 16 */


#define ETHERNET_RX_BD_NUMBER       (8)      /*!< ���������� ������������ ������� �������      */
#define ETHERNET_TX_BD_NUMBER       (4)      /*!< ���������� ������������ ������� ��� �������   */




/*=============================================================================================================*/


/*=============================================================================================================*/

_BOOL   dnepr_ethernet_lwip_open            (struct netif*);
int     dnepr_ethernet_fec_init             (
                                                const u8        *mac_adress,    
                                                t_txrx_desc*    rx_desc_buf,
                                                size_t          rx_buf_len,
                                                t_txrx_desc*    tx_desc_buf,
                                                size_t          tx_buf_len                                                  
                                            );
int     dnepr_ethernet_phy_init             (void);
void    dnepr_ethernet_sfpport_autoneg_mode ( 
                                                u8      sfp_num,        /*!< [in] ����� sfp 1,2                                   */
                                                u8      mode_flag       /*!< [in] �������� ��� ��������� �����  0 - ����, 1 - ��� */
                                            );
void    dnepr_ethernet_str_2_mac            ( 
                                                u8*         out,        /*!< [out] ���������� �������� �������� (������ �� 6 ���������)   */
                                                const char* str         /*!< [in]  �������� ������ � IP-�������                           */
                                            );

/*=============================================================================================================*/


//! ��� �������� ��� �������.
#pragma pack(push)
#pragma pack(4)

struct POCKET_POOL
{
	size_t 	pocket_len ; 	        //!< ����� ������ ������ ������.
	size_t 	pockets_number ;        //!< ������� ����� ������� �������.
	_BOOL 	*pockets_busy;	        //!< ������ � ������, ������������� �������� �� ���������� ����� ������.
	u8	**pockets_array;	//!< ������ � �������� ������ �������.
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

//! ��� �������� ��� �������.
typedef struct Packet_Pool_t_
{
	size_t 	packet_len ; 	//!< ����� ������ ������ ������.
	size_t 	packets_number ;//!< ������� ����� ������� �������.
	_BOOL* 	packets_busy ;	//!< ������ � ������, ������������� �������� �� ���������� ����� ������.
	u8**	packets_array ;	//!< ������ � �������� ������ �������.
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

