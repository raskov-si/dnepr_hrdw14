/*!
\file PMBus_Commands.h
\brief Common PMBus commands and addresses.
\details Supported devices:
	\li Maxim MAX31785 (fan controller unit);
	\li Emerson DS1000DC-3, DS1200DC-3 (power supply unit).
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 26.03.2012
*/


#ifndef __PMBUS_COMMANDS_H
#define __PMBUS_COMMANDS_H

#include "Binary.h"

/*!
\defgroup PMBus_Driver
\{
*/

/*!
\defgroup PMBus_Command_Codes_Defines
\brief Standard PMBus commands. 
\{
*/

#define PMB_PAGE 0x00
#define PMB_OPERATION 0x01
#define PMB_ON_OFF_CONFIG 0x02
#define PMB_CLEAR_FAULTS 0x03
#define PMB_WRITE_PROTECT 0x10
#define PMB_STORE_DEFAULT_ALL 0x11
#define PMB_RESTORE_DEFAULT_ALL 0x12
#define PMB_STORE_USER_ALL 0x15
#define PMB_RESTORE_USER_ALL 0x16
#define PMB_CAPABILITY 0x19
#define PMB_VOUT_MODE 0x20
#define PMB_VOUT_COMMAND 0x21
#define PMB_VOUT_TRIM 0x22
#define PMB_CAL_OFFSET 0x23
#define PMB_VOUT_MAX 0x24
#define PMB_VOUT_SCALE_MONITOR 0x2A
#define PMB_COEFFICIENTS 0x30
#define PMB_POUT_MAX 0x31
#define PMB_VIN_ON 0x35
#define PMB_VIN_OFF 0x36
#define PMB_IOUT_CAL_GAIN 0x38
#define PMB_IOUT_CAL_OFFSET 0x39
#define PMB_FAN_CONFIG_1_2 0x3A
#define PMB_FAN_COMMAND_1 0x3B
#define PMB_VOUT_OV_FAULT_LIMIT 0x40
#define PMB_VOUT_OV_FAULT_RESPONSE 0x41
#define PMB_VOUT_OV_WARN_LIMIT 0x42
#define PMB_VOUT_UV_WARN_LIMIT 0x43
#define PMB_VOUT_UV_FAULT_LIMIT 0x44
#define PMB_VOUT_UV_FAULT_RESPONSE 0x45
#define PMB_IOUT_OC_FAULT_LIMIT 0x46
#define PMB_IOUT_OC_FAULT_RESPONSE 0x47
#define PMB_IOUT_OC_WARN_LIMIT 0x4A
#define PMB_OT_FAULT_LIMIT 0x4F
#define PMB_OT_FAULT_RESPONSE 0x50
#define PMB_OT_WARN_LIMIT 0x51
#define PMB_VIN_OV_FAULT_LIMIT 0x55
#define PMB_VIN_OV_FAULT_RESPONSE 0x56
#define PMB_VIN_OV_WARN_LIMIT 0x57
#define PMB_VIN_UV_WARN_LIMIT 0x58
#define PMB_VIN_UV_FAULT_LIMIT 0x59
#define PMB_VIN_UV_FAULT_RESPONSE 0x5A
#define PMB_IIN_OC_FAULT_LIMIT 0x5B
#define PMB_IIN_OC_FAULT_RESPONSE 0x5C
#define PMB_POWER_GOOD_ON 0x5E
#define PMB_POWER_GOOD_OFF 0x5F
#define PMB_TON_DELAY 0x60
#define PMB_TON_RISE 0x61
#define PMB_TOFF_DELAY 0x64
#define PMB_STATUS_BYTE 0x78
#define PMB_STATUS_WORD 0x79
#define PMB_STATUS_VOUT 0x7A
#define PMB_STATUS_IOUT 0x7B
#define PMB_STATUS_INPUT 0x7C
#define PMB_STATUS_TEMPERATURE 0x7D
#define PMB_STATUS_CML 0x7E
#define PMB_STATUS_MFR_SPECIFIC 0x80
#define PMB_STATUS_FANS_1_2 0x81
#define PMB_READ_VIN 0x88
#define PMB_READ_IIN 0x89
#define PMB_READ_VCAP 0x8A
#define PMB_READ_VOUT 0x8B
#define PMB_READ_IOUT 0x8C
#define PMB_READ_TEMPERATURE_1 0x8D
#define PMB_READ_TEMPERATURE_2 0x8E
#define PMB_READ_FAN_SPEED_1 0x90
#define PMB_READ_POUT 0x96
#define PMB_READ_PIN 0x97
#define PMB_PMBUS_REVISION 0x98
#define PMB_MFR_ID 0x99
#define PMB_MFR_MODEL 0x9A
#define PMB_MFR_REVISON 0x9B
#define PMB_MFR_LOCATION 0x9C
#define PMB_MFR_DATE 0x9D
#define PMB_MFR_SERIAL 0x9E
#define PMB_MFR_VIN_MIN 0xA0
#define PMB_MFR_VIN_MAX 0xA1
#define PMB_MFR_IIN_MAX 0xA2
#define PMB_MFR_PIN_MAX 0xA3
#define PMB_MFR_VOUT_MIN 0xA4
#define PMB_MFR_VOUT_MAX 0xA5
#define PMB_MFR_IOUT_MAX 0xA6
#define PMB_MFR_POUT_MAX 0xA7
#define PMB_MFR_TAMBIENT_MAX 0xA8
#define PMB_MFR_TAMBIENT_MIN 0xA9
#define PMB_MFR_EFFICIENCY_LL 0xAA
#define PMB_MFR_EFFICIENTY_HL 0xAB
#define PMB_MFR_MODE 0xD1
#define PMB_MFR_VOUT_PEAK 0xD4
#define PMB_MFR_TEMPERATURE_PEAK 0xD6
#define PMB_MFR_FAULT_RESPONSE 0xD9
#define PMB_MFR_NV_FAULT_LOG 0xDC
#define PMB_MFR_TIME_COUNT 0xDD
#define PMB_MFR_TEMP_SENSOR_CONFIG 0xF0
#define PMB_MFR_FAN_CONFIG 0xF1
#define PMB_MFR_FAN_LUT 0xF2
#define PMB_MFR_READ_FAN_PWM 0xF3
#define PMB_MFR_FAN_FAULT_LIMIT 0xF5
#define PMB_MFR_WARN_LIMIT 0xF6
#define PMB_MFR_FAN_RUN_TIME 0xF7
#define PMB_MFR_FAN_PWM_AVG 0xF8
#define PMB_MFR_FAN_PWM2RPM 0xF9


/*!
\}
*/ //PMBus_Command_Codes_Defines

/*!
\defgroup PMBus_Address_Routines_Defines
\Device addresses, R/W macros etc. 
\{
*/

/*! PMBus ARA */
#define PMB_ALERT_RESPONSE_ADDRESS B8(0001100<<1)

/*!
\brief DS75LS's address
\details " 1 0 0 1 A2 A1 A0 "
*/
enum PMB_DS75LS_ADDR{
	PMB_TEMP_SENSOR_0_ADDRESS = B8(1001000<<1), /*!< IC temperature sensor address if A2=Low, A1=Low, A0=Low */
	PMB_TEMP_SENSOR_1_ADDRESS = B8(1001001<<1), /*!< IC temperature sensor address if A2=Low, A1=Low, A0=High */
	PMB_TEMP_SENSOR_2_ADDRESS = B8(1001010<<1), /*!< IC temperature sensor address if A2=Low, A1=High, A0=Low */
	PMB_TEMP_SENSOR_3_ADDRESS = B8(1001011<<1)  /*!< IC temperature sensor address if A2=Low, A1=High, A0=High */
};

/*! Sets LSB in Address byte to "R" */
#define PMB_READ(addr)  (addr | 0x01)
/*! Sets LSB in Address byte to "/W" */
#define PMB_WRITE(addr) (addr & 0xFE)

/*!
\}
*/ //PMBus_Address_Routines_Defines

/*!
\}
*/ //PMBus_Driver


#endif
