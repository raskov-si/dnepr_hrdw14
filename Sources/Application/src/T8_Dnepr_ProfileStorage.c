/*!
\file T8_Dnepr_Profile.c
\brief ��� ��� ������-������ ���������� ������� �� ���� � EEPROM.
\author <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jan 2014
*/

#include "Application/inc/T8_Dnepr_ProfileStorage.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "Application/inc/T8_Dnepr_ProfileStorage.h"
#include "Application/inc/T8_Dnepr_Profile_params.h"
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_LED.h"
#include "HAL/BSP/inc/T8_Dnepr_filesystem.h"

#include <string.h>

#include "common_lib/memory.h"

#include "support_common.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"
#include "prio.h"

#include "pr_ch.h"

static void   ProfileStorage_SyncParam_thread( void *pdata );
static _BOOL  Dnepr_ProfileStorage_FS_Sync() ;
//static _BOOL  Dnepr_ProfileStorage_eeprom_sync() ;
static _BOOL  write_colors( const u8* param_name, u8 failure_color, u8 degrade_color, u8 normal_color);
static _BOOL  read_colors( const u8* param_name, u8* failure_color, u8* degrade_color, u8* normal_color );


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma data_alignment=4
static OS_STK  taskSyncParamThr[2048];

/*=============================================================================================================*/
/*! \brief ��������� ������� � eeprom � ������� ���������� �������.

    \details ���� ������� ���� ��� �� ����������� �����. 
    \details � ������� ����������� ������� ������� eeprom ��� �������������� ���������� ��� ���������� 
    \details ������ �������
    \sa  Dnepr_BPEEPROM_CheckPresent, T8_Dnepr_SetLedStatus
*/
/*=============================================================================================================*/
extern void __slot_power_onoff_init();

void  dnepr_wait_eeprom_contact 
(
    u32  time           /*!< [in] ����� � ������� �������� ������ ��������� ������� */
)
{     
    _BOOL   ret = FALSE;
    u32     time_begin  = OSTimeGet();
    u32     time_counter; 
    
    
    T8_Dnepr_LedStatusTypedef alarm_nocontact_leds = {
	    				                (T8_Dnepr_LedTypedef){RED, TRUE},
        						(T8_Dnepr_LedTypedef){RED, TRUE},
	        					(T8_Dnepr_LedTypedef){RED, TRUE}
                                                     };
    T8_Dnepr_LedStatusTypedef init_leds =            {
							(T8_Dnepr_LedTypedef){GREEN, TRUE},
        						(T8_Dnepr_LedTypedef){GREEN, TRUE},
	        					(T8_Dnepr_LedTypedef){GREEN, TRUE}
                                                     };
    T8_Dnepr_SetLedStatus( &alarm_nocontact_leds );
                   
    do  {
            ret = Dnepr_BPEEPROM_CheckPresent();
            time_counter = OSTimeGet();            
    } while ( (ret != TRUE) && ( (time_begin + time) > time_counter) );
   
    T8_Dnepr_SetLedStatus( &init_leds );            
   
    // ������ ��� EEPROM � HotSwap Controller'� � ����������� ��������� ���, �������
    // ��� �� ����������
    __slot_power_onoff_init() ;
        
}



void Dnepr_ProfileStorage_Init()
{
	INT8U return_code = OS_ERR_NONE;

	// �������������� �������� ������� � ����
	Dnepr_filesystem_Init() ;
	// ������ �� EEPROM ���������
//	Dnepr_BPEEPROM_Init() ;

	// ������������� ������� ������������ ���������� � sfp �� ������ ��� ��������.
	Dnepr_params_Init() ;
    // ������������� �������.
    SYS_Init() ;
    // ������������� ���������� �� ���������� �� ����.
    Dnepr_ProfileStorage_FS_Sync() ;
	// ������������� ���������� �� ���������� �� EEPROM Backplane'�.
//    Dnepr_ProfileStorage_eeprom_sync() ;
	// ����������� ��������� ���������� � ������������ � ����� ������.
	Dnepr_params_Init2_after_EEPROM();
    
	assert(OSTaskCreateExt(ProfileStorage_SyncParam_thread, (void *)0, (void *)&taskSyncParamThr[2047], SyncParam_prio, SyncParam_prio, (void *)&taskSyncParamThr, 2048, NULL, OS_TASK_OPT_STK_CHK ) == OS_ERR_NONE );
    OSTaskNameSet( SyncParam_prio, "SyncParam", &return_code ) ;
    assert( return_code == OS_ERR_NONE ) ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! ������ ���������� ��������� � taskCU

typedef struct __Dnepr_ProfileStorage_SyncParam_message {
	PARAM_INDEX *p_ix ;
	_BOOL write_colors ;
} Dnepr_ProfileStorage_SyncParam_message_t ;

#define threadPSync_messages_len 8
static Dnepr_ProfileStorage_SyncParam_message_t threadPSync_message[threadPSync_messages_len] ;
static size_t threadPSync_message_cur_num = 0 ;
static OS_EVENT  *__PSyncRcvQueue = 0;

static void Dnepr_ProfileStorage_SyncParam( PARAM_INDEX *p_ix )
{
	threadPSync_message[ threadPSync_message_cur_num ].p_ix = p_ix ;
	threadPSync_message[ threadPSync_message_cur_num ].write_colors = FALSE ;
	// ���� ��� ������� -- �� ����� � ���������
	if( __PSyncRcvQueue  ){
		OSQPost( __PSyncRcvQueue, (void*)&threadPSync_message[ threadPSync_message_cur_num ] ) ;
		if( ++threadPSync_message_cur_num >= threadPSync_messages_len ){
			threadPSync_message_cur_num = 0 ;
		}
	}
}

static void Dnepr_ProfileStorage_SyncColor( PARAM_INDEX *p_ix )
{
	threadPSync_message[ threadPSync_message_cur_num ].p_ix = p_ix ;
	threadPSync_message[ threadPSync_message_cur_num ].write_colors = TRUE ;
	// ���� ��� ������� -- �� ����� � ���������
	if( __PSyncRcvQueue  ){
		OSQPost( __PSyncRcvQueue, (void*)&threadPSync_message[ threadPSync_message_cur_num ] ) ;
		if( ++threadPSync_message_cur_num >= threadPSync_messages_len ){
			threadPSync_message_cur_num = 0 ;
		}
	}
}


static _BOOL    access_flag;

void denpr_eeprom_backplane_accessflag
(
  _BOOL   flag
)
{
    access_flag = flag;
}

static void ProfileStorage_SyncParam_thread(void *pdata)
{
	INT8U return_code = OS_ERR_NONE;
    void    *messages_array[threadPSync_messages_len] ; // ��������� ��� ������� ���������
    Dnepr_ProfileStorage_SyncParam_message_t *qCurMessage ;
    u8 buff[ PROFILESTORAGE_VAL_MAXLEN ]; // ����� ��� ��������. ���� ����� �� ������ ��� � � eeprom_storage.h
    u8 *par_name;
	u32 param_flag ;
    s32 param_len ;
    size_t param_max_len ;
    u32 iter_result = ERROR;

    pdata = pdata;      // ����� �� ���� warning'� � ���������������

    // ������� ��������� � ���� ����
    __PSyncRcvQueue = OSQCreate( messages_array, threadPSync_messages_len ) ;

	while(TRUE){
		// ��� ������� � ������ ��������� �� ��� ���������
		qCurMessage = (Dnepr_ProfileStorage_SyncParam_message_t*)OSQPend( __PSyncRcvQueue, 0, &return_code );

		// �������� ��� ������� �� ���� ������ � ����� �������� � eeprom � ����
		do{
			PARAM* param = (void*)qCurMessage->p_ix->parent;

			assert( return_code == OS_ERR_NONE );
			// �������� ��� ���������
			par_name = (u8*) SYSSetIndexByPtr( (void*)qCurMessage->p_ix->parent, 0, SYS_PARAM_NAME );
			if( par_name == NULL ){
				continue ;
			}
			// ���� ����� �����
			if( qCurMessage->write_colors ){
				if (param != NULL) {
					if (param->par_type == PR_DYNAMIC_PARAM) {
						u8* colors;
						if ((colors = param->colors_ptr) != NULL) {
							write_colors( par_name, colors[ 0 ], colors[ 1 ], colors[ 2 ] );
						}
					}
				}
				continue ;
			}
			param_max_len = qCurMessage->p_ix->parent->data_len ;
			// ������ �������� �� ���� � ����������� �� ����
			switch( qCurMessage->p_ix->parent->data_type ){
				case UINTEGER:
				case ENUM:
				case INTEGER:
				case HEX:
					iter_result = PROFILE_INTValueAccess( qCurMessage->p_ix, buff, PROFILESTORAGE_VAL_MAXLEN );
					param_len = 4 ;
					break ;
				case BOOL:
					iter_result = PROFILE_BOOLValueAccess( qCurMessage->p_ix, buff, PROFILESTORAGE_VAL_MAXLEN );
					param_len = 1 ;
					break ;
				case REAL:
					iter_result = PROFILE_REALValueAccess( qCurMessage->p_ix, buff, PROFILESTORAGE_VAL_MAXLEN );
					param_len = 4 ;
					break ;
				case CHARLINE:
				case OID:
				case IPADDR:
					if( (iter_result = PROFILE_CHARLINEValueAccess( qCurMessage->p_ix, buff, PROFILESTORAGE_VAL_MAXLEN ))
							== OK ){
						param_len = strnlen( (const char*)buff, param_max_len );
						if( (param_len < 0) || (param_len >= param_max_len)){
							iter_result = ERROR ;
						} else {
							++param_len ; // �������� ������ '\0'
						}
					}
					break ;
				default :
					assert( FALSE );
			}//switch TYPE
			// ������ �����������
			if( iter_result == OK ){
				// ���� ���� -- ����� � EEPROM �� backplane
				param_flag = SYSGetParamFlag( SYSGetParamID( qCurMessage->p_ix ) );
                                if ( access_flag == TRUE ) {
    				  if( (param_flag & EEPROM_PARAM) && (param_flag != ERROR) ){
					  Dnepr_BPEEPROM_Write( par_name, buff, param_len ) ;
				  }
                                }
				// ����� � �� �� ����
				Dnepr_filesystem_Write( par_name, buff, param_len );
			}
		} while( (qCurMessage = OSQAccept( __PSyncRcvQueue, &return_code )) );
		assert( (return_code == OS_ERR_NONE) || (return_code == OS_ERR_Q_EMPTY) );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern PARAM_INDEX param_ix[];

//! \brief �������� �� ���� ��������������� ����������, ���� ���� �� ���� � ���� ��������
//! �� �����.
static _BOOL Dnepr_ProfileStorage_FS_Sync()
{
	PARAM_INDEX *p_ix = param_ix;
	size_t p_id ;
	size_t new_params_cnt = 0 ;
	u8* colors;

	// �������� �� ���� ����������
	for( p_ix = param_ix, p_id = 0; p_ix->parent != NULL; p_ix++, p_id++ ){
		u32 param_flag ;
		u8 *par_name;
		size_t param_max_len, param_actual_len ;
		void* param_ram_addr;
		PARAM* param = SYSGetParentParam(p_id);

		// �������� ��� ���������
		par_name = (u8*) SYSSetIndexByPtr( (void*)p_ix->parent, 0, SYS_PARAM_NAME );
		if( par_name == NULL ){
			continue ;
		}

		param_flag = SYSGetParamFlag( p_id );
		// ���� � ��������� ���� ����� -- ������ �� �������� �� �����
		if (param != NULL) {
			if (param->par_type == PR_DYNAMIC_PARAM) {
				if ((colors = param->colors_ptr) != NULL) {
					u8 d, f, n;
					if( read_colors( par_name, &f, &d, &n ) ){
						colors[ 0 ] = f ;
						colors[ 1 ] = d ;
						colors[ 2 ] = n ;
					}
				}
			}
		}
		// ���� �������� �������� �� ���� ��� �� ��������������� -- ��������������
		// ��� �������� � ��� � ����.
		if( (((param_flag & EEPROM_PARAM) == 0) || (param_flag == ERROR)) && 
		   			((p_ix->parent->sec_level & CL_ALL_W) == 0) ){
			continue ;
		}
		// ���� �� ��� ������ �� ����� ���� ���������� ����������
		if( (p_ix->parent->sec_level & CL_ALL_RW) == 0 ){
			continue ;
		}
		
		param_max_len = p_ix->parent->data_len ;
		// �������� ����� �������� �� ���������� ����-������
		if( (param_ram_addr = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE)) == 0){
			continue ;
		}

		if( !Dnepr_filesystem_Read( par_name, param_ram_addr, &param_actual_len, param_max_len ) ){
			// �� ������� ��������� ����� ���� -- ����� ���
			Dnepr_filesystem_Write( par_name, param_ram_addr, param_max_len );
			new_params_cnt++ ;
		}
	}
	return TRUE ;
}

//! \brief �������� �� ���� ����������, ���� ��, ������� �����������
//! � eeprom �� backplane (���� ���� EEPROM_PARAM) � ��������������
//! ��������� �������� � eeprom. ���������� �� ������.
_BOOL Dnepr_ProfileStorage_eeprom_sync(void)
{
	_BOOL eeprom_success = FALSE ;
	PARAM_INDEX *p_ix = param_ix;
	u32 param_flag ;
	u32 result = OK ;
	u32 iter_result = ERROR;
	void* param_ram_addr;
	size_t eeprom_param_len ;
	s32 param_len ;
	size_t param_max_len ;
	size_t p_id ;
	u32 p_type ;
	u8 buff[ PROFILESTORAGE_VAL_MAXLEN ]; // ����� ��� ��������. ���� ����� �� ������ ��� � � eeprom_storage.h
	u8 *par_name;

	#ifdef __DEBUG
	return TRUE ;
	#endif

	// �������� �� ���� ����������
	for( p_ix = param_ix, p_id = 0; p_ix->parent != NULL; p_ix++, p_id++ ){
		param_flag = SYSGetParamFlag( p_id );
		// ���� ���� �������� �� ��������� � eeprom ���������� �������
		if( !(param_flag & EEPROM_PARAM) || ( param_flag == ERROR ) ){
			continue ;
		}
		// ���� �� ��� ������ �� ����� ���� ���������� ����������
		if( (p_ix->parent->sec_level & CL_ALL_RW) == 0 ){
			continue ;
		}
		// �������� ��� ���������
		par_name = (u8*) SYSSetIndexByPtr( (void*)p_ix->parent, 0, SYS_PARAM_NAME );
		if( par_name == NULL ){
			continue ;
		}
		param_max_len = p_ix->parent->data_len ;
		// ������ �������� �� EEPROM
		eeprom_success = Dnepr_BPEEPROM_Read( par_name, buff, &eeprom_param_len, PROFILESTORAGE_VAL_MAXLEN );
		// �������� ����� �������� �� ���������� ����-������
		param_ram_addr = SYS_setIndex((void*)p_ix->parent, 0, SYS_VALUE);
		if (param_ram_addr == NULL) {
			continue ;
		}
		p_type = SYSGetParamType(p_id);
		// ���� �������� �� ������� � EEPROM -- ����� ��� �������� �� ��� � EEPROM
		if( !eeprom_success ){
			// ������ ��� ��������
			// ����, ��� SYSGetParamValue �� ����� ����� ���������
			switch(p_type){
				case UINTEGER:
				case ENUM:
				case INTEGER:
				case HEX:
					iter_result = PROFILE_INTValueAccess(&param_ix[p_id], buff, PROFILESTORAGE_VAL_MAXLEN );
					param_len = 4 ;
					break ;
				case BOOL:
					iter_result = PROFILE_BOOLValueAccess(&param_ix[p_id], buff, PROFILESTORAGE_VAL_MAXLEN );
					param_len = 1 ;
					break ;
				case REAL:
					iter_result = PROFILE_REALValueAccess(&param_ix[p_id], buff, PROFILESTORAGE_VAL_MAXLEN );
					param_len = 4 ;
					break ;
				case CHARLINE:
				case OID:
				case IPADDR:
					iter_result = PROFILE_CHARLINEValueAccess(&param_ix[p_id], buff, PROFILESTORAGE_VAL_MAXLEN );
					param_len = strnlen( (const char*)buff, param_max_len );
					// �� ��� ����� �������� ��� �������������� 0 -- ���� ��� ��������
					if( (param_len < 0) || (param_len >= param_max_len) ){
						param_len = 1 ;
						buff[ 0 ] = 0 ;
						t8_memcopy( param_ram_addr, buff, param_len );
						// ������������ ����
						Dnepr_filesystem_Write( par_name, param_ram_addr, param_len );
					} else {
						++param_len ; // �������� ������ '\0'
					}
					break ;
			}//switch TYPE
			if( iter_result != OK ){
				result |= iter_result ;
				continue ;
			}
			if( !Dnepr_BPEEPROM_Write( par_name, buff, param_len ) ){
				result |= ERROR ;
			}
		// ����� ����� �������� �� EEPROM � ��� � ��
		} else {
			// ���� � EEPROM ���� �������� �������� ��� �������������� 0 ���
			// ���� � EEPROM ��� ������ ��������� ��������� ��� ���� � ������� 
			// ����� --
			// ������������ ���
			if( ((p_type == CHARLINE) || (p_type == OID) || (p_type == IPADDR)) &&
										 (buff[eeprom_param_len-1] != 0) ){
				eeprom_param_len = MIN( param_max_len, eeprom_param_len+1 );
				buff[ eeprom_param_len-1 ] = 0 ;
				if( !Dnepr_BPEEPROM_Write( par_name, buff, eeprom_param_len ) ){
					result |= ERROR ;
				}
			}
			param_len = MIN( param_max_len, eeprom_param_len );
			// ���� ������ �������� � eeprom �� ����� �������� �� ���� -- �����
			// �� ����
			if( memcmp(buff, param_ram_addr, param_len ) != 0 ){
				// ������������ � ��� � � ����
				t8_memcopy( param_ram_addr, buff, param_len );
				Dnepr_filesystem_Write( par_name, param_ram_addr, param_len );
			} else {
				result |= OK;
			}
		}
	}
	return result;
}	

static _BOOL write_colors( const u8* param_name, u8 failure_color, u8 degrade_color, u8 normal_color)
{
	u8 col_name[64];
	col_name[ 0 ] = 0 ;
	u8 cols[3] = { failure_color, degrade_color, normal_color };
	strcpy( (char*)col_name, (const char*)param_name );
	strncat( (char*)col_name, "_col", sizeof(col_name) );
	return Dnepr_filesystem_Write( col_name, cols, sizeof( cols ));
}

static _BOOL read_colors( const u8* param_name, u8* failure_color, u8* degrade_color, u8* normal_color )
{
	u8 col_name[64];
	col_name[ 0 ] = 0 ;
	u8 cols[3] ;
	size_t cols_len ;
	strcpy( (char*)col_name, (const char*)param_name );
	strncat( (char*)col_name, "_col", sizeof(col_name) );
	if( !Dnepr_filesystem_Read( col_name, cols, &cols_len, sizeof(cols) ) ){
		return FALSE ;
	}

	*failure_color = cols[ 0 ];
	*degrade_color = cols[ 1 ];
	*normal_color = cols[ 2 ];

	return TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 Dnepr_IPADDRValueUpdate(PARAM_INDEX* p_ix, void* buff)
{
	u32 param_flag ;
	u32 ret ;

	ret = PROFILE_IPADDRValueUpdate( p_ix, buff );
	if( ret == ERROR ){
		goto _EXIT ;
	}

	param_flag = SYSGetParamFlag( SYSGetParamID( p_ix ) );
	if( param_flag == ERROR ){
		goto _EXIT ;
	}
	Dnepr_ProfileStorage_SyncParam( p_ix );

_EXIT:
	return ret ;
}

u32 Dnepr_CHARLINEValueUpdate(PARAM_INDEX* p_ix, void* buff)
{
	u32 param_flag ;
	u32 ret ;

	ret = PROFILE_CHARLINEValueUpdate( p_ix, buff );
	if( ret == ERROR ){
		goto _EXIT ;
	}

	param_flag = SYSGetParamFlag( SYSGetParamID( p_ix ) );
	if( param_flag == ERROR ){
		goto _EXIT ;
	}
	Dnepr_ProfileStorage_SyncParam( p_ix );


_EXIT:
	return ret ;
}

u32 Dnepr_REALValueUpdate(PARAM_INDEX* p_ix, void* buff)
{
	u32 param_flag ;
	u32 ret ;

	ret = PROFILE_REALValueUpdate( p_ix, buff );
	if( ret == ERROR ){
		goto _EXIT ;
	}

	param_flag = SYSGetParamFlag( SYSGetParamID( p_ix ) );
	if( param_flag == ERROR ){
		goto _EXIT ;
	}
	Dnepr_ProfileStorage_SyncParam( p_ix );

_EXIT:	
	return ret ;
}

u32 Dnepr_INTValueUpdate(PARAM_INDEX* p_ix, void* buff)
{
	u32 param_flag ;
	u32 ret ;

	ret = PROFILE_INTValueUpdate( p_ix, buff );
	if( ret == ERROR ){
		goto _EXIT ;
	}

	param_flag = SYSGetParamFlag( SYSGetParamID( p_ix ) );
	if( param_flag == ERROR ){
		goto _EXIT ;
	}
	Dnepr_ProfileStorage_SyncParam( p_ix );

_EXIT:	
	return ret ;
}

u32 Dnepr_BOOLValueUpdate(PARAM_INDEX* p_ix, void* buff)
{
	u32 param_flag ;
	u32 ret ;

	ret = PROFILE_BOOLValueUpdate( p_ix, buff );
	if( ret == ERROR ){
		goto _EXIT ;
	}

	param_flag = SYSGetParamFlag( SYSGetParamID( p_ix ) );
	if( param_flag == ERROR ){
		goto _EXIT ;
	}
	Dnepr_ProfileStorage_SyncParam( p_ix );

_EXIT:	
	return ret ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 Dnepr_GetParamColors(u32 p_id, u8* failure_color, u8* degrade_color, u8* normal_color)
{
	u32 flag=ERROR;
	PARAM* param;
	u8* colors;
	param = SYSGetParentParam(p_id);
	if (param != NULL) {
		if (param->par_type == PR_DYNAMIC_PARAM) {
			if ((colors = param->colors_ptr) != NULL) {
				if (failure_color != NULL)
					*failure_color = *colors;
				if (degrade_color != NULL)
					*degrade_color = *(colors+1);
				if (normal_color != NULL)
					*normal_color = *(colors+2);
				flag = OK;
			}
		}
	}
	return flag;
}

u32 Dnepr_WriteParamColors(u32 p_id, u8 failure_color, u8 degrade_color, u8 normal_color)
{
	u32 flag=ERROR;
	PARAM* param;
	u8* colors;
	param = SYSGetParentParam(p_id);
	if (param != NULL) {
		if (param->par_type == PR_DYNAMIC_PARAM) {
			colors = param->colors_ptr;
			if (colors != NULL) {
				colors[0] = failure_color;
				colors[1] = degrade_color;
				colors[2] = normal_color;

				Dnepr_ProfileStorage_SyncColor( &param_ix[p_id] );
				flag = OK;
			}
		}
	}
	return flag;
}
