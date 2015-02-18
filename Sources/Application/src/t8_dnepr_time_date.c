#include "HAL/MCU/inc/T8_5282_timers.h"
#include "Application/inc/t8_dnepr_time_date.h"

volatile   long long   overload_pit_value = 0;       /* ������ ����� */

clock_t clock(void)
{
    uint32_t    timer_value = ( (uint32_t)overload_pit_value << 16) | (0xFFFF - pit_get_value(1));
    clock_t     clock_value = (clock_t)timer_value * PITTIME_MICROSECONDS_PER_TICK;
    
    return clock_value;
}


time_t time ( time_t *tptr )
{
    clock_t     time_value = (clock_t)(overload_pit_value * PITTIME_OVL_SECONDS);
    
    if (tptr != NULL)
    { *tptr = time_value;  }
    return time_value;
}




/*
===================================================================================================
Description: ��������� ���������� ������� �������
Returns    : 
Notes      : ������ ��������� �������� � ���������
===================================================================================================
*/
void timer_reset
(
  clock_t *timer_saved_value
)
{
  *timer_saved_value = clock();
}

/*
===================================================================================================
Description:
Returns    : 
Notes      :
===================================================================================================
*/
clock_t timer_get_value
(
  clock_t *timer_saved_value
)
{
  uint32_t nowtime = (uint32_t)clock();
  
  return (clock_t)( ((nowtime < (uint32_t)*timer_saved_value)) ? (0xFFFFFFFFu - (uint32_t)*timer_saved_value + nowtime) : (nowtime - (uint32_t)*timer_saved_value) );
}


/*
===================================================================================================
Description: �������� ��������� �������, ������������ �������� timer_set
Returns    : 1 - ������ �����, 0 - ������ �� �����
Notes      : 
===================================================================================================
*/
int timer_is_expired
(
  const clock_t *timer_saved_value, 
  clock_t       interval
)
{
  clock_t nowTicks = clock();
  clock_t nowInterval;
  
  if (*timer_saved_value > nowTicks) {
    /* �����������, �������� clock �� ������ ����� */
    nowInterval = 0xFFFFFFFFu - (uint32_t)*timer_saved_value;
    nowInterval += nowTicks;
  } else {
    nowInterval = (nowTicks - *timer_saved_value);
  }
  
  /* ���������� ������������� ��������, ���� ������ ����� */
  if (nowInterval > interval)
  { return 1; }
  
  return 0;
}




//__ATTRIBUTES __time32_t __time32(__time32_t *tptr)
//{
//    if (tptr != NULL)
//    { *tptr = time_value;  }
//    return time_value;
//}


