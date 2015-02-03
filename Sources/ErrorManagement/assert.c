/*!
\file assert.c
\brief Assertion handler for Dnepr.
\author Daniel Leonov, <a href="mailto:leonov@t8.ru">leonov@t8.ru</a>
\date 23.05.2012
*/

#include "support_common.h"
#include "ErrorManagement/assert.h"
#include <string.h>
#include <stdio.h>   /* sprintf() */
#include "uCOS_II.H" /* OSTimeGet() */

#define ASSERT_CLOCKS_PER_SEC OS_TICKS_PER_SEC
#define ASSERT_MESSAGE_DEBUG_MODE



#ifdef ASSERT_WRITE_TO_FILE
static void ASSERT_WriteToFile(char* sFilePath, ASSERT_ConditionParamsTypedef* tConditionParamsStructure);
#endif //ASSERT_WRITE_TO_FILE

static ASSERT_ConditionParamsTypedef __tAssertConditionParameters;

void ASSERT_FailedAssertionHandler(const char* sCondition, const char* sFile, const char* sFunction, unsigned short int nLine){
	/*
	\brief Assert handler for Dnepr.
	\param sCondition Failed condition.
	\param sFile Path to file.
	\param sFunction Function name.
	\param nLine Number of line in file.
	\retval None.
	*/
	STORAGE_ATOMIC() ;
	//char sAssertMessage[256];
	
	START_ATOMIC();
	
	/* Preparing parameters for the message */
	ASSERT_InitConditionParamsStructure(&__tAssertConditionParameters);
	strncpy(__tAssertConditionParameters.sCondition, sCondition, ASSERT_CONDITION_MAXLEN - 1 );
	__tAssertConditionParameters.sCondition[ ASSERT_CONDITION_MAXLEN - 1 ]='\0';
	strncpy(__tAssertConditionParameters.sFile, sFile, ASSERT_FILE_NAME_MAXLEN - 1 );
	__tAssertConditionParameters.sFile[ ASSERT_FILE_NAME_MAXLEN - 1] = '\0' ;
	strncpy(__tAssertConditionParameters.sFunction, sFunction, ASSERT_FUNCTION_NAME_MAXLEN - 1 );
	__tAssertConditionParameters.sFunction[ ASSERT_FUNCTION_NAME_MAXLEN -1 ] = '\0' ;
	__tAssertConditionParameters.nLine=nLine;
	__tAssertConditionParameters.nTimeL= llUptime ;
	
	//Sending message (in any way)
	ASSERT_SendMessage(&__tAssertConditionParameters);	
	//Going to loop/handler/...
	ASSERT_Idle();
	
	return;
}

void ASSERT_InitConditionParamsStructure(ASSERT_ConditionParamsTypedef* tConditionParamsStructure){
	/*
	\brief Fills \ref ASSERT_ConditionParamsTypedef structure with its default values.
	\param tConditionParamsStructure Target structure.
	\retval None.
	*/	
	
	strcpy(tConditionParamsStructure->sCondition, "\0");
	strcpy(tConditionParamsStructure->sFile, "\0");
	strcpy(tConditionParamsStructure->sFunction, "\0");
	tConditionParamsStructure->nLine  = 0x0000;
	tConditionParamsStructure->nTimeH = 0x00000000;
	tConditionParamsStructure->nTimeL = 0x00000000;
	
	return;
}

void ASSERT_SendMessage(ASSERT_ConditionParamsTypedef* tConditionParamsStructure){
	/*
	\brief Sends a message about failed assert \em somewhere.
	\param tConditionParamsStructure Failed assertion details.
	\retval None.
	*/
	
	/* Save it somewhere */
	#ifdef ASSERT_WRITE_TO_FILE
	ASSERT_WriteToFile(ASSERT_PATH_TO_FILE, tConditionParamsStructure);
	#endif //ASSERT_WRITE_TO_FILE
	
	/* Loop inside function for debug cases */
	#ifdef ASSERT_MESSAGE_DEBUG_MODE
	while(1);
	#endif //ASSERT_MESSAGE_DEBUG_MODE
	
	/* After all messages are sent, idle loop. */
	ASSERT_Idle(); 
	return;
}

void ASSERT_Idle(void){
	/*
	\brief Infinite loop for a failed assertion condition.
	\details This is the loop program jumps into after any \c assert() has failed and all respective messages were sent.
	\retval None.
	*/
	
	while(1);
}


#ifdef ASSERT_WRITE_TO_FILE
static char __sAssertMessage[256];

static void ASSERT_WriteToFile(char* sFilePath, ASSERT_ConditionParamsTypedef* tConditionParamsStructure){
	/*
	\brief Adds assert message to a given file in FAT32 storage device.
	\param sFilePath Path to file to write to.
	\param __sAssertMessage Record to add to a file.
	\retval None.
	\details This function requires ititialized file system and created directories.
	*/
	FS_FILE *fAssertFile;
	u32 nStrLen;
	
	/* Composing text message */
	sprintf(__sAssertMessage, ASSERT_MESSAGE_FORMAT,
			tConditionParamsStructure->sCondition,
			tConditionParamsStructure->sFile,
			tConditionParamsStructure->sFunction,
			tConditionParamsStructure->nLine,
			(f32)(tConditionParamsStructure->nTimeL)/ASSERT_CLOCKS_PER_SEC );
	
	fAssertFile = FS_FOpen(sFilePath, "a");
	if( fAssertFile==NULL ) return; /* We cannot assert while we assert */
	
	nStrLen = strlen(__sAssertMessage);
	
	FS_FWrite(__sAssertMessage, 1, nStrLen, fAssertFile);
	FS_FClose(fAssertFile);
	
	return;		
}
#endif //ASSERT_WRITE_TO_FILE

#ifdef ASSERT_WRITE_TO_FILE
_BOOL ASSERT_ReadFile(char* sFilePath, char* sReadBuffer, u32 nBufferSize){
	/*!
	\brief Reads messages from assert log file.
	\param sFilePath Path to assert log file.
	\param sReadBuffer Array pointer to read to.
	\param nBufferSize Number of bytes to read.
	\note If \p ASSERT_READ_FROM_EOF is defined, this function reads \p nBufferSize last characters in the file. Otherwise it reads from begining of the file.
	*/
	FS_FILE *fAssertFile;

	fAssertFile = FS_FOpen(sFilePath, "r+");
	if( fAssertFile==NULL ) return 0; /* Or try-catch */

	/* Read from beginning or the eno of the file */
	#ifdef ASSERT_READ_FROM_EOF
	FS_FSeek(fAssertFile, -nBufferSize-ASSERT_EOF_SIZE, FS_SEEK_END);
	#endif //ASSERT_READ_FROM_EOF

	FS_FRead(sReadBuffer, 1, nBufferSize, fAssertFile);

	FS_FClose(fAssertFile);

	return 1;
}
#endif //ASSERT_WRITE_TO_FILE

#ifdef ASSERT_WRITE_TO_FILE
_BOOL ASSERT_WipeFile(char* sFilePath){
	/*!
	\brief Clears assert log file
	\retval 1 if file was successfullt cleared, 0 otherwise.
	*/
	
	FS_FILE *fAssertFile;

	fAssertFile = FS_FOpen(sFilePath, "w");
	if( fAssertFile==NULL ) return 0;
	FS_FClose(fAssertFile);

	return 1;
}
#endif //ASSERT_WRITE_TO_FILE


