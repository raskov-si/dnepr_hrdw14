/*************************************************************************
 *
 *    Used with ICCCF and ACF.
 *
 *    (c) Copyright IAR Systems 2008
 *
 *    File name   : interrupt_controller.c
 *    Description : Interrupt Controller module driver for MCF5282
 *
 *    History :
 *    1. Date        : 19, September 2007
 *       Author      : Stanimir Bonev
 *       Description : Create
 *
 *    2. Date        : 29 September 2008
 *       Author      : Andreas Wallberg
 *       Description : Modified to MCF52259
 *
 *    3. Date        : 30 October 2008
 *       Author      : Frederick
 *       Description : Modified to MCF5282
 *    $Revision: 1208 $
 **************************************************************************/
#include "HAL/MCU/inc/T8_5282_interrupts.h"
#include "support_common.h"
#include <intrinsics.h>
#include "io5282.h"

//! здесь сохраняется текущий IPL при запрете прерываний 
static unsigned int mcu_prev_int_level = 0 ;

/*************************************************************************
 * Function Name: ICM_Init
 * Parameters: void
 *
 * Return: none
 *
 * Description: Init ICM
 *
 *************************************************************************/
void MCU_Interrupt_Init (void)
{
  __disable_interrupts();
  // mask all interrupts и убираем MASCKALL
  MCF_INTC0_IMRH = MCF_INTC0_IMRL = 0xFFFFFFFE;
  MCF_INTC1_IMRH = MCF_INTC1_IMRL = 0xFFFFFFFE;
  // clear all forced interrupts
  MCF_INTC0_INTFRCH = MCF_INTC0_INTFRCL = 0;
  MCF_INTC1_INTFRCH = MCF_INTC1_INTFRCL = 0;
}

/*************************************************************************
 * Function Name: ICM_ConfigureIntr
 * Parameters: vu8 ID, vu8 Level, vu8 Prio, pIntrFunc_t Func
 *
 * Return: pIntrFunc_t
 *
 * Description: Configure an interrupt
 *
 *************************************************************************/
void MCU_ConfigureIntr (unsigned char ID, unsigned char  Level, unsigned char Prio)
{
  assert(ID < 128);
  assert(Level < 8);
  assert(Prio < 8);
  if(ID < 64)
  {
    // Coldfire processor and internal exceptions
  }
  else
  {
    // Peripherals interrupts
    ID -= 64;
    // Set interrupt control register
    *(&MCF_INTC0_ICR01 - 1 + ID) = (Prio | (Level << 3));
  }
}

/*************************************************************************
 * Function Name: ICM_EnableIntr
 * Parameters: vu8 ID, Boolean State
 *
 * Return: none
 *
 * Description: Enable/Disable interrupt
 *
 *************************************************************************/
void MCU_EnableIntr (unsigned char ID, unsigned char State)
{
  if (State)
  {
    // enable interrupt
    if( ID > 96){
      int tmp;
      tmp = MCF_INTC0_IMRH ;
      ID-=96;
      tmp &= ~(0x0001<<ID);
      MCF_INTC0_IMRH = tmp;
    }
    else{
      int tmp;
      tmp = MCF_INTC0_IMRL ;
      ID-=64;
      tmp &= ~(0x0001<<ID);
      tmp &= 0xFFFE;
      MCF_INTC0_IMRL = tmp;
    }
  }
  else
  {
    // disable interrupt
    *((ID < 32)?&MCF_INTC0_IMRL:&MCF_INTC0_IMRH) |=  1UL << (ID & 0x1F);
  }
}

/********************************************************************/
void MCU_irq_disable (void)
{
	unsigned short s = __get_status_register();
	mcu_prev_int_level = (0x0700 & s) >> 8 ;
	__disable_interrupts() ;
}

/********************************************************************/

void MCU_irq_restore (void)
{
//COMM: ASM-function call
	unsigned short s = __get_status_register() ;
	s = s & 0xF8FF ;
	s = ((mcu_prev_int_level << 8)  & 0x0700) | s ;
	__set_status_register( s ) ;
}

/********************************************************************/

void MCU_irq_blind_enable()
{
	__enable_interrupts() ;
}

void MCU_irq_blind_disable()
{
	__disable_interrupts();
}

