#include <time.h>
#include "rsrv_os.h"
#include "rsrv_discr.h"
#include "reserv/core/inc/rsrv_typedef.h"
#include "reserv/core/inc/stmch.h"
#include "reserv/core/inc/rsrv_mcumcu_protocol_stmchn.h"
#include "reserv/core/inc/rsrv_decision.h"
#include "rsrv_backplane.h"


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
TrsrvOsMbox                     ethernet_main_mbox;
struct _MCU_VIEW_PAIR           McuViewPair;

/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void rsrv_mcumcu_protocol_start(void)
{
    TrsrvIntertaskMessage   msg;
    
    msg.sender_task  = RESERV_TASKS_ADRESS_MAIN;
    msg.recv_task    = RESERV_TASKS_ADRESS_MCUMCU;
    msg.msg_code     = RESERV_INTERCODES_MCUMCU_START;    
    rsrv_os_mbox_new(&mcumcu_main_mbox, &msg);  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void rsrv_ethernet_protocol_start(void)
{
    TrsrvIntertaskMessage   msg;
    
    msg.sender_task  = RESERV_TASKS_ADRESS_MAIN;
    msg.recv_task    = RESERV_TASKS_ADRESS_ETH;
    msg.msg_code     = RESERV_INTERCODES_MCUCPU_START;    
    rsrv_os_mbox_new(&ethernet_main_mbox, &msg);  
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
    
    do {
        retval = rsrv_os_mbox_fetch(&mcumcu_main_mbox, (void*)&msg, RSRV_MCUMCU_START_TIMEOUT);          
        if ( retval == 1 ) {
            break;
        } else if ( retval < 0 ) {
            break;
        }
    } while (     (msg->sender_task != RESERV_TASKS_ADRESS_MCUMCU) 
              ||  (msg->recv_task   != RESERV_TASKS_ADRESS_MAIN) 
              ||  (msg->msg_code    != RESERV_INTERCODES_MCUMCU_ENDDIAG) );        
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
    /* узнаем номер слота (резервный или основной) */
    McuViewPair.Local.RackNum = rsrv_get_dneprslotnum() ;
        
    if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT ) {
      McuViewPair.Remote.RackNum = RSRV_DNEPR_MAIN_SLOT;
    } else {
      McuViewPair.Remote.RackNum = RSRV_DNEPR_RESERVE_SLOT;
    }        
    
    /* определение сигнала present соседнего MCU */
    McuViewPair.Local.RemotePresent = rsrv_get_dneprpresent();
            
    /* инициализируем VLAN */
//    rsrv_vlan_config();
  
    /* инициализируем протоколы UART и ethernet */
    rsrv_mcumcu_protocol_start();
//    rsrv_ethernet_protocol_start();
    
    main_signal_transition = INIT_GOTO_DIAG;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_diag(int state, int signal)
{
    TRoles                mcu_role;
    TreservMCUDiagVector  mcu_diag_vector;
      
    resrv_wait_mcumcu_diag();   /*  ждем сообщения от автомата протокола MCUMCU, что диагностика сигнальныхлиний завершена */
    
    mcu_diag_vector.UARTTxRx = RESERV_TREESTATE_CHECKED;
  
    /* определяем состояние и роль соседнего CU, если он есть */  
    mcu_diag_vector.NeighborPresent = McuViewPair.Local.RemotePresent;
    mcu_diag_vector.NeighborRole    = McuViewPair.Remote.Role; 
    
    /* по составленной диагностике определяем какое состояние должно быть у MCU */
    mcu_diag_vector.SlotPosition = McuViewPair.Local.RackNum;
    mcu_role = rsrv_make_mcu_descision( &mcu_diag_vector );

    /* если пассивное переходим на passive state */
    if ( mcu_role != RESERV_ROLES_MASTER )  {
        main_signal_transition = DIAG_GOTO_PASSIVE;
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
    /* синхронизируем профиль в eeprom */
    rsrv_backplane_sync();
  
    /* выдаем на CPU роль стать активным */
    
    
  
    /* выбираем роль в зависимости от ответа */
    main_signal_transition = DIAG_GOTO_ACTIVE;       
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_passive(int state, int signal)
{
    /* подавляем любую активность на внешних шинах */
    rsrv_backplane_stop_access();
    
    /* выдаем световую индикацию */
    

  
    /* принимаем пинги по UARTу, отсылаем понги */
  
    /* если принимаем пинг с соседним "пассивным", переходим на диагностику CPU */
    
    /* если теряем сигнал present от активного и сигнал по uart, переходим на диагностику CPU   */
    
  
    return;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
static void stmch_active(int state, int signal)
{
    /* выдаем световую индикацию */
  
    /* работаем в обычном режиме, отсылаем пинги по уарту, принимаем понги */
  
    /* разрешаем работу всего шасси */
     rsrv_backplane_start_access();
    
  
    /* следим за работой CPU (ответами по uDP)если ответов нет в течении 3 сек меняем MCU на пассивное */
    {
        rsrv_backplane_stop_access();
    
        /* отсылаем "пинг" с пассивным сосстоянием */
    
        main_signal_transition = ACTIVE_GOTO_PASSIVE;
    }
    
  
    return;
}






/*=============================================================================================================*/


static void rsrv_mcuview_init (TmcuView *mcu_view)
{
    mcu_view->Adr = 0;
    mcu_view->COOLER  = RESERV_TREESTATE_UNKNOWN;
    mcu_view->CPUEth  = RESERV_TREESTATE_UNKNOWN;
    mcu_view->CPUrev  = 0;
    mcu_view->CPUState = RESERV_TREESTATE_UNKNOWN;
    mcu_view->DegradedMode = 0;
    mcu_view->EEPROM0 = RESERV_TREESTATE_UNKNOWN;
    mcu_view->EEPROM1 = RESERV_TREESTATE_UNKNOWN;
    mcu_view->ExtNetEth = RESERV_TREESTATE_UNKNOWN;
    mcu_view->I2CMCUComm = RESERV_TREESTATE_UNKNOWN;
    mcu_view->I2CShelfComm = RESERV_TREESTATE_UNKNOWN;
    mcu_view->IntNetEth = RESERV_TREESTATE_UNKNOWN;
    mcu_view->LARole = RESERV_ROLES_UNKNOWN;
    mcu_view->MCUEth = RESERV_TREESTATE_UNKNOWN;
    mcu_view->MCUFlash = RESERV_TREESTATE_UNKNOWN;
    mcu_view->MCUrev = 0;
    mcu_view->Phase = RESERV_MCU_NOT_STARTED;
    mcu_view->PWR = RESERV_TREESTATE_UNKNOWN;
    mcu_view->RackNum = RSRV_DNEPR_RESERVE_SLOT;
    mcu_view->RemotePresent = 0;
    mcu_view->Role = RESERV_ROLES_UNKNOWN;
    mcu_view->SwitchEth = RESERV_TREESTATE_UNKNOWN;
    mcu_view->SWITCHrev = 0;
    mcu_view->SwitchState = RESERV_TREESTATE_UNKNOWN;
    mcu_view->SysNetEth = RESERV_TREESTATE_UNKNOWN;
    mcu_view->UARTRx = RESERV_TREESTATE_UNKNOWN;
    mcu_view->UARTTx = RESERV_TREESTATE_UNKNOWN;
    mcu_view->UARTSwitchComm = RESERV_TREESTATE_UNKNOWN;
}


void  task_reserv_init(void)
{
    unsigned int adress = 1, present = 0;
    TrsrvIntertaskMessage   msg;
    
    msg.sender_task  = RESERV_TASKS_ADRESS_MAIN;
    msg.recv_task    = RESERV_TASKS_ADRESS_MAIN;
    msg.msg_code     = RESERV_INTERCODES_VOIDMSG;    
    rsrv_os_mbox_new(&mcumcu_main_mbox, &msg);

    msg.sender_task  = RESERV_TASKS_ADRESS_MAIN;
    msg.recv_task    = RESERV_TASKS_ADRESS_MAIN;
    msg.msg_code     = RESERV_INTERCODES_VOIDMSG;    
    rsrv_os_mbox_new(&ethernet_main_mbox, &msg);
    
    
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
        DO_STATE_MACHINE_TABLE(enum _STM_MCUMCU_STATES, rsrv_mcumcu_prtcl_get_currect_state, resrv_mcumcu_prtcl_recv_signal, rsrv_mcumcu_show_stchtbl)
    }
}





TRoles rsrv_main_get_cpu_role(void)
{
  return  RESERV_ROLES_MASTER;
}

void rsrv_main_set_cpu_udp_error (void)
{
  ;
}

/*=============================================================================================================*/

