#ifndef _RSRV_BACKPLANE_H_
#define _RSRV_BACKPLANE_H_

#ifdef	__cplusplus
    extern "C" {
#endif


/*=============================================================================================================*/

void  rsrv_backplane_sync(void);
void  rsrv_backplane_start_access(void);
void  rsrv_backplane_stop_access(void);

/*=============================================================================================================*/

#ifdef	__cplusplus
    }
#endif

#endif  /* _RSRV_OS_H_ */
      