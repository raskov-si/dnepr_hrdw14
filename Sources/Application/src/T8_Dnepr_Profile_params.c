/*!
\file T8_Dnepr_params.c
\brief Инструменты для профильной системы и тела всех функций доступа к параметрам профиля.
\author <a href="mailto:baranovm@t8.ru">Mikhail Baranov</a>
\date june 2012
*/
#include <string.h>
#include <stdio.h>
#include "Threads/inc/threadMeasure.h"
#include "Threads/inc/threadCU.h"
#include "Threads/inc/threadDeviceController.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_Fans.h"
#include "HAL/IC/inc/PMB_interface.h"
#include <float.h>
#include "common_lib/crc.h"
#include "common_lib/memory.h"
#include "common_lib/CommonEEPROMFormat.h"
#include "T8_Atomiccode.h"
#include "Application/inc/T8_Dnepr_Profile_params.h"
#include "Application/inc/T8_Dnepr_ProfileStorage.h"
#include "Profile/inc/pr_ch.h"
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

u32	GL_PARAM_BUFF;			//index of param's buffer for CU
u32	APP_PARAM_BUFF;			//app param's buffer index
u32 GL_PARAM_BUFF_RDY[2];	//ready state of two param's buffer
P32	dyn_param[DYN_PAR_ALL][2];	//BUFFERED copy of DYNAMIC params
static taskCU_message_t	cu_message ;

// настройки вкл/выкл SFP из Profile/Generated/value.c
extern u32 val_CMSFP1TxEnable; // L - нижний
extern u32 val_CMSFP2TxEnable ; // U - верхний
extern u32 val_CMSFP1AutoNeg; // L - нижний
extern u32 val_CMSFP2AutoNeg ; // U - верхний

extern s8 val_FUVendor[17] ;
extern s8 val_FUHwNumberSet[33] ;
extern s8 val_FUPtNumber[33] ;
extern s8 val_FUSrNumber[33] ;
extern u32 val_FUFanNum ;
extern s8  val_FUFanModels[33] ;
extern u32 val_FUFanMaxLife ;
extern u32 val_FUFanMinRPM ;
extern u32 val_FUFanMaxRPM ;

static _BOOL Dnepr_NVParams_Init() ;
static void Dnepr_NVParams_Recalculate_CRC() ;

static void Dnepr_Params_Reload_SFP_Thresholds( const DNEPR_SFP_NUMBER sfp_id_ );
static void Dnepr_Params_Renew_SLOTSFans_num() ; // прячет FUFan?Speed в зависимости от val_VType

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! \note вызывается из потока threadCU
void Dnepr_params_Init()
{
	GL_PARAM_BUFF = 0;
	APP_PARAM_BUFF = 0;
	GL_PARAM_BUFF_RDY[0] = 0;
	GL_PARAM_BUFF_RDY[1] = 0;

	Dnepr_NVParams_Init(  );
//	// инициализируем SFP после включения
//	Dnepr_Measure_SFP_ChangeState( val_CMSFP1TxEnable == 1, val_CMSFP2TxEnable == 1 );
}

void Dnepr_params_Init2_after_EEPROM()
{
	// проверяем, сколько надо отображать вентиляторов
	Dnepr_Params_Renew_SLOTSFans_num() ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! Параметры, хранимые между разными загрузками и после пропадания питания (батарейка на плате)
struct tNonVolatileParams
{
	u32 boot_cnt ;
	u32 watchdog_resets_cnt ;
	u32 external_resets_cnt ;
	u32 power_on_resets_cnt ;
	u32 lowvoltage_resets_cnt ;

 	//! USER_SETUP, если пользователь сам поменял статические параметры sfp1, иначе SFP_SETUP
	SFP_PARAMS_SETUP_SOURCE sfp1_setup_source ;
	T8_SFP_STR16 sfp_1_vendor ;		//!< последний вендор sfp1, обновляется при загрузке и втыкании sfp1
	T8_SFP_STR16 sfp_1_sn;          		//!< последний серийный номер sfp1, обновляется при загрузке и втыкании sfp1
	T8_SFP_STR16 sfp_1_pn;				//!< последний партнамбер sfp1, обновляется при загрузке и втыкании sfp1
	T8_SFP_STR16 sfp_1_vendor_rev;		//!< последняя ревизия sfp1, обновляется при загрузке и втыкании sfp1

 	//! USER_SETUP, если пользователь сам поменял статические параметры sfp2, иначе SFP_SETUP
	SFP_PARAMS_SETUP_SOURCE sfp2_setup_source ; //!< USER_SETUP, если пользователь сам поменял статические параметры sfp2
	T8_SFP_STR16 sfp_2_vendor ;			//!< последний вендор sfp2, обновляется при загрузке и втыкании sfp2
	T8_SFP_STR16 sfp_2_sn ;        		//!< последний серийный номер sfp2, обновляется при загрузке и втыкании sfp2
	T8_SFP_STR16 sfp_2_pn ;				//!< последний партнамбер sfp2, обновляется при загрузке и втыкании sfp2
	T8_SFP_STR16 sfp_2_vendor_rev ;		//!< последняя ревизия sfp2, обновляется при загрузке и втыкании sfp2
};
_Pragma("location=\"nonvolatile_sram\"")
//! параметры, хранящиеся в ОЗУ, питаемой от батарейки на плате.
__no_init static struct tNonVolatileParams __nv_params ;

_Pragma("location=\"nonvolatile_sram\"")
//! если не совпадает c crc(__nv_params), значит не было инициализации -- инициализируем
__no_init static u32 nv_params_crc ;


static _BOOL Dnepr_NVParams_Init()
{
	_BOOL params_initialized ;
	// сумма не совпала, инициализируем
	if( Crc32( (u8*)&__nv_params, sizeof(struct tNonVolatileParams) ) != nv_params_crc ){
		t8_memzero( (u8*)&__nv_params, sizeof(struct tNonVolatileParams) );
		__nv_params.sfp1_setup_source = __nv_params.sfp2_setup_source = SFP_SETUP ;
		params_initialized = 1 ;
	// данные верные
	} else {
		__nv_params.boot_cnt++;
		if(MCF_RCM_RSR & MCF_RCM_RSR_WDR)
			__nv_params.watchdog_resets_cnt++;
		if(MCF_RCM_RSR & MCF_RCM_RSR_EXT)
			__nv_params.external_resets_cnt++;
		if(MCF_RCM_RSR & MCF_RCM_RSR_POR)
			__nv_params.power_on_resets_cnt++;
		if(MCF_RCM_RSR & MCF_RCM_RSR_LVD)
			__nv_params.lowvoltage_resets_cnt++;
		params_initialized = 0 ;
	}
	Dnepr_NVParams_Recalculate_CRC() ;
	return params_initialized ;
}

static void Dnepr_NVParams_Recalculate_CRC()
{
	nv_params_crc = Crc32( (u8*)&__nv_params, sizeof(struct tNonVolatileParams) );	
}

//! Обозначает, что текущие пороги sfp1 установлены пользователем
#define SFP1_SET_BY_USER() __nv_params.sfp1_setup_source = USER_SETUP; Dnepr_NVParams_Recalculate_CRC();
//! Обозначает, что текущие пороги sfp1 установлены пользователем
#define SFP2_SET_BY_USER() __nv_params.sfp2_setup_source = USER_SETUP; Dnepr_NVParams_Recalculate_CRC();

//! Обозначает, что текущие пороги sfp1 взяты из sfp
#define SFP1_SET_BY_SFP() __nv_params.sfp1_setup_source = SFP_SETUP; Dnepr_NVParams_Recalculate_CRC();
//! Обозначает, что текущие пороги sfp2 взяты из sfp
#define SFP2_SET_BY_SFP() __nv_params.sfp2_setup_source = SFP_SETUP; Dnepr_NVParams_Recalculate_CRC();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! Сбрасывает названия, серийные номера и пр. sfp, хранимые в озу под батарейкой, чтобы
//! после перезагрузки параметры считались из самих модулей. Нужно например перед 
//! перепрошивкой.
void Dnepr_Params_Cancel_sfp_thresholds()
{
	__nv_params.sfp_1_vendor.value[ 0 ] = 0 ;
	__nv_params.sfp_1_sn.value[ 0 ] = 0 ;
	__nv_params.sfp_1_pn.value[ 0 ] = 0 ;
	__nv_params.sfp_1_vendor_rev.value[ 0 ] = 0 ;

	__nv_params.sfp_2_vendor.value[ 0 ] = 0 ;
	__nv_params.sfp_2_sn.value[ 0 ] = 0 ;
	__nv_params.sfp_2_pn.value[ 0 ] = 0 ;
	__nv_params.sfp_2_vendor_rev.value[ 0 ] = 0 ;

	Dnepr_NVParams_Recalculate_CRC() ;
}

//! \brief вызывается при включении и перетыкании sfp, перечитывает пороги из обоих sfp, если нужно.
//! \note Вызывать из потока threadCU! Потому что запись во внутреннюю флеш ведём только в этом потоке.
//! \todo убрать копипаст
void Dnepr_Params_renew_sfp_thresholds()
{
	// если вставили новую sfp -- переписываем пороги из неё к себе в параметры профиля
	if( Dnepr_Measure_SFP_L_Info() ){
		// если sfp не соовпадает с той, с которой пороги были считаны
		if((strncmp(__nv_params.sfp_1_vendor.value, Dnepr_Measure_SFP_L_Info()->sfp_vendor.value, 16) != 0) ||
				  (strncmp(__nv_params.sfp_1_sn.value, Dnepr_Measure_SFP_L_Info()->sfp_sn.value, 16) != 0) ||
				  (strncmp(__nv_params.sfp_1_pn.value, Dnepr_Measure_SFP_L_Info()->sfp_pn.value, 16) != 0) ||
				  (strncmp(__nv_params.sfp_1_vendor_rev.value, Dnepr_Measure_SFP_L_Info()->sfp_vendor_rev.value, 16) != 0)){
			strncpy( __nv_params.sfp_1_vendor.value, Dnepr_Measure_SFP_L_Info()->sfp_vendor.value, 16 );
			__nv_params.sfp_1_vendor.value[16] = '\0' ;
			strncpy( __nv_params.sfp_1_sn.value, Dnepr_Measure_SFP_L_Info()->sfp_sn.value, 16 );
			__nv_params.sfp_1_sn.value[16] = '\0' ;
			strncpy( __nv_params.sfp_1_pn.value, Dnepr_Measure_SFP_L_Info()->sfp_pn.value, 16 );
			__nv_params.sfp_1_pn.value[16] = '\0' ;
			strncpy( __nv_params.sfp_1_vendor_rev.value, Dnepr_Measure_SFP_L_Info()->sfp_vendor_rev.value, 16 );
			__nv_params.sfp_1_vendor_rev.value[16] = '\0' ;
			Dnepr_NVParams_Recalculate_CRC() ; // обновили __nv_params -- надо пересчитать их crc

			Dnepr_Params_Reload_SFP_Thresholds(SFP_1);
		}
	} 
	if( Dnepr_Measure_SFP_U_Info() ){
		// если sfp не соовпадает с той, с которой пороги были считаны
		if((strncmp(__nv_params.sfp_2_vendor.value, Dnepr_Measure_SFP_U_Info()->sfp_vendor.value, 16) != 0) ||
			  (strncmp(__nv_params.sfp_2_sn.value, Dnepr_Measure_SFP_U_Info()->sfp_sn.value, 16) != 0) ||
			  (strncmp(__nv_params.sfp_2_pn.value, Dnepr_Measure_SFP_U_Info()->sfp_pn.value, 16) != 0) ||
			  (strncmp(__nv_params.sfp_2_vendor_rev.value, Dnepr_Measure_SFP_U_Info()->sfp_vendor_rev.value, 16) != 0)){
	 		strncpy( __nv_params.sfp_2_vendor.value, Dnepr_Measure_SFP_U_Info()->sfp_vendor.value, 16 );
		 	__nv_params.sfp_2_vendor.value[16] = '\0' ;
			strncpy( __nv_params.sfp_2_sn.value, Dnepr_Measure_SFP_U_Info()->sfp_sn.value, 16 );
			__nv_params.sfp_2_sn.value[16] = '\0' ;
			strncpy( __nv_params.sfp_2_pn.value, Dnepr_Measure_SFP_U_Info()->sfp_pn.value, 16 );
			__nv_params.sfp_2_pn.value[16] = '\0' ;
			strncpy( __nv_params.sfp_2_vendor_rev.value, Dnepr_Measure_SFP_U_Info()->sfp_vendor_rev.value, 16 );
			__nv_params.sfp_2_vendor_rev.value[16] = '\0' ;
			Dnepr_NVParams_Recalculate_CRC() ; // обновили __nv_params -- надо пересчитать их crc

			Dnepr_Params_Reload_SFP_Thresholds(SFP_2);
		}
	}
}

SFP_PARAMS_SETUP_SOURCE Dnepr_Params_SFP1_Setup_source()
{
	return __nv_params.sfp1_setup_source ;
}

SFP_PARAMS_SETUP_SOURCE Dnepr_Params_SFP2_Setup_source()
{
	return __nv_params.sfp2_setup_source ;
}


extern PARAM_INDEX param_ix[];

//! \brief перечитываем параметры из SFP и сохраняем во флеш в профильные пороги
//! \param sfp_id_ номер sfp (1 -- нижний, 2 -- верхний)
//! \note Запускается из потока профиля, работающего с внутренней флеш.
static void Dnepr_Params_Reload_SFP_Thresholds( const DNEPR_SFP_NUMBER sfp_id_ )
{
	if((sfp_id_ == SFP_1) && (Dnepr_Measure_SFP_L_Info())){
		SFP1_SET_BY_SFP();
		// пишем пороги входной мощности нижней sfp
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PinCMin],
							(void*)&Dnepr_Measure_SFP_L_Info()->rx_pwr_ths.la_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PinWMin],
							(void*)&Dnepr_Measure_SFP_L_Info()->rx_pwr_ths.lw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PinWMax],
							(void*)&Dnepr_Measure_SFP_L_Info()->rx_pwr_ths.hw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PinCMax],
							(void*)&Dnepr_Measure_SFP_L_Info()->rx_pwr_ths.ha_threshold );
		// пишем пороги выходной мощности нижней sfp
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PoutCMin],
							(void*)&Dnepr_Measure_SFP_L_Info()->tx_pwr_ths.la_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PoutWMin],
							(void*)&Dnepr_Measure_SFP_L_Info()->tx_pwr_ths.lw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PoutWMax],
							(void*)&Dnepr_Measure_SFP_L_Info()->tx_pwr_ths.hw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP1PoutCMax],
							(void*)&Dnepr_Measure_SFP_L_Info()->tx_pwr_ths.ha_threshold );
	} else if(sfp_id_ == SFP_2){
		SFP2_SET_BY_SFP();
		Dnepr_NVParams_Recalculate_CRC();
		// пишем пороги входной мощности нижней sfp
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PinCMin],
							(void*)&Dnepr_Measure_SFP_U_Info()->rx_pwr_ths.la_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PinWMin],
							(void*)&Dnepr_Measure_SFP_U_Info()->rx_pwr_ths.lw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PinWMax],
							(void*)&Dnepr_Measure_SFP_U_Info()->rx_pwr_ths.hw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PinCMax],
							(void*)&Dnepr_Measure_SFP_U_Info()->rx_pwr_ths.ha_threshold );
		// пишем пороги выходной мощности нижней sfp
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PoutCMin],
							(void*)&Dnepr_Measure_SFP_U_Info()->tx_pwr_ths.la_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PoutWMin],
							(void*)&Dnepr_Measure_SFP_U_Info()->tx_pwr_ths.lw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PoutWMax],
							(void*)&Dnepr_Measure_SFP_U_Info()->tx_pwr_ths.hw_threshold );
		Dnepr_REALValueUpdate( &param_ix[CMSFP2PoutCMax],
							(void*)&Dnepr_Measure_SFP_U_Info()->tx_pwr_ths.ha_threshold );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// описывает какие параметры скрывать в каком крейте
// 0 -- показывать параметр, 1 -- скрывать
static u32 __show_hide_params_id[][6] = {
	//	Колво слотов	1 		3		7 		13	13 горизонтальный
	{FUFan3Speed,		0,		1, 		0,		0,	0},
	{FUFan4Speed,		0,		1, 		0,		0,	0},
	{FUFan5Speed,		0,		1, 		1,		0,	0},
	{FUFan6Speed,		0,		1, 		1,		0,	0},
	{Slot2PowerStatus,	1,		0, 		0,		0,	0},
	{Slot2Name,	 	1,		0, 		0,		0,	0},
	{Slot2Power,	 	1,		0, 		0,		0,	0},
	{Slot3PowerStatus,	1,		0, 		0,		0,	0},
	{Slot3Name,	 	1,		0, 		0,		0,	0},
	{Slot3Power,	 	1,		0, 		0,		0,	0},
	{Slot4PowerStatus,	1,		1, 		0,		0,	0},
	{Slot4Name,	 	1,		1, 		0,		0,	0},
	{Slot4Power,	 	1,		1, 		0,		0,	0},
	{Slot5PowerStatus,	1,		1, 		0,		0,	0},
	{Slot5Name,	 	1,		1, 		0,		0,	0},
	{Slot5Power,	 	1,		1, 		0,		0,	0},
	{Slot6PowerStatus,	1,		1, 		0,		0,	0},
	{Slot6Name,	 	1,		1, 		0,		0,	0},
	{Slot6Power,	 	1,		1, 		0,		0,	0},
	{Slot7PowerStatus,	1,		1, 		0,		0,	0},
	{Slot7Name,	 	1,		1, 		0,		0,	0},
	{Slot7Power,	 	1,		1, 		0,		0,	0},
	{Slot8PowerStatus,	1,		1, 		1,		0,	0},
	{Slot8Name,	 	1,		1, 		1,		0,	0},
	{Slot8Power,	 	1,		1, 		1,		0,	0},
	{Slot9PowerStatus,	1,		1, 		1,		0,	0},
	{Slot9Name,	 	1,		1, 		1,		0,	0},
	{Slot9Power,	 	1,		1, 		1,		0,	0},
	{Slot10PowerStatus,	1,		1, 		1,		0,	0},
	{Slot10Name,	 	1,		1, 		1,		0,	0},
	{Slot10Power,	 	1,		1, 		1,		0,	0},
	{Slot11PowerStatus,	1,		1, 		1,		0,	0},
	{Slot11Name,	 	1,		1, 		1,		0,	0},
	{Slot11Power,	 	1,		1, 		1,		0,	0},
	{Slot12PowerStatus,	1,		1, 		1,		0,	0},
	{Slot12Name,	 	1,		1, 		1,		0,	0},
	{Slot12Power,	 	1,		1, 		1,		0,	0},
	{Slot13PowerStatus,	1,		1, 		1,		0,	0},
	{Slot13Name,	 	1,		1, 		1,		0,	0},
	{Slot13Power,	 	1,		1, 		1,		0,	0},
	{Slot2Enable,	 	1,		0, 		0,		0,	0},
	{Slot3Enable,	 	1,		0, 		0,		0,	0},
	{Slot4Enable,	 	1,		1, 		0,		0,	0},
	{Slot5Enable,	 	1,		1, 		0,		0,	0},
	{Slot6Enable,	 	1,		1, 		0,		0,	0},
	{Slot7Enable,	 	1,		1, 		0,		0,	0},
	{Slot8Enable,	 	1,		1, 		1,		0,	0},
	{Slot9Enable,	 	1,		1, 		1,		0,	0},
	{Slot10Enable,	 	1,		1, 		1,		0,	0},
	{Slot11Enable,	 	1,		1, 		1,		0,	0},
	{Slot12Enable,	 	1,		1, 		1,		0,	0},
	{Slot13Enable,	 	1,		1, 		1,		0,	0},
	{Slot2,			1,		0,		0,		0,	0},
	{Slot3,			1,		0,		0,		0,	0},
	{Slot4,			1,		1,		0,		0,	0},
	{Slot5,			1,		1,		0,		0,	0},
	{Slot6,			1,		1,		0,		0,	0},
	{Slot7,			1,		1,		0,		0,	0},
	{Slot8,			1,		1,		1,		0,	0},
	{Slot9,			1,		1,		1,		0,	0},
	{Slot10,		1,		1,		1,		0,	0},
	{Slot11,		1,		1,		1,		0,	0},
	{Slot12,		1,		1,		1,		0,	0},
	{Slot13,		1,		1,		1,		0,	0},
	{Slot14,		1,		1,		1,		0,	0},
	{PS1OutCurrent,	0,		1,		1,		1,	1},
	{PS2OutCurrent,	0,		1,		1,		1,	1},

	// конец массива
	{0,			0,		0, 		0,		0} 
};

extern u32 val_VType ;
// прячет FUFan?Speed в зависимости от val_VType
static void Dnepr_Params_Renew_SLOTSFans_num()
{
	u32 i ;
	// 1 слот 6 вентиляторов
	if( val_VType == 5 ){
		for( i = 0; __show_hide_params_id[i][0] != 0; ++i ){
			SYS_hide_param( __show_hide_params_id[i][0],
					__show_hide_params_id[i][1] );
		}
	}
	// 3 слота, 2 вентилятора
	if( (val_VType == 2) || (val_VType == 10) ){
		for( i = 0; __show_hide_params_id[i][0] != 0; ++i ){
			SYS_hide_param( __show_hide_params_id[i][0],
					__show_hide_params_id[i][2] );
		}
	}
	// 7 слотов, 4 вентилятора
	if( (val_VType == 4) || (val_VType == 9) ){
		for( i = 0; __show_hide_params_id[i][0] != 0; ++i ){
			SYS_hide_param( __show_hide_params_id[i][0],
					__show_hide_params_id[i][3] );
		}
	}
	// 13 слотов, 6 вентиляторов
	if( (val_VType == 3) || (val_VType == 8) ){
		for( i = 0; __show_hide_params_id[i][0] != 0; ++i ){
			SYS_hide_param( __show_hide_params_id[i][0],
					__show_hide_params_id[i][4] );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// одна процедура для чтения всех динамических параметров
// вызывается из sys.c, из потока, в котором обрабатываются строки профиля (threadCU)
// сами данные снимаются в потоке threadMeasure
u32 dyn_par_access(PARAM_INDEX* p_ix, void* buff, u32 len){
	//return value of DYNAMIC section param from GLOBAL BUFF
	s8* pText;
	u32 dyn_param_num = (u32)p_ix->parent->local_num-1;
	u32 p_id   = SYSGetParamID(p_ix);//(u32)p_ix->parent->global_num-1;
	u32 p_type = SYSGetParamType(p_id);
	if (p_type == ERROR)
		return p_type;
	if(	GL_PARAM_BUFF_RDY[GL_PARAM_BUFF] &&
		dyn_param_num < DYN_PAR_ALL &&
		dyn_param[dyn_param_num][GL_PARAM_BUFF].ready &&
		len >= 4){//call from SYSGetParamValueStr(,,8)
		switch (p_type){
			case REAL:
				*(f32*)buff = dyn_param[dyn_param_num][GL_PARAM_BUFF].value.F32;
				break;
			case BOOL:
				*(u8*)buff = (u8)dyn_param[dyn_param_num][GL_PARAM_BUFF].value.U32;
				break;
			case CHARLINE:
			case OID:
			case IPADDR:
				pText = dyn_param[dyn_param_num][GL_PARAM_BUFF].value.text;
				if( strnlen( pText, len ) >= 0 ){
					strcpy(buff,pText);
					return OK;
				}
				return ERROR;
			default:
				*(u32*)buff = (u32)dyn_param[dyn_param_num][GL_PARAM_BUFF].value.U32;
				break;
		}//switch
		return OK;	
	}
	return ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// процедура для чтения значения динамического параметра из буфера со стороны профиля
// вызывается из sys.c, из потока, в котором вызываются *_getvalue() (threadMeasure),
// когда заполняется значение цвета параметра
u32 app_get_dyn_param_val_appbuf(u32 p_id, void* buff, u32 len){
	//return value of DYNAMIC section params from APP BUFF
	PARAM_INDEX* p_ix = SYSGetParamIx(p_id);
	u32 dyn_param_num = p_id - DynamicSect-1;//(u32)p_ix->parent->local_num-1;
	if(	dyn_param_num < DYN_PAR_ALL &&
		dyn_param[dyn_param_num][APP_PARAM_BUFF].ready &&
		len >= 4){
		*(u32*)buff = (u32)dyn_param[dyn_param_num][APP_PARAM_BUFF].value.U32;
		return OK;
	}
	return ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// процедура читает значение цвета дин. параметра
// вызывается из sys.c, из потока профиля (threadCU), когда выполняется команда 0x77
u32 app_get_dyn_param_color_glbuf(u32 p_id){
	//return color of DYNAMIC section params from GLOBAL BUFF
	PARAM_INDEX* p_ix = SYSGetParamIx(p_id);
	u32 dyn_param_num = p_id - DynamicSect-1;//(u32)p_ix->parent->local_num-1;
	if(GL_PARAM_BUFF_RDY[GL_PARAM_BUFF] &&
			dyn_param_num < DYN_PAR_ALL){
		return dyn_param[dyn_param_num][GL_PARAM_BUFF].par_color;
	}
	return ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// тела функций доступа ко всем параметрам профиля
// надо бы разбить на файлы

// вспомогательная функция, копирует строку в buff
static void __copy_str_2_buff( const s8* src_, s8* dst_, const u32 max_len_ )
{
	u32 len, i ;
	len = strlen( src_ )+1 ;
	len = MIN( len, max_len_ );
	if( len == 0 ){
		*((s8*)dst_) = 0 ;
	} else {
		strncpy( dst_, src_, len - 1 );
		((s8*)dst_)[len - 1] = 0 ;	
		for( i = 0; i < len - 1; ++i ){
			s8 ch = ((s8*)dst_)[i] ;
			// если символ непечатоемый (C0-FF -- кириллица CP1251)
			// или символ @ или ; -- заменяем на 0
			if( (ch < 0x20) || ((ch > 0x7e) && (ch < 0xC0)) 
				|| (ch == '@') || (ch == ';') ){
				// заменяем на символ 0
				((s8*)dst_)[i] = 0x30 ;
			}
		}
	}
}

#include "version"
u32 cmswhash_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	__copy_str_2_buff( sVerion, buff, buff_len );
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// счетчики загрузок

u32 cmrebootcnt_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*(u32*)buff = 0xFFFFFFFF ;
	return OK ;
}

u32 cmmcurebootcnt_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*(u32*)buff = __nv_params.boot_cnt ;
	return OK;
}

u32 cmmcuwatchdogrebootcnt_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*(u32*)buff = __nv_params.watchdog_resets_cnt ;
	return OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SFP 

u32 cmsfpautoneg_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) 
{
  if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR ) {
	return ERROR ;	
  }
  return OK ;  
}


u32 cmsfpautoneg_update(PARAM_INDEX* p_ix, void* buff) 
{
    u32 mode_value;
    
    if( Dnepr_INTValueUpdate( p_ix, buff ) == ERROR ) {
        return ERROR ;
    }
    
    mode_value = *((u32*)p_ix->parent->value_ptr);
    dnepr_measure_SFP_change_autoneg_mode(p_ix->parent->owner+1, (u8)mode_value);
  
    return OK ;  
}


u32 cmsfpthrreset_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	*(u32*)buff = 1 ; // на чтение всегда OFF
	return OK;
}

// сброс порогов sfp номер #OWNER
u32 cmsfpthrreset_update(PARAM_INDEX* p_ix, void* buff) {
	if( *(u32*)buff != 0 ){
		return OK ;
	}
	if(p_ix->parent->owner == 0)
		Dnepr_Params_Reload_SFP_Thresholds( SFP_1 );
	if(p_ix->parent->owner == 1)
		Dnepr_Params_Reload_SFP_Thresholds( SFP_2 );
	return OK ;
}


u32 cmsfptxenable_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}


u32 cmsfptxenable_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_INTValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	
	Dnepr_Measure_SFP_ChangeState( val_CMSFP1TxEnable == 1, val_CMSFP2TxEnable == 1 );
	return OK ;
}


// SFP 1 vendor name
u32 cmsfp1vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	const T8_SFP_OPTICAL_CHANNEL* pSFP1_info = 	Dnepr_Measure_SFP_L_Info() ;
	if( !pSFP1_info )
		return ERROR ; // не вставили

	__copy_str_2_buff( pSFP1_info->sfp_vendor.value,
		(s8*)buff, MIN( 16, buff_len ) ) ;
	return OK;
}

// SFP 1 part number provided by vendor
u32 cmsfp1ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const T8_SFP_OPTICAL_CHANNEL* pSFP1_info = 	Dnepr_Measure_SFP_L_Info() ;
	if( !pSFP1_info )
		return ERROR ; // не вставили

	__copy_str_2_buff( pSFP1_info->sfp_pn.value,
		(s8*)buff, MIN( 16, buff_len ) ) ;
	return OK;
}

// SFP 1 vendor serial number
u32 cmsfp1srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const T8_SFP_OPTICAL_CHANNEL* pSFP1_info = 	Dnepr_Measure_SFP_L_Info() ;
	if( !pSFP1_info )
		return ERROR ; // не вставили

	__copy_str_2_buff( pSFP1_info->sfp_sn.value,
		(s8*)buff, MIN( 16, buff_len ) ) ;
	return OK;	
}

// SFP 1 laser wavelength
u32 cmsfp1wl_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const T8_SFP_OPTICAL_CHANNEL* pSFP1_info = 	Dnepr_Measure_SFP_L_Info() ;
	if( !pSFP1_info )
		return ERROR ; // не вставили
	*(u32*)buff = pSFP1_info->sfp_wl ;
	return OK ;
}

u32 cmsfp1rev_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	if( Dnepr_Measure_SFP_L_Info() == NULL )
		return ERROR ;

	__copy_str_2_buff( Dnepr_Measure_SFP_L_Info()->sfp_vendor_rev.value,
		(s8*)buff, MIN(4, buff_len) );
	return OK ;
}

u32 cmsfp1ddmstate_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	if(!Dnepr_Measure_SFP_L_Info())
		return ERROR ;
	*(u32*)buff = Dnepr_Measure_SFP1_DDM_Compliance();
	return OK ;
}

u32 cmsfp1pincmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1pincmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}


u32 cmsfp1pinwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1pinwmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}


u32 cmsfp1pinwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1pinwmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}


u32 cmsfp1pincmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1pincmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}


u32 cmsfp1poutcmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1poutcmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}


u32 cmsfp1poutwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1poutwmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}


u32 cmsfp1poutwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1poutwmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}


u32 cmsfp1poutcmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len );
}

u32 cmsfp1poutcmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP1_SET_BY_USER();
	return OK ;
}

// SFP 2 vendor name
u32 cmsfp2vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const T8_SFP_OPTICAL_CHANNEL* pSFP2_info = 	Dnepr_Measure_SFP_U_Info() ;
	if( !pSFP2_info )
		return ERROR ; // не вставили

	__copy_str_2_buff( pSFP2_info->sfp_vendor.value,
		(s8*)buff, MIN( 16, buff_len ) ) ;
	return OK;	
}

// SFP 2 part number provided by vendor
u32 cmsfp2ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const T8_SFP_OPTICAL_CHANNEL* pSFP2_info = 	Dnepr_Measure_SFP_U_Info() ;
	if( !pSFP2_info )
		return ERROR ; // не вставили

	__copy_str_2_buff( pSFP2_info->sfp_pn.value,
		(s8*)buff, MIN( 16, buff_len ) ) ;
	return OK;	
}

// SFP 2 vendor serial number
u32 cmsfp2srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const T8_SFP_OPTICAL_CHANNEL* pSFP2_info = 	Dnepr_Measure_SFP_U_Info() ;
	if( !pSFP2_info )
		return ERROR ; // не вставили

	__copy_str_2_buff( pSFP2_info->sfp_sn.value,
		(s8*)buff, MIN( 16, buff_len ) ) ;
	return OK;	
}

// SFP 2 laser wavelength
u32 cmsfp2wl_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const T8_SFP_OPTICAL_CHANNEL* pSFP2_info = 	Dnepr_Measure_SFP_U_Info() ;
	if( !pSFP2_info )
		return ERROR ; // не вставили
	*(u32*)buff = pSFP2_info->sfp_wl ;
	return OK ;
}

u32 cmsfp2rev_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	if( Dnepr_Measure_SFP_U_Info() == NULL )
		return ERROR ;

	__copy_str_2_buff( Dnepr_Measure_SFP_U_Info()->sfp_vendor_rev.value,
		(s8*)buff, MIN(4, buff_len) );
	return OK ;
}

u32 cmsfp2ddmstate_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	if(!Dnepr_Measure_SFP_U_Info())
		return ERROR ;
	*(u32*)buff = Dnepr_Measure_SFP1_DDM_Compliance();
	return OK ;
}

u32 cmsfp2pincmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2pincmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


u32 cmsfp2pinwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2pinwmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


u32 cmsfp2pinwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2pinwmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


u32 cmsfp2pincmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2pincmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


u32 cmsfp2poutcmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2poutcmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


u32 cmsfp2poutwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2poutwmax_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


u32 cmsfp2poutwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2poutwmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


u32 cmsfp2poutcmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	return PROFILE_REALValueAccess( p_ix, buff, buff_len ) ;
}

u32 cmsfp2poutcmin_update(PARAM_INDEX* p_ix, void* buff) {
	if( Dnepr_REALValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	SFP2_SET_BY_USER();
	return OK ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// параметры блоков питания

u32 ps1vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;
	if( pPSUInfo ){
		__copy_str_2_buff( pPSUInfo->sManufacturer, buff, PSU_MFR_MAXLEN );
		return OK ;
	} else {
		return ERROR ;
	}
}

u32 ps1ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;
	if( pPSUInfo ){
		__copy_str_2_buff( pPSUInfo->sModel, buff, PSU_MODEL_MAXLEN );
		return OK ;
	} else {
		return ERROR ;
	}
}

u32 ps1power_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;
	if( pPSUInfo ){
		*((u32*)buff) = (u32)pPSUInfo->fPower ;
		return OK ;
	} else {
		return ERROR ;
	}
}


u32 ps1srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
#define PSU_SRLEN_NUM 32

	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;

	if( pPSUInfo ){
		__copy_str_2_buff( Dnepr_Backplane_GetPSU_Info(0)->sUniqueSerial, buff, PSU_SRLEN_NUM );
		return OK ;
	} else {
		return ERROR ;
	}
#undef PSU_SRLEN_NUM
}

u32 ps2vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;
	if( pPSUInfo ){
		__copy_str_2_buff( pPSUInfo->sManufacturer, buff, PSU_MFR_MAXLEN );
		return OK ;
	} else {
		return ERROR ;
	}
}

u32 ps2ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;
	if( pPSUInfo ){
		__copy_str_2_buff( pPSUInfo->sModel, buff, PSU_MODEL_MAXLEN );
		return OK ;
	} else {
		return ERROR ;
	}
}

u32 ps2power_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;
	if( pPSUInfo ){
		*((u32*)buff) = (u32)pPSUInfo->fPower ;
		return OK ;
	} else {
		return ERROR ;
	}
}


u32 ps2srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
#define PSU_SRLEN_NUM 32
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;

	if( pPSUInfo ){
		__copy_str_2_buff( Dnepr_Backplane_GetPSU_Info(1)->sUniqueSerial, buff, PSU_SRLEN_NUM );
		return OK ;
	} else {
		return ERROR ;
	}

#undef PSU_SRLEN_NUM
}

/*! \todo доделать доступ к  */
u32 ps3vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
    const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;
    if( pPSUInfo ){
	  __copy_str_2_buff( pPSUInfo->sManufacturer, buff, PSU_MFR_MAXLEN );
        return OK ;
    } else {
	return ERROR ;
    }  
}


u32 ps3ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
  const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;
  if( pPSUInfo ){
	__copy_str_2_buff( pPSUInfo->sModel, buff, PSU_MODEL_MAXLEN );
	return OK ;
  } else {
	return ERROR ;
  }  
}


u32 ps3power_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;
	if( pPSUInfo ){
		*((u32*)buff) = (u32)pPSUInfo->fPower ;
		return OK ;
	} else {
		return ERROR ;
	}    
}

u32 ps3srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
#define PSU_SRLEN_NUM 32

	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(0) ;

	if( pPSUInfo ){
		__copy_str_2_buff( Dnepr_Backplane_GetPSU_Info(0)->sUniqueSerial, buff, PSU_SRLEN_NUM );
		return OK ;
	} else {
		return ERROR ;
	}
#undef PSU_SRLEN_NUM
}

u32 ps4vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
    const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;
    if( pPSUInfo ){
	  __copy_str_2_buff( pPSUInfo->sManufacturer, buff, PSU_MFR_MAXLEN );
        return OK ;
    } else {
	return ERROR ;
    }    
}

u32 ps4ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
  const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;
  if( pPSUInfo ){
	__copy_str_2_buff( pPSUInfo->sModel, buff, PSU_MODEL_MAXLEN );
	return OK ;
  } else {
	return ERROR ;
  }  
}

u32 ps4power_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;
	if( pPSUInfo ){
		*((u32*)buff) = (u32)pPSUInfo->fPower ;
		return OK ;
	} else {
		return ERROR ;
	}      
}

u32 ps4srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
#define PSU_SRLEN_NUM 32

	const PSU_UnitInfoTypedef* pPSUInfo = Dnepr_Backplane_GetPSU_Info(1) ;

	if( pPSUInfo ){
		__copy_str_2_buff( Dnepr_Backplane_GetPSU_Info(1)->sUniqueSerial, buff, PSU_SRLEN_NUM );
		return OK ;
	} else {
		return ERROR ;
	}
#undef PSU_SRLEN_NUM  
}







u32 pswreeprom_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*((u32*)buff) = 0 ;
	return OK ;
}

extern s8  val_PSVendorSet[33] ;
extern s8  val_PSPtNumberSet[33] ;
extern u32 val_PSPowerSet ;
extern s8  val_PSSrNumberSet[33] ;
// Прошиваем содержимое EEPROM БП
u32 pswreeprom_update(PARAM_INDEX* p_ix, void* buff)
{
	// проверяем, что это V1
	if( val_VType != 5 ){
		return ERROR ;
	}

	Dnepr_UpdatePSUEEPROM( (*(u32*)buff)-1, val_PSVendorSet, val_PSPtNumberSet,
													val_PSPowerSet, val_PSSrNumberSet );
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// про EEPROM backplane'а

u32 vformateeprom_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*((u32*)buff) = 0 ;
	return OK ;
}

u32 vformateeprom_update(PARAM_INDEX* p_ix, void* buff)
{
	cu_message.message_type = FORMAT_BPEEPROM ;

	if( *((u32*)buff) == 1 ){
		CU_send_queue_message( &cu_message );
	}
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Плата вентиляции

u32 fuvendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		__copy_str_2_buff( (s8*)Dnepr_Measure_Fans_Calib()->vendor, (s8*)buff, buff_len );
		return OK ;
	}
}

u32 fuvendor_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	__copy_str_2_buff( buff, (s8*)Dnepr_Measure_Fans_Calib()->vendor, DNEPR_FANS_VENDOR_LEN );
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;	
}

u32 fufanmodels_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		__copy_str_2_buff( (s8*)Dnepr_Measure_Fans_Calib()->fans_partnumber, (s8*)buff, buff_len );
		return OK ;
	}
}

u32 fufanmodels_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	__copy_str_2_buff( buff, (s8*)Dnepr_Measure_Fans_Calib()->fans_partnumber, DNEPR_FANS_PARTNUM_LEN );
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}

u32 fufanmaxlife_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		*(u32*)buff = Dnepr_Measure_Fans_Calib()->max_work_hrs ;
		return OK ;
	}
}

u32 fufanmaxlife_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	Dnepr_Measure_Fans_Calib()->max_work_hrs = *(u32*)buff ;
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}

u32 fufanminrpm_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		*(u32*)buff = Dnepr_Measure_Fans_Calib()->min_rpm ;
		return OK ;
	}
}

u32 fufanminrpm_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	Dnepr_Measure_Fans_Calib()->min_rpm = *(u32*)buff ;
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}

u32 fufanmaxrpm_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		*(u32*)buff = Dnepr_Measure_Fans_Calib()->max_rpm ;
		return OK ;
	}
}

u32 fufanmaxrpm_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	Dnepr_Measure_Fans_Calib()->max_rpm = *(u32*)buff ;
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}

u32 fufannum_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		*(u32*)buff = Dnepr_Measure_Fans_Calib()->fans_num ;
		return OK ;
	}
}

u32 fufannum_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	Dnepr_Measure_Fans_Calib()->fans_num = *(u32*)buff;
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}


u32 fuhwnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		__copy_str_2_buff( (s8*)Dnepr_Measure_Fans_Calib()->hw_number, (s8*)buff, buff_len );
		return OK ;
	}
}

u32 fuhwnumber_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	__copy_str_2_buff( buff, (s8*)Dnepr_Measure_Fans_Calib()->hw_number, DNEPR_FANS_HW_LEN );
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}

u32 fuptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		__copy_str_2_buff( (s8*)Dnepr_Measure_Fans_Calib()->pt_number, (s8*)buff, buff_len );
		return OK ;
	}
}

u32 fuptnumber_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	__copy_str_2_buff( buff, (s8*)Dnepr_Measure_Fans_Calib()->pt_number, DNEPR_FANS_PT_LEN );
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}

u32 fusrnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	} else {
		__copy_str_2_buff( (const s8*)Dnepr_Measure_Fans_Calib()->sr_number, buff, buff_len );
		return OK ;
	}
}

u32 fusrnumber_update(PARAM_INDEX* p_ix, void* buff)
{
	if( (!I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM]) ){
		return ERROR ;
	}
	__copy_str_2_buff( buff, (s8*)Dnepr_Measure_Fans_Calib()->sr_number, DNEPR_FANS_SR_LEN );
	Dnepr_Measure_Fans_Calib_Update();
	return OK ;
}


u32 fufanspeedset_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

u32 fufanspeedset_update(PARAM_INDEX* p_ix, void* buff)
{
	if( *(u32*)buff > 100 ){
		*(u32*)buff = 100 ;
	}
	Dnepr_INTValueUpdate( p_ix,	buff );
	Dnepr_DControl_SetFans() ;
	return OK ;
}

extern u32 val_FUFanMode ;
extern u32 val_FUFanThreshold75 ;
extern u32 val_FUFanThreshold90 ;

u32 fufanmode_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

u32 fufanmode_update(PARAM_INDEX* p_ix, void* buff)
{
	Dnepr_INTValueUpdate( p_ix,	buff );
	Dnepr_DControl_SetFans() ;
	return OK ;
}

u32 fufanthreshold75_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

u32 fufanthreshold75_update(PARAM_INDEX* p_ix, void* buff)
{
	if( *(u32*)buff > val_FUFanThreshold90 ){
		return ERROR ;
	}
	Dnepr_INTValueUpdate( p_ix,	buff );
	if( !val_FUFanMode ){
		Dnepr_DControl_SetFans() ;
	}
	return OK ;
}

u32 fufanthreshold90_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

u32 fufanthreshold90_update(PARAM_INDEX* p_ix, void* buff)
{
	if( *(u32*)buff < val_FUFanThreshold75 ){
		return ERROR ;
	}
	Dnepr_INTValueUpdate( p_ix,	buff );
	if( !val_FUFanMode ){
		Dnepr_DControl_SetFans() ;
	}
        return OK ;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 vpowerlimitsource_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

extern u32 val_VPowerLimit ;
extern u32 val_VPowerLimitSource ;
u32 vpowerlimitsource_update(PARAM_INDEX* p_ix, void* buff)
{
	if( Dnepr_INTValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;

	Dnepr_DControl_SetPowerLimitSource( val_VPowerLimitSource == 0, (f32)val_VPowerLimit );
	return OK ;
}

u32 vpowerlimit_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

u32 vpowerlimit_update(PARAM_INDEX* p_ix, void* buff)
{
	if( Dnepr_INTValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;

	Dnepr_DControl_SetPowerLimitSource( val_VPowerLimitSource == 0, (f32)(val_VPowerLimit < 1 ? FLT_MAX : val_VPowerLimit) );
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern u32 val_VType ;
u32 countslots_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	// VType;VB8U3@2|VB8U10@3
	switch( val_VType ){
		// 3 слота
		case 2 :
			*((u32*)buff) = 3 ;
		break;
		// 7 слотов, 4 вентилятора
		case 4 :
			*((u32*)buff) = 7 ;
		break ;
		// 13 слотов
		case 3 :
			*((u32*)buff) = 13 ;
		break ;
		default :
			return ERROR ;
	}
	return OK ;
}

u32 vtype_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

u32 vtype_update(PARAM_INDEX* p_ix, void* buff)
{
	if( Dnepr_INTValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;

	Dnepr_Params_Renew_SLOTSFans_num()	;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern u32 val_SlotPowerSet ;
extern s8  val_SlotNameSet[16] ;
extern f32 val_Slot12v0Bypass ;
extern u32 val_Slot12v0OCAR ;
extern u32 val_Slot12v0UVAR ;
extern u32 val_Slot12v0OVAR ;
extern f32 val_Slot3v3Bypass ;
extern u32 val_Slot3v3OCAR ;
extern u32 val_Slot3v3UVAR ;
extern u32 val_Slot3v3OVAR ;

static SlotHSEEPROMParams_t __eeprom_params ;

// записать настройки в слотовое устройство
u32 slotwreeprom_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*(u32*)buff = 0 ;
	return OK ;
}

u32 slotwreeprom_update(PARAM_INDEX* p_ix, void* buff)
{
	u32 slot_num = *(u32*)buff ;
	slot_num -= 1 ;
	if( slot_num > (SPI_FPGA_SLOTS_NUMBER-1)){
		return OK ;
	}
	if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[slot_num] ){
		return OK ;
	}
	__eeprom_params.version = SLOTEEPROM_DATA_VERSION ;
	__eeprom_params.dMaxPower = val_SlotPowerSet ;
	strncpy( __eeprom_params.sName, val_SlotNameSet, SLOTEEPROM_SLOTNAME_LEN - 1 );
	__eeprom_params.sName[ SLOTEEPROM_SLOTNAME_LEN - 1 ] = '\0' ;
	__eeprom_params.fHS_12_bypass = val_Slot12v0Bypass ;
	__eeprom_params.fHS_33_bypass = val_Slot3v3Bypass ;
	__eeprom_params.oc_ar1 = val_Slot12v0OCAR ;
	__eeprom_params.oc_ar2 = val_Slot3v3OCAR ;
	__eeprom_params.uv_ar1 = val_Slot12v0UVAR ;
	__eeprom_params.uv_ar2 = val_Slot3v3UVAR ;
	__eeprom_params.ov_ar1 = val_Slot12v0OVAR ;
	__eeprom_params.ov_ar2 = val_Slot3v3OVAR ;
	Dnepr_Measure_SlotEEPROM_Write( slot_num, &__eeprom_params );
	return OK ;
}

u32 slotopteepromclear_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*(u32*)buff = 0 ;
	return OK ;
}

u32 slotopteepromclear_update(PARAM_INDEX* p_ix, void* buff)
{
	u32 slot_num = *(u32*)buff ;
	slot_num -= 1 ;
	if( slot_num > (SPI_FPGA_SLOTS_NUMBER-1)){
		return OK ;
	}
	if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[slot_num] ){
		return OK ;
	}
	Dnepr_Measure_SlotOptionalEEPROM_Clear( slot_num );
	return OK ;
}

extern u32 val_SlotOptPassive ;
extern s8  val_SlotOptDescription[32] ;
extern s8  val_SlotOptDestination[32] ;
extern s8  val_SlotOptSerial[32] ;
extern s8  val_SlotOptClass[32] ;


u32 slotopteepromwrite_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*(u32*)buff = 0 ;
	return OK ;
}

u32 slotopteepromwrite_update(PARAM_INDEX* p_ix, void* buff)
{
	return OK ;
}

u32 slotenable_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	if( PROFILE_INTValueAccess( p_ix, buff, buff_len ) == ERROR )
		return ERROR ;	
	return OK ;
}

u32 slotenable_update(PARAM_INDEX* p_ix, void* buff)
{
	s32 slot_num = p_ix->parent->owner ;
	if( (slot_num < 0) || (slot_num > 13) )
		return ERROR ;
	
	if( Dnepr_INTValueUpdate( p_ix, buff ) == ERROR )
		return ERROR ;
	// команда пересмотреть включенность устройств
	Dnepr_DControl_RereadPowerOnOff( slot_num ) ;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 cmreset_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*((u32*)buff) = 0 ;
	return OK ;
}

u32 cmreset_update(PARAM_INDEX* p_ix, void* buff)
{
	Dnepr_Measure_PSequencer_Reset() ;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// состояние Power Sequencer'а
u32 cmsequencerstate_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	*(u32*)buff = Dnepr_Measure_PSequencer_State() ;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 slot_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len)
{
	u32 slot_num = p_ix->parent->owner ;
	Dnepr_SlotEEPROM_Optional_t * pEEPROM = Dnepr_SlotOptionalEEPROM_val( slot_num );
	if( !Dnepr_SlotOptionalEEPROM_Available( slot_num ) ){
		return ERROR ;
	}
	
	*(u8*)buff = 0 ;
	if( !pEEPROM ){
		*(u8*)buff = 0 ;
		return OK ;
	}
	strncpy( (s8*)buff, (const s8*)pEEPROM->pData, MIN(buff_len, DNEPR_PASSIVE_STRUCT_DATALEN) );
	return OK ;
}

u32 slot_update(PARAM_INDEX* p_ix, void* buff)
{
	u32 slot_num = p_ix->parent->owner ;
	Dnepr_SlotEEPROM_Optional_t * pEEPROM = Dnepr_SlotOptionalEEPROM_val( slot_num );

	strncpy( (s8*)pEEPROM->pData, buff, DNEPR_PASSIVE_STRUCT_DATALEN );
	pEEPROM->length = DNEPR_PASSIVE_STRUCT_DATALEN ;
	Dnepr_Measure_SlotOptionalEEPROM_Write( slot_num, pEEPROM );
	return OK ;
}
