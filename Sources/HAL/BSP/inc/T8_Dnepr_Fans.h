/*!
\file T8_Dnepr_Fans.h
\brief ��������� ��� ������ � ������ ����������
\author <a href="mailto:leonov@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#ifndef __DNEPR_FANS_H_
#define __DNEPR_FANS_H_

#define	DNEPR_FANS_MAX_FANS		6

#define DNEPR_FANS_VENDOR_LEN	16
#define DNEPR_FANS_HW_LEN		32
#define DNEPR_FANS_PT_LEN		32
#define DNEPR_FANS_SR_LEN		32
#define DNEPR_FANS_PARTNUM_LEN	32

// �� ������ ������ eeprom ����� ��������� ������������� ���������
#define	Dnepr_Fans_CalData_t_STARTADDRESS		0x8000

#define DNEPR_FANS_DATA_VERSION					0x01

//! ������, �������� � eeprom ����� ����������, ����������� 
//! � ���������������� ������������
typedef struct {
	//! ����� ������ ���������
	u8 version ;
	//! �������� � ��� ��� ���������� 40% � ������� �����������
	u16 pwm40_rpm[ DNEPR_FANS_MAX_FANS ];
	//! �������� � ��� ��� ���������� 60% � ������� �����������
	u16 pwm60_rpm[ DNEPR_FANS_MAX_FANS ];
	//! �������� � ��� ��� ���������� 70% � ������� �����������
	u16 pwm80_rpm[ DNEPR_FANS_MAX_FANS ];
	//! �������� � ��� ��� ���������� 40% � ������� �����������
	u16 pwm100_rpm[ DNEPR_FANS_MAX_FANS ];

	u16	max_rpm ;
	u16 min_rpm ;

	// ������� ����� ����� ���������� �� ������ �� ���������� ������� �����������
	u32 max_work_hrs ;

	// ���������� ������������
	u8 fans_num ;

	u8 vendor[ DNEPR_FANS_VENDOR_LEN ]; // ��� �������, �������� �������������
	u8 hw_number[ DNEPR_FANS_HW_LEN ];	// ��� �������, ������ ���������� �����
	u8 pt_number[ DNEPR_FANS_PT_LEN ];	// ��� �������, ����������
	u8 sr_number[ DNEPR_FANS_SR_LEN ];	// ��� �������, �������� �����

	u8 fans_partnumber[ DNEPR_FANS_PARTNUM_LEN ]; // �������� ������������
	u32 crc ;
} Dnepr_Fans_CalData_t ;


//! ������ ���������� �� ����� ����������
_BOOL	Dnepr_Fans_ReadCalibration();

//! \brief ���������� ����������
_BOOL	Dnepr_Fans_Calibrate( const Dnepr_Fans_CalData_t* calib_data );

//! ������������� ������� ��� ���� ������������
_BOOL	Dnepr_Fans_SetRPM( const u32 rpm );

//! �������� ������� ������� ����������� ����� fan_num (�� 1 �� 6)
u16 	Dnepr_Fans_GetRPM( const u32 fan_num );

//! ��������� �� ���������� �� ����� ����������
_BOOL Dnepr_Fans_Calibrated();

//! ���������� ������ ����������, ����������� � ����� ����������
Dnepr_Fans_CalData_t *Dnepr_Fans_CalData();

//! �������������� �����������, �� �� �������� ��
void Dnepr_Fans_Init( );

#endif
