/*************************************************************************
 *
 *    Used with ICCCF and ACF.
 *
 *    (c) Copyright IAR Systems 2008
 *
 *    File name   : interrupt_controller.h
 *    Description : Interrupt Controller module driver for MCF5225x
 *
 *    History :
 *    1. Date        : 19, September 2007
 *       Author      : Stanimir Bonev
 *       Description : Create
 *
 *    2. Date        : 29 September 2008
 *       Author      : Andreas Wallberg
 *       Description : Modified to MCF5282
 *
 *    $Revision: 1208 $
 **************************************************************************/
//#include <string.h>
#include "support_common.h"

#ifndef __INTERRUPT_CONTROLLER_H
#define __INTERRUPT_CONTROLLER_H

/*************************************************************************
 * Function Name: ICM_Init
 * Parameters: void
 *
 * Return: none
 *
 * Description: Init ICM and vector table
 *
 *************************************************************************/
void MCU_Interrupt_Init (void);

/*************************************************************************
 * Function Name: ICM_ConfigureIntr
 * Parameters: vu8 ID, vu8 Level, vu8 Prio, pIntrFunc_t Func
 *
 * Return: pIntrFunc_t
 *
 * Description: Configure an interrupt
 *
 *************************************************************************/
void MCU_ConfigureIntr (unsigned char ID, unsigned char  Level, unsigned char Prio);

/*************************************************************************
 * Function Name: ICM_EnableIntr
 * Parameters: vu8 ID, Boolean State
 *
 * Return: none
 *
 * Description: Enable/Disable interrupt
 *
 *************************************************************************/
void MCU_EnableIntr (unsigned char ID, unsigned char State);

//! запоминает текущий уровень прерывания и запрещает все прерывания
void MCU_irq_disable(void) ;
//! восстанавливает уровень прерывания, запомненный в MCU_irq_disable()
void MCU_irq_restore (void);
//! обёртка к intrinsic
void MCU_irq_blind_disable(void);
//! обёртка к intrinsic
void MCU_irq_blind_enable(void);

/*************************************************************************
 * Interrupts ID
 *************************************************************************/
#define INTR_ID_EPF1       1+64
#define INTR_ID_EPF2       2+64
#define INTR_ID_EPF3       3+64
#define INTR_ID_EPF4       4+64
#define INTR_ID_EPF5       5+64
#define INTR_ID_EPF6       6+64
#define INTR_ID_EPF7       7+64
#define INTR_ID_SWT        8+64
#define INTR_ID_DMA0       9+64
#define INTR_ID_DMA1      10+64
#define INTR_ID_DMA2      11+64
#define INTR_ID_DMA3      12+64
#define INTR_ID_UART0     13+64
#define INTR_ID_UART1     14+64
#define INTR_ID_UART2     15+64
#define INTR_ID_I2S       17+64
#define INTR_ID_QSPI      18+64
#define INTR_ID_DTIM0     19+64
#define INTR_ID_DTIM1     20+64
#define INTR_ID_DTIM2     21+64
#define INTR_ID_DTIM3     22+64

#define INTR_ID_FEC_X_INTF     		23+64
#define INTR_ID_FEC_X_INTB     		24+64
#define INTR_ID_FEC_UN     		25+64
#define INTR_ID_FEC_RL     		26+64
#define INTR_ID_FEC_R_INTF     		27+64
#define INTR_ID_FEC_R_INTB     		28+64
#define INTR_ID_FEC_MII     		29+64
#define INTR_ID_FEC_LC     		30+64
#define INTR_ID_FEC_HBERR     		31+64
#define INTR_ID_FEC_GRA     		32+64
#define INTR_ID_FEC_EBERR     		33+64
#define INTR_ID_FEC_BABT     		34+64
#define INTR_ID_FEC_BABR     		35+64


#define INTR_ID_GPT_OVF   41+64
#define INTR_ID_GPT_PAI   42+64
#define INTR_ID_GPT_PAOVF 43+64
#define INTR_ID_GPT_C0    44+64
#define INTR_ID_GPT_C1    45+64
#define INTR_ID_GPT_C2    46+64
#define INTR_ID_GPT_C3    47+64
#define INTR_ID_GPT_LVD   48+64
#define INTR_ID_ADC_A     49+64
#define INTR_ID_ADC_B     50+64
#define INTR_ID_ADC       51+64
#define INTR_ID_PWM       52+64
#define INTR_ID_USB       53+64
#define INTR_ID_PIT0      55+64
#define INTR_ID_PIT1      56+64
#define INTR_ID_CFM_BE    59+64
#define INTR_ID_CFM_CC    60+64
#define INTR_ID_CFM_PV    61+64
#define INTR_ID_CFM_AE    62+64
#define INTR_ID_RTC       63+64

#endif // __INTERRUPT_CONTROLLER_H
