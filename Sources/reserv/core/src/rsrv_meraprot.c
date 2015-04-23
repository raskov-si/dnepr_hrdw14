#include "reserv/core/inc/rsrv_meraprot.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/tcpip.h"


static uint32_t   current_cookie = 0;

int meraprot_setrole_cmd
(
  uint8_t *payload,
  TRoles  current_cpu_role
)
{
    int   len;
  
    struct  _STR_MERAPROT_COMMANDHEADER         *header = (struct  _STR_MERAPROT_COMMANDHEADER*)payload;
    struct  _STR_MERAPROT_COMMAND_SETROLE_REQ   *data   = (struct  _STR_MERAPROT_COMMAND_SETROLE_REQ*)&header->endData;
  
    
    switch ( current_cpu_role )
    {
    case RESERV_ROLES_MASTER:
      data->role = ROLESELECTOR_ROLE_ACTIVE;
      break;
      
    case RESERV_ROLES_UNKNOWN:
    case RESERV_ROLES_SLAVE:
      data->role = ROLESELECTOR_ROLE_PASSIVE;
      break;
    }
    
    data->role = htonl(data->role);
    
    header->commandID = SetRoleReq;
    header->cookie    = ++current_cookie;
    header->length = len = 12+4;
    
    header->commandID = htonl(header->commandID);
    header->cookie    = htonl(header->cookie);
    header->length    = htonl(header->length);
  
    return len;
}

int meraprot_setrole_cfm
(
  uint8_t *payload,
  TRoles  current_cpu_role  
)
{
    struct  _STR_MERAPROT_COMMANDHEADER         *header = (struct  _STR_MERAPROT_COMMANDHEADER*)payload;
    struct  _STR_MERAPROT_COMMAND_SETROLE_CFM   *data   = (struct  _STR_MERAPROT_COMMAND_SETROLE_CFM*)&header->endData;

    if ( header->length != MERAPROT_ROLECFM_LEN ) {
        return MERAPROT_CMDCODE_ERR_LEN;            
    }
    
    if ( header->cookie != current_cookie ) {
        return MERAPROT_CMDCODE_ERR_COK;      
    }
    
    if ( header->commandID != SetRoleCfm ) {
        return MERAPROT_CMDCODE_ERR_ID;      
    }
    
    if ( data->result != ROLESELECTOR_RESULT_OK ) {
        return MERAPROT_CMDCODE_ERR_RESULT;      
    }

    switch ( current_cpu_role )
    {
    case RESERV_ROLES_MASTER:
      if (data->role == ROLESELECTOR_ROLE_PASSIVE) {
        return MERAPROT_CMDCODE_ERR_DATA;      
      }
      break;
      
    case RESERV_ROLES_UNKNOWN:
    case RESERV_ROLES_SLAVE:
      if (data->role == ROLESELECTOR_ROLE_ACTIVE) {
        return MERAPROT_CMDCODE_ERR_DATA;      
      }
      break;
    }
      
    return MERAPROT_CMDCODE_OK;
}
