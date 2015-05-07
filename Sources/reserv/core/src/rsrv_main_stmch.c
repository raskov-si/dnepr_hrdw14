#include <time.h>
#include "rsrv_os.h"
#include "rsrv_discr.h"
#include "reserv/core/inc/rsrv_typedef.h"
#include "reserv/core/inc/stmch.h"
#include "reserv/core/inc/rsrv_mcumcu_protocol_stmchn.h"
#include "reserv/core/inc/rsrv_mcumcu_protocol_func.h"
#include "reserv/core/inc/rsrv_decision.h"
#include "rsrv_backplane.h"
#include "prio.h"


/*=============================================================================================================*/

#define UNUSED_ARG(arg) (void)*(arg);

/*=============================================================================================================*/
/*------------------------------------------- автомат MAIN SHOW------------------------------------------------*/

enum CU_SHOW_STM_STATES
{
    STATE_INIT = 0,
    STATE_DIAG,
    STATE_DIAG_CPU,
    STATE_PASSIVE,
    STATE_ACTIVE
};

/*--------------------------------------------------------------------------------------------------------------*/

enum _CU_SHOW_STM_SIGNALS
{
    SIG_STATIC            = 0,
    
    INIT_GOTO_DIAG        = 1,
    
    DIAG_GOTO_CPU_DIAG    = 1,    
    DIAG_GOTO_PASSIVE     = 2,

    DIAG_GOTO_ACTIVE      = 1,
          
    PASSIVE_GOTO_CPU_DIAG = 1,
    
    ACTIVE_GOTO_PASSIVE   = 1
};

/*--------------------------------------------------------------------------------------------------------------*/

static void stmch_init      (int state, int signal);
static void stmch_diag      (int state, int signal);
static void stmch_diag_cpu  (int state, int signal);
static void stmch_passive   (int state, int signal);
static void stmch_active    (int state, int signal);

/*--------------------------------------------------------------------------------------------------------------*/

const struct STM_TRANSITION  _cu_show_main_state_machine_tbl[5][3] =
{
    [STATE_INIT][SIG_STATIC]                      = {STATE_INIT,     stmch_init},
    [STATE_INIT][INIT_GOTO_DIAG]                  = {STATE_DIAG,     NULL},
    
    [STATE_DIAG][SIG_STATIC]                      = {STATE_DIAG,     stmch_diag},
    [STATE_DIAG][DIAG_GOTO_CPU_DIAG]              = {STATE_DIAG_CPU, NULL},
    [STATE_DIAG][DIAG_GOTO_PASSIVE]               = {STATE_PASSIVE,  NULL},
        
    [STATE_DIAG_CPU][SIG_STATIC]                  = {STATE_DIAG_CPU, stmch_diag_cpu},
    [STATE_DIAG_CPU][DIAG_GOTO_ACTIVE]            = {STATE_ACTIVE,   NULL},
    [STATE_DIAG_CPU][DIAG_GOTO_PASSIVE]           = {STATE_PASSIVE,  NULL},

    [STATE_PASSIVE][SIG_STATIC]                   = {STATE_PASSIVE,  stmch_passive},
    [STATE_PASSIVE][PASSIVE_GOTO_CPU_DIAG]        = {STATE_DIAG_CPU, NULL},
    
    [STATE_ACTIVE][SIG_STATIC]                    = {STATE_ACTIVE,   stmch_active},
    [STATE_ACTIVE][ACTIVE_GOTO_PASSIVE]           = {STATE_PASSIVE,  NULL},
};

/*--------------------------------------------------------------------------------------------------------------*/

static enum CU_SHOW_STM_STATES             current_main_state            = STATE_INIT;
static enum _CU_SHOW_STM_SIGNALS           current_main_signal           = SIG_STATIC;
static enum _CU_SHOW_STM_SIGNALS           main_signal_transition        = SIG_STATIC;

/*=============================================================================================================*/

TrsrvOsMbox                     mcumcu_main_mbox;
TrsrvIntertaskMessage           *mcumcu_main_mbox_message[1];
TrsrvOsMbox                     ethernet_main_mbox;
TrsrvIntertaskMessage           *ethernet_main_mbox_message[1];
struct _MCU_VIEW_PAIR           McuViewPair;
TRoles                         mcu_role = RESERV_ROLES_UNKNOWN;
TRoles                         cpu_role = RESERV_ROLES_UNKNOWN;


/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void rsrv_mcumcu_protocol_start(void)
{
    TrsrvIntertaskMessage   msg;
    
    msg = RESERV_INTERCODES_MCUMCU_START;    
    rsrv_os_mbox_post(mcumcu_main_mbox, &msg);  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void rsrv_ethernet_protocol_start(void)
{
    TrsrvIntertaskMessage   msg;
    
    msg = RESERV_INTERCODES_MCUCPU_START;    
    rsrv_os_mbox_post(ethernet_main_mbox, &msg);  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void resrv_wait_mcumcu_diag(void)
{  
    TrsrvIntertaskMessage   *msg;    
    int                     retval;
    
//    do {
        retval = rsrv_os_mbox_fetch(mcumcu_main_mbox, (void*)&msg, RSRV_MCUMCU_START_TIMEOUT);          
//        if ( retval == 1 ) {
//            break;
//        } else if ( retval < 0 ) {
//            break;
//        }
//    } while ( *msg != RESERV_INTERCODES_MCUMCU_ENDDIAG );        
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void rsrv_vlan_config(void)
{
  
}



/*--------------------------------------функции конечного автомата MAIN--------------------------------------*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
enum CU_SHOW_STM_STATES  *mainstmch_get_current_state (void)
{
    return &current_main_state;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
int mainstmch_recv_signal (void)
{
    current_main_signal       = main_signal_transition;
    main_signal_transition    = SIG_STATIC;
    return current_main_signal;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_init(int state, int signal)
{  
    /* подавляем любую активность на внешних шинах */
    rsrv_backplane_stop_access();
  
    /* узнаем номер слота (резервный или основной) */
    rsrv_os_lock(&McuViewPair.Sem);                
    McuViewPair.Local.RackNum = rsrv_get_dneprslotnum() ;
    rsrv_os_unlock(&McuViewPair.Sem);          
        
    if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT ) {
      McuViewPair.Remote.RackNum = RSRV_DNEPR_MAIN_SLOT;
    } else {
      McuViewPair.Remote.RackNum = RSRV_DNEPR_RESERVE_SLOT;
    }        
    
    /* определение сигнала present соседнего MCU */
    rsrv_os_lock(&McuViewPair.Sem);        
    McuViewPair.Local.RemotePresent = rsrv_get_dneprpresent();
    rsrv_os_unlock(&McuViewPair.Sem);      
            
      
    /* инициализируем VLAN */
//    rsrv_vlan_config();
  
    /* инициализируем протоколы UART и ethernet */
    rsrv_mcumcu_protocol_start();
    OSTaskSuspend(OS_PRIO_SELF);
    
    main_signal_transition = INIT_GOTO_DIAG;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_diag(int state, int signal)
{
    TreservMCUDiagVector    mcu_diag_vector;
      
    resrv_wait_mcumcu_diag();   /*  ждем сообщения от автомата протокола MCUMCU, что диагностика сигнальныхлиний завершена */
    
    mcu_diag_vector.UARTTxRx = (McuViewPair.Local.UARTTx == RESERV_TREESTATE_CHECKED && McuViewPair.Local.UARTRx == RESERV_TREESTATE_CHECKED)
                               ? RESERV_TREESTATE_CHECKED : RESERV_TREESTATE_DAMAGED;
  
    /* определяем состояние и роль соседнего CU, если он есть */
    rsrv_os_lock(&McuViewPair.Sem);                
    McuViewPair.Remote = rsrv_get_recevied_mcupair()->Remote;     
    rsrv_os_unlock(&McuViewPair.Sem);                      
    
    mcu_diag_vector.NeighborPresent = McuViewPair.Local.RemotePresent;
    mcu_diag_vector.NeighborRole    = McuViewPair.Remote.Role; 
    
    /* по составленной диагностике определяем какое состояние должно быть у MCU */
    mcu_diag_vector.SlotPosition = McuViewPair.Local.RackNum;
    mcu_role = rsrv_make_mcu_descision( &mcu_diag_vector );

    /* если пассивное переходим на passive state */
    if ( mcu_role != RESERV_ROLES_MASTER )  {
        main_signal_transition = DIAG_GOTO_PASSIVE;
        return;
    }

    /* если активное переходим на diag_cpu */
    main_signal_transition = DIAG_GOTO_CPU_DIAG;
    
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_diag_cpu(int state, int signal)
{
    rsrv_os_lock(&McuViewPair.Sem);        
    McuViewPair.Local.Role = RESERV_ROLES_UNKNOWN;
    rsrv_os_unlock(&McuViewPair.Sem);      
    
    rsrv_ethernet_protocol_start();
  
    /* синхронизируем профиль в eeprom */
    rsrv_backplane_sync();
    
    OSTaskSuspend(OS_PRIO_SELF);    
          
    /* выдаем на CPU роль стать активным */
    if ( McuViewPair.Local.CPUState == RESERV_TREESTATE_CHECKED ) {
        cpu_role = RESERV_ROLES_MASTER;
      /* выбираем роль в зависимости от ответа (максимум через 3 сек)*/    
        main_signal_transition = DIAG_GOTO_ACTIVE;       
        return;
    }
    
    
    cpu_role = RESERV_ROLES_SLAVE;
    /* выбираем роль в зависимости от ответа (максимум через 3 сек)*/    
    main_signal_transition = DIAG_GOTO_PASSIVE;       
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_passive(int state, int signal)
{
    /* выдаем на CPU роль стать пассивным */
    cpu_role = RESERV_ROLES_SLAVE;
  
    rsrv_os_lock(&McuViewPair.Sem);        
    McuViewPair.Local.Role = RESERV_ROLES_SLAVE;
    rsrv_os_unlock(&McuViewPair.Sem);      
    
    /* подавляем любую активность на внешних шинах */
    rsrv_backplane_stop_access();
    
    /* выдаем световую индикацию */
    rsrv_leds_setstate(McuViewPair.Local.Role);
    
  
    /* принимаем пинги по UARTу, отсылаем понги */
    while ( TRUE )
    {
        OSTimeDly( 1000 );                  /* время резервирования */
    
        /* если теряем сигнал present от активного и сигнал по uart, переходим на диагностику CPU   */
        rsrv_os_lock(&McuViewPair.Sem);        
        McuViewPair.Local.RemotePresent = rsrv_get_dneprpresent();
        McuViewPair.Remote = rsrv_get_recevied_mcupair()->Remote;        
        rsrv_os_unlock(&McuViewPair.Sem);      
            
        if (  McuViewPair.Local.RemotePresent == RSRV_DNEPR_PRESSIGNAL_NONE ) {            
            main_signal_transition = PASSIVE_GOTO_CPU_DIAG;
            return;
        }
      
        if ( McuViewPair.Local.UARTTx == RESERV_TREESTATE_DAMAGED || McuViewPair.Local.UARTRx == RESERV_TREESTATE_DAMAGED ) {
            main_signal_transition = PASSIVE_GOTO_CPU_DIAG;               
            return;
        }
        
        /* если принимаем пинг с соседним "пассивным" через некоторое время, переходим на диагностику CPU */
        if (  rsrv_get_recevied_mcupair()->Remote.Role == RESERV_ROLES_SLAVE ) {
            main_signal_transition = PASSIVE_GOTO_CPU_DIAG;                           
            return;
        }          
    }
      
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_active(int state, int signal)
{
    cpu_role = RESERV_ROLES_MASTER;
  
    rsrv_os_lock(&McuViewPair.Sem);        
    McuViewPair.Local.Role = RESERV_ROLES_MASTER;
    rsrv_os_unlock(&McuViewPair.Sem);      

    /* выдаем световую индикацию */
    rsrv_leds_setstate(McuViewPair.Local.Role);
  
    /* работаем в обычном режиме, отсылаем пинги по уарту, принимаем понги */
    { ;  }
    
    /* разрешаем работу всего шасси */
    rsrv_backplane_start_access();
    
  
    while ( TRUE )
    {
        OSTimeDly( 1000 );                  /* время резервирования */
        
        /* если теряем сигнал present от активного и сигнал по uart, переходим на диагностику CPU   */
        rsrv_os_lock(&McuViewPair.Sem);        
        McuViewPair.Local.RemotePresent = rsrv_get_dneprpresent();
        McuViewPair.Remote = rsrv_get_recevied_mcupair()->Remote;
        rsrv_os_unlock(&McuViewPair.Sem);              
        
        
              
        /* следим за работой CPU (ответами по uDP)если ответов нет в течении 3 сек меняем MCU на пассивное */
        if ( McuViewPair.Local.CPUState == RESERV_TREESTATE_DAMAGED )
        {
            rsrv_backplane_stop_access();
            main_signal_transition = ACTIVE_GOTO_PASSIVE;
        }
    }
      
}






/*=============================================================================================================*/



void  task_reserv_init(void)
{
    unsigned int adress = 1, present = 0;
    TrsrvIntertaskMessage   *msg;
    
    rsrv_os_mbox_new(&mcumcu_main_mbox, mcumcu_main_mbox_message);
    rsrv_os_mbox_new(&ethernet_main_mbox, ethernet_main_mbox_message);    
    
    rsrv_os_lock_create(&McuViewPair.Sem);
    
    rsrv_os_lock(&McuViewPair.Sem);        
    rsrv_mcuview_init(&McuViewPair.Local);
    rsrv_mcuview_init(&McuViewPair.Remote);
    McuViewPair.Local.Adr = adress;
    McuViewPair.Local.RemotePresent = present;
    McuViewPair.Remote.RemotePresent = 1;
    rsrv_os_unlock(&McuViewPair.Sem);
}

void task_reserv(TrsrvOsThreadArg   *arg)
{
    UNUSED_ARG(arg);

    /*      */
    task_reserv_init();

    while (1) {
        DO_STATE_MACHINE_TABLE(enum CU_SHOW_STM_STATES, mainstmch_get_current_state, mainstmch_recv_signal, _cu_show_main_state_machine_tbl)      
    }
}



void task_mcumcu(TrsrvOsThreadArg   *arg)
{
    UNUSED_ARG(arg);

    while (1) {
        DO_STATE_MACHINE_TABLE(enum _STM_MCUMCU_STATES, rsrv_mcumcu_prtcl_get_currect_state, resrv_mcumcu_prtcl_recv_signal, rsrv_mcumcu_show_stchtbl)
    }
}




TRoles rsrv_main_get_cpu_role(void)
{
  return  cpu_role;
//  return  RESERV_ROLES_MASTER;
}


void rsrv_main_set_cpu_udp_error 
(
    TThreeState   cpu_state
)
{
    TrsrvIntertaskMessage *msg;
    
    rsrv_os_lock(&McuViewPair.Sem);            
    McuViewPair.Local.CPUState = cpu_state;
    rsrv_os_unlock(&McuViewPair.Sem);

    if ( rsrv_os_mbox_accept(ethernet_main_mbox, &msg) == OS_ERR_NONE )    {
        OSTaskResume(TASK_RSRV_PRIORITY);
    }
}



/*=============================================================================================================*/

