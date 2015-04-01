#ifndef _RSRV_I2C_H_
#define _RSRV_I2C_H_

#ifdef	__cplusplus
    extern "C" {
#endif

/*=============================================================================================================*/
      
    #include <stdint.h>
    #include <time.h>
      
      
/*=============================================================================================================*/

int rsrv_mcumcu_i2c_rx_init (void);
int rsrv_mcumcu_i2c_tx_init (void);
int rsrv_mcumcu_i2c_receive(uint8_t*, uint16_t*, uint16_t, clock_t);


/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_I2C_H_ */

