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
