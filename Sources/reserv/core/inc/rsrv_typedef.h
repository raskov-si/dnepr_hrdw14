#ifndef _RESERV_TYPEDEF_H_
#define _RESERV_TYPEDEF_H_

#ifdef  __cplusplus
extern "C" {
#endif

/*=============================================================================================================*/

    #include <stdint.h>
    #include "Project_Headers\Binary.h"
    #include "rsrv_os.h"

/*=============================================================================================================*/
  
#define   RSRV_DNEPR_MAIN_SLOT     (0u)  
#define   RSRV_DNEPR_RESERVE_SLOT  (1u)  

#define   RSRV_DNEPR_PRESSIGNAL_NONE  (0u)  
#define   RSRV_DNEPR_PRESSIGNAL_PRES  (1u)  

  
/*=============================================================================================================*/
    enum EN_RESRV_RETURN_CODES {
            RSRV_OK         =  0,
            RSRV_TIMEOUT    = -1,
            RSRV_NOANSW     = -2,
            RSRV_CRC_ERR    = -3,
            RSRV_STRUCT_ERR = -4,
            RSRV_OS_ERR     = -5,
            RSRV_HRDVR_ERR  = -6
    };

    enum EN_RESERV_MCU_STATE {
        RESERV_MCU_NOT_STARTED,     /*! фаза до согласования предварительного ведущего */
        RESERV_MCU_PRELIMINARY,     /*! фаза выбора предварительного ведущего */
        RESERV_MCU_DECIDED          /*! фаза выбора окончательного ведущего   */
    };

    enum EN_RESERV_ROLES {
        RESERV_ROLES_UNKNOWN,       /*! роль не определена  */
        RESERV_ROLES_MASTER,        /*! роль ведущего       */
        RESERV_ROLES_SLAVE,         /*! роль ведомого       */
    };

    enum  EN_RESERV_TREESTATE {
        RESERV_TREESTATE_UNKNOWN = 0,   /*! значение диагностики на данный момент неизвестно    */
        RESERV_TREESTATE_CHECKED = 1,   /*! оборудование проверено и готово                     */
        RESERV_TREESTATE_DAMAGED = 2    /*! оборудование повреждено                             */      
    };
    
    enum EN_RESERV_TASKS_ADRESS {
        RESERV_TASKS_ADRESS_MAIN      = 0,
        RESERV_TASKS_ADRESS_MCUMCU    = 1,
        RESERV_TASKS_ADRESS_ETH       = 2,      
    };
    

    typedef enum  EN_RESERV_MCU_STATE   TPhases;
    typedef enum  EN_RESERV_ROLES       TRoles;
    typedef enum  EN_RESERV_TREESTATE   TThreeState;
    typedef unsigned int                TRevisionData;

#pragma pack (push)
#pragma pack (1)
    /*! \brief значение структуры при передаче данных записываются в формате little-endian  */
    struct RESERV_MCU_VIEW {
        unsigned int    Adr;            /*!         */
        TRoles          Role;
        TRoles          LARole;
        TThreeState     I2CMCUComm;
        TThreeState     I2CShelfComm;
        TThreeState     MCUFlash;
        TThreeState     UARTTx;
        TThreeState     UARTRx;
        TThreeState     EEPROM0;
        TThreeState     EEPROM1;
        TRevisionData   MCUrev;
        TRevisionData   CPUrev;
        TRevisionData   SWITCHrev;
        TThreeState     PWR;
        TThreeState     COOLER;
        TThreeState     UARTSwitchComm;
        TThreeState     MCUEth;
        TThreeState     CPUEth;
        TThreeState     SwitchEth;
        TThreeState     ExtNetEth;
        TThreeState     IntNetEth;
        TThreeState     SysNetEth;
        TThreeState     CPUState;
        TThreeState     SwitchState;
        unsigned int    RackNum;
        unsigned int    RemotePresent;
        TPhases         Phase;
        unsigned int    DegradedMode;
    };
#pragma pack (pop)
    
//    struct RESERV_MSG
//    {
//        enum EN_RESERV_TASKS_ADRESS       sender_task;
//        enum EN_RESERV_TASKS_ADRESS       recv_task;
//        enum EN_RESERV_INTERTASK_CODES    msg_code;
//    };


/*=============================================================================================================*/

    typedef struct RESERV_MCU_VIEW            TmcuView;

    struct _MCU_VIEW_PAIR {
        TmcuView        Local;
        TmcuView        Remote;
        TrsrvOsSem      Sem;
    };
    
    
    
/*=============================================================================================================*/
static inline void rsrv_mcuview_init (TmcuView *mcu_view)
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
    mcu_view->RemotePresent = RSRV_DNEPR_PRESSIGNAL_NONE;
    mcu_view->Role = RESERV_ROLES_UNKNOWN;
    mcu_view->SwitchEth = RESERV_TREESTATE_UNKNOWN;
    mcu_view->SWITCHrev = 0;
    mcu_view->SwitchState = RESERV_TREESTATE_UNKNOWN;
    mcu_view->SysNetEth = RESERV_TREESTATE_UNKNOWN;
    mcu_view->UARTRx = RESERV_TREESTATE_UNKNOWN;
    mcu_view->UARTTx = RESERV_TREESTATE_UNKNOWN;
    mcu_view->UARTSwitchComm = RESERV_TREESTATE_UNKNOWN;
}
    

/*=============================================================================================================*/

#ifdef  __cplusplus
}
#endif

#endif /* _RESERV_TYPEDEF_H_ */




