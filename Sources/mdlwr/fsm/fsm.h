#ifndef _FSM_H_
#define _FSM_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "mdlwr/fsm/pt/pt.h"


/*=============================================================================================================*/

typedef void (*transition_worker)(int state, int signal);
typedef char (*pt_transition_worker)(struct pt *pt);


#if defined(__ICCCF__) || defined(__ICCARM__)
#pragma pack (push)
#pragma pack (4)
#endif

typedef struct FSM_TRANSITION_TABLE
{
    int                     new_state;
    transition_worker       worker;
} fsm_table_t ;


typedef struct FSMPTH_TRANSITION_TABLE
{
    int                     new_state;
    pt_transition_worker    worker;
} fsmpth_table_t ;



#if defined(__ICCCF__) || defined(__ICCARM__)
#pragma pack (pop)
#endif

/*=============================================================================================================*/

#define  WORK_OUT_STATIC_FSM_TABLE(state_type, ptr_state, func_get_signal, fsmtable) { \
        						int                         cur_signal;\
                                                        transition_worker           worker; \
                                                        \
							cur_signal = func_get_signal();\
                                                        worker = fsmtable[(int)*ptr_state][cur_signal].worker; \
                                                        if (worker != NULL) { \
                                                            worker((int)*ptr_state, cur_signal); \
                                                        } \
                                                        *ptr_state = (state_type)fsmtable[(int)*ptr_state][cur_signal].new_state; \
                                                      }


#define  WORK_OUT_FSM_TABLE(state_type, func_ptr_state, func_get_signal, fsmtable) { \
                                                        state_type                 *cur_state;\
                                                        int                         cur_signal;\
                                                        transition_worker           worker; \
                                                        \
                                                        cur_state  = func_ptr_state();\
                                                        cur_signal = func_get_signal();\
                                                        worker = fsmtable[(int)*cur_state][cur_signal].worker; \
                                                        if (worker != NULL) { \
                                                            worker((int)*cur_state, cur_signal); \
                                                        } \
                                                        *cur_state = (state_type)fsmtable[(int)*cur_state][cur_signal].new_state; \
                                                      }


#define  WORK_OUT_FSM_PTHTABLE(state_type, func_ptr_state, func_get_signal, func_pt_state, fsmpthtable) { \
                                                        state_type              *cur_state;\
                                                        int                     cur_signal;\
                                                        struct pt               *cur_pt;\
                                                        pt_transition_worker    worker;\
                                                        \
                                                        cur_state  = func_ptr_state();\
                                                        cur_signal = func_get_signal();\
                                                        cur_pt = func_pt_state();\
                                                        worker = fsmpthtable[(int)*cur_state][cur_signal].worker; \
                                                        if (worker != NULL) { \
                                                            PT_SCHEDULE( worker(cur_pt) ); \
                                                        } \
                                                        if ( *cur_state != (state_type)fsmpthtable[(int)*cur_state][cur_signal].new_state) {\
                                                          PT_INIT(cur_pt);\
                                                        }\
                                                        *cur_state = (state_type)fsmpthtable[(int)*cur_state][cur_signal].new_state; \
                                                      }


#define  WORK_OUT_PTHFSM_TABLE(state_type, func_ptr_state, pt_get_signal, gets_pt_state, fsmtable) {\
                                                        state_type              *cur_state;\
                                                        int                     cur_signal;\
                                                        struct pt               *signal_pt;\
                                                        transition_worker       worker;\
                                                        \
                                                        cur_state = func_ptr_state();\
                                                        signal_pt = gets_pt_state();\
                                                        if ( PT_SCHEDULE (pt_get_signal(signal_pt, &cur_signal)) == 0 ) { \
                                                            worker = fsmtable[(int)*cur_state][cur_signal].worker; \
                                                            if (worker != NULL) { \
                                                                worker((int)*cur_state, cur_signal); \
                                                            } \
                                                            *cur_state = (state_type)fsmtable[(int)*cur_state][cur_signal].new_state; \
                                                        }\
                                                        }\

#define FSM_VARNOUSE(a)        (a = a)



/*=============================================================================================================*/

#ifdef  __cplusplus
}
#endif

#endif  /* _FSM_H_ */


