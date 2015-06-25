/*!
\file T8_Dnepr_Ethernet.h
\brief ��� ��� ������ � ����� � ������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_1_CHIPADDR			0x01
//! �����  ������� MV88E6095 �� SMI � multichip mode
#define MV88E6095_2_CHIPADDR			0x10

#include "support_common.h"

u32 Dnepr_Ethernet_Init( const u8* maddr );
void dnepr_ethernet_sfpport_autoneg_mode
( 
    u8      sfp_num,                        /*!< [in] ����� sfp 1,2                                   */
    u8      mode_flag                       /*!< [in] �������� ��� ��������� �����  0 - ����, 1 - ��� */
);

_BOOL dnepr_ethernet_get_FEport_link_status
( 
    u8      num                               /*!< [in] ����� FE 1,2,3                                   */
);

_BOOL dnepr_ethernet_get_GEport_link_status
( 
    u8      num                               /*!< [in] ����� GE 1,2                                   */
);

u32 Dnepr_Ethernet_Reset( void );
