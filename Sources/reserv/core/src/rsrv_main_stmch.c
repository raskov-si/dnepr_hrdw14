#include <time.h>
#include "rsrv_os.h"
#include "reserv/core/inc/rsrv_typedef.h"
#include "reserv/core/inc/stmch.h"
#include "reserv/core/inc/rsrv_mcumcu_protocol_stmchn.h"



#define UNUSED_ARG(arg) (void)*(arg);


static  struct _MCU_VIEW_PAIR   McuViewPair;



enum CU_STM_STATES
{
    state_initial = 0,
    state_preliminary_master,
    state_master,
};


enum _CU_STM_SIGNALS
{
    INITIAL_GOTO_INITIAL     = 0,
    INITIAL_GOTO_PREMASTER   = 1,

};


struct STM_TRANSITION  _cu_state_machine[1][2] =
{
    [state_initial][INITIAL_GOTO_INITIAL]   = {state_initial, NULL},
    [state_initial][INITIAL_GOTO_PREMASTER] = {state_preliminary_master, NULL},
};









// resrv_pong_is_receive_uart(*mcu_answer_local, *mcu_answer_remote)
// resrv_pong_is_receive_i2с(*mcu_answer_local, *mcu_answer_remote)

/* \todo переписать на состояния конечного автомата */

/* инициализация  */
/* отсылка пинга по уарту  */
/* отсылка пинга  по уарту  понга по i2c */
/* выбор предварительного ведущего */




static void reserv_perliminary_phase(void)
{

//    DO_STATE_MACHINE_TABLE (rsrv_mcumcu_prtcl_answ_get_currect_state, resrv_mcumcu_answ_prtcl_recv_signal, rsrv_mcumcu_answer_stchtbl)
//    DO_STATE_MACHINE_TABLE (rsrv_mcumcu_prtcl_get_currect_state, resrv_mcumcu_prtcl_recv_signal, rsrv_mcu_mcu_stchtbl)


//  TmcuView    answer_remote;
//  TmcuView    answer_local;

    /* инициализируем  прием команд и отсылку ответов */
    //resrv_init_pong_answer(*answer_remote, *answer_local, our_cu_adress,)
    // resrv_mcumcu_pong, resrv_pong_is_receive_uart, resrv_pong_is_receive_i2c);

    /* ждем отведенный таймаут перед началом отсылки пингов  */
    //resrv_wait_for_beginning(our_cu_adress);


    /* отсылаем пинг по MCU-MCU uart */
    /* проверяем получили ли понг во время таймаута */
//    resrv_mcumcu_ping(*mcu_answer_local, *mcu_answer_remote, our_cu_adress, timeout, try,  UART_ANSWER);

    /* синхронизируем данные с ответом если он таки есть */
//      resrv_sync_mcuview(*mcu_answer_local, *mcu_answer_remote);



    /* отсылаем "пинг i2c" - команду по уарту прислать назад ответ по i2c, если не получили ответ по uart у */
    /* проверяем получили ли понг по I2C во время таймаута */
//    resrv_mcumcu_ping(*mcu_answer_local, *mcu_answer_remote,  our_cu_adress, timeout, try, I2C_ANSWER );

    /* синхронизируем данные с ответом */
//      resrv_sync_mcuview(*mcu_answer_local, *mcu_answer_remote);


    /* принимаем решение о том что мы - предварительный ведущий или нет */
//    reserv_make_preliminary_role_descision()

}

static void rsrv_mcuview_init (TmcuView *mcu_view)
{
    mcu_view->Address = 0;
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
    mcu_view->RackNum = 0;
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
    unsigned int adress, present;

//    adress = rsrv_get_mcu_local_adress();
    //present = rsrv_get_remote_present();
    rsrv_os_lock_create(&McuViewPair.Sem);

    rsrv_os_lock(&McuViewPair.Sem);        
    rsrv_mcuview_init(&McuViewPair.Local);
    rsrv_mcuview_init(&McuViewPair.Remote);
    McuViewPair.Local.Address = adress;
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
        DO_STATE_MACHINE_TABLE (rsrv_mcumcu_prtcl_get_currect_state, resrv_mcumcu_prtcl_recv_signal, rsrv_mcu_mcu_stchtbl)
    }
}
