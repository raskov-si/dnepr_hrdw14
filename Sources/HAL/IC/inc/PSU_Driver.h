/*!
\file PSU_Driver.h
\brief Header for Emerson DS650DC-3 driver.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 26.03.2012
*/

#ifndef __PSU_DRIVER_H
#define __PSU_DRIVER_H

#include "support_common.h"
#include "HAL/IC/inc/PMB_interface.h"
#include "HAL/IC/inc/PMBus_Commands.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

//OPERATION
#define PSU_OPERATION_IMMEDIATE_OFF B8(00000000)
#define PSU_OPERATION_SOFT_OFF B8(01000000)
#define PSU_OPERATION_ON B8(10000000)
#define PSU_OPERATION_NEW_STATE(b, st) ( ( b & B8(00111111)) | st )

//COEFFICIENTS
#define PSU_DIRECT_m 0x0001
#define PSU_DIRECT_b 0x0000
#define PSU_DIRECT_R 0x0002

//FAN_CONFIG_1_2
#define PSU_FAN_INSTALLED(ch) ( B8(1000) << ((ch-1)*4) )
#define PSU_FAN_RPM(ch)       ( B8(0100) << ((ch-1)*4) )
#define PSU_FAN_1_TACH_P(ch)  ( B8(0000) << ((ch-1)*4) )
#define PSU_FAN_2_TACH_P(ch)  ( B8(0001) << ((ch-1)*4) )
#define PSU_FAN_3_TACH_P(ch)  ( B8(0010) << ((ch-1)*4) )
#define PSU_FAN_4_TACH_P(ch)  ( B8(0011) << ((ch-1)*4) )

//STATUS_BYTE, STATUS_WORD
#define PSU_STATUS_VOUT              B16(10000000, 00000000)
#define PSU_STATUS_IOUT_POUT         B16(01000000, 00000000)
#define PSU_STATUS_INPUT             B16(00100000, 00000000)
#define PSU_STATUS_MFR               B16(00010000, 00000000)
#define PSU_STATUS_POWER_GOOD        B16(00001000, 00000000)
#define PSU_STATUS_FANS              B16(00000100, 00000000)
#define PSU_STATUS_OTHER             B16(00000010, 00000000)
#define PSU_STATUS_UNKOWN            B16(00000001, 00000000)
#define PSU_STATUS_BUSY              B16(00000000, 10000000)
#define PSU_STATUS_OFF               B16(00000000, 01000000)
#define PSU_STATUS_VOUT_OV           B16(00000000, 00100000)
#define PSU_STATUS_IOUT_OC           B16(00000000, 00010000)
#define PSU_STATUS_VIN_UV            B16(00000000, 00001000)
#define PSU_STATUS_TEMPERATURE       B16(00000000, 00000100)
#define PSU_STATUS_CML               B16(00000000, 00000010)
#define PSU_STATUS_NONE_OF_THE_ABOVE B16(00000000, 00000001)


#define PSU_CHANNEL_ERROR 	      ERROR

#define PSU_PMBUS_STATUS_REG	      0xEF

#define PSU_MFR_MAXLEN                32
#define PSU_MODEL_MAXLEN              32
#define PSU_MODEL_ID_MAXLEN           32
#define PSU_UNIQUE_SERIAL_MAXLEN      32


/*! адреса Main Controller I2C (динамические параметры) для Emerson (Astec) PSU  */
enum PMB_PSU_ADDR1{
	PMB_PSU_ADDR1_00 = 0x78,  /*!< (0x3C << 1) PSU адрес A1=Low, A0=Low */
	PMB_PSU_ADDR1_01 = 0x7A,  /*!< (0x3D << 1) PSU адрес A1=Low, A0=High */
	PMB_PSU_ADDR1_10 = 0x7C,  /*!< (0x3E << 1) PSU адрес A1=High, A0=Low */
	PMB_PSU_ADDR1_11 = 0x7E   /*!< (0x3F << 1) PSU адрес A1=High, A0=High */
};

/*! адреса Main Controller I2C (динамические параметры) для muRata мощьностью более 1000W  */
enum PMB_PSU_ADDR2{
	PMB_PSU_ADDR2_00 = 0xB8,  /*!< (0x5C << 1) PSU адрес A1=Low, A0=Low */
	PMB_PSU_ADDR2_01 = 0xBA,  /*!< (0x5D << 1) PSU адрес A1=Low, A0=High */
	PMB_PSU_ADDR2_10 = 0xBC,  /*!< (0x5E << 1) PSU адрес A1=High, A0=Low */
	PMB_PSU_ADDR2_11 = 0xBE   /*!< (0x5F << 1) PSU адрес A1=High, A0=High */
};

/*! адреса Main Controller I2C (динамические параметры) для muRata мощьностью менее 1000W   */
enum PMB_PSU_ADDR3{
	PMB_PSU_ADDR3_00 = 0xB0,  /*!< (0x58 << 1) PSU адрес A1=Low, A0=Low */
	PMB_PSU_ADDR3_01 = 0xB2,  /*!< (0x59 << 1) PSU адрес A1=Low, A0=High */
	PMB_PSU_ADDR3_10 = 0xB4,  /*!< (0x5A << 1) PSU адрес A1=High, A0=Low */
	PMB_PSU_ADDR3_11 = 0xB6   /*!< (0x5B << 1) PSU адрес A1=High, A0=High */
};


/*! адреса PSU's EEPROM I2C (статические параметры) для Emerson (Astec) PSU, для muRata мощьностью более 1000W  */
enum PMB_FRU_ADDR1{
	PMB_FRU_ADDR1_00 = 0xA8,  /*!< (0x54 << 1) FRU адрес A1=Low, A0=Low   */
	PMB_FRU_ADDR1_01 = 0xAA,  /*!< (0x55 << 1) FRU адрес A1=Low, A0=High  */
	PMB_FRU_ADDR1_10 = 0xAC,  /*!< (0x56 << 1) FRU адрес A1=High, A0=Low  */
	PMB_FRU_ADDR1_11 = 0xAE   /*!< (0x57 << 1) FRU адрес A1=High, A0=High */
};


/*! адреса PSU's EEPROM I2C (статические параметры)  для muRata мощьностью менее 1000W  */
enum PMB_FRU_ADDR2{
	PMB_FRU_ADDR2_00 = 0xA0,  /*!< (0x50 << 1) FRU адрес A1=Low, A0=Low   */
	PMB_FRU_ADDR2_01 = 0xA2,  /*!< (0x51 << 1) FRU адрес A1=Low, A0=High  */
	PMB_FRU_ADDR2_10 = 0xA4,  /*!< (0x52 << 1) FRU адрес A1=High, A0=Low  */
	PMB_FRU_ADDR2_11 = 0xA6   /*!< (0x53 << 1) FRU адрес A1=High, A0=High */
};










/*!
\defgroup PSU_Driver
\{
*/

/*!
\defgroup PSU_Driver_Exported_Types
\{
*/

/*! Data from FRU table. */
typedef struct __PSU_UnitInfoTypedef{
	char sManufacturer[PSU_MFR_MAXLEN]; 			/*!< Manufacturer name */
	char sModel[PSU_MODEL_MAXLEN];      			/*!< Product name */
	char sModelId[PSU_MODEL_ID_MAXLEN]; 			/*!< Model ID */
	char sUniqueSerial[PSU_UNIQUE_SERIAL_MAXLEN];	/*!< Unique Serial Number */
	f32 fPower;                         			/*!< Output power, W */
	f32 fLowIn;                         			/*!< Minimal input, V */
	f32 fHighIn;                         			/*!< Maximum input, V */
	u8 nFruAddress;                     			/*!< I2C FRU address */
	u8 nPsuAddress;                     			/*!< I2C PSU address */
	u8 bPmbusSupport :1;                			/*!< Whether supports PMBus interface */
	u8 bPsmiSupport  :1;                			/*!< Whether supports PSMI interface */
}PSU_UnitInfoTypedef;


//! \brief текущие измерения из БП безотносительно типа связи с ним (PSMI или PMBUS)
typedef struct __PSU_UnitMeasurements 
{
	f32 fTemperature; /*!< Temperature sensors (&deg;C) */
	f32 fVout;        /*!< Output voltage sensors (V) */
	f32 fVin;                                /*!< Input voltage sensor (V) */
	f32 fIout ;        /*!< Output current sensors (A) */
	f32 fIin;                                /*!< Input current sensor (A) */	
	u16 nFanSpeed; /*!< Fan speed sensors (RPM) */
} PSU_UnitMeasurements;

/*!
\}
*/ //PSU_Driver_Exported_Types

/*!
\}
*/ //PSU_Driver

//#define PMB_PSU_ADDR PMB_DS650DC_ADDR_11

#ifndef _BOOL
#define _BOOL u8
#endif

/*I2C/PMBus peripherial interfaces*/
void PSU_InitPeripherialInterface(PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface);

//FRU Routines
_BOOL __PSU_InitUnitInfo (PSU_UnitInfoTypedef* PSU_UnitInfoStructure);
_BOOL __PSU_PrintUnitInfo(PSU_UnitInfoTypedef* PSU_UnitInfoStructure);
_BOOL PSU_Setup(u8 nAddressIndex, PSU_UnitInfoTypedef* unitInfoStruct);
_BOOL PSU_WriteEEPROM( const u8 nUnitNumber, const s8* sManufacturer, const s8* sName,
	const s8* sModel, const s8* sSerial, const s8* sTag, const s8* sFileId, const u16 power );
//! \brief очищает информацию о модуле, вызывать, когда модуль вынут
//! \param номер БП
void PSU_ClearUnit( const u8 nUnitNumber );

//Low-level general functions
_BOOL __PSU_SetControl(_BOOL bState); //Control pin
_BOOL __PSU_StoreDefaultAll(); //STORE_DEFAULT_ALL
_BOOL __PSU_RestoreDefaultAll(); //RESTORE_DEFAULT_ALL
_BOOL __PSU_StoreUserAll(); //STORE_USER_ALL
_BOOL __PSU_RestoreUserAll(); //RESTORE_USER_ALL
_BOOL __PSU_ClearFaults(); //CLEAR_FAULTS

//High-level functions
_BOOL PSU_IsEnabled();
_BOOL PSU_PMB_Enable(u8 nUnitNumber);
_BOOL PSU_PMB_Disable(u8 nUnitNumber);
f32 PSU_ReadVin();
f32 PSU_ReadIin();
f32 PSU_ReadVcap();
f32 PSU_ReadVout();
f32 PSU_ReadIout();
f32 PSU_ReadTemperature();
f32 PSU_ReadIout();
f32 PSU_ReadFanRPM();
f32 PSU_ReadPout();
f32 PSU_ReadPin();

//PSMI
f32 PSU_PSMI_ReadTemp(u8 nUnitNumber, u8 nSensor);
f32 PSU_PSMI_ReadVout(u8 nUnitNumber, u8 nSensor);
f32 PSU_PSMI_ReadVin (u8 nUnitNumber, u8 nSensor);
f32 PSU_PSMI_ReadIout(u8 nUnitNumber, u8 nSensor);
f32 PSU_PSMI_ReadIin (u8 nUnitNumber, u8 nSensor);
f32 PSU_PSMI_ReadPeakIout(u8 nUnitNumber, u8 nSensor);
f32 PSU_PSMI_ReadPeakIin (u8 nUnitNumber, u8 nSensor);
unsigned long PSU_PSMI_ReadFanSpeed(u8 nUnitNumber, u8 nSensor);
unsigned short PSU_PSMI_GetStatusRegister(u8 nUnitNumber);
_BOOL PSU_GetUnitInfo(u8 nAddressIndex, PSU_UnitInfoTypedef* PSU_UnitInfoStructure);

// PMBus

//! \brief читает коэффициенты для преобразования параметров PMBus к физическим величинам
//! \param nUnitNumber номер БП
//! \retval удалось ли прочитать
_BOOL 	PSU_PMBus_ReadCoefficients( const u8 nUnitNumber );
//! \brief меняем руками коэф.-ты
//! \param nUnitNumber номер БП
//! \param m 1/m
//! \param b (x*10^-r -b) 1/m
//! \param r 10^-r
void PSU_PMB_SetCoefficients( 	const u8 nUnitNumber, const s16 m, const s16 b, 
								const s8 r );
//! \brief прочитаны коэф.-ты для этого БП
//! \param nUnitNumber номер БП
_BOOL 	PSU_PMBus_CoeffsLoaded( const u8 nUnitNumber );
//! \brief сбрасываем коэфициенты, когда БП вынимают
void 	PSU_PMBus_CoeffsClear( const u8 nUnitNumber );

f32 PSU_PMBus_ReadTemp(u8 nUnitNumber );
f32 PSU_PMBus_ReadVout(u8 nUnitNumber );
f32 PSU_PMBus_ReadVin (u8 nUnitNumber );
f32 PSU_PMBus_ReadIout(u8 nUnitNumber );
f32 PSU_PMBus_ReadIin (u8 nUnitNumber );
f32 PSU_PMBus_ReadPeakIout(u8 nUnitNumber );
f32 PSU_PMBus_ReadPeakIin (u8 nUnitNumber );
f32 PSU_PMBus_ReadFanSpeed(u8 nUnitNumber );
unsigned short PSU_PMBus_GetStatus(u8 nUnitNumber);

_BOOL PSU_GetUnitMeasurements(u8 nUnitNumber, PSU_UnitMeasurements* tUnitMeasuesStructure);


#endif
