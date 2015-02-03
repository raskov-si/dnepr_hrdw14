/*!
\file T8_Dnepr_FPGA.c
\brief Driver for Baikal SPI FPGA.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 05.05.2012
\todo Test all read operations.
*/

#include "support_common.h"
#include "HAL\BSP\inc\T8_Dnepr_FPGA.h"
#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "HAL/IC/inc/NXP_PCA9551.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"
#include "prio.h"

/* Pointer to peripherial access stricture and a \c define assigned to it */
static SPI_PeriphInterfaceTypedef* tSpiFpgaInterfaceStructure = NULL;
#define SPI_FPGA_PERIPH_INTERFACE_STRUCTURE tSpiFpgaInterfaceStructure

static _BOOL SPI_FPGA_WriteWord( u16 mAddress, u16 nData );

/*!
\addtogroup SPI_FPGA_Driver
\{
*/

/*!
\defgroup SPI_FPGA_Driver_Exported_Functions
\{
*/

void SPI_FPGA_InitPeripherialInterface(SPI_PeriphInterfaceTypedef* tSpiPeriphInterface){
	/*!
	\brief Inits driver's peripherial interface.
	\details Use it before calling any other function from this driver as it gives this driver access to peripherial interface API.
	\param tSpiPeriphInterface Pointer to SPI_FpgaPeriphInterfaceTypedef structure.
	\warning This driver does not copy tSpiPeriphInterface fields, it stores only this pointer.
	\warning So any changes in this structure outside this driver will also cause changes in this driver's peripherial routines.
	*/	

	assert( tSpiPeriphInterface != NULL );
	SPI_FPGA_PERIPH_INTERFACE_STRUCTURE = tSpiPeriphInterface;
	assert(SPI_FPGA_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction!=NULL);
}

static _BOOL SPI_FPGA_WriteWord( u16 mAddress, u16 nData )
{
	/*!
	\brief FPGA write word routine
	\param mChipSelect Number of SPI Chip Select line.
	\param mAddress 16-bit FPGA register address.
	\param nData 16-bit data to be written into given register.
	\retval 1 if the transaction was successful, 0 otherwise.
	*/
	u8 anBytesToSend[4]={0};

	assert(SPI_FPGA_PERIPH_INTERFACE_STRUCTURE!=NULL);
	assert(SPI_FPGA_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction!=NULL);
	
	/* Two first bytes are address with R/W bit set to Write, two next bytes are data */
	anBytesToSend[0]=_MSB(SPI_FPGA_WRITE(mAddress));
	anBytesToSend[1]=_LSB(SPI_FPGA_WRITE(mAddress));
	anBytesToSend[2]=_MSB(nData);
	anBytesToSend[3]=_LSB(nData);
	
	return SPI_FPGA_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction( anBytesToSend, 4, NULL, 0 );
}

static u16 SPI_FPGA_ReadWord( u16 mAddress )
{
	/*!
	\brief FPGA write word routine
	\param mChipSelect Number of SPI Chip Select line.
	\param mAddress 16-bit FPGA register address.
	\retval Recieved 16-bit value.
	*/
	u8 anBytesToSend[4]={0}, anBytesToGet[4]={0};

	assert(SPI_FPGA_PERIPH_INTERFACE_STRUCTURE!=NULL);
	assert(SPI_FPGA_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction!=NULL);

	/* Two first bytes are address with R/W bit set to Read, two next bytes are dummy bytes (00h) */
	anBytesToSend[0]=_MSB(SPI_FPGA_READ(mAddress));
	anBytesToSend[1]=_LSB(SPI_FPGA_READ(mAddress));
	
	SPI_FPGA_PERIPH_INTERFACE_STRUCTURE->SPI_GenericByteTransaction( anBytesToSend, 4, anBytesToGet, 4 );
	
	return _WORD(anBytesToGet[2], anBytesToGet[3]);
}

static u16 __current_slot_num = 0x8000 ;
_BOOL SPI_FPGA_SelectSlot(u8 nSlotNumber){
	/*!
	\brief Enables given slot's PMBus/I2C bus.
	\param nSlotNumber Number of slot to be chosen (0 through 13 or \c SPI_FPGA_ALL_SLOTS to choose all slots simultaneously).
	\retval 1 если переключение SELECT'ов имело место.
	 */
	u16 nFpgaSelectRegister=SPI_FPGA_SET_SLOT(nSlotNumber);
	_BOOL ret ;

	// если слот уже выбран ничего не делаем
	if( __current_slot_num == nSlotNumber ){
		ret = FALSE ;
		goto select_slot_ret ;
	}
	//Invalid slot number
	if( (nSlotNumber>(SPI_FPGA_SLOTS_NUMBER-1)) && (nSlotNumber!=SPI_FPGA_ALL_SLOTS) ){
		ret = FALSE ;
		goto select_slot_ret ;
	}
	__current_slot_num = nSlotNumber ;
	//All slots
	if( nSlotNumber==SPI_FPGA_ALL_SLOTS ){
		nFpgaSelectRegister=SPI_FPGA_SELECT_ALL;
	}

	SPI_FPGA_WriteWord( SPI_FPGA_SELECT, nFpgaSelectRegister );
	ret = TRUE ;
select_slot_ret:	
	return ret ;
}

u16 SPI_FPGA_Get_SelectSlot()
{
	/*!
	\brief Return enabled slot's PMBus/I2C 
	\retval two in power of enabled slot number
	 */
	u16 sel_slot ;
	
	sel_slot = SPI_FPGA_ReadWord( SPI_FPGA_SELECT );

	return sel_slot;
}

u16 SPI_FPGA_GetIrqSr(void){
	/*!
	\brief Gets IRQ Status register value.
	\retval IRQ Status register value. 
	*/
	u16 res ;

	res = SPI_FPGA_ReadWord( SPI_FPGA_IRQ_SR );

	return res ;
}

u16 SPI_FPGA_GetPresent(void){
	/*!
	\brief Gets Present FPGA register value.
	\retval Present FPGA register value.
	*/
	u16 res ;

	res = SPI_FPGA_ReadWord( SPI_FPGA_PRESENT );

	return res ;
}

/*!
\}
*/ //SPI_FPGA_Driver_Exported_Functions

/*!
\}
*/ //SPI_FPGA_Driver
