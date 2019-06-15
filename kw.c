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
 * kw.c - KW11 device simulator.
 */


#include "defines.h"


/*
 * KW11 Stuff.
 */

#define KW_IE		0100
#define KW_VECTOR	0100
#define KW_PRI		6
#define KW_DELAY	16667	/* about a 60th of a second in usec */

d_word kw;


/*
 * kw_init() - Initialize the KW11 controller.
 */

int
kw_init()
{
	kw = 0;
}


/*
 * kw_read() - Handle the reading of an KW11 register.
 */

int
kw_read( addr, word )
c_addr addr;
d_word *word;
{
	switch( addr ) {
	case KW11:
		*word = kw;
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * kw_write() - Handle a write to one of the KW11
 * registers.
 */

int
kw_write( addr, word )
c_addr addr;
d_word word;
{
	int kw_finish();

	switch( addr ) {
	case KW11:
		kw = word & KW_IE;
		if ( kw & KW_IE )
			ev_register( KW_PRI, kw_finish,
				(unsigned long) KW_DELAY, (d_word) 0 );
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * kw_bwrite() - Handle a byte write by doing a word write.
 */

int
kw_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	d_word word = byte;

	return kw_write( addr, word );
}


/*
 * kw_finish()
 */

kw_finish( info )
d_word info;
{
	kw &= ~KW_IE;
	service( (d_word) KW_VECTOR );
}
