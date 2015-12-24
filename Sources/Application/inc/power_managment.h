/* Oct 23, 2015 */

#ifndef POWER_MANAGMENT_H_
#define POWER_MANAGMENT_H_

#ifdef	__cplusplus
extern "C" {
#endif

/*!  \file 	power_managment.h
 *   \brief  
 */

/*includes==========================================================================================================*/
#include <stdint.h>
#include "Profile/inc/prof_val.h"
#include "mdlwr/fsm/fsm.h"


/*defines===========================================================================================================*/


/*types=============================================================================================================*/
enum _POWER_STATES {
	POW_STATE_INIT = 0,			/*!< �������� ������� ��������� ������������ 	*/
	POW_STATE_GETSLOTSSTATE,	/*!< ������ �������� ����������  				*/
	POW_STATE_POWER_DELAY,
	POW_STATE_SOURCE_READ,
	POW_STATE_FAN_READ,
	POW_STATE_SLOTS_READ,
	POW_STATE_PASSIVE_CHECK,
	POW_STATE_FAN_ON,
	POW_STATE_POWEROFF_IGNORANT,
	POW_STATE_POWER_RESRV_CALC,
	POW_STATE_PROHIBITED,
	POW_STATE_SMARTSTART,
	POW_STATE_READY
};


/*prototypes========================================================================================================*/
extern const  fsm_table_t	powermang_trans_table[13][5];
extern enum _POWER_STATES       curnt_state;

int     pwmng_get_current_signal (void);
int     pwmng_is_slot_ready(int);
int     pwmng_is_slot_passive(int);
int     pwmng_get_power_status(int);
float   pwmng_get_power_limit(void);
int     pwmng_get_fan_status(void);



#ifdef	__cplusplus
}
#endif

#endif /* POWER_MANAGMENT_H_ */
