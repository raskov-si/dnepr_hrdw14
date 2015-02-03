/*!
\file T8_Dnepr_Profile_params.h
\brief Static parameters load & store
\author <a href="mailto:baranovm@t8.ru">Mikhail Baranov</a>
\date june 2012
*/

#ifndef __DNEPR_PARAMS_H_
#define __DNEPR_PARAMS_H_

#include "support_common.h"
#include "Profile/Generated/profile_def.h"
#include "Profile/inc/sys.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! �������������� ��������� ������
void Dnepr_params_Init();
//! �������������� ��������� ����� �������� BP EEPROM
void Dnepr_params_Init2_after_EEPROM();
//! ���������� ����� ������ �� UART
u8	Dnepr_Profile_Address(void) ;

//! ���������� �������� �������� (�������) ����������� SFP
typedef enum 
{
	SFP_SETUP = 0,		//!< ����� �� ����� sfp
	USER_SETUP = 1		//!< ��������� ����������
} SFP_PARAMS_SETUP_SOURCE;

//! ���������� ��������, �������� ������ � ��. sfp, �������� � ��� ��� ����������, �����
//! ����� ������������ ��������� ��������� �� ����� �������. ����� �������� ����� 
//! �������������.
void Dnepr_Params_Cancel_sfp_thresholds() ;

//! \brief ���������� ��� ��������� � ����������� sfp, ������������ ������ �� ����� sfp, ���� �����.
//! \note �������� �� ������ threadMeasure! ������ ��� ������ �� ���������� ���� ���� ������ � ���� ������.
//! \todo ������ ��������
void Dnepr_Params_renew_sfp_thresholds();
//! \brief �������� �������� sfp 1
SFP_PARAMS_SETUP_SOURCE Dnepr_Params_SFP1_Setup_source();
//! \brief �������� �������� sfp 2
SFP_PARAMS_SETUP_SOURCE Dnepr_Params_SFP2_Setup_source();

//! \brief ��������� ������ �������� ����� ���. ���������, ��������� sys.c
u32		app_get_dyn_param_color_glbuf(u32 p_id);
//! \brief ��������� ��� ������ �������� ������������� ��������� �� ������ �� ������� �������, ��������� sys.c
u32		app_get_dyn_param_val_appbuf(u32 p_id, void* buff, u32 len);
//! \brief ��������� ��� ������ ���� ������������ ����������, ��������� sys.c
u32		dyn_par_access(PARAM_INDEX* p_ix, void* buff, u32 len);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ����� �� ���� ������

//must be synchronyze with profile
enum _DYN_PAR_ENUM{
	ALARM1_IX  = 0UL,	
	ALARM2_IX,
	BLOCK_COLOR_IX
};

//!!!!!!!!!!!!!!  update if profile change !!!!!!!!!!!!!!
#ifndef DYN_PAR_ALL
#error "DYN_PAR_ALL not defined"
#endif

#define DYN_PAR_SPEC		3
#define DYN_PAR_PROF		6
#define DYN_PAR_LOC			(DYN_PAR_ALL-DYN_PAR_SPEC-DYN_PAR_PROF)

//all Transponder & SFP params
#define DYN_TRSP_PAR_NUM	21
//included SFP params
#define DYN_SFP_PAR_NUM		6
#define OCH_NUMBER			2
//!!!!!!!!!!!!!! end of profile SYNC part !!!!!!!!!!!!!!!!


#define DYN_PARAM_ORG		0
#define DYN_PAR_PROF_ORG	(DYN_PAR_SPEC)
#define DYN_PAR_LOC_ORG		(DYN_PAR_PROF_ORG + DYN_PAR_PROF)

extern u32	APP_PARAM_BUFF;
extern u32	GL_PARAM_BUFF;			//index of param's buffer for CU
extern P32	dyn_param[DYN_PAR_ALL][2];	//BUFFERED copy of DYNAMIC params
extern u32	GL_PARAM_BUFF_RDY[2];	//ready state of two param's buffer
// extern SFP_OPTICAL_CHANNEL sfp_modules[OCH_NUMBER];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __DNEPR_PARAMS_H_
