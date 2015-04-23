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
        RESERV_MCU_NOT_STARTED,     /*! ���� �� ������������ ���������������� �������� */
        RESERV_MCU_PRELIMINARY,     /*! ���� ������ ���������������� �������� */
        RESERV_MCU_DECIDED          /*! ���� ������ �������������� ��������   */
    };

    enum EN_RESERV_ROLES {
        RESERV_ROLES_UNKNOWN,       /*! ���� �� ����������  */
        RESERV_ROLES_MASTER,        /*! ���� ��������       */
        RESERV_ROLES_SLAVE,         /*! ���� ��������       */
    };

    enum  EN_RESERV_TREESTATE {
        RESERV_TREESTATE_UNKNOWN = 0,   /*! �������� ����������� �� ������ ������ ����������    */
        RESERV_TREESTATE_CHECKED = 1,   /*! ������������ ��������� � ������                     */
        RESERV_TREESTATE_DAMAGED = 2    /*! ������������ ����������                             */      
    };
    
    enum EN_RESERV_TASKS_ADRESS {
        RESERV_TASKS_ADRESS_MAIN      = 0,
        RESERV_TASKS_ADRESS_MCUMCU    = 1,
        RESERV_TASKS_ADRESS_ETH       = 2,      
    };
    
    enum EN_RESERV_INTERTASK_CODES {
        RESERV_INTERCODES_VOIDMSG         = 0,
        RESERV_INTERCODES_MCUMCU_START    = 1,
        RESERV_INTERCODES_MCUCPU_START    = 2,
        RESERV_INTERCODES_MCUMCU_ENDDIAG  = 3,
    };

    typedef enum  EN_RESERV_MCU_STATE   TPhases;
    typedef enum  EN_RESERV_ROLES       TRoles;
    typedef enum  EN_RESERV_TREESTATE   TThreeState;
    typedef unsigned int                TRevisionData;

#pragma pack (push)
#pragma pack (1)
    /*! \brief �������� ��������� ��� �������� ������ ������������ � ������� little-endian  */
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
    
    struct RESERV_MSG
    {
        enum EN_RESERV_TASKS_ADRESS       sender_task;
        enum EN_RESERV_TASKS_ADRESS       recv_task;
        enum EN_RESERV_INTERTASK_CODES    msg_code;
    };


/*=============================================================================================================*/

    typedef struct RESERV_MCU_VIEW      TmcuView;
    typedef struct RESERV_MSG           TrsrvIntertaskMessage;

    struct _MCU_VIEW_PAIR {
        TmcuView        Local;
        TmcuView        Remote;
        TrsrvOsSem      Sem;
    };

/*=============================================================================================================*/

#ifdef  __cplusplus
}
#endif

#endif /* _RESERV_TYPEDEF_H_ */




