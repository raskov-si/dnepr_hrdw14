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

void dnepr_ethernet_str_2_mac
( 
    u8*         out,            /*!< [out] ���������� �������� �������� (������ �� 6 ���������)   */
    const char* str             /*!< [in]  �������� ������ � IP-�������                           */
);

