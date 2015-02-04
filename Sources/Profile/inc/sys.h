//--------------------------------------------------------------------------
//-| FILENAME: sys.h
//-|
//-| Created on: 07.09.2009
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#ifndef SYS_H_
#define SYS_H_

#include "support_common.h"
#include "ErrorManagement/status_codes.h"
#include "Profile/Generated/profile_def.h"

//UAV maximum param length
#ifndef PARAM_MAX_LEN
#	define PARAM_MAX_LEN 256
#endif

#ifndef MAX_PRSECTIONS_NUMBER
	#define MAX_PRSECTIONS_NUMBER 24
#endif

#ifndef MAX_PORT_NUMBER
	#define MAX_PORT_NUMBER		  41
#endif

#define PR_HEADER_NAME		0x00
#define PR_INFORM_PARAM		0x01
#define PR_DYNAMIC_PARAM	0x02
#define PR_SET_PARAM		0x03
#define PR_STATIC_PARAM		0x04
#define PR_LED_PARAM		0x05
#define PR_CALIB_DEVICE		0x06
#define PR_CALIB_COEFF		0x07
#define PR_PORT				0x08
#define PR_ENUMPARAM		0x09
#define PR_COLORTRANSLATOR	0x0A
#define PR_PARAMRELATIONS	0x0B
#define PR_PARAMSWITCHES	0x0C
#define PR_GROUPPARAM		0x0D
#define PR_ADDSECT100		100
#define PR_ADDSECT101		101
#define PR_ADDSECT102		102
#define PR_ADDSECT103		103
#define PR_ADDSECT200		200


#define COLOR_TRANSLATOR 	0x08

/*user level*/
#define UL1_R	0x01
#define UL1_W	0x02
#define UL1_RW	(UL1_R | UL1_W)
#define UL2_R	0x04
#define UL2_W	0x08
#define UL2_RW	(UL2_R | UL2_W)
#define UL3_R	0x10
#define UL3_W	0x20
#define UL3_RW	(UL3_R | UL3_W)
#define UL4_R	0x40
#define UL4_W	0x80
#define UL4_RW	(UL4_R | UL4_W)
#define UL_ALL_R	(UL1_R | UL2_R | UL3_R | UL4_R)
#define UL_ALL_W	(UL1_W | UL2_W | UL3_W | UL4_W)
#define UL_ALL_RW	(UL_ALL_R | UL_ALL_W)


/*constant level*/
#define CL1_R	0x01  /* Monitor read */
#define CL1_W	0x02  /* Monitor write */
#define CL2_R	0x04  /* User read */
#define CL2_W	0x08  /* User write */
#define CL3_R	0x10  /* Admin read */
#define CL3_W	0x20  /* Admin write */
#define CL4_R	0x40  /* Supervisor read */
#define CL4_W	0x80  /* Supervisor write */
#define CL1_RW	(CL1_R | CL1_W)
#define CL2_RW	(CL2_R | CL2_W)
#define CL3_RW	(CL3_R | CL3_W)
#define CL4_RW	(CL4_R | CL4_W)
#define CL_ALL_R	(CL1_R | CL2_R | CL3_R | CL4_R)
#define CL_ALL_W	(CL1_W | CL2_W | CL3_W | CL4_W)
#define CL_ALL_RW	(CL_ALL_R | CL_ALL_W)

#define ALL_RW  (CL_ALL_RW | UL_ALL_RW)
#define ALL_R	(CL_ALL_R | UL_ALL_R)


#define NOT_ACCESSIBLE 0

#define CU_W	0
#define CU_RW	0

//type u32
#define BOOL			0x01
#define INTEGER			0x02
#define UINTEGER		0x04
#define REAL			0x08
#define HEX				0x10
#define CHARLINE		0x20
#define ENUM			0x40
#define OID				0x80
#define IPADDR			0x100

#define CHARLINE_LEN	0x7F
#define REAL_LEN		0x04
#define UINTEGER_LEN	0x04
#define ENUM_LEN		0x04
#define INTEGER_LEN		0x04
#define HEX_LEN			0x04
#define BOOL_LEN		0x01

//======== MFLAG u16 ==========
#define NONMETER		0x0000          /*!< параметр  */
//precision
#define PRECISION0		0x0010
#define PRECISION1		0x0011
#define PRECISION2		0x0012
#define PRECISION3		0x0013
#define PRECISION4		0x0014
#define PRECISIONE		0x001E	/* printf in dddE+-dd */
#define PRECISION_MASK	0x001F
//flags
#define GRP_ACCESS		0x0020          /*!< параметр  */
#define PR_ONLY			0x0040          /*!< параметр  */
#define TRP_COLOR		0x0100
#define SFP_COLOR		0x0200
#define BLK_COLOR		0x0400
#define AL1_COLOR		0x0800
#define AL2_COLOR		0x1000
#define AL12_COLOR		(AL1_COLOR|AL2_COLOR)
#define EEPROM_PARAM	        (0x2000)	/*!< параметр дублируется в EEPROM backplane'а */
//uint lead zero
#define UINT_ZEROL0		0x0000
#define UINT_ZEROL1		0x0001
#define UINT_ZEROL2		0x0002
#define UINT_ZEROL3		0x0003
#define UINT_ZEROL4		0x0004
#define UINT_ZEROL5		0x0005
#define UINT_ZEROL6		0x0006
#define UINT_ZEROL7		0x0007
#define UINT_ZEROL8		0x0008
#define UINT_ZEROL9		0x0009
#define UINT_ZEROLA		0x000A
#define UINT_ZERO_MASK	0x000F
//#define NO_COLOR		0x100


//#define DDM_RXP		0x10
//#define DDM_TXP		0x11
//#define DDM_ILD		0x12
//#define DDM_VCC		0x13
//#define DDM_TTX		0x14

#define SYS_EVENT_STR 		0x12
#define SYS_LEDS_MASKS 		0x13
#define SYS_EV_MASKS		0x14
#define SYS_PARAM_NAME		0x15
#define SYS_COMMENT			0x16
#define SYS_VALUE			0x17
#define SYS_SECLEVEL		0x18
#define SYS_UNIT			0x19
#define SYS_MAXVALUE		0x20
#define SYS_MINVALUE		0x21
#define SYS_THRESHOLDS		0x22
#define SYS_ALARM_COLORS	0x23

//UAV
/*enum _LIMITS_PLACE_ENUM{
	LIMITS_APP = 0,
	LIMTS_PROFILE,
};*/
typedef enum _PARAM_ALARM_STATE_ENUM{
	ALARM_UNDEF		= 0UL,
	ALARM_NORMAL	= 3UL,
	ALARM_WARNING	= 5UL,
	ALARM_ALARM		= 7UL
}PARAM_ALARM_STATE;

typedef enum _LED_STATE_ENUM{
	SYS_LED_OFF		= 0UL,
	SYS_LED_GREEN	= 1UL,
	SYS_LED_RED		= 2UL,
	SYS_LED_YELLOW	= 3UL
} LED_STATE, *LED_STATE_PTR;
//==========================================
typedef enum _SYS_COLORS_ENUM{
	SYS_NA_COLOR = 0,
	SYS_NO_COLOR,
	SYS_INFO_COLOR,
	SYS_NORMAL_COLOR,
	SYS_WARNING_COLOR,
	SYS_MINOR_COLOR,
	SYS_MAJOR_COLOR,
	SYS_CRITICAL_COLOR,
	SYS_EMPTY_COLOR_ERROR // не писать цвет у параметра
} SYS_COLORS;

typedef enum _PORT_SERVICE_STATES_ENUM{
	PORT_OUT_OF_SERVICE=0,
	PORT_OUT_OF_SERVICE_MT=1,
	PORT_IN_SERVICE=2,
	PORT_UNDEFINED = 255
} PORT_SERVICE_STATES;

typedef enum _RELATION_ENUM {
	GREATER_THAN = 0x4911UL,
	LESS_THAN,
	EQUAL,
	NO_TRESHOLD,
	PASS
}RELATION;

typedef enum _LIMIT_ENUM{
	CMIN=0,WMIN,WMAX,CMAX,LIMIT_UNDEF=255
}LIMIT_ENUM;

/* UAV add common PARAMS structure */
typedef struct _P32{
	u32 ready;
	u32 par_color;
	union {
		f32 F32;
		u32 U32;
		s32 S32;
		LED_STATE LED;
		s8 * text;
	} value;
}P32,*P32_PTR;

struct _RAM_PARAM;
typedef  struct _RAM_PARAM RAM_PARAM;

struct  _PARAM_INDEX_STRUCT ;
typedef struct  _PARAM_INDEX_STRUCT PARAM_INDEX ;

///////////////////* UAV added *///////////////////////////////////


/*This is constant part of parameter, that live in flash*/
/*It's length = ... bytes*/
/*Field "owner": value [0:1023] - port id, value [64512:65535] - system internal id*/
/*UAV: Dependence with ProfileCreator.exe*/
typedef struct _PARAM{
	u32	alter_length;
	u32 sec_level;
	u32 (*access)(PARAM_INDEX*, void*, u32);	//for CU
	u32 (*update)(PARAM_INDEX*, void*);			//for CU
	u32 (*getvalue)(PARAM_INDEX*,P32_PTR);		//MCU internal
	u16 owner;
	u16 global_num;//1,2,3, .........
	u16 local_num;//0,1,2, ....
	u16 data_len;
	u16  par_type;
	u16 data_type;
	u16  m_flag;
	void* value_ptr;//UAV added
	void* colors_ptr;//UAV added
/* Further we have some variable data fields length, and fields
 * that not included in all parameters
 * u32 mask;
 * u8 leds_masks[5];
 * s8* name;
 * s8* comment;
 *
 *  event string
 *
 *
 * */
} PARAM;

struct _RAM_PARAM{
	struct _RAM_PARAM* next;
	struct _PARAM* index;
	u32 ix;
	u32 zone;
	u8	ready;
	u8	locked;
};

/*UAV: Dependence with ProfileCreator.exe*/
struct _PARAM_INDEX_STRUCT{
	PARAM const * 		parent;
	RAM_PARAM* 	child;
} ;

typedef struct {
	u32 start;
	u32 quantity;
} PROFINFO;

typedef struct {
	u32 lines_quantity;
	PROFINFO info[MAX_PRSECTIONS_NUMBER];
} PROFILE_INFO;

#if(0)
//TODO : remove conditional compilation, must be the same for all projects
//#if  defined BAIKAL || defined AKSAY 
//[0] - HMin, ..., [5] - HMaxDEV_PORT2_CHNUM,
typedef struct {
	u32	limit_present;
	s32 limits[LIMITS_CAPACITY];
} DEVICE_LIMITS_S32_STRUCT;


//[0] - HMin, ..., [5] - HMaxDEV_PORT2_CHNUM,
typedef struct {
	u32	limit_present;
	u32 limits[LIMITS_CAPACITY];
} DEVICE_LIMITS_U32_STRUCT;


//[0] - HMin, ..., [5] - HMax
typedef struct {
	u32	limit_present;
	f32 limits[LIMITS_CAPACITY];
} DEVICE_LIMITS_F32_STRUCT;


//[0] - HMin, ..., [5] - HMax
typedef struct {
	u32	limit_present;
	union {
		f32 F32[LIMITS_CAPACITY];
		u32 U32[LIMITS_CAPACITY];
		s32 S32[LIMITS_CAPACITY];
	} limits;
} DEVICE_LIMITS_32BIT_STRUCT;
#endif

// Bootloader
typedef void (* func)(void);
typedef void (*ldrEntry)(u32,u32);
#define LOADER_ENTRY 0x00001fe0 

extern PROFILE_INFO	prof_info;


void SYS_Init();

u32 SYSGetParamZone(u32 p_id);
u32 SYSGetParamZoneVal(u32 p_id,u32* trh_val_ptr);
u32 SYSGetParamColor(u32 p_id);
f32 SYS_round(f32 par,u32 precision);
u32 SYSConvertColor(u32 p_id,u32 param_color);
u32 SYSGetParamFlag(u32 p_id);
f64 SYS_round64(f64 par,u32 mflag);
u32 SYS_checkF32LimitsValVal(f32 val, f32* limit);

u32 SYSGetParamParamType(u32 p_id);
u32 SYSGetParamType(u32 p_id);
u32 SYSGetParamPortID(u32 p_id);
PARAM* SYSGetParentParam(u32 p_id);
u32 SYSGetParamID(PARAM_INDEX* p_ix);
PARAM_INDEX* SYSGetParamIx(u32 p_id);

u32 SYSGetParamValueStr(u32 p_id, s8* buff_str, u16 buff_len);
u32 SYSGetParamValueStr2(u32 p_id, s8* buff_str, u16 buff_len);
u32 SYSGetParamValue(u32 p_id,  void* buff, u32 buff_len);
u32 SYSSetParamValue(u32 p_id,  void* buff);
u32 SYS_profstat_add(PARAM*);
void SYS_profstat_init();
u32 SYSSetIndexBySN(u32);
u32 SYSSetIndexBySN2(u32,u32);
void* SYSSetIndexByPtr(void*,u32, u32);
u32 SYSSectionFirst(u32);
u32 SYSSectionNext(u32);
u32 SYSCalcLines();
u32 SYSSetParamValueStr(u32 p_id,  s8* buff);
u32 SYSGetParamColors(u32 p_id, u8* failure_color, u8* degrade_color, u8* normal_color);
u32 SYSWriteParamColors(u32 p_id, u8 failure_color, u8 degrade_color, u8 normal_color);
PARAM_INDEX* SYSGetParamIx(u32 p_id);
void SYSRegPortFunc(u32 (*port_func)(u32 port_id));
u32 SYSGetPortState(u32 port_id);
u32 SYS_CheckValue(PARAM_INDEX* p_ix, void* value);
void* SYS_setIndex(void* p, u32 owner, u16 key);
u32 SYS_convert_sect_number(u32 sect_n);
u32 SYS_check_limit_in_range(u32 param_id, void* buff);

// прячут параметр в системе управления совсем (делают уровень доступа SL(0,0,0,0)),
// смотри profile_create_str
void SYS_hide_param( u32 param_id, u32 hide ); // hide == 1 или 0
u32  SYS_is_param_hidden( u32 param_id );


#endif /* SYS_H_ */
