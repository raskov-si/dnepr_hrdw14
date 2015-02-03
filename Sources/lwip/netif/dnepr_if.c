#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/ppp/pppoe.h"
#include "lwip\debug.h"

#include "support_common.h"
#include "common_lib/memory.h"

//#include "Threads/inc/threadNet.h"

#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
//#include "HAL/MCU/inc/T8_5282_FEC.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'e'

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct ethernetif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
  u16 portnum ;
  u16 vlan ;
};

/* Forward declarations. */
static void  ethernetif_input(struct netif *netif);

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
low_level_init(struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
// debug  
//  t8_memcopy( netif->hwaddr, Dnepr_net_get_mac(), 6 );

  /* maximum transfer unit */
  netif->mtu = 1500;
  
  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
 
  /* Do whatever else is needed to initialize interface. */  
}

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
static size_t __tx_packet_mess_ind = 0 ;
#define TX_PACKET_MESS_LEN  32
//debug
//static Dnepr_Ethertnet_TX_Frame_t __tx_packet_mess[ TX_PACKET_MESS_LEN ] ;

static size_t __tx_hdr_i ;
#define TX_HDR_NUM  16
#define TX_HDR_LEN  (6+6+sizeof(FROM_CPU_TAG)) // длина заголовка: 2 мак-адреса и FROM_CPU_TAG
#pragma data_alignment=16
_Pragma("location=\"packets_sram\"")
//debug
//__no_init static u8 __tx_hdr_array[TX_HDR_NUM][TX_HDR_LEN] ;

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;

//debug
//  FROM_CPU_TAG from_cpu_tag ;
  size_t hdr_len = 0 ;

  //initiate transfer();
  
#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif


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

  if( ++__tx_hdr_i >= TX_HDR_NUM ){
    __tx_hdr_i = 0 ;
  }


#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
  
  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
low_level_input(struct netif *netif)
{
//   struct ethernetif *ethernetif = netif->state;
//   struct pbuf *p, *q;
//   u16_t len;

//   // Obtain the size of the packet and put it into the "len"
//   // variable. 
//   len = ;

// #if ETH_PAD_SIZE
//   len += ETH_PAD_SIZE;  allow room for Ethernet padding 
// #endif

//    // We allocate a pbuf chain of pbufs from the pool. 
//   p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  
//   if (p != NULL) {

// #if ETH_PAD_SIZE
//     pbuf_header(p, -ETH_PAD_SIZE); // drop the padding word 
// #endif

//      // We iterate over the pbuf chain until we have read the entire
//      // * packet into the pbuf. 
//     for(q = p; q != NULL; q = q->next) {
//        // Read enough bytes to fill this pbuf in the chain. The
//        // available data in the pbuf is given by the q->len
//        // variable.
//        // This does not necessarily have to be a memcpy, you can also preallocate
//        // pbufs for a DMA-enabled MAC and after receiving truncate it to the
//        // actually received size. In this case, ensure the tot_len member of the
//        // pbuf is the sum of the chained pbuf len members.
       
//       read data into(q->payload, q->len);
//     }
//     acknowledge that packet has been read();

// #if ETH_PAD_SIZE
//     pbuf_header(p, ETH_PAD_SIZE); // reclaim the padding word
// #endif

//     LINK_STATS_INC(link.recv);
//   } else {
//     drop packet();
//     LINK_STATS_INC(link.memerr);
//     LINK_STATS_INC(link.drop);
//   }

//   return p;  
  return NULL ;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
static void
ethernetif_input(struct netif *netif)
{
  struct ethernetif *ethernetif;
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  ethernetif = netif->state;

  /* move received packet into a new pbuf */
  p = low_level_input(netif);
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

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethernetif_init(struct netif *netif)
{
  struct ethernetif *ethernetif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));
    
  ethernetif = mem_malloc(sizeof(struct ethernetif));
  if (ethernetif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
    return ERR_MEM;
  }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->state = ethernetif;
  netif->name[0] = IFNAME0;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;
  
  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  
  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}
