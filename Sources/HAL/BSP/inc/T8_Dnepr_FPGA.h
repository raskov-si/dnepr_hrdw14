/*!
\file SPI_FPGA_Driver.h
\brief Driver for Baikal SPI FPGA.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 04.05.2012
*/

#ifndef __SPI_FPGA_DRIVER_H
#define __SPI_FPGA_DRIVER_H

#include "HAL\IC\inc\SPI_Interface.h"

/*! Boolean type definition */

//Registers
#define SPI_FPGA_LED_SPEED  0x0000
#define SPI_FPGA_LED_STATES 0x0001
#define SPI_FPGA_PS_ON      0x0002
#define SPI_FPGA_SELECT     0x0003
#define SPI_FPGA_PRESENT    0x0008
#define SPI_FPGA_PS_STATUS  0x0009
#define SPI_FPGA_IRQ_SR     0x000A

//SPI_FPGA_PS_ON
#define SPI_FPGA_PS1_ON 0x0001
#define SPI_FPGA_PS2_ON 0x0002
#define SPI_FPGA_IS_PS1_ON(w) ( ( (w)&SPI_FPGA_PS1_ON ) )
#define SPI_FPGA_IS_PS2_ON(w) ( ( (w)&SPI_FPGA_PS2_ON ) >> 1 )
#define SPI_FPGA_SET_PS1_ON_STATE(b) (b)
#define SPI_FPGA_SET_PS2_ON_STATE(b) ((b)<<1)

//SPI_FPGA_PRESENT
#define I2C_FPGA_IS_PRESENT(w, ch) ( w & ( 1<<(ch) ) ) /*! \param ch = 0..13 */
#define SPI_FPGA_nGLOBAL_PG (0x01<<14)

//SPI_FPGA_PS_STATUS
#define SPI_FPGA_PS1_SEATED     0x0001
#define SPI_FPGA_PS1_IN_OK      0x0002
#define SPI_FPGA_PS1_POWER_GOOD 0x0004
#define SPI_FPGA_PS1_FAN_FAIL   0x0008
#define SPI_FPGA_PS1_TEMP_OK    0x0010

#define SPI_FPGA_PS2_SEATED     0x0100
#define SPI_FPGA_PS2_IN_OK      0x0200
#define SPI_FPGA_PS2_POWER_GOOD 0x0400
#define SPI_FPGA_PS2_FAN_FAIL   0x0800
#define SPI_FPGA_PS2_TEMP_OK    0x1000

#define SPI_FPGA_IS_PS1_SEATED(w)     ( ( (w)&SPI_FPGA_PS1_SEATED ) )
#define SPI_FPGA_IS_PS1_IN_OK(w)      ( ( (w)&SPI_FPGA_PS1_IN_OK )>>1 )
#define SPI_FPGA_IS_PS1_POWER_GOOD(w) ( ( (w)&SPI_FPGA_PS1_POWER_GOOD )>>2 )
#define SPI_FPGA_IS_PS1_FAN_FAIL(w)   ( ( (w)&SPI_FPGA_PS1_FAN_FAIL )>>3 )
#define SPI_FPGA_IS_PS1_TEMP_OK(w)    ( ( (w)&SPI_FPGA_PS1_TEMP_OK )>>4 )

#define SPI_FPGA_IS_PS2_SEATED(w)     ( ( (w)&SPI_FPGA_PS2_SEATED )>>8 )
#define SPI_FPGA_IS_PS2_IN_OK(w)      ( ( (w)&SPI_FPGA_PS2_IN_OK )>>9 )
#define SPI_FPGA_IS_PS2_POWER_GOOD(w) ( ( (w)&SPI_FPGA_PS2_POWER_GOOD )>>10 )
#define SPI_FPGA_IS_PS2_FAN_FAIL(w)   ( ( (w)&SPI_FPGA_PS2_FAN_FAIL )>>11 )
#define SPI_FPGA_IS_PS2_TEMP_OK(w)    ( ( (w)&SPI_FPGA_PS2_TEMP_OK )>>12 )

//SPI_FPGA_SELECT
#define SPI_FPGA_SLOTS_NUMBER 14     /*!< Number of backplane slots. */
#define SPI_FPGA_ALL_SLOTS    255    /*!< Could be used as \c SPI_FPGA_SetSelect() argument. */
#define SPI_FPGA_SELECT_ALL   0x3FFF /*!< Use to set all \c SPI_FPGA_SLOTS_NUMBER select GPIO lines to "Enable" state. */
#define SPI_FPGA_SET_SLOT(n)  ( 1<<(n) )

/* R/W bit */
#define SPI_FPGA_READ(b)  ( b | 0x8000 )
#define SPI_FPGA_WRITE(b) ( b & 0x7FFF )

/*!
\defgroup SPI_FPGA_Driver
\{
*/

/*!
\defgroup SPI_FPGA_Driver_Exported_Types
\{
*/


/*!
\}
*/ //SPI_FPGA_Driver_Exported_Types

/*!
\}
*/ //SPI_FPGA_Driver


void SPI_FPGA_InitPeripherialInterface(SPI_PeriphInterfaceTypedef* tSpiPeriphInterface);

u16   SPI_FPGA_GetPresent(void);
u16   SPI_FPGA_GetIrqSr(void);
_BOOL SPI_FPGA_SelectSlot(u8 nSlotNumber);

#endif //__SPI_FPGA_DRIVER_H
