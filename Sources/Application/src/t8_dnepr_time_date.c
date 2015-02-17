
#include "Application/inc/t8_dnepr_time_date.h"

static time_t     time_value = 0;        /* отсчет секунд */
static clock_t    clock_value = 0;       /* отсчет тиков */


clock_t clock(void)
{
    return clock_value;
}


time_t time ( time_t *tptr )
{
    if (tptr != NULL)
    { *tptr = time_value;  }
    return time_value;
}


//__ATTRIBUTES __time32_t __time32(__time32_t *tptr)
//{
//    if (tptr != NULL)
//    { *tptr = time_value;  }
//    return time_value;
//}


