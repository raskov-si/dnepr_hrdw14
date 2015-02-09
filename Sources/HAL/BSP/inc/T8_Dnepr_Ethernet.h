/*!
\file T8_Dnepr_Ethernet.h
\brief Код для работы с сетью в Днепре
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/


#include "support_common.h"

//! адрес  первого MV88E6095 по SMI в multichip mode
#define MV88E6095_1_CHIPADDR			0x01
//! адрес  второго MV88E6095 по SMI в multichip mode
#define MV88E6095_2_CHIPADDR			0x10

/** Maximum packet size we can handle on a FEC. Since we don't
 * scatter/gather this is both max buffer and max frame size, and
 * applies to both transmit and receive.
 */
#define MAX_ETH_PKT 1522  /* The recommended default value to be programmed is 1518 or 1522 if VLAN tags are supported. */

#define ETHERNET_RX_BD_NUMBER    (8)      /*!< количество дескрипторов приёмных буферов      */
#define ETHERNET_TX_BD_NUMBER    (4)      /*!< количество дескрипторов буферов для отсылки   */


u32 Dnepr_Ethernet_Init( const u8* maddr );

int dnepr_ethernet_fec_init
(
    const u8 *mac_adress
);

int dnepr_ethernet_phy_init(void);


void dnepr_ethernet_sfpport_autoneg_mode
( 
    u8      sfp_num,                        /*!< [in] номер sfp 1,2                                   */
    u8      mode_flag                       /*!< [in] включить или выключить режим  0 - выкл, 1 - вкл */
);

void dnepr_ethernet_str_2_mac
( 
    u8*         out,            /*!< [out] получаемое числовое значение (массив из 6 элементов)   */
    const char* str             /*!< [in]  входящая строка с IP-адресом                           */
);

