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
  pPar->value.U32 = 0;
  pPar->par_color = SYS_MINOR_COLOR ;        //SYS_MAJOR_COLOR
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
  pPar->value.U32 = 0;
  pPar->par_color = SYS_MINOR_COLOR ;          //SYS_MAJOR_COLOR
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
  pPar->par_color = SYS_NORMAL_COLOR ;     //SYS_CRITICAL_COLOR     
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
  pPar->value.U32 = 0;
  pPar->par_color = SYS_NORMAL_COLOR ;     //SYS_CRITICAL_COLOR     
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
  pPar->par_color = SYS_NORMAL_COLOR ;       //SYS_CRITICAL_COLOR  
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
  pPar->value.U32 = 0;
  pPar->par_color = SYS_NORMAL_COLOR ;       //SYS_CRITICAL_COLOR   
  pPar->ready = 1;
  return OK ;  
}






