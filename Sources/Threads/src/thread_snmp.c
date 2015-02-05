/*=============================================================================================================*/
/*!  \file thread_snmp.c
*    \brief     поток задач обслуживания сетевого протокола
*    \details   
*/
/*=============================================================================================================*/

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "common_lib/memory.h"


//#include "HAL/MCU/inc/T8_5282_FEC.h"
#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"

#include "Threads/inc/thread_snmp.h"

#include "lwip/opt.h" 
#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timers.h"
#include "lwip/inet_chksum.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

/** ping receive timeout - in milliseconds */
#define PING_RCV_TIMEO 1000
/** ping delay - in milliseconds */
#define PING_DELAY     1000 
/** ping additional data size to include in the packet */
#define PING_DATA_SIZE 32
/** ping identifier - must fit on a u16_t */
#define PING_ID        0xAFAFu 


#define PING_THREAD_STACKSIZE   (100)
#define PING_THREAD_PRIO        (36)


#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any) 

/*=============================================================================================================*/
/* ping variables */
static u16 ping_seq_num;
static u32 ping_time; 





//static struct
//{
//    u8          mac_address[6];   /*!< адрес ethernet */
//    
//    ip_addr_t   ip_adress;
//    ip_addr_t   netmask;
//    ip_addr_t   gw;
//    
//} snmp_stack_config;

static struct netif net;

/*=============================================================================================================*/
/** Prepare a echo ICMP request */
static void
ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id     = PING_ID;
  iecho->seqno  = htons(++ping_seq_num);

  /* fill the additional data buffer with some data */
  for(i = 0; i < data_len; i++) {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

  iecho->chksum = inet_chksum(iecho, len);
}


/* Ping using the socket ip */
static err_t
ping_send(int s, ip_addr_t *addr)
{
    int     err;
    struct  icmp_echo_hdr *iecho;
    struct  sockaddr_in to;
    size_t  ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
  
    LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

    iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
    if (!iecho) {
        return ERR_MEM;
    }

    ping_prepare_echo(iecho, (u16_t)ping_size);

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    inet_addr_from_ipaddr(&to.sin_addr, addr);

    err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));

    mem_free(iecho);

    return (err ? ERR_OK : ERR_VAL);
} 


static void
ping_recv(int s)
{
  char buf[64];
  int fromlen, len;
  struct sockaddr_in from;
  struct ip_hdr *iphdr;
  struct icmp_echo_hdr *iecho;

  while((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
    if (len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) {
      ip_addr_t fromaddr;
      inet_addr_to_ipaddr(&fromaddr, &from.sin_addr);
      LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
      ip_addr_debug_print(PING_DEBUG, &fromaddr);
      LWIP_DEBUGF( PING_DEBUG, (" %"U32_F" ms\n", (sys_now() - ping_time)));

      iphdr = (struct ip_hdr *)buf;
      iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
      if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))) {
        /* do some ping result processing */
        PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER));
        return;
      } else {
        LWIP_DEBUGF( PING_DEBUG, ("ping: drop\n"));
      }
    }
  }

  if (len == 0) {
    LWIP_DEBUGF( PING_DEBUG, ("ping: recv - %"U32_F" ms - timeout\n", (sys_now()-ping_time)));
  }

  /* do some ping result processing */
  PING_RESULT(0);
} 

static void ping_thread
(
    void *arg
)
{
  int s;
  int timeout = PING_RCV_TIMEO;
  ip_addr_t   ping_target;

  LWIP_UNUSED_ARG(arg);

  if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
    return;
  }

  lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  while (1) {
    ping_target = PING_TARGET;

    if (ping_send(s, &ping_target) == ERR_OK) {
      LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
      ip_addr_debug_print(PING_DEBUG, &ping_target);
      LWIP_DEBUGF( PING_DEBUG, ("\n"));

      ping_time = sys_now();
      ping_recv(s);
    } else {
      LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
      ip_addr_debug_print(PING_DEBUG, &ping_target);
      LWIP_DEBUGF( PING_DEBUG, (" - error\n"));
    }
    sys_msleep(PING_DELAY);
  }
} 
/*=============================================================================================================*/


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void task_snmp( void *pdata )
{

/* Стартуем LwIP */
//    netif_add( &net, ip_addr_t *ipaddr, ip_addr_t *netmask,
//              ip_addr_t *gw, void *state, cb_dnepr_eth0_if_init(), netif_input_fn input);
  
    

	// tcpip_init( __tcpip_init_done, NULL );
	// 
  
    /* ping */
//    sys_thread_new("ping_thread", ping_thread, NULL, PING_THREAD_STACKSIZE, PING_THREAD_PRIO);
    
    
    return;
}
