/*********************************************************************
 *
 * Copyright:
 *	1999-2000 MOTOROLA, INC. All Rights Reserved.  
 *  You are hereby granted a copyright license to use, modify, and
 *  distribute the SOFTWARE so long as this entire notice is
 *  retained without alteration in any modified and/or redistributed
 *  versions, and that such modified versions are clearly identified
 *  as such. No licenses are granted by implication, estoppel or
 *  otherwise under any patents or trademarks of Motorola, Inc. This 
 *  software is provided on an "AS IS" basis and without warranty.
 *
 *  To the maximum extent permitted by applicable law, MOTOROLA 
 *  DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED, INCLUDING 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR
 *  PURPOSE AND ANY WARRANTY AGAINST INFRINGEMENT WITH REGARD TO THE 
 *  SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF) AND ANY 
 *  ACCOMPANYING WRITTEN MATERIALS.
 * 
 *  To the maximum extent permitted by applicable law, IN NO EVENT
 *  SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER (INCLUDING 
 *  WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS 
 *  INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY
 *  LOSS) ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.   
 * 
 *  Motorola assumes no responsibility for the maintenance and support
 *  of this software
 ********************************************************************/

/*
 * File:		MCF5282.h
 * Purpose:		MCF5282 definitions
 *
 * Notes:
 */
//#include "mcf5xxx.h"
//#include "ctypes.h"
#ifndef _CPU_MCF_H
#define _CPU_MCF_H

//i2c

#define MCF_GPIO_PASPAR  	(*(vu16 *)(void *)(&__IPSBAR[0x100056]))

#define MCF_GPIO_PASPAR_PASPA5(x)		(((x)&0x3)<<10)
#define MCF_GPIO_PASPAR_PASPA4(x)		(((x)&0x3)<<8)
#define MCF_GPIO_PASPAR_PASPA3(x)		(((x)&0x3)<<6)
#define MCF_GPIO_PASPAR_PASPA2(x)		(((x)&0x3)<<4)
#define MCF_GPIO_PASPAR_PASPA1(x)		(((x)&0x3)<<2)
#define MCF_GPIO_PASPAR_PASPA0(x)		(((x)&0x3))

//spi

#define MCF_QSPI_QCR_CONT				(0x8000)
#define MCF_QSPI_QCR_CS(x)				(((x)&0x000F)<<8)
#define MCF_QSPI_QMR_BITS_8  			(0x2000)

#define MCF_GPIO_PQSPAR  	(*(vu8  *)(void *)(&__IPSBAR[0x100059]))

#define MCF_GPIO_PQSPAR_PQSPA6			(0x40)
#define MCF_GPIO_PQSPAR_PQSPA5			(0x20)
#define MCF_GPIO_PQSPAR_PQSPA4			(0x10)
#define MCF_GPIO_PQSPAR_PQSPA3			(0x08)
#define MCF_GPIO_PQSPAR_PQSPA2			(0x04)
#define MCF_GPIO_PQSPAR_PQSPA1			(0x02)
#define MCF_GPIO_PQSPAR_PQSPA0			(0x01)
#define MCF_GPIO_PQSPAR_PQSPA(x)		(0x01<<x)
#define MCF_GPIO_PQSPAR_PQSPAR0(x)     (((x)&0x0003)<<0)
#define MCF_GPIO_PQSPAR_PQSPAR1(x)     (((x)&0x0003)<<2)
#define MCF_GPIO_PQSPAR_PQSPAR2(x)     (((x)&0x0003)<<4)
#define MCF_GPIO_PQSPAR_PQSPAR3(x)     (((x)&0x0003)<<6)
#define MCF_GPIO_PQSPAR_PQSPAR4(x)     (((x)&0x0003)<<8)
#define MCF_GPIO_PQSPAR_PQSPAR5(x)     (((x)&0x0003)<<10)
#define MCF_GPIO_PQSPAR_PQSPAR6(x)     (((x)&0x0003)<<12)


#endif	/* _CPU_MCF5272_H */
