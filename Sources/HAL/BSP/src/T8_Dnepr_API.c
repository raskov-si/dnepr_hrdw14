/*!
\file I2C_Board_Driver.h
\brief Driver for Dnepr I2C peripherials.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 05.05.2012
*/
#include "HAL/MCU/inc/T8_5282_spi.h"

#include "HAL/IC/inc/MV_88E6095.h"
#include "HAL/IC/inc/at45db321d.h"
#include "HAL/IC/inc/ST_M95M01.h"
#include "HAL/IC/inc/TI_TMP112.h"
#include "HAL/IC/inc/TI_UCD9080.h"
#include "HAL/IC/inc/TI_TCA9539.h"

#include "HAL/BSP/inc/T8_Dnepr_I2C.h"
#include "HAL/BSP/inc/T8_Dnepr_GPIO.h"
#include "HAL/BSP/inc/T8_Dnepr_API.h"
#include "HAL/BSP/inc/T8_Dnepr_Ethernet.h"
#include "HAL/BSP/inc/T8_Dnepr_Backplane.h"
#include "HAL/BSP/inc/T8_Dnepr_EdgePort.h"
#include "HAL/BSP/inc/T8_Dnepr_QSPI.h"
#include "HAL/BSP/inc/T8_Dnepr_I2C_Interface_Funcs_templates.h"
#include "HAL/BSP/inc/T8_Dnepr_LED.h"
#include "HAL/BSP/inc/T8_Dnepr_SMI.h"
#include "HAL/BSP/inc/T8_Dnepr_Select.h"
#include "HAL/BSP/inc/T8_Dnepr_BP_CurrentMeasure.h"

#include "Application/inc/T8_Dnepr_TaskSynchronization.h"

#include "uCOS_II.H"
#include <intrinsics.h>
#include "io5282.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TCA9539_PRESENT_ADDR	0xE8

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static I2C_DNEPR_PresentDevicesTypedef tCuPresentDevices;
unsigned int nCuPresentDevicesInits=0;
static TI_TCA9555_PortsTypedef tSfpPinsConfig;

//static SMI_PeriphInterfaceTypedef tSMIPeriphInterface ;
static SPI_PeriphInterfaceTypedef tSpiBPADCInterface ;
static SPI_PeriphInterfaceTypedef tSpiFLASHInterface ;
static SPI_PeriphInterfaceTypedef tSpiEEPROMInterface ;

static _BOOL SPI_BP_ADC_ByteTransaction( 	u8* anDataToSend, u32 tx_len, 
											u8* anDataToGet, u32 rx_len );
static _BOOL SPI_Flash_ByteTransaction( 	u8* anDataToSend, u32 tx_len, 
											u8* anDataToGet, u32 rx_len );
static _BOOL SPI_BPEEPROM_ByteTransaction( 	u8* anDataToSend, u32 tx_len, 
											u8* anDataToGet, u32 rx_len );

/*!
 *\brief Инициализирует всё железо
 *\details Инициализация интерфейсов переферии: spi, i2c mdio etc, и микросхем на этих интерфейсах
 */
void DNEPR_InitPeripherials()
{
	T8_Dnepr_LedStatusTypedef __def_leds = {
							(T8_Dnepr_LedTypedef){GREEN, TRUE},
							(T8_Dnepr_LedTypedef){GREEN, TRUE},
							(T8_Dnepr_LedTypedef){GREEN, TRUE}};
	// TODO: должно быть в конфигах
	u8 switch_mac_addr_default[8] = { 0x00, 0x01, 0x01, 0x02, 0x02, 0x04, 0 } ;

	Dnepr_I2C_init() ;
	
	/////////////////////////////////////////////////////////////////////////
	// Ceть: MDIO: 88E6095
//	tSMIPeriphInterface.SMI_Read 	= &t8_m5282_fec_mdio_read ;
//        tSMIPeriphInterface.SMI_Write	= &t8_m5282_fec_mdio_write ;
//        MV_88E6095_InitPeripheralInterface( &tSMIPeriphInterface ) ;
        MV_88E6095_InitPeripheralInterface( dnepr_smi_get_driver_descriptor() ) ;

	Dnepr_Ethernet_Init( switch_mac_addr_default ) ;

	/////////////////////////////////////////////////////////////////////////
	// SPI

	Dnepr_QSPI_Init() ;

	tSpiFLASHInterface.SPI_GenericByteTransaction = &SPI_Flash_ByteTransaction;
	AT_AT45DB321D_InitPeripherialInterface( &tSpiFLASHInterface );

	tSpiEEPROMInterface.SPI_GenericByteTransaction = &SPI_BPEEPROM_ByteTransaction ;
	ST_M95M01_InitPeripherialInterface( &tSpiEEPROMInterface );

	tSpiBPADCInterface.SPI_GenericByteTransaction = &SPI_BP_ADC_ByteTransaction;	
	Dnepr_BP_Current_Init( &tSpiBPADCInterface );

	/////////////////////////////////////////////////////////////////////////
	// I2C: TCA9555, PCA9544A
	// микросхемы, которые сидят на внутренней шине
	// весь верхнеуровневый доступ реализован внутри этого модуля
	
	TI_TCA9555_InitPeripherialInterface( Dnepr_I2C_Get_I2C_IO_Expander_Driver() );
	T8_Dnepr_Led_Init();

	// включаем направление чтения презентов
	TCA9539_SetDirectionRead( Dnepr_I2C_Get_PMBUS_INT_Driver(), TCA9539_PRESENT_ADDR, 0xFEBF );
	Dnepr_Refresh_Presents(); // перечитываем презенты, чтобы снять прерывание

	// включаем все диоды мигающим
	T8_Dnepr_SetLedStatus( &__def_leds );

	/////////////////////////////////////////////////////////////////////////
	// PMBus: LTC4222 -- в каждом обращении к драйверу отдаётся хендлер шины
	
	// I2C на том же луче: TMP112, DS28CM00, UCD9080

	TI_TMP112_InitPeripherialInterface( Dnepr_I2C_Get_I2C_INT_Driver(), I2C_DNEPR_TMP112_ADDR );
	TI_UCD9080_InitPeripherialInterface( Dnepr_I2C_Get_I2C_INT_Driver(), I2C_DNEPR_UCD9080_ADDR );

	/////////////////////////////////////////////////////////////////////////
	// SFP

	//IO Expander for SFP
    TI_TCA9555_InitPortsStructure(&tSfpPinsConfig);    
    
    tSfpPinsConfig.nOutputP0 = TI_TCA9555_SFP_DEFAULT_STATE_CONFIG_0; //RateSelect="High"
    tSfpPinsConfig.nOutputP1 = TI_TCA9555_SFP_DEFAULT_STATE_CONFIG_1; //the rest are "Low"
    tSfpPinsConfig.nConfigP0 = TI_TCA9555_SFP_DEFAULT_IO_CONFIG_0 ;    //RateSelect and Tx_Disable are outputs
    tSfpPinsConfig.nConfigP1 = TI_TCA9555_SFP_DEFAULT_IO_CONFIG_1;    //the rest are inputs 
	
	TI_TCA9555_WriteConfig( I2C_DNEPR_IO_EXPANDER_ADDRESS, &tSfpPinsConfig );

	/////////////////////////////////////////////////////////////////////////
	// Backplane

	Dnepr_InitSelect() ;
	Dnepr_Backplane_Init();

	// включаем внешние прерывания, когда микросхемы сконфигурированы
	Dnepr_EdgePortInit() ;
}

/*!
\addtogroup I2C_Board_Driver
\{
*/

/*!
\defgroup I2C_Board_Driver_Exported_Functions
\{
*/

_BOOL I2C_DNEPR_ClearPresentDevices(I2C_DNEPR_PresentDevicesTypedef* tPresentDevices)
{
	/*!
	\brief Fills \ref I2C_DNEPR_PresentDevicesTypedef structure with its default values.
	\param tPresentDevices Target structure.
	\retval 1.
	*/
	unsigned char i;
	
	assert(tPresentDevices!=NULL);
	
	for(i=0; i<I2C_DNEPR_NUMBER_OF_SLOTS; i++){
		tPresentDevices->bSlotPresent[i]=0;
	}
	for(i=0; i<I2C_DNEPR_NUMBER_OF_SFP; i++){
		tPresentDevices->bSfpPresent[i]=0;
	}
	nCuPresentDevicesInits++;
	
	return 1;
}

//! \brief перечитывает present'ы в ПЛИС
_BOOL Dnepr_Refresh_Presents()
{
	u16 nPresent = 0 ;
	u32 i ;

	if(!nCuPresentDevicesInits){
		I2C_DNEPR_ClearPresentDevices(&tCuPresentDevices);
	}

	/* Get Present*/
	for( i = 0; i < 3; i++ ){
		if( TCA9539_ReadGPIO( Dnepr_I2C_Get_PMBUS_INT_Driver(), TCA9539_PRESENT_ADDR, &nPresent ) ){
			break ;
		}
	}
	// не удалось прочитать present'ы
	if( i >= 3 ){
		return FALSE ;
	}

	tCuPresentDevices.bSlotPresent[0] = 	(nPresent & 0x0001) == 0 ;
	tCuPresentDevices.bSlotPresent[1] = 	(nPresent & 0x0002) == 0 ;
	tCuPresentDevices.bSlotPresent[2] = 	(nPresent & 0x0004) == 0 ;
	tCuPresentDevices.bSlotPresent[3] = 	(nPresent & 0x0008) == 0 ;
	tCuPresentDevices.bSlotPresent[4] = 	(nPresent & 0x0010) == 0 ;
	tCuPresentDevices.bSlotPresent[5] = 	(nPresent & 0x0020) == 0 ;
	tCuPresentDevices.bSlotPresent[6] = 	(nPresent & 0x0080) == 0 ;
	tCuPresentDevices.bSlotPresent[7] = 	(nPresent & 0x0200) == 0 ;
	tCuPresentDevices.bSlotPresent[8] = 	(nPresent & 0x4000) == 0 ;
	tCuPresentDevices.bSlotPresent[9] = 	(nPresent & 0x2000) == 0 ;
	tCuPresentDevices.bSlotPresent[10] = 	(nPresent & 0x1000) == 0 ;
	tCuPresentDevices.bSlotPresent[11] = 	(nPresent & 0x0800) == 0 ;
	tCuPresentDevices.bSlotPresent[12] = 	(nPresent & 0x0400) == 0 ;
	tCuPresentDevices.bSlotPresent[13] = 	(nPresent & 0x8000) == 0 ;

	return TRUE ;
}


//! \brief перечитывает present'ы SFP
_BOOL Dnepr_Refresh_SFP_Presents()
{
	TI_TCA9555_PortsTypedef tIoPorts;

	if(!nCuPresentDevicesInits){
		I2C_DNEPR_ClearPresentDevices(&tCuPresentDevices);
	}

	/* Get nPresent states from I/O Expander*/
	TI_TCA9555_InitPortsStructure(&tIoPorts);
	TI_TCA9555_ReadInputs(I2C_DNEPR_IO_EXPANDER_ADDRESS, &tIoPorts);
	/* P0=SFP_L, P1=SFP_H */
	if( TI_TCA9555_IS_SFP_L_PRESENT(tIoPorts) ) {
		tCuPresentDevices.bSfpPresent[I2C_DNEPR_SFP_L_INDEX]=1;
	}
	else{
		tCuPresentDevices.bSfpPresent[I2C_DNEPR_SFP_L_INDEX]=0;
	}
	if( TI_TCA9555_IS_SFP_U_PRESENT(tIoPorts) ){
		tCuPresentDevices.bSfpPresent[I2C_DNEPR_SFP_U_INDEX]=1;
	}
	else{
		tCuPresentDevices.bSfpPresent[I2C_DNEPR_SFP_U_INDEX]=0;
	}

	return TRUE ;
}

I2C_DNEPR_PresentDevicesTypedef* I2C_DNEPR_GetPresentDevices(void){
	/*!
	\brief Returns a pointer to \ref I2C_DNEPR_PresentDevicesTypedef structure filled with up-to-date values.
	\retval A pointer to \ref I2C_DNEPR_PresentDevicesTypedef structure;
	*/	
	return &tCuPresentDevices;
}

//! \brief Включает или выключает лазер в обоих sfp
_BOOL I2C_Dnepr_SFP_OnOff( const _BOOL sfp_1_on_, const _BOOL sfp_2_on_ )
{
	_BOOL ret ;
	if(sfp_1_on_ == TRUE )
		tSfpPinsConfig.nOutputP0 &= 0xFF ^ 0x20 ;
	else 
		tSfpPinsConfig.nOutputP0 |= 0x20 ;
	
	if(sfp_2_on_ == TRUE )
		tSfpPinsConfig.nOutputP0 &= 0xFF ^ 0x10 ;
	else 
		tSfpPinsConfig.nOutputP0 |= 0x10 ;

    ret = TI_TCA9555_WriteConfig(I2C_DNEPR_IO_EXPANDER_ADDRESS, &tSfpPinsConfig); 
	return ret ;
}

void I2C_Dnepr_SFP_Renew( T8_SFP_OPTICAL_CHANNEL* sfp_params,
					const _BOOL renew_static_info, const u32 sfp_num )
{
	size_t i = 0 ;
	// 1й, нижний sfp
	if( sfp_num == 0 ){
		if( renew_static_info ){
			for( i = 0;
				(T8_SFP_Init( Dnepr_I2C_Get_I2C_SFP_L_Driver(), sfp_params, sfp_num ) != OK) &&
				i < 3; i++ );
		}
		for( i = 0;
				(T8_SFP_GetSfpVolatileValues( Dnepr_I2C_Get_I2C_SFP_L_Driver(), sfp_params ) != OK) &&
				i < 3; i++ );
	// 2й, верхний sfp
	} else if( sfp_num == 1 ){
		if( renew_static_info ){
			for( i = 0;
				(T8_SFP_Init( Dnepr_I2C_Get_I2C_SFP_U_Driver(), sfp_params, sfp_num ) != OK) &&
				i < 3; i++ );
		}
		for( i = 0;
				(T8_SFP_GetSfpVolatileValues( Dnepr_I2C_Get_I2C_SFP_U_Driver(), sfp_params ) != OK) &&
				i < 3; i++ );

	}
}

_BOOL Dnepr_ReadDS28CM00_Internal(MAX_DS28CM00_ContentsTypedef* tContents)
{
	_BOOL ret ;
	ret = MAX_DS28CM00_ReadContents( Dnepr_Internal_DS28CM00_I2C_handle(), tContents );
	return ret ;
}

/*!
\}
*/ //I2C_Board_Driver_Exported_Functions

/*!
\}
*/ //I2C_Board_Driver

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// прерывание от Core Watchdog timer -- в нём например можно будет смотреть program counter,
// на котором произошло зависание

void isr_Watchdog()
{}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ВАЖНО: spi защищается мьютексом на уровне функций модуля работы с BPEEPROM или
// FPGA из соображений эффективности

//! Делитель системной шины для SPI
#define SPILINE_BPADC_DIV		50
#define SPILINE_FLASH_DIV		2
#define SPILINE_EEPROM_DIV		15

//! интерфейс записи/чтения spi в ПЛИС
static _BOOL SPI_BP_ADC_ByteTransaction( 	u8* anDataToSend, u32 tx_len, 
											u8* anDataToGet, u32 rx_len )
{
	assert( anDataToSend );
	return Dnepr_QSPI_ReadWriteArray( SPI_CS_3, anDataToSend, tx_len, anDataToGet, rx_len, SPILINE_BPADC_DIV, FALSE, FALSE );
}

//! интерфейс записи/чтения spi в Flash
static _BOOL SPI_Flash_ByteTransaction( 	u8* anDataToSend, u32 tx_len, 
											u8* anDataToGet, u32 rx_len )
{
	assert( anDataToSend );
	return Dnepr_QSPI_ReadWriteArray( SPI_CS_0, anDataToSend, tx_len, anDataToGet, rx_len, SPILINE_FLASH_DIV, TRUE, TRUE );
}

//! интерфейс записи/чтения spi в EEPROM на бекплейне
static _BOOL SPI_BPEEPROM_ByteTransaction( 	u8* anDataToSend, u32 tx_len, 
											u8* anDataToGet, u32 rx_len )
{
	assert( anDataToSend );
	return Dnepr_QSPI_ReadWriteArray( SPI_CS_2, anDataToSend, tx_len, anDataToGet, rx_len, SPILINE_EEPROM_DIV, TRUE, TRUE );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// хендлеры шин PMBus и I2C для разных устройств

I2C_PeriphInterfaceTypedef *Dnepr_Internal_DS28CM00_I2C_handle()
{
	return Dnepr_I2C_Get_I2C_INT_Driver() ;
}
