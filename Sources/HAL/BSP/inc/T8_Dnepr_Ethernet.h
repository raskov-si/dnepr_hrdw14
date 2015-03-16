/*!
\file T8_Dnepr_Ethernet.h
\brief ��� ��� ������ � ����� � ������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/


#include "support_common.h"
#include "lwip/netif.h"


//    if (!LWIP_HOOK_VLAN_CHECK(netif, ethhdr, vlan)) {
//#elif defined(ETHARP_VLAN_CHECK_FN)
//    if (!ETHARP_VLAN_CHECK_FN(ethhdr, vlan)) {
//#elif defined(ETHARP_VLAN_CHECK)
//    if (VLAN_ID(vlan) != ETHARP_VLAN_CHECK) {


//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_1_CHIPADDR			0x01
//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_2_CHIPADDR			0x10

/** Maximum packet size we can handle on a FEC. Since we don't
 * scatter/gather this is both max buffer and max frame size, and
 * applies to both transmit and receive.
 */
#define MAX_ETH_PKT             1522    /*! The recommended default value to be programmed is 1518 or 1522 if VLAN tags are supported. */
#define MAX_ETH_BUFF_SIZE 	1536    /*! ����� ���������� ������ � ��������, �.�. �� ������ MAX_ETH_PKT � �������� �� 16 */


#define ETHERNET_RX_BD_NUMBER    (8)      /*!< ���������� ������������ ������� �������      */
#define ETHERNET_TX_BD_NUMBER    (4)      /*!< ���������� ������������ ������� ��� �������   */

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

struct net_meassage_t_
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
  struct net_meassage_t_         *rx_cur_message;
  struct net_meassage_t_         *tx_cur_message;  
#endif  
};


struct ETH_DESC {
    struct netif            netif;
    struct ETH_POOL_DESC    data_descr;
};

#pragma pack(pop)


typedef struct POCKET_POOL      t_eth_pocket_pool;
typedef struct net_meassage_t_  t_net_message;
typedef struct ETH_POOL_DESC    t_eth_data_descr;
typedef struct ETH_DESC         t_eth;




_BOOL  dnepr_ethernet_open 
(
    t_eth    *out_descr
);

u32 Dnepr_Ethernet_Init( const u8* maddr );

int dnepr_ethernet_fec_init
(
    const u8 *mac_adress
);

int dnepr_ethernet_phy_init(void);


void dnepr_ethernet_sfpport_autoneg_mode
( 
    u8      sfp_num,                        /*!< [in] ����� sfp 1,2                                   */
    u8      mode_flag                       /*!< [in] �������� ��� ��������� �����  0 - ����, 1 - ��� */
);

void dnepr_ethernet_str_2_mac
( 
    u8*         out,            /*!< [out] ���������� �������� �������� (������ �� 6 ���������)   */
    const char* str             /*!< [in]  �������� ������ � IP-�������                           */
);

