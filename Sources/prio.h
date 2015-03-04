/*!
\file prio.h
\brief приоритеты всех задач и прерываний
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date july 2012
*/

// Прерываня разрешаются там же, где производится инициализация соответствующей переферии
// переферии.
// Уровни прерываний во всё проекте:
// НАЗВАНИЕ							Lvl		Prio
// 	INTR_ID_EPF1					1		5
// 	EDGEPORT1						1 		0	// SFP_nINT
// 	PIT0 (системный таймер) 		3		0
//	Trap15 (сис. вызовы)			3		1
//  QSPI							5 		1
// 	UART1 (Kontron)					5		2
//	EDGEPORT3						5 		5	// FPGA_nINT
//	EDGEPORT4						5 		6	// I2C_SW_INT

#define taskInit_PRIO			20	//!< приоритет задачи инициализации остальных задач, очень большой приоритет


#define dyn_param_mutex_PRIO 	12 	//!< приоритет мьютекса для переключения буфера динамических параметров
#define spi_mutex_PRIO 			13 	//!< приоритет мьютекса для переключения буфера параметров
#define i2c_mutex_PRIO 			14 	//!< приоритет мьютекса для переключения буфера параметров
#define SMI_MUTEX_PRIO                  15      //!< приоритет мьютекса для смены параметров PHY (свитча)
#define ffs_mutex_PRIO 			16 	//!< приоритет мьютекса для защиты функций hotswap'а
#define hs_mutex_PRIO 			17 	//!< приоритет мьютекса для защиты функций hotswap'а

#define tackDController_PRIO	30
#define TASK_ETH_PRIORITY      (35)
#define TASK_SNMP_PRIORITY     (TASK_ETH_PRIORITY+1)
#define taskCU_PRIO     		40	//!< приоритет задачи работы с backplane-uart'ом


#define taskMeasure_PRIO    	50	//!< приоритет задачи измерения динамических параметров
#define SyncParam_prio          51	//!< приоритет задачи измерения динамических параметров
#define taskFPGA_COMM_PRIO  	52	//!< 
#define taskFlash_COMM_PRIO 	53	//!< 
#define taskFS_COMM_PRIO    	54	//!< 

#define TASKTERM_COMM_PRIO     		(52)	//!< приоритет задачи работы с терминалом на backplane

#define taskWDT_PRIO    		(OS_LOWEST_PRIO - 4)	//!< нужно чтобы любой другой НАШ подвисший поток не дал выполниться этому потоку
