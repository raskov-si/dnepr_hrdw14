/*!
\file T8_Dnepr_Ethernet.h
\brief ��� ��� ������ � ����� � ������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/


#include "support_common.h"

//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_1_CHIPADDR			0x01
//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_2_CHIPADDR			0x10

/** Maximum packet size we can handle on a FEC. Since we don't
 * scatter/gather this is both max buffer and max frame size, and
 * applies to both transmit and receive.
 */
#define MAX_ETH_PKT 1522  /* The recommended default value to be programmed is 1518 or 1522 if VLAN tags are supported. */

#define ETHERNET_RX_BD_NUMBER    (8)      /*!< ���������� ������������ ������� �������      */
#define ETHERNET_TX_BD_NUMBER    (4)      /*!< ���������� ������������ ������� ��� �������   */


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

