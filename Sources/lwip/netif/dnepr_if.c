/*=============================================================================================================*/
/*!  \file  dnepr_if.c
*    \brief Порт ethernet для lwIP
*    \details   
*/
/*=============================================================================================================*/
#include <string.h>

#include "lwip/debug.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/debug.h"
#include "netif/etharp.h"
#include "netif/ppp/pppoe.h"
#include "lwip/include/arch/sys_arch.h"
#include "lwip/include/lwip/sys.h"

#include "support_common.h"
#include "common_lib/memory.h"

//#include "Threads/inc/threadNet.h"

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
//#include "HAL/MCU/inc/T8_5282_FEC.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'
#define MTU_FEC  (512)


#define INC_RX_BD_INDEX(idx) { if (++idx >= ETHERNET_RX_BD_NUMBER) idx = 0;     }
#define INC_TX_BD_INDEX(idx) { if (++idx >= ETHERNET_TX_BD_NUMBER) idx = 0;     }
#define DEC_TX_BD_INDEX(idx) { if (idx-- == 0) idx = ETHERNET_TX_BD_NUMBER-1;   }


/*=============================================================================================================*/
/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
#pragma pack(push)
#pragma pack(8)
struct ethernetif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
  t_txrx_desc     rxbd_a[ETHERNET_RX_BD_NUMBER];      // Rx descriptor ring. Must be aligned to double-word
  t_txrx_desc     txbd_a[ETHERNET_TX_BD_NUMBER];      // Tx descriptor ring. Must be aligned to double-word
  struct pbuf     *rx_pbuf_a[ETHERNET_RX_BD_NUMBER];  // Array of pbufs corresponding to payloads in rx desc ring.
  struct pbuf     *tx_pbuf_a[ETHERNET_TX_BD_NUMBER];  // Array of pbufs corresponding to payloads in tx desc ring.
  u32             rx_remove;                          // Index that driver will remove next rx frame from.
  u32             rx_insert;                          // Index that driver will insert next empty rx buffer.
  u32             tx_insert;                          // Index that driver will insert next tx frame to.
  u32             tx_remove;                          // Index that driver will clean up next tx buffer.
  u32             tx_free;                            // Number of free transmit descriptors.
  u32             rx_buf_len;                         // number of bytes in a rx buffer (that we can use).   
};
#pragma pack(pop)

/*=============================================================================================================*/

typedef  struct ethernetif  t_dnepr_if;

/*=============================================================================================================*/


static void     low_level_init(struct netif *netif);
static err_t    low_level_output(struct netif *netif, struct pbuf *p); 
err_t           dnepr_if_output(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr);

extern  sys_prot_t  sys_arch_protect(void);
extern  void        sys_arch_unprotect(sys_prot_t pval);

/*=============================================================================================================*/

t_dnepr_if      s_dnepr_if;

extern s8       val_CMPhyAddr[];
s8              val_CMMAC[19] = "00:01:02:03:04:05";


/*=============================================================================================================*/

/*-----------------------------------------Инициализация----------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief Функция обратного вызова инициализирующая интерфейс ethernet для lwIP
 *    \details  Should be called at the beginning of the program to set up the
 *              network interface. It calls the function low_level_init() to do the
 *              actual setup of the hardware.
 *
 *              This function should be passed as a parameter to netif_add().
 *  \param  netif the lwip network interface structure for this ethernetif
 *  \return ERR_OK if the loopif is initialized
 *          ERR_MEM if private data couldn't be allocated
 *          any other err_t on error
 *  \sa netif_add, low_level_init
 */
/*=============================================================================================================*/
err_t dnepr_if_init(struct netif *netif)
{

    LWIP_PLATFORM_ASSERT( netif != NULL );
        
    
    netif->state = &s_dnepr_if;                               /* указатель на пользовательскую структуру отображающую состояние */
    
#if LWIP_NETIF_STATUS_CALLBACK
//  netif->status_callback = ;
#endif /* LWIP_NETIF_STATUS_CALLBACK */
      
    netif->name[0] = IFNAME0;                                /* имя интерфейса */
    netif->name[1] = IFNAME1;    
    
#if LWIP_NETIF_HOSTNAME
    netif->hostname = "lwip";                                /* Initialize interface hostname */
#endif /* LWIP_NETIF_HOSTNAME */
    
        
    netif->hwaddr_len = 6;                                  /* set MAC hardware address length */
    
    dnepr_ethernet_str_2_mac( netif->hwaddr, val_CMMAC );     /* set MAC hardware address */

    /* maximum transfer unit */
//    netif->mtu = MTU_FEC - 18; (512-18)
//    netif->mtu = FEC_MTU;  (1518)
    netif->mtu = MAX_ETH_BUFF_SIZE - 18;  //(1518)
      
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHERNET | NETIF_FLAG_UP;  /* broadcast capability */
       
   /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
    
  netif->output = etharp_output;
#if LWIP_IPV6
//  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  
  netif->linkoutput = low_level_output;
          
#if LWIP_NETIF_LINK_CALLBACK
//  /** This function is called when the netif link is set to up or down
//   */
//  netif->link_callback =;
#endif /* LWIP_NETIF_LINK_CALLBACK */

/*
 * Initialize the snmp variables and counters inside the struct netif.
 * The last argument should be replaced with your link speed, in units
 * of bits per second.
 */
//    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);
      
    
    low_level_init(netif);                  /* инициализируем "железо" и модуль работы с дма */
  
    return (s8)ERR_OK;   
}


/*=============================================================================================================*/
/*! \brief 

    \return 
    \retval 
    \sa 
*/
/*=============================================================================================================*/
static void fill_rx_ring(t_dnepr_if *dnepr_if)
{
    struct pbuf         *p;
    t_txrx_desc         *p_rxbd;
    int                 i               = dnepr_if->rx_insert;
    void                *new_payload;
    u32_t               u_p_pay;
    
    /* Try and fill as many receive buffers as we can */
    while ( dnepr_if->rx_pbuf_a[i] == 0 )
    {
        p = pbuf_alloc(PBUF_RAW, (u16_t) dnepr_if->rx_buf_len, PBUF_POOL);

        if (p == 0) {           
            return;    /* No pbufs, so can't refill ring */
        }

        /* Align payload start to be divisible by 16 as required by HW */
        u_p_pay = (u32_t) p->payload;
        new_payload = p->payload = (void *) (((u_p_pay + 15) / 16) * 16);
        
        dnepr_if->rx_pbuf_a[i] = p;
        p_rxbd = &dnepr_if->rxbd_a[i];
        p_rxbd->starting_adress = (u8_t *) new_payload;
        p_rxbd->contr_status_flags = (p_rxbd->contr_status_flags & MCF_FEC_RxBD_W) | MCF_FEC_RxBD_E;
        INC_RX_BD_INDEX(dnepr_if->rx_insert);
        i = dnepr_if->rx_insert;
    }
}


/*=============================================================================================================*/
/*!  \brief 
     \sa 
*/
/*=============================================================================================================*/
static void dnepr_if_buf_clear
(
    t_dnepr_if  *dnepr_if
)
{
    int i;
  
    for (i = 0; i < ETHERNET_RX_BD_NUMBER; i++) {
        if (dnepr_if->rx_pbuf_a[i])   {
              pbuf_free(dnepr_if->rx_pbuf_a[i]);
              dnepr_if->rx_pbuf_a[i] = 0;
              dnepr_if->rxbd_a->starting_adress = 0;
        }
    }
    
        for (i = 0; i < ETHERNET_TX_BD_NUMBER; i++) {
            if (dnepr_if->tx_pbuf_a[i])   {
                pbuf_free(dnepr_if->tx_pbuf_a[i]);
                dnepr_if->tx_pbuf_a[i] = 0;
                dnepr_if->txbd_a->starting_adress = 0;
            }
        }
  
}
/*=============================================================================================================*/
/*!  \brief 
     \sa 
*/
/*=============================================================================================================*/

static void dnepr_if_buf_init
(
    t_dnepr_if  *dnepr_if
)
{
    int i;

    /* Initialize empty tx descriptor ring */
    for(i = 0; i < ETHERNET_TX_BD_NUMBER-1; i++)    {
        dnepr_if->txbd_a[i].contr_status_flags = 0;
    }

    /* Set wrap bit for last descriptor */
    dnepr_if->txbd_a[i].contr_status_flags = MCF_FEC_TxBD_W;
        

    /* initialize tx indexes */
    dnepr_if->tx_remove = dnepr_if->tx_insert = 0;
    dnepr_if->tx_free = ETHERNET_TX_BD_NUMBER;

        /* Initialize empty rx descriptor ring */
    for (i = 0; i < ETHERNET_RX_BD_NUMBER-1; i++) {
        dnepr_if->rxbd_a[i].contr_status_flags = 0;
    }

    /* Set wrap bit for last descriptor */
    dnepr_if->rxbd_a[i].contr_status_flags = MCF_FEC_RxBD_W;

    /* Initialize rx indexes */
    dnepr_if->rx_remove = dnepr_if->rx_insert = 0;

    /* Fill receive descriptor ring */
    fill_rx_ring(dnepr_if);
}




/*=============================================================================================================*/
/*!  \brief Функция инициализирующая интерфейс ethernet для lwIP
     \sa dnepr_if_init
*/
/*=============================================================================================================*/
static void low_level_init(struct netif *netif)
{
    t_dnepr_if          *dnepr_if;
    struct pbuf         *p;
    int                 i;

#ifdef MCF5282_DEBUG
    printf("low_level_init\n");
#endif
    
    dnepr_if = netif->state;
    
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

        dnepr_if->rx_buf_len = PBUF_POOL_BUFSIZE-16-i;

        pbuf_free(p);
    }
     
    dnepr_if_buf_clear(dnepr_if);
    
    (void)dnepr_ethernet_lwip_open(netif);    
    (void)dnepr_ethernet_fec_init(netif->hwaddr);   
    (void)dnepr_ethernet_phy_init();
    
    dnepr_if_buf_init(dnepr_if);
}



/*-----------------------------------------Передача---------------------------------------------------*/

/*=============================================================================================================*/
/*!  \brief
     \detail  Called when a raw link packet is ready to be transmitted. 
     \detail  This function should not add any more headers. 
     \detail  You must set netif->linkoutput to the address of this function.
     \detail   Should do the actual transmission of the packet. The packet is
     \detail  contained in the pbuf that is passed to the function. This pbuf might be chained.
     \detail  canonical name is low_level_output()
     \sa 
*/
/*=============================================================================================================*/
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    t_dnepr_if      *dnepr_if = netif->state;
    struct pbuf     *q, *r;
    char            *ptr;
    char            *pStart;
    unsigned int    len;
        
#if SYS_LIGHTWEIGHT_PROT == 1
    u32_t           old_level;
#endif

    
#if SYS_LIGHTWEIGHT_PROT == 1
    /* Interrupts are disabled through this whole thing to support  multi-threading
     * transmit calls. Also this function might be called from an ISR. */
    old_level = sys_arch_protect();
#endif

    // make sure a descriptor free
    if (dnepr_if->tx_free)    {
        r = pbuf_alloc(PBUF_RAW, p->tot_len + 16, PBUF_RAM);            
        // alloc mem for buffer

        if (r != NULL) {
            ptr = pStart = (void *) ((((u32)r->payload + 15) / 16) * 16);         // get start address aligned on 16 byte boundary

            for(q = p, len = 0; q != NULL; q = q->next) {
                memcpy(ptr + len, q->payload, q->len);
                len += q->len;
            }

#if defined(MCF5282_DEBUG)
            printf("low_level_output: mcf5282->tx_insert: %d\n", mcf5282->tx_insert);
#endif

                                                                                            /* put buffer on descriptor ring */
            dnepr_if->tx_free--;                                     
                                                                                            // dec free descriptors
            dnepr_if->tx_pbuf_a[dnepr_if->tx_insert] = r;                         
                                                                                            // save pointer to pbuf
            dnepr_if->txbd_a[dnepr_if->tx_insert].starting_adress = (volatile u8*)pStart;                 
                                                                                            // set start address of data
            dnepr_if->txbd_a[dnepr_if->tx_insert].data_length = len;                 
                                                                                            // set len of data

#if defined(MCF5282_DEBUG)
                printf("low_level_output: pbuf: 0x%08X, buf: 0x%08X, data_len: %d\n", dnepr_if->tx_pbuf_a[dnepr_if->tx_insert], 
                dnepr_if->txbd_a[mcf5282->tx_insert].starting_adress, 
                dnepr_if->txbd_a[mcf5282->tx_insert].data_len);
                printf("p_buf:\n");
                printf("\tdest mac: ");
                for(len = 0; len < 6; len++) {
                        printf("%02X%c", ((BYTE*)dnepr_if->txbd_a[dnepr_if->tx_insert].p_buf)[len], len == 5 ? '\n' : ':');
                }
                printf("\tsrc mac: ");
                for(; len < 12; len++) {
                    printf("%02X%c", ((BYTE*)dnepr_if->txbd_a[dnepr_if->tx_insert].p_buf)[len], len == 11 ? '\n' : ':');
                }

                for(; len < mcf5282->txbd_a[dnepr_if->tx_insert].data_len; len++) {
                    printf("%02X ", ((BYTE*)dnepr_if->txbd_a[dnepr_if->tx_insert].p_buf)[len]);
                }
                printf("\n");
#endif

                            // set flags
           dnepr_if->txbd_a[dnepr_if->tx_insert].contr_status_flags = (u16_t)(MCF_FEC_TxBD_R 
                                                                         | (dnepr_if->txbd_a[dnepr_if->tx_insert].contr_status_flags & MCF_FEC_TxBD_W)
                                                                         | (MCF_FEC_TxBD_L | MCF_FEC_TxBD_TC));

           INC_TX_BD_INDEX(dnepr_if->tx_insert);

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
    } /* if (dnepr_if->tx_free) */

        /* Drop the frame, we have no place to put it */
#ifdef LINK_STATS
            lwip_stats.link.memerr++;
#endif
#if SYS_LIGHTWEIGHT_PROT == 1
    sys_arch_unprotect(old_level);
#endif

   return (u8_t)ERR_MEM;
}


/*=============================================================================================================*/
/*!  \brief
     \detail  Called by ip_output when a packet is ready for transmission.  Any link headers will be added here.
     \detail   This function should call the myif_link_output function when the packet is ready.
     \detail  You must set netif->output to the address of this function.
     \detail  If your driver supports ARP, you can simply set netif->output to etharp_output.      
     \sa 
*/
/*=============================================================================================================*/
err_t dnepr_if_output(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr)
{
        /* resolve hardware address, then send (or queue) packet */
        return etharp_output(netif, p, ipaddr);
}



/*=============================================================================================================*/
/*! \brief 

    \return 
    \retval 
    \sa 
*/
/*=============================================================================================================*/
void dnepr_if_tx_cleanup(void)
{
    struct pbuf     *p;
    unsigned int    tx_remove_sof;
    unsigned int    tx_remove_eof;
    unsigned int    i;
    u16_t           flags;
    t_dnepr_if      *dnepr_if = &s_dnepr_if;
    

#if SYS_LIGHTWEIGHT_PROT == 1
    sys_prot_t       old_level;
#endif

    tx_remove_sof = tx_remove_eof = dnepr_if->tx_remove;

#if defined(MCF5282_DEBUG)
        printf("mcf5282fec_tx_cleanup: tx_remove_eof: %d\n", tx_remove_eof);
#endif

    /* We must protect reading the flags and then reading the buffer pointer. They must
       both be read together. */

#if SYS_LIGHTWEIGHT_PROT == 1
    old_level = sys_arch_protect();
#endif

    /* Loop, looking for completed buffers at eof */
    while ((((flags = dnepr_if->txbd_a[tx_remove_eof].contr_status_flags) & MCF_FEC_TxBD_R) == 0) &&
           (dnepr_if->tx_pbuf_a[tx_remove_eof] != 0))    {
        /* See if this is last buffer in frame */
        if ((flags & MCF_FEC_TxBD_L) != 0)  {
            i = tx_remove_eof;
            /* This frame is complete. Take the frame off backwards */
            do {
                p = dnepr_if->tx_pbuf_a[i];
                dnepr_if->tx_pbuf_a[i] = 0;
                dnepr_if->txbd_a[i].starting_adress = 0;
                dnepr_if->tx_free++;
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
    dnepr_if->tx_remove = tx_remove_sof;

        //MCF5282_FEC_EIMR |= MCF5282_FEC_EIMR_TXF;             // renable interrupt

#if SYS_LIGHTWEIGHT_PROT == 1
    sys_arch_unprotect(old_level);
#endif
}



/*-----------------------------------------Прием-------------------------------------------------------*/

/*=============================================================================================================*/
/*! \brief
    \detail  This function should be called when a packet is ready to be read
    \detail  from the interface. It uses the function low_level_input() that
    \detail  should handle the actual reception of bytes from the network
    \detail  interface. Then the type of the received packet is determined and
    \detail  the appropriate input function is called.
 
    \param netif the lwip network interface structure for this ethernetif
    \sa 
*/
/*=============================================================================================================*/
static void
//ethernetif_input(struct netif *netif)
ethernetif_input(struct pbuf  *p, struct netif *netif)
{
  struct eth_hdr        *ethhdr;
//  struct pbuf           *p;
//  /* move received packet into a new pbuf */
//  p = low_level_input(netif);
  
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = p->payload;

  switch (htons(ethhdr->type)) {
  /* IP or ARP packet? */
  case ETHTYPE_IP:
  case ETHTYPE_IPV6:
  case ETHTYPE_ARP:
#if PPPOE_SUPPORT
  /* PPPoE packet? */
  case ETHTYPE_PPPOEDISC:
  case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
    /* full packet send to tcpip_thread to process */
    if (netif->input(p, netif)!=ERR_OK)
     { LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
       pbuf_free(p);
       p = NULL;
     }
    break;

  default:
    pbuf_free(p);
    p = NULL;
    break;
  }
}


/*=============================================================================================================*/
/*! \brief
    \detail  Should allocate a pbuf and transfer the bytes of the incoming
    \detail  packet from the interface into the pbuf.
    \param netif the lwip network interface structure for this ethernetif
    \return a pbuf filled with the received packet (including MAC header) NULL on memory error
    \sa 
*/
/*=============================================================================================================*/
void low_level_input(struct netif *netif)
{
    u16_t               flags;
    unsigned int        rx_remove_sof;
    unsigned int        rx_remove_eof;
    struct pbuf         *p;
    t_dnepr_if          *dnepr_if = netif->state;    
    
    rx_remove_sof = rx_remove_eof = dnepr_if->rx_remove;

        /* Loop, looking for filled buffers at eof */
    while ((((flags = dnepr_if->rxbd_a[rx_remove_eof].contr_status_flags) & MCF_FEC_RxBD_E) == 0) && (dnepr_if->rx_pbuf_a[rx_remove_eof] != 0))  {
      /* See if this is last buffer in frame */
        if ((flags & MCF_FEC_RxBD_L) != 0)  {
            /* This frame is ready to go. Start at first descriptor in frame. */
            p = 0;
            do  {
                /* Adjust pbuf length if this is last buffer in frame */
                if (rx_remove_sof == rx_remove_eof)  {
                    dnepr_if->rx_pbuf_a[rx_remove_sof]->tot_len = dnepr_if->rx_pbuf_a[rx_remove_sof]->len = (u16_t)(dnepr_if->rxbd_a[rx_remove_sof].data_length - (p ? p->tot_len : 0));
                }
                else    {
                    dnepr_if->rx_pbuf_a[rx_remove_sof]->len =  dnepr_if->rx_pbuf_a[rx_remove_sof]->tot_len = dnepr_if->rxbd_a[rx_remove_sof].data_length;
                }
                
                /* Chain pbuf */
                if (p == 0)
                {
                    p = dnepr_if->rx_pbuf_a[rx_remove_sof];       // First in chain
                    p->tot_len = p->len;                        // Important since len might have changed
                } else {
                    pbuf_chain(p, dnepr_if->rx_pbuf_a[rx_remove_sof]);
                    pbuf_free(dnepr_if->rx_pbuf_a[rx_remove_sof]);
                }
                
                /* Clear pointer to mark descriptor as free */
                dnepr_if->rx_pbuf_a[rx_remove_sof] = 0;
                dnepr_if->rxbd_a[rx_remove_sof].starting_adress = 0;
                
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
            } else {
                /* Good frame. increment stat */
#ifdef LINK_STATS
                lwip_stats.link.recv++;
#endif
                   ethernetif_input(p, netif);
            }
        }
        INC_RX_BD_INDEX(rx_remove_eof);
    }
    dnepr_if->rx_remove = rx_remove_sof;

        /* Fill up empty descriptor rings */
    fill_rx_ring(dnepr_if);

        /* Set rx interrupt bit again */
    MCF_FEC_EIMR |= MCF_FEC_EIMR_RXF;

    /* Tell fec that we have filled up her ring */
    MCF_FEC_RDAR = 1;
}





/*=============================================================================================================*/




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








/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */




// Массив с сообщениями для посылки в очередь, из которой прерывание фрейма забирает эти
// сообщения и назначает массивы дескрипторам.


  // Делаем DSA tag
//  Dnepr_net_Fill_From_CPU_Tag( &from_cpu_tag, ethernetif->portnum );
  // копируем в буфер
//  t8_memcopy( &__tx_hdr_array[ __tx_hdr_i ][ hdr_len ], (u8*)p->payload, 12 );
//  hdr_len += 12 ;
//  Dnepr_net_Fill_From_CPU_Tag( &from_cpu_tag, 0 );
//  t8_memcopy( &__tx_hdr_array[ __tx_hdr_i ][ hdr_len ], (u8*)&from_cpu_tag, sizeof( from_cpu_tag ) );
//  hdr_len += 4 ;
//
//  fec_Transmit_2_BD( &__tx_hdr_array[ __tx_hdr_i ][ 0 ], hdr_len, (u8*)p->payload, p->len );


  // for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    // send data from(q->payload, q->len);
  // }

  // signal that packet should be sent();


