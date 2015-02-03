//--------------------------------------------------------------------------
//-| FILENAME: cfm.h
//-|
//-| Created on: 17.06.2010
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#ifndef CFM_H_
#define CFM_H_

#include "support_common.h"

u32 CFM_WriteData(void* addr, void* data, u32 len);
u32 CFM_WriteData2048(void* addr, void* data);


#endif /* CFM_H_ */
