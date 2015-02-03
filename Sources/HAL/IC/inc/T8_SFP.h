/*!
\file T8_SFP.h
\brief SFF-8472 Rev. 11.0 compatible SFP driver.
\date 23.05.2010 / 20.06.2012
\author Konstantin Tyurin, refactored by Daniel Leonov (<a href="mailto:leonov@t8.ru">leonov@t8.ru</a>)
*/

#ifndef __T8_SFP_H
#define __T8_SFP_H

//#include "rtos.h"
#include "support_common.h"
#include "ErrorManagement\status_codes.h"
#include "HAL/IC/inc/I2C_interface.h"

#include "ctypes.h"

#define T8_SFP_DDM_I2CADDR   (0x51<<1)
#define T8_SFP_SFPID_I2CADDR (0x50<<1)

/*******************Data Fields � Address A0h*******************/
#define T8_SFP_SFPID_DDM_TYPE_REG     92
#define T8_SFP_DDM_SFF8472_COMPLIANCE 0x80
#define T8_SFP_DDM_IMPLEMENTED        0x40
#define T8_SFP_DDM_INTERNALLY         0x20
#define T8_SFP_DDM_EXTERNALLY         0x10
#define T8_SFP_DDM_RXP_MTYPE          0x08
#define T8_SFP_DDM_ADDR_CHANGE        0x04

#define T8_SFP_SFPID_ID_REG 0x0
#define T8_SFP_SFPID_WL     60
#define T8_SFP_SFPID_SN     68
#define T8_SFP_SFPID_PN     40
#define T8_SFP_SFPID_VN     20
#define T8_SFP_SFPID_REV    56

/***************************************************************/

#define T8_SFP_SFPDDM_TX_BIAS_REG       100
#define T8_SFP_SFPDDM_TX_POWER_REG      102
#define T8_SFP_SFPDDM_RX_POWER_REG      104
#define T8_SFP_SFPDDM_TX_I_SLOPE_REG    76
#define T8_SFP_SFPDDM_TX_I_OFFSET_REG   78
#define T8_SFP_SFPDDM_TX_PWR_SLOPE_REG  80
#define T8_SFP_SFPDDM_TX_PWR_OFFSET_REG 82
#define T8_SFP_SFPDDM_TEMP_REG          96
#define T8_SFP_SFPDDM_TEMP_SLOPE_REG    84
#define T8_SFP_SFPDDM_TEMP_OFFSET_REG   86
#define T8_SFP_SFPDDM_RX_PWR4_REG       56
#define T8_SFP_SFPDDM_RX_PWR3_REG       60
#define T8_SFP_SFPDDM_RX_PWR2_REG       64
#define T8_SFP_SFPDDM_RX_PWR1_REG       68
#define T8_SFP_SFPDDM_RX_PWR0_REG       72
#define T8_SFP_SFPDDM_VCC_REG           98
#define T8_SFP_SFPDDM_VCC_SLOPE_REG     88
#define T8_SFP_SFPDDM_VCC_OFFSET_REG    90

#define T8_SFP_SFPDDM_TX_PWR_HAlarm_REG   24
#define T8_SFP_SFPDDM_TX_PWR_LAlarm_REG   26
#define T8_SFP_SFPDDM_TX_PWR_HWarning_REG 28
#define T8_SFP_SFPDDM_TX_PWR_LWarning_REG 30

#define T8_SFP_SFPDDM_RX_PWR_HAlarm_REG   32
#define T8_SFP_SFPDDM_RX_PWR_LAlarm_REG   34
#define T8_SFP_SFPDDM_RX_PWR_HWarning_REG 36
#define T8_SFP_SFPDDM_RX_PWR_LWarning_REG 38

#define T8_SFP_SFPDDM_BIAS_HAlarm_REG   16
#define T8_SFP_SFPDDM_BIAS_LAlarm_REG   18
#define T8_SFP_SFPDDM_BIAS_HWarning_REG 20
#define T8_SFP_SFPDDM_BIAS_LWarning_REG 22

#define T8_SFP_SFPDDM_Temp_HAlarm_REG   0
#define T8_SFP_SFPDDM_Temp_LAlarm_REG   2
#define T8_SFP_SFPDDM_Temp_HWarning_REG 4
#define T8_SFP_SFPDDM_Temp_LWarning_REG 6

#define T8_SFP_SFPDDM_Vcc_HAlarm_REG   8
#define T8_SFP_SFPDDM_Vcc_LAlarm_REG   10
#define T8_SFP_SFPDDM_Vcc_HWarning_REG 12
#define T8_SFP_SFPDDM_Vcc_LWarning_REG 14

#define T8_SFP_SFPDDM_Status_REG        110
//status of Alalrms & Warnings
#define T8_SFP_SFPDDM_StatusAlarm_REG   112
#define T8_SFP_SFPDDM_StatusWarning_REG 116
/*
 * TMHL.VCHL.IBHL.TXHL.RXHL.--.--.--
 * xxHL - 2 BITS: H.L 
 */

/*
 * TMHL.VCHL.IBHL.TXHL.RXHL.--.--.--
 * xxHL - 2 BITS: H.L 
 */
#define T8_SFP_PARAM_N 5

/*!
\ingroup I2C_SFP_Driver
\defgroup I2C_SFP_Driver_Exported_Types
\{
*/

/*!
\brief Flags.
\note Not currently used in this driver.
*/
typedef enum __T8_SFP_FLAGS{
	T8_SFP_NOCOMPLIANCE_SFF8472=1,    /*!< No SFF8472 compliance */
	T8_SFP_DDM_NOTIMPLEMENTED,        /*!< DDM is not implemented */
	T8_SFP_DDM_INTERNALLY_CALIBRATED, /*!< Internally calibrated */
	T8_SFP_DDM_EXTERNALLY_CALIBRATED  /*!< Externally calibrated */
}T8_SFP_FLAGS;

/*!
\brief Alarm/warning threshold types
\note Not currently used in this driver.
*/
typedef enum __T8_SFP_THRESHOLD_TYPE{
	T8_SFP_HIGH_ALARM,   /*!< Alarm, higher threshold */
	T8_SFP_LOW_ALARM,    /*!< Alarm, lower threshold */
	T8_SFP_HIGH_WARNING, /*!< Warning, higher threshold */
	T8_SFP_LOW_WARNING   /*!< Warning, lower threshold */
}T8_SFP_THRESHOLD_TYPE;

/*!
\brief Internal/external calibration options
\note Not currently used in this driver.
*/
typedef enum __T8_SFP_CALIBRATION{
	T8_SFP_Internal, /*!< Internal calibration */
	T8_SFP_External  /*!< External calibration */
}T8_SFP_CALIBRATION;

/*!
\brief Calibration constants for External Calibration Option.
\details See <a href="ftp://ftp.seagate.com/sff/SFF-8472.PDF">SFF-8472 Rev 11.0</a>, table 3.16 for details.
\todo Add more thorough field details.
 */
typedef struct __T8_SFP_DDM_INFO{
	f32 rx_pwr4; /*!< rx_pwr4 coefficient */
	f32 rx_pwr3; /*!< rx_pwr3 coefficient */
	f32 rx_pwr2; /*!< rx_pwr2 coefficient */
	f32 rx_pwr1; /*!< rx_pwr1 coefficient */
	f32 rx_pwr0; /*!< rx_pwr0 coefficient */
	u16 tx_i_slope; /*!< tx_i_slope coefficient */
	s16 tx_i_offset; /*!< tx_i_offset coefficient */
	u16 tx_pwr_slope; /*!< tx_pwr_slope coefficient */
	s16 tx_pwr_offset; /*!< tx_pwr_offset coefficient */
	u16 t_slope; /*!< t_slope coefficient */
	s16 t_offset; /*!< t_offset coefficient */
	u16 v_slope; /*!< v_slope coefficient */
	s16 v_offset; /*!< v_offset coefficient */
	u8 flags; /*!< Flag register value */
	u8 sfp_id; /*!< SFP I2C address */
} T8_SFP_DDM_INFO;


/*! Floating-point threshold values structure */
typedef struct __T8_SFP_THRESHOLDS_F32{
	f32 ha_threshold; /*!< High alarm threshold */
	f32 hw_threshold; /*!< High warning threshold */
	f32 lw_threshold; /*!< Low warning threshold */
	f32 la_threshold; /*!< Low alarm threshold */	
} T8_SFP_THRESHOLDS_F32;

/*! Signed integer threshold values structure */
typedef struct __T8_SFP_THRESHOLDS_S32{
	s32 ha_threshold; /*!< High alarm threshold */
	s32 hw_threshold; /*!< High warning threshold */
	s32 lw_threshold; /*!< Low warning threshold */
	s32 la_threshold; /*!< Low alarm threshold */	
} T8_SFP_THRESHOLDS_S32;

/*! Unsigned integer threshold values structure */
typedef struct __T8_SFP_THRESHOLDS_U32{
	u32 ha_threshold; /*!< High alarm threshold */
	u32 hw_threshold; /*!< High warning threshold */
	u32 lw_threshold; /*!< Low warning threshold */
	u32 la_threshold; /*!< Low alarm threshold */	
} T8_SFP_THRESHOLDS_U32;


/*! 16-character ASCII string structure */
typedef struct __T8_SFP_STR16{
	s8 value[17]; /*!< 16-character ASCII string */
} T8_SFP_STR16;

typedef enum {
	SFP_ALARM_ALARM,
	SFP_ALARM_WARNING,
	SFP_ALARM_NORMAL
} SFP_PAR_ALARMS ;

/*!
\brief SFP data fields - main structure.
\details See <a href="ftp://ftp.seagate.com/sff/SFF-8472.PDF">SFF-8472 Rev 11.0</a>, tables 3.1, 3.15 and 3.17 for details.
 */
typedef struct __T8_SFP_OPTICAL_CHANNEL{
	T8_SFP_DDM_INFO sfp_info;			/*!< Calibration constants & other DDM info */
	f32 rx_pwr;							/*!< Measured Rx input power */
	f32 tx_pwr;							/*!< Measured Tx output power */
	u32 itxld;							/*!< Laser Diode Current */
	f32 vcc;							/*!< Internally measured supply voltage in transceiver */
	f32 ttx;							/*!< Internally measured module temperature (?) */
	T8_SFP_THRESHOLDS_F32 rx_pwr_ths;	/*!< Tx Power Alarm/Warning Thresholds */
	T8_SFP_THRESHOLDS_F32 tx_pwr_ths;	/*!< Rx Power Alarm/Warning Thresholds */
	T8_SFP_THRESHOLDS_U32 itxld_ths;	/*!< Bias (?) Alarm/Warning Thresholds */
	T8_SFP_THRESHOLDS_F32 vcc_ths;		/*!< Voltage Alarm/Warning Thresholds */
	T8_SFP_THRESHOLDS_S32 ttx_ths;		/*!< Temperature (?) Alarm/Warning Thresholds */
	T8_SFP_STR16 sfp_vendor;      		/*!< SFP vendor name */
	T8_SFP_STR16 sfp_sn;          		/*!< Serial number provided by vendor (ASCII) */
	T8_SFP_STR16 sfp_pn;				/*!< Part number provided by SFP vendor (ASCII) */
	T8_SFP_STR16 sfp_vendor_rev;			/*!< Revision level for part number provided by vendor*/
	u16	sfp_wl;							/*!< Laser wavelength (Passive/Active Cable Specification Compliance) */
	u16 StatusWarning;					/*!< Warning */
	u16 StatusAlarm;					/*!< Alarm */
	SFP_PAR_ALARMS par_alarm;			/*!< ??? */
	u8 exist;							/*!< Whether unit exists or not */
	u8 number;							/*!< Number of optical channel (user-defined) */
} T8_SFP_OPTICAL_CHANNEL;
/*!
\}
*/ //I2C_SFP_Driver_Exported_Types


/* General functions */
ReturnStatus T8_SFP_Init(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_OPTICAL_CHANNEL* och, u32 i);
ReturnStatus T8_SFP_GetSfpVolatileValues(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_OPTICAL_CHANNEL* och);

/* Atomic (lower-level) functions */
ReturnStatus T8_SFP_TxPower(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* tx_power, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_RxPower(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* rx_power, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_TxBiasCurrent(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, u32* tx_bias, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_Temperature(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* temp, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_Vcc(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* temp, T8_SFP_DDM_INFO* och);

ReturnStatus T8_SFP_TxPowerThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_F32* sfp_thresholds, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_RxPowerThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_F32* sfp_thresholds, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_TxBiasCurrentThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_U32* sfp_thresholds, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_TemperatureThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_S32* thresholds, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_VccThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_F32* sfp_thresholds, T8_SFP_DDM_INFO* och);

ReturnStatus T8_SFP_LoadCoefficients(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_DDM_INFO* och);
ReturnStatus T8_SFP_GetAlarmParameters(  T8_SFP_OPTICAL_CHANNEL* och);
ReturnStatus T8_SFP_GetAlarmWarningFlags(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_OPTICAL_CHANNEL* och);

/*!
\}
*/ //I2C_SFP_Driver_Exported_Functions

#endif /* __T8_SFP_H */
