#include "rsrv_discr.h"

/*=============================================================================================================*/

#include "HAL/BSP/inc/T8_Dnepr_Select.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_LED.h"
#include "reserv/core/inc/rsrv_typedef.h"

/*=============================================================================================================*/

#define RSRV_DNEPR_SLOT_INDX      SELECT_SLOT_12         /*!< 13 сигнал select на XC-9 */
#define RSRV_DNEPR_PRESENT_INDX   12                     /*!< 13 сигнал present соседнего "днепра" на XC-9 */
      
/*=============================================================================================================*/


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
unsigned int rsrv_get_dneprslotnum (void)
{
  if ( dnepr_select_slot_read(RSRV_DNEPR_SLOT_INDX) == TRUE ) {
      return RSRV_DNEPR_MAIN_SLOT;
  }
  
  return RSRV_DNEPR_RESERVE_SLOT;
}


/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
unsigned int rsrv_get_dneprpresent (void)
{
  if ( dnepr_neighbor_present_read(RSRV_DNEPR_PRESENT_INDX) == TRUE ) {
      return RSRV_DNEPR_PRESSIGNAL_PRES;
  }
      
  return RSRV_DNEPR_PRESSIGNAL_NONE;
}


extern  struct _MCU_VIEW_PAIR   McuViewPair;
static T8_Dnepr_LedColorTypedef  cpuled_role_color;
/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
unsigned int rsrv_leds_setstate(TRoles  cur_role)
{
  if ( cur_role == RESERV_ROLES_MASTER ) {    
    cpuled_role_color = GREEN;
  } else {
    cpuled_role_color = YELLOW;
  }
  
  return (unsigned int)(RSRV_OK);
}


T8_Dnepr_LedColorTypedef get_now_led_state(void)
{
  if ( McuViewPair.Local.CPUState == RESERV_TREESTATE_DAMAGED ) {
      return RED;    
  }  
    return  cpuled_role_color;
}