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


/** Maximum packet size we can handle on a FEC. Since we don't
 * scatter/gather this is both max buffer and max frame size, and
 * applies to both transmit and receive.
 */
#define MAX_ETH_PKT                 1522    /*! The recommended default value to be programmed is 1518 or 1522 if VLAN tags are supported. */
#define MAX_ETH_BUFF_SIZE 	    1536    /*! длина отдельного буфера в приёмнике, д.б. не меньше MAX_ETH_PKT и делиться на 16 */


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
                                                u8      sfp_num,        /*!< [in] номер sfp 1,2                                   */
                                                u8      mode_flag       /*!< [in] включить или выключить режим  0 - выкл, 1 - вкл */
                                            );
void    dnepr_ethernet_str_2_mac            ( 
                                                u8*         out,        /*!< [out] получаемое числовое значение (массив из 6 элементов)   */
                                                const char* str         /*!< [in]  входящая строка с IP-адресом                           */
                                            );

/*=============================================================================================================*/

