//--------------------------------------------------------------------------
//-| FILENAME: i2c.c
//-|
//-| Created on: 21.10.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------
/*
 * UAV 16.04.2012
 * add i2c_GPIO_reset()
 * 
 */
//#include "common.h"
//#include "rtos.h"
#include "HAL/MCU/inc/T8_5282_i2c.h"
//#include "settings.h"
//#include "crc.h"

static I2CWaitTimer_t __wait_timer = { NULL, 10 } ;

void* I2CRESOURCE_ID = (void*)0x85710034;
/////////////////////////////////////////////////////////////////
typedef enum _I2C_PINS{SCL=0, SDA=1} I2C_PIN;

////////////////////////////////////////////////////////////////////
#define I2C_ERROR_EXIT(x) { \
	I2Cinit() ; \
	x \
	return flag; \
}

extern void I2C_ICR_init(u8, u8);
u32 I2C_receiveByte(u8 id, u8 address, u8* data);
u32 i2c_wait_byte_transfer(BYTE_WAIT_TYPE type);
u32 i2c_wait_while_bus_busy();


void I2CRestore() {
	if (MCF_I2C_I2SR & MCF_I2C_I2SR_IAL) {
		MCF_I2C_I2SR = 0;
		MCF_I2C_I2CR = 0;
		I2Cinit();
	}
	return;
}

/* Functions */

/*
 * I2Cinit: I2C initilazation as master
 *
 * Parameters: None.
 *
 * Return : None.
 */
u32 i2c_reinit_cnt = 0;
void I2Cinit() {
	int	cnt;
	
	i2c_reinit_cnt++;
	MCF_I2C_I2CR = 0;
	/* I2C pins configuration */
	//temp16 = MCF_GPIO_PASPAR & ~((u16)(MCF_GPIO_PASPAR_PASPA1(3) | MCF_GPIO_PASPAR_PASPA0(3)));
	//MCF_GPIO_PASPAR = temp16 | MCF_GPIO_PASPAR_PASPA1(3) | MCF_GPIO_PASPAR_PASPA0(3);
	MCF_GPIO_PASPAR |= MCF_GPIO_PASPAR_PASPA1(3) | MCF_GPIO_PASPAR_PASPA0(3);
	//Set up I2C clock: [Fsys ~ 75 MHz]. Divider = 896, so SCL < 100 KHz
#ifdef AKSAY
	MCF_I2C_I2FDR = MCF_I2C_I2FDR_IC(0x3A);//87 khz
#elif defined NEVA
	MCF_I2C_I2FDR = MCF_I2C_I2FDR_IC(0x39);//98 khz
//	MCF_I2C_I2FDR = MCF_I2C_I2FDR_IC(0x1F);//20 khz
#elif defined AMUR
	MCF_I2C_I2FDR = MCF_I2C_I2FDR_IC(0x0E);//192 -> 390 khz
#else
	MCF_I2C_I2FDR = MCF_I2C_I2FDR_IC(0x3A);//87 khz	
#endif
//	MCF_I2C_I2ADR = MCF_I2C_I2ADR_ADDR(0x1D);

//If bus is busy, reset it
/*
I2CR = 0x0
I2CR = 0xA0
dummy read of I2DR
I2SR = 0x0
I2CR = 0x0
I2CR = 0x80 ; re-enable 
 */	
	MCF_I2C_I2CR = MCF_I2C_I2CR_IEN;//UAV added
	cnt = 10;
//	if ((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) && (cnt)) {
	while ((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) && (cnt)) {
		MCF_I2C_I2CR = 0;
		MCF_I2C_I2CR = MCF_I2C_I2CR_IEN | MCF_I2C_I2CR_MSTA;
		MCF_I2C_I2SR = 0;//was changed order I2CR, I2SR UAV
		MCF_I2C_I2CR = 0;//
		MCF_I2C_I2CR = MCF_I2C_I2CR_IEN; //uav
		cnt--;
	}
	MCF_I2C_I2CR = MCF_I2C_I2CR_IEN | MCF_I2C_I2CR_IIEN | MCF_I2C_I2CR_TXAK;//cause IBB set UAV
	//I2C_ICR_init(3,3);
	return;
}

void I2CInit_Timer( const I2CWaitTimer_t timer )
{
	assert( timer.CurrTick );
	__wait_timer.CurrTick = timer.CurrTick ;
	__wait_timer.wait_time = timer.wait_time ;
}

//TODO: Add simple guard (in case all interrupts disable, and system timer not function)
//for different I2C speeds
#pragma optimize=none // см. Help-> Pragma directives : Descriptions of pragma directives : optimize
u32 i2c_wait_byte_transfer(BYTE_WAIT_TYPE type)
{
	u32 curr_time, base_time ;
	u32 flag=ERROR;
	u8 fault_keys;
	fault_keys = MCF_I2C_I2SR_IAL;
	if (type == WAIT_ACK) 
		fault_keys |= MCF_I2C_I2SR_RXAK;
	// если есть ф-ция таймера -- ждём конечное время и ресетим
	if(__wait_timer.CurrTick){
		curr_time = base_time = __wait_timer.CurrTick() ;
		// ждём окончания передачи
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF ) &&
				// или пока не кончится время
				((curr_time - base_time) < __wait_timer.wait_time) ){

			curr_time = __wait_timer.CurrTick() ;
		}
		//Something wrong			
		if (MCF_I2C_I2SR & fault_keys) {
			flag=ERROR;
		}
		else if (MCF_I2C_I2SR & MCF_I2C_I2SR_ICF)
			flag = OK;
	// функция таймера не определена
	} else {
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF ));// && !UAlarmTimer());
		//Something wrong			
		if (MCF_I2C_I2SR & fault_keys) {
			flag=ERROR;
		}
		else if (MCF_I2C_I2SR & MCF_I2C_I2SR_ICF)
			flag = OK;
	}
	MCF_I2C_I2SR = 0;
	return flag;
}


u32 i2c_wait_while_bus_busy()
{
	u32 curr_time, base_time ;
	// если есть ф-ция таймера -- ждём конечное время и ресетим
	if(__wait_timer.CurrTick){
		curr_time = base_time = __wait_timer.CurrTick() ;
		while ( (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) && 
				((curr_time - base_time) < __wait_timer.wait_time) ){

			curr_time = __wait_timer.CurrTick() ;
		}
	// функция таймера не определена
	} else
		while ((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB));// && !Timer(&timer));		

	if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB)
			return ERROR;
	else
		return OK;
}

/*
 * I2CsendByte: send byte to I2C device
 *
 * Parameters: data: byte to write
 *			   address: address to write
 *			   id: I2C device to write
 *
 * Return : None.
 */
u32 I2CsendByte(u8 id, u8 address, u8 data) {
	u32 flag=ERROR;
	id = id << 1;
//	ENTER_SAVE_SECTION
//TODO: Change I2CInit for I2CResetBus(). This for all functions.
//When we read some XFP the I2C bus left in busy state...
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	// setting in Tx mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;
// generates start condition
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;
	MCF_I2C_I2DR = id;							/* set devide ID to write */
/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT()
	/* clear the completion transfer flag */
	MCF_I2C_I2SR &= ~MCF_I2C_I2SR_IIF;
	MCF_I2C_I2DR = address;						/* memory address */
	/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT()

	MCF_I2C_I2DR = data;						/* memory data */
	/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
/* generates stop condition */
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();
//	EXIT_SAVE_SECTION
	return flag;
}

//--------------------------------------------------------------------------
// u32 I2C_receiveByte(u8 id, u8 address, u8* data)
//
// I2C read byte
//--------------------------------------------------------------------------

u32 I2C_receiveByte(u8 id, u8 address, u8* data) {
	u32 flag=ERROR;
	u8 tmp_data;
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;			/* setting in Tx mode */
	/* send start condition */
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;
	MCF_I2C_I2DR = id;							/* devide ID to write */
	/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
/* if there is no response,  return ERROR*/
	if (flag == ERROR)
		I2C_ERROR_EXIT(;)
	MCF_I2C_I2DR = address;						/* memory address */
/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT(;)
	/* clear the completion transfer flag */
	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;			/* resend start */
	MCF_I2C_I2DR = id | 0x01;					/* device id to read */
	/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT(;)
	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;			/* send NO ACK */
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;			/* setting in Rx mode */
	tmp_data = MCF_I2C_I2DR;					/* dummy read */
/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT(;)
	/* clear the completion transfer flag */
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;
	tmp_data = MCF_I2C_I2DR;
	
	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	*data = tmp_data;
	return flag;
}



/*Sequential reading*/
/*If address length > than 1 byte, bytes are sent from high to low*/
u32 I2CReceiveArray(u8 id, u32 start_address, I2C_ADDRESS_LENGTH address_len, 
		u8* buff, u32 data_len, I2C_DATA_INTEGRITY integrity) {
	u32 flag=ERROR;
	u16 tmp_data;
	s16 i;
	/*if (data_len < 2) {
		return I2C_receiveByte(id, start_address, buff);
	}*/
/***********Block of initial checks***********/
	if ( address_len > I2C_ADDRESS_WORD )
		return ERROR;
/*********************************************/
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;			/* setting in Tx mode */
	/* send start condition */
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;
	MCF_I2C_I2DR = id;							/* device ID to write */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	
	for (i=address_len; i>=0; i--) {
		MCF_I2C_I2DR = (u8)(0xFF & (start_address>>(i*8)) ); /* memory address */
		flag = i2c_wait_byte_transfer(WAIT_ACK);
//		if (flag == ERROR)
//			I2C_ERROR_EXIT()
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;			/* resend start */
	MCF_I2C_I2DR = id | 0x01;					/* device id to read */
/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()

	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_TXAK;			/* send ACK */
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;			/* setting in Rx mode */

//	if (integrity == STRONG_DATA_INTEGRITY)
//		BLOCK_EALL_INTERRUPTS(irq_lev)
	
	tmp_data = MCF_I2C_I2DR;						/* dummy read */
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	for (i=0; i<data_len; i++) {
		if (i+1 == data_len-1) {
			MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;
		}
		if (i == data_len-1) {
			MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;
			buff[i] = MCF_I2C_I2DR;
		}
		else {
			buff[i] = MCF_I2C_I2DR;
			flag = i2c_wait_byte_transfer(nWAIT_ACK);

		}
	}

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}


u32 I2CreadByte(u8 id, u8* data) {
	u32 flag=ERROR;
	u8 tmp_data;
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;
	MCF_I2C_I2DR = id | 0x01;					/* device id to read */
/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT()
	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;			/* send NO ACK */
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;			/* setting in Rx mode */
	tmp_data = MCF_I2C_I2DR;						/* dummy read */
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT()
/* generates stop condition */
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;
	tmp_data = MCF_I2C_I2DR;						/* read data received */

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	*data = tmp_data;
	return flag;
}


u32 I2CwriteByte(u8 id, u8 data) {
	u32 flag=ERROR;
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
// setting in Tx mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;
// generates start condition
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;
	MCF_I2C_I2SR = 0 ;
	MCF_I2C_I2DR = id;							/* set devide ID to write */
/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT()


	MCF_I2C_I2DR = data;				  /* memory address */
	/* wait until one byte transfer completion */
	flag = i2c_wait_byte_transfer(WAIT_ACK);
	if (flag == ERROR)
		I2C_ERROR_EXIT()
/* generates stop condition */
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}


u32 I2CWriteShortLH(u8 id, u8 address, u16 data){
	/*!
	 * \brief Sends a word (16-bit) via I2C; data byte low first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Data to be sent. 
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = (u8)data;                 //Data byte low
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = (u8)(data>>8);           //Data byte high
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Stop.

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

u32 I2CReceiveShortLH(u8 id, u8 address, u16* data, I2C_DATA_INTEGRITY integrity){
	/*!
	 * \brief Receives a word (16-bit) via I2C; data byte low first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Data to be read. 
	 * \param integrity See \ref I2C_DATA_INTEGRITY.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	u16 tmp_data;
	u8 part16;
	
//	ENTER_SAVE_SECTION
//	HWait(5);	
//	
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode.
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start.
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
	

	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;       //Repeated start.
	MCF_I2C_I2DR = id | 0x01;                //Target device address, read.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()		

	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_TXAK;      //ACK.		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;       //Rx mode.
	
	part16 = MCF_I2C_I2DR;                   //Dummy read.
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
	if (flag == ERROR){
		I2C_ERROR_EXIT()
	}	
	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;       //NACK.
	part16 = MCF_I2C_I2DR;                   //Data byte low.
	tmp_data = (u16)part16;
	
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
	if (flag == ERROR) {
		I2C_ERROR_EXIT()
	}
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Clear the completion transfer flag.
	part16 = MCF_I2C_I2DR;                   //Data byte high.	
	tmp_data |= ((u16)part16) << 8;

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();
	
	*data = tmp_data;
	return flag;
}


u32 I2CReceiveShortHL(u8 id, u8 address, u16* data, I2C_DATA_INTEGRITY integrity){
	/*!
	 * \brief Receives a word (16-bit) via I2C; data byte low first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Data to be read. 
	 * \param integrity See \ref I2C_DATA_INTEGRITY.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	u16 tmp_data;
	u8 part16;
	
//	ENTER_SAVE_SECTION
//	HWait(5);	
//	
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode.
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start.
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
	

	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;       //Repeated start.
	MCF_I2C_I2DR = id | 0x01;                //Target device address, read.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()		

	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_TXAK;      //ACK.		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;       //Rx mode.
	

	part16 = MCF_I2C_I2DR;                   //Dummy read.
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
	if (flag == ERROR){
		I2C_ERROR_EXIT()
	}	
	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;       //NACK.
	part16 = MCF_I2C_I2DR;                   //Data byte high.
	tmp_data = ((u16)part16) << 8;
	
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
	if (flag == ERROR) {
		I2C_ERROR_EXIT()
	}
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Clear the completion transfer flag.
	part16 = MCF_I2C_I2DR;                   //Data byte low.	
	tmp_data |= (u16)part16;
	
	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	*data = tmp_data;
	return flag;
}

u32 I2CWriteShortLHArray(u8 id, u8 address, u16* buff, u32 data_len){
	/*!
	 * \brief Sends a word (16-bit) array via I2C; data byte low first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param buff Pointer to an array of words. 
	 * \param data_len Number of words to be sent from \em buff array.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 i;
	u32 flag=ERROR;
	
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	
	for (i=0; i<data_len; i++){	
		MCF_I2C_I2DR = (u8)buff[i];              //Data byte low
		flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//		if (flag == ERROR)
//			I2C_ERROR_EXIT()
			
		MCF_I2C_I2DR = (u8)((buff[i])>>8);      //Data byte high
		flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//		if (flag == ERROR)
//			I2C_ERROR_EXIT()
	}
		
		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Stop.

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

u32 I2CWriteByteArray(u8 id, u8 address, u8* buff, u32 data_len){
	/*!
	 * \brief Sends a byte array via I2C; data byte low first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param buff Pointer to an array of bytes. 
	 * \param data_len Number of words to be sent from \em buff array.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 i;
	u32 flag=ERROR;
	
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	
	for (i=0; i<data_len; i++){	
		MCF_I2C_I2DR = buff[i];              //Data byte.
		flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//		if (flag == ERROR)
//			I2C_ERROR_EXIT()
	}
		
		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Stop.

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

u32 I2CWriteByteArray_16(u8 id, u16 address, u8* buff, u32 data_len){
	/*!
	 * \brief Sends a byte array via I2C; data byte low first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param buff Pointer to an array of bytes. 
	 * \param data_len Number of words to be sent from \em buff array.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 i;
	u32 flag=ERROR;
	
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = (address >> 8) & 0xFF ; // Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()

	MCF_I2C_I2DR = address & 0xFF ; // Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
	
	for (i=0; i<data_len; i++){	
		MCF_I2C_I2DR = buff[i];              //Data byte.
		flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
		if (flag == ERROR)
			I2C_ERROR_EXIT()
	}
		
		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Stop.

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

u32 I2CReceiveByteArray(u8 id, u8 address, u8* buff, u32 data_len, I2C_DATA_INTEGRITY integrity){
	/*!
	 * \brief Receives a byte array via I2C.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Pointer to an array to be written to. 
	 * \param data_len Number of bytes to be sent to \em buff array.
	 * \param integrity See \ref I2C_DATA_INTEGRITY.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	u32 i;
	
//	ENTER_SAVE_SECTION
//	HWait(5);	
	
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode.
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start.
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()

	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag |= i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	

	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;       //Repeated start.
	MCF_I2C_I2DR = id | 0x01;                //Target device address, read.
	flag |= i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()		

	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_TXAK;      //ACK.		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;       //Rx mode.
	
	buff[0] = MCF_I2C_I2DR;                  //Dummy read.
	
	for (i=0; i<data_len; i++){	
		flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
//		if (flag == ERROR){
//			if (integrity == STRONG_DATA_INTEGRITY)
//				RESTORE_INTERRUPTS_LEVEL(irq_lev)
//			I2C_ERROR_EXIT()
//		}	
		buff[i] = MCF_I2C_I2DR;              //Data byte.
	}

	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;       //NACK.	
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
//	if (flag == ERROR) {
//		if (integrity == STRONG_DATA_INTEGRITY)
//			RESTORE_INTERRUPTS_LEVEL(irq_lev)
//		I2C_ERROR_EXIT()
//	}
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Clear the completion transfer flag.
	buff[i] = MCF_I2C_I2DR;                  //Last data byte (with NACK).
	
	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

u32 I2CReceiveByteArray_16(u8 id, u16 address, u8* buff, u32 data_len, I2C_DATA_INTEGRITY integrity)
{
	/*!
	 * \brief Receives a byte array via I2C.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Pointer to an array to be written to. 
	 * \param data_len Number of bytes to be sent to \em buff array.
	 * \param integrity See \ref I2C_DATA_INTEGRITY.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	u32 i;
	
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode.
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start.
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = (address >> 8) & 0xFF ;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()

	MCF_I2C_I2DR = address & 0xFF ;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()
	

	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;       //Repeated start.
	MCF_I2C_I2DR = id | 0x01;                //Target device address, read.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
	if (flag == ERROR)
		I2C_ERROR_EXIT()		

	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_TXAK;      //ACK.		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;       //Rx mode.
	
	buff[0] = MCF_I2C_I2DR;                  //Dummy read.
	
	for (i=0; i < (data_len-1); i++){	
		flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
		if (flag == ERROR){
			I2C_ERROR_EXIT()
		}
		buff[i] = MCF_I2C_I2DR;              //Data byte.
	}

	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;       //NACK.	
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
	if (flag == ERROR) {
		I2C_ERROR_EXIT()
	}
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Clear the completion transfer flag.
	buff[i] = MCF_I2C_I2DR;                  //Last data byte (with NACK).
	
	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

u32 I2CReceiveShortLHArray(u8 id, u8 address, u16* buff, u32 data_len, I2C_DATA_INTEGRITY integrity){
	/*!
	 * \brief Receives a word (16-bit) array via I2C; data byte low first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Pointer to an array to be written to. 
	 * \param data_len Number of words to be sent to \em buff array.
	 * \param integrity See \ref I2C_DATA_INTEGRITY.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	u32 i;
	u8 byte_high, byte_low; 
	
//	ENTER_SAVE_SECTION
//	HWait(5);	
	
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode.
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start.
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	

	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;       //Repeated start.
	MCF_I2C_I2DR = id | 0x01;                //Target device address, read.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()		

	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_TXAK;      //ACK.		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;       //Rx mode.
	
	for (i=0; i<data_len; i++){	
		
		flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
//		if (flag == ERROR){
//			if (integrity == STRONG_DATA_INTEGRITY)
//				RESTORE_INTERRUPTS_LEVEL(irq_lev)
//			I2C_ERROR_EXIT()
//		}	
		byte_low = MCF_I2C_I2DR;              //Data byte low.
		
		flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
//		if (flag == ERROR){
//			if (integrity == STRONG_DATA_INTEGRITY)
//				RESTORE_INTERRUPTS_LEVEL(irq_lev)
//			I2C_ERROR_EXIT()
//		}	
		byte_high = MCF_I2C_I2DR;              //Data byte high.
		
		buff[i]=(((u16)byte_high) << 8) | byte_low;
		
	}
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
//	if (flag == ERROR){
//		if (integrity == STRONG_DATA_INTEGRITY)
//			RESTORE_INTERRUPTS_LEVEL(irq_lev)
//		I2C_ERROR_EXIT()
//	}	
	byte_low = MCF_I2C_I2DR;              //Last data byte low (still with ACK).

	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;       //NACK.	
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
//	if (flag == ERROR) {
//		if (integrity == STRONG_DATA_INTEGRITY)
//			RESTORE_INTERRUPTS_LEVEL(irq_lev)
//		I2C_ERROR_EXIT()
//	}
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Clear the completion transfer flag.
	byte_high = MCF_I2C_I2DR;                  //Last data byte high (with NACK).	

	buff[i]=(((u16)byte_high) << 8) | byte_low;
	
	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}


u32 I2CReceiveShortHLArray(u8 id, u8 address, u16* buff, u32 data_len, I2C_DATA_INTEGRITY integrity){
	/*!
	 * \brief Receives a word (16-bit) array via I2C; data byte high first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Pointer to an array to be written to. 
	 * \param data_len Number of words to be sent to \em buff array.
	 * \param integrity See \ref I2C_DATA_INTEGRITY.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	u32 i;
	u8 byte_high, byte_low; 
	
//	ENTER_SAVE_SECTION
//	HWait(5);	
	
	id = id << 1;
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode.
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start.
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	

	MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;       //Repeated start.
	MCF_I2C_I2DR = id | 0x01;                //Target device address, read.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()		

	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_TXAK;      //ACK.		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MTX;       //Rx mode.
	
//	if (integrity == STRONG_DATA_INTEGRITY)
//		BLOCK_EALL_INTERRUPTS(irq_lev)
		
	buff[0] = MCF_I2C_I2DR;                  //Dummy read.
	
	for (i=0; i<data_len; i++){	
		
		flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
//		if (flag == ERROR){
//			if (integrity == STRONG_DATA_INTEGRITY)
//				RESTORE_INTERRUPTS_LEVEL(irq_lev)
//			I2C_ERROR_EXIT()
//		}	
		byte_high = MCF_I2C_I2DR;             //Data byte high.
		
		flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
//		if (flag == ERROR){
//			if (integrity == STRONG_DATA_INTEGRITY)
//				RESTORE_INTERRUPTS_LEVEL(irq_lev)
//			I2C_ERROR_EXIT()
//		}	
		byte_low = MCF_I2C_I2DR;             //Data byte low.
		
		buff[i]=(((u16)byte_high) << 8) | byte_low;
		
	}
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);//Wait for completion.
//	if (flag == ERROR){
//		if (integrity == STRONG_DATA_INTEGRITY)
//			RESTORE_INTERRUPTS_LEVEL(irq_lev)
//		I2C_ERROR_EXIT()
//	}	
	byte_high = MCF_I2C_I2DR;                //Last data byte high (still with ACK).

	MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK;       //NACK.	
	
	flag = i2c_wait_byte_transfer(nWAIT_ACK);
//	if (flag == ERROR) {
//		if (integrity == STRONG_DATA_INTEGRITY)
//			RESTORE_INTERRUPTS_LEVEL(irq_lev)
//		I2C_ERROR_EXIT()
//	}
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Clear the completion transfer flag.
	byte_low = MCF_I2C_I2DR;                 //Last data byte low (with NACK).	

	buff[i]=(((u16)byte_high) << 8) | byte_low;
	

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

u32 I2CWriteShortHLArray(u8 id, u8 address, u16* buff, u32 data_len){
	/*!
	 * \brief Sends a word (16-bit) array via I2C; data byte high first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param buff Pointer to an array of words. 
	 * \param data_len Number of words to be sent from \em buff array.
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 i;
	u32 flag=ERROR;
	
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
	
	for (i=0; i<data_len; i++){	
		MCF_I2C_I2DR = (u8)buff[i]>>8;           //Data byte high.
		flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//		if (flag == ERROR)
//			I2C_ERROR_EXIT()
			
		MCF_I2C_I2DR = (u8)((buff[i]));          //Data byte low.
		flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//		if (flag == ERROR)
//			I2C_ERROR_EXIT()
	}
		
		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Stop.

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}


u32 I2CWriteShortHL(u8 id, u8 address, u16 data){
	/*!
	 * \brief Sends a word (16-bit) via I2C; data byte high first.
	 * \param id Target device I2C address.
	 * \param address Address of target register (or command) on device.
	 * \param data Data to be sent. 
	 * \retval \ref OK if the transmission was successful, \ref ERROR otherwise.
	 */

	u32 flag=ERROR;
	id = id << 1;
//	ENTER_SAVE_SECTION
	if (i2c_wait_while_bus_busy() == ERROR) {
		I2Cinit();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_IBB) {
//			EXIT_SAVE_SECTION
			return ERROR;
		}
	}
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MTX;        //TX mode
	MCF_I2C_I2CR |= MCF_I2C_I2CR_MSTA;       //Start
	
	MCF_I2C_I2DR = id;                       //Target device address, write.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = address;                  //Command (target data register address).
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = (u8)(data>>8);            //Data byte high.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2DR = (u8)data;                 //Data byte low.
	flag = i2c_wait_byte_transfer(WAIT_ACK); //Wait for completion.
//	if (flag == ERROR)
//		I2C_ERROR_EXIT()
		
	MCF_I2C_I2CR &= ~MCF_I2C_I2CR_MSTA;      //Stop.

	// ждём стопового сигнала
	i2c_wait_while_bus_busy();

	return flag;
}

