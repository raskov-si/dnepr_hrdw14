/*!  \file thread_snmp.c
*    \brief     внешний интерфейс к модулю потока задач обслуживания сетевого протокола
*    \details   
*/

#ifndef THREAD_SNMP_H
#define	THREAD_SNMP_H

#ifdef	__cplusplus
    extern "C" {
#endif
      

      
void task_snmp( void *pdata );
     
      
 
      
#ifdef	__cplusplus
    }
#endif

#endif	/* THREAD_SNMP_H */