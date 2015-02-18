#ifndef __DNEPR_TIME_DATE_H_
#define __DNEPR_TIME_DATE_H_

#ifdef	__cplusplus
    extern "C" {
#endif
      
#include <time.h>      

void timer_reset
(
  clock_t *timer_saved_value
);

clock_t timer_get_value
(
  clock_t *timer_saved_value
);

int timer_is_expired
(
  const clock_t *timer_saved_value, 
  clock_t       interval
);
      
      
      

#ifdef	__cplusplus
    }
#endif      
      


#endif  /* __DNEPR_TIME_DATE_H_ */
