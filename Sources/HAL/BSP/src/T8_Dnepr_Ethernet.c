/*!
\file T8_Dnepr_Ethernet.c
\brief ��� ��� ������ � ����� � ������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/IC/inc/MV_88E6095.h"
#include "HAL/MCU/inc/T8_5282_MII.h"

/*!
\brief �������������� ��� MV88E6095 ��� ����� �����, ��� RSTP, ��� ����� ��������
\param maddr ������ 6�� ���� ���-������, ������� ����� ������ ����� ������ flow control
\retval OK/ERROR
*/
u32 Dnepr_Ethernet_Init( const u8* maddr )
{
	u16	usBuffer = 0 ;
	s32 i ;

	MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED((u32)( SYSTEM_CLOCK_KHZ/1000/5 )) ;

// Switch Management Register

	// interswitch �����

	// PCS Control Register -- ForcedLink (88E6095 datasheet page 61 Note) � LinkValue 
	MV88E6095_multichip_smi_read( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );
	MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
	MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );

	// �������������� MAC
	if( !maddr )
		goto _err ;
	for(i=0;i<3;i++){
		usBuffer = ((u16)(maddr[i*2]) << 8) | (u16)maddr[i*2+1];
		MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer );
		if(i==2)
			MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer+1 );
		else
			MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR,  MV88E6095_GLOBAL, (u8)(i+1), (u16)usBuffer );
	}

	return OK ; 

_err:
	return ERROR ;
}

/*=============================================================================================================*/
/*!  \brief ��������� ������� ��������������� ���� ���� auto-negotiation ��� SFP
     \details ��������� ��� �������� ����� � ����������� �� ����� mode_flag ��� ����������� ����� switch'� 
     \details MV88E6095 PORT9 ��� SFP1, PORT10 ��� SFP2
     \details �� ��������� ����� ������ ������ ����� �������

     \sa MV88E6095_multichip_smi_read, cmsfpautoneg_update 
*/
/*=============================================================================================================*/
void dnepr_ethernet_sfpport_autoneg_mode
( 
    u8      sfp_num,                        /*!< [in] ����� sfp 1,2                                   */
    u8      mode_flag                       /*!< [in] �������� ��� ��������� �����  0 - ����, 1 - ��� */
)
{  
    u8      port;
    u16     temp_reg;
    
    switch (sfp_num)
    {
    case 1:     port = MV88E6095_PORT10;     break;  /* sfp1 - ������  L port10  */
    case 2:     port = MV88E6095_PORT9;      break;  /* sfp2 - ������� U port9   */
    default:    return;
    }
  
    MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, port, MV88E6095_PCS_CTRL_REG, &temp_reg  );  
    if ( mode_flag == 0 )    {
        /* ��������� ����� auto-negotiation */
        temp_reg &= ~(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_MASK | MV88E6095_PCS_CTRL_REG_AUTONEG_ENABLE);        
        temp_reg |= MV88E6095_PCS_CTRL_REG_FORCE_SPEED(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_1000B);
    } else {
        /* �������� ����� auto-negotiation */
        temp_reg &= ~(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_MASK);        
        temp_reg |= MV88E6095_PCS_CTRL_REG_FORCE_SPEED(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_AUTO) | MV88E6095_PCS_CTRL_REG_AUTONEG_ENABLE ;
    }
    MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, port, MV88E6095_PCS_CTRL_REG, temp_reg  );
}
