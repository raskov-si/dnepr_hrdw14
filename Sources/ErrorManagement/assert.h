/*
\file assert.h
\date 23.05.2012
\author Daniel Leonov
 */

#ifndef __ASSERT_H_
#define __ASSERT_H_

#include "T8_Atomiccode.h"
#include <fs_api.h> /* File I/O routines */

#define ASSERT_CONDITION_MAXLEN 64
#define ASSERT_FILE_NAME_MAXLEN 256
#define ASSERT_FUNCTION_NAME_MAXLEN 64

/* Uncomment this define if you wish to write assert messages to a file.  */
#ifndef __DEBUG
//#define ASSERT_WRITE_TO_FILE
#endif

#ifdef ASSERT_WRITE_TO_FILE
	#define ASSERT_MESSAGE_FORMAT "Assertion  \"%s\" failed in \"%s\", function \"%s\", line %d, uptime %0.3f s. \n"
	
	//#define ASSERT_PATH_TO_FILE "F:\\Assert.txt"
	#define ASSERT_PATH_TO_FILE "F:\\Assert\\Assert.dat"
	#define ASSERT_FILE_DIR     "F:\\Assert"
	#define ASSERT_EOF_SIZE 1

/*	Uncomment ASSERT_READ_FROM_EOF to read n last bytes instead of n first bytes with ASSERT_ReadFile() */
#define ASSERT_READ_FROM_EOF

#endif //ASSERT_WRITE_TO_FILE

/*! Structre for condition, file name, line number etc. */
typedef struct __ASSERT_ConditionParamsTypedef{
	char sCondition[ASSERT_CONDITION_MAXLEN];    /*! Failed condition */
	char sFile[ASSERT_FILE_NAME_MAXLEN];         /*! Path to file */
	char sFunction[ASSERT_FUNCTION_NAME_MAXLEN]; /*! Function name */
	u16 nLine;                                   /*! Number of line in file */
	u32 nTimeH;                                  /*! 64-bit time value, 4 bytes high */
	u32 nTimeL;                                  /*! 64-bit time value, 4 bytes low */
}ASSERT_ConditionParamsTypedef;

void ASSERT_InitConditionParamsStructure(ASSERT_ConditionParamsTypedef* tConditionParamsStructure);
void ASSERT_SendMessage(ASSERT_ConditionParamsTypedef* tConditionParamsStructure);
void ASSERT_Idle(void);
void ASSERT_FailedAssertionHandler(const char* sCondition, const char* sFile, const char* sFunction, unsigned short int nLine);

#define assert(condition) ((condition) ? ((void) 0) : ASSERT_FailedAssertionHandler(#condition, __FILE__, __func__, __LINE__))

#ifdef ASSERT_WRITE_TO_FILE
_BOOL ASSERT_ReadFile(char* sFilePath, char* sReadBuffer, u32 nBufferSize);
_BOOL ASSERT_WipeFile(char* sFilePath);
#endif //ASSERT_WRITE_TO_FILE

#endif /* __ASSERT_H_ */
