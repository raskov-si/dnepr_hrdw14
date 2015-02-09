/*!
\file T8_Dnepr_Ethernet.c
\brief ��� ��� ������ � ����� � ������
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "common_lib/memory.h"

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/IC/inc/MV_88E6095.h"
#include "HAL/MCU/inc/T8_5282_FEC.h"


/*=============================================================================================================*/

#pragma data_alignment=16
_Pragma("location=\"packets_sram\"")
__no_init static t_txrx_desc rx_bd[ ETHERNET_RX_BD_NUMBER ] ;
#pragma data_alignment=16
_Pragma("location=\"packets_sram\"")
__no_init static t_txrx_desc tx_bd[ ETHERNET_TX_BD_NUMBER ] ;

/*=============================================================================================================*/


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



/*=============================================================================================================*/
/*!  \brief ������������� FEC ��� �����
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_fec_init
(
    const u8 *mac_adress
)
{
  t_fec_config      mii_config;
  
  mii_config.fec_mii_speed = FEC_MII_CLOCK_DEV_CALC(2500);
  memcpy(mii_config.mac_addr, mac_adress, 6);
  mii_config.fec_max_eth_pocket = MAX_ETH_PKT;
  
  
  t8_m5282_fec_init(&mii_config);
  
  return 0;
}

  
/*=============================================================================================================*/
/*!  \brief ������������� PHY ��� �����
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_phy_init(void)
{
    return 0;
}

/*=============================================================================================================*/
/*!  \brief ������������� switch, VLAN
*
*   \sa 
*/
/*=============================================================================================================*/
//int dnepr_ethernet_switch_init(void)


/*=============================================================================================================*/
/*!  \brief ��������� MAC-����� �� ���������� � �������� ���������
*/
/*=============================================================================================================*/
void dnepr_ethernet_str_2_mac
( 
    u8*         out,            /*!< [out] ���������� �������� �������� (������ �� 6 ���������)   */
    const char* str             /*!< [in]  �������� ������ � IP-�������                           */
)
{
	u32 flag=FALSE;
	s8 mac_byte[3];
	u8 new_mac_addr[6];
	u8 mac_byte_num=0;
	u8 i,k, data_len;
	flag = TRUE;

	if( !out || !str ){
		return ;
	}

	data_len = strnlen( str, 18 );

	for (i=0, k=0; (i<data_len) && (flag == TRUE); i++) {
		if ((str[i] != ':') && (str[i] != '-') && (str[i] != '.')) {
			if (k<2)
				mac_byte[k++] = str[i];
			else
				flag = FALSE;
		}
		else if (k && (k<3)) {
			mac_byte[k] = 0;
			k = 0;
			errno = 0;
			if (mac_byte_num < 6) {
				new_mac_addr[mac_byte_num++] = strtol(mac_byte, NULL, 16);
				if (errno == ERANGE)
					flag = FALSE;
			}
			else
				flag = FALSE;
		}
	}
//Last byte. We don't find delimiter, but find 0
	if ((mac_byte_num == 5) && (flag == TRUE))
		if (k && (k<3)) {
			mac_byte[k] = 0;
			errno = 0;
			new_mac_addr[mac_byte_num++] = strtol(mac_byte, NULL, 16);
			if (errno == ERANGE)
				flag = FALSE;
	}
	if (flag == TRUE){
		memcpy( out, new_mac_addr, sizeof(new_mac_addr) );
	}
}



/*=============================================================================================================*/
/*!  \brief ��������� MAC-����� �� ��������� � ��������� ���������
*/
/*=============================================================================================================*/
void dnepr_ethernet_mac_2_str
( 
    char        *str,           /*!< [out]  ��������� ������ � IP-�������                           */
    const u8    *mac            /*!< [in]   �������� �������� MAC-������ (������ �� 6 ���������)    */    
)
{
	size_t i ;
	size_t istr = 0 ;
	static const char alf[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	assert( str != NULL );

	for( i = 0; i < 6; ++i ){
		str[istr++] = alf[ (mac[ i ] & 0xF0) >> 4 ];
		str[istr++] = alf[mac[ i ] & 0x0F ];
		if( i < 5 ){
			str[istr++] = ':' ;
		}
	}
}
