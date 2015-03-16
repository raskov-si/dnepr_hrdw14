/*!
\file T8_Dnepr_Ethernet.c
\brief Код для работы с сетью в Днепре
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "common_lib/memory.h"

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/IC/inc/MV_88E6095.h"
#include "HAL/MCU/inc/T8_5282_FEC.h"
#include "HAL/MCU/inc/T8_5282_interrupts.h"

#include "lwip/include/arch/sys_arch.h"
#include "lwip/include/netif/etharp.h"
#include "lwip/include/lwip/opt.h"
#include "lwip/include/lwip/sys.h"
#include "lwip/include/lwip/stats.h"
#include "lwip/include/lwip/dhcp.h"
#include "lwip/tcpip.h"



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

/*=============================================================================================================*/

#define RX_PACKET_POOL_LEN	ETHERNET_RX_BD_NUMBER        
#define TX_PACKET_POOL_LEN	ETHERNET_TX_BD_NUMBER

#define RX_QUEUE_LEN	        ETHERNET_RX_BD_NUMBER        
#define TX_QUEUE_LEN	        ETHERNET_TX_BD_NUMBER




#define PAYLOAD2PBUF(p) (struct pbuf*)((u8*)(p) - LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))) 

#define INC_RX_BD_INDEX(idx) { if (++idx >= ETHERNET_RX_BD_NUMBER) idx = 0;     }
#define INC_TX_BD_INDEX(idx) { if (++idx >= ETHERNET_TX_BD_NUMBER) idx = 0;     }
#define DEC_TX_BD_INDEX(idx) { if (idx-- == 0) idx = ETHERNET_TX_BD_NUMBER-1;   }


#define IFNAME0 'e'
#define IFNAME1 'n'







//#pragma data_alignment=8
#pragma pack(push)
#pragma pack(8)
struct STR_MCF5282_IF
{
    t_txrx_desc     rxbd_a[ETHERNET_RX_BD_NUMBER];      // Rx descriptor ring. Must be aligned to double-word
    t_txrx_desc     txbd_a[ETHERNET_TX_BD_NUMBER];      // Tx descriptor ring. Must be aligned to double-word
    struct pbuf     *rx_pbuf_a[ETHERNET_RX_BD_NUMBER];  // Array of pbufs corresponding to payloads in rx desc ring.
    struct pbuf     *tx_pbuf_a[ETHERNET_TX_BD_NUMBER];  // Array of pbufs corresponding to payloads in tx desc ring.
    unsigned int    rx_remove;                          // Index that driver will remove next rx frame from.
    unsigned int    rx_insert;                          // Index that driver will insert next empty rx buffer.
    unsigned int    tx_insert;                          // Index that driver will insert next tx frame to.
    unsigned int    tx_remove;                          // Index that driver will clean up next tx buffer.
    unsigned int    tx_free;                            // Number of free transmit descriptors.
    unsigned int    rx_buf_len;                         // number of bytes in a rx buffer (that we can use).
    struct eth_addr *ethaddr;
};
#pragma pack(pop)

typedef  struct STR_MCF5282_IF  t_mcf5282_if;

void        mcf5282_if_input(struct netif *netif);
err_t       mcf5282_if_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr);
void        low_level_input(struct netif *netif);
void        eth_input(struct pbuf *p, struct netif *netif);
sys_prot_t  sys_arch_protect(void);
void        sys_arch_unprotect(sys_prot_t pval);



//#pragma data_alignment=8
static  t_mcf5282_if            mcf5282_if;
static  sys_sem_t               tx_sem;
static  sys_sem_t               rx_sem;

struct netif                    mcf282_fec_netif;

//static const struct eth_addr    ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};


/*=============================================================================================================*/
/*! \brief 

    \return 
    \retval 
    \sa 
*/
/*=============================================================================================================*/
void fill_rx_ring(t_mcf5282_if *mcf5282)
{
    struct pbuf         *p;
    t_txrx_desc         *p_rxbd;
    int                 i               = mcf5282->rx_insert;
    void                *new_payload;
    u32_t               u_p_pay;
    
    /* Try and fill as many receive buffers as we can */
    while (mcf5282->rx_pbuf_a[i] == 0)
    {
        p = pbuf_alloc(PBUF_RAW, (u16_t) mcf5282->rx_buf_len, PBUF_POOL);

        if (p == 0) {           
            return;    /* No pbufs, so can't refill ring */
        }

        /* Align payload start to be divisible by 16 as required by HW */
        u_p_pay = (u32_t) p->payload;
        new_payload = p->payload = (void *) (((u_p_pay + 15) / 16) * 16);
        
        mcf5282->rx_pbuf_a[i] = p;
        p_rxbd = &mcf5282->rxbd_a[i];
        p_rxbd->starting_adress = (u8_t *) new_payload;
        p_rxbd->contr_status_flags = (p_rxbd->contr_status_flags & MCF_FEC_RxBD_W) | MCF_FEC_RxBD_E;
        INC_RX_BD_INDEX(mcf5282->rx_insert);
        i = mcf5282->rx_insert;
    }
}


/*=============================================================================================================*/
/*! \brief 

    \return 
    \retval 
    \sa 
*/
/*=============================================================================================================*/
void fec_disable(t_mcf5282_if *mcf5282)
{
        int i;

        /* Reset the FEC - equivalent to a hard reset */
        MCF_FEC_ECR = MCF_FEC_ECR_RESET;

        /* Wait for the reset sequence to complete */
        while (MCF_FEC_ECR & MCF_FEC_ECR_RESET);

        /* Set the Graceful Transmit Stop bit */
        //MCF5282_FEC_TCR = (MCF5282_FEC_TCR | MCF5282_FEC_TCR_GTS);

        /* Wait for the current transmission to complete */
        //while ( !(MCF5282_FEC_EIR & MCF5282_FEC_EIR_GRA));

        /* Clear the GRA event */
        //MCF5282_FEC_EIR = MCF5282_FEC_EIR_GRA;

        /* Disable the FEC */
        MCF_FEC_ECR = 0;

        /* Disable all FEC interrupts by clearing the IMR register */
        MCF_FEC_EIMR = 0;

        /* Clear the GTS bit so frames can be tranmitted when restarted */
        MCF_FEC_TCR = (MCF_FEC_TCR & ~MCF_FEC_TCR_GTS);

        /* Release all buffers attached to the descriptors.  Since the driver
     * ALWAYS zeros the pbuf array locations and descriptors when buffers are
     * removed, we know we just have to free any non-zero descriptors */
        for (i = 0; i < ETHERNET_RX_BD_NUMBER; i++) {
            if (mcf5282->rx_pbuf_a[i])   {
                pbuf_free(mcf5282->rx_pbuf_a[i]);
                mcf5282->rx_pbuf_a[i] = 0;
                mcf5282->rxbd_a->starting_adress = 0;
            }
        }
    
        for (i = 0; i < ETHERNET_TX_BD_NUMBER; i++) {
            if (mcf5282->tx_pbuf_a[i])   {
                pbuf_free(mcf5282->tx_pbuf_a[i]);
                mcf5282->tx_pbuf_a[i] = 0;
                mcf5282->txbd_a->starting_adress = 0;
            }
        }
}



/*=============================================================================================================*/
/*! \brief 

    \return 
    \retval 
    \sa 
*/
/*=============================================================================================================*/
void fec_enable(t_mcf5282_if *mcf5282)
{
    int i;

    /* Initialize empty tx descriptor ring */
    for(i = 0; i < ETHERNET_TX_BD_NUMBER-1; i++)    {
        mcf5282->txbd_a[i].contr_status_flags = 0;
    }

    /* Set wrap bit for last descriptor */
    mcf5282->txbd_a[i].contr_status_flags = MCF_FEC_TxBD_W;
        

    /* initialize tx indexes */
    mcf5282->tx_remove = mcf5282->tx_insert = 0;
    mcf5282->tx_free = ETHERNET_TX_BD_NUMBER;

        /* Initialize empty rx descriptor ring */
    for (i = 0; i < ETHERNET_RX_BD_NUMBER-1; i++) {
        mcf5282->rxbd_a[i].contr_status_flags = 0;
    }

    /* Set wrap bit for last descriptor */
    mcf5282->rxbd_a[i].contr_status_flags = MCF_FEC_RxBD_W;

    /* Initialize rx indexes */
    mcf5282->rx_remove = mcf5282->rx_insert = 0;

    /* Fill receive descriptor ring */
    fill_rx_ring(mcf5282);

//        sim->gpio.pehlpar = 0xC0;             /* в файле pins */
//        sim->gpio.paspar = 0x0F00;

        /* Enable FEC */
    MCF_FEC_ECR = MCF_FEC_ECR_ETHER_EN;

    /* Allow interrupts by setting IMR register */
    MCF_FEC_EIMR = MCF_FEC_EIMR_RXF | MCF_FEC_EIMR_TXF;

    /* Indicate that there have been empty receive buffers produced */
    MCF_FEC_RDAR = 1;
}



/*=============================================================================================================*/
/*! \brief 

    \return 
    \retval 
    \sa 
*/
/*=============================================================================================================*/
void mcf5282fec_tx_cleanup(void)
{
    struct pbuf     *p;
    unsigned int    tx_remove_sof;
    unsigned int    tx_remove_eof;
    unsigned int    i;
    u16_t           flags;
    t_mcf5282_if    *mcf5282 = mcf282_fec_netif.state;              //////!!!!!! Пререправить на актульную netif

#if SYS_LIGHTWEIGHT_PROT == 1
    sys_prot_t       old_level;
#endif

    tx_remove_sof = tx_remove_eof = mcf5282->tx_remove;

#if defined(MCF5282_DEBUG)
        printf("mcf5282fec_tx_cleanup: tx_remove_eof: %d\n", tx_remove_eof);
#endif

    /* We must protect reading the flags and then reading the buffer pointer. They must
       both be read together. */

#if SYS_LIGHTWEIGHT_PROT == 1
    old_level = sys_arch_protect();
#endif

    /* Loop, looking for completed buffers at eof */
    while ((((flags = mcf5282->txbd_a[tx_remove_eof].contr_status_flags) & MCF_FEC_TxBD_R) == 0) &&
           (mcf5282->tx_pbuf_a[tx_remove_eof] != 0))    {
        /* See if this is last buffer in frame */
        if ((flags & MCF_FEC_TxBD_L) != 0)  {
            i = tx_remove_eof;
            /* This frame is complete. Take the frame off backwards */
            do {
                p = mcf5282->tx_pbuf_a[i];
                mcf5282->tx_pbuf_a[i] = 0;
                mcf5282->txbd_a[i].starting_adress = 0;
                mcf5282->tx_free++;
                if (i != tx_remove_sof) {
                    DEC_TX_BD_INDEX(i);
                } else {
                    break;
                }
            } while (TRUE);

#if SYS_LIGHTWEIGHT_PROT == 1
            sys_arch_unprotect(old_level);
#endif
#if defined(MCF5282_DEBUG)
            printf("mcf5282fec_tx_cleanup: pbuf_free -> 0x%08X\n", p);
#endif
            pbuf_free(p);       // Will be head of chain

#if SYS_LIGHTWEIGHT_PROT == 1
            old_level = sys_arch_protect();
#endif
            /* Look at next descriptor */
            INC_TX_BD_INDEX(tx_remove_eof);
            tx_remove_sof = tx_remove_eof;
        }
        else
            INC_TX_BD_INDEX(tx_remove_eof);
    }
    mcf5282->tx_remove = tx_remove_sof;

        //MCF5282_FEC_EIMR |= MCF5282_FEC_EIMR_TXF;             // renable interrupt

#if SYS_LIGHTWEIGHT_PROT == 1
    sys_arch_unprotect(old_level);
#endif
}



void low_level_init(struct netif *netif)
{
    t_mcf5282_if       *mcf5282;
    struct pbuf         *p;
    int                 i;

#ifdef MCF5282_DEBUG
    printf("low_level_init\n");
#endif

    mcf5282 = netif->state;

    fec_disable(mcf5282);

    /* set MAC hardware address length */
    netif->hwaddr_len = 6;

    /* set MAC hardware address */
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0xCF;
    netif->hwaddr[2] = 0x52;
    netif->hwaddr[3] = 0x82;
    netif->hwaddr[4] = 0x00;
    netif->hwaddr[5] = 0x01;

    /* maximum transfer unit */
//    netif->mtu = MTU_FEC - 18; (512-18)
//    netif->mtu = FEC_MTU;  (1518)
    netif->mtu = MAX_ETH_BUFF_SIZE - 18;  //(1518)

    /* broadcast capability */
    netif->flags = NETIF_FLAG_BROADCAST;

    
    // install int handlers
//    SetInterruptVector(MCF52XX_FEC_X_INTF_VECTOR + MCF52XX_INTC0_VECTOR_BASE, &mcf5282_fec_int_txf);
//    SetInterruptVector(MCF52XX_FEC_R_INTF_VECTOR + MCF52XX_INTC0_VECTOR_BASE, &mcf5282_fec_int_rxf);
//
//    sim->intc[0].icrn[MCF52XX_FEC_X_INTF_VECTOR] = MCF52XX_FEC_X_INTF_IACKLPR;              // set int level and priority
//    sim->intc[0].icrn[MCF52XX_FEC_R_INTF_VECTOR] = MCF52XX_FEC_R_INTF_IACKLPR;              // set int level and priority
//
//    EnableInterrupt(MCF52XX_FEC_X_INTF_VECTOR);                             
//                                              // enable ints
//    EnableInterrupt(MCF52XX_FEC_R_INTF_VECTOR);                             
//                                                // enable ints
        
// инициализация прерывания о конце передаче фрейма
    MCU_ConfigureIntr(INTR_ID_FEC_X_INTF, 6, 1 );
    MCU_EnableIntr(INTR_ID_FEC_X_INTF,1);
        
// инициализация прерывания о приёме фрейма
    MCU_ConfigureIntr(INTR_ID_FEC_R_INTF, 6, 3 );
    MCU_EnableIntr(INTR_ID_FEC_R_INTF,1);
    
    
     /* Set the source address for the controller */
    MCF_FEC_PALR = 0 
                        | (netif->hwaddr[0] <<24) 
                        | (netif->hwaddr[1] <<16)       
                        | (netif->hwaddr[2] <<8) 
                        | (netif->hwaddr[3] <<0);
    MCF_FEC_PAUR = 0
                        | (netif->hwaddr[4] <<24)
                        | (netif->hwaddr[5] <<16);

    /* Initialize the hash table registers */
    MCF_FEC_IAUR = 0;
    MCF_FEC_IALR = 0;

   /* Set Receive Buffer Size. We subtract 16 because the start of the receive
    *  buffer MUST be divisible by 16, so depending on where the payload really
    *  starts in the pbuf, we might be increasing the start point by up to 15 bytes.
    *  See the alignment code in fill_rx_ring() */
    /* There might be an offset to the payload address and we should subtract
     * that offset */
    p = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
    i = 0;
    if (p)
    {
        struct pbuf *q = p;
        
        while ((q = q->next) != 0) {
            i += q->len;
        }

        mcf5282->rx_buf_len = PBUF_POOL_BUFSIZE-16-i;

        pbuf_free(p);
    }

    MCF_FEC_EMRBR = (u16_t) mcf5282->rx_buf_len;

    /* Point to the start of the circular Rx buffer descriptor queue */
    MCF_FEC_ERDSR = ((u32_t) &mcf5282->rxbd_a[0]);

    /* Point to the start of the circular Tx buffer descriptor queue */
    MCF_FEC_ETDSR = ((u32_t) &mcf5282->txbd_a[0]);
    
    /* Set the tranceiver interface to MII mode */
    MCF_FEC_RCR =   (netif->mtu << 16)
                  | MCF_FEC_RCR_MII_MODE
                  | MCF_FEC_RCR_DRT;           // half duplex
                                           //| MCF5282_FEC_RCR_PROM;            
// accept all frames

    /* Only operate in half-duplex, no heart beat control */
    MCF_FEC_TCR = 0;

    /* Set MII bus speed */
    MCF_FEC_MSCR = 0x0D;

        /* Clear MII interrupt status */
    //MCF5282_FEC_EIR = MCF5282_FEC_EIMR_MII;
    // enable fec
    fec_enable(mcf5282);
}


/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
err_t low_level_output(struct netif *netif, struct pbuf *p)
{
        t_mcf5282_if *mcf5282 = netif->state;
        struct pbuf *q, *r;
        char *ptr;
        char *pStart;
        unsigned int len;
        
#if SYS_LIGHTWEIGHT_PROT == 1
    u32_t old_level;
#endif

#if SYS_LIGHTWEIGHT_PROT == 1
    /* Interrupts are disabled through this whole thing to support 
multi-threading
     * transmit calls. Also this function might be called from an ISR. */
        old_level = sys_arch_protect();
#endif

        // make sure a descriptor free
        if (mcf5282->tx_free)
        {
                r = pbuf_alloc(PBUF_RAW, p->tot_len + 16, PBUF_RAM);            
                // alloc mem for buffer

                if (r != NULL) 
                {
                  ptr = pStart = (void *) ((((u32)r->payload + 15) / 16) * 16);         // get start address aligned on 16 byte boundary

                        for(q = p, len = 0; q != NULL; q = q->next)
                        {
                                memcpy(ptr + len, q->payload, q->len);
                                len += q->len;
                        }

#if defined(MCF5282_DEBUG)
                        printf("low_level_output: mcf5282->tx_insert: %d\n", mcf5282->tx_insert);
#endif

                        // put buffer on descriptor ring
                        mcf5282->tx_free--;                                     
                                                // dec free descriptors
            mcf5282->tx_pbuf_a[mcf5282->tx_insert] = r;                         
        // save pointer to pbuf
            mcf5282->txbd_a[mcf5282->tx_insert].starting_adress = (volatile u8*)pStart;                 
// set start address of data
            mcf5282->txbd_a[mcf5282->tx_insert].data_length = len;                 
// set len of data

#if defined(MCF5282_DEBUG)
                        printf("low_level_output: pbuf: 0x%08X, buf: 0x%08X, data_len: %d\n", mcf5282->tx_pbuf_a[mcf5282->tx_insert], 
                                mcf5282->txbd_a[mcf5282->tx_insert].starting_adress, 
                                mcf5282->txbd_a[mcf5282->tx_insert].data_len);
                        printf("p_buf:\n");
                        printf("\tdest mac: ");
                        for(len = 0; len < 6; len++)
                                printf("%02X%c", ((BYTE*)mcf5282->txbd_a[mcf5282->tx_insert].p_buf)[len], len == 5 ? '\n' : ':');
                        printf("\tsrc mac: ");
                        for(; len < 12; len++)
                          printf("%02X%c", ((BYTE*)mcf5282->txbd_a[mcf5282->tx_insert].p_buf)[len], len == 11 ? '\n' : ':');

                        for(; len < mcf5282->txbd_a[mcf5282->tx_insert].data_len; len++)
                                printf("%02X ", ((BYTE*)mcf5282->txbd_a[mcf5282->tx_insert].p_buf)[len]);
                        printf("\n");
#endif

                        // set flags
                        mcf5282->txbd_a[mcf5282->tx_insert].contr_status_flags = (u16_t)(MCF_FEC_TxBD_R 
                                                                                         | (mcf5282->txbd_a[mcf5282->tx_insert].contr_status_flags & MCF_FEC_TxBD_W)
                                                                                         | (MCF_FEC_TxBD_L | MCF_FEC_TxBD_TC));

                        INC_TX_BD_INDEX(mcf5282->tx_insert);

#ifdef LINK_STATS
                        lwip_stats.link.xmit++;
#endif
                        /* Indicate that there has been a transmit buffer produced */
                        MCF_FEC_TDAR = 1;
        
#if SYS_LIGHTWEIGHT_PROT == 1
                        sys_arch_unprotect(old_level);
#endif
                        return ERR_OK;    
                } /*if (r != NULL) */
        } /* if (mcf5282->tx_free) */

        /* Drop the frame, we have no place to put it */
#ifdef LINK_STATS
        lwip_stats.link.memerr++;
#endif
#if SYS_LIGHTWEIGHT_PROT == 1
        sys_arch_unprotect(old_level);
#endif

        return (u8_t)ERR_MEM;
}


/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */

void low_level_input(struct netif *netif)
{
    u16_t               flags;
    unsigned int        rx_remove_sof;
    unsigned int        rx_remove_eof;
    struct pbuf         *p;
    t_mcf5282_if        *mcf5282 = netif->state;
    
    rx_remove_sof = rx_remove_eof = mcf5282->rx_remove;

        /* Loop, looking for filled buffers at eof */
    while ((((flags = mcf5282->rxbd_a[rx_remove_eof].contr_status_flags) & MCF_FEC_RxBD_E) == 0) && (mcf5282->rx_pbuf_a[rx_remove_eof] != 0))  {
      /* See if this is last buffer in frame */
        if ((flags & MCF_FEC_RxBD_L) != 0)  {
            /* This frame is ready to go. Start at first descriptor in frame. */
            p = 0;
            do  {
                /* Adjust pbuf length if this is last buffer in frame */
                if (rx_remove_sof == rx_remove_eof)  {
                    mcf5282->rx_pbuf_a[rx_remove_sof]->tot_len = mcf5282->rx_pbuf_a[rx_remove_sof]->len = (u16_t)(mcf5282->rxbd_a[rx_remove_sof].data_length - (p ? p->tot_len : 0));
                }
                else    {
                    mcf5282->rx_pbuf_a[rx_remove_sof]->len =  mcf5282->rx_pbuf_a[rx_remove_sof]->tot_len = mcf5282->rxbd_a[rx_remove_sof].data_length;
                }
                
                /* Chain pbuf */
                if (p == 0)
                {
                    p = mcf5282->rx_pbuf_a[rx_remove_sof];       // First in chain
                    p->tot_len = p->len;                        // Important since len might have changed
                } else {
                    pbuf_chain(p, mcf5282->rx_pbuf_a[rx_remove_sof]);
                    pbuf_free(mcf5282->rx_pbuf_a[rx_remove_sof]);
                }
                
                /* Clear pointer to mark descriptor as free */
                mcf5282->rx_pbuf_a[rx_remove_sof] = 0;
                mcf5282->rxbd_a[rx_remove_sof].starting_adress = 0;
                
                if (rx_remove_sof != rx_remove_eof) {
                    INC_RX_BD_INDEX(rx_remove_sof);
                }
                else    {
                    break;
                }
               
            } while (TRUE);

            INC_RX_BD_INDEX(rx_remove_sof);

            /* Check error status of frame */
            if (flags & (MCF_FEC_RxBD_LG |
                         MCF_FEC_RxBD_NO |
                         MCF_FEC_RxBD_CR |
                         MCF_FEC_RxBD_OV))
            {
#ifdef LINK_STATS
                lwip_stats.link.drop++;
                if (flags & MCF_FEC_RxBD_LG)    {
                    lwip_stats.link.lenerr++;                //Jumbo gram
                } else {
                  if (flags & (MCF_FEC_RxBD_NO | MCF_FEC_RxBD_OV)) {
                        lwip_stats.link.err++;
                  } else {
                    if (flags & MCF_FEC_RxBD_CR) {
                            lwip_stats.link.chkerr++;        // CRC errors
                    }
                  }
                }
#endif
                /* Drop errored frame */
                pbuf_free(p);
            } else 
                        {
                /* Good frame. increment stat */
#ifdef LINK_STATS
                lwip_stats.link.recv++;
#endif
                                eth_input(p, netif);
            }
        }
        INC_RX_BD_INDEX(rx_remove_eof);
    }
    mcf5282->rx_remove = rx_remove_sof;

        /* Fill up empty descriptor rings */
    fill_rx_ring(mcf5282);

        /* Set rx interrupt bit again */
    MCF_FEC_EIMR |= MCF_FEC_EIMR_RXF;

    /* Tell fec that we have filled up her ring */
    MCF_FEC_RDAR = 1;
}

/*
 * mcf5282_if_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actual transmission of the packet.
 *
 */

err_t mcf5282_if_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr)
{
        /* resolve hardware address, then send (or queue) packet */
        return etharp_output(netif, p, ipaddr);
}



void eth_input(struct pbuf *p, struct netif *netif)
{
    /* Ethernet protocol layer */
//    struct eth_hdr  *ethhdr;
//    t_mcf5282_if    *mcf5282 = netif->state;
//
//    ethhdr = p->payload;
    
    ethernet_input(p, netif);
//    switch ( htons(ethhdr->type) ) 
//        {
//      case ETHTYPE_IP:
//        etharp_ip_input(netif, p);
//        pbuf_header(p, -(u16_t)sizeof(struct eth_hdr));
//        netif->input(p, netif);
//        break;
//      case ETHTYPE_ARP:
//        etharp_arp_input(netif, mcf5282->ethaddr, p);
//        break;
//      default:
//        pbuf_free(p);
//                p = NULL;
//        break;
//    }
}



/*
 * mcf5282_if_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */

err_t mcf5282_if_init(struct netif *netif)
{
        t_mcf5282_if *mcf5282if = &mcf5282_if;

        printf("mcf5282_if_init: %08X\n", (u32_t)&mcf5282_if);
          
        if (mcf5282if == NULL)  {
                LWIP_DEBUGF(NETIF_DEBUG, ("mcf5282_if_init: out of memory\n"));
                return (u8_t)ERR_MEM;
        }

        netif->state = mcf5282if;
        netif->name[0] = IFNAME0;
        netif->name[1] = IFNAME1;
        netif->output = mcf5282_if_output;
        netif->linkoutput = low_level_output;
//        netif->mtu = FEC_MTU - 18;      // mtu without ethernet header and crc
        netif->mtu = MAX_ETH_BUFF_SIZE - 18;

        netif->hwaddr_len = 6; /* Ethernet interface */

        mcf5282if->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
          
        low_level_init(netif);

        etharp_init();

        return (u8_t)ERR_OK;
}


//void arp_timer(void *arg)
//{
//        etharp_tmr();
//        sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
//}
//
//void dhcp_fine_timer(void *arg)
//{
//        dhcp_fine_tmr();
//        sys_timeout(DHCP_FINE_TIMER_MSECS, dhcp_fine_timer, NULL);
//}
//
//void dhcp_coarse_timer(void *arg)
//{
//        dhcp_coarse_tmr();
//        sys_timeout(DHCP_COARSE_TIMER_SECS * 1000, dhcp_coarse_timer, NULL);
//}
//
void tcpip_init_done(void *arg)
{
        sys_sem_t *sem;

#ifdef DEBUG_PRINTF
        printf("tcpip thread: %d\n", TS_GetCurrentTask());
#endif

        sem = arg;
        sys_sem_signal(sem);
}

//void mcf5282_ethernet_timers_thread(void *arg)
//{
//        sys_sem_t tmr_sem;
//
//        tmr_sem = sys_sem_new(0);               // create sem
//
//        // start timers
//        sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
//        sys_timeout(DHCP_FINE_TIMER_MSECS, dhcp_fine_timer, NULL);
//        sys_timeout(DHCP_COARSE_TIMER_SECS * 1000, dhcp_coarse_timer, NULL);
//
//        while(1)
//        {
//                sys_sem_wait(tmr_sem);          // wait forever
//        }
//}

/** \brief rx thread
 *      \author Errin Bechtel 
 *      \date 09-21-2004
 *
 *      <b> Modified:</b><br>
 *
 * \param *arg 
 */
void mcf5282_ethernet_rx_thread(void *arg)
{
    while (1)
    {
        sys_sem_wait(&rx_sem);

        low_level_input(&mcf282_fec_netif);
    }
}

/** \brief transmit thread
 *      \author Errin Bechtel 
 *      \date 09-21-2004
 *
 *      <b> Modified:</b><br>
 *
 * \param *arg 
 */
void mcf5282_ethernet_thread(void *arg)
{
    while (1)
    {
        sys_sem_wait(&tx_sem);

        mcf5282fec_tx_cleanup();
    }
}

void mcf5282_ethernet_init(void)
{
        sys_sem_t sem;
        
        struct ip_addr ipaddr, netmask, gw;


#ifdef STATS
        stats_init();
#endif

        sys_init();

        mem_init();
        memp_init();
        pbuf_init();

        netif_init();

        sys_sem_new(&rx_sem, 0);                // create receive semaphore
        sys_sem_new(&tx_sem, 0);                // create transmit semaphore

        // start new threads for tx, rx and timers
        sys_thread_new("rx_eth", mcf5282_ethernet_rx_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
        sys_thread_new("tx_eth", mcf5282_ethernet_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
//        sys_thread_new(mcf5282_ethernet_timers_thread, NULL, DEFAULT_THREAD_PRIO);

        sys_sem_new(&sem, 0);
        tcpip_init(tcpip_init_done, &sem);
        sys_sem_wait(&sem);
        sys_sem_free(&sem);

        printf("TCP/IP initialized.\n");

        IP4_ADDR(&gw, 0,0,0,0);
        IP4_ADDR(&ipaddr, 0,0,0,0);
        IP4_ADDR(&netmask, 0,0,0,0);

        netif_add(&mcf282_fec_netif, &ipaddr, &netmask, &gw, NULL, mcf5282_if_init, tcpip_input);
        netif_set_default(&mcf282_fec_netif);
        //netif_set_up(&mcf282_fec_netif);


        dhcp_start(&mcf282_fec_netif);          // start dhcp

        //httpd_init();                                         // start web server
}


void ethernet_stats_display(void)
{
        t_mcf5282_if *mcf5282 = mcf282_fec_netif.state;

        stats_display();

        // print IP address
        printf("IP addr: %08X\n", mcf282_fec_netif.ip_addr.addr);
        printf("netif_is_up: %d\n", netif_is_up(&mcf282_fec_netif));
        printf("tx_free: %d\n", mcf5282->tx_free);
}

















/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/
void isr_FEC_TxFrame_Handler()
{
    /* clear interrupt status for tx interrupt */
    MCF_FEC_EIR = MCF_FEC_EIR_TXF;
    sys_sem_signal(&tx_sem);
}


/*=============================================================================================================*/
/*! \brief      */
/*=============================================================================================================*/
void isr_FEC_ReceiveFrame_Handler()
{
    MCF_FEC_EIMR &= ~MCF_FEC_EIMR_RXF;  // disable rx int
    MCF_FEC_EIR = MCF_FEC_EIR_RXF;          // clear int flag
    
    sys_sem_signal(&rx_sem);        
}
//    t8_m5282_fec_reset_rx_isr();        



void isr_FEC_TxBuffer_Handler() 
{return;}

void isr_FEC_ReceiveBuffer_Handler()
{return;}

void isr_FEC_MII_Handler()
{return;}


void isr_FEC_FIFOUnderrun_Handler()
{return;}

void isr_FEC_CollisionRetryLimit_Handler()
{return;}

void isr_FEC_LateCollision_Handler()
{return;}

void isr_FEC_HeartbeatError_Handler()
{return;}

void isr_FEC_GracefulStopComplete_Handler()
{return;}

void isr_FEC_EthernetBusError_Handler()
{return;}

void isr_FEC_BabblingTransmitError_Handler()
{return;}

void isr_FEC_BabblingReceiveError_Handler()
{return;}

























#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static t_txrx_desc rx_bd[ ETHERNET_RX_BD_NUMBER ] ;  /*! дескрипторы на прием пакетов для DMA */

#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static t_txrx_desc tx_bd[ ETHERNET_TX_BD_NUMBER ] ;  /*! дескрипторы на передачу пакетов для DMA */

#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static struct pbuf *rx_pbuf_array[ETHERNET_RX_BD_NUMBER];


#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static u8     rx_arrays [ RX_PACKET_POOL_LEN ][ MAX_ETH_BUFF_SIZE ]; /*! массивы для приема пакетов для DMA */
_Pragma("location=\"sdram\"")
__no_init static _BOOL  rx_busy   [ RX_PACKET_POOL_LEN ];


#pragma data_alignment=16
//_Pragma("location=\"packets_sram\"")
_Pragma("location=\"sdram\"")
__no_init static u8     tx_arrays [ TX_PACKET_POOL_LEN ][ MAX_ETH_BUFF_SIZE ]; /*! массивы для передачи пакетов для DMA */
_Pragma("location=\"sdram\"")
__no_init static _BOOL  tx_busy   [ TX_PACKET_POOL_LEN ];


static  t_eth                   default_eth;
static  u8                      last_rx_buf_descr = 0;

#if LWIP_TCPIP_CORE_LOCKING_INPUT
    static OS_EVENT             *eth0_rcv_queue = NULL;                     /*!   */    
    static void                 *rx_messages_array[RX_QUEUE_LEN] ;          /*! указатели для очереди сообщений  */
    
    static OS_EVENT             *eth0_tcm_queue = NULL;                     /*!   */    
    static void                 *tx_messages_array[TX_QUEUE_LEN] ;          /*! указатели для очереди сообщений  */
#endif




/*=============================================================================================================*/

_BOOL  dnepr_ethernet_open 
(
    t_eth    *out_descr
)
{
    u8      i;
    
    out_descr = out_descr;
    
    default_eth.data_descr.rx_pool.pocket_len      = MAX_ETH_BUFF_SIZE;
    default_eth.data_descr.rx_pool.pockets_number  = RX_PACKET_POOL_LEN;
    default_eth.data_descr.rx_pool.pockets_busy    = rx_busy;
    default_eth.data_descr.rx_pool.pockets_array   = (u8**)rx_arrays;
    
    default_eth.data_descr.tx_pool.pocket_len      = MAX_ETH_BUFF_SIZE;
    default_eth.data_descr.tx_pool.pockets_number  = TX_PACKET_POOL_LEN;
    default_eth.data_descr.tx_pool.pockets_busy    = tx_busy;
    default_eth.data_descr.tx_pool.pockets_array   = (u8**)tx_arrays;

    
    
    for ( i = 0; i < RX_PACKET_POOL_LEN; ++i )  {
        rx_bd[i].contr_status_flags = 0 ;
        rx_bd[i].data_length = 0 ;      
        rx_bd[i].starting_adress = &rx_arrays[i][0];    
    }
    
    for ( i = 0; i < TX_PACKET_POOL_LEN; ++i )  {
      
        tx_bd[i].contr_status_flags = 0 ;
        tx_bd[i].data_length = 0 ;      
        tx_bd[i].starting_adress = &tx_arrays[i][0];
    }
    
#if LWIP_TCPIP_CORE_LOCKING_INPUT    
    eth0_rcv_queue = OSQCreate( rx_messages_array, RX_QUEUE_LEN ) ;
#endif    
    
      
    out_descr = &default_eth;    
    return TRUE;      
}


/*!
\brief Инициализирует два MV88E6095 как тупой свитч, без RSTP, все порты включены
\param maddr массив 6ти байт мак-адреса, которым свитч должен слать пакеты flow control
\retval OK/ERROR
*/
u32 Dnepr_Ethernet_Init( const u8* maddr )
{
	u16	usBuffer = 0 ;
	s32 i ;

	MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED((u32)( SYSTEM_CLOCK_KHZ/1000/5 )) ;

// Switch Management Register

	// interswitch порты

	// PCS Control Register -- ForcedLink (88E6095 datasheet page 61 Note) и LinkValue 
	MV88E6095_multichip_smi_read( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
	MV88E6095_multichip_smi_write( MV88E6095_1_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );
	MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, &usBuffer  );
	usBuffer |= FORCE_LINK | LINK_FORCED_VALUE(1) ;
	MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, MV88E6095_PORT8, MV88E6095_PCS_CTRL_REG, usBuffer  );

	// Инициализируем MAC
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
/*!  \brief Управляем режимом автоопределения типа сети auto-negotiation для SFP
     \details Отключаем или включаем режим в зависимости от флага mode_flag для конкретного порта switch'а 
     \details MV88E6095 PORT9 для SFP1, PORT10 для SFP2
     \details По умолчанию после сброса данный режим включен

     \sa MV88E6095_multichip_smi_read, cmsfpautoneg_update 
*/
/*=============================================================================================================*/
void dnepr_ethernet_sfpport_autoneg_mode
( 
    u8      sfp_num,                        /*!< [in] номер sfp 1,2                                   */
    u8      mode_flag                       /*!< [in] включить или выключить режим  0 - выкл, 1 - вкл */
)
{  
    u8      port;
    u16     temp_reg;
    
    switch (sfp_num)
    {
    case 1:     port = MV88E6095_PORT10;     break;  /* sfp1 - нижняя  L port10  */
    case 2:     port = MV88E6095_PORT9;      break;  /* sfp2 - верхняя U port9   */
    default:    return;
    }
  
    MV88E6095_multichip_smi_read( MV88E6095_2_CHIPADDR, port, MV88E6095_PCS_CTRL_REG, &temp_reg  );  
    if ( mode_flag == 0 )    {
        /* отключаем режим auto-negotiation */
        temp_reg &= ~(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_MASK | MV88E6095_PCS_CTRL_REG_AUTONEG_ENABLE);        
        temp_reg |= MV88E6095_PCS_CTRL_REG_FORCE_SPEED(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_1000B);
    } else {
        /* включаем режим auto-negotiation */
        temp_reg &= ~(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_MASK);        
        temp_reg |= MV88E6095_PCS_CTRL_REG_FORCE_SPEED(MV88E6095_PCS_CTRL_REG_FORCE_SPEED_VALUE_AUTO) | MV88E6095_PCS_CTRL_REG_AUTONEG_ENABLE ;
    }
    MV88E6095_multichip_smi_write( MV88E6095_2_CHIPADDR, port, MV88E6095_PCS_CTRL_REG, temp_reg  );
}



/*=============================================================================================================*/
/*!  \brief Инициализация FEC для стека
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
    
    mii_config.rxbd_ring = rx_bd;
    mii_config.rxbd_ring_len = ETHERNET_RX_BD_NUMBER;

    mii_config.txbd_ring = tx_bd;
    mii_config.txbd_ring_len = ETHERNET_TX_BD_NUMBER;
          
    t8_m5282_fec_init(&mii_config);
  
// инициализация прерывания о конце передаче фрейма
//    MCU_ConfigureIntr(INTR_ID_FEC_X_INTF, 6, 1 );
//    MCU_EnableIntr(INTR_ID_FEC_X_INTF,1);
    
// инициализация прерывания о конце передачи одного буфера
//    MCU_ConfigureIntr(INTR_ID_FEC_X_INTB, 6, 2 );
//    MCU_EnableIntr(INTR_ID_FEC_X_INTB,1);
    
// инициализация прерывания о приёме фрейма
    MCU_ConfigureIntr(INTR_ID_FEC_R_INTF, 6, 3 );
    MCU_EnableIntr(INTR_ID_FEC_R_INTF,1);
  
    return 0;
}

  
/*=============================================================================================================*/
/*!  \brief Инициализация PHY для стека
*
*   \sa netif_add
*/
/*=============================================================================================================*/
int dnepr_ethernet_phy_init(void)
{
//  u32   i;

//  /* Configure EMDC clock, frequency = 2.50 MHz */
//  FEC_MSCR = MSCR_MII_SPEED;
//  PHY_EPHYCTL0 &= ~(EPHYCTL0_DIS10 | EPHYCTL0_DIS100);
//  PHY_EPHYCTL0 |= EPHYCTL0_LEDEN;
//  /* Enable EPHY */
//  PHY_EPHYCTL0 |= EPHYCTL0_EPHYEN;
//  /* Delay for startup time */
//  for (i = PHY_DELAY; i != 0; i--) 
//  {    
//    /*
//    * Put something here to make sure
//    * the compiler doesn't optimize the loop away
//    */
//  }
//  /* Link speed = 100 Mbps, full-duplex */
//  FEC_MMFR = MMFR_ST | MMFR_OP_WR | MMFR_TA | MMFR_RA_CONTROL |
//                      EPHY_DATARATE | EPHY_DPLX;
//  
//  /* Poll for MII Write Frame completion */
//  while ((FEC_EIR & EIR_MII) == 0) { }  
//  /* Request status register read (link status polling) */
//  FEC_MMFR = MMFR_ST | MMFR_OP_RD | MMFR_TA | MMFR_RA_STATUS;   
  
  return 0;
}

/*=============================================================================================================*/
/*!  \brief Инициализация switch, VLAN
*
*   \sa 
*/
/*=============================================================================================================*/
//int dnepr_ethernet_switch_init(void)


/*=============================================================================================================*/
/*!  \brief Переводим MAC-адрес из строкового в числовое выражение
*/
/*=============================================================================================================*/
void dnepr_ethernet_str_2_mac
( 
    u8*         out,            /*!< [out] получаемое числовое значение (массив из 6 элементов)   */
    const char* str             /*!< [in]  входящая строка с IP-адресом                           */
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
/*!  \brief Переводим MAC-адрес из числового в строковое выражение
*/
/*=============================================================================================================*/
void dnepr_ethernet_mac_2_str
( 
    char        *str,           /*!< [out]  выходящая строка с IP-адресом                           */
    const u8    *mac            /*!< [in]   числовое значение MAC-адреса (массив из 6 элементов)    */    
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


#if LWIP_TCPIP_CORE_LOCKING_INPUT
_BOOL   dnepr_ethernet_rx_check
(
    t_eth_data_descr    *data_descr
)
{   
    _BOOL    ret_code;
   
    data_descr->rx_cur_message = (net_meassage_t*)OSQAccept( eth0_rcv_queue, 0, &ret_code );
    
    return ret_code;
}
#endif




//static size_t __tx_bd_isr_i = 0 ;
//void isr_FEC_TxBuffer_Handler()
//{
//	u8 i, err ;
//	Dnepr_Ethertnet_TX_Frame_t *q_mess ;
//
//	MCF_FEC_EIR |= MCF_FEC_EIR_TXB ;
//	__tx_cnt2-- ;
//
//
//	return ;
//	if( !__tx_queue ){
//		return ;
//	}
//	// Проверяем есть ли фреймы в очереди
//	// Сначала идёт сообщение с заголовком пакета
//	q_mess = (Dnepr_Ethertnet_TX_Frame_t*)OSQAccept( __tx_queue, &err );
//	assert( err == OS_ERR_NONE || err == OS_ERR_Q_EMPTY );
//	if( err == OS_ERR_Q_EMPTY ){
//		T8_Dnepr_TS_Ethernet_TX_Disactivate() ;
//		return ;
//	}
//
//	// Добавляем их в дескрипторы, если есть свободные. 
//	// Надо добавить весь пакет
//	i = __tx_bd_isr_i ;
//	do {
//		if( ++i >= FEC_TX_BD_NUMBER ){
//			i = 0 ;
//		}
//		// Если дескриптор свободен -- добавляем заголовок.
//		if( fec_tx_cstatus( i ) & MCF_FEC_TxBD_R == 0 ){
//			break ;
//		}
//
//	} while( i != __tx_bd_isr_i );
//	// Нет свободных дескрипторов, где-то ошибка.
//	assert( i != __tx_bd_isr_i );
//
//	++__tx_bd_isr_i ;
//	fec_add_tx_buffer( q_mess->data, i, q_mess->length );
//	if( ++i >= FEC_TX_BD_NUMBER ){
//		i = 0 ;
//	}
//	// Следующий дескриптор тоже должен быть пустым, потому что посылаем по-пакетно,
//	// а в каждом пакете два дескриптора.
//	assert( fec_tx_cstatus( i ) & MCF_FEC_TxBD_R == 0 );
//
//	// Теперь сообщение с телом пакета
//	q_mess = (Dnepr_Ethertnet_TX_Frame_t*)OSQAccept( __tx_queue, &err );
//	assert( err == OS_ERR_NONE );
//	fec_add_tx_buffer( q_mess->data, i, q_mess->length );
//}



//void isr_FEC_ReceiveFrame_Handler()
//{
//    u8    ret;
//        
//    t8_m5282_fec_reset_rx_isr();
//        
////        pocket_pools.pockets_array[last_rx_buf_descr][0]
//#if LWIP_TCPIP_CORE_LOCKING_INPUT
//        
//    OSQPost( __NetRcvQueue, (void*)&__change_mac_struct );                                              /* сообщение собственному потоку */
//    
//#else        
//    // if (rx_pbuf_array[last_rx_buf_descr] == NULL)                                           /* пихаем сообщения в tcp/ip поток lwip */
//    rx_pbuf_array[last_rx_buf_descr]->payload = &default_eth.data_descr.rx_pool.pockets_array[last_rx_buf_descr][0];
//    ret = (*default_eth.netif.input)(rx_pbuf_array[last_rx_buf_descr], &default_eth.netif);
//    if ( ret == ERR_OK )    
//#endif                                                                                                    /* начинаем прием в следующий буфер если его уже обработали (очередь увеличилась) */
//    {          
//        if ( ++last_rx_buf_descr == ETHERNET_RX_BD_NUMBER ) {
//            last_rx_buf_descr = 0;
//        }
//    }
//        
//    t8_m5282_fec_set_empty_status(&rx_bd[last_rx_buf_descr].contr_status_flags);        
//    t8_m5282_fec_start_rx();
//}



