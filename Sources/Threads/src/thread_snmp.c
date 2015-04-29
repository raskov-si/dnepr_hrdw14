/*=============================================================================================================*/
/*!  \file thread_snmp.c
*    \brief     поток задач обслуживания сетевого протокола
*    \details   
*/
/*=============================================================================================================*/

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "common_lib/memory.h"


//#include "HAL/MCU/inc/T8_5282_FEC.h"
#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "reserv/core/inc/rsrv_meraprot.h"
#include "Application/inc/t8_dnepr_time_date.h"



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
    #include <stdio.h>
    #include "Threads/inc/threadTerminal.h"
#endif

//#define   CPU_MERA_HOST         "192.168.1.100"
#define   CPU_MERA_HOST         "192.168.10.1"
//#define   CPU_MERA_HOST         val_CUNetCPUIPAddress
#define   CPU_MERA_PORT         (0x7777u)
#define   CPU_MERA_RCV_TIMEO        500



/** ping receive timeout - in milliseconds */
#define PING_RCV_TIMEO 1000
/** ping delay - in milliseconds */
#define PING_DELAY     5000 
/** ping additional data size to include in the packet */
#define PING_DATA_SIZE 32
/** ping identifier - must fit on a u16_t */
#define PING_ID        0xAFAFu 

#define PING_THREAD_STACKSIZE   (100)
#define PING_THREAD_PRIO        (36)
#define PING_RESULT(ping_ok)
//#define PING_TARGET             (netif_default?netif_default->gw:ip_addr_any)
#define SET_TARGET(addr)             IP4_ADDR(&addr, 192,168,1,100);



/*=============================================================================================================*/

#ifdef DEBUG_NET
    int         debug_netcmd_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog);
    int         debug_netlog_term(const char* in, char* out, size_t out_len_max, t_log_cmd *sendlog);
#endif
    extern  err_t   dnepr_if_init(struct netif *netif);
    extern  TRoles  rsrv_main_get_cpu_role(void);

    
    static void     ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len);
    static void     ping_recv(int s);
    static err_t    ping_send(int s, ip_addr_t *addr);
    
    
    

/*=============================================================================================================*/
/* ping variables */
//static t_eth            *eth0;
static struct netif     eth0_netif;
static u16              ping_seq_num;
//static u32 ping_time; 
extern    s8  val_CUNetCPUIPAddress[18];
extern    s8  val_CUNetMCUIPAddress[18];
extern    s8  val_CUNetIPMask[18];

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
//  char  pars_buf[5+1];
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
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void tcpip_init_done(void *arg)
{
        sys_sem_t *sem;


        sem = arg;
        sys_sem_signal(sem);
}



/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void task_eth_init (void)
{
 /* Network interface variables */
    ip_addr_t       ipaddr, netmask, gw;
    sys_sem_t       sem;
    
#ifdef DEBUG_NET

    volatile  char*  temp;
    volatile  char*  temp2;

    temp =  (volatile  char*)debug_netlog_term_handler.cmd_name;
    temp2 = temp;
    temp =  (volatile  char*)debug_netcmd_term_handler.cmd_name;
    temp = temp2;   
        
#endif        
    
       
/* Стартуем LwIP */  
 /* Set network address variables */
//    IP4_ADDR(&gw,          192,168,1,100);
//    IP4_ADDR(&ipaddr,      192,168,1,3);
//    IP4_ADDR(&netmask,     255,255,255,0);        
    
    ipaddr_aton("0.0.0.0", &gw);
//    ipaddr_aton("192.168.1.3", &ipaddr);
    ipaddr_aton("192.168.10.2", &ipaddr);
//    ipaddr_aton(val_CUNetMCUIPAddress, &ipaddr);
    ipaddr_aton(val_CUNetIPMask, &netmask);
 
// Start the TCP/IP thread & init stuff  
    sys_sem_new(&sem, 0);
    tcpip_init(tcpip_init_done, &sem);
    sys_sem_wait(&sem);
    sys_sem_free(&sem);
    
// WARNING: This must only be run after the OS has been started.  
    // Typically this is the case, however, if not, you must place this  
    // in a post-OS initialization  
    netifapi_netif_add(&eth0_netif, &ipaddr, &netmask, &gw, NULL, dnepr_if_init, tcpip_input);
    netif_set_default(&eth0_netif);
    netif_set_up(&eth0_netif);
}





/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void task_eth( void *pdata )
{
    int         sock_desc;
    int         retsock;
    int         timeout     = CPU_MERA_RCV_TIMEO;
    u32         answ_timer  = OSTimeGet();
    u32         now_time    = OSTimeGet();
    
    struct sockaddr_in  serv_addr;
    struct sockaddr_in  client_addr;
    u8                  buf[64];
    u8                  len;
  
    LWIP_UNUSED_ARG(pdata);

    task_eth_init();
    
    sock_desc = socket(AF_INET, SOCK_DGRAM, IP_PROTO_UDP);
    assert(sock_desc >= 0);
    
    retsock = setsockopt(sock_desc, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));    
    assert(retsock == 0);
        
    /* set up address to connect to */  
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    
    client_addr.sin_len = sizeof(client_addr);
    client_addr.sin_family = AF_INET;  
    client_addr.sin_port = PP_HTONS(CPU_MERA_PORT);
    client_addr.sin_addr.s_addr = inet_addr("192.168.10.2");    
    
    retsock = bind(sock_desc, (struct sockaddr*)&client_addr, sizeof(client_addr)); 
    assert(retsock == 0);       
      
    serv_addr.sin_len = sizeof(serv_addr);
    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_port = PP_HTONS(CPU_MERA_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(CPU_MERA_HOST);    
    
    while ( TRUE )    {
      memset(buf, 0, 64);
      len = meraprot_setrole_cmd(buf, rsrv_main_get_cpu_role());
      retsock = sendto(sock_desc, buf, len, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
      
      memset(buf, 0, 64);
      retsock = recvfrom(sock_desc, buf, MERAPROT_ROLECFM_LEN, 0, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr);
      now_time = OSTimeGet();
      if (retsock == MERAPROT_ROLECFM_LEN) {
          meraprot_setrole_cfm(buf, rsrv_main_get_cpu_role());
          answ_timer  = OSTimeGet();
          sys_msleep(1000);
      } else if ( (answ_timer + 3000) < now_time ) {                              
          rsrv_main_set_cpu_udp_error();
      }
            
    }
    
    retsock = close(sock_desc);
    assert(retsock == 0);    
}



/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void task_snmp( void *pdata )
{
    while ( TRUE )    {
        OSTimeDly(1 * OS_TICKS_PER_SEC);
    }      
}








/*=============================================================================================================*/
/*=============================================================================================================*/




/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
//void task_eth( void *pdata )
//{
//    int         sock_desc;
//    ip_addr_t   ping_target;
//    int         timeout         = PING_RCV_TIMEO;
//    int         retsock;
//  
//    LWIP_UNUSED_ARG(pdata);
//
//    task_eth_init();
//
//    sock_desc = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
//    assert(sock_desc >= 0);
//
//    retsock = setsockopt(sock_desc, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));    
//    assert(retsock == 0);
//
//    while ( TRUE )    {
//        /* ping */          
//      SET_TARGET(ping_target);
//      
//    if (ping_send(sock_desc, &ping_target) == ERR_OK) {
//      LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
//      ip_addr_debug_print(PING_DEBUG, &ping_target);
//      LWIP_DEBUGF( PING_DEBUG, ("\n"));
//
//      ping_recv(sock_desc);
//    } else {
//      LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
//      ip_addr_debug_print(PING_DEBUG, &ping_target);
//      LWIP_DEBUGF( PING_DEBUG, (" - error\n"));
//    }
//      
//      sys_msleep(PING_DELAY);                    
//    }      
//      
//}



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
        return (err_t)ERR_MEM;
    }

    ping_prepare_echo(iecho, (u16_t)ping_size);

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    inet_addr_from_ipaddr(&to.sin_addr, addr);

    err = sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));

    mem_free(iecho);

    return (err_t)(err ? ERR_OK : ERR_VAL);
} 


static void
ping_recv(int s)
{
  char buf[64];
  int fromlen, len;
  struct sockaddr_in from;
  struct ip_hdr *iphdr;
  struct icmp_echo_hdr *iecho;

  while((len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
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
/*=============================================================================================================*/


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static int eth_get_ipv4addr_from_str
(
  ip_addr_t   *addr,
  s8          *addr_string         
)
{
  u8          one,two,three,four;
  char        *beg;
  char        *end;
  char        buf[4];
  u8          len;
  
  assert (addr != NULL);
  assert (addr_string != NULL);
    
  beg =  addr_string;
  if ( (*beg == '\0') || ( (end = strstr(beg, ".")) == NULL ) ) {
    return -1;
  }
  len = end-beg;        
        if (len > 3) {
          return -1;
        }
  strncpy(buf, beg, len);        
  buf[3] = '\0';
  one = atoi(buf);     
  
  beg =  end+1;
  if ( (*beg == '\0') || ( (end = strstr(beg, ".")) == NULL ) ) {
    return -1;
  }
  len = end-beg;        
        if (len > 3) {
          return -1;
        }
  strncpy(buf, beg, len);        
  buf[3] = '\0';
  two = atoi(buf);     

  beg =  end+1;
  if ( (*beg == '\0') || ( (end = strstr(beg, ".")) == NULL ) ) {
    return -1;
  }
  len = end-beg;
        if (len > 3) {
          return -1;
        }
  strncpy(buf, beg, len);        
  buf[3] = '\0';
  three = atoi(buf);     

  beg =  end+1;
  if ( *beg == '\0' ) {
    return -1;
  }
  strncpy(buf, beg, 3);        
  buf[3] = '\0';
  four = atoi(buf);     

  IP4_ADDR(addr,one,two,three,four);  
  return 0;
}
