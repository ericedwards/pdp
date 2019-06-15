/*
 * This file is part of 'pdp', a PDP-11 simulator.
 *
 * For information contact:
 *
 *   Computer Science House
 *   Attn: Eric Edwards
 *   Box 861
 *   25 Andrews Memorial Drive
 *   Rochester, NY 14623
 *
 * Email:  mag@potter.csh.rit.edu
 * FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
 * 
 * Copyright 1994, Eric A. Edwards
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Eric A. Edwards makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * boot.c - Boot Code
 */


#include "defines.h"


d_word bootcode_tm[] = {
	0012700,	/* mov	#172526,r0	*/
	0172526,
	0010040,	/* mov	r0,-(r0)	*/
	0012740,	/* mov	#60003,-(r0)	*/
	0060003,
	0012700,	/* mov	#172522,r0	*/
	0172522,
	0105710,	/* tstb	(r0)		*/
	0100376,	/* bpl	-1		*/
	0005000,	/* clr	r0		*/
	0000110		/* jmp	(r0)		*/
};

d_word bootcode_rl[] = {
	0012737,	/* mov	#10,174400	*/
	0000010,
	0174400,
	0105737,	/* tstb	174400		*/
	0174400,
	0100375,	/* bpl	-2		*/
	0013700,	/* mov	174406,r0	*/
	0174406,
	0042700,	/* bic	#177,r0		*/
	0000177,
	0052700,	/* bis	#1,r0		*/
	0000001,
	0010037,	/* mov	r0,174404	*/
	0174404,
	0012737,	/* mov	#6,174400	*/
	0000006,
	0174400,
	0105737,	/* tstb	174400		*/
	0174400,
	0100375,	/* bpl	-2		*/
	0012700,	/* mov	#174406,r0	*/
	0174406,
	0012710,	/* mov	#177400,(r0)	*/
	0177400,
	0005040,	/* clr	-(r0)		*/
	0005040,	/* clr	-(r0)		*/
	0012740,	/* mov	#14,-(r0)	*/
	0000014,
	0105737,	/* tstb	174400		*/
	0174400,
	0100375,	/* bpl	-2		*/
	0005000,	/* clr	r0		*/
	0000110		/* jmp	(r0)		*/
};


/*
 * boot_read() - Read a word of the boot code.
 */

int
boot_read( addr, word )
c_addr addr;
d_word *word;
{
	unsigned offset;

	offset = (addr & 0177) / 2;

	switch(( addr & 0600 ) >> 7 ) {
	case 0:
		*word = bootcode_rl[ offset % (sizeof(bootcode_rl) / 2)];
		break;
	case 1:
		*word = bootcode_tm[ offset % (sizeof(bootcode_tm) / 2)];
		break;
	default:
		*word = 0;
		break;
	}
	return OK;
}


/*
 * boot_write() - Write a word of the boot code.
 */

/*ARGSUSED*/
int
boot_write( addr, word )
c_addr addr;
d_word word;
{
	return OK;
}


/*
 * boot_bwrite() - Write a byte of the boot code.
 */

/*ARGSUSED*/
int
boot_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	return OK;
}
