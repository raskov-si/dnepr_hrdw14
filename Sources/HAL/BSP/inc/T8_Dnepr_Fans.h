/*!
\file T8_Dnepr_Fans.h
\brief Процедуры для работы с платой вентиляции
\author <a href="mailto:leonov@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#ifndef __DNEPR_FANS_H_
#define __DNEPR_FANS_H_

#define	DNEPR_FANS_MAX_FANS		6

#define DNEPR_FANS_VENDOR_LEN	16
#define DNEPR_FANS_HW_LEN		32
#define DNEPR_FANS_PT_LEN		32
#define DNEPR_FANS_SR_LEN		32
#define DNEPR_FANS_PARTNUM_LEN	32

// по какому адресу eeprom будут храниться калибровочные параметры
#define	Dnepr_Fans_CalData_t_STARTADDRESS		0x8000

#define DNEPR_FANS_DATA_VERSION					0x01

//! данные, хранимые в eeprom платы вентиляции, относящиеся 
//! к функционированию вентиляторов
typedef struct {
	//! номер версии структуры
	u8 version ;
	//! оборотов в мин при скважности 40% у каждого вентилятора
	u16 pwm40_rpm[ DNEPR_FANS_MAX_FANS ];
	//! оборотов в мин при скважности 60% у каждого вентилятора
	u16 pwm60_rpm[ DNEPR_FANS_MAX_FANS ];
	//! оборотов в мин при скважности 70% у каждого вентилятора
	u16 pwm80_rpm[ DNEPR_FANS_MAX_FANS ];
	//! оборотов в мин при скважности 40% у каждого вентилятора
	u16 pwm100_rpm[ DNEPR_FANS_MAX_FANS ];

	u16	max_rpm ;
	u16 min_rpm ;

	// сколько часов может отработать до аларма по превышению ресурса вентилятора
	u32 max_work_hrs ;

	// количество вентиляторов
	u8 fans_num ;

	u8 vendor[ DNEPR_FANS_VENDOR_LEN ]; // Для профиля, название производителя
	u8 hw_number[ DNEPR_FANS_HW_LEN ];	// Для профиля, версия аппаратной части
	u8 pt_number[ DNEPR_FANS_PT_LEN ];	// Для профиля, партнамбер
	u8 sr_number[ DNEPR_FANS_SR_LEN ];	// Для профиля, серийный номер

	u8 fans_partnumber[ DNEPR_FANS_PARTNUM_LEN ]; // название вентиляторов
	u32 crc ;
} Dnepr_Fans_CalData_t ;


//! читает калибровку из платы вентиляции
_BOOL	Dnepr_Fans_ReadCalibration();

//! \brief производит калибровку
_BOOL	Dnepr_Fans_Calibrate( const Dnepr_Fans_CalData_t* calib_data );

//! Устанавливает обороты для всех вентиляторов
_BOOL	Dnepr_Fans_SetRPM( const u32 rpm );

//! получает текущие обороты вентилятора номер fan_num (от 1 до 6)
u16 	Dnepr_Fans_GetRPM( const u32 fan_num );

//! прочитана ли калибровка из платы вентиляции
_BOOL Dnepr_Fans_Calibrated();

//! возвращает данные калибровки, прочитанные с платы вентиляции
Dnepr_Fans_CalData_t *Dnepr_Fans_CalData();

//! инициализирует вентиляторы, но не включает их
void Dnepr_Fans_Init( );

#endif
