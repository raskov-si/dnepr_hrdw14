/*!
\file T8_Dnepr_SDRAM.c
\brief Код инициализации внешней ОЗУ
\author Baranov Mikhail, <a href="mailto:baranovm@t8.ru">baranovm@t8.ru</a>
*/


#include "support_common.h"
#include "uCOS_II.H"

static _BOOL __status = FALSE ;
static size_t __check_time = 0 ;

void Dnepr_SDRAM_init( u32 sdram_address, u32 system_clock_khz )
{
	volatile size_t i ;
	// Check to see if the SDRAM has already been initialized by a run control tool
	if (!(MCF_SDRAMC_DACR0 & MCF_SDRAMC_DACR_RE)){
		// Initialize DRAM Control Register: DCR
		MCF_SDRAMC_DCR = (0
							| MCF_SDRAMC_DCR_RTIM_6 | MCF_SDRAMC_DCR_RC((15 * system_clock_khz/1000)>>4));
		// Initialize DACR0
		MCF_SDRAMC_DACR0 = ( 0
							| MCF_SDRAMC_DACR_BA(sdram_address)
							| MCF_SDRAMC_DACR_CASL(1)
							| MCF_SDRAMC_DACR_CBM(4)
							| MCF_SDRAMC_DACR_PS_16 );
		i = ( 0
							| MCF_SDRAMC_DACR_BA(sdram_address)
							| MCF_SDRAMC_DACR_CASL(1)
							| MCF_SDRAMC_DACR_CBM(4)
							| MCF_SDRAMC_DACR_PS_16 );
		// Initialize DMR0
		MCF_SDRAMC_DMR0 = (0
                           | MCF_SDRAMC_DMR_BAM_16M
                           | MCF_SDRAMC_DMR_V );
		i = (0
                           | MCF_SDRAMC_DMR_BAM_16M
                           | MCF_SDRAMC_DMR_V );
		// Set IP (bit 3) in DACR
		MCF_SDRAMC_DACR0 |= MCF_SDRAMC_DACR_IP;
		// Wait 30ns to allow banks to precharge
		for( i = 0; i < 5; i++ ){
#ifndef __MWERKS__
			asm(" nop");
#else
			asm( nop);
#endif
		}
		// Write to this block to initiate precharge
		*(u16 *)(sdram_address) = 0xA5A5; // 0x9696;
		// Set RE (bit 15) in DACR
		MCF_SDRAMC_DACR0 |= MCF_SDRAMC_DACR_RE;
		// Wait for at least 8 auto refresh cycles to occur
		for( i = 0; i < 2000; i++ ){
#ifndef __MWERKS__
			asm(" nop");
#else
			asm( nop);
#endif
		}

		// Finish the configuration by issuing the IMRS.
		MCF_SDRAMC_DACR0 |= MCF_SDRAMC_DACR_IMRS ;

		*(u16*)(sdram_address + 0x000) = 0x0020;
	}
}

_BOOL Dnepr_SDRAM_Check( size_t sdram_address, size_t sdram_len )
{
	_BOOL ret = TRUE ;
	size_t i = 0 ;
	u8 *data = (u8*)sdram_address ;
	u32 * dwData = (u32*)sdram_address ;
	size_t start_time ;

	*dwData = 0x00 ;
	 if( *dwData != 0x0 ){
	 	ret = FALSE ;
	 }
	 *dwData = 0xAAAA55CC ;
	 if( *dwData != 0xAAAA55CC ){
	 	ret = FALSE ;
	 }
	 *data = 0x78 ;
	 if( *dwData != 0x78AA55CC ){
	 	ret = FALSE ;
	 }

	start_time = llUptime ;
	// заполняем
	for( i = 0; i < sdram_len; ++i ){
		*data++ = 0xFF & (0xD5 ^ i) ;
	}
	data = (u8*)sdram_address ;
	// проверяем
	for( i = 0; i < sdram_len; ++i ){
		ret = ret && (*data++ == (0xFF & (0xD5 ^ i))) ;
	}
	__check_time = llUptime - start_time ;
	return __status = ret ;
}

_BOOL Dnepr_SDRAM_GetStatus()
{
	return __status ;
}
