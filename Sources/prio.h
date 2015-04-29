/*!
\file prio.h
\brief ���������� ���� ����� � ����������
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date july 2012
*/

// ��������� ����������� ��� ��, ��� ������������ ������������� ��������������� ���������
// ���������.
// ������ ���������� �� �� �������:
// ��������							Lvl		Prio
// 	INTR_ID_EPF1					1		5
// 	EDGEPORT1						1 		0	// SFP_nINT
// 	PIT0 (��������� ������) 		3		0
//	Trap15 (���. ������)			3		1
//  QSPI							5 		1
// 	UART1 (Kontron)					5		2
//	EDGEPORT3						5 		5	// FPGA_nINT
//	EDGEPORT4						5 		6	// I2C_SW_INT

#include "lwip/opt.h" 


#define taskInit_PRIO			20	//!< ��������� ������ ������������� ��������� �����, ����� ������� ���������


#define dyn_param_mutex_PRIO 	12 	//!< ��������� �������� ��� ������������ ������ ������������ ����������
#define spi_mutex_PRIO 			13 	//!< ��������� �������� ��� ������������ ������ ����������
#define i2c_mutex_PRIO 			14 	//!< ��������� �������� ��� ������������ ������ ����������
#define SMI_MUTEX_PRIO                  15      //!< ��������� �������� ��� ����� ���������� PHY (������)
#define ffs_mutex_PRIO 			16 	//!< ��������� �������� ��� ������ ������� hotswap'�
#define hs_mutex_PRIO 			17 	//!< ��������� �������� ��� ������ ������� hotswap'�

#define tackDController_PRIO	30
#define taskCU_PRIO     		31	//!< ��������� ������ ������ � backplane-uart'��

#define TASK_ETH_PRIORITY      (TCPIP_THREAD_PRIO+1)
#define TASK_SNMP_PRIORITY     (TCPIP_THREAD_PRIO+2)

#define TASK_RSRV_PRIORITY            (taskCU_PRIO+1)
#define TASK_RSRV_MCUMCU_PRIORITY     (taskCU_PRIO+2)


#define taskMeasure_PRIO    	50	//!< ��������� ������ ��������� ������������ ����������
#define SyncParam_prio          51	//!< ��������� ������ ��������� ������������ ����������
#define taskFPGA_COMM_PRIO  	52	//!< 
#define taskFlash_COMM_PRIO 	53	//!< 
#define taskFS_COMM_PRIO    	54	//!< 

#define TASKTERM_COMM_PRIO     		(56)	//!< ��������� ������ ������ � ���������� �� backplane

#define taskWDT_PRIO    		(OS_LOWEST_PRIO - 4)	//!< ����� ����� ����� ������ ��� ��������� ����� �� ��� ����������� ����� ������
