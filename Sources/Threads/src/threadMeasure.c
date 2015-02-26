/*!
\file threadMeasure.h
\brief поток для измерения динамических параметров
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date jule 2012
*/

#include <stdio.h>

#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_Fans.h"
#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/BSP/inc/T8_Dnepr_BP_CurrentMeasure.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "HAL/IC/inc/MAX_MAX31785.h"
#include "HAL/IC/inc/TI_TMP112.h"
#include "HAL/IC/inc/TI_UCD9080.h"
#include "T8_Atomiccode.h"
#include "Threads/inc/threadMeasure.h"
#include "Threads/inc/threadDeviceController.h"
#include "Threads/inc/threadCU.h"
#include "Application/inc/T8_Dnepr_Profile_params.h"
#include "Profile/inc/sys.h"
#include <string.h>
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "support_common.h"
#include "common_lib/memory.h"
#include "common_lib/medfilt.h"
#include "Profile/Generated/profile_def.h"
#include "Profile/inc/sys.h"
#include "HAL/IC/inc/T8_SFP.h"
#include "T8_Dnepr_SequencerSettings.h"

#include "prio.h"
#include "Application/inc/T8_Dnepr_Profile_params.h"

u32		app_init(void);
void	app_service(void);

// настройки вкл/выкл SFP из Profile/Generated/value.c
extern u32 val_CMSFP1TxEnable; // L - нижний
extern u32 val_CMSFP2TxEnable ; // U - верхний
extern u32 val_CMSFP1AutoNeg; // L - нижний
extern u32 val_CMSFP2AutoNeg ; // U - верхний


// предельные значения оборотов вентиляторов по умолчанию
extern u32 val_FUFanDefaulMaxRPM, val_FUFanDefaulMinRPM ;

OS_EVENT *dyn_params_mutex = NULL ;

_BOOL __init_done = FALSE ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Динамические параметры
LT_LTC4222_ControlRegisterTypedef    tControlRegisterStructure1, tControlRegisterStructure2;
LT_LTC4222_FaultAlertRegisterTypedef tAlertRegisterStructure1,   tAlertRegisterStructure2;
LT_LTC4222_StatusRegisterTypedef     tStateRegisterStructure1,   tStateRegisterStructure2;
LT_LTC4222_FaultAlertRegisterTypedef tFaultRegisterStructure1,   tFaultRegisterStructure2;
LT_LTC4222_AdcVoltagesStructure      tAdcVoltagesStructure;
LT_LTC4222_AdcVoltagesStructure      tFansAdcVoltagesStructure;

MAX_DS28CM00_ContentsTypedef tSnContents_internal;
MAX_DS28CM00_ContentsTypedef tSnContents_external;
static I2C_DNEPR_PresentDevicesTypedef* _pDev_presents = NULL;

// запоминается вставленность SFP на предыдущий момент считывания параметров
static _BOOL __sfp_l_last_presence = FALSE ;
static T8_SFP_OPTICAL_CHANNEL _sfp_l_params ;
// запоминается вставленность SFP на предыдущий момент считывания параметров
static _BOOL __sfp_u_last_presence = FALSE ;
static T8_SFP_OPTICAL_CHANNEL _sfp_u_params ;

static _BOOL __sfp_1_on ; //!< включен лазер 1й sfp
static _BOOL __sfp_2_on ; //!< включен лазер 2й sfp
static _BOOL __sfp_on_changed = FALSE ; //!< TRUE, когда поменяли __sfp_*_on, но ещё не вызвали I2C_Dnepr_SFP_OnOff

static struct
{
  _BOOL    state;               /*!< включен или нет режим auto-negotiation */
  _BOOL    changed_flag;        /*!< флаг о том что параметры изменились    */
} autoneg_state_now[2] = {0};


//! устанавливается в соотв. со считанным регистром 92 по адресу A0h из sfp
static SFP_DDM_COMPLIANCE __sfp1_ddm_compliance = DDM_COMPLIANCE_NO_ANSWER ;
//! устанавливается в соотв. со считанным регистром 92 по адресу A0h из sfp
static SFP_DDM_COMPLIANCE __sfp2_ddm_compliance = DDM_COMPLIANCE_NO_ANSWER ;

//! значения светодиодов отдаваемые в плис
static T8_Dnepr_LedStatusTypedef __led_status = {{ 	NONE, FALSE },	//	Power
												{	NONE, FALSE },		// CPU
												{	NONE, FALSE }};	// Alarm
static _BOOL __led_status_changed = TRUE ; //!< TRUE, когда в плис надо отдать значения светодиодов

// сохраняем цвета всех параметров отмеченных AL12_COLOR, чтобы отдать blockcolor_getvalue
static _BOOL mAlarm1State, mAlarm2State;

// запоминается наличие платы вентиляции при последнем считывании
// 0 -- плата не вставлена, n > 0, когда плата вставлена n цикло threadMeasure (чтобы обращаться к ней через задержку)
static u32 __fan_last_presence = 0 ;
// через сколько циклов threadMeasure можно обращаться к плате вентиляции после её вставления
static const u32 __fan_last_presence_delay = 5 ;

// статические/информационнные параметры платы вентиляции
static Dnepr_Fans_CalData_t __fans_calib_data =	{ .max_rpm = 4000, .min_rpm = 1000, .max_work_hrs = 30000,
	 .fans_num = 2, .vendor = "T8", .hw_number="", .pt_number="", .sr_number="", .fans_partnumber="" };

// когда TRUE -- поступаила команда на калибровку платы вентиляции
static _BOOL __do_calibration = FALSE ;

static _BOOL __set_fans_power = TRUE ;
static u32 __fan_power ;

static PowerSequencerFlashState_t _powerseq_state = PSEQ_UNKNOWN ;

static _BOOL __powersequencer_reset = FALSE ;

static _BOOL __write_psu_eeprom = FALSE ;
static u8 __ps_num = 0 ;
static s8* __vendor ;
static s8* __ptnumber ;
static u16 __power ;
static s8* __srnumber ;

static u32 __sl_eeprom_num = 0 ;
static SlotHSEEPROMParams_t *__sl_eeprom_data = NULL ;
static _BOOL __sl_eeprom_write = FALSE ;

static u32 __sl_opt_eeprom_num = 0 ;
static Dnepr_SlotEEPROM_Optional_t * __sl_opt_eeprom_data = NULL ;
static _BOOL __sl_opt_eeprom_write = FALSE ;
static _BOOL __sl_opt_eeprom_clear = FALSE ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Инициализируем локальные данные до того, как начнём измерять
static void _Init();
// заполняется буфер динамических параметров dyn_param
static u32 _Fill_Values();
// собственно проводятся сами измерения
static void _Do_Measurements();
// прошивка power sequencer'а
static void Dnepr_CheckSequencerFirmware(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
\brief I2C Peripherials monitoring.
*/
void taskMeasure(void *pdata)
{
	taskCU_message_t	reset_params_cu_mess = {.message_type = RESET2DEFAULT,
												.rcvmsg_len = 0, .rcv_cbuff = 0 };
	u32 button_cnt = 0 ;
	T8_Dnepr_LedStatusTypedef button_led_status = 	{{ 	RED, TRUE },	//	Power
													{	RED, TRUE },	// CPU
													{	RED, TRUE }};	// Alarm;
    pdata = pdata;
    _Init() ;
	    
    while (TRUE) {
	    _Do_Measurements();
    	_Fill_Values();
		
    	// если зажали кнопку и держат первые 5 секунд
		if( Dnepr_GPIO_FrontPanel_Button_Pressed() && ( button_cnt < 25 ) ){
			button_cnt++ ;
			// только что нажали
			if( button_cnt == 1 ){
				button_led_status.tPower.mColor = RED ;
				button_led_status.tCPU.mColor = RED ;
				button_led_status.tAlarm.mColor = RED ;
				T8_Dnepr_SetLedStatus( &button_led_status );
			} else if( button_cnt == 24 ){
				// сигнализируем о записи
				button_led_status.tPower.mColor = YELLOW ;
				button_led_status.tCPU.mColor = YELLOW ;
				button_led_status.tAlarm.mColor = YELLOW ;
				T8_Dnepr_SetLedStatus( &button_led_status );

				// пишем умолчальные настройки
				CU_send_queue_message( &reset_params_cu_mess );
				
				// ждём пока они сбросятся в EEPROM
				OSTimeDly( 1500 );

				// перезагружаемся
				__powersequencer_reset = TRUE ;
			} else if( button_cnt == 25 ){
				// меняем светодиоды на обычную индикацию
				__led_status_changed = TRUE ;
			}
		// поменялись значения светодиодов, надо обновить
		} else if( __led_status_changed == TRUE ){
			T8_Dnepr_SetLedStatus( &__led_status );
			__led_status_changed = FALSE ;
		// отпустили кнопку
		} else if( !Dnepr_GPIO_FrontPanel_Button_Pressed() ) {
			// а индикацию не выключили
			if( button_cnt < 25 ){
				__led_status_changed = TRUE ;
			}
			button_cnt = 0 ;
		}
		// надо зашить в eeprom слотового устройства данные
		if( __sl_eeprom_write && __sl_eeprom_data ){
			__sl_eeprom_write = FALSE ;
			Dnepr_SlotEEPROMWrite( __sl_eeprom_num, __sl_eeprom_data );
		}
		// надо защить в дополнительную секцию eeprom слотового устройства
		if( __sl_opt_eeprom_write && __sl_opt_eeprom_data ){
			__sl_opt_eeprom_write = FALSE ;
			Dnepr_SlotOptionalEEPROM_Write( __sl_opt_eeprom_num, __sl_opt_eeprom_data );
		}
		if( __sl_opt_eeprom_clear ){
			__sl_opt_eeprom_clear = FALSE ;
			Dnepr_SlotOptionalEEPROM_Clear( __sl_opt_eeprom_num );
		}
		// снаружи была вызвана Dnepr_Measure_SFP_ChangeState(...)
		if( __sfp_on_changed == TRUE ){
			I2C_Dnepr_SFP_OnOff( __sfp_1_on, __sfp_2_on );
			__sfp_on_changed = FALSE ;
		}
                /* включаем или отключаем режим автоопределения типа сети */
                if (autoneg_state_now[0].changed_flag) {
                    dnepr_ethernet_sfpport_autoneg_mode(1, autoneg_state_now[0].state);
                    autoneg_state_now[0].changed_flag = FALSE;
                }
                if (autoneg_state_now[1].changed_flag) {
                    dnepr_ethernet_sfpport_autoneg_mode(2, autoneg_state_now[1].state);
                    autoneg_state_now[1].changed_flag = FALSE;                  
                }                               
		// производим калибровку платы вентиляции
		if(  __do_calibration ){
			Dnepr_Fans_Calibrate( &__fans_calib_data );
			__do_calibration = FALSE ;
			Dnepr_Fans_Init() ;
			__set_fans_power = TRUE ;
		}
		// задали скорость вращения вентиляторов
		if( __set_fans_power ){
			u32 max_rpm, min_rpm ;
			__set_fans_power = FALSE ;
			// переводим из % в Об/мин
			// если eeprom доступна -- берём макс и мин из неё
			if( Dnepr_Fans_CalData() ){
				max_rpm = Dnepr_Fans_CalData()->max_rpm ;
				min_rpm = Dnepr_Fans_CalData()->min_rpm ;
				Dnepr_Fans_SetRPM( (u32)(((f32)__fan_power / 100) * (f32)(max_rpm - min_rpm) + (f32)min_rpm) );
			}
		}
		// команда на прошивку БП
		if( __write_psu_eeprom ){
			__write_psu_eeprom = FALSE ;
			
			if( PSU_WriteEEPROM( __ps_num, __vendor, __ptnumber, "NAME", "MODEL_SERIAL", __srnumber,
																		"FILEID", __power) ){
				Dnepr_DControl_ResetMaxPower() ;
			}
		}
		// команда перезагрузки секвенсора
		if( __powersequencer_reset ){
			__powersequencer_reset = FALSE ;
			TI_UCD9080_Restart() ;
		}
    	OSTimeDly(250);                       /* Delay task execution for one clock tick -- 1 мс       */
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Записывает содержимое флеш power sequencer'а, если нужно.
// Скрипт par2c.py генерирует файлы T8_Dnepr_SequencerSettings.[c|h],
// в массив sequencer_flash_chars пишет необходимое содержимое массива настроек,
// в sequencer_userspace_chars содержимое userspace.
// В userspace пишется хеш коммита файла, из которого берутся настройки sequencer'а
// (powersequencer.c)
static void Dnepr_CheckSequencerFirmware(void)
{
	u8 SequencerUserData[SEQ_USERSPACE_LEN];
    u8* sSequencerUserData = SequencerUserData;
    u8 SequencerFlash[SEQ_FLASH_LEN];
    u8* sSequencerFlash = SequencerFlash;
	_BOOL compares_res = TRUE ;
    size_t iter_counter = 0 ;
    const size_t max_iter_counter = 3 ;

    // сравниваем UserSpace секвенсора с тем, который есть у нас, если отличается -- 
    // переписываем весь секвенсор
    TI_UCD9080_ReadUserData( sSequencerUserData, SEQ_USERSPACE_LEN );
    // не отличается -- всё ок, выходим
	if( memcmp(sSequencerUserData, sequencer_userspace_chars, SEQ_USERSPACE_LEN ) == 0){
			_powerseq_state = PSEQ_OK ;
			return ;
	}

    do {
        TI_UCD9080_WriteConfig( sequencer_flash_chars,     SEQ_FLASH_LEN );
    	TI_UCD9080_WriteUserData( sequencer_userspace_chars, SEQ_USERSPACE_LEN );
    	TI_UCD9080_ClearErrorLog();

        TI_UCD9080_ReadConfig( sSequencerFlash, SEQ_FLASH_LEN );
        TI_UCD9080_ReadUserData( sSequencerUserData, SEQ_USERSPACE_LEN );
		
		compares_res = (memcmp(sSequencerFlash, sequencer_flash_chars, SEQ_FLASH_LEN ) == 0);
		compares_res = compares_res && (memcmp(sSequencerUserData, sequencer_userspace_chars, SEQ_USERSPACE_LEN ) == 0);
    } while(( !compares_res ) && ( ++iter_counter < max_iter_counter ));

    if(( iter_counter >= max_iter_counter ) && ( !compares_res )){
    	_powerseq_state = PSEQ_FAIL ;
    } else {
    	_powerseq_state = PSEQ_WROTE ;
		// всё ок, теперь нужно, чтобы оператор перезапустил ресет (параметр CMReset)
    }
}

PowerSequencerFlashState_t Dnepr_Measure_PSequencer_State()
{
	return _powerseq_state ;
}

void Dnepr_Measure_PSequencer_Reset()
{
	__powersequencer_reset = TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// лочим двойной буфер динамических параметров от переключения
// вызывается в т.ч. из других потоков
void Dnepr_Measure_DynBuff_Lock()
{
    INT8U return_code = OS_ERR_NONE;
	// самое надёжное решение, которе не влечёт наложения требований на порядок инициализации потоков или их преоритетов
	while( __init_done == FALSE )
		OSTimeDly( 10 );
    OSMutexPend( dyn_params_mutex, 0, &return_code );
    assert( return_code == OS_ERR_NONE );	
}

// разлочим двойной буфер динамических параметров
// вызывается в т.ч. из других потоков
void Dnepr_Measure_DynBuff_Unlock()
{
	while( __init_done == FALSE )
		OSTimeDly( 10 );
    OSMutexPost( dyn_params_mutex );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_Measure_SetCPULed( const T8_Dnepr_LedTypedef led )
{
	if( (__led_status.tCPU.mColor != led.mColor) ||
		(__led_status.tCPU.bBlink != led.bBlink) )
	{
		__led_status.tCPU = led ;
		__led_status_changed = TRUE ;	
	}
}

void Dnepr_Measure_SetPowerLed( const T8_Dnepr_LedTypedef led )
{
	if( (__led_status.tPower.mColor != led.mColor) ||
		(__led_status.tPower.bBlink != led.bBlink) )
	{
		__led_status.tPower = led ;
		__led_status_changed = TRUE ;	
	}	
}

void Dnepr_Measure_SetAlarmLed( const T8_Dnepr_LedTypedef led )
{
	if( (__led_status.tAlarm.mColor != led.mColor) ||
		(__led_status.tAlarm.bBlink != led.bBlink) )
	{
		__led_status.tAlarm = led ;
		__led_status_changed = TRUE ;	
	}	
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_UpdatePSUEEPROM( const u8 ps_num, const s8* vendor, const s8* ptnumber,
													const u16 power, const s8* srnumber )
{
	__ps_num = ps_num ;
	__vendor = (s8*)vendor ;
	__ptnumber = (s8*)ptnumber ;
	__power = power ;
	__srnumber = (s8*)srnumber ;
	__write_psu_eeprom = TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dnepr_Measure_SlotEEPROM_Write( const u32 slot_num, SlotHSEEPROMParams_t *pSlotParam )
{
	__sl_eeprom_num = slot_num ;
	__sl_eeprom_data = pSlotParam ;
	__sl_eeprom_write = TRUE ;
}

void Dnepr_Measure_SlotOptionalEEPROM_Write( const u32 slot_num, Dnepr_SlotEEPROM_Optional_t* pData )
{
	__sl_opt_eeprom_num = slot_num ;
	__sl_opt_eeprom_data = pData ;
	__sl_opt_eeprom_write = TRUE ;
}

void Dnepr_Measure_SlotOptionalEEPROM_Clear( const u32 slot_num )
{
	__sl_opt_eeprom_num = slot_num ;
	__sl_opt_eeprom_clear = TRUE ;
}

const T8_SFP_OPTICAL_CHANNEL* Dnepr_Measure_SFP_L_Info()
{
	// ещё не заполнили
	if( !_pDev_presents )
		return NULL ;
	// не вставили
	if( !_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_L_INDEX] )
		return NULL ;
	return &_sfp_l_params ;
}

const T8_SFP_OPTICAL_CHANNEL* Dnepr_Measure_SFP_U_Info()
{
	// ещё не заполнили
	if( !_pDev_presents )
		return NULL ;
	// не вставили
	if( !_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_U_INDEX] )
		return NULL ;
	return &_sfp_u_params ;
}

//! \brief Вызывается из других потоков, передаёт команду потоку taskMeasure поменять состояние SFP
void Dnepr_Measure_SFP_ChangeState( const _BOOL sfp_1_on_, const _BOOL sfp_2_on_ )
{
	if( sfp_1_on_ != __sfp_1_on ){
		__sfp_1_on = sfp_1_on_ ;
		__sfp_on_changed = TRUE ;
	}
	if( sfp_2_on_ != __sfp_2_on ){
		__sfp_2_on = sfp_2_on_ ;
		__sfp_on_changed = TRUE ;
	}
}

/*=============================================================================================================*/
/*!  \brief функция устанавливающая флаги сигнализирующие о том, что надо поменять состояние режима Auto-Negotiation

     \sa autoneg_state_now
*/
/*=============================================================================================================*/
void dnepr_measure_SFP_change_autoneg_mode
(
    u8 sfp_num,                         /*!< [in] номер sfp по порядку                      */
    u8 autoneg_state                    /*!< [in] состояние в которое нужно переключиться   */
)
{
    if ( autoneg_state_now[sfp_num-1].state != autoneg_state ) {
        autoneg_state_now[sfp_num-1].state        = autoneg_state;
        autoneg_state_now[sfp_num-1].changed_flag = TRUE;
    }
}


MAX_DS28CM00_ContentsTypedef *Dnepr_Measure_SerialNumber()
{
	if((__init_done == TRUE) && MAX_DS28CM00_CheckCRC(&tSnContents_internal))
		return &tSnContents_internal ;
	else
		return NULL;
} 

MAX_DS28CM00_ContentsTypedef *Dnepr_Measure_Backplane_SerialNumber()
{
	if((__init_done == TRUE) && MAX_DS28CM00_CheckCRC(&tSnContents_external))
		return &tSnContents_external ;
	else
		return NULL;
} 

SFP_DDM_COMPLIANCE Dnepr_Measure_SFP1_DDM_Compliance()
{
	return __sfp1_ddm_compliance ;
}

SFP_DDM_COMPLIANCE Dnepr_Measure_SFP2_DDM_Compliance()
{
	return __sfp2_ddm_compliance ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Dnepr_Fans_CalData_t* Dnepr_Measure_Fans_Calib()
{
	return &__fans_calib_data ;
}

void Dnepr_Measure_Fans_Calib_Update()
{
	if( __fan_last_presence >= __fan_last_presence_delay ){
		__do_calibration = TRUE ;
	}
}

void Dnepr_Measure_UpdateFansRPM( const u32 fan_power )
{
	__fan_power = fan_power ;
	__set_fans_power = TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void _Init()
{
	LT_LTC4222_AdcControlRegisterTypedef tAdcControlRegisterStructure ;
    INT8U return_code = OS_ERR_NONE;

	_pDev_presents = Dnepr_DControl_SlotPresent() ;

	dyn_params_mutex = OSMutexCreate( dyn_param_mutex_PRIO, &return_code );
	assert( return_code == OS_ERR_NONE );

	// Serial Number
	Dnepr_ReadDS28CM00_Internal( &tSnContents_internal );
	Dnepr_ReadDS28CM00_Backplane( &tSnContents_external );
	
	// устанавливаем во внутреннем HotSwap'е запрет на ALERT при завершении преобразования АЦП
	LT_LTC4222_GetAdcControl( Dnepr_I2C_Get_PMBUS_INT_Driver(), I2C_DNEPR_HSW_ADDRESS, &tAdcControlRegisterStructure );
	tAdcControlRegisterStructure.bAdcAlert = FALSE ;
	LT_LTC4222_SetAdcControl( Dnepr_I2C_Get_PMBUS_INT_Driver(), I2C_DNEPR_HSW_ADDRESS, &tAdcControlRegisterStructure );

	// прошиваем Power Sequencer
	Dnepr_CheckSequencerFirmware() ;

	// включаем SFP в соотв. с параметрами профиля
	__sfp_1_on = val_CMSFP1TxEnable ;
	__sfp_2_on = val_CMSFP2TxEnable ;
	I2C_Dnepr_SFP_OnOff( __sfp_1_on, __sfp_2_on );
        
        autoneg_state_now[0].state = val_CMSFP1AutoNeg;
        dnepr_ethernet_sfpport_autoneg_mode(1, autoneg_state_now[0].state);        
        autoneg_state_now[1].state = val_CMSFP2AutoNeg;
        dnepr_ethernet_sfpport_autoneg_mode(2, autoneg_state_now[1].state);
        

	__init_done = TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! заполняем массив dyn_param уже измеренными данными. Вызывается из threadMeasure
static u32 _Fill_Values()
{
	u32 i ;
	u32 j ;
	PARAM_INDEX* p_ix;
	u32 flag = 0;

	// заполняем двойной буфер с данными
	GL_PARAM_BUFF_RDY[APP_PARAM_BUFF] = 0;
	for(	i = 0,				// индекс в dyn_param
			j = DynamicSect+1;	// индекс в массиве pT** 
			i < DYN_PAR_ALL;i++, j++) {

		p_ix = SYSGetParamIx(j);
		if(p_ix)
			flag |= p_ix->parent->getvalue(p_ix,&dyn_param[i][APP_PARAM_BUFF]);
		else
			return ERROR;
	}
	
	Dnepr_Measure_DynBuff_Lock();
	GL_PARAM_BUFF = APP_PARAM_BUFF;
	APP_PARAM_BUFF ^= 1UL;
	GL_PARAM_BUFF_RDY[GL_PARAM_BUFF] = 1;//buffer ready		
	Dnepr_Measure_DynBuff_Unlock();

	return flag ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// если вставили sfp -- надо послать сообщение в taskCU, чтобы обновились пороги
static taskCU_message_t	renew_sfp_cu_mess ;

static void _Do_Measurements()
{
	// послать команду на перечитывание порогов SFP? Ставится в TRUE, если только что вставили
	// какой-либо SFP
	_BOOL renew_sfp_params = FALSE ;
        
    //Hot Swap Controller -- i2c switches into driver     	
	LT_LTC4222_GetStates(Dnepr_I2C_Get_PMBUS_INT_Driver(), I2C_DNEPR_HSW_ADDRESS, LT_LTC4222_CHANNEL1, &tStateRegisterStructure1, &tFaultRegisterStructure1);
	OSTimeDly(1);   
	LT_LTC4222_GetStates(Dnepr_I2C_Get_PMBUS_INT_Driver(), I2C_DNEPR_HSW_ADDRESS, LT_LTC4222_CHANNEL2, &tStateRegisterStructure2, &tFaultRegisterStructure2);
	OSTimeDly(1);   
	LT_LTC4222_GetAdcVoltages(Dnepr_I2C_Get_PMBUS_INT_Driver(), I2C_DNEPR_HSW_ADDRESS, &tAdcVoltagesStructure);
	OSTimeDly(1);   
	
///debug        
//        return;
///debug        
        
	Dnepr_DControl_sfp_process_present() ;

	// SFP & Fans
	if( _pDev_presents ){
		// вставлен верхний
		if( _pDev_presents->bSfpPresent[I2C_DNEPR_SFP_U_INDEX] ){
			// только что вставили модуль -- перечитываем параметры
			if( __sfp_u_last_presence == FALSE ){
				renew_sfp_params = TRUE ;
				I2C_Dnepr_SFP_Renew( &_sfp_u_params, TRUE, 1 );
				__sfp_u_last_presence = TRUE ;
				if(_sfp_u_params.sfp_info.flags & T8_SFP_DDM_SFF8472_COMPLIANCE){
					__sfp1_ddm_compliance = DDM_COMPLIANCE_NOT_COMPLY_SFF8472 ;
				} else if(!(_sfp_u_params.sfp_info.flags & T8_SFP_DDM_IMPLEMENTED)){
					__sfp1_ddm_compliance = DDM_COMPLIANCE_NO_DDM ;
				} else if(!(_sfp_u_params.sfp_info.flags & T8_SFP_DDM_INTERNALLY)){
					__sfp1_ddm_compliance = DDM_COMPLIANCE_INTERNAL_CALIBRATION ;
				} else if(!(_sfp_u_params.sfp_info.flags & T8_SFP_DDM_EXTERNALLY)){
					__sfp1_ddm_compliance = DDM_COMPLIANCE_EXTERNAL_CALIBRATION ;
				}
			} else {
				I2C_Dnepr_SFP_Renew( &_sfp_u_params, FALSE, 1 );
			}
		} else {
			__sfp_u_last_presence = FALSE ;
		}

		// вставлен нижний
		if( _pDev_presents->bSfpPresent[I2C_DNEPR_SFP_L_INDEX] ){
			// только что вставили модуль -- перечитываем параметры
			if( __sfp_l_last_presence == FALSE ){
				renew_sfp_params = TRUE ;
				I2C_Dnepr_SFP_Renew( &_sfp_l_params, TRUE, 0 );
				__sfp_l_last_presence = TRUE ;
				if(_sfp_l_params.sfp_info.flags & T8_SFP_DDM_SFF8472_COMPLIANCE){
					__sfp2_ddm_compliance = DDM_COMPLIANCE_NOT_COMPLY_SFF8472 ;
				} else if(!(_sfp_l_params.sfp_info.flags & T8_SFP_DDM_IMPLEMENTED)){
					__sfp2_ddm_compliance = DDM_COMPLIANCE_NO_DDM ;
				} else if(!(_sfp_l_params.sfp_info.flags & T8_SFP_DDM_INTERNALLY)){
					__sfp2_ddm_compliance = DDM_COMPLIANCE_INTERNAL_CALIBRATION ;
				} else if(!(_sfp_l_params.sfp_info.flags & T8_SFP_DDM_EXTERNALLY)){
					__sfp2_ddm_compliance = DDM_COMPLIANCE_EXTERNAL_CALIBRATION ;
				}
			} else {
				I2C_Dnepr_SFP_Renew( &_sfp_l_params, FALSE, 0 );
			}
		} else {
			__sfp_l_last_presence = FALSE ;
		}
 		if( renew_sfp_params ){
 			renew_sfp_params = FALSE ;
			renew_sfp_cu_mess.message_type = RENEW_SFP_PARAMS ;
			CU_send_queue_message( &renew_sfp_cu_mess );
 		}

		if( _pDev_presents->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM] ){
			// только что вставили модуль вентиляции (или включились)
			if( __fan_last_presence < __fan_last_presence_delay ){
				++__fan_last_presence ;
			} else if( __fan_last_presence == __fan_last_presence_delay ){
				++__fan_last_presence ;
				// читаем EEPROM
				Dnepr_Fans_ReadCalibration() ;
				// инициализируем
				Dnepr_Fans_Init( );
				// запустим вентиляторы в верхней процедуре потока
				__set_fans_power = TRUE ;
				// обновляем локальные данные
				if( Dnepr_Fans_CalData() ){
					t8_memcopy( (u8*)&__fans_calib_data, (u8*)Dnepr_Fans_CalData(), sizeof(Dnepr_Fans_CalData_t) );
				}
			}
			// читаем HotSwap
			LT_LTC4222_GetAdcVoltages( Dnepr_I2C_Get_PMBUS_EXT_Driver( I2C_DNEPR_FAN_SLOT_NUM ), DNEPR_BACKPLANE_FAN_HS, &tFansAdcVoltagesStructure);

		} else {
			__fan_last_presence = 0 ;
		}
	}
//	Dnepr_Backplane_Reload_PSU_DynInfo() ; // динамические параметры из БП

	// зажигаем critical alarm
	if(mAlarm2State)
		Dnepr_Measure_SetAlarmLed( (T8_Dnepr_LedTypedef){RED, FALSE} );
	// зажигаем warning
	else if(mAlarm1State)
		Dnepr_Measure_SetAlarmLed( (T8_Dnepr_LedTypedef){YELLOW, FALSE} );		
	// всё ок, зажигаем зелёный
	else
		Dnepr_Measure_SetAlarmLed( (T8_Dnepr_LedTypedef){GREEN, FALSE} );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 alarm1_getvalue(PARAM_INDEX* p_ix, P32_PTR pPar) {
    
	u32 j, p_id;
    PARAM_INDEX* tParamIndex;
	u32 nFlag;
    u32 mColor = SYS_NORMAL_COLOR;
    _BOOL _mAlarm1State = FALSE ;
    
    /* ALARM1 is "1" if any of dynamic parameters is in "Degrade" (yellow) state, "0" otherwise.
       We judge state by color here. */
    for(j = 0, p_id = DynamicSect + 1; j < DYN_PAR_ALL; j++, p_id++){
        tParamIndex = SYSGetParamIx(p_id);
        if(tParamIndex){
            nFlag = SYSGetParamFlag(p_id);
            if(nFlag & AL1_COLOR){
                if(dyn_param[j][APP_PARAM_BUFF].par_color == SYS_MINOR_COLOR){
                    _mAlarm1State = TRUE ;
                    mColor = SYS_MINOR_COLOR;
                }
            }
        }
	}//param cycle

	mAlarm1State = _mAlarm1State ;
	pPar->value.U32 = ( mAlarm1State == TRUE ? 1 : 0 );
	pPar->par_color = mColor;
	pPar->ready = 1;
	return OK;
}

u32 alarm2_getvalue(PARAM_INDEX* p_ix, P32_PTR pPar) {
	u32 j, p_id;
    PARAM_INDEX* tParamIndex;
	u32 nFlag;
    u32 mColor = SYS_NORMAL_COLOR;
    _BOOL _mAlarm2State = FALSE ;

    /* ALARM2 is "1" if any of dynamic parameters is in "Failure" (red) state, "0" otherwise.
       We judge state by color here. */
    for(j = 0, p_id = DynamicSect + 1; j < DYN_PAR_ALL; j++, p_id++){
        tParamIndex = SYSGetParamIx(p_id);
        if(tParamIndex){
            nFlag = SYSGetParamFlag(p_id);
            if(nFlag & AL2_COLOR){
                if(dyn_param[j][APP_PARAM_BUFF].par_color == SYS_CRITICAL_COLOR){
                	// если параметр виден
                	if( SYS_is_param_hidden( p_id ) == ERROR ){
						_mAlarm2State = TRUE ;
						mColor = SYS_CRITICAL_COLOR;
	                }
                }
            }
        }
	}//param cycle

	mAlarm2State = _mAlarm2State ;
	pPar->value.U32 = ( mAlarm2State == TRUE ? 1 : 0 );
	pPar->par_color = mColor;
	pPar->ready = 1;
	return OK;
}

// Block Color
u32 blockcolor_getvalue(PARAM_INDEX* p_ix, P32_PTR pPar) {

    u32 mColor = SYS_NORMAL_COLOR;
    // tTerekLeds.mStatusLedColor = T8_TEREK_PanelLedGreen;  
    
    /* BlockColor is Red if any of dynamic parameters is red,
       yellow if any of dynamic parameters is yellow and no paraeter is red,
       green otherwise. We judge colors by ALARM1 and ALARM2 states here. */       
    /* Status LED color is set here as well.
       This value gets written to physical LED in main I2C communication loop. */
    if (mAlarm2State == TRUE){
        mColor = SYS_CRITICAL_COLOR;
        // tTerekLeds.mStatusLedColor = T8_TEREK_PanelLedRed;  
    }
    else{
        if (mAlarm1State == TRUE){
            mColor = SYS_MINOR_COLOR;
            // tTerekLeds.mStatusLedColor = T8_TEREK_PanelLedYellow;  
        }
    }
    
    /* TODO: Implement correct value */
	pPar->value.U32 = mColor;
	pPar->par_color = mColor;
	pPar->ready = 1;
	return OK;
}


// time
u32 cmtime_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	pPar->value.U32 = (u32)round(((float)llUptime)/(float)OS_TICKS_PER_SEC) ;
	pPar->par_color = SYS_EMPTY_COLOR_ERROR;
	pPar->ready = 1;
	return OK ;
}


// SlotsState
// наличие устройств в слотах
u32 slotsstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	SlotPowerState_t stat ;
	u32 slot_present_2_hex ;
	u32 i ;

	if(_pDev_presents){
		// проходим по всем и складываем в одну переменную
		slot_present_2_hex = 0 ;
		for(i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
			// Если в EEPROM устройства есть дополнительная запись,
			// в которой указано, что устройство пассивное, -- пропускаем это устройство.
			if( Dnepr_SlotOptionalEEPROM_Available(i) == TRUE ){
				continue ;
			}

			// проверяем состояние hotswap'а слота
			stat = Dnepr_DControl_PowerStatus( i );
			// если слот заполнен
			if(	(_pDev_presents->bSlotPresent[i] == TRUE) &&
				// и его хотсвап присутствует или непрочитан
				((stat == HSSLOT_ON) ||
				(stat == HSSLOT_UNAVAILABLE)) ){
				// отмечаем слот как заполненный
				slot_present_2_hex |= 1 << i ;
			}
		}

		if( Dnepr_Backplane_GetPSU_Status()->tPs1.bSeated == 0 )
			slot_present_2_hex |= (u32)1 << 30 ;
		if( Dnepr_Backplane_GetPSU_Status()->tPs2.bSeated == 0 )
			slot_present_2_hex |= (u32)1 << 31 ;

		pPar->value.U32 = slot_present_2_hex ;
		pPar->ready = 1 ;
	} else 
		pPar->ready = 0 ;

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	return OK ;
}

u32 passiveslotstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	SlotPowerState_t stat ;
	u32 slot_present_2_hex ;
	u32 i ;

	if(_pDev_presents){
		// проходим по всем и складываем в одну переменную
		slot_present_2_hex = 0 ;
		for(i = 0; i < I2C_DNEPR_NUMBER_OF_SLOTS; i++ ){
			// если в eeprom уст-ва записано, что оно активное
			// -- пропускаем это устройство
			if( Dnepr_SlotOptionalEEPROM_Available(i) == FALSE ){
				continue ;
			}

			// проверяем состояние hotswap'а слота
			stat = Dnepr_DControl_PowerStatus( i );
			// если слот заполнен
			if(	(_pDev_presents->bSlotPresent[i] == TRUE) &&
				// и его хотсвап присутствует или непрочитан
				((stat == HSSLOT_ON) ||
				(stat == HSSLOT_UNAVAILABLE)) ){
				// отмечаем слот как заполненный
				slot_present_2_hex |= 1 << i ;
			}
		}
		pPar->value.U32 = slot_present_2_hex ;
		pPar->ready = 1 ;
	} else 
		pPar->ready = 0 ;

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// параметры, которые нужны только чтобы CU в них что-то вставил

u32 cpuusage_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// CPUUsage
	pPar->value.S32 = 0 ;
	pPar->par_color = SYS_INFO_COLOR;
	pPar->ready = 1;
	return OK ;
}

u32 memload_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// MemLoad
	pPar->value.S32 = 0 ;
	pPar->par_color = SYS_INFO_COLOR;
	pPar->ready = 1;
	return OK ;
}


u32 diskspace_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// DiskSpace
	pPar->value.S32 = 0 ;
	pPar->par_color = SYS_INFO_COLOR;
	pPar->ready = 1;
	return OK ;
}


u32 localtime_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// LocalTime
	pPar->value.text = "" ;
	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1;
	return OK ;
}


u32 localdate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// LocalDate
	pPar->value.text = "" ;
	pPar->par_color = SYS_EMPTY_COLOR_ERROR;
	pPar->ready = 1;
	return OK ;
}


u32 uptime_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// UpTime
	pPar->value.text = "" ;
	pPar->par_color = SYS_EMPTY_COLOR_ERROR;
	pPar->ready = 1;
	return OK ;
}


u32 vstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// CrateState
	pPar->value.U32 = 0 ;
	pPar->par_color = SYS_EMPTY_COLOR_ERROR;
	pPar->ready = 1;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define APP_SET_COLOR(color,y) color=(y)?SYS_NORMAL_COLOR:SYS_CRITICAL_COLOR
#define STR_NO_TRAP_COLOR SYS_NO_COLOR

MEDFILT_CREATE( tcase_medfilt );
u32 tcase_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	pPar->value.F32 = t8_medfilt_f32( &tcase_medfilt, TI_TMP112_ReadTemperature() );
	pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
	pPar->ready = 1;
	return OK ;
}

MEDFILT_CREATE( v33v_medfilt );
u32 v33v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// V33
	pPar->value.F32 = t8_medfilt_f32( &v33v_medfilt, tAdcVoltagesStructure.fSource2 );
	pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
	pPar->ready = 1;
	return OK ;
}


u32 cmi33v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// I33
	// 10 мОм шунт R234 и умножаем на 1000, чтобы получить мА
	pPar->value.F32 = (tAdcVoltagesStructure.fSense2/0.010)*1000 ;
	pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
	pPar->ready = 1;
	return OK ;
}


MEDFILT_CREATE( v12v_medfilt );
u32 v12v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// V12
	pPar->value.F32 = t8_medfilt_f32( &v12v_medfilt, tAdcVoltagesStructure.fSource1 );
	pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
	pPar->ready = 1;
	return OK ;
}


u32 cmi12v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	// I12
	// 5 мОм шунт R235 и умножаем на 1000, чтобы получить мА
	pPar->value.F32 = (tAdcVoltagesStructure.fSense1/0.005)*1000 ;
	pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
	pPar->ready = 1;
	return OK ;
}

// наличие sfp
u32 cmsfppres_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	const u32 sfp_num = p_ix->parent->owner; // Lower -- 1, Upper -- 2

	if((sfp_num == 0) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_L_INDEX])){
		// наличие SFP 1
		// Enum: SFP?Pres;Empty@0|Present@1
		pPar->value.U32 = 1 ;
		pPar->ready = 1;
		pPar->par_color = SYS_EMPTY_COLOR_ERROR;
	} else if((sfp_num == 1) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_U_INDEX])){

		pPar->value.U32 = 1 ;
		pPar->ready = 1;
		pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	} else {
		// отсутствует
		pPar->value.U32 = 0 ;
		pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
		pPar->ready = 0;
	}
	pPar->ready = 1;

	return OK ;
}

MEDFILT_CREATE( sfpin1_medfilt );
MEDFILT_CREATE( sfpin2_medfilt );
u32 cmsfppin_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	const u32 sfp_num = p_ix->parent->owner; // Lower -- 1, Upper -- 2

	// SFP Input Optical Power
	if((sfp_num == 0) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_L_INDEX])){
		pPar->value.F32 = t8_medfilt_f32( &sfpin1_medfilt, _sfp_l_params.rx_pwr );
		pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
		pPar->ready = 1;
	} else if((sfp_num == 1) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_U_INDEX])){
		pPar->value.F32 = t8_medfilt_f32( &sfpin2_medfilt, _sfp_u_params.rx_pwr );
		pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
		pPar->ready = 1;
	} else {
		// отсутствует
		// чистим фильтр
		if( sfp_num == 0 ){
			sfpin1_medfilt.filled = 0 ;
		} else if( sfp_num == 1 ){
			sfpin2_medfilt.filled = 0 ;
		}
		pPar->value.F32 = 0 ;
		pPar->par_color = STR_NO_TRAP_COLOR ;
		pPar->ready = 0;
	}

	return OK ;
}

MEDFILT_CREATE( sfpout1_medfilt );
MEDFILT_CREATE( sfpout2_medfilt );
u32 cmsfppout_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	const u32 sfp_num = p_ix->parent->owner; // Lower -- 1, Upper -- 2

	// SFP Output optical power
	if((sfp_num == 0) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_L_INDEX])){
		pPar->value.F32 = t8_medfilt_f32( &sfpout1_medfilt, _sfp_l_params.tx_pwr);
		pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
		pPar->ready = 1;
	} else if((sfp_num == 1) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_U_INDEX])){
		pPar->value.F32 = t8_medfilt_f32( &sfpout2_medfilt, _sfp_u_params.tx_pwr );
		pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
		pPar->ready = 1;
	} else {
		// отсутствует
		// чистим фильтр
		if( sfp_num == 0 ){
			sfpout1_medfilt.filled = 0 ;
		} else if( sfp_num == 1 ){
			sfpout2_medfilt.filled = 0 ;
		}
		pPar->value.F32 = 0 ;
		pPar->par_color = STR_NO_TRAP_COLOR ;
		pPar->ready = 0;
	}

	return OK ;
}

u32 cmsfpthrmode_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar) {
	const u32 sfp_num = p_ix->parent->owner; // Lower -- 1, Upper -- 2

	// SFP Thresholds source
	if((sfp_num == 0) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_L_INDEX])){
		pPar->value.U32 = 
				Dnepr_Params_SFP1_Setup_source() == SFP_SETUP ? 0 : 1;
		pPar->ready = 1;
	} else if((sfp_num == 1) && 
		(_pDev_presents->bSfpPresent[I2C_DNEPR_SFP_U_INDEX])){
		pPar->value.U32 = 
				Dnepr_Params_SFP2_Setup_source() == SFP_SETUP ? 0 : 1;
		pPar->ready = 1;
	} else {
		// отсутствует
		pPar->value.U32 = 0 ;
		pPar->ready = 0;
	}

	pPar->par_color = STR_NO_TRAP_COLOR ;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define __PSU_MEASUREMENTS_(psu_num,measurements_field)		const PSU_UnitMeasurements* pPSUMsrs = Dnepr_Backplane_GetPSU_Measures(psu_num);\
															if( pPSUMsrs ){																	\
																pPar->value.F32 = pPSUMsrs->measurements_field ;							\
																pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );					\
																pPar->ready = 1 ;															\
																return OK ;																	\
															} else {																		\
																pPar->value.F32 = 0 ;														\
																pPar->par_color =  0 ;														\
																pPar->ready = 0 ;															\
																return ERROR ;																\
															}

MEDFILT_CREATE( ps1pres_medfilt );
MEDFILT_CREATE( ps2pres_medfilt );
u32 pspres_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	if( p_ix->parent->owner == 0 ){
		pPar->value.U32 = t8_medfilt_u32( &ps1pres_medfilt,
										Dnepr_Backplane_PS1_Present() );
		pPar->ready = 1 ;
		if(pPar->value.U32)
			pPar->par_color = SYS_NORMAL_COLOR ;
		else
			pPar->par_color = SYS_MINOR_COLOR ;
		return OK ;
	} else if( p_ix->parent->owner == 1 ){
		pPar->value.U32 = t8_medfilt_u32( &ps2pres_medfilt, 
											Dnepr_Backplane_PS2_Present() );
		pPar->ready = 1 ;
		if(pPar->value.U32)
			pPar->par_color = SYS_NORMAL_COLOR ;
		else
			pPar->par_color = SYS_MINOR_COLOR ;
		return OK ;
	}
	return ERROR ;
}

u32 psinpwrstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	if( p_ix->parent->owner == 0 ){
		if( Dnepr_Backplane_PS1_Present() ){
			pPar->value.U32 = Dnepr_Backplane_GetPSU_Status()->tPs1.bInOk ;
		} else {
			pPar->ready = 0 ;
			return ERROR ;
		}
	} else if( p_ix->parent->owner == 1 ){
		if( Dnepr_Backplane_PS2_Present() ){
			pPar->value.U32 = Dnepr_Backplane_GetPSU_Status()->tPs2.bInOk ;
		} else {
			pPar->ready = 0 ;
			return ERROR ;
		}
	} else {
		return ERROR ;
	}

	if( pPar->value.U32 ){
		pPar->par_color = SYS_MINOR_COLOR ;
	} else {
		pPar->par_color = SYS_NORMAL_COLOR ;
	}
	pPar->ready = 1 ;
	return OK ;
}

MEDFILT_CREATE( ps1outpwr_medfilt );
MEDFILT_CREATE( ps2outpwr_medfilt );
u32 psoutpwrstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	if( p_ix->parent->owner == 0 ){
		if( Dnepr_Backplane_PS1_Present() ){
			pPar->value.U32 = t8_medfilt_u32( &ps1outpwr_medfilt, Dnepr_Backplane_GetPSU_Status()->tPs1.bPowerGood );
			pPar->par_color = pPar->value.U32 == 1 ? SYS_NORMAL_COLOR : SYS_MINOR_COLOR ;
			pPar->ready = 1 ;
			return OK ;
		} else {
			pPar->ready = 0 ;
			pPar->par_color = STR_NO_TRAP_COLOR ;
			return ERROR ;
		}
	} else if( p_ix->parent->owner == 1 ){
		if( Dnepr_Backplane_PS2_Present() ){
			pPar->value.U32 = t8_medfilt_u32( &ps2outpwr_medfilt, Dnepr_Backplane_GetPSU_Status()->tPs2.bPowerGood );
			pPar->par_color = pPar->value.U32 == 1 ? SYS_NORMAL_COLOR : SYS_MINOR_COLOR ;
			pPar->ready = 1 ;
			return OK ;
		} else {
			pPar->ready = 0 ;
			pPar->par_color = STR_NO_TRAP_COLOR ;
			return ERROR ;
		}
	} else {
		return ERROR ;
	}
}

extern f32 val_PS1CurrentK ;
extern f32 val_PS1CurrentB ;
extern f32 val_PS2CurrentK ;
extern f32 val_PS2CurrentB ;

MEDFILT_CREATE( ps1outcurr_medfilt );
MEDFILT_CREATE( ps2outcurr_medfilt );
static f32 _ps1_curr, _ps2_curr ;
u32 ps1outcurrent_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	Dnepr_BP_Current_Measure( &_ps1_curr, val_PS1CurrentK, val_PS1CurrentB,
										&_ps2_curr, val_PS2CurrentK, val_PS2CurrentB );
	pPar->value.F32 = t8_medfilt_f32( &ps1outcurr_medfilt, _ps1_curr );
	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;
	return OK ;
}

u32 ps2outcurrent_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	// измерение обоих каналок делаем в ps1outcurrent_getvalue
	
	pPar->value.F32 = t8_medfilt_f32( &ps2outcurr_medfilt, _ps2_curr );
	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;
	return OK ;
}

u32 psvout_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{		__PSU_MEASUREMENTS_(p_ix->parent->owner,fVout) }

u32 psvin_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{	__PSU_MEASUREMENTS_(p_ix->parent->owner,fVin) }

u32 psfanspeed_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{	__PSU_MEASUREMENTS_(p_ix->parent->owner,nFanSpeed) }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// вентиляторы

MEDFILT_CREATE( fupresent_medfilt );
u32 fupresent_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	u32 presence = t8_medfilt_u32( &fupresent_medfilt, I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM] );
	// вентиляторы отсутсвутют
	if( !presence ){
		pPar->value.U32 = 1 ;
		pPar->par_color = SYS_CRITICAL_COLOR ;
		pPar->ready = 1 ;
		return OK ;
	// вентиляторы присутствуют
	} else {
		pPar->value.U32 = 0 ;
		pPar->par_color = SYS_NORMAL_COLOR ;
		pPar->ready = 1 ;
		return OK ;
	}
}

u32 fueepromstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	// блока вентиляции нет -- не отображаем состоянии EEPROM
	if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM] ){
		pPar->value.U32 = 1 ;
		pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
		pPar->ready = 1 ;
		return ERROR ;
	// блок есть -- отображаем статус EEPROM
	} else {
		pPar->value.U32 = Dnepr_Fans_Calibrated() == TRUE ? 0 : 1 ;
		pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
		pPar->ready = 1 ;
		return OK ;
	}
}

t8_medfilt_t fufanspeed_medfilt[ 6 ] = {
											{ {0,0,0}, 0, 0 },
											{ {0,0,0}, 0, 0 },
											{ {0,0,0}, 0, 0 },
											{ {0,0,0}, 0, 0 },
											{ {0,0,0}, 0, 0 },
											{ {0,0,0}, 0, 0 }
										};

u32 fufanspeed_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{ 
	u32 max_rpm, min_rpm ;
	// вентиляторов нет -- ничего не отображаем
	if( !I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM] ){
		pPar->value.F32 = 0 ;
		pPar->par_color = SYS_NO_COLOR ;
		pPar->ready = 1 ;
		return ERROR ;
	// вентиляторы есть
	} else {
		u16 speed = t8_medfilt_u32( &fufanspeed_medfilt[p_ix->parent->owner],
												Dnepr_Fans_GetRPM( p_ix->parent->owner ));
		// если есть данные из EEPROM берём min и max оборотов из неё
		if( Dnepr_Fans_CalData() ) {
			max_rpm = __fans_calib_data.max_rpm ;
			min_rpm = __fans_calib_data.min_rpm ;
		// если EEPROM не исправна берём данные из умолчальных значений:
		// параметры FUFanDefaulMinRPM и FUFanDefaulMaxRPM
		} else {
			max_rpm = val_FUFanDefaulMaxRPM ;
			min_rpm = val_FUFanDefaulMinRPM ;
		}
		// ограничиваем скорость, чтобы не отображать обороты -33% или 21358%
		if( speed < min_rpm ){
			speed = min_rpm ;
		} else if( speed > max_rpm ){
			speed = max_rpm ;
		}
		pPar->value.F32 = (((f32)speed-(f32)min_rpm)/(f32)(max_rpm-min_rpm)*100.) ;
		pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) );
		pPar->ready = 1 ;
		return OK ;
	}
}

u32 fuv12i_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	if( I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM] && Dnepr_Fans_CalData() ){
		pPar->value.F32 = tFansAdcVoltagesStructure.fSense1 / Dnepr_SlotEEPROM_12v0_Bypass( I2C_DNEPR_FAN_SLOT_NUM ) * 1000000. ;
		pPar->par_color = SYSGetParamZone( SYSGetParamID(p_ix) ) ;
		pPar->ready = 1 ;
		return OK ;
	} else {
		pPar->ready = 0 ;
		return ERROR ;
	}
}

u32 fuv33i_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	if( I2C_DNEPR_GetPresentDevices()->bSlotPresent[I2C_DNEPR_FAN_SLOT_NUM] && Dnepr_Fans_CalData() ){
		pPar->value.F32 = tFansAdcVoltagesStructure.fSense2 / Dnepr_SlotEEPROM_3v3_Bypass( I2C_DNEPR_FAN_SLOT_NUM ) * 1000000. ;
		pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
		pPar->ready = 1 ;
		return OK ;
	} else {
		pPar->ready = 0 ;
		return ERROR ;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 0 отсутствует, 1 всё ок, 2 ошибки
u32 veepromstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	// BPEEPROM_NONE,			//!< отсутствует
	// BPEEPROM_OK,			//!< всё хорошо
	// BPEEPROM_ERRORNEOUS		//!< ошибки

	pPar->value.U32 = Dnepr_eeprom_GetState() != BPEEPROM_OK ;
	pPar->par_color = Dnepr_eeprom_GetState() == BPEEPROM_OK ? SYS_NORMAL_COLOR : SYS_MINOR_COLOR ;
	pPar->ready = 1 ;
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static size_t __sPmbalarm_len = 0 ;
static s8 __sPmbalarm_str[ 128 ] = { [0] = 0 };
u32 vpmbusalarm_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	const size_t str_len = 64 ;
	s8* str = pPar->value.text = __sPmbalarm_str ;
	
	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;

	__sPmbalarm_len += Dnepr_DControl_NextPMBusAlarm( &str[ __sPmbalarm_len ],
												str_len - __sPmbalarm_len );
	// если заполнилось -- пишем заново
	if( __sPmbalarm_len >= str_len ){
		t8_memzero( (u8*)str, str_len );
		__sPmbalarm_len = 0 ;
	}
	if( __sPmbalarm_len == 0 ){
		sprintf( str, "None" );
	}
	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// параметры включения слотовых устройств

// состоаяние hotswap'а слота
u32 slotpowerstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	u32 slot_num = p_ix->parent->owner ;
	SlotPowerState_t stat ;
	if( slot_num > 13){
		return ERROR ;
	}

	stat = Dnepr_DControl_PowerStatus( slot_num );
	if( stat == HSSLOT_UNAVAILABLE ){
		pPar->value.U32 = 0 ; // Unavailable
	}else if( stat == HSSLOT_ON ){
		pPar->value.U32 = 1 ; // On
	}else if( stat == HSSLOT_REGULAR_OFF ){
		pPar->value.U32 = 3 ; // SwitchedOff
	}else if( stat == HSSLOT_OVERLIMIT_OFF ){
		pPar->value.U32 = 2 ; // PowerLimitOff
	}else if( stat == HSSLOT_WAITING ){
		pPar->value.U32 = 0 ; // Unavailable
	}

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;

	return OK ;
}

// имя устройства из EEPROM
u32 slotname_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	u32 slot_num = p_ix->parent->owner ;
	if( slot_num > 13){
		return ERROR ;
	}
	pPar->value.text = (s8*)Dnepr_SlotEEPROM_DeviceName( slot_num ) ;

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;
	return OK ;
}

// мощность потребляемая слотом
u32 slotpower_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	u32 slot_num = p_ix->parent->owner ;
	if( slot_num > 13){
		return ERROR ;
	}

	pPar->value.U32 = Dnepr_SlotEEPROM_SlotPower( slot_num );

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;
	return OK ;
}

// Максимальная мощность вставленного слотового устройства, вычисляется в threadDevices
u32 slotmaxpower_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	pPar->value.F32 = Dnepr_DControl_MaxPower() ;

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;
	return OK ;
}

// состояние хотсвапа в блоке вентиляции
u32 fupowerstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	SlotPowerState_t stat ;
	
	stat = Dnepr_DControl_PowerStatus( I2C_DNEPR_FAN_SLOT_NUM );
	if( stat == HSSLOT_UNAVAILABLE ){
		pPar->value.U32 = 0 ; // Unavailable
	}else if( stat == HSSLOT_ON ){
		pPar->value.U32 = 1 ; // On
	}else if( stat == HSSLOT_REGULAR_OFF ){
		pPar->value.U32 = 3 ; // SwitchedOff
	}else if( stat == HSSLOT_OVERLIMIT_OFF ){
		pPar->value.U32 = 2 ; // PowerLimitOff
	}else if( stat == HSSLOT_WAITING ){
		pPar->value.U32 = 0 ; // Unavailable
	}

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;

	return OK ;
}

// имя, записанное в eeprom блока вентиляции
u32 funame_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	pPar->value.text = (s8*)Dnepr_SlotEEPROM_DeviceName( I2C_DNEPR_FAN_SLOT_NUM ) ;

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;
	return OK ;
}

// потребляемая мощность, записанная в блоке вентиляции
u32 fupower_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	pPar->value.U32 = Dnepr_SlotEEPROM_SlotPower( I2C_DNEPR_FAN_SLOT_NUM );

	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;
	return OK ;
}

// сколько осталось мощности, 0 -- бесконечно
u32 vpowerreserve_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	f32 fReserve = Dnepr_DControl_PowerReserv() ;
	pPar->value.F32 = fReserve ;
	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;

	return OK ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 cmprofdelay_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar)
{
	pPar->value.F32 = (f32)CU_get_max_delay() * 0.1092266667 ;
	pPar->par_color = SYS_EMPTY_COLOR_ERROR ;
	pPar->ready = 1 ;

	return OK ;	
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// светодиоды

u32 led1state_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
    u32 mColorTmp;
    
    switch(__led_status.tPower.mColor){
    case NONE:
        mColorTmp = 0;
        break;
    case GREEN:
        mColorTmp = 1;
        break;
    case RED:
        mColorTmp = 2;
        break;
    case YELLOW:
        mColorTmp = 3;
        break;
    }   
    
    *(u32*)buff = mColorTmp;
	return OK ;
}


u32 led2state_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
    u32 mColorTmp;
    
    switch(__led_status.tCPU.mColor){
    case NONE:
        mColorTmp = 0;
        break;
    case RED:
        mColorTmp = 2;
        break;
    case GREEN:
        mColorTmp = 1;
        break;
    case YELLOW:
        mColorTmp = 3;
        break;
    }   
    
    *(u32*)buff = mColorTmp;
	return OK ;
}


u32 led3state_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len) {
    u32 mColorTmp;
    
    switch(__led_status.tAlarm.mColor){
    case NONE:
        mColorTmp = 0;
        break;
    case RED:
        mColorTmp = 2;
        break;
    case GREEN:
        mColorTmp = 1;
        break;
    case YELLOW:
        mColorTmp = 3;
        break;
    }   
    
    *(u32*)buff = mColorTmp;
	return OK ;
}
