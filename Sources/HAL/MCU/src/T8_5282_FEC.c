/*
 * File:    mii.c
 * Purpose:     
 *
 * Notes:       
 */

#include "support_common.h"
#include "common_lib/crc.h"
#include "common_lib\memory.h"


#include "HAL\MCU\inc\T8_5282_FEC.h"


/*=============================================================================================================*/

static fec_mib_t *p_mib = (fec_mib_t*)&MCF_FEC_RMON_T_DROP ;


/*=============================================================================================================*/
/*! \brief   
    \return 
    \retval 
    \sa  
*/
/*=============================================================================================================*/
static u32 fec_multicast_address_set
(
    u16 mac_upper2, 
    u32 mac_lower4
)
{
    u8 mac[8];
    u8 hash_pos;
    mac[0] = (u8)(mac_upper2>>8);
    mac[1] = (u8)(mac_upper2);
    mac[2] = (u8)(mac_lower4>>24);
    mac[3] = (u8)(mac_lower4>>16);
    mac[4] = (u8)(mac_lower4>>8);
    mac[5] = (u8)(mac_lower4>>0);
    hash_pos = (~Crc32(mac, 6) >> 26);
    if (hash_pos & 0x20)  {      
        MCF_FEC_GAUR |= 1 << (0x1F & hash_pos);
    }  else  {
        MCF_FEC_GALR |= 1 << (0x1F & hash_pos);
    }
    return OK;
}


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
/* 
 * Инициализация, которую необходимо сделать пользователю. Последовательность выполнения не важна 
 *   Initialize EIMR
 *   Clear EIR (write 0xFFFF_FFFF)
 *   TFWR (optional)
 *   IALR / IAUR
 *   GAUR / GALR
 *   PALR / PAUR (only needed for full duplex flow control)
 *   OPD (only needed for full duplex flow control)
 *   RCR
 *   TCR
 *   MSCR (optional)
 *   Clear MIB_RAM  
 */
  
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
    
    input->fec_max_eth_pocket = ( input->fec_max_eth_pocket > 0 ) ? input->fec_max_eth_pocket : 1518;   /* инициализация приемника */
    MCF_FEC_RCR =    MCF_FEC_RCR_MAX_FL(input->fec_max_eth_pocket) 
                  |  MCF_FEC_RCR_FCE 
                  | ((input->fec_mode == FEC_MODE_7WIRE) ? FEC_MODE_7WIRE : MCF_FEC_RCR_MII_MODE )
                  | (input->ignore_mac_adress_when_recv > 0 ? MCF_FEC_RCR_PROM : 0)
                  | (input->rcv_broadcast_clock > 0 ? 0: MCF_FEC_RCR_BC_REJ);
                  
    if (input->fec_mode == FEC_MODE_LOOPBACK) {
        MCF_FEC_RCR |= MCF_FEC_RCR_LOOP;
        MCF_FEC_RCR &= ~MCF_FEC_RCR_DRT;
    }

    MCF_FEC_TCR = MCF_FEC_TCR_FDEN ;                                                  /* инициализация передатчика */
    
        
    input->rxbd_ring[ input->rxbd_ring_len-1 ].contr_status_inf |= MCF_FEC_RxBD_W ;   /* инициализация приемного буфера */   
    MCF_FEC_ERDSR = (u32)input->rxbd_ring;
    MCF_FEC_EMRBR = input->rxbd_ring_len;
    

    input->txbd_ring[ input->txbd_ring_len-1 ].contr_status_inf |= MCF_FEC_TxBD_W ;  /* инициализация передающего буфера */  
    MCF_FEC_ETSDR = (uint32_t)input->txbd_ring;
    
      
    MCF_FEC_ECR |= MCF_FEC_ECR_ETHER_EN ;                                            /* включаем приемник с передатчиком */

    fec_multicast_address_set(0x0180, 0xC2000000);
    
    // disable MIB
    MCF_FEC_MIBC |= MCF_FEC_MIBC_MIB_DISABLE ;
    // чистим память для MIB
    t8_memzero( (u8*)p_mib, sizeof(fec_mib_t) ) ;
    // включаем MIB
    MCF_FEC_MIBC &= ~MCF_FEC_MIBC_MIB_DISABLE ;
}




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



