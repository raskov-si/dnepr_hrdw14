//--------------------------------------------------------------------------
//-| FILENAME: sys.c
//-|
//-| Created on: 07.09.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------
/* 27.02.2012
 * UAV new
 * 1. Param color, round ->now in APP_SERVICE thread
 * 2. Precision (m_flag)
 * 3. double param buffering
 * 
 * 02.03.2012 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * New ProfileCreator3 allows us follow:
 * New PARAM TYPE - PROFILE_ONLY (PR_ONLY - in m_flag)
 * 		allow process these params witout our attention (no need _access & _update subroutines)
 * New Dynamic param processing, One DYN_PAR_ACCESS procedure for all params
 * New, now	We can SELF made color of param, in this case:
 * 		no need COLORTRANSLATOR  field
 * 		for alarm1,2 - also no need LIMITS
 * 
 *  03.08.2012
	new print format UINT_ZEROL0...UINT_ZEROLA %00u..%010u
	correct CMAX compare
	add limits check valid (in range)
 * 
 */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>


#include "support_common.h"
#include "HAL/MCU/inc/T8_5282_cfm.h"
#include "T8_atomic_heap.h"
#include "Application/inc/T8_Dnepr_ProfileStorage.h" // нужны обёртки для ***ValueUpdate
#include "Profile/inc/sys.h"
#include "Profile/inc/pr_ch.h"
#include "Profile/inc/extras.h"

//===== API to APP service thread ============
extern u32  app_get_dyn_param_color_glbuf(u32 p_id);
extern u32  app_get_dyn_param_val_appbuf(u32 p_id, void* buff, u32 len);

extern const PARAM pT0;
extern PARAM_INDEX param_ix[];

u32				mparams_quantity;
PROFILE_INFO	prof_info;
u32				g_paramsQuantity;
u32(*gL_portstate_func)(u32 port_id);

void log_add(const char* c){};

void* SYS_setIndex_old(void* p, u32 owner, u16 key) {
	//returne address of param
	u8* ptr;
	PARAM* header = (PARAM*)p;
	u8* ptr_limit = (u8*)INCVOID(p,header->alter_length + sizeof(PARAM));
	ptr = (u8*)INCVOID(p,sizeof(PARAM));
	while(1) {
		if (*((u16*)ptr) == key) {
			ptr += 4;
			break;
		}
		else {
			ptr += 2;
			ptr = ptr + 6 + *((u16*)ptr);
			if ((u32)ptr & 0x1)
				return NULL;
		}
		if (ptr >= ptr_limit)
			return NULL;
	}
	ptr += 4;
	return (void*)ptr;
}
//new UAV
u32 SYSGetParamID(PARAM_INDEX* p_ix){
	u32 p_id = ((u32)p_ix - (u32)param_ix)/sizeof(PARAM_INDEX);
	return p_id;
	//return p_ix->parent->global_num - 1;
}
void* SYS_setIndex(void* p, u32 owner, u16 key) {
//return address of param

	PARAM* ptr = (PARAM*)p;
	if(!p)
		return p;
	if(  (key == SYS_VALUE) && (ptr->sec_level != NOT_ACCESSIBLE) )
		return ptr->value_ptr;
	else if(key == SYS_ALARM_COLORS)
		return (ptr->colors_ptr);
	return SYS_setIndex_old(p, owner, key);
}



PARAM_INDEX* SYSGetParamIx(u32 p_id) {
	if (p_id >= g_paramsQuantity)
		return NULL;
	else
		return &param_ix[p_id];
}

PARAM* SYSGetParentParam(u32 p_id) {
	if (p_id >= g_paramsQuantity)
		return NULL;
	else
		return (PARAM*)param_ix[p_id].parent;
}

u32 SYSGetParamParamType(u32 p_id) {
	PARAM* p;
	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL ))
		return ERROR;
	else {
		p = (PARAM*)param_ix[p_id].parent;
		return (u32)p->par_type;
	}
}
u32 SYSGetParamType(u32 p_id) {
	PARAM* p;
	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL ))
		return ERROR;
	else {
		p = (PARAM*)param_ix[p_id].parent;
		return p->data_type;
	}
}
u32 SYSGetParamFlag(u32 p_id) {
	PARAM* p;
	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL ))
		return ERROR;
	else {
		p = (PARAM*)param_ix[p_id].parent;
		return (u32)p->m_flag;
	}
}
u32 SYSGetParamValue(u32 p_id,  void* buff, u32 buff_len) {
	u32 p_flag,p_type;
	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL ))
		return ERROR;

	p_flag = SYSGetParamFlag(p_id);
	if (p_flag == ERROR)
		return p_flag;	
	if ( (p_flag & PR_ONLY) || (param_ix[p_id].parent->access == NULL) ){
		//ONLY profile param
		p_type = SYSGetParamType(p_id);
		switch(p_type){
			case UINTEGER:
			case ENUM:
			case INTEGER:
			case HEX:
				return PROFILE_INTValueAccess(&param_ix[p_id], buff, buff_len);
			case BOOL:
				return PROFILE_BOOLValueAccess(&param_ix[p_id], buff, buff_len);
			case REAL:
				return PROFILE_REALValueAccess(&param_ix[p_id], buff, buff_len);
			case CHARLINE:
			case OID:
			case IPADDR:
				return PROFILE_CHARLINEValueAccess(&param_ix[p_id], buff, buff_len);
			default:
				return ERROR;
		}//switch TYPE
	}
	else{
		//call APP ACCESS
		if (param_ix[p_id].parent->access != NULL) {
			return param_ix[p_id].parent->access(&param_ix[p_id], buff, buff_len);
		}
		return ERROR;
	}
}

u32 SYSSetParamValue(u32 p_id,  void* buff) {
	u32 p_flag,p_type;

	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL ))
		return ERROR;

	//=== check may be thresholds =====
	if(OK != SYS_check_limit_in_range(p_id, buff))
		return ERROR;
	
	p_flag = SYSGetParamFlag(p_id);
	if (p_flag == ERROR)
		return p_flag;	
	if( (p_flag & PR_ONLY) || (param_ix[p_id].parent->update == NULL) ){
		//ONLY profile param
		p_type = SYSGetParamType(p_id);
		switch(p_type){
			case UINTEGER:
			case ENUM:
			case INTEGER:
			case HEX:
				return Dnepr_INTValueUpdate(&param_ix[p_id], buff);
			case BOOL:
				return  Dnepr_BOOLValueUpdate(&param_ix[p_id], buff);
			case REAL:
				return  Dnepr_REALValueUpdate(&param_ix[p_id], buff);
			case CHARLINE:
			case OID:
				return  Dnepr_CHARLINEValueUpdate(&param_ix[p_id], buff);
			case IPADDR:
				return  Dnepr_IPADDRValueUpdate(&param_ix[p_id], buff);
			default:
				return  ERROR;
		}//switch TYPE
	}
	else{
		//call APP UPDATE
		if (param_ix[p_id].parent->update == NULL)
			return ERROR;
		return  param_ix[p_id].parent->update(&param_ix[p_id], buff);
	}

}

u32 SYSSetParamValueStr(u32 p_id,  s8* buff) {
	u32 flag;
	u32 p_type;//,i;//,tmp_ix;//, val_u32;
	u8 tmp_buf[16];
	s8* tmp_pch;
	f32 tempf32;
	
	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL )){
		log_add("SYSSetParamValueStr: access PROFILE fault");
		return ERROR;
	}

	p_type = SYSGetParamType(p_id);
	switch (p_type) {
		case UINTEGER:
		case ENUM:
			errno = 0;
			*(u32*)tmp_buf = strtoul(buff,&tmp_pch,10);
			if (*tmp_pch == 'x' || *tmp_pch == 'X') {
				errno = 0;
				*(u32*)tmp_buf = strtoul(buff,&tmp_pch,16);
			}
			if (*tmp_pch == 0 && errno == 0)
				flag = SYSSetParamValue(p_id, (void*)tmp_buf);
			else
				flag = ERROR;
			break;
		case HEX:
			errno = 0;
			*(u32*)tmp_buf = strtoul(buff,&tmp_pch,16);
			if (*tmp_pch == 0 && errno == 0)
				flag = SYSSetParamValue(p_id, (void*)tmp_buf);
			else
				flag = ERROR;
			break;
		case INTEGER:
			errno = 0;
			*(s32*)tmp_buf = strtol(buff,&tmp_pch,10);
			if (*tmp_pch == 0 && errno == 0)
				flag = SYSSetParamValue(p_id, (void*)tmp_buf);
			else
				flag = ERROR;
			break;
		case REAL:
			errno = 0;
			*(f64*)tmp_buf = strtod(buff, &tmp_pch);
			if (*tmp_pch == 0 && errno == 0){
				tempf32 = (f32)*(f64*)tmp_buf;
				tempf32 = SYS_round(tempf32 , SYSGetParamFlag(p_id));
				flag = SYSSetParamValue(p_id, (void*)&tempf32);
			}
			else
				flag = ERROR;
			break;
		case CHARLINE:
		case OID:
		case IPADDR:
			flag = SYSSetParamValue(p_id, (void*)buff);
			break;
		case BOOL:
			*(u8*)tmp_buf = buff[0] - 0x30;
			flag = SYSSetParamValue(p_id, (void*)tmp_buf);
			break;

		default:
			log_add("SYSSetParamValueStr: invalid TYPE");
			flag = ERROR;
	}//switch
	return flag;
}
u32 SYSGetParamValueStr(u32 p_id, s8* buff_str, u16 buff_len) {
	u32 flag;
	u32 p_type;
	int sprintf_res;
	s8 tmp_buf[32];
	s8 format[16];
	f32 f32_tmp;
	u32 zero_lead;

	u8 precision;
	tmp_buf[0] = 0;
	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL )){
		log_add("SYSGetParamValueStr: access PROFILE fault");
		return ERROR;
	}
	
	p_type = SYSGetParamType(p_id);
	switch (p_type) {
		case UINTEGER:
		case ENUM:
			flag = SYSGetParamValue(p_id, (void*)tmp_buf, sizeof(u32));
			



			if(flag == OK){
				zero_lead = SYSGetParamFlag(p_id) & UINT_ZERO_MASK;
				if(zero_lead){
					sprintf(format,"%%0%uu",zero_lead);
					sprintf_res = sprintf(tmp_buf, format, tmp_buf); //FIXME: в оригинале был вызов vsprintf -- надо разобраться
				}
				else
					sprintf_res = sprintf(tmp_buf,"%u",*(u32*)tmp_buf);
				
				if (sprintf_res > 0 && sprintf_res < buff_len)
					strcpy(buff_str, tmp_buf);
				else
					flag=ERROR;
			}
			break;

		case HEX:
			flag = SYSGetParamValue(p_id, (void*)tmp_buf, 8);
			if (flag == OK) {
				errno = 0;
				t8_ultoa( *(u32*)tmp_buf, tmp_buf, 9, 16 );
				if ((errno == 0) && (buff_len>strlen(tmp_buf)))
					strcpy(buff_str, tmp_buf);
				else
					flag = ERROR;
			}
			break;
		case INTEGER:
			flag = SYSGetParamValue(p_id, (void*)tmp_buf, 8);
			if (flag == OK) {
				errno = 0;
				t8_itoa( *(s32*)tmp_buf, tmp_buf, 12, 10 );
				if ((errno == 0) && (buff_len>strlen(tmp_buf)))
					strcpy(buff_str, tmp_buf);
				else
					flag = ERROR;
			}
			break;
		case BOOL:
			flag = SYSGetParamValue(p_id, (void*)tmp_buf, 8);
			if (flag == OK) {
				if (buff_len >=2) {
					*(u8*)tmp_buf?(buff_str[0]='1'):(buff_str[0]='0');
					buff_str[1]=0;
				}
				else
					flag = ERROR;
			}
			break;
		case REAL:
			flag = SYSGetParamValue(p_id, (void*)tmp_buf, sizeof(f32));
			precision = SYSGetParamFlag(p_id) & PRECISION_MASK;				
			if (flag == OK) {
				f32_tmp = ASF32(tmp_buf);
				if(f32_tmp >= 100000.){
					strcpy(tmp_buf,"overflow");
					return flag;
				}					
				//errno = 0;
				switch(precision){
					case PRECISION0:
						sprintf_res = sprintf(tmp_buf, "%.0f", *(f32*)tmp_buf);
						break;
					case PRECISION1:
					default:
						sprintf_res = sprintf(tmp_buf, "%.1f", *(f32*)tmp_buf);
						break;
					case PRECISION2:
						sprintf_res = sprintf(tmp_buf, "%.2f", *(f32*)tmp_buf);
						break;
					case PRECISION3:
						sprintf_res = sprintf(tmp_buf, "%.3f", *(f32*)tmp_buf);
						break;
					case PRECISION4:
						sprintf_res = sprintf(tmp_buf, "%.4f", *(f32*)tmp_buf);
						break;
					case PRECISIONE:
						sprintf_res = sprintf(tmp_buf, "%E", *(f32*)tmp_buf);
						break;
						
				}//switch PRECISION
				
				if (sprintf_res > 0 && buff_len>strlen(tmp_buf))
					strcpy(buff_str, tmp_buf);
				else
					flag=ERROR;
			}
			break;
		case CHARLINE:
		case OID:
		case IPADDR:
			flag = SYSGetParamValue(p_id, (void*)buff_str, buff_len);
			break;
		default:
			log_add("SYSGetParamValueStr: invalid TYPE");
			flag = ERROR;
	}//SWITCH

	assert(strlen(tmp_buf) <= sizeof(tmp_buf)-1);
	return flag;
}


///////////////// U A V start /////////////////////////////////////////////

static u32 SYS_getLimitParamIds(u32 param_id, u32** trh) {
	//UAV, return pointer to list of PARAM_ID of LIMITS 
	PARAM* p = SYSGetParentParam(param_id);
	if (p == NULL || trh == NULL) 
		return ERROR;
	*trh = (u32*)SYS_setIndex((void*)p, 0, SYS_THRESHOLDS);
	if (*trh == NULL)
		return ERROR;
	return OK;
}
f32 SYS_round(f32 par,u32 mflag){
	u32 precision;
	switch (mflag & PRECISION_MASK){
	case PRECISION0:
		return (f32)round(par);
	case PRECISION1:
	default:
		precision = 10;
		break;
	case PRECISION2:
		precision = 100;
		break;
	case PRECISION3:
		precision = 1000;
		break;		
	case PRECISION4:
		precision = 10000;
		break;		
	case PRECISIONE:
		return par;
	}
	return (f32)round(par * precision)/precision;
}
static u32 SYS_compare_paramid_trhsval(u32 param_id, u32 trh){
	//compare param value with tresholds value
	u32 flag, par_type, par;
	f32 par_flt;
	
	flag = app_get_dyn_param_val_appbuf(param_id,&par,sizeof(par));//get param value
	par_type = SYSGetParamType(param_id);//get param type
	if((par_type == ERROR) || (flag != OK))
		return flag;

	switch(par_type){
		case REAL:

			par_flt = ASF32(par);
			if (par_flt > ASF32(trh))	return GREATER_THAN;
			if (par_flt < ASF32(trh))	return LESS_THAN;
			break;
		case UINTEGER:
		case ENUM:
		case BOOL:
			if (par > trh) return GREATER_THAN;
			if (par < trh) return LESS_THAN;
			break;
		case INTEGER:
			if (ASS32(par) > ASS32(trh)) return GREATER_THAN;
			if (ASS32(par) < ASS32(trh)) return LESS_THAN;
			break;
		default:
			return ERROR;
	}
	return EQUAL;
	
}
static u32 SYS_compare(u32 param_id, u32 thr_id){
	//UAV, extract PARAM from APP_PARAM_BUFF, used only for DYNAMIC params
	u32 flag, par_type;
	u32	par;
	u32 trh;
	f32 trhf32,par_flt;
	
	if(thr_id == 0)
		return NO_TRESHOLD;
	flag = SYSGetParamValue(thr_id, &trh, sizeof(trh));
	flag |= app_get_dyn_param_val_appbuf(param_id,&par,sizeof(par));
	if (flag != OK)
		return flag;
	par_type = SYSGetParamType(param_id);
	if (par_type != SYSGetParamType(thr_id)){
		log_add("SYS_compare: par/thres types differnt");
		return ERROR;
	}
	
	switch(par_type){
		case REAL:
			//par_flt = SYS_round(ASF32(par),SYSGetParamFlag(param_id));
			par_flt = ASF32(par);
			trhf32 = ASF32(trh);//(f32)ASF64(trh);
			if (par_flt > trhf32)	return GREATER_THAN;
			if (par_flt < trhf32)	return LESS_THAN;
			break;
		case UINTEGER:
		case ENUM:
		case BOOL:
			if (ASBOOL(par) > ASBOOL(trh)) return GREATER_THAN;
			if (ASBOOL(par) < ASBOOL(trh)) return LESS_THAN;
			break;
		case INTEGER:
			if (ASS32(par) > ASS32(trh)) return GREATER_THAN;
			if (ASS32(par) < ASS32(trh)) return LESS_THAN;
			break;
		default:
			return ERROR;
	}
	return EQUAL;
}
const SYS_COLORS thr_result[] = {
	SYS_CRITICAL_COLOR, SYS_MINOR_COLOR,  SYS_NORMAL_COLOR,  SYS_MINOR_COLOR, SYS_CRITICAL_COLOR
};
const RELATION thr_relation[] = {
		EQUAL, EQUAL, PASS, PASS
};
static u32 SYS_checkLimitsVal(u32 param_id, u32* trh_val_ptr){
	/*==== limits order is: Cmax,Wmax,Wmin,Cmin ==== */
	int i;
	u32 flag;

	

	for(i=0;i<4;i++){
		flag = SYS_compare_paramid_trhsval(param_id,*trh_val_ptr++);
		if (ERROR == flag)
			return flag;
		if (flag == GREATER_THAN || flag == thr_relation[i])
			return thr_result[i];
	}
	return SYS_CRITICAL_COLOR;
}
static u32 CompareF32Val(f32 val, f32 thr){
	if(val > thr) return GREATER_THAN;
	if(val < thr) return LESS_THAN;
	return EQUAL;
}
u32 SYS_checkF32LimitsValVal(f32 val, f32* limit){
	/*==== limits order is: Cmax,Wmax,Wmin,Cmin ==== All 4 thresholds must exist ======*/
	int i;
	u32 flag;
	
	for(i=0;i<4;i++){
		flag = CompareF32Val(val, *limit++);
		if (flag == GREATER_THAN || flag == thr_relation[i])
			return thr_result[i];
	}
	return SYS_CRITICAL_COLOR;
}

u32 SYSGetParamZoneVal(u32 p_id, u32* trh_val_ptr){
	if ((p_id >= g_paramsQuantity) || (param_ix[p_id].parent == NULL ))
		return ERROR;
	return SYS_checkLimitsVal(p_id, trh_val_ptr);
}
u32 SYS_check_limit_in_range(u32 param_id, void* buff){
	u32* p_thr;
	u32 thr[4];
	u32 flag,i,temp,par_type,p_id,j;
	LIMIT_ENUM limit_type = LIMIT_UNDEF;
	//u32 limit_value
	
	//find parent param in DYNAMIC section & get pointer to thresholds IDs & get threshold's type
	for(p_id=DynamicSect+1;p_id<SetSect;p_id++){
		if(OK == SYS_getLimitParamIds(p_id, &p_thr)){
			for(i=0;i<4;i++){
				if(p_thr[i] == param_id){
					limit_type = (LIMIT_ENUM)i;//save limit type(CMAX,....)
					break;//parent found
				}
			}
			if(limit_type != LIMIT_UNDEF)
				break;
		}
	}
	if(p_id >= SetSect)
		return OK;//param isn't threshold
	
	//check param type
	par_type = SYSGetParamType(param_id);
	if( (par_type == ERROR) || (par_type != SYSGetParamType(p_id))){
		log_add("SYS_check_limit_in_range: invalid param type");
		return ERROR;
	}

	//read thresholds values
	for(i=0;i<4;i++){
		if(!p_thr[i]) continue;//pass if no threshold
		flag = SYSGetParamValue(p_thr[i], (void*)&thr[i], sizeof(u32));
		if (flag != OK)
			return flag;
	}
	
	//check all limits values in correct range
/*	if(limit_value > CMAX){
		log_add("SYS_check_limit_in_range:  invalid limit type");
		return ERROR;		
	}*/
	memcpy(&thr[limit_type],buff,4);
	//thr[limit_type] = ASU32(buff);	//set new threshold value
	for(i=0;i<3;i++){
		if(!p_thr[i]) continue;//pass if no threshold
		temp = thr[i];
		for(j=i+1;j<=3;j++){
			if(!p_thr[j]) continue;//pass if no threshold
			switch(par_type){
				case REAL:
					if (ASF32(temp) > ASF32(thr[j]))	return ERROR;
					break;
				case UINTEGER:
				case ENUM:
					if (temp >= thr[j]) return ERROR;
					break;
				case BOOL:
					if(ASBOOL(temp) >= ASBOOL(thr[j])) return ERROR;
					break;
				case INTEGER:
					if (ASS32(temp) > ASS32(thr[j])) return ERROR;
					break;
				default:
					return ERROR;		
			}//switch
		}//for j
	}//for i
	return OK;
}
static u32 SYS_checkLimits(u32 param_id){
	/* ======   limits order is : Cmin,Wmin,Wmax,Cmax	======== */
	u32* thr;
	int i;
	u32 flag;
	RELATION rel[4];
	
	if(OK != SYS_getLimitParamIds(param_id, &thr))
		return ERROR;
	//--- do 4 compare ---
	flag = 0;
	for(i=0;i<4;i++){
		rel[i] = (RELATION)SYS_compare(param_id,thr[i]);
		flag |= rel[i];
	}
	//--- check result of compare --
	if(flag == ERROR)
		return flag;
	if(  (rel[0] == LESS_THAN)    || (rel[0] == EQUAL) ||
		 (rel[3] == GREATER_THAN) || (rel[3] == EQUAL)	)
		return SYS_CRITICAL_COLOR;
	if(  (rel[1] == LESS_THAN)    || (rel[1] == EQUAL) ||
		 (rel[2] == GREATER_THAN) || (rel[2] == EQUAL)	)
		return SYS_MINOR_COLOR;
	return SYS_NORMAL_COLOR;

}
u32 SYSGetParamZone(u32 p_id){
/* UAV new get param zone
 * Input:
 * 		p_id - profile line's number
 * Return:
 * 	2|3|5|7 
 */
	if (NULL == SYSGetParentParam(p_id))
		return ERROR;
	return SYS_checkLimits(p_id);
	

}
u32 SYSConvertColor(u32 p_id,u32 param_color){
	u32 flag;
	u8 failure_color, degrade_color, normal_color;

	flag = SYSGetParamColors(p_id, &failure_color, &degrade_color, &normal_color);
	if (flag == OK){
		switch(param_color){
			case	7:
				if(failure_color)
					param_color = failure_color;
				break;
			case	5:
				if(degrade_color)
					param_color = degrade_color;
				break;
			case	3:
				if(normal_color)
					param_color = normal_color;
				break;
			default:
				//param_color = SYS_EMPTY_COLOR_ERROR;
				break;
		}
	}
	return param_color;
}
///////////////////////////////////////// U A V end /////////////////////////////////////


void ta_involver();

void SYS_Init() {

	PARAM* param_header;

	void* p = (void*)(&pT0);
	ta_involver();
	g_paramsQuantity = 0;
	SYS_profstat_init();
	gL_portstate_func = NULL;

	while (*((u32*)p)) {
		g_paramsQuantity++;
		param_header = (PARAM*)p;
		if (param_header->owner < 64512)
			SYS_profstat_add(param_header);

		p = INCVOID(p,param_header->alter_length + sizeof(PARAM));
	}

}

void SYS_profstat_init() {
	u32 i;
	prof_info.lines_quantity = 0;
	for (i=0; i<MAX_PRSECTIONS_NUMBER; i++)
		prof_info.info[i].start = ERROR;
	return;
}

u32 SYS_convert_sect_number(u32 sect_n) {
	switch (sect_n) {
		case 0:   return 0;
		case 1:   return 1;
		case 2:   return 2;
		case 3:   return 3;
		case 4:   return 4;
		case 5:   return 5;
		case 6:   return 6;
		case 7:   return 7;
		case 8:   return 8;
		case 9:   return 9;
		case 10:  return 10;
		case 11:  return 11;
		case 12:  return 12;
		case 13:  return 13;
		case 100: return 14;
		case 101: return 15;
		case 102: return 16;
		case 103: return 17;
		case 200: return 18;
		default:  return ERROR;
	}
}

u32 SYS_profstat_add(PARAM* par) {
	u32 i;
	u32 sect_ix;
	prof_info.lines_quantity++;
	sect_ix = SYS_convert_sect_number(par->par_type);
	if (sect_ix != ERROR) {
		if (prof_info.info[sect_ix].start == ERROR) {
			for (i=0; param_ix[i].parent != NULL; i++) {
				if ((void*)param_ix[i].parent == par) {
					prof_info.info[sect_ix].start = i;
					prof_info.info[sect_ix].quantity = 1;
					break;
				}
			}
		}
		else
			prof_info.info[sect_ix].quantity++;
	}
	return 0;
}


u32 SYSSetIndexBySN(u32 sn) {
	u32 start_id;
	PARAM* p;
//*****BLOCK OF INITIAL CHECKS*****
	if (sn == 0)
		return ERROR;
//*********************************
	start_id = sn-1;
	while ((p = SYSGetParentParam(start_id)) != NULL) {
		if (p->global_num == sn)
			break;
		else
			start_id += 1;
	}
	if (p!= NULL)
		return start_id;
	else
		return ERROR;
}


void* SYSSetIndexByPtr(void* p,u32 owner, u32 key) {
	return SYS_setIndex(p,owner,key);
}

u32 SYSCalcLines() {
	return prof_info.lines_quantity;
}

u32 SYSSectionFirst(u32 sect_n) {
	u32 i;
	PARAM* p_header;
	if ((((sect_n <= PR_CALIB_COEFF) && (sect_n >= PR_INFORM_PARAM))||
		 ((sect_n<=PR_ADDSECT200) && (sect_n>=PR_ADDSECT100)))) {
		sect_n = SYS_convert_sect_number(sect_n);
		if (sect_n == ERROR)
			return ERROR;
		p_header = SYSGetParentParam(prof_info.info[sect_n].start);
		if (p_header==NULL)
			return ERROR;
		for (i=prof_info.info[sect_n].start; p_header != NULL && p_header->local_num == 0; i++)
			p_header = SYSGetParentParam(i);
		if (i==0)
			return i;
		else
			return i-1;
	}
	return ERROR;
}

u32 SYSSectionNext(u32 par) {
	u32 flag=ERROR;
	PARAM* header_next;
	PARAM* header_prev;
//*****BLOCK OF INITIAL CHECKS*****
	if ((header_prev = SYSGetParentParam(par)) == NULL)
		return flag;
//*********************************
//Find next parameter in the same section with same owner 	
	for (par=par+1; (header_next=SYSGetParentParam(par)) != NULL; par=par+1) {
		if (header_next->par_type != header_prev->par_type)
			break;
		if (header_next->owner < 64512) {
			flag = OK;
			break;
		}
	}
	if (flag==OK)
		return par;
	else
		return flag;
}


u32 SYSSetIndexBySN2(u32 sect_n, u32 param_n) {
	PARAM* header;
	u32 p_id;
	sect_n = SYS_convert_sect_number(sect_n);
	if (sect_n>=MAX_PRSECTIONS_NUMBER)
		return ERROR;
	if (param_n < 1)
		return ERROR;
	if (prof_info.info[sect_n].start == 0)
		return ERROR;
	if (prof_info.info[sect_n].quantity<=param_n)
		return ERROR;
	p_id = param_n - 1 + prof_info.info[sect_n].start;
	header = SYSGetParentParam(p_id);
	while (header != NULL && header->local_num != param_n) {
		p_id++;
		header = SYSGetParentParam(p_id);
	}
	if (header != NULL && header->local_num == param_n)
		return p_id;
	else
		return ERROR;
}



u32 SYS_CheckValue(PARAM_INDEX* p_ix, void* value) {
	u32 flag = ERROR;
	void* threshold;
	if (p_ix != NULL) {
		switch (p_ix->parent->data_type) {
		case REAL:
			if ((threshold = SYS_setIndex((void*)p_ix->parent, 0, SYS_MAXVALUE))
					!= NULL)
				if (*(f64*)threshold < *(f64*)value)
					{flag = ERROR; break;}
			if ((threshold = SYS_setIndex((void*)p_ix->parent, 0, SYS_MINVALUE))
					!= NULL)
				if (*(f64*)threshold > *(f64*)value)
				{flag = ERROR; break;}
			flag = OK; break;
		case INTEGER:
			if ((threshold = SYS_setIndex((void*)p_ix->parent, 0, SYS_MAXVALUE))
					!= NULL)
				if (*(s32*)threshold < *(s32*)value)
					{flag = ERROR; break;}
			if ((threshold = SYS_setIndex((void*)p_ix->parent, 0, SYS_MINVALUE))
					!= NULL)
				if (*(s32*)threshold > *(s32*)value)
				{flag = ERROR; break;}
			flag = OK; break;
		case UINTEGER:
			if ((threshold = SYS_setIndex((void*)p_ix->parent, 0, SYS_MAXVALUE))
					!= NULL)
				if (*(u32*)threshold < *(u32*)value)
					{flag = ERROR; break;}
			if ((threshold = SYS_setIndex((void*)p_ix->parent, 0, SYS_MINVALUE))
					!= NULL)
				if (*(u32*)threshold > *(u32*)value)
					{flag = ERROR; break;}
			flag = OK; break;
		case ENUM:
		case HEX:
		case BOOL:
			flag = OK;
		}
	}
	return flag;
}



u32 SYSGetParamColors(u32 p_id, u8* failure_color, u8* degrade_color, u8* normal_color) {
	u32 flag=ERROR;
	PARAM* param;
	u8* colors;
	param = SYSGetParentParam(p_id);
	if (param != NULL) {
		if (param->par_type == PR_DYNAMIC_PARAM) {
			if ((colors = SYS_setIndex(param, 0, SYS_ALARM_COLORS)) != NULL) {
				if (failure_color != NULL)
					*failure_color = *colors;
				if (degrade_color != NULL)
					*degrade_color = *(colors+1);
				if (normal_color != NULL)
					*normal_color = *(colors+2);
				flag = OK;
			}
		}
	}
	return flag;
}


u32 SYSWriteParamColors(u32 p_id, u8 failure_color, u8 degrade_color, u8 normal_color) {
	u32 flag=ERROR;
	PARAM* param;
	void* colors;
	u8 new_colors[3];
	param = SYSGetParentParam(p_id);
	if (param != NULL) {
		if (param->par_type == PR_DYNAMIC_PARAM) {
			colors = SYS_setIndex(param, 0, SYS_ALARM_COLORS);
			if (colors != NULL) {
				new_colors[0] = failure_color;
				new_colors[1] = degrade_color;
				new_colors[2] = normal_color;
				CFM_WriteData(colors, new_colors, sizeof(new_colors));
				flag = OK;
			}
		}
	}
	return flag;
}


void SYSRegPortFunc(u32 (*port_func)(u32 port_id)) {
	gL_portstate_func = port_func;
	return;
}


u32 SYSGetPortState(u32 port_id) {
	if (gL_portstate_func != NULL) 
		return gL_portstate_func(port_id);
	return ERROR;
}


u32 SYSGetParamPortID(u32 p_id) {
	PARAM* param;
	param = SYSGetParentParam(p_id);
	if (param != NULL) {
		if (param->owner < 1024)
			return param->owner;
	}
	return ERROR;
}

#define __SYS_HIDDEN_PARAMS_LEN		128 // сколько параметров может быть спрятано
// PARAM ID спрятаных параметров
static u32 __sys_hidden_params[ __SYS_HIDDEN_PARAMS_LEN ];
// сколько ячеек __sys_hidden_params занято
static u32 __sys_hidden_params_ind = 0 ;

void SYS_hide_param( u32 param_id, u32 hide )
{
	u32 i ;
	// ищем этот параметр
	for( i = 0; (__sys_hidden_params[i] != param_id) && (i < __sys_hidden_params_ind); ++i );
	// такой параметр уже есть и его надо удалить
	if( (i < __sys_hidden_params_ind) && (!hide) ){
		for( ; i < __sys_hidden_params_ind; i++ ){
			__sys_hidden_params[i] = __sys_hidden_params[i+1] ;
		}
		--__sys_hidden_params_ind ;
	// такого параметра нет и его надо добавить
	} else if( (i >= __sys_hidden_params_ind) && (hide) &&
				// длины массива хватит
				(__sys_hidden_params_ind < (__SYS_HIDDEN_PARAMS_LEN-1)) ){
		__sys_hidden_params[__sys_hidden_params_ind++] = param_id ;
	}
}

u32  SYS_is_param_hidden( u32 param_id )
{
	u32 i ;
	// ищем этот параметр
	for( i = 0; (__sys_hidden_params[i] != param_id) && (i < __sys_hidden_params_ind); ++i );
	// есть такой параметр в массиве
	if( i < __sys_hidden_params_ind ){
		return OK ;
	} else {
		return ERROR ;
	}
}

