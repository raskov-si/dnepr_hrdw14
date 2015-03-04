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
#include "lwip/tcpip.h"

#ifdef DEBUG_I2C
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "Threads/inc/threadTerminal.h"
#endif


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

#ifdef DEBUG_NET
    int         debug_netcmd_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog);
    int         debug_netlog_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog);
#endif
    extern  err_t cb_dnepr_eth0_if_init(struct netif *netif);


/*=============================================================================================================*/
/* ping variables */
static t_eth    *eth0;

static u16 ping_seq_num;
static u32 ping_time; 

#ifdef DEBUG_NET

REGISTER_COMMAND("netcmd", debug_netcmd_term, NULL, NULL, NULL);
//REGISTER_COMMAND("netlog", debug_netlog_term, debug_netlog_term_get_message_num, debug_netlog_term_get_message, NULL);
REGISTER_COMMAND("netlog", debug_netlog_term, NULL, NULL, NULL);

#endif



/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_NET
int         debug_netcmd_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog)
{
    
  
    return 0;
}
#endif


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
#ifdef DEBUG_NET
int         debug_netlog_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog)
{
  char  *second_word;
  char  pars_buf[5+1];
//  u8    idx_beg;
  
  if ( (second_word = strstr (in, "show")) != NULL )  {
     
     char  *third_word = strstr (second_word, "mac");
     
     if ( third_word != NULL ) {
        /* текущее состояние */       
        return snprintf(out, out_len_max, "0x%X, \r\n", 12345);
     }
     
     third_word = strstr (second_word, "ip");
     if ( third_word != NULL ) {
        /* текущее состояние */       
        return snprintf(out, out_len_max, "1.3.6.7\r\n");
     }

  }    
  
    return 0;
}
#endif

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
static void task_eth_init (void)
{
 /* Network interface variables */
    ip_addr_t       ipaddr, netmask, gw;
    
#ifdef DEBUG_NET

    volatile  char*  temp;
    volatile  char*  temp2;

    temp =  (volatile  char*)debug_netlog_term_handler.cmd_name;
    temp2 = temp;
    temp =  (volatile  char*)debug_netcmd_term_handler.cmd_name;
    temp = temp2;   
        
#endif    
    
    (void)dnepr_ethernet_open(eth0);
    
    
/* Стартуем LwIP */
  
 /* Set network address variables */
    IP4_ADDR(&gw,          192,168,1,100);
    IP4_ADDR(&ipaddr,      192,168,1,7);
    IP4_ADDR(&netmask,     255,255,255,0);
 
 // Start the TCP/IP thread & init stuff  
    tcpip_init(NULL, NULL);      
    
// WARNING: This must only be run after the OS has been started.  
    // Typically this is the case, however, if not, you must place this  
    // in a post-OS initialization  
    netifapi_netif_add(&eth0->netif, &ipaddr, &netmask, &gw, NULL, cb_dnepr_eth0_if_init, tcpip_input);      
  
//  netif_set_up(&net);
        
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void task_eth( void *pdata )
{
      task_eth_init();
                  
      
#if LWIP_TCPIP_CORE_LOCKING_INPUT
      while ( TRUE )    {
//          watch_dog_thread_is_alive()          
        
//        dnepr_ethernet_rx_check(eth0.data_descr)t_eth_data_descr
        
            {
             
            /* принимаем сообщения */
            //      eth0.netif->input(eth0.data_descr->pockets_array[eth0 msg][0]);         
              ;
            }
            
//        dnepr_ethernet_tx_check(eth0.data_descr)
            {
              
            }
      };
#else
         OSTaskDel( OS_PRIO_SELF ) ;           
#endif        
}



/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void task_snmp( void *pdata )
{

  
    /* ping */
        
    while ( TRUE )    {
        OSTimeDly(1 * OS_TICKS_PER_SEC);
    }      
}
