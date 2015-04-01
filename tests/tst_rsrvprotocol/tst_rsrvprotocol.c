#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include "reserv/core/inc/rsrv_mcumcu_protocol_stmchn.h"
#include "reserv/core/inc/rsrv_typedef.h"

struct _MCU_VIEW_PAIR   McuViewPair;

unsigned int rsrv_get_mcu_local_adress(void)
{
    return 1;
}

unsigned int rsrv_get_remote_present (void)
{
    return 1;
}


int main (int argc, char *argv[])
{
    /* init mcumcu */
    {
        unsigned int adress, present;
        adress = rsrv_get_mcu_local_adress();
        present = rsrv_get_remote_present();

        rsrv_os_lock_create(&McuViewPair.Sem);

        rsrv_os_lock(&McuViewPair.Sem);        
        McuViewPair.Remote.Address = 0;
        McuViewPair.Remote.RemotePresent = 1;
        McuViewPair.Local.Address = adress;
        McuViewPair.Local.RemotePresent = present;
        rsrv_os_unlock(&McuViewPair.Sem);
    }

    while ( 1 ) {
      DO_STATE_MACHINE_TABLE (rsrv_mcumcu_prtcl_get_currect_state, resrv_mcumcu_prtcl_recv_signal, rsrv_mcu_mcu_stchtbl)
      printf("protocol MCUMCU, table 0x%X, signal %u, state %u,\n", rsrv_mcu_mcu_stchtbl, rsrv_mcumcu_prtcl_get_currect_signal(), *(rsrv_mcumcu_prtcl_get_currect_state()));
    }

    return(0);
}

