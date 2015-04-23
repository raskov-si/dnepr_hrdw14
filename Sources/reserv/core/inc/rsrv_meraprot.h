#ifndef _RSRV_MERAPROT_H_
#define _RSRV_MERAPROT_H_

#ifdef	__cplusplus
    extern "C" {
#endif

/*=============================================================================================================*/
      
#include "reserv/core/inc/rsrv_typedef.h"
      
/*=============================================================================================================*/
      
#define ROLESELECTOR_RESULT_OK           0
#define ROLESELECTOR_RESULT_ERROR        1
      
#define ROLESELECTOR_ROLE_PASSIVE        0
#define ROLESELECTOR_ROLE_ACTIVE         1
      
#define MERAPROT_ROLECFM_LEN             20
      
#define MERAPROT_CMDCODE_OK              (0)
#define MERAPROT_CMDCODE_ERR_LEN         (-1)
#define MERAPROT_CMDCODE_ERR_COK         (-2)
#define MERAPROT_CMDCODE_ERR_ID          (-3)
#define MERAPROT_CMDCODE_ERR_DATA        (-4)
#define MERAPROT_CMDCODE_ERR_RESULT      (-5)
      
      
/*=============================================================================================================*/

enum _EN_MERAPROT_COMMANDID {      
      SetRoleReq      = 0,	/*!< Установить роль */
      SetRoleCfm      = 1,      /*!< Подтверждение */
      SynchronizeReq  = 2,      /*!< Синхронизировать данные */
      SynchronizeCfm  =	3,	/*!< Подтверждение   */
      SetUsersDataReq =	4,	/*!< Установить данные пользователей */
      SetUsersDataCfm =	5	/*!< Подтверждение */
};
      

struct  _STR_MERAPROT_COMMANDHEADER
{
    uint32_t  length;
//    uint32_t serviceID;
//    uint32_t servicePV;
    uint32_t  cookie;
    uint32_t  commandID;
    uint8_t   endData[];
};


struct  _STR_MERAPROT_COMMAND_SETROLE_REQ
{
    uint32_t role;
};


struct _STR_MERAPROT_COMMAND_SETROLE_CFM
{
    uint32_t result;
    uint32_t role;
};

/*=============================================================================================================*/

int meraprot_setrole_cmd
(
  uint8_t *payload,
  TRoles  current_cpu_role
);

int meraprot_setrole_cfm
(
  uint8_t *payload,
  TRoles  current_cpu_role  
);

void rsrv_main_set_cpu_udp_error (void);


      
      
/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_MERAPROT_H_ */

      