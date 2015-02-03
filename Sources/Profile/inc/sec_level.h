/*
 * sec_level.h
 *
 *  Created on: 04.06.2012
 *      Author: urusov
 */

#ifndef SEC_LEVEL_H_
#define SEC_LEVEL_H_

/*#define MR	1
#define MW	2
#define UR	4
#define UW	8
#define	AR	16
#define AW	32
#define SR	64
#define SW	128*/

#define	n  0
#define r  1
#define w  2
#define rw 3
#define SL(SL_M,SL_U,SL_A,SL_S) (SL_M | (SL_U<<2) | (SL_A<<4) | (SL_S<<6))

enum _PARAM_ON_OFF{
	Off = 0,
	On
};
enum _MODULE_STATE{
	OOS=0,
	OOS_MT,
	IS
};
enum _MODULE_MODE{
	NORMA=0,
	REGENERATOR,
	TEST_REGENERATOR
};
enum _SFP_TRH{
	SFP = 0,
	User
};
enum _LOOP_BACK{
	Client = 0,
	Line,
	No
};
#endif /* SEC_LEVEL_H_ */
