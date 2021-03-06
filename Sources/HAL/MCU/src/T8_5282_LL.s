/*
 * File:    mcf5xxx.s
 * Purpose: Lowest level routines for all ColdFire processors.
 *
 * Notes: ��. NEVA/mcf5xxx.s  
 *
 */

    .global asm_set_ipl
    .global _asm_set_ipl
    .global mcf5xxx_wr_sr
    .global _mcf5xxx_wr_sr

    .text


/********************************************************************/
/*
 * This routines changes the IPL to the value passed into the routine.
 * It also returns the old IPL value back.
 * Calling convention from C:
 *   old_ipl = asm_set_ipl(new_ipl);
 * For the Diab Data C compiler, it passes return value thru D0.
 * Note that only the least significant three bits of the passed
 * value are used.
 */

asm_set_ipl:
_asm_set_ipl:
    link    A6,#-8
    movem.l D6-D7,(SP)

    move.w  SR,D7       	/* current sr    */

    move.l  D7,D0       	/* prepare return value  */
    andi.l  #0x0700,D0  	/* mask out IPL  */
    lsr.l   #8,D0       	/* IPL   */

    move.l  8(A6),D6    	/* get argument  */
    andi.l  #0x07,D6        /* least significant three bits  */
    lsl.l   #8,D6       	/* move over to make mask    */

    andi.l  #0x0000F8FF,D7  /* zero out current IPL  */
    or.l    D6,D7           /* place new IPL in sr   */
    move.w  D7,SR

    movem.l (SP),D6-D7
    lea.l     8(SP),SP
//	adda.l	#8, SP			// FIXME: ���� ������ ���� ����������������
    unlk    A6
    rts
 
/********************************************************************/
/*
 * These routines write to the special purpose registers in the ColdFire
 * core.  Since these registers are write-only in the supervisor model,
 * no corresponding read routines exist.
 */
   
mcf5xxx_wr_sr:
_mcf5xxx_wr_sr:
    move.l  4(SP),D0
    move.w  D0,SR
    rts

/********************************************************************/
    .end
