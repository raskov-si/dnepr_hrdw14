
//_DLIB_THREAD_SUPPORT

/*
 * If _DLIB_MULTI_THREAD != 0, the following lock interface must be defined: 
 *
 *   typedef void *__iar_Rmtx;            // Lock info object
 *   void __iar_Mtxinit(__iar_Rmtx *);    // Initialize a lock
 *   void __iar_Mtxdst(__iar_Rmtx *);     // Destroy a lock
 *   void __iar_Mtxlock(__iar_Rmtx *);    // Lock a lock
 *   void __iar_Mtxunlock(__iar_Rmtx *);  // Unlock a lock
 * 
 * and the three once-initialization symbols must be defined (done in 
 * DLib_product.h):
 *
 *   _DLIB_THREAD_LOCK_ONCE_TYPE
 *   _DLIB_THREAD_LOCK_ONCE_MACRO(control_variable, init_function)
 *   _DLIB_THREAD_LOCK_ONCE_TYPE_INIT
 *
 * and, if an external TLS interface is used, the following must
 * be defined:
 *   typedef int __iar_Tlskey_t;
 *   typedef void (*__iar_Tlsdtor_t)(void *);
 *   int __iar_Tlsalloc(__iar_Tlskey_t *, __iar_Tlsdtor_t); 
 *                                                    // Allocate a TLS element
 *   int __iar_Tlsfree(__iar_Tlskey_t);               // Free a TLS element
 *   int __iar_Tlsset(__iar_Tlskey_t, void *);        // Set a TLS element
 *   void *__iar_Tlsget(__iar_Tlskey_t);              // Get a TLS element
 *
 */

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"
#include "Application/inc/DLib_prod.h"
//#include <DLib_Defaults.h>


// Initialize a lock
__ATTRIBUTES void __iar_Mtxinit  (__iar_Rmtx  *iar_lib_mutex)
{
    INT8U  return_code;
    
    iar_lib_mutex = iar_lib_mutex;    
    iar_lib_mutex = (__iar_Rmtx*)OSMutexCreate( IARLIB_MUTEX_PRIO, &return_code );
    assert( return_code == OS_ERR_NONE );
}


// Destroy a lock
__ATTRIBUTES void __iar_Mtxdst(__iar_Rmtx  *iar_lib_mutex)
{
    INT8U  return_code;
    
    OSMutexDel( (OS_EVENT*)iar_lib_mutex, OS_DEL_ALWAYS, &return_code);
    assert( return_code == OS_ERR_NONE );    
}

// Lock a lock
__ATTRIBUTES void __iar_Mtxlock(__iar_Rmtx *iar_lib_mutex)
{
    INT8U  return_code;
    
    OSMutexPend( (OS_EVENT*)iar_lib_mutex, 0, &return_code );        
    assert( return_code == OS_ERR_NONE );    
}

// Unlock a lock
__ATTRIBUTES void __iar_Mtxunlock(__iar_Rmtx *iar_lib_mutex)
{  
    OSMutexPost( (OS_EVENT*)iar_lib_mutex );
}
