#include "reserv/core/inc/rsrv_profile.h"
#include "Application/inc/T8_Dnepr_Profile_params.h"

/*=============================================================================================================*/

extern  struct _MCU_VIEW_PAIR   McuViewPair;

/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
u32 ovlpcumainrole_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
  
  //NotInstalled(0) /Active(1) /Passive(2)/Failed(3)  
  if ( McuViewPair.Local.RackNum == RSRV_DNEPR_MAIN_SLOT ) {
    if (McuViewPair.Local.CPUState == RESERV_TREESTATE_DAMAGED
//               || McuViewPair.Local.UARTRx == RESERV_TREESTATE_DAMAGED
              ) {
       pPar->value.U32 = 3;
       pPar->par_color = SYS_CRITICAL_COLOR ;        //SYS_MAJOR_COLOR            
    } else if ( McuViewPair.Local.Role == RESERV_ROLES_SLAVE ) {
       pPar->value.U32 = 2;
       pPar->par_color = SYS_MINOR_COLOR ;        //SYS_MAJOR_COLOR      
    } else if ( McuViewPair.Local.Role == RESERV_ROLES_MASTER ) {
       pPar->value.U32 = 1;
       pPar->par_color = SYS_MAJOR_COLOR ;        //SYS_MAJOR_COLOR      
    }
  
  } else if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT )  {
    if ( McuViewPair.Local.RemotePresent == RSRV_DNEPR_PRESSIGNAL_NONE ) {
       pPar->value.U32 = 0;
       pPar->par_color = SYS_NO_COLOR ;        //SYS_MAJOR_COLOR
    } else if (McuViewPair.Remote.CPUState == RESERV_TREESTATE_DAMAGED
               || McuViewPair.Remote.UARTRx == RESERV_TREESTATE_DAMAGED
              ) {
       pPar->value.U32 = 3;
       pPar->par_color = SYS_CRITICAL_COLOR ;        //SYS_MAJOR_COLOR            
    } else if ( McuViewPair.Remote.Role == RESERV_ROLES_SLAVE ) {
       pPar->value.U32 = 2;
       pPar->par_color = SYS_MINOR_COLOR ;        //SYS_MAJOR_COLOR      
    } else if ( McuViewPair.Remote.Role == RESERV_ROLES_MASTER ) {
       pPar->value.U32 = 1;
       pPar->par_color = SYS_MAJOR_COLOR ;        //SYS_MAJOR_COLOR      
    }
  } else {
    pPar->ready = 0;    
    return OK ;      
  }    
  
  pPar->ready = 1;
  
  return OK ;  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
u32 ovlpcursrvrole_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
  if ( McuViewPair.Local.RackNum == RSRV_DNEPR_MAIN_SLOT ) {
    if ( McuViewPair.Local.RemotePresent == RSRV_DNEPR_PRESSIGNAL_NONE ) {
       pPar->value.U32 = 0;
       pPar->par_color = SYS_NO_COLOR ;        
    } else if (McuViewPair.Remote.CPUState == RESERV_TREESTATE_DAMAGED
               || McuViewPair.Remote.UARTRx == RESERV_TREESTATE_DAMAGED
              ) {
       pPar->value.U32 = 3;
       pPar->par_color = SYS_CRITICAL_COLOR ;        //SYS_MAJOR_COLOR            
    } else if ( McuViewPair.Remote.Role == RESERV_ROLES_SLAVE ) {
       pPar->value.U32 = 2;
       pPar->par_color = SYS_MINOR_COLOR ;        //SYS_MAJOR_COLOR      
    } else if ( McuViewPair.Remote.Role == RESERV_ROLES_MASTER ) {
       pPar->value.U32 = 1;
       pPar->par_color = SYS_MAJOR_COLOR ;        //SYS_MAJOR_COLOR  
    }
    
  } else if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT )  {
    if (McuViewPair.Local.CPUState == RESERV_TREESTATE_DAMAGED
//               || McuViewPair.Local.UARTRx == RESERV_TREESTATE_DAMAGED
              ) {
       pPar->value.U32 = 3;
       pPar->par_color = SYS_CRITICAL_COLOR ;        //SYS_MAJOR_COLOR            
    } else if ( McuViewPair.Local.Role == RESERV_ROLES_SLAVE ) {
       pPar->value.U32 = 2;
       pPar->par_color = SYS_MINOR_COLOR ;        //SYS_MAJOR_COLOR      
    } else if ( McuViewPair.Local.Role == RESERV_ROLES_MASTER ) {
       pPar->value.U32 = 1;
       pPar->par_color = SYS_MAJOR_COLOR ;        //SYS_MAJOR_COLOR 
    }
  } else {
    pPar->ready = 0;    
    return OK ;      
  }    
    
  pPar->ready = 1;
  
  return OK ;  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
u32 ovlpcpumainstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
   pPar->value.U32 = 0;
   
  //(NoStatus(0)/Working(1) /Failed(2)
   
  if ( McuViewPair.Local.RackNum == RSRV_DNEPR_MAIN_SLOT ) {
      pPar->value.U32 = (uint32_t)McuViewPair.Local.CPUState;      
  } else if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT ) {
      pPar->value.U32 = (McuViewPair.Local.RemotePresent == RSRV_DNEPR_PRESSIGNAL_NONE)
                        ? 0 : (uint32_t)McuViewPair.Remote.CPUState;          
  }
    
  switch ( pPar->value.U32 )
  {
  case 1:     pPar->par_color = SYS_NORMAL_COLOR ;    break;
  case 2:     pPar->par_color = SYS_CRITICAL_COLOR ;  break;
  default:    pPar->par_color = SYS_NO_COLOR ;        break;
  }
    
  pPar->ready = 1;
  return OK ;  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
u32 ovlpmcumainstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
  
  if ( McuViewPair.Local.RackNum == RSRV_DNEPR_MAIN_SLOT ) {
      pPar->value.U32 = 1;      
  } else if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT ) {
          pPar->value.U32 = (McuViewPair.Local.RemotePresent == RSRV_DNEPR_PRESSIGNAL_NONE)
                        ? 0 : (uint32_t)McuViewPair.Remote.UARTTx;          
  }

  switch ( pPar->value.U32 )
  {
  case 1:     pPar->par_color = SYS_NORMAL_COLOR ;      break;
  case 2:     pPar->par_color = SYS_CRITICAL_COLOR ;    break;
  default:    pPar->par_color = SYS_NO_COLOR ;          break;
  }
    
  pPar->ready = 1;
  return OK ;  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
u32 ovlpcpureservstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
   pPar->value.U32 = 0;
   
  //(NoStatus(0)/Working(1) /Failed(2)
  if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT ) {
      pPar->value.U32 = (uint32_t)McuViewPair.Local.CPUState;      
  } else if ( McuViewPair.Local.RackNum == RSRV_DNEPR_MAIN_SLOT ) {
      pPar->value.U32 = (McuViewPair.Local.RemotePresent == RSRV_DNEPR_PRESSIGNAL_NONE)
                        ? 0 : (uint32_t)McuViewPair.Remote.CPUState;                    
  }

  switch ( pPar->value.U32 )
  {
  case 1:     pPar->par_color = SYS_NORMAL_COLOR ;      break;
  case 2:     pPar->par_color = SYS_CRITICAL_COLOR ;    break;
  default:    pPar->par_color = SYS_NO_COLOR ;          break;
  }
    
  
  pPar->ready = 1;
  return OK ;  
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
u32 ovlpmcureservstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
  if ( McuViewPair.Local.RackNum == RSRV_DNEPR_RESERVE_SLOT ) {
      pPar->value.U32 = 1;      
  } else if ( McuViewPair.Local.RackNum == RSRV_DNEPR_MAIN_SLOT ) {
          pPar->value.U32 = (McuViewPair.Local.RemotePresent == RSRV_DNEPR_PRESSIGNAL_NONE)
                        ? 0 : (uint32_t)McuViewPair.Remote.UARTTx;          
  }

  switch ( pPar->value.U32 )
  {
  case 1:     pPar->par_color = SYS_NORMAL_COLOR ;      break;
  case 2:     pPar->par_color = SYS_CRITICAL_COLOR ;    break;
  default:    pPar->par_color = SYS_NO_COLOR ;          break;
  }
    
  pPar->ready = 1;
  return OK ;  
}






