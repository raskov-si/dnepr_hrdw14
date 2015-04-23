#include "rsrv_backplane.h"
#include "Application/inc/T8_Dnepr_ProfileStorage.h"
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"


/*=============================================================================================================*/

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void rsrv_backplane_sync(void)
{
    Dnepr_BPEEPROM_Init() ;
    Dnepr_ProfileStorage_eeprom_sync() ;
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void  rsrv_backplane_start_access(void)
{
    denpr_eeprom_backplane_accessflag(TRUE);  
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void  rsrv_backplane_stop_access(void)
{
    denpr_eeprom_backplane_accessflag(FALSE); 
}
