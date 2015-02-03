/*!
\file TI_PCA9544A.h
\brief Driver for TI PCA9544A I2C switch.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 30.04.2012
*/

#ifndef __TI_PCA9544A_DRIVER_H
#define __TI_PCA9544A_DRIVER_H

#include "support_common.h"
#include "HAL/IC/inc/I2C_interface.h"

/*!
\brief I2C Multiplexer address.
\param hw Hardware configurable pins state (e.g. \c 02h for A2=0, A1=1, A0=0).
*/
#define TI_PCA9544A_ADDRESS(hw) ( 0xE0 & ( hw<<1 ) )


//Control Register
#define TI_PCA9544A_CR_EN  0x04
#define TI_PCA9544A_CR_CH0 0x00
#define TI_PCA9544A_CR_CH1 0x01
#define TI_PCA9544A_CR_CH2 0x02
#define TI_PCA9544A_CR_CH3 0x03
#define TI_PCA9544A_CR_ENABLE(b)      ( b | TI_PCA9544A_CR_EN )
#define TI_PCA9544A_CR_DISABLE(b)     ( b & (~TI_PCA9544A_CR_EN) )
#define TI_PCA9544A_CR_GET_CHANNEL(b) ( b & 0x03 )
#define TI_PCA9544A_CR_INT0 0x10
#define TI_PCA9544A_CR_INT1 0x20
#define TI_PCA9544A_CR_INT2 0x40
#define TI_PCA9544A_CR_INT3 0x80
#define TI_PCA9544A_CR_IS_INT0(b) ( b & TI_PCA9544A_CR_INT0 )
#define TI_PCA9544A_CR_IS_INT1(b) ( b & TI_PCA9544A_CR_INT1 )
#define TI_PCA9544A_CR_IS_INT2(b) ( b & TI_PCA9544A_CR_INT2 )
#define TI_PCA9544A_CR_IS_INT3(b) ( b & TI_PCA9544A_CR_INT3 )

/*!
\defgroup TI_PCA9544A_Driver
\{
*/

/*!
\defgroup TI_PCA9544A_Driver_Exported_Types
\{
*/

/*!
\brief Structure for channel state storage.
\details "1" - channel is enabled, "0" - disabled.
*/
typedef struct __TI_PCA9544A_ChannelStateTypedef{
	u8 bCh0State: 1; /*!< State of channel 0 */
	u8 bCh1State: 1; /*!< State of channel 1 */
	u8 bCh2State: 1; /*!< State of channel 2 */
	u8 bCh3State: 1; /*!< State of channel 3 */
}
TI_PCA9544A_ChannelStateTypedef;

/*!
\brief Structure for channel interrupt state storage.
\details "1" - there is interrupt on respective channel, "0" - no interrupt.
*/
typedef struct __TI_PCA9544A_ChannelInterruptTypedef{
	u8 bInt0State: 1; /*!< State of channel 0 interrupt */
	u8 bInt1State: 1; /*!< State of channel 1 interrupt */
	u8 bInt2State: 1; /*!< State of channel 2 interrupt */
	u8 bInt3State: 1; /*!< State of channel 3 interrupt */
}
TI_PCA9544A_ChannelInterruptTypedef;

/*!
\brief Channel number enumeration.
*/
typedef enum __TI_PCA9544A_ChannelNumberTypedef{
	TI_PCA9544A_Channel0 = 0, /*!< Channel 0 */
	TI_PCA9544A_Channel1 = 1, /*!< Channel 1 */
	TI_PCA9544A_Channel2 = 2, /*!< Channel 2 */
	TI_PCA9544A_Channel3 = 3  /*!< Channel 3 */
}
TI_PCA9544A_ChannelNumberTypedef;

/*!
\}
*/ //TI_PCA9544A_Driver_Exported_Types

/*!
\}
*/ //I2C_SwitchDriver

void TI_PCA9544A_InitChannelStateStructure( TI_PCA9544A_ChannelStateTypedef* tChannelStates);
void TI_PCA9544A_InitChannelInterruptStructure( TI_PCA9544A_ChannelInterruptTypedef* tChannelInterrupts);

_BOOL TI_PCA9544A_EnableChannel( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, u8 nAddress, TI_PCA9544A_ChannelNumberTypedef mChannel);
_BOOL TI_PCA9544A_GetInterruptStates( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, u8 nAddress, TI_PCA9544A_ChannelInterruptTypedef* tChannelInterrupts);

#endif //__TI_PCA9544A_DRIVER_H
