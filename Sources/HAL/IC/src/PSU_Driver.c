/*!
\file PSU_Driver.c
\brief Driver for EMERSON PSU (PSMI and PMBus).
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date ??.04.2012
\todo Implement and test Fan speed setting for PSMI PSU.
\todo Implement all functions for PMBus PSU.
*/

#include "HAL/IC/inc/PSU_Driver.h"
#include "HAL/IC/inc/PSMI_Registers.h"
#include "HAL/IC/inc/FRU_Registers.h"
#include "HAL/IC/inc/IPMI.h"
#include "Binary.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

//! см. DS1200DC-3 datasheet стр. 19-20 и
//! http://pmbus.org/docs/PMBus_Specification_Part_II_Rev_1-1_20070205.pdf 
//! раздел 7, стр. 20-23
typedef struct __PSU_UnitCoefficients
{
	s16 m ;	//!< 1/m
	s16 b ;	//!< -b
	s8 	r ; //!< 10^-r
} PSU_UnitCoefficients;

//! Тип связи с модулем БП
typedef enum {
	PMBUS_UNIT,
	PSMI_UNIT,
	UNKNOWN_UNIT,
	UNSET 			//!< модуль неустановлен
} UnitConnectionType_t;

typedef struct __Multirecord_Header
{
	u8 type ;
	u8 det ;
	u8 len ;
	u8 sum ;
	u8 header_sum ;
} Multirecord_Header_t;

//typedef enum 
//{
//  PSU_PMBUS_INTERFACE = 0,
//  PSU_PSMI_INTERFACE,
//} t_psu_interface_type;

typedef struct {
    u8    maincontroller_adress;
    u8    eeprom_adress;
} t_psu_adresses;



/*! List of unit addresses on the bus. */
static u8 naPsuAddress[4] = {PMB_PSU_ADDR1_00, PMB_PSU_ADDR1_01, PMB_PSU_ADDR1_10, PMB_PSU_ADDR1_11};

static const t_psu_adresses  psu_adresses[3][4] = 
{
/* Emerson (Astec) */
  {
    {PMB_PSU_ADDR1_00, PMB_FRU_ADDR1_00},
    {PMB_PSU_ADDR1_01, PMB_FRU_ADDR1_01},
    {PMB_PSU_ADDR1_10, PMB_FRU_ADDR1_10},
    {PMB_PSU_ADDR1_11, PMB_FRU_ADDR1_11},
  },
/* muRata мощьностью более 1000W  */  
  {
    {PMB_PSU_ADDR2_00, PMB_FRU_ADDR1_00},
    {PMB_PSU_ADDR2_01, PMB_FRU_ADDR1_01},
    {PMB_PSU_ADDR2_10, PMB_FRU_ADDR1_10},
    {PMB_PSU_ADDR2_11, PMB_FRU_ADDR1_11},
  },  
/* muRata мощьностью менее 1000W  */
  {
    {PMB_PSU_ADDR3_00, PMB_FRU_ADDR2_00},
    {PMB_PSU_ADDR3_01, PMB_FRU_ADDR2_01},
    {PMB_PSU_ADDR3_10, PMB_FRU_ADDR2_10},
    {PMB_PSU_ADDR3_11, PMB_FRU_ADDR2_11},
  },  
};

const u8 naEmersonPsuAddress[4]={PMB_PSU_ADDR1_00, PMB_PSU_ADDR1_01, PMB_PSU_ADDR1_10, PMB_PSU_ADDR1_11};
/*! List of FRU EEPROM addresses on the bus. */
const u8 naEmersonFruAddress[4]={PMB_FRU_ADDR1_00, PMB_FRU_ADDR1_01, PMB_FRU_ADDR1_10, PMB_FRU_ADDR1_11};

/*! List of unit addresses on the bus. */
const u8 naMurataPsuAddress[4]={PMB_PSU_ADDR2_00, PMB_PSU_ADDR2_01, PMB_PSU_ADDR2_10, PMB_PSU_ADDR2_11};
/*! List of FRU EEPROM in Murata PSU addresses on the bus. */
const u8 naMurataFruAddress[4]={PMB_FRU_ADDR2_00, PMB_FRU_ADDR2_01, PMB_FRU_ADDR2_10, PMB_FRU_ADDR2_11};

//! какой тип связи поддерживает какой модуль
static UnitConnectionType_t __unit_conn_type[4] = { UNSET, UNSET, UNSET, UNSET };
//! Коэффициенты для чтения параметров в формате Direct
static PSU_UnitCoefficients __pmbus_unit_coefficients[4] ;
//! были ли прочитаны коэф.-ты в __pmbus_unit_coefficients
static _BOOL __pmbus_coefs_read[4] = { FALSE, FALSE, FALSE, FALSE };

/*! List of PMBus-compatible units */
const char saPmbusUnits[][13]={	"DS1200DC-3", "DS1200-3", "\0"};
/*! List of PSMI-compatible units */
const char saPsmiUnits[][13]={	"DS650DC-3", "DS850DC-3", "\0"};

//ip addr add 192.168.180.1/255.255.255.0 broadcast 192.168.1.255 dev eth0

static const struct {  
	char                    *manufacturer_name;
	char                    *name;
	char                    *model;
        u16                     max_power;
        s16                     min_input_voltage;
        s16                     max_input_voltage;
//        t_psu_interface_type    interface_type;
        UnitConnectionType_t    protocol_type;
} psu_known_product[] = 
{
  /* произв,     название,    модель,                   мощность, мин вх нпр(10 мв), макс вх напр(10 мв),  тип протокола */
  {"Murata-PS", "TQ1804",     "D1U3CS-D-850-12-HC3C",   850,      4000,               7200,                 PSMI_UNIT  },
  {"Murata-PS", "M1831",      "D1U3CS-D-1600-12-HC3EC", 1600,     4000,               7200,                 PMBUS_UNIT },
  {"ASTEC",     "DS650DC-3",  "DS650DC-3",              650,      3700,               7500,                 PSMI_UNIT  },
  {"ASTEC",     "DS850DC-3",  "DS850DC-3",              850,      3800,               7500,                 PSMI_UNIT  },
  {"ASTEC",     "DS1200DC-3", "DS1200DC-3",             1200,     2,                  1,                    PMBUS_UNIT },
  {"ASTEC",     "DS1200-3",   "DS1200-3",               1200,     9000,               26400,                PMBUS_UNIT }    //переменка
};


/* Pointer to peripherial access stricture and a \c define assigned to it */
PMB_PeriphInterfaceTypedef* tPsuPeriphInterface = NULL;
#define PMB_PERIPH_INTERFACE_STRUCT_PTR tPsuPeriphInterface

#define PSU_PSMI_TEMP_SENSORS 4
#define PSU_PSMI_VOUT_SENSORS 10 
#define PSU_PSMI_IOUT_SENSORS 10
#define PSU_PSMI_FAN_SPEED_SENSORS 4

/*! Status of PSMI-compliant unit (voltages, temperatures etc.) */
typedef struct __PSMI_UnitStatusTypedef{
	f32 fTemperature[PSU_PSMI_TEMP_SENSORS]; /*!< Temperature sensors (&deg;C) */
	f32 fVout[PSU_PSMI_VOUT_SENSORS];        /*!< Output voltage sensors (V) */
	f32 fVin;                                /*!< Input voltage sensor (V) */
	f32 fIout[PSU_PSMI_VOUT_SENSORS];        /*!< Output current sensors (A) */
	f32 fIin;                                /*!< Input current sensor (A) */
	f32 fPeakIout[PSU_PSMI_VOUT_SENSORS];    /*!< Peak output currents (A) */
	f32 fPeakIin;                            /*!< Peak input currents (A) */
	u16 nFanSpeed[PSU_PSMI_FAN_SPEED_SENSORS]; /*!< Fan speed sensors (RPM) */
} PSMI_UnitStatusTypedef;
static _BOOL PSU_PSMI_GetUnitStatus(u8 nUnitNumber, PSMI_UnitStatusTypedef* tUnitStatusStructure);

/* Для упрощения написания соотв. драйверов */
#define __PMB_WriteByte          (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteByte)
#define __PMB_ReadByte           (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadByte)
#define __PMB_ReadWord           (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadWord)
#define __PMB_WriteWord          (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteWord)
#define __PMB_SendCommand        (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_SendCommand)
#define __PMB_ReadMultipleBytes  (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadMultipleBytes)
#define __PMB_WriteMultipleBytes (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteMultipleBytes)
#define __PMB_ReadMultipleWords  (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_ReadMultipleWords)
#define __PMB_WriteMultipleWords (*PMB_PERIPH_INTERFACE_STRUCT_PTR->PMB_WriteMultipleWords)


/*!
\defgroup PSU_Driver
\{
*/

/*!
\defgroup PSU_Driver_Exported_Functions
\{
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PSU_InitPeripherialInterface(PMB_PeriphInterfaceTypedef* tPmbusPeriphInterface){
	/*!
	\brief Inits driver's peripherial interface.
	\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
	\param tPmbusPeriphInterface Pointer to PMB_PeriphInterfaceTypedef structure.
	\warning This driver does not copy tPmbusPeriphInterface fields, it stores only this pointer.
	\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
	*/	
	assert( tPmbusPeriphInterface != NULL );
	PMB_PERIPH_INTERFACE_STRUCT_PTR = tPmbusPeriphInterface;
}

_BOOL __PSU_InitUnitInfo(PSU_UnitInfoTypedef* PSU_UnitInfoStructure){
	/*!
	\brief Fills fields of the \c PSU_UnitInfoStructure with its default values.
	\param PSU_UnitInfoStructure Target structure.
	\retval 1.
	*/
	PSU_UnitInfoStructure->fPower			=	0.0;
	PSU_UnitInfoStructure->nFruAddress		=	0;
	PSU_UnitInfoStructure->nPsuAddress		=	0;
	PSU_UnitInfoStructure->bPmbusSupport	        =	0;
	PSU_UnitInfoStructure->bPsmiSupport		=	0;
	strncpy( PSU_UnitInfoStructure->sManufacturer,	"None", PSU_MFR_MAXLEN );
	strncpy( PSU_UnitInfoStructure->sModel, 	"None", PSU_MODEL_MAXLEN );
	strncpy( PSU_UnitInfoStructure->sModelId, 	"None", PSU_MODEL_ID_MAXLEN );
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*=============================================================================================================*/
/*!  \brief заполнение области PSU данными из таблицы поддерживаемых блоков питания с известными параметрами

     \return признак опознания устройства по имени
     \retval TRUE, FALSE
     \sa ipmi_known_product
*/
/*=============================================================================================================*/
static _BOOL psu_find_manufacture_and_fullfill_psu_area
(
    const IPMI_FRU                            *fru,               /* [in]  данные о полях в EEPROM */
    const char                                *manufacturer_name, /* [in]  наименование производителя */
    const char                                *model,             /* [in]  полное наименование модели  */ 
    _BOOL                                     power_flag,         /* [in]  FALSE - надо взять мощьность из таблицы констант                          */
    IPMI_MultiRecord_PowerSupplyInformation   *psu_area,          /* [out] указатель для чтения данных Power Supply Information (Record Type 0x00)  */
    UnitConnectionType_t                      *protocol_type      /* [out] тип протокола используемый модулем () */
)
{
  u8  i;
  
  /* если отсутвует область статических данных о мощьности берем ее из сведений о производителе */
  for ( i = 0; i < (sizeof psu_known_product / sizeof psu_known_product[0]) && (fru->header.product_area_offset != 0); i++)
  {
      if (    (strcmp (psu_known_product[i].manufacturer_name, manufacturer_name) == 0)
          &&  (strncmp (psu_known_product[i].model, model, 13) == 0 )    
      )
      {
          /* ватты */
        if ( power_flag == FALSE ) {
            psu_area->overall_capacity                = psu_known_product[i].max_power;
            psu_area->low_end_input_voltage_range_1   = psu_known_product[i].min_input_voltage;
            psu_area->high_end_input_voltage_range_1  = psu_known_product[i].max_input_voltage;  
        }
        
        *protocol_type = psu_known_product[i].protocol_type;                    
        return TRUE;
      }
  }
               
  return power_flag;               
}


/*=============================================================================================================*/
/*! \brief   Читает FRU блока питания с указанным номером и заполняет структуру unitInfoStruct. Определяет тип протокола.

    \details Общая функция для всех блоков питания (PMBus, PSMI). Также может работать с другими блоками питания,
    \details совместимыми со спецификацией Intel IPMI.
    \return признак того что блок питания опознан и поддерживается в данном ПО
    \retval TRUE, FALSE
    \sa  PSU_UnitInfoTypedef,
*/
/*=============================================================================================================*/
_BOOL PSU_Setup
(
  const u8            nUnitNumber,        /*!< [in]  номер блока питания, начиная с 1               */
  PSU_UnitInfoTypedef *unitInfoStruct     /*!< [out] заполняемая прочитанными параметрами структура */
)
{
  s32                                       i, j;
//  u8                                        internal_offset ;
//  u8                                        chassis_offset ;
//  u8                                        board_offset ;
//  u8                                        product_offset ;
//  u8                                        multirecord_offset ;
  IPMI_FRU                                  fru ;
  IPMI_MultiRecordHeader                    multi_header;
  IPMI_MultiRecord_PowerSupplyInformation   psu_area;
  _BOOL                                     ret                     = FALSE ;
  UnitConnectionType_t                      portcl_flag;
  
  assert(PMB_PERIPH_INTERFACE_STRUCT_PTR != NULL);
  
  /* \todo проверяем адрес murata по умолчанию конфигурим нужный ???  */

  
  /* на шине PMBus ищем адреса контроллеров pmbus в зависимости от слота */
  for( j = 0; j < 3 ; j++ )  {
     u32 times = 5;
     u8  tmp;
     
     do {
//        ret = ipmi_get_adress_acknowledge( PMB_PERIPH_INTERFACE_STRUCT_PTR, psu_adresses[j][nUnitNumber-1].maincontroller_adress );
          ret = ipmi_read_common_header( PMB_PERIPH_INTERFACE_STRUCT_PTR, psu_adresses[j][nUnitNumber-1].eeprom_adress, &fru );
     } while ( (ret != TRUE) && (--times != 0) );
      
      if ( (ret == TRUE) )
     { break;  }       
  }
  
  if (j == 3)
  {  return FALSE;  } 
  
  /* выбираем адреса */  
  unitInfoStruct->nFruAddress = psu_adresses[j][nUnitNumber-1].eeprom_adress;
  unitInfoStruct->nPsuAddress = psu_adresses[j][nUnitNumber-1].maincontroller_adress;

  /* пытаемся читать общий для всех устройтсв заголовок, если он читается то считаем адрес доступным */
  if ( ipmi_read_common_header( PMB_PERIPH_INTERFACE_STRUCT_PTR, psu_adresses[j][nUnitNumber-1].eeprom_adress, &fru ) )  {        
        
  /* если есть область product_info, читаем информацию о производителе с eeprom
     если нет то пытаемся считать информацию с контроллера */
    if ( ret = ipmi_read_product_info(PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, unitInfoStruct->nPsuAddress, &fru) )  {
 
      unitInfoStruct->sManufacturer[0] = 0 ;
      strncpy( unitInfoStruct->sManufacturer, (s8*)fru.product_info.manufacturer_name, fru.product_info.manufacturer_name_length );
      unitInfoStruct->sModel[0] = 0 ;
      strncpy( unitInfoStruct->sModel, fru.product_info.model, fru.product_info.model_length );	
      unitInfoStruct->sModelId[0] = 0 ;
      strncpy( unitInfoStruct->sModelId, (s8*)fru.product_info.name, fru.product_info.name_length );        
      unitInfoStruct->sUniqueSerial[0] = 0 ;
      strncpy( unitInfoStruct->sUniqueSerial, (s8*)fru.product_info.asset_tag, fru.product_info.asset_tag_len );                                 
      
      // убираем пробелы в начале
      while( (unitInfoStruct->sModel[0] == 0x20) && (unitInfoStruct->sModel[1] != 0) ){
	  for( i = 1; (unitInfoStruct->sModel[i] != 0) && (i < PSU_MODEL_MAXLEN); ++i ){
		unitInfoStruct->sModel[i-1] = unitInfoStruct->sModel[i] ;
	  }
      }

      while( (unitInfoStruct->sManufacturer[0] == 0x20) && (unitInfoStruct->sManufacturer[1] != 0) ){
	  for( i = 1; (unitInfoStruct->sManufacturer[i] != 0) && (i < PSU_MFR_MAXLEN); ++i ){
		unitInfoStruct->sManufacturer[i-1] = unitInfoStruct->sManufacturer[i] ;
	  }
      }

      while( (unitInfoStruct->sModelId[0] == 0x20) && (unitInfoStruct->sModelId[1] != 0) ){
	  for( i = 1; (unitInfoStruct->sModelId[i] != 0) && (i < PSU_MODEL_ID_MAXLEN); ++i ){
		unitInfoStruct->sModelId[i-1] = unitInfoStruct->sModelId[i] ;
	  }
      }
      
      
      // убираем последние пробелы
      // идём пока не встретим пробел или 0 или кончится длина массива
      for( i = 0; (unitInfoStruct->sModel[i] != 0x20) && (unitInfoStruct->sModel[i] != 0) && (i < PSU_MODEL_MAXLEN); ++i ) {
        continue;
      }
      
      if( (i < PSU_MODEL_MAXLEN) && (unitInfoStruct->sModel[i] == 0x20) ) {
	  unitInfoStruct->sModel[i] = 0 ;
      }
      
      
      for( i = 0; (unitInfoStruct->sManufacturer[i] != 0x20) && (unitInfoStruct->sManufacturer[i] != 0) && (i < PSU_MFR_MAXLEN); ++i ) {
        continue;
      }
      
      if( (i < PSU_MFR_MAXLEN) && (unitInfoStruct->sManufacturer[i] == 0x20) ) {
	  unitInfoStruct->sManufacturer[i] = 0 ;
      }
      

      for( i = 0; (unitInfoStruct->sModelId[i] != 0x20) && (unitInfoStruct->sModelId[i] != 0) && (i < PSU_MODEL_ID_MAXLEN); ++i ) {
        continue;
      }
      
      if( (i < PSU_MODEL_ID_MAXLEN) && (unitInfoStruct->sModelId[i] == 0x20) ) {
	  unitInfoStruct->sModelId[i] = 0 ;
      }
      
            
    } /* if ( ret = (ret && ipmi_read_product_info(PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, unitInfoStruct->nPsuAddress, &fru)) ) */
    
    if (ret == FALSE) {
      fru.header.product_area_offset = 0;
    }
        
   /* если есть область MultiRecord/PSU, читаем статические параметры с eeprom
      если нет, то пытаемся считать информацию с контроллера, */
    /* если считать статические параметры (максимальную мощьность) не удается, но есть product_info
       находим данную модель в списке поддерживаемых и берем максимальную мощность оттуда */
    /* плюс определяем по какому протоколу работает блок pmbus/psmi */
    ret = psu_find_manufacture_and_fullfill_psu_area (
                  &fru,                                    
                  unitInfoStruct->sManufacturer,
                  unitInfoStruct->sModel,  
                  IPMI_Find_n_Read_PSU_MultiArea( PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, unitInfoStruct->nPsuAddress, &fru, &multi_header, &psu_area ),
                  &psu_area,
                  &portcl_flag);
          
    if ( ret == FALSE ) {
        __unit_conn_type[nUnitNumber-1] = UNKNOWN_UNIT ;
	unitInfoStruct->bPmbusSupport = 0;
	unitInfoStruct->bPsmiSupport = 0;      
        return ret;
    }
    
    naPsuAddress[nUnitNumber-1] = psu_adresses[j][nUnitNumber-1].maincontroller_adress;
    __unit_conn_type[nUnitNumber-1] = portcl_flag ;    
    if ( portcl_flag == PMBUS_UNIT) {
        unitInfoStruct->bPmbusSupport = 1;
    } else {
        unitInfoStruct->bPsmiSupport = 0;
    }
          
    unitInfoStruct->fPower  = (f32)psu_area.overall_capacity ;
    unitInfoStruct->fLowIn  = (f32)psu_area.low_end_input_voltage_range_1 / 100. ;
    unitInfoStruct->fHighIn = (f32)psu_area.high_end_input_voltage_range_1 / 100. ;
    			            
  } /* if (ret = ipmi_read_common_header( PMB_PERIPH_INTERFACE_STRUCT_PTR, psu_adresses[j][nUnitNumber-1].eeprom_adress, &fru ) ) */                   

        
  /* пробуем читать общие для блоков питания заголовки из адресов Emerson, 
     если не получилось, читаем IPMI из адресов, специфических для Murata */	
//  unitInfoStruct->nPsuAddress=naEmersonPsuAddress[nUnitNumber-1];
//  unitInfoStruct->nFruAddress=naEmersonFruAddress[nUnitNumber-1];
//  
//  if( IPMI_Read_FRU_Headers( PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, &fru ) ) {
//    
//            ret = IPMI_Find_n_Read_PSU_MultiArea( PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, unitInfoStruct->nPsuAddress, &fru, &multi_header, &psu_area );
//            naPsuAddress[nUnitNumber-1] = naEmersonPsuAddress[nUnitNumber-1];
//  } else {
//            unitInfoStruct->nPsuAddress=naMurataPsuAddress[nUnitNumber-1];
//            unitInfoStruct->nFruAddress=naMurataFruAddress[nUnitNumber-1];
//            
//            if( ret = IPMI_Read_FRU_Headers( PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, &fru ) ) {
//                  naPsuAddress[nUnitNumber-1] = naMurataPsuAddress[nUnitNumber-1];              
//	          ret = IPMI_Find_n_Read_PSU_MultiArea( PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, unitInfoStruct->nPsuAddress, &fru, &multi_header, &psu_area );
//            }
//  } /* if( IPMI_Read_FRU_Headers( PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, &fru ) ) */
//  
//
//  
//            unitInfoStruct->sManufacturer[0] = 0 ;
//    	    strncpy( unitInfoStruct->sManufacturer, (s8*)fru.product_info.manufacturer_name, fru.product_info.manufacturer_name_length );
//      	    unitInfoStruct->sModel[0] = 0 ;
//	    strncpy( unitInfoStruct->sModel, (s8*)fru.product_info.name, fru.product_info.name_length );
//            unitInfoStruct->sModelId[0] = 0 ;
//	    strncpy( unitInfoStruct->sModelId, (s8*)fru.product_info.serial_number, fru.product_info.serial_number_len );
//	    unitInfoStruct->sUniqueSerial[0] = 0 ;
//	    strncpy( unitInfoStruct->sUniqueSerial, (s8*)fru.product_info.asset_tag, fru.product_info.asset_tag_len );
//
//
//        if( !ret ) {
//            return ret ;
//        }
//
//        unitInfoStruct->fPower = (f32)psu_area.overall_capacity ;
//	unitInfoStruct->fLowIn = (f32)psu_area.low_end_input_voltage_range_1 / 100. ;
//	unitInfoStruct->fHighIn = (f32)psu_area.high_end_input_voltage_range_1 / 100. ;
	


//	// убираем пробелы в начале
//	while( (unitInfoStruct->sModel[0] == 0x20) && (unitInfoStruct->sModel[1] != 0) ){
//		for( i = 1; (unitInfoStruct->sModel[i] != 0) && (i < PSU_MODEL_MAXLEN); ++i ){
//			unitInfoStruct->sModel[i-1] = unitInfoStruct->sModel[i] ;
//		}
//	}
//	// убираем последние пробелы
//	// идём пока не встретим пробел или 0 или кончится длина массива
//	for( i = 0; (unitInfoStruct->sModel[i] != 0x20) && (unitInfoStruct->sModel[i] != 0) && (i < PSU_MODEL_MAXLEN); ++i );
//	if( (i < PSU_MODEL_MAXLEN) && (unitInfoStruct->sModel[i] == 0x20) ){
//		unitInfoStruct->sModel[i] = 0 ;
//	}
  
//	// ищем имя модели среди известных БП с PMBus
//	for( i = 0; saPmbusUnits[i][0] != '\0'; i++ ){
//		// вот наша модель
//		if(strncmp( unitInfoStruct->sModel, saPmbusUnits[i], 13) == 0){
//			// вместо измерения коэфф-тов ставим константы. Те, которые читаются дают какую-то ерунду. А эти числа стоят в даташите в примерах
//			PSU_PMB_SetCoefficients( nUnitNumber, 1, 0, 2 );
//
//			__unit_conn_type[nUnitNumber-1] = PMBUS_UNIT ;
//			unitInfoStruct->bPmbusSupport = 1;
//			unitInfoStruct->bPsmiSupport = 0;
//			
//			// читаем мощность в FRU
//			__PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, 1, &internal_offset );
//			__PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, 2, &chassis_offset );
//			__PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, 3, &board_offset );
//			__PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, 4, &product_offset );
//			__PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, unitInfoStruct->nFruAddress, 5, &multirecord_offset );
//
//			return TRUE ;
//		}
//	}
//	// ищем имя модели среди известных БП с PSMI
//	for( i = 0; saPsmiUnits[i][0] != '\0'; i++ ){
//		// вот наша модель
//		if(strncmp( unitInfoStruct->sModel, saPsmiUnits[i], 13) == 0){
//			__unit_conn_type[nUnitNumber-1] = PSMI_UNIT ;
//			unitInfoStruct->bPmbusSupport = 0;
//			unitInfoStruct->bPsmiSupport = 1;
//
//			return TRUE ;
//		}
//	}
//
//	// не нашли ни в одном массиве моделей -- неизвестный модуль
//	__unit_conn_type[nUnitNumber-1] = UNKNOWN_UNIT ;
//	unitInfoStruct->bPmbusSupport = 0;
//	unitInfoStruct->bPsmiSupport = 0;
	return FALSE;
}

_BOOL PSU_WriteEEPROM( const u8 nUnitNumber, const s8* sManufacturer, const s8* sName,
				const s8* sModel, const s8* sSerial, const s8* sTag, const s8* sFileId, const u16 power )
{
	IPMI_FRU fru ;
	IPMI_MultiRecordHeader multi_header;
	IPMI_MultiRecord_PowerSupplyInformation psu_area;

	fru.header.format_version 			= 0x01 ;
	fru.header.internal_area_offset 	= 0x00 ;
	fru.header.chassis_info_area_offset = 0x00 ;
	fru.header.board_area_offset 		= 0x00 ;
	fru.header.product_area_offset 		= 0x01 ;
	fru.header.multirecord_area_offset 	= 0x10 ;

	fru.product_info.format_version 	= 0x01 ;
	fru.product_info.product_area_length = 15 ;
	fru.product_info.language_code 		= 0 ;
	fru.product_info.manufacturer_name_length	= strlen( sManufacturer ) + 1 ;
	strncpy( (s8*)fru.product_info.manufacturer_name, sManufacturer, IPMI_STRINGS_LEN );
	fru.product_info.name_length		= strlen( sName ) + 1 ;
	strncpy( (s8*)fru.product_info.name, sName, IPMI_STRINGS_LEN );
	fru.product_info.model_length		= strlen( sModel)+ 1 ;
	strncpy( (s8*)fru.product_info.model, sModel, IPMI_STRINGS_LEN );
	fru.product_info.serial_number_len		= strlen( sSerial )+ 1 ;
	strncpy( (s8*)fru.product_info.serial_number, sSerial, IPMI_STRINGS_LEN );
	fru.product_info.asset_tag_len		= strlen( sTag )+ 1 ;
	strncpy( (s8*)fru.product_info.asset_tag, sTag, IPMI_STRINGS_LEN );
	fru.product_info.fru_file_id_len		= strlen( sFileId ) + 1 ;
	strncpy( (s8*)fru.product_info.fru_file_id, sFileId, IPMI_STRINGS_LEN );

	multi_header.type_id = IPMI_MRT_POWER_SUPPLY ;
	multi_header.end_n_format = 0 ;
	multi_header.length = sizeof(IPMI_MultiRecord_PowerSupplyInformation) ;
	multi_header.record_checksum = 0 ; // контрольную сумму пока не используем
	multi_header.header_checksum = 0 ;

	// переворачиваем все многобайтовые поля
//	#define SWAP_BYTES_16W(x)   x = (_WORD(_LSB(x), _MSB(x) ))
	psu_area.overall_capacity = power ;
	SWAP_BYTES_16W(	psu_area.overall_capacity );
	psu_area.peak_va = 0 ;
	psu_area.inrush_current = 0 ;
	psu_area.inrush_interval_ms = 0 ;
	psu_area.low_end_input_voltage_range_1 = 0 ;
	psu_area.high_end_input_voltage_range_1 = 0 ;
	psu_area.low_end_input_voltage_range_2 = 0 ;
	psu_area.high_end_input_voltage_range_2 = 0 ;

	psu_area.low_end_input_frequency_range = 0 ;
	psu_area.high_end_input_frequency_range = 0 ;
	psu_area.input_dropout_tolerance_ms = 0 ;
	psu_area.binary_flags = 0 ;
	psu_area.peak_watt = 0 ;
	psu_area.combined_wattage[0] = 0 ;
	psu_area.combined_wattage[1] = 0 ;
	psu_area.combined_wattage[2] = 0 ;
	psu_area.predictive_fail_tach = 0 ;

	return IPMI_Write_FRU( PMB_PERIPH_INTERFACE_STRUCT_PTR, naEmersonFruAddress[nUnitNumber], &fru, &multi_header, &psu_area );
}

void PSU_ClearUnit( const u8 nUnitNumber )
{
	__unit_conn_type[nUnitNumber-1] = UNKNOWN_UNIT ;
	PSU_PMBus_CoeffsClear( nUnitNumber );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_BOOL PSU_PMB_Enable(u8 nUnitNumber){
	/*! 
	\brief Enables power supply unit.
	*/
	//u8 nOperation;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	//nOperation=__PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, nPsuAddress[nUnitNumber-1], PMB_OPERATION);
	//return __PMB_WriteByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, nPsuAddress[nUnitNumber-1], PMB_OPERATION, PSU_OPERATION_NEW_STATE(nOperation, PSU_OPERATION_ON));
	return __PMB_WriteByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_OPERATION, 0x80);
}
_BOOL PSU_PMB_Disable(u8 nUnitNumber){
	/*!
	\brief Disables power supply unit.
	*/

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	//nOperation=__PMB_ReadByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, nPsuAddress[nUnitNumber-1], PMB_OPERATION);
	//return __PMB_WriteByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, nPsuAddress[nUnitNumber-1], PMB_OPERATION, PSU_OPERATION_NEW_STATE(nOperation, PSU_OPERATION_SOFT_OFF));
	return __PMB_WriteByte(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_OPERATION, 0x40);
}

f32 PSU_PSMI_ReadTemp(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets temperature sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Temperature in &deg;C.
	*/
	u16 nTmp;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_TEMP_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_TEMP_SENSOR(nSensor), &nTmp );
	return (f32)nTmp*PSMI_TEMP_SENSOR_COEFFICIENT;
}
f32 PSU_PSMI_ReadVout(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets output voltage sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Voltage in V.
	*/
	u16 nTmp;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_VOUT_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_VOUT_SENSOR(nSensor), &nTmp );
	return (f32)nTmp*PSMI_VOUT_SENSOR_COEFFICIENT;
}
f32 PSU_PSMI_ReadVin(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets input voltage sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Voltage in V.
	\bug Returned value is incorrect - check coefficient.
	*/
	u16 nTmp;
	
	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_VIN_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_VIN_SENSOR(nSensor), &nTmp );
	return (f32)nTmp*PSMI_VIN_SENSOR_COEFFICIENT;
}
f32 PSU_PSMI_ReadIout(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets output current sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Current in A.
	*/
	u16 nTmp;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_IOUT_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_IOUT_SENSOR(nSensor), &nTmp );
	return (f32)nTmp*PSMI_IOUT_SENSOR_COEFFICIENT;
}
f32 PSU_PSMI_ReadIin(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets input current sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Current in A.
	\bug Returned value is incorrect - check coefficient.
	*/
	u16 nTmp;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_IIN_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_IIN_SENSOR(nSensor), &nTmp );
	return (f32)nTmp*PSMI_IIN_SENSOR_COEFFICIENT;
}
f32 PSU_PSMI_ReadPeakIout(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets peak output current sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Current in A.
	*/
	u16 nTmp;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_IOUT_PEAK_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_IOUT_PEAK_SENSOR(nSensor), &nTmp );
	return (f32)nTmp*PSMI_IOUT_PEAK_SENSOR_COEFFICIENT;
}
f32 PSU_PSMI_ReadPeakIin(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets peak input current sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Current in A.
	\bug Returned value is incorrect - check coefficient.
	*/
	u16 nTmp;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_IIN_PEAK_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_IIN_PEAK_SENSOR(nSensor), &nTmp );
	return (f32)nTmp*PSMI_IIN_PEAK_SENSOR_COEFFICIENT;
}
unsigned long PSU_PSMI_ReadFanSpeed(u8 nUnitNumber, u8 nSensor){	
	/*!
	\brief Gets fan speed sensor sensor value.
	\param nUnitNumber Number of I2C unit.
	\param nSensor Number of sensor.
	\retval Fan speed in RPM.
	\todo Check actual coefficient - might be wrong.
	*/
	u16 nTmp;
	
	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	if( nSensor>PSMI_FAN_SPEED_SENSOR_NUMBER ) return PSU_CHANNEL_ERROR;
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_FAN_SPEED_SENSOR(nSensor), &nTmp );

	return (u32)((float)nTmp*PSMI_FAN_SPEED_SENSOR_COEFFICIENT);
}
unsigned short PSU_PSMI_GetStatusRegister(u8 nUnitNumber){
	/*!
	\brief Gets status register value.
	\param nUnitNumber Number of I2C unit.
	\retval Status register value.
	*/
	u16 nTmp;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	__PMB_ReadWord(PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSMI_STATUS_REGISTER, &nTmp );
	return nTmp;
}
_BOOL PSU_PSMI_GetUnitStatus(u8 nUnitNumber, PSMI_UnitStatusTypedef* tUnitStatusStructure){
	/*!
	\brief Gets current PSMI-compatible Unit status (voltages, currents, temperatures etc.).
	\param nUnitNumber Number of I2C unit.
	\param tUnitStatusStructure Target structure.
	\note Understanding of which sensor is related to which input/output/fan etc. is up to user. PSMI protocol only gives access to this sensors and does not determine their layout.
	\retval 1.
	*/
	int i, j ;
	const u32 pause = 10000 ;

	assert(PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL);
	
	for(i=1; i<=4; i++){
		tUnitStatusStructure->fTemperature[i-1]=PSU_PSMI_ReadTemp(nUnitNumber, i);
		for( j = 0; j < pause; ++j){
			j = j ;
		}
	}

	for(i=1; i<=4; i++){
		tUnitStatusStructure->fVout[i-1]=PSU_PSMI_ReadVout(nUnitNumber, i);
		for( j = 0; j < pause; ++j){
			j = j ;
		}
	}

	tUnitStatusStructure->fVin=PSU_PSMI_ReadVin(nUnitNumber, 1);
	for( j = 0; j < pause; ++j){
		j = j ;
	}

	for(i=1; i<=4; i++){
		tUnitStatusStructure->fIout[i-1]=PSU_PSMI_ReadIout(nUnitNumber, i);
		for( j = 0; j < pause; ++j){
			j = j ;
		}
	}
	
	tUnitStatusStructure->fIin=PSU_PSMI_ReadIin(nUnitNumber, 1);
	for( j = 0; j < pause; ++j){
		j = j ;
	}

	for(i=1; i<=4; i++){
		tUnitStatusStructure->fPeakIout[i-1]=PSU_PSMI_ReadPeakIout(nUnitNumber, i);
		for( j = 0; j < pause; ++j){
			j = j ;
		}
	}

	tUnitStatusStructure->fPeakIin=PSU_PSMI_ReadPeakIin(nUnitNumber, 1);
	for( j = 0; j < pause; ++j){
		j = j ;
	}

	for(i=1; i<=4; i++){
		tUnitStatusStructure->nFanSpeed[i-1]=PSU_PSMI_ReadFanSpeed(nUnitNumber, i);
		for( j = 0; j < pause; ++j){
			j = j ;
		}
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PMBus

static f32 __PSU_PMBus_ConverDirectVal( const u8 nUnitNumber, const s32 val )
{
	f32 m, b, r;
	assert( __pmbus_coefs_read[nUnitNumber-1] == TRUE );

	m = __pmbus_unit_coefficients[nUnitNumber-1].m ;
	b = __pmbus_unit_coefficients[nUnitNumber-1].b ;
	r = __pmbus_unit_coefficients[nUnitNumber-1].r ;

	return ((f32)val * pow( 10.0, -r ) - b) / m ;
}

f32 __PSU_PMBus_ConvertLinearVal( u16 val )
{
	s16 n, y ;
	f32 x ;
	// x = y * 2^n = y << n
	//	val's bits 	|---n---| |---------y---------|
	//				7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
	y = (val & 0x400) ? (val | 0xF800) : val & 0x03FF ;
	val >>= 11 ;
	n = ( val & 0x10) ? (val | 0xFFE0) : val & 0x000F ;

	if( n > 0 )
		x = (f32)y * (f32)( 1 << n );
	else
		x = (f32)y / (f32)( 1 << -n );

	return x ;
}

_BOOL PSU_PMBus_ReadCoefficients( const u8 nUnitNumber )
{
	u8 nBuff[8] ;

	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	if(! __PMB_ReadMultipleBytes( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_COEFFICIENTS,
		nBuff, 6 ) )
		return FALSE ;

	__pmbus_unit_coefficients[nUnitNumber-1].m = (nBuff[1] << 8) | nBuff[0] ;
	__pmbus_unit_coefficients[nUnitNumber-1].b = (nBuff[3] << 8) | nBuff[2] ;
	__pmbus_unit_coefficients[nUnitNumber-1].r = nBuff[5] ;
	__pmbus_coefs_read[nUnitNumber-1] = TRUE ;
	return TRUE ;
}

void PSU_PMB_SetCoefficients( 	const u8 nUnitNumber, const s16 m, const s16 b, 
								const s8 r )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );

	__pmbus_unit_coefficients[nUnitNumber-1].m = m ;
	__pmbus_unit_coefficients[nUnitNumber-1].b = b ;
	__pmbus_unit_coefficients[nUnitNumber-1].r = r ;
	__pmbus_coefs_read[nUnitNumber-1] = TRUE ;
}

_BOOL PSU_PMBus_CoeffsLoaded( const u8 nUnitNumber )
{
	return __pmbus_coefs_read[nUnitNumber-1] ;
}

void PSU_PMBus_CoeffsClear( const u8 nUnitNumber )
{
	__pmbus_coefs_read[nUnitNumber-1] = FALSE ;
}

f32 PSU_PMBus_ReadTemp(u8 nUnitNumber )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	u16 val;
//        f32 temp;
        
        OSTimeDly( 100 );
	
        __PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_TEMPERATURE_2, &val );
        
//	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_VOUT, &val );
//	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_IOUT, &val );
	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_VIN, &val );
//	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_IIN, &val );
//	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_FAN_SPEED_1, &val );
//	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSU_PMBUS_STATUS_REG, &val );
//        temp = __PSU_PMBus_ConvertLinearVal(val);
        
//        return temp;
        
	return __PSU_PMBus_ConverDirectVal( nUnitNumber, (s32)val );
}

f32 PSU_PMBus_ReadVout(u8 nUnitNumber )
{	
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	u16 val;
	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_VOUT, &val );

	return __PSU_PMBus_ConverDirectVal( nUnitNumber, (s32)val ) ;
}

f32 PSU_PMBus_ReadIout(u8 nUnitNumber )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	u16 val;
	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_IOUT, &val );

	return __PSU_PMBus_ConverDirectVal( nUnitNumber, (s32)val ) ;
}

f32 PSU_PMBus_ReadVin (u8 nUnitNumber )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	u16 val ;
	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_VIN, &val );
	return __PSU_PMBus_ConvertLinearVal( val );
}

f32 PSU_PMBus_ReadIin (u8 nUnitNumber )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	u16 val ;
	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_IIN, &val );
	return __PSU_PMBus_ConvertLinearVal( val );
}

float PSU_PMBus_ReadFanSpeed(u8 nUnitNumber )
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	u16 val ;
	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PMB_READ_FAN_SPEED_1, &val );
	return __PSU_PMBus_ConvertLinearVal( val );
}

unsigned short PSU_PMBus_GetStatus(u8 nUnitNumber)
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	u16 res ;
	__PMB_ReadWord( PMB_PERIPH_INTERFACE_STRUCT_PTR, naPsuAddress[nUnitNumber-1], PSU_PMBUS_STATUS_REG, &res );
	return res ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PSMI_UnitStatusTypedef __psmi_unit_measues_buffer ; //!< складываем измерения сюда, потом берём из нужных мест числа

_BOOL PSU_GetUnitMeasurements(u8 nUnitNumber, PSU_UnitMeasurements* tUnitMeasuesStructure)
{
	assert( PMB_PERIPH_INTERFACE_STRUCT_PTR!=NULL );
	assert( tUnitMeasuesStructure != NULL );
        
	if( __unit_conn_type[nUnitNumber-1] == PMBUS_UNIT ){
		tUnitMeasuesStructure->fTemperature     = PSU_PMBus_ReadTemp( nUnitNumber );
		tUnitMeasuesStructure->fVout 		= PSU_PMBus_ReadVout( nUnitNumber );
		tUnitMeasuesStructure->fVin 		= PSU_PMBus_ReadVin( nUnitNumber );
		tUnitMeasuesStructure->fIout 		= PSU_PMBus_ReadIout( nUnitNumber );
		tUnitMeasuesStructure->fIin 		= PSU_PMBus_ReadIin( nUnitNumber );
		tUnitMeasuesStructure->nFanSpeed 	= (u16)PSU_PMBus_ReadFanSpeed( nUnitNumber );

		return TRUE ;
	} else if( __unit_conn_type[nUnitNumber-1] == PSMI_UNIT ){
		PSU_PSMI_GetUnitStatus( nUnitNumber, &__psmi_unit_measues_buffer );

		tUnitMeasuesStructure->fTemperature     = __psmi_unit_measues_buffer.fTemperature[1] ;
		tUnitMeasuesStructure->fVout 		= __psmi_unit_measues_buffer.fVout[0]; // 0 -- 12 В, 1 -- 3.3 В
		tUnitMeasuesStructure->fVin 		= __psmi_unit_measues_buffer.fVin ;
		tUnitMeasuesStructure->fIout 		= __psmi_unit_measues_buffer.fIout[0];
		tUnitMeasuesStructure->fIin 		= __psmi_unit_measues_buffer.fIin ;
		tUnitMeasuesStructure->nFanSpeed 	= __psmi_unit_measues_buffer.nFanSpeed[0];

		return TRUE ;
	} else {
		tUnitMeasuesStructure->fTemperature = 0.0;
		tUnitMeasuesStructure->fVout 		= 0.0;
		tUnitMeasuesStructure->fVin 		= 0.0;
		tUnitMeasuesStructure->fIout 		= 0.0;
		tUnitMeasuesStructure->fIin 		= 0.0;
		tUnitMeasuesStructure->nFanSpeed 	= 0;

		return FALSE ;
	}
}

/*!
\}
*/ //PSU_Driver_Exported_Functions

/*!
\}
*/ //PSU_Driver
