//--------------------------------------------------------------------------
//-| FILENAME: pr_ch.h
//-|
//-| Created on: 28.06.2010
//-| Author: Konstantin Tyurin
//--------------------------------------------------------------------------

#ifndef PR_CH_H_
#define PR_CH_H_

#include "Profile/inc/sys.h"

u32 PROFILE_IPADDRValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 PROFILE_CHARLINEValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 PROFILE_REALValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 PROFILE_INTValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 PROFILE_BOOLValueAccess(PARAM_INDEX* p_ix, void* buff, u32 buff_len);

u32 PROFILE_IPADDRValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 PROFILE_CHARLINEValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 PROFILE_INTValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 PROFILE_REALValueUpdate(PARAM_INDEX* p_ix, void* buff);
u32 PROFILE_BOOLValueUpdate(PARAM_INDEX* p_ix, void* buff);


#endif /* PR_CH_H_ */
