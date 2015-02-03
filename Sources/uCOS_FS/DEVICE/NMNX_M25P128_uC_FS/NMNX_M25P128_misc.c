/*!
\file NMNX_M25P128_misc.h
\brief uC/FS Device driver layer for Numonix M25P128 Flash IC.
\todo Implement some wear-off protection! It writes block-wise (512 bytes = 2 pages = 1/512th of erase sector).
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 05.06.2012
*/

#include "stdio.h"
#include "fs_port.h"
#include "fs_dev.h" 
#include "fs_lbl.h" 
#include "fs_conf.h"

#if FS_USE_NMNX_M25P128_DRIVER

//#include "includes.h"
#include "HAL/IC/inc/NMNX_M25P128.h"
#include "HAL\BSP\inc\T8_Dnepr_API.h"
#include "fs_api.h"
#include "fs_clib.h"

static int _FS_NMNX_M25P128_DevIoCtl(FS_u32 id, FS_i32 cmd, FS_i32 aux, void *buffer);
static int _FS_NMNX_M25P128_DevRead(FS_u32 id, FS_u32 block, void *buffer);
static int _FS_NMNX_M25P128_DevStatus(FS_u32 id);
static int _FS_NMNX_M25P128_DevWrite(FS_u32 id, FS_u32 block, void *buffer);


static int _FS_NMNX_M25P128_DevIoCtl(FS_u32 id, FS_i32 cmd, FS_i32 aux, void *buffer){
	/*!
	\brief Executes commands.
	\details In fact it does nothing.
	\param id Number of media (0…N).
	\param cmd Command. Only \p FS_CMD_FLUSH_CACHE is supported at the moment.
	\param aux Parameter for command.
	\param buffer Pointer to data required by command.
	\retval In case of success, the return value is 0. Upon failure, the return value is –1.
	*/
	if (cmd==FS_CMD_FLUSH_CACHE) {
		/* nothing to do here */
		return 0;
	}
	return -1;
}
static int _FS_NMNX_M25P128_DevRead(FS_u32 id, FS_u32 block, void *buffer){
	/*!
	\brief Reads 1 block = 2 pages of data from Flash.
	\param id Number of media (0…N).
	\param block Block number to be read from the media.
	\param buff Data buffer to which the data is transferred.
	\retval In case of success, the return value is 0. Upon failure, the return value is –1.
	*/
	if (id != 0) {
		return -1;  /* Invalid unit number */
	}
	if (id >= FS_NMNX_M25P128_BLOCKNUM) {
		return -1;  /* Out of physical range */
	}

	NMNX_M25P128_WaitForWriteCompletion();  
	NMNX_M25P128_ReadBytes( NMNX_M25P128_BLOCK_PAGE_BASE(block, 0), (u8*)buffer, NMNX_M25P128_PAGE_SIZE );
	NMNX_M25P128_WaitForWriteCompletion();
	NMNX_M25P128_ReadBytes( NMNX_M25P128_BLOCK_PAGE_BASE(block, 1), (u8*)buffer + NMNX_M25P128_PAGE_SIZE, NMNX_M25P128_PAGE_SIZE );
	NMNX_M25P128_WaitForWriteCompletion();
	return 0;
}
static int _FS_NMNX_M25P128_DevStatus(FS_u32 id){
	/*!
	\brief Current status of the device.
	\param id Number of media (0…N).
	\retval The function returns 0 if the device can be accessed. Any value < 0 should be interpreted as an error.
	\note This function is used by uC/FS to check if the media has changed (e.g. a card removed or replaced) and the device can be accessed. This one always returns 0 since the device is hard wired.
	*/
	return 0;
}
static int _FS_NMNX_M25P128_DevWrite(FS_u32 id, FS_u32 block, void *buffer){
	/*!
	\brief Writes 1 block = 2 pages of data from Flash.
	\param id Number of media (0…N).
	\param block Block number to be written on media.
	\param buff Pointer to data for transfer to the media.
	\retval In case of success, the return value is 0. Upon failure, the return value is –1.
	\todo Think thoroughly about "wait for write completion" routines placements. 
	*/
	if (id != 0) {
		return -1;  /* Invalid unit number */
	}
	if (id >= FS_NMNX_M25P128_BLOCKNUM) {
		return -1;  /* Out of physical range */
	}
	
	NMNX_M25P128_WaitForWriteCompletion();  
	NMNX_M25P128_SectorErase( NMNX_M25P128_BLOCK_PAGE_BASE(block, 0) );
	NMNX_M25P128_WaitForWriteCompletion();		
	NMNX_M25P128_PageProgram( NMNX_M25P128_BLOCK_PAGE_BASE(block, 0), (u8*)buffer, NMNX_M25P128_PAGE_SIZE );
	NMNX_M25P128_WaitForWriteCompletion();	
	NMNX_M25P128_PageProgram( NMNX_M25P128_BLOCK_PAGE_BASE(block, 1), (u8*)buffer + NMNX_M25P128_PAGE_SIZE, NMNX_M25P128_PAGE_SIZE );
	NMNX_M25P128_WaitForWriteCompletion();	
	
	return 0;
}

const FS__device_type FS__NMNX_M25P128_driver = {
  "Numonix M25P128 device",
  _FS_NMNX_M25P128_DevStatus,
  _FS_NMNX_M25P128_DevRead,
  _FS_NMNX_M25P128_DevWrite,
  _FS_NMNX_M25P128_DevIoCtl
};

#endif /* FS_USE_NMNX_M25P128_DRIVER */
