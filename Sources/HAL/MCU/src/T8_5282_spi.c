//--------------------------------------------------------------------------
//-| FILENAME: spi.c
//-|
//-| Created on: 25.11.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

//#include "common.h"
#include "HAL/MCU/inc/T8_5282_spi.h"
#include <intrinsics.h>
#include "io5282.h"
#include <string.h>

#define AT25DF321_PROGRAM_OPCODE 0x02
#define AT25DF321_READ_OPCODE	 0x0B
#define AT25DF321_CS 0x04
#define M23K256_CS   0x02

#define M23K256_READ_OPCODE 0x03
#define M23K256_WRITE_OPCODE 0x02
#define M23K256_READSTATUS_OPCODE 0x05
#define M23K256_WRITESTATUS_OPCODE 0x01

static u8 T8_QSPI_DATA_REGISTER_READ_DELAY = 10;


void qspi_wait(u32 timeout);

void qspi_wait(u32 timeout) {
	u32 nTicks=50*timeout;
//	u32 nTicks=512*timeout;
//	RTOS_TIMER timer;
//	SetTimer(&timer, timeout, RTOS_TIMER_USEC);
//	while ((MCF_QSPI_QDLYR & MCF_QSPI_QDLYR_SPE) && !Timer(&timer));
	while(nTicks--);
	return;
}

void QSPIInit() {
	//Enable all I/o ports
	MCF_PAD_PQSPAR = 0x7F;
	//CPOL=0, CPHA=1, Master, 8 bits
	// MCF_QSPI_QMR = MCF_QSPI_QMR_MSTR | MCF_QSPI_QMR_BITS(8) | MCF_QSPI_QMR_CPHA;
	MCF_QSPI_QMR = MCF_QSPI_QMR_MSTR | MCF_QSPI_QMR_BITS(8) ;
	//Baudrate=fsys/(2*baud)=77000kHz/(2*4Dh)=500kHz
	MCF_QSPI_QMR |= MCF_QSPI_QMR_BAUD(0x4D);
	//QSPI_CS inactive level = active low.
	MCF_QSPI_QWR = MCF_QSPI_QWR_CSIV;
	//No interrupts
	MCF_QSPI_QIR = 0;
	//QSPI Enable (?)
	//MCF_QSPI_QDLYR |= MCF_QSPI_QDLYR_SPE;
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TRANSMIT_RAM 0x0
#define RECEIVE_RAM 0x10
#define COMMAND_RAM 0x20

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void QSPI_Init()
{
	//Enable all I/o ports
	MCF_PAD_PQSPAR |= 0x07 ; // MOSI, MISO и CLK, CS управлю€тс€ "в ручном режиме" вышесто€щим кодом
	//CPOL=0, CPHA=1, Master, 8 bits
//	 MCF_QSPI_QMR = MCF_QSPI_QMR_MSTR | MCF_QSPI_QMR_BITS(8) | MCF_QSPI_QMR_CPHA | MCF_QSPI_QMR_CPOL ;
	MCF_QSPI_QMR = MCF_QSPI_QMR_MSTR | MCF_QSPI_QMR_BITS(8) | MCF_QSPI_QMR_CPOL ;
        MCF_QSPI_QMR &= ~(MCF_QSPI_QMR_CPHA | MCF_QSPI_QMR_CPOL);
	//QSPI_CS inactive level = active low.
	MCF_QSPI_QWR = MCF_QSPI_QWR_CSIV;
	// 200 нс задержка между фронтом CS и началом/концом передачи
	// MCF_QSPI_QDLYR = MCF_QSPI_QDLYR_DTL( 16 ) | MCF_QSPI_QDLYR_QCD( 16 );
	// прерывани€
	MCF_QSPI_QIR = MCF_QSPI_QIR_SPIFE ;
}

static const u8 *__tx_array ;
static u8 *__rx_array ;
static u8 __cur_cs ;
static size_t __tx_len, __rx_len ;
static size_t __transaction_len = 0 ;
static _BOOL __transfer_words ;

void __buffer_transmit_recieve();

void QSPI_ReadWriteArray( const u8 cur_cs, const u8 *tx_data, const size_t tx_len,
						u8 *rx_data, const size_t rx_len,
						const u8 divider, _BOOL cpol, _BOOL cpha )
{
	__tx_array = tx_data ;
	__tx_len = tx_len ;
	__rx_array = rx_data ;
	__rx_len = rx_len ;
	__cur_cs = cur_cs ;

	// MCF_QSPI_QMR &= ~MCF_QSPI_QMR_CPOL ;
	// MCF_QSPI_QMR &= ~MCF_QSPI_QMR_CPHA ;
	// MCF_QSPI_QMR &= ~(MCF_QSPI_QMR_BAUD(0xFF));
	MCF_QSPI_QMR &= 0xFFFF ^ (MCF_QSPI_QMR_BAUD(0xFF) | MCF_QSPI_QMR_CPHA );
	// MCF_QSPI_QMR |= MCF_QSPI_QMR_BAUD(divider) | MCF_QSPI_QMR_CPOL | MCF_QSPI_QMR_CPHA ;
	MCF_QSPI_QMR |= MCF_QSPI_QMR_BAUD(divider) 		|
				 ( cpha ? MCF_QSPI_QMR_CPHA : 0 ) | ( cpol ? MCF_QSPI_QMR_CPOL : 0 )	;

	MCF_QSPI_QIR |= MCF_QSPI_QIR_SPIFE ;
	__buffer_transmit_recieve();
}

_BOOL QSPI_isr()
{
	size_t i ;
	// снимаем флаг прерывани€
	MCF_QSPI_QIR |= MCF_QSPI_QIR_SPIF;

	// читаем полученные слова
	MCF_QSPI_QAR = RECEIVE_RAM ;
	if( __transfer_words ){
		for( i = 0; (i < __transaction_len) && (__rx_len > 0);
						++i, __rx_len -= 2 ){
			*((u16*)__rx_array) = MCF_QSPI_QDR ;
			__rx_array += 2 ;
		}
	} else {
		for( i = 0; (i < __transaction_len) && (__rx_len > 0);
						++i, __rx_len -= 1 ){
			*((u8*)__rx_array) = MCF_QSPI_QDR ;
			__rx_array += 1 ;
		}
	}
	// передача закончилась -- возвращаем CS на место
	if( (__tx_len == 0) && (__rx_len == 0) ){
		return TRUE ;
	// не закончилась -- шлЄм ещЄ
	} else {
		__buffer_transmit_recieve() ;
		return FALSE ;
	}
}

// вызываетс€ дл€ инициализации передачи очередной пачки (максимум 32 байта)
// из QSPI_ReadWriteArray или из прерывани€
// как параметры принимает статические глобальные переменные:
// __tx_array -- откуда берЄм данные на выдачу
// __tx_len -- сколько там данных
// __rx_array -- куда кладЄм прин€тые байты
// __rx_len -- сколько там данных осталось положить
// __cur_cs -- какую конфигурацию выставить в CS
void __buffer_transmit_recieve()
{
	
	size_t i ;
	u16 temp16;

	// длина оставшихс€ буферов больше, чем то, что влезет в аппаратный буфер 32 байта
	if( ((__tx_len >= 32) || (__tx_len == 0)) &&
		((__rx_len >= 32) || (__rx_len == 0)) ){
		__transaction_len = 16 ;
		__transfer_words = TRUE ; // по 2 байта
		// по 16 бит
		temp16 = MCF_QSPI_QMR & ~MCF_QSPI_QMR_BITS(15) ;
		MCF_QSPI_QMR = temp16 | MCF_QSPI_QMR_BITS(0) ;
	// передаЄм байтами
	} else {
		// по 8 бит
		temp16 = MCF_QSPI_QMR & ~MCF_QSPI_QMR_BITS(15) ;
		MCF_QSPI_QMR = temp16 | MCF_QSPI_QMR_BITS(8) ;
		__transaction_len = MIN( MAX( __tx_len, __rx_len ), 16 );
		__transfer_words = FALSE ; // в байтах
	}

	/* Setting start and end of queue pointers. */
	temp16 = MCF_QSPI_QWR & ~( MCF_QSPI_QWR_NEWQP(0xF) | MCF_QSPI_QWR_ENDQP(0xF) );
	MCF_QSPI_QWR = temp16 | MCF_QSPI_QWR_NEWQP(0x0) | MCF_QSPI_QWR_ENDQP( __transaction_len - 1 );

	for( i = 0; i < __transaction_len; i++ ){
		if( __transfer_words ){		
			MCF_QSPI_QAR = COMMAND_RAM + i ;
			/* Continuous Chip Select during all transmission. */
			MCF_QSPI_QDR = MCF_QSPI_QDR_CONT | MCF_QSPI_QDR_BITSE | MCF_QSPI_QCR_CS( __cur_cs );
			MCF_QSPI_QAR = TRANSMIT_RAM + i ;
			if( __tx_len > 0 ){
				MCF_QSPI_QDR = *( (u16*)__tx_array );
				__tx_array += 2 ;
				__tx_len -= 2 ;
			} else {
				MCF_QSPI_QDR = 0 ;
			}
		} else {
			MCF_QSPI_QAR = COMMAND_RAM + i ;
			/* Continuous Chip Select during all transmission. */
			MCF_QSPI_QDR = MCF_QSPI_QDR_CONT | MCF_QSPI_QCR_CS( __cur_cs );
			MCF_QSPI_QAR = TRANSMIT_RAM + i ;

			if( __tx_len > 0 ){
				MCF_QSPI_QDR = *( (u8*)__tx_array );
				__tx_array += 1 ;
				__tx_len -= 1 ;
			} else {
				MCF_QSPI_QDR = 0 ;
			}
		}
	}
	// запускаем передачу
	MCF_QSPI_QDLYR |= MCF_QSPI_QDLYR_SPE;
}
