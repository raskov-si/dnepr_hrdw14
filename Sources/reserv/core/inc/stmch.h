#ifndef _STMCH_H_
#define _STMCH_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <assert.h>

/*=============================================================================================================*/

typedef void (*transition_worker)(int state, int signal);

#if defined(__ICCCF__) || defined(__ICCARM__)
#pragma pack (push)
#pragma pack (4)
#endif
struct STM_TRANSITION
{
    int                     new_state;
    transition_worker       worker;
};
#if defined(__ICCCF__) || defined(__ICCARM__)
#pragma pack (pop)
#endif

/*=============================================================================================================*/
#define  DO_STATE_MACHINE_TABLE(state_type, func_ptr_state, func_get_signal, table) { \
                                                        state_type                 *cur_state;\
                                                        int                         cur_signal;\
                                                        transition_worker           worker; \
                                                        \
                                                        assert(func_ptr_state != NULL);\
                                                        assert(func_get_signal != NULL);\
                                                        cur_state  = func_ptr_state();\
                                                        cur_signal = func_get_signal();\
                                                        assert(cur_state != NULL);\
                                                        worker = table[(int)*cur_state][cur_signal].worker; \
                                                        if (worker != NULL) { \
                                                            worker((int)*cur_state, cur_signal); \
                                                        } \
                                                        *cur_state = (state_type)table[(int)*cur_state][cur_signal].new_state; \
                                                      }

/*=============================================================================================================*/

#ifdef  __cplusplus
}
#endif

#endif  /* _RESERV_PROTOCOL_H_ */


