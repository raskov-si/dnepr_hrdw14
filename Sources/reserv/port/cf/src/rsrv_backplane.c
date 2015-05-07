#include "rsrv_backplane.h"
#include "Application/inc/T8_Dnepr_ProfileStorage.h"
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_I2C.h"


extern void Measure_reset_sfp_und_fans (void);

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
    //i2c крейт fan + power + slots 
    I2C_set_access();     
    Measure_reset_sfp_und_fans();
    
}

/*=============================================================================================================*/
/*!  \brief 

     \sa 
*/
/*=============================================================================================================*/
void  rsrv_backplane_stop_access(void)
{
    //i2c крейт fan + power + slots 
    I2C_clear_access();
    
    denpr_eeprom_backplane_accessflag(FALSE);
    //
    
}
