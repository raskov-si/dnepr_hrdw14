/*!
\file T8_SFP.c
\brief SFF-8472 Rev. 11.0 compatible SFP driver.
\date 23.05.2010 / 20.06.2012
\author Konstantin Tyurin, refactored by Daniel Leonov (<a href="mailto:leonov@t8.ru">leonov@t8.ru</a>)
\warning Uses logarythmic calculations (uW to dBm conversions), may require quite large stack while multitasking.
*/

#include "HAL\IC\inc\T8_SFP.h"
#include <math.h>
// костыль, потому что нужно OSTimeDly
#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "HAL\MCU\inc\I2C_GenericDriver.h" //Remove!

/* Private functions */
static ReturnStatus T8_SFP_ReadString(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_STR16* pTxt, u32 addr);
static ReturnStatus T8_SFP_ReadRxCoefficients( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* rx_pwr0, f32* rx_pwr1, f32* rx_pwr2, f32* rx_pwr3, f32* rx_pwr4, u8 sfp_id);
static f32          T8_SFP_uW2dBm(f32 uw);

/*!
\ingroup I2C_SFP_Driver
\defgroup I2C_SFP_Driver_Exported_Functions
\{
*/

ReturnStatus T8_SFP_GetSfpVolatileValues(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_OPTICAL_CHANNEL* och){
	/*!
	\brief Reads all dynamic values from SFP module (temperature, rx/tx power, voltages, currents etc.).
	\details Use it to refresh values loaded with \ref T8_SFP_Init().
	\param och Target structure (see \c \ref T8_SFP_OPTICAL_CHANNEL).
	\warning Use only after static values were loaded (see \ref T8_SFP_Init()).
	\retval See \ref ReturnStatus.
	*/
	ReturnStatus flag;
	
	flag  = T8_SFP_GetAlarmWarningFlags(tI2cPeriphInterface, och);
	flag |= T8_SFP_GetAlarmParameters( och );
	
	flag |= T8_SFP_TxPower(tI2cPeriphInterface, &och->tx_pwr, &och->sfp_info);
	flag |= T8_SFP_RxPower(tI2cPeriphInterface, &och->rx_pwr, &och->sfp_info);
	flag |= T8_SFP_TxBiasCurrent(tI2cPeriphInterface, &och->itxld, &och->sfp_info);
	flag |= T8_SFP_Temperature(tI2cPeriphInterface, &och->ttx, &och->sfp_info);
	flag |= T8_SFP_Vcc(tI2cPeriphInterface, &och->vcc, &och->sfp_info);
	
	return flag;	
}

//UAV: Init SFP & read static params
ReturnStatus T8_SFP_Init( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_OPTICAL_CHANNEL* och, u32 i){
	/*!
	\brief Reads all static data from SFP module (coefficients, P/N, S/N, vendor name etc.).
	\param och Target structure (see \c \ref T8_SFP_OPTICAL_CHANNEL).
	\param i Number of SFP module (value of \p number \p och field will be set to (i+1)).
	\retval See \ref ReturnStatus.
	\bug Attempt to load Tx and/or Rx parameters messes up program execution (stack error).
	*/
	OS_CPU_SR  cpu_sr = 0u;

	ReturnStatus flag = ERROR;

	och->number = (u8)(i+1);
	// OS_ENTER_CRITICAL() ;
	//--------- read port 92	
	OSTimeDly(1);
	tI2cPeriphInterface->I2C_ReadByte(tI2cPeriphInterface,T8_SFP_SFPID_I2CADDR, T8_SFP_SFPID_DDM_TYPE_REG,
				&(och->sfp_info.flags));	
	OSTimeDly(1);

	//--------- read from A0 pages PN SN VENDOR
	flag  = T8_SFP_ReadString( tI2cPeriphInterface, &och->sfp_pn,T8_SFP_SFPID_PN);//PN
	OSTimeDly(1);
	flag |= T8_SFP_ReadString( tI2cPeriphInterface, &och->sfp_sn,T8_SFP_SFPID_SN);//SN
	OSTimeDly(1);
	flag |= T8_SFP_ReadString( tI2cPeriphInterface, &och->sfp_vendor,T8_SFP_SFPID_VN);//VENDOR
	OSTimeDly(1);
	flag |= T8_SFP_ReadString( tI2cPeriphInterface, &och->sfp_vendor_rev, T8_SFP_SFPID_REV );//Revision
	OSTimeDly(1);
	och->sfp_vendor_rev.value[4] = 0 ; // на самом деле в поле 4 байта, но почему-то функция читает только 16
	if(flag != OK){
		// OS_EXIT_CRITICAL() ;
		return flag;
	}

	//======== DDM ===========
	if ((och->sfp_info.flags & T8_SFP_DDM_IMPLEMENTED)&& !(och->sfp_info.flags & T8_SFP_DDM_SFF8472_COMPLIANCE)){
		//----------- read wave len -----------------
		
		tI2cPeriphInterface->I2C_ReadWord(tI2cPeriphInterface,T8_SFP_SFPID_I2CADDR, T8_SFP_SFPID_WL,
				(u16*)&(och->sfp_wl));
		OSTimeDly(500);
		
		//---------- Switch to Page A2 -------------
		if (och->sfp_info.flags & T8_SFP_DDM_ADDR_CHANGE){

			if( tI2cPeriphInterface->I2C_WriteByte( tI2cPeriphInterface, 0x00, 0x04, 0x82) ){
				och->sfp_info.sfp_id = (0x50<<1);
			}
			OSTimeDly(500);
		}
		else
			och->sfp_info.sfp_id = (0x51<<1);
		//--------  read static params
		if (och->sfp_info.flags & T8_SFP_DDM_EXTERNALLY){
			flag = T8_SFP_LoadCoefficients( tI2cPeriphInterface, &och->sfp_info);//coeffs
			OSTimeDly(500);
		}
		//read limits
		

		flag |= T8_SFP_TxBiasCurrentThresholds( tI2cPeriphInterface, &och->itxld_ths,&och->sfp_info);
		OSTimeDly(500);
		flag |= T8_SFP_TemperatureThresholds( tI2cPeriphInterface, &och->ttx_ths,&och->sfp_info);
		OSTimeDly(500);
		flag |= T8_SFP_VccThresholds( tI2cPeriphInterface, &och->vcc_ths,&och->sfp_info);
		OSTimeDly(500);
		flag |= T8_SFP_TxPowerThresholds( tI2cPeriphInterface, &och->tx_pwr_ths,&och->sfp_info);
		OSTimeDly(500);
		flag |= T8_SFP_RxPowerThresholds( tI2cPeriphInterface, &och->rx_pwr_ths,&och->sfp_info);
		OSTimeDly(500);
		
	}//IF DDM implemented
	else{
		flag = OK;//DDM no
	}
	
	if(flag == OK)
		och->exist = 1;
	// OS_EXIT_CRITICAL() ;
	return flag;
}

ReturnStatus T8_SFP_GetAlarmWarningFlags( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_OPTICAL_CHANNEL* och){
	/*!
	\brief Gets Alarm Warning flags.
	\param och Target \ref T8_SFP_OPTICAL_CHANNEL structure.
	\retval See \ref ReturnStatus.
	*/
	ReturnStatus flag = OK;
	
	tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_info.sfp_id, T8_SFP_SFPDDM_StatusWarning_REG,
											&(och->StatusWarning));
	tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_info.sfp_id, T8_SFP_SFPDDM_StatusAlarm_REG,
											&(och->StatusAlarm));		
	return flag;
}

ReturnStatus T8_SFP_GetAlarmParameters(T8_SFP_OPTICAL_CHANNEL* och){
	/*!
	\brief New Alarm/Warning detection.
	\param och Target \ref T8_SFP_OPTICAL_CHANNEL structure.
	\retval See \ref ReturnStatus.
	*/
	
	u32 i;
	f32* pParam = &och->rx_pwr;
	u32 a,w,a1,w1;
	u16 warning,alarm;

	a = w = 0;	
	warning = och->StatusWarning >> 6;
	alarm = och->StatusAlarm >> 6;	
	//check Warning & Alarms
	for(i=0;i<T8_SFP_PARAM_N;i++){
//		pParam->par_alarm = ALARM_NORMAL;
		//check warning
		w1 = (u32)(warning & 0x03);//param warning
		if(w1)
//			w = pParam->par_alarm = ALARM_WARNING;
		warning >>= 2;
		//check alarm
		a1 = (u32)(alarm & 0x03);
		if(a1)
//			a = pParam->par_alarm = ALARM_ALARM;			 
		alarm >>= 2;
		pParam++;
	}
	if (a)
		och->par_alarm = SFP_ALARM_ALARM;
	else if (w)
		och->par_alarm = SFP_ALARM_WARNING;
	else
		och->par_alarm = SFP_ALARM_NORMAL;
	
	return OK;
}

static ReturnStatus T8_SFP_ReadRxCoefficients( I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* rx_pwr0, f32* rx_pwr1, f32* rx_pwr2, f32* rx_pwr3, f32* rx_pwr4, u8 sfp_id) {
	/*!
	\brief Loads Rx coefficients.
	\param rx_pwr0, rx_pwr1, rx_pwr2, rx_pwr3, rx_pwr4 Pointers to target coefficient variables.
	\param sfp_id SFP I2C bus address.
	\retval See \ref ReturnStatus.
	*/

	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR0_REG,   (u8*)rx_pwr0,   2);
	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR0_REG+2, (u8*)rx_pwr0+2, 2);	

	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR1_REG,   (u8*)rx_pwr1,   2);	
	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR1_REG+2, (u8*)rx_pwr1+2, 2);

	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR2_REG,   (u8*)rx_pwr2,   2);
	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR2_REG+2, (u8*)rx_pwr2+2, 2);

	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR3_REG,   (u8*)rx_pwr3,   2);
	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR3_REG+2, (u8*)rx_pwr3+2, 2);

	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR4_REG,   (u8*)rx_pwr4,   2);
	tI2cPeriphInterface->I2C_ReadMultipleBytes( tI2cPeriphInterface, sfp_id, T8_SFP_SFPDDM_RX_PWR4_REG+2, (u8*)rx_pwr4+2, 2);
	
	return OK;
}

ReturnStatus T8_SFP_TxBiasCurrent(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, u32* tx_bias, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Gets Tx bias current.
	\param tx_bias Pointer to target Tx bias variable.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Tx bias current value is in mA.
	*/
	u16 tx_bias_reg;
	
	u16 slope_reg;
	s16 offset;
	f32 slope;

	u8 data = och->flags;
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_BIAS_REG,
															&(tx_bias_reg));
		if (data & T8_SFP_DDM_INTERNALLY) {//Internally calibrated
			 *tx_bias = (u32)2*(tx_bias_reg)/1000; 
		}
		else if (data & T8_SFP_DDM_EXTERNALLY) {//Externally calibrated
			slope_reg = och->tx_i_slope;
			offset = och->tx_i_offset;
			slope = 0xFF&slope_reg;
			slope = slope/256 + (slope_reg>>8);
			*tx_bias = (u32)(2*((slope*(s16)tx_bias_reg)/1000+offset));
		}
	}
	return OK;
}


ReturnStatus T8_SFP_TxPower(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface,f32* tx_power, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Gets internally measured Tx power.
	\param tx_power Pointer to a target Tx power variable.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Tx power value is represented in dBm.
	*/
	u16 tx_power_reg;
	u8 data = och->flags;
	
	u16 slope_reg;
	s16 offset;
	f32 slope;

	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {

		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_POWER_REG,
													&(tx_power_reg));	
		if (data & T8_SFP_DDM_INTERNALLY) {//Internally calibrated
			if (tx_power_reg <= 1){
				*tx_power = -40.0;
			}
			else{
				*tx_power = (f32)(10*log10((f32)tx_power_reg) - 40.);
			}
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY) {//Externally calibrated
				slope_reg = och->tx_pwr_slope;
				offset = och->tx_pwr_offset;
				slope = 0xFF & slope_reg;
				slope = slope/256 + (slope_reg>>8);
				*tx_power = slope*tx_power_reg+offset;
				if (*tx_power <= 1.0){
					*tx_power = -40.0; /* NAN */
				}
				else{
					*tx_power = (f32)(10*log10(*tx_power) - 40.); 
				}
			}
		}
	}
	return OK;
}

ReturnStatus T8_SFP_RxPower(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* rx_power, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Gets internally measured Rx power.
	\param rx_power Pointer to a target Rx power variable.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Rx power value is represented in dBm.
	*/
	u16 rx_power_reg;
	u8 data = och->flags;
	f32 rx_pwr0, rx_pwr1, rx_pwr2, rx_pwr3, rx_pwr4;
	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_RX_POWER_REG,
													&(rx_power_reg));
		
		if (data & T8_SFP_DDM_INTERNALLY) { //Internally calibrated
			if(rx_power_reg <= 1){
				*rx_power = -40.0;
			}
			else{
				*rx_power = (f32)(10*log10((f32)rx_power_reg) - 40.);
			}
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY){//Externaly calibrated
				rx_pwr0 = och->rx_pwr0;
				rx_pwr1 = och->rx_pwr1;
				rx_pwr2 = och->rx_pwr2;
				rx_pwr3 = och->rx_pwr3;
				rx_pwr4 = och->rx_pwr4;
				*rx_power = rx_power_reg;
				*rx_power = (f32)(pow(*rx_power, 4)*rx_pwr4 + pow(*rx_power, 3)*rx_pwr3 + \
					pow(*rx_power, 2)*rx_pwr2 + *rx_power*rx_pwr1 + rx_pwr0);
				if (*rx_power <= 1.0){
					*rx_power = -40.0; /* NAN */
				}
				else{
					*rx_power = (f32)(10*log10(*rx_power) - 40.);
				}
			}
		}
	}
	return OK;
}

ReturnStatus T8_SFP_Temperature(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* temp, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Gets internally measured temperature.
	\param temp Pointer to a target temperature variable.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Temperature value is represented in C.
	*/
	u16 temp_reg;
	u8 data = och->flags;
	
	u16 slope_reg;
	s16 offset;
	f32 slope;
	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TEMP_REG,
															(u16*)&(temp_reg));
		
		if (data & T8_SFP_DDM_INTERNALLY) {//Internally calibrated
			*temp = (f32)((f32)temp_reg/256.0);
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY){//Externaly calibrated
				slope_reg = och->t_slope;
				offset = och->t_offset;
				slope = 0xFF&slope_reg;
				slope = slope/256 + (slope_reg>>8);
				*temp = (slope*temp_reg+offset)/256;
			}
		}		
	}
	return OK;
}
ReturnStatus T8_SFP_Vcc(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, f32* temp, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Gets internally measured supply voltage (Vcc).
	\param temp Pointer to a target Vcc variable.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Vcc value is represented in V.
	*/
	s16 temp_reg;
	u8 data = och->flags;
	
	u16 slope_reg;
	s16 offset;
	f32 slope;
	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_VCC_REG,
															(u16*)&(temp_reg));
		if (data & T8_SFP_DDM_INTERNALLY) {//Internally calibrated
			*temp =	(f32)0.0001*temp_reg;
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY){//Externaly calibrated
				slope_reg = och->v_slope;
				offset = och->v_offset;
				slope = 0xFF&slope_reg;
				slope = slope/256 + (slope_reg>>8);
				*temp = (f32)(0.0001*(f32)(slope*temp_reg+offset));
			}
		}
	}
	return OK;
}

ReturnStatus T8_SFP_VccThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_F32* sfp_thresholds, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Loads SFP Vcc thresholds.
	\param sfp_thresholds Pointer to a target Vcc thresholds structure.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Vcc threshold values are represented in V.
	*/
	u16 u16vccthresholds[4] = {0, 0, 0, 0};
	u8 data = och->flags;
	
	u16 slope_reg;
	s16 offset;
	f32 slope;

	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Vcc_LAlarm_REG,
													&(u16vccthresholds[0]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Vcc_LWarning_REG,
													&(u16vccthresholds[1]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Vcc_HWarning_REG,
													&(u16vccthresholds[2]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Vcc_HAlarm_REG,
													&(u16vccthresholds[3]));
			
		if (data & T8_SFP_DDM_INTERNALLY) { //Internally calibrated
			sfp_thresholds->la_threshold = (f32)0.0001*(f32)u16vccthresholds[0];
			sfp_thresholds->lw_threshold = (f32)0.0001*(f32)u16vccthresholds[1];
			sfp_thresholds->hw_threshold = (f32)0.0001*(f32)u16vccthresholds[2];
			sfp_thresholds->ha_threshold = (f32)0.0001*(f32)u16vccthresholds[3];
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY){ //Externally calibrated
				slope_reg = och->v_slope;
				offset = och->v_offset;
				slope = 0xFF & slope_reg;
				slope = slope/256 + (slope_reg>>8);
				sfp_thresholds->la_threshold = (f32)0.0001*(f32)(slope*u16vccthresholds[0]+offset);
				sfp_thresholds->lw_threshold = (f32)0.0001*(f32)(slope*u16vccthresholds[1]+offset);
				sfp_thresholds->hw_threshold = (f32)0.0001*(f32)(slope*u16vccthresholds[2]+offset);
				sfp_thresholds->ha_threshold = (f32)0.0001*(f32)(slope*u16vccthresholds[3]+offset);						
			}
		}
	}
	return OK;	
}

static f32 T8_SFP_uW2dBm(f32 uw){
	/*!
	\brief Converts power from uW to dBm
	\param uw Power in uW.
	\retval Power in dBm.
	\todo Think about making it \c inline.
	*/
	if (uw <= 1.0)
		return -40.0; /* NAN */
	else
		return (f32)( 10*log10(uw)-40. ); 
}

//static f32 T8_SFP_uW2dBm(f32 uw){
//	
//	return uw;
//}

/* #define KDBM .0001 */
ReturnStatus T8_SFP_TxPowerThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_F32* sfp_thresholds, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Loads SPF Tx power thresholds.
	\param tx_power Pointer to a target Tx power thresholds structure.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Tx power threshold values are represented in dBm.
	*/

	u16 u16txpwrthresholds[4] = {0, 0, 0, 0};
	u8 data = och->flags;
	
	u16 slope_reg;
	s16 offset;
	f32 slope;
	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {

		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_PWR_LAlarm_REG,
													&(u16txpwrthresholds[0]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_PWR_LWarning_REG,
													&(u16txpwrthresholds[1]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_PWR_HWarning_REG,
													&(u16txpwrthresholds[2]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_PWR_HAlarm_REG,
													&(u16txpwrthresholds[3]));

		if (data & T8_SFP_DDM_INTERNALLY) { //Internally calibrated
			sfp_thresholds->la_threshold = T8_SFP_uW2dBm((f32)u16txpwrthresholds[0]);
			sfp_thresholds->lw_threshold = T8_SFP_uW2dBm((f32)u16txpwrthresholds[1]);
			sfp_thresholds->hw_threshold = T8_SFP_uW2dBm((f32)u16txpwrthresholds[2]);
			sfp_thresholds->ha_threshold = T8_SFP_uW2dBm((f32)u16txpwrthresholds[3]);
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY){ //Externally calibrated
				slope_reg = och->tx_pwr_slope;
				offset = och->tx_pwr_offset;
				slope = 0xFF & slope_reg;
				slope = slope/256 + (slope_reg>>8);
				sfp_thresholds->la_threshold = T8_SFP_uW2dBm((f32)(slope*u16txpwrthresholds[0]+offset));
				sfp_thresholds->lw_threshold = T8_SFP_uW2dBm((f32)(slope*u16txpwrthresholds[1]+offset));
				sfp_thresholds->hw_threshold = T8_SFP_uW2dBm((f32)(slope*u16txpwrthresholds[2]+offset));
				sfp_thresholds->ha_threshold = T8_SFP_uW2dBm((f32)(slope*u16txpwrthresholds[3]+offset));						
			}
		}
	}
	return OK;	
}
ReturnStatus T8_SFP_RxPowerThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_F32* sfp_thresholds, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Loads SPF Rx power thresholds.
	\param tx_power Pointer to a target Rx power thresholds structure.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Rx power threshold values are represented in dBm.
	*/
	
	ReturnStatus flag=OK;
	
	u16 u16rxpwrthresholds[4] = {0, 0, 0, 0};
	u8 data = och->flags;
	f32 rx_pwr0, rx_pwr1, rx_pwr2, rx_pwr3, rx_pwr4;
	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_RX_PWR_LAlarm_REG,
												&(u16rxpwrthresholds[0]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_RX_PWR_LWarning_REG,
												&(u16rxpwrthresholds[1]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_RX_PWR_HWarning_REG,
												&(u16rxpwrthresholds[2]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_RX_PWR_HAlarm_REG,
												&(u16rxpwrthresholds[3]));
		
		if (data & T8_SFP_DDM_INTERNALLY) { //Internally calibrated
			sfp_thresholds->la_threshold = T8_SFP_uW2dBm((f32)u16rxpwrthresholds[0]); 
			sfp_thresholds->lw_threshold = T8_SFP_uW2dBm((f32)u16rxpwrthresholds[1]);
			sfp_thresholds->hw_threshold = T8_SFP_uW2dBm((f32)u16rxpwrthresholds[2]);
			sfp_thresholds->ha_threshold = T8_SFP_uW2dBm((f32)u16rxpwrthresholds[3]);
		}		
		else{
			if (data & T8_SFP_DDM_EXTERNALLY) {
				rx_pwr0 = och->rx_pwr0;
				rx_pwr1 = och->rx_pwr1;
				rx_pwr2 = och->rx_pwr2;
				rx_pwr3 = och->rx_pwr3;
				rx_pwr4 = och->rx_pwr4;
				//Externally calibrated
				sfp_thresholds->la_threshold = u16rxpwrthresholds[0];
				sfp_thresholds->la_threshold = T8_SFP_uW2dBm((f32)(
					pow(sfp_thresholds->la_threshold, 4)*rx_pwr4 + 
					pow(sfp_thresholds->la_threshold, 3)*rx_pwr3 +
					pow(sfp_thresholds->la_threshold, 2)*rx_pwr2 + 
					sfp_thresholds->la_threshold*rx_pwr1 + rx_pwr0));
				sfp_thresholds->lw_threshold = u16rxpwrthresholds[1];
				sfp_thresholds->lw_threshold = T8_SFP_uW2dBm((f32)( 
					pow(sfp_thresholds->lw_threshold, 4)*rx_pwr4 + 
					pow(sfp_thresholds->lw_threshold, 3)*rx_pwr3 +
					pow(sfp_thresholds->lw_threshold, 2)*rx_pwr2 + 
					sfp_thresholds->lw_threshold*rx_pwr1 + rx_pwr0));
				sfp_thresholds->hw_threshold = u16rxpwrthresholds[2];
				sfp_thresholds->hw_threshold = T8_SFP_uW2dBm((f32)( 
					pow(sfp_thresholds->hw_threshold, 4)*rx_pwr4 + 
					pow(sfp_thresholds->hw_threshold, 3)*rx_pwr3 +
					pow(sfp_thresholds->hw_threshold, 2)*rx_pwr2 + 
					sfp_thresholds->hw_threshold*rx_pwr1 + rx_pwr0));
				sfp_thresholds->ha_threshold = u16rxpwrthresholds[3];
				sfp_thresholds->ha_threshold = T8_SFP_uW2dBm((f32)( 
					pow(sfp_thresholds->ha_threshold,4)*rx_pwr4 + 
					pow(sfp_thresholds->ha_threshold,3)*rx_pwr3 +
					pow(sfp_thresholds->ha_threshold,2)*rx_pwr2 + 
					sfp_thresholds->ha_threshold*rx_pwr1 + rx_pwr0));							
			}
		}				
	}
	else{
		flag = ERROR;
	}
	
	return flag;
}

ReturnStatus T8_SFP_TxBiasCurrentThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_U32* sfp_thresholds, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Loads SFP Tx bias current thresholds.
	\param sfp_thresholds Pointer to a target Tx bias current thresholds structure.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Tx bias current threshold values are represented in mA.
	*/
	u16 u16txbiasthresholds[4] = {0, 0, 0, 0};
	u8 data = och->flags;
	
	u16 slope_reg;
	s16 offset;
	f32 slope;
	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_BIAS_LAlarm_REG,
												&(u16txbiasthresholds[0]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_BIAS_LWarning_REG,
												&(u16txbiasthresholds[1]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_BIAS_HWarning_REG,
												&(u16txbiasthresholds[2]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_BIAS_HAlarm_REG,
												&(u16txbiasthresholds[3]));
		

		if (data & T8_SFP_DDM_INTERNALLY) { //Internally calibrated
			sfp_thresholds->la_threshold = 2*(u32)(u16txbiasthresholds[0])/1000;
			sfp_thresholds->lw_threshold = 2*(u32)(u16txbiasthresholds[1])/1000;
			sfp_thresholds->hw_threshold = 2*(u32)(u16txbiasthresholds[2])/1000;
			sfp_thresholds->ha_threshold = 2*(u32)(u16txbiasthresholds[3])/1000;
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY){ //Externally calibrated

				slope_reg = och->tx_i_slope;
				offset = och->tx_i_offset;
				slope = 0xFF&slope_reg;
				slope = slope/256 + (slope_reg>>8);
				sfp_thresholds->la_threshold = (u32)((2*(slope*(s16)u16txbiasthresholds[0])/1000 + offset));
				sfp_thresholds->lw_threshold = (u32)((2*(slope*(s16)u16txbiasthresholds[1])/1000 + offset));
				sfp_thresholds->hw_threshold = (u32)((2*(slope*(s16)u16txbiasthresholds[2])/1000 + offset));
				sfp_thresholds->ha_threshold = (u32)((2*(slope*(s16)u16txbiasthresholds[3])/1000 + offset));
			}
		}
	}	
	return OK;	
}

ReturnStatus T8_SFP_TemperatureThresholds(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_THRESHOLDS_S32* sfp_thresholds, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Loads SFP temperature thresholds.
	\param sfp_thresholds Pointer to a target temperature thresholds structure.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	\note Temperature threshold values are represented in C.
	*/

	u8 data;
	s16 s16temperaturethresholds[4] = {0, 0, 0, 0};
	data = och->flags;
	
	if ((data & T8_SFP_DDM_IMPLEMENTED) && !(data & T8_SFP_DDM_SFF8472_COMPLIANCE)) {
		
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Temp_LAlarm_REG,
												(u16*)&(s16temperaturethresholds[0]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Temp_LWarning_REG,
												(u16*)&(s16temperaturethresholds[1]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Temp_HWarning_REG,
												(u16*)&(s16temperaturethresholds[2]));
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_Temp_HAlarm_REG,
												(u16*)&(s16temperaturethresholds[3]));
		
		if (data & T8_SFP_DDM_INTERNALLY) { //Internally calibrated
			sfp_thresholds->la_threshold = s16temperaturethresholds[0]/256;
			sfp_thresholds->lw_threshold = s16temperaturethresholds[1]/256;
			sfp_thresholds->hw_threshold = s16temperaturethresholds[2]/256;
			sfp_thresholds->ha_threshold = s16temperaturethresholds[3]/256;
		}
		else{
			if (data & T8_SFP_DDM_EXTERNALLY){ //Externally calibrated
				u16 slope_reg;
				s16 offset;
				f32 slope;
				slope_reg = och->t_slope;
				offset = och->t_offset;
				slope = 0xFF&slope_reg;
				slope = slope/256 + (slope_reg>>8);
				sfp_thresholds->la_threshold = (s32)(slope*s16temperaturethresholds[0]+offset)/256;
				sfp_thresholds->lw_threshold = (s32)(slope*s16temperaturethresholds[1]+offset)/256;
				sfp_thresholds->hw_threshold = (s32)(slope*s16temperaturethresholds[2]+offset)/256;
				sfp_thresholds->ha_threshold = (s32)(slope*s16temperaturethresholds[3]+offset)/256;
			}
		}		
	}
	return OK;
}

static ReturnStatus T8_SFP_ReadString(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_STR16* pTxt, u32 addr){
	/*!
	\brief Loads a string from SFP.
	\param pTxt Pointer to a target \ref T8_SFP_STR16 structure.
	\param addr Terget internal SFP string register address.
	\retval See \ref ReturnStatus.
	*/
	ReturnStatus flag;
	
	if( tI2cPeriphInterface->I2C_ReadMultipleBytes(tI2cPeriphInterface,T8_SFP_SFPID_I2CADDR, (u8)addr, (u8*)pTxt->value, 16) ){
		flag = OK;
		pTxt->value[16] = 0;
	}
	else{
		flag = OK;
		pTxt->value[0] = 0;
	}
	return flag;
}

ReturnStatus T8_SFP_LoadCoefficients(I2C_PeriphInterfaceTypedef* tI2cPeriphInterface, T8_SFP_DDM_INFO* och) {
	/*!
	\brief Loads necessary coefficients from SFP.
	\details Loads tx_i_slope, tx_i_offset, t_slope, t_offset, tx_pwr_slope, tx_pwr_offset, v_slope, v_offset T8_SFP_DDM_INFO fields.
	\param och Pointer to a \ref T8_SFP_DDM_INFO structure.
	\retval See \ref ReturnStatus.
	*/
	ReturnStatus flag=ERROR;
	
	if (och->flags & T8_SFP_DDM_EXTERNALLY) {
	//Externally calibrated
		flag = T8_SFP_ReadRxCoefficients( tI2cPeriphInterface, &och->rx_pwr0, &och->rx_pwr1, &och->rx_pwr2, &och->rx_pwr3, &och->rx_pwr4, och->sfp_id);
		
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_I_SLOPE_REG, &och->tx_i_slope );
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_I_OFFSET_REG,(u16*)&och->tx_i_offset);

		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TEMP_SLOPE_REG,&och->t_slope );
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TEMP_OFFSET_REG,(u16*)&och->t_offset);
		
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_PWR_SLOPE_REG,&och->tx_pwr_slope );
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_TX_PWR_OFFSET_REG,(u16*)&och->tx_pwr_offset);

		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_VCC_SLOPE_REG,&och->v_slope );
		tI2cPeriphInterface->I2C_ReadWord( tI2cPeriphInterface, och->sfp_id, T8_SFP_SFPDDM_VCC_OFFSET_REG,(u16*)&och->v_offset);
	}
	
	return flag;
}

/*!
\}
*/ //I2C_SFP_Driver_Exported_Functions
