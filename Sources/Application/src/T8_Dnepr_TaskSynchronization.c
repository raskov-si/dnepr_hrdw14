/*!
\file T8_Dnepr_TaskSynchonization.c
\brief обёртки над мьютексами для синхронизации потоков в контексте взаимодействия с spi, i2c и т. п.
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
\date dec 2012
*/

#include "Application/inc/T8_Dnepr_TaskSynchronization.h"
#include "prio.h"

#include "OS_CPU.H"
#include "OS_CFG.H"
#include "uCOS_II.H"

#include "support_common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static OS_EVENT *__spi_mutex = NULL ; //!< блокирует одновременный доступ из разных потоков
static OS_EVENT *__i2c_mutex = NULL ; //!< блокирует одновременный доступ из разных потоков
static OS_EVENT *__ffs_mutex = NULL ; //!< защищает файловую систему

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void T8_Dnepr_TS_Init()
{
	INT8U return_code = OS_ERR_NONE;

	__spi_mutex = OSMutexCreate( spi_mutex_PRIO, &return_code );
	assert( return_code == OS_ERR_NONE );

	__i2c_mutex = OSMutexCreate( i2c_mutex_PRIO, &return_code );
	assert( return_code == OS_ERR_NONE );

	__ffs_mutex = OSMutexCreate( ffs_mutex_PRIO, &return_code );
	assert( return_code == OS_ERR_NONE );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void T8_Dnepr_TS_SPI_Lock()
{
	INT8U return_code = OS_ERR_NONE;
	assert( __spi_mutex );
    OSMutexPend( __spi_mutex, 0, &return_code );
    assert( return_code == OS_ERR_NONE );
}

void T8_Dnepr_TS_SPI_Unlock()
{
	assert( __spi_mutex );
	OSMutexPost( __spi_mutex );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void T8_Dnepr_TS_I2C_Lock()
{
	INT8U return_code = OS_ERR_NONE;
	assert( __i2c_mutex );
    OSMutexPend( __i2c_mutex, 0, &return_code );
    assert( return_code == OS_ERR_NONE );
}

void T8_Dnepr_TS_I2C_Unlock()
{
	assert( __i2c_mutex );
	OSMutexPost( __i2c_mutex );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void T8_Dnepr_TS_FFS_Lock()
{
	INT8U return_code = OS_ERR_NONE;
	assert( __ffs_mutex );
    OSMutexPend( __ffs_mutex, 0, &return_code );
    assert( return_code == OS_ERR_NONE );
}

void T8_Dnepr_TS_FFS_Unlock()
{
	assert( __ffs_mutex );
	OSMutexPost( __ffs_mutex );
}

