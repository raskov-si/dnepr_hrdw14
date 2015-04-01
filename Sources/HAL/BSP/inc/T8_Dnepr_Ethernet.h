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


/** Maximum packet size we can handle on a FEC. Since we don't
 * scatter/gather this is both max buffer and max frame size, and
 * applies to both transmit and receive.
 */
#define MAX_ETH_PKT                 1522    /*! The recommended default value to be programmed is 1518 or 1522 if VLAN tags are supported. */
#define MAX_ETH_BUFF_SIZE 	    1536    /*! ����� ���������� ������ � ��������, �.�. �� ������ MAX_ETH_PKT � �������� �� 16 */


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

