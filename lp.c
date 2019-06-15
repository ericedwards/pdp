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
 * lp.c - LP11 device simulator.  
 */


#include "defines.h"


/*
 * LP11 Stuff.
 */

#define LP_VECTOR	0200
#define LP_PRI		4
#define LP_DELAY	6	/* 1/10th of a second delay.... */

struct lp_regs {
	d_word csr;
	d_word data;
	FILE *fp;
} lp;

#define LP_ERR	0100000
#define LP_RDY	0200
#define LP_IE	0100


/*
 * lp_init() - Initialize the LP11 controller.
 */

int
lp_init()
{
	if ( lp.fp != NULL )
		fclose( lp.fp );
	if (( lp.fp = fopen( lp_gpath, "a" )) == NULL )
		lp.csr = LP_ERR;
	else
		lp.csr = LP_RDY;
}


/*
 * lp_read() - Handle the reading of an LP11 register.
 */

int
lp_read( addr, word )
c_addr addr;
d_word *word;
{
	switch( addr ) {
	case LP11:
		*word = lp.csr;
		break;
	case LP11 + 2:
		*word = lp.data;
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * lp_write() - Handle a write to one of the LP11
 * registers.
 */

int
lp_write( addr, word )
c_addr addr;
d_word word;
{
	int lp_finish();
	int c;

	switch( addr ) {
	case LP11:
		lp.csr &= ~LP_IE;	/* only write to LP_IE */
		lp.csr |= word & LP_IE;
		if (( lp.csr & LP_IE ) == LP_IE )
			ev_register( LP_PRI, lp_finish,
			 (unsigned long) LP_DELAY, (d_word) 0 );
		break;
	case LP11 + 2:
		if ( lp.fp != NULL ) {
			c = word & 0377;
			fputc( c, lp.fp );
			if (( c == '\r' ) || ( c == '\n' )) {
				fflush( lp.fp );
				lp.csr &= ~LP_RDY;
			}
		}
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * lp_bwrite() - Handle a byte write by doing a word write.
 */

int
lp_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	d_word word = byte;

	return lp_write( addr, word );
}


/*
 * lp_finish()
 */

lp_finish( info )
d_word info;
{
	lp.csr |= LP_RDY;
	lp.csr &= ~LP_IE;
	service( (d_word) LP_VECTOR );
}
