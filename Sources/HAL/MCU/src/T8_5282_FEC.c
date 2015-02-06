/*
 * File:    mii.c
 * Purpose:     
 *
 * Notes:       
 */

#include "support_common.h"
#include "HAL\MCU\inc\T8_5282_FEC.h"


/*=============================================================================================================*/
/*! \brief   
    \return 
    \retval 
    \sa  
*/
/*=============================================================================================================*/
void t8_m5282_fec_init
(
    t_fec_config    *input
)
{        
    MCF_FEC_ECR |= MCF_FEC_ECR_RESET;                                   /* Сброс FEC */
    while( MCF_FEC_ECR & MCF_FEC_ECR_RESET ) {
        continue;
    };
    
    MCF_FEC_EIMR = MCF_FEC_EIMR_UNMASK_ALL;                             /*  чистим все флаги прерываний */    
    MCF_FEC_EIR  = MCF_FEC_EIR_CLEAR_ALL;

    MCF_FEC_GAUR = 0;                                                   /* чистим хэши для распознавания адресов при групповом и индивидуальном приеме */
    MCF_FEC_GALR = 0;
    MCF_FEC_IAUR = 0;
    MCF_FEC_IALR = 0;
    
    MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED( input->fec_mii_speed ) ;     /* инициализируем делитель для системной частоты MII_SPEED */
                                                    
                                                                        /* записываем MAC-адрес  */
    MCF_FEC_PALR = (u32)((0
        | (input->mac_addr[0] <<24)
        | (input->mac_addr[1] <<16)
        | (input->mac_addr[2] <<8)
        | (input->mac_addr[3] <<0)));
    MCF_FEC_PAUR = (u32)((0
        | (input->mac_addr[4] <<24)
        | (input->mac_addr[5] <<16)
        | MCF_FEC_PAUR_TYPE(0x00008808)));
    
    input->fec_max_eth_pocket = ( input->fec_max_eth_pocket > 0 ) ? input->fec_max_eth_pocket : 1518;
    // FIXME: MCF_FEC_RCR_PROM !!!
    MCF_FEC_RCR = MCF_FEC_RCR_MAX_FL(input->fec_max_eth_pocket) 
                  |  MCF_FEC_RCR_FCE 
                  | ((input->fec_mode == FEC_MODE_7WIRE) ? FEC_MODE_7WIRE : MCF_FEC_RCR_MII_MODE )
                  | (input->rcv_broadcast_clock > 0 ? 0: MCF_FEC_RCR_BC_REJ);
                  
    if (input->fec_mode == FEC_MODE_LOOPBACK) {
        MCF_FEC_RCR |= MCF_FEC_RCR_LOOP;
        MCF_FEC_RCR &= ~MCF_FEC_RCR_DRT;
    }
                  
    
//  MCF_FEC_ETDSR = (uint32_t)txbd_ring;
//  MCF_FEC_ERDSR = (uint32_t)rxbd_ring;
  
//  MCF_FEC_EMRBR = LWIP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE);
//  MCF_FEC_ECR |= ECR_ETHER_EN;
}


//void fec_init(const u32 sys_clk_MHz_, FEC_Config_t * conf_ )
//{
//    s32 i ;
//
////    // Семафор массива дескрипторов на посылку. В 2 раза меньше количества дескрипторов,
////    // потому что на каждый пакет приходиться по 2 дескриптора -- заголовок и тело отдельно.
////    __eth_tx_sem = OSSemCreate( (u8)(FEC_TX_BD_NUMBER / 2) );
//
//
//
//
//
//
//
//    // полный дуплекс
//    MCF_FEC_TCR = MCF_FEC_TCR_FDEN ;
//
//    //Program receive buffer size
//    MCF_FEC_EMRBR = FEC_MAX_RCV_BUFF_SIZE ;
//    // Configure Rx BD ring
//    MCF_FEC_ERDSR = (u32)&__rx_bd[0];
//    for( i = 0; i < FEC_RX_BD_NUMBER; i++ ){
//        __rx_bd[ i ].bd_addr = 0 ;
//        __rx_bd[ i ].bd_cstatus = 0 ;
//        __rx_bd[ i ].bd_length = 0 ;
//    }
//    __rx_bd[ i-1 ].bd_cstatus |= MCF_FEC_RxBD_W ;
//
//    // Configure Tx BD ring
//    MCF_FEC_ETSDR =  (u32)&__tx_bd[0];
//    for( i = 0; i < FEC_TX_BD_NUMBER; i++ ){
//        __tx_bd[ i ].bd_addr = 0 ;
//        __tx_bd[ i ].bd_cstatus = 0 ;
//        __tx_bd[ i ].bd_length = 0 ;
//    }
//    __tx_bd[ i-1 ].bd_cstatus |= MCF_FEC_TxBD_W ;
//
//    FECMulticastAddressSet(0x0180, 0xC2000000);
//
//    MCF_FEC_ECR = MCF_FEC_ECR_ETHER_EN ;
//
//    // disable MIB
//    MCF_FEC_MIBC |= MCF_FEC_MIBC_MIB_DISABLE ;
//    // чистим память для MIB
//    t8_memzero( (u8*)pMIB, sizeof(FEC_MIB_t) ) ;
//    // включаем MIB
//    MCF_FEC_MIBC &= ~MCF_FEC_MIBC_MIB_DISABLE ;
//}
//







/********************************************************************/
/*
 * Write a value to a PHY's MII register.
 *
 * Parameters:
 *  phy_addr    Address of the PHY.
 *  reg_addr    Address of the register in the PHY.
 *  data        Data to be written to the PHY register.
 *
 * Return Values:
 *  0 on failure
 *  1 on success.
 *
 * Please refer to your PHY manual for registers and their meanings.
 * mii_write() polls for the FEC's MII interrupt event and clears it. 
 * If after a suitable amount of time the event isn't triggered, a 
 * value of 0 is returned.
 */
u32 t8_m5282_fec_mdio_write(u32 phy_addr, u32 reg_addr, u16 data)
{
    u32 timeout;
    u32 eimr;

    /* Clear the MII interrupt bit */
    MCF_FEC_EIR = MCF_FEC_EIR_MII;

    /* Mask the MII interrupt */
    eimr = MCF_FEC_EIMR;
    MCF_FEC_EIMR &= ~MCF_FEC_EIMR_MII;

    /* Write to the MII Management Frame Register to kick-off the MII write */
    MCF_FEC_MMFR = (u32)(0
        | MCF_FEC_MMFR_ST_01
        | MCF_FEC_MMFR_OP_WRITE
        | MCF_FEC_MMFR_PA(phy_addr)
        | MCF_FEC_MMFR_RA(reg_addr)
        | MCF_FEC_MMFR_TA_10
        | MCF_FEC_MMFR_DATA(data));

    /* Poll for the MII interrupt (interrupt should be masked) */
    for (timeout = 0; timeout < FEC_MII_TIMEOUT; timeout++)
    {
        if (MCF_FEC_EIR & MCF_FEC_EIR_MII)
            break;
    }
    
    if(timeout == FEC_MII_TIMEOUT)
        return 0;

    /* Clear the MII interrupt bit */
    MCF_FEC_EIR = MCF_FEC_EIR_MII;

    /* Restore the EIMR */
    MCF_FEC_EIMR = eimr;

    return 1;
}


/********************************************************************/
/*
 * Read a value from a PHY's MII register.
 *
 * Parameters:
 *  phy_addr    Address of the PHY.
 *  reg_addr    Address of the register in the PHY.
 *  data        Pointer to storage for the Data to be read
 *              from the PHY register (passed by reference)
 *
 * Return Values:
 *  0 on failure
 *  1 on success.
 *
 * Please refer to your PHY manual for registers and their meanings.
 * mii_read() polls for the FEC's MII interrupt event and clears it. 
 * If after a suitable amount of time the event isn't triggered, a 
 * value of 0 is returned.
 */
u32 t8_m5282_fec_mdio_read(u32 phy_addr, u32 reg_addr, u16* data)
{
    u32 timeout;
    u32 eimr;

    /* Clear the MII interrupt bit */
    MCF_FEC_EIR = MCF_FEC_EIR_MII;

    /* Mask the MII interrupt */
    eimr = MCF_FEC_EIMR;
    MCF_FEC_EIMR &= ~MCF_FEC_EIMR_MII;

    /* Write to the MII Management Frame Register to kick-off the MII read */
    MCF_FEC_MMFR = (u32)(0
        | MCF_FEC_MMFR_ST_01
        | MCF_FEC_MMFR_OP_READ
        | MCF_FEC_MMFR_PA(phy_addr)
        | MCF_FEC_MMFR_RA(reg_addr)
        | MCF_FEC_MMFR_TA_10);

    /* Poll for the MII interrupt (interrupt should be masked) */
    for (timeout = 0; timeout < FEC_MII_TIMEOUT; timeout++)
    {
        if (MCF_FEC_EIR & MCF_FEC_EIR_MII)
            break;
    }

    if(timeout == FEC_MII_TIMEOUT)
        return 0;

    /* Clear the MII interrupt bit */
    MCF_FEC_EIR = MCF_FEC_EIR_MII;

    /* Restore the EIMR */
    MCF_FEC_EIMR = eimr;

    *data = (u16)(MCF_FEC_MMFR & 0x0000FFFF);

    return 1;
}
/********************************************************************/



