/*!
\file profile_processing.h
\brief Main profile interworks
\author <a href="mailto:baranovm@t8.ru">Mikhail Baranov</a>
\date june 2012
*/

#include "Application/inc/T8_Dnepr_Profile_params.h"
#include "Application/inc/T8_Dnepr_ProfileStorage.h"
#include "Profile/inc/profile_processing.h"
#include "Profile/inc/sys.h"
#include "Profile/inc/extras.h"
#include "common_lib/crc.h"
#include "Profile/inc/pr_ch.h"
#include "Storage/inc/eeprom_storage.h"
#include "HAL/BSP/inc/T8_Dnepr_BPEEPROM.h"
#include "HAL/BSP/inc/T8_Dnepr_Uart_Profile.h"
#include "HAL/MCU/inc/T8_5282_cfm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static _BOOL Profile_write_enable_flag = 0 ;

typedef enum {
	CMD_0x71=0x71,
	CMD_0x77=0x77
} SECT_READ_CMD_SUBTYPE;

//static s8* profile_string = NULL ;

extern const u8 langpack_header[] ;
const u8* profile_32Kblock = langpack_header ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 profile_create_str(s8* prof_str, PARAM* p) {
	PARAM* header;
	s8* buf_str;
	u32 six;
	if (p==NULL) {
		prof_str[0] = 'N';
		prof_str[1] = 'A';
		return 2;
	}
	header = (PARAM*)p;
	if ((((header->par_type <= PR_CALIB_COEFF) && (header->par_type >= PR_INFORM_PARAM))
			|| ((header->par_type<=PR_ADDSECT200) && (header->par_type>=PR_ADDSECT100)))
			&& (header->local_num)) {
		// имя параметра
		buf_str = (s8*)SYSSetIndexByPtr(p,0,SYS_PARAM_NAME);
		strcpy(prof_str,buf_str);
		six = strlen(prof_str);
		prof_str[six++] = ';';
		// номер параметра в секции
		t8_ultoa( header->local_num, prof_str+six, 11, 10 );
		six += strlen(prof_str+six);
		prof_str[six++] = ';';
		// тип данных
		t8_ultoa( header->data_type, prof_str+six, 11, 10 );
		six += strlen(prof_str+six);
		prof_str[six++] = ';';
		// уровень доступа
		if( SYS_is_param_hidden(header->global_num-1) == OK ){
			prof_str[six] = '0' ;
			prof_str[six+1] = '\0' ;
		} else {
			t8_ultoa( header->sec_level, prof_str+six, 11, 10 );
		}
		six += strlen(prof_str+six);
		prof_str[six++] = ';';
		// еденициы измерения
		buf_str = (s8*)SYSSetIndexByPtr(p,0,SYS_UNIT) ;
		if (buf_str) {
			strcpy(prof_str+six,buf_str);
			six += strlen(prof_str+six);
		}
		prof_str[six++] = ';';
		prof_str[six++] = ';';
		// описание
		buf_str = (s8*)SYSSetIndexByPtr(p,0,SYS_COMMENT);
		if (buf_str != NULL) {
			strcpy(prof_str+six,buf_str);
			six += strlen(prof_str+six);
		}
		else
			prof_str[six] = 0;
		return six;
	}
	else {
		strcpy(prof_str, (s8*)SYSSetIndexByPtr(p,0,SYS_VALUE));
		return strlen(prof_str);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*Command 0x73 - Profile reading*/
ReturnStatus profile_read_cmd(const s8* buf_str, s8* const answer_ ) {
	s8* pch;
	u32 val,p_id,temp;
	u16 crc;
	u8 device_addr;
	
	errno = 0;
	device_addr = Dnepr_Profile_Address();
	val = strtoul((s8*)buf_str,&pch,10); //parameter serial number
	if (errno || *pch != 0x0D) {
		return ERROR;
	}
	answer_[0] = (u8)(0x80+device_addr);
	answer_[1] = 0x73;
	strncpy(&answer_[2],buf_str,(size_t)((const s8*)pch - buf_str));
	answer_[2 + (size_t)((const s8*)pch - buf_str)] = '\0' ;
	answer_[2+(const s8*)pch-buf_str] = ';';
	answer_[2+(const s8*)pch-buf_str+1] = 0;
	temp = strlen(answer_);
	if (val) {
		p_id = SYSSetIndexBySN(val);
		temp += profile_create_str(answer_+temp,SYSGetParentParam(p_id));
	}
	else {
		t8_ultoa( SYSCalcLines(), answer_+temp, 11, 10 );
		temp += strlen(answer_+temp);
	}
	answer_[temp++] = ';';
	crc = Crc16((u8*)answer_,temp);

	t8_ultoa( crc, answer_+temp, 11, 10 );
	temp += strlen(answer_+temp);
	answer_[temp++] = 0x0D;
	answer_[temp] = 0;
	return OK ;
}

ReturnStatus section_read_cmd(const s8* buf_str, s8* const answer_, SECT_READ_CMD_SUBTYPE subtype) {
	u32 par,flag;
	s8* pch;
	u32 sect_n;
	u32 temp;
	u32 color;//, port_id;
	u32 param_len;
	u16 crc = 0;
	
	errno = 0; 
	sect_n = strtoul(buf_str,&pch,10); //Table == section number
	if (errno || *pch != 0x0D) {
		return ERROR;
	}
	answer_[0] = (s8)(0x80+Dnepr_Profile_Address());
	answer_[1] = subtype;
	sprintf(answer_ + 2,"%u;",sect_n);
	temp = strlen(answer_);
	crc = Crc16_mem((u8*)answer_,(u16)temp,crc);
	pch = answer_ + temp;

	if( sect_n == 200 ){
		temp = 0;
		temp++ ;
	}

	if ((par = SYSSectionFirst(sect_n)) != ERROR) {
		while (par != ERROR) {
			//============ get one param ===================
			temp = strlen(pch);//save current position
			flag = SYSGetParamValueStr(par, pch, PARAM_MAX_LEN);//if error, param not writed to output buffer
			if ( flag != ERROR){
				//===== param value is placed into output buffer ====
				//===== add param COLOR if CMD == 0x77 && DYNAMIC section====
				if (subtype == CMD_0x77 && sect_n == 2) {
					// FIXME: stub
					color = 0 ;
					color = app_get_dyn_param_color_glbuf(par);
					color = SYSConvertColor(par,color);
					if((color != ERROR) && (color != SYS_EMPTY_COLOR_ERROR)){
						sprintf(pch+strlen(pch),"@%u",color);
					}
				}//CMD == 0x77
			}
			strcat(pch,";");//closed param's field
			param_len =  strlen(pch) - temp;
			crc = Crc16_mem((u8*)pch,(u16)param_len,crc);//add crc calculation
			pch += param_len;			//pointer move to string end
			
			*pch = 0;
			
			//Get Next Param
			par = SYSSectionNext(par);
		}
		temp = 0;
	}
	else {
		pch[0] = 'N';
		pch[1] = 'A';
		pch[2] = ';';
		pch[3] = 0;
		crc = Crc16_mem((u8*)pch,3,crc);
		temp = 3;
	}
	sprintf(&pch[temp],"%u\r",crc);
	return OK ;
}


ReturnStatus parameter_write( const s8* buf_str, s8* const answer_ ) {
	u32 p_id, i;
	s8* pch;
	s8* ch;
	u32 sect_n, param_n;
	u32 temp, value_length;
	u16 crc1, crc2 = 0;
	u8 semi;
//	u8 device_addr ;
//	device_addr = Dnepr_Profile_Address();
// Check CRC
	for (i=0, semi=0, value_length=0; buf_str[i] && (semi < 3); i++) {
		if (buf_str[i] == ';') {
			semi = semi + 1;
		}
		else if (semi == 2)
			value_length++;
	}
	if (semi != 3)
		return ERROR;
	crc1 = Crc16((u8*)buf_str, i);
	crc2 = (u16)strtoul(&buf_str[i], &pch, 10);
	if (*pch != 0x0D)
		return ERROR;
	if (crc1 != crc2)
		return ERROR;
	errno = 0;
	sect_n = strtoul(buf_str+2,&pch,10); //section serial number
	if (errno || *pch != ';') {
		return ERROR;
	}
	param_n = strtoul(pch+1,&pch,10); //parameter serial number
	if (errno || *pch != ';') {
		return ERROR;
	}
	strncpy(answer_,(s8*)buf_str,(size_t)(pch - buf_str));
	answer_[(size_t)(pch - buf_str)] = '\0' ;
	answer_[pch-buf_str] = ';';
	answer_[pch-buf_str+1] = 0;
	temp = strlen(answer_);
	ch = answer_+temp;
	pch++;
	memcpy(ch, pch, value_length);
	ch[value_length] = 0;

	p_id = SYSSetIndexBySN2(sect_n,param_n) ;
	if (p_id && Profile_write_enable_flag ) {
		 if (SYSSetParamValueStr(p_id, ch) == OK) {
			 strcpy(answer_+temp, ch);
			 temp += strlen(ch);
		 }
		 else {
			answer_[temp++] = 'N';
			answer_[temp++] = 'A';
		 }
		 answer_[temp++] = ';';
	}
	else {
		answer_[temp++] = 'N';
		answer_[temp++] = 'A';
		answer_[temp++] = ';';
	}
	crc1 = Crc16((u8*)answer_,temp);
	t8_itoa( crc1, answer_+temp, 12, 10 );
	temp += strlen(answer_+temp);
	answer_[temp++] = 0x0D;
	answer_[temp] = 0;
	return OK ;
}

ReturnStatus parameter_write_on( s8* const answer_ )
{
	answer_[0] = (u8)(0x80+Dnepr_Profile_Address());
	answer_[1] = 0x49;
	answer_[2] = 'O';
	answer_[3] = 'K';
	answer_[4] = 0x0D;
	answer_[5] = 0;

	return OK ;
}


ReturnStatus device_class_read(const s8* buf_str, s8* const answer_ ) {
	u8 semi=0;
	u8 ix, i;
	answer_[0] = (u8)(0x80+Dnepr_Profile_Address());
	answer_[1] = 0x01;
	SYSGetParamValueStr(Header2, &answer_[2], PARAM_MAX_LEN-4);
	for (i=2, ix=2; i< strlen(&answer_[2]) && semi<2; i++) {
		if (answer_[i] == ';')
			semi++;
		else if (semi==1)
			answer_[ix++] = answer_[i];
	}
	answer_[ix++] = 0xD;
	answer_[ix] = 0;

	return OK ;
}

ReturnStatus flash_section_read(const s8* buf_str, s8* const answer_ )
{
	//UAV corrected
	u32 block_num;
	u16 crc;
	
	if(1 != sscanf((char*)(buf_str),"  %u ",&block_num))	
		return ERROR;
	
	answer_[0] = (s8)0x80+(s8)Dnepr_Profile_Address();
	answer_[1] = 0x7B;
	sprintf(answer_+2,"%u;",block_num);

	if (block_num < 128) {
		// FIXME: stub
		strncat(answer_+strlen(answer_),(char*)&profile_32Kblock[block_num*256],256);
	} else
		strcat(answer_,"NA");
	
	strcat(answer_,";");
	crc = Crc16((u8*)answer_, (u16)strlen(answer_));
	sprintf(answer_+strlen(answer_),"%u\r",crc);

	return OK ;
}

ReturnStatus flash_section_write(const s8* buf_str, s8* const answer_ )
{
	u16 crc;
	u32 block_num;
	u8* p_data;
	
	if(1 != sscanf((char*)(buf_str),"  %u ",&block_num))	
		return ERROR;
	
	//find Block start
	p_data = (u8*)strchr((s8*)buf_str,';');
	if (!p_data)
		return ERROR;
	p_data++;

	answer_[0] = (u8)0x80+Dnepr_Profile_Address();
	answer_[1] = 0x7A;
	sprintf(answer_+2,"%u;",block_num);
	
	// FIXME: stub
	if ((block_num < 128) && (OK == /*LpWriteBlock(p_data, block_num)*/ERROR))
		strcat(answer_,"OK;");
	else
		strcat(answer_,"NA;");

	crc = Crc16((u8*)answer_, strlen(answer_));
	sprintf(answer_+strlen(answer_),"%u\r",crc);
	return OK ;
}


ReturnStatus alarm_colors_write(s8* buf_str, s8* const answer_ ) {
	u32 p_id, i,flag;
	s8* pch;
	u32 sect_n, param_n, value_pos;  
	u16 crc1, crc2 = 0;
	u8 semi;
	u8 failure_color, degrade_color, normal_color;
// Check CRC
	for (i=0, semi=0, value_pos=0; buf_str[i] && (semi < 4); i++) {
		if (buf_str[i] == ';') {
			semi = semi + 1;
			if (semi == 2)
				value_pos = i+1;
		}		
	}
	if (semi != 4)
		return ERROR;
	crc1 = Crc16((u8*)buf_str, i);
	crc2 = (u16)strtoul(&buf_str[i], &pch, 10);
	if (*pch != 0x0D)
		return ERROR;
	if (crc1 != crc2)
		return ERROR;
	errno = 0;
	sect_n = strtoul(buf_str+2,&pch,10); //section serial number
	if (errno || *pch != ';') {
		return ERROR;
	}
	param_n = strtoul(pch+1,&pch,10); //parameter serial number
	if (errno || *pch != ';') {
		return ERROR;
	}
	if(Profile_write_enable_flag){
		errno = 0;
		degrade_color = (u8)strtoul(&buf_str[value_pos],&pch,10);
		if (errno || *pch != ';')
			return ERROR;
		failure_color = (u8)strtoul(pch+1,&pch,10);
		if (errno || *pch != ';')
			return ERROR;
		p_id = SYSSetIndexBySN2(sect_n,param_n);
		Dnepr_GetParamColors(p_id, NULL, NULL, &normal_color);
		flag = Dnepr_WriteParamColors(p_id, failure_color, degrade_color, normal_color);
	//*****Form answer*****	
		strncpy(answer_, buf_str, value_pos );
		answer_[ value_pos ] = '\0' ;
		if (flag == OK) {
			Dnepr_GetParamColors(p_id, &failure_color, &degrade_color, NULL);
			value_pos += sprintf(&answer_[value_pos], "%d;%d;", degrade_color, failure_color);
		}
		else {
			answer_[value_pos++]='N';
			answer_[value_pos++]='A';
			answer_[value_pos++]=';';
		}
	} else { // Profile_write_enable_flag 
		answer_[value_pos++]='N';
		answer_[value_pos++]='A';
		answer_[value_pos++]=';';
	}
	crc1 = Crc16((u8*)answer_, value_pos);
	value_pos += sprintf(&answer_[value_pos], "%d%c", crc1, '\x0D');
	return OK ;
}


ReturnStatus alarm_colors_read(s8* buf_str, s8* const answer_ )
{
	u32 par;
	s8* pch;
	u32 sect_n, ans_ix;  
	u16 crc=0;
	u8 failure_color, degrade_color;
// Check CRC
	errno = 0;
	sect_n = strtoul(buf_str+2,&pch,10); //section serial number
	if (errno || *pch != 0x0D)
		return ERROR;
	if (sect_n == 2) {
	//Form answer
		ans_ix = (size_t)(pch - buf_str) ;
		strncpy((s8*)answer_, buf_str, ans_ix );
		answer_[ans_ix ] = '\0' ;
		crc = Crc16_mem((u8*)answer_,ans_ix,crc);
		pch = &answer_[strlen(answer_)]; 
		if ((par = SYSSectionFirst(sect_n)) != ERROR) {
			while (par != ERROR) {
				Dnepr_GetParamColors(par, &failure_color, &degrade_color, NULL);
				ans_ix = (size_t)sprintf(pch, ";%d@%d", degrade_color, failure_color);
				crc = Crc16_mem((u8*)pch,ans_ix,crc);
				pch += ans_ix ;
				par = SYSSectionNext(par);
			}
/////////////////////////////////////
			pch[0] = ';' ;
			crc = Crc16_mem((u8*)pch,1,crc);
			ans_ix = sprintf(pch, ";%d%c", crc, '\x0D');
/////////////////////////////////////
		}
	}
	else {
		ans_ix = (size_t)(pch - buf_str) ;
		strncpy((s8*)answer_, buf_str, ans_ix );
		answer_[ ans_ix ] = '\0' ;
		answer_[ans_ix++] = ';';
		answer_[ans_ix++] = 'N';
		answer_[ans_ix++] = 'A';
		answer_[ans_ix++] = ';';
		crc = Crc16((u8*)answer_, ans_ix);
		ans_ix += sprintf(&answer_[ans_ix], "%d%c",crc,'\x0D');
	}
	return OK;
}


ReturnStatus parameter_undefined(const s8* buf_str, s8* const answer_ ) {
	answer_[0] = (u8)(Dnepr_Profile_Address() + 0x80);
	answer_[1] = buf_str[0];
	answer_[2] = 'N';
	answer_[3] = 'A';
	answer_[4] = 0x0D;
	answer_[5] = 0;

	return OK ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! \brief формирует ответ на команду профиля
//! \param cBuffer сама команда
//! \param Answer_ указатель строку, куда пишется ответ
ReturnStatus Profile_message_processing(const s8 * recv_buff_,
			const size_t recv_command_len_,  s8* answer_ )
{
	STORAGE_ATOMIC() ;
	ReturnStatus flag = OK ;
	size_t i ;
	s8* pL ;
	ldrEntry vP;
	u32* pFlash;
	u32  crc;
	register u8 bAddr = Dnepr_Profile_Address() ;
	for( i = 0; (i < recv_command_len_) && recv_buff_[i] != (0x80 | bAddr); i++ ){}
	if(i == recv_command_len_)
		return ERROR ;
	// сейчас i -- номер первого байта посылки с нашим адресом в recv_buff_
	switch (recv_buff_[i+1]) {
	// Чтение профиля
	case 0x73:
		flag = profile_read_cmd(recv_buff_+2, answer_ );
		break;
	// Чтение параметров профиля
	case 0x71:
		START_ATOMIC();
		flag = section_read_cmd(recv_buff_+2, answer_, CMD_0x71);
		STOP_ATOMIC();
		break;
	// Запись параметров профиля
	case 0x72:
		flag = parameter_write((s8*)recv_buff_, answer_ );
		Profile_write_enable_flag = 0 ;
		break;
	// Запрос на разрешение записи параметров
	case 0x49:
		flag = parameter_write_on( answer_ );
		Profile_write_enable_flag = TRUE ;
		break;
	// Чтение параметров профиля с раскраской и трэпами
	case 0x77:
		START_ATOMIC();
		flag = section_read_cmd(recv_buff_+2, answer_, CMD_0x77);
		STOP_ATOMIC();		
		break;
	// Чтение текстовых блоков данных из Флэш памяти MCU
	case 0x7B:
		flag = flash_section_read(recv_buff_+2, answer_);
		break;
	// Загрузка текстовых блоков данных в Флэш память MCU
	case 0x7A:
		flag = flash_section_write(recv_buff_+2, answer_);
		break;
	// Запись цветов раскрашиваемых параметров
	case 0x7C:
		flag = alarm_colors_write((s8*)recv_buff_, answer_);
		Profile_write_enable_flag = 0 ;
		break;
	// Чтение цветов раскрашиваемых параметров
	case 0x7D:
		flag = alarm_colors_read((s8*)recv_buff_, answer_);
		break;		
	case 0x01:
		flag = device_class_read(recv_buff_+2, answer_);
		break;
	// COLD Reset
	case 0x20:
		pL = strchr((char*)recv_buff_+2,0x0d);
		if (pL){
			*pL = 0;
			if(0 == strcmp((char*)recv_buff_+2,"ResetMcu_ucMteseR"))
				MCF_RCM_RCR = MCF_RCM_RCR_SOFTRST ;	//soft reset				
		}
		flag = parameter_undefined(recv_buff_+1, answer_);
		break;
		
	//try to switch to loader
	case 0x7e:
		// сбрасываем пороги sfp, после перепрошивки они должны перечитаться
		Dnepr_Params_Cancel_sfp_thresholds() ;
		pFlash = (u32*)LOADER_ENTRY;//loader api_table
		if(*pFlash != 0x12345678){
			flag = parameter_undefined(recv_buff_+1, answer_);			
			break;
		}	
		vP = (ldrEntry)pFlash[1];
		if((u32)vP > 0 && (u32)vP < 0x00080000UL && (((u32)vP & 0x00000003UL) == 0UL)	){
			__disable_interrupts() ;
			vP(0,BACKPLANE_UART);//call (void)T8Loader(u32,u32)
		}
		flag = parameter_undefined(recv_buff_+1, answer_);			
		break;
		
	//some difinition: SIGN, LOADER_VER
	case 0x7f:
		pFlash = (u32*)LOADER_ENTRY;//loader api_table
		if(pFlash[0] != 0x12345678){
			flag = parameter_undefined(recv_buff_+1, answer_);			
			break;
		}	
		vP = (ldrEntry)pFlash[1];
		if((u32)vP > 0 && (u32)vP < 0x00080000UL && (((u32)vP & 0x00000003UL) == 0UL)	){
			answer_[0] = 0x80+Dnepr_Profile_Address() ;
			answer_[1] = recv_buff_[1];
			answer_[2] = 0;
			strcat(answer_,(s8*)pFlash[2]);
			strcat(answer_,";");
			crc = Crc16((u8*)answer_,strlen(answer_));
			sprintf(answer_ + strlen(answer_),"%d\x0D",crc);
			flag = OK;
		}
		else
			flag = parameter_undefined(recv_buff_+1, answer_);			
		break;
	default:
		flag = parameter_undefined(recv_buff_+1, answer_);
	}

	i = strlen(answer_) ;
	return flag ;
}
