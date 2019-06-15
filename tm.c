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
 * tm.c - TM11 device simulator.
 */

#include "defines.h"

/*
 * Definitions for the TM11 simulation.
 */

#define TM_DELAY	5000	/* delay in microseconds = 1 ms */
#define TM_PRI		5	/* interrupt priority */
#define TM_VECTOR	0224	/* interrupt vector */

/*
 * Register bit defines.
 */

#define TM_GO		01
#define TM_RCOM		02
#define TM_WCOM		04
#define TM_WEOF		06
#define TM_SFORW	010
#define TM_SREV		012
#define TM_WIRG		014
#define TM_REW		016

#define TM_CRDY		0200
#define TM_IE		0100
#define	TM_CE		0100000

#define TM_TUR		01
#define TM_WRL		04
#define TM_BOT		040
#define	TM_SELR		0100
#define TM_NXM		0200
#define	TM_EOT		02000
#define	TM_EOF		040000
#define	TM_ILC		0100000

/*
 * Per controller info.
 */

struct tm_regs {
	d_word tmer;		/* controller register images */
	d_word tmcs;
	d_word tmbc;
	d_word tmba;
	d_word tmdb;
	d_word tmrd;
	FILE *fp;		/* the current file used for tape data */
	char path[BUFSIZ];	/* full pathname to tape data files */
	int number;		/* which file is currently open, -1 = EOT */
};

/*
 * The TM11 controller information.
 */

struct tm_regs tm;
d_word tm_buffer[512/2];
d_word tm_swab[512/2];

/*
 * tm_init() - Initialize the TM11 controller.
 */

int
tm_init()
{
	tm.tmcs = TM_CRDY;
	tm.tmer = TM_TUR;
	if ( tm.fp != NULL ) {		/* for reinits */
		fclose( tm.fp );
	}
	tm.fp = NULL;
	strcpy( tm.path, tm_gpath );
	tm.number = 0;
	tm_next_file();
}

/*
 * tm_next_file() - Open the Unix file that simulates the next file on
 * the tape.
 */

tm_next_file()
{
	char fullpath[BUFSIZ];

	strcpy( fullpath, tm.path );
	sprintf( &fullpath[strlen(fullpath)], ".%d", tm.number );

	if ( tm.fp != NULL )
		fclose( tm.fp );
	if (( tm.fp = fopen( fullpath, "a+" )) == NULL ) { 
		if (( tm.fp = fopen( fullpath, "r" )) == NULL ) {
			tm.tmer |= TM_EOT;
			tm.number = -1;
		} else {
			tm.tmer |= TM_WRL;
		}
	} else {
		rewind( tm.fp );
		tm.tmer &= ~TM_WRL;
	}
	tm.tmer |= TM_SELR;
	if ( tm.number == 0 )
		tm.tmer |= TM_BOT;
}

/*
 * tm_read() - Handle the reading of an TM11 register.
 */

int
tm_read( addr, word )
c_addr addr;
d_word *word;
{
	switch( addr ) {
	case TM11:
		*word = tm.tmer;
		break;
	case TM11 + 2:
		*word = tm.tmcs;
		break;
	case TM11 + 4:
		*word = tm.tmbc;
		break;
	case TM11 + 6:
		*word = tm.tmba;
		break;
	case TM11 + 010:
		*word = tm.tmdb;
		break;
	case TM11 + 012:
		*word = tm.tmrd;
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}

/*
 * tm_write() - Handle a write to one of the TM11
 * registers.  A command is started when the control
 * and status register is written with the TM_GO bit
 * set.
 */

int
tm_write( addr, word )
c_addr addr;
d_word word;
{
	switch( addr ) {
	case TM11:

		/*
		 * Don't really let them write the error register.
		 */

		break;

	case TM11 + 2:

		/*
		 * Don't let the program stomp the controller ready
		 * or error bits.
		 */

		tm.tmcs &= (TM_CRDY|TM_CE);
		tm.tmcs |= word & ~(TM_CRDY|TM_CE);

		/*
		 * Used to check for just TM_GO, but the 2.9 BSD
		 * autoconfig routine just sets TM_IE to make
		 * the TM interrupt.  Woo woo.
		 */

		if ( tm.tmcs & ( TM_GO|TM_IE )) {
			tm.tmcs &= ~TM_CRDY;
			tm_exec();
		}
		break;

	case TM11 + 4:
		tm.tmbc = word;
		break;
	case TM11 + 6:
		tm.tmba = word;
		break;
	case TM11 + 010:
		tm.tmdb = word;
		break;
	case TM11 + 012:
		tm.tmrd = word;
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}

/*
 * tm_bwrite() - Ok, who's the retard?  Byte writes should
 * probably be allowed to the TM11, but I'm too lazy.
 */

/*ARGSUSED*/
int
tm_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	return BUS_ERROR;
}

/*
 * tm_exec() - Perform a command execution.
 */

int
tm_exec()
{
	void tm_do_read();
	void tm_do_write();
	void tm_do_weof();
	void tm_do_sforw();
	void tm_do_srev();
	int tm_finish();

	unsigned long tm_delay = TM_DELAY;

	tm.tmcs &= ~TM_CE;		/* clear error conditions */
	tm.tmer &= ~(TM_ILC|TM_NXM);
	switch ( tm.tmcs  & 016 ) {
	case TM_REW:
		tm.tmer &= ~(TM_EOT|TM_EOF);	/* perform the rewind */
		tm.number = 0;
		tm_next_file();
		break;
	case TM_RCOM:			/* read command */
		tm_do_read();
		break;
	case TM_WCOM:			/* write command */
	case TM_WIRG:			/* maybe do this one too... */
		tm_do_write();
		break;
	case TM_WEOF:			/* write eof command */
		tm_do_weof();
		break;
	case TM_SFORW:			/* space forward */
		tm_do_sforw();
		break;
	case TM_SREV:			/* space reverse */
		tm_do_srev();
		break;
	default:
		if ( tm.tmcs & TM_GO ) {
			tm.tmer |= TM_ILC;	/* bad command, if go set */
		}
		tm_delay = 0;
		break;
	}

	if ( tm.tmcs & TM_IE )
		ev_register( TM_PRI, tm_finish, tm_delay, (d_word) 0 );
	else
		tm_finish( (d_word) 0 );
}

/*
 * tm_finish()
 */

tm_finish( info )
d_word info;
{
	if ( tm.tmer & ( TM_ILC|TM_EOT|TM_NXM ))
		tm.tmcs |= TM_CE;
	tm.tmcs |= TM_CRDY;

	if ( tm.tmcs & TM_IE ) {
		tm.tmcs &= ~TM_IE;
		service( (d_word) TM_VECTOR );
	}
}

/*
 * tm_do_sforw() - We'll sleeze through this one too, assume record size
 * is always 512 bytes for the space forward command.
 */

void
tm_do_sforw()
{
	unsigned count;

	tm.tmer &= ~TM_EOF;
	count = ( 0177777 - tm.tmbc ) + 1;
	while( count-- && (tm.fp != NULL)) {
		tm.tmer &= ~TM_BOT;
		if ( fread( tm_buffer, 512, 1, tm.fp ) == 0 ) {
			tm.tmer |= TM_EOF;
			tm.number++;
			tm_next_file();
			break;
		}
	}
	tm.tmbc = ( 0177777 - count ) + 1;
}

/*
 * tm_do_srev() - Required by the 2.9 tm driver during writes.  The
 * routine tm_close writes two EOFs then backspaces over the last one.
 * We'll try to simulate the backspace just for this special case.
 */

void
tm_do_srev()
{
	unsigned count;

	tm.tmer &= ~TM_EOF;
	count = ( 0177777 - tm.tmbc ) + 1;
	--count;
	tm.tmer &= ~TM_BOT;
	if ( tm.number > 0 ) {
		tm.number--;
		tm_next_file();
	}
	tm.tmbc = ( 0177777 - count ) + 1;
}

/*
 * tm_do_read() - Perform a read.  We can simplify things
 * by assuming that the number of bytes asked for will be
 * equal to what the the record size was on the real tape.
 * This is a _very_ big assumtion, read the tape at your own
 * risk.
 */

void
tm_do_read()
{
	unsigned count;
	unsigned bytes;
	c_addr addr;
	unsigned t;

	tm.tmer &= ~TM_EOF;
	count = ( 0177777 - tm.tmbc ) + 1;
	addr = tm.tmba;
	addr |= ((c_addr)(tm.tmcs & 060)) << 12;

	while( count && (tm.fp != NULL)) {
		tm.tmer &= ~TM_BOT;
		bytes = ( count >= 512 ) ? 512 : count;
		count -= bytes;
#ifndef SWAB
		if ( fread( tm_buffer, bytes, 1, tm.fp ) == 0 ) {
#else
		if ( fread( tm_swab, bytes, 1, tm.fp ) == 0 ) {
#endif
			tm.tmer |= TM_EOF;
			tm.number++;
			tm_next_file();
			break;
		} else {
#ifdef SWAB
			swab((char *) tm_swab, (char *) tm_buffer,
				sizeof( tm_swab ));
#endif
			for( t = 0;  t < ( bytes / 2 ); ++t ) {
				if ( sc_word( addr, tm_buffer[t] ) != OK ) {
					tm.tmer |= TM_NXM;
					return;
				}
				addr += 2;
			}
		}
	}
	tm.tmbc = ( 0177777 - count ) + 1;
}

/*
 * tm_do_write() - Perform a write.
 */

void
tm_do_write()
{
	unsigned count;
	unsigned bytes;
	c_addr addr;
	unsigned t;

	if ( tm.tmer & TM_WRL ) {
		tm.tmer |= TM_ILC;
		return;
	}

	tm.tmer &= ~TM_EOF;
	count = ( 0177777 - tm.tmbc ) + 1;
	addr = tm.tmba;
	addr |= ((c_addr)(tm.tmcs & 060)) << 12;

	while( count && (tm.fp != NULL)) {
		tm.tmer &= ~TM_BOT;
		bytes = ( count >= 512 ) ? 512 : count;
		count -= bytes;
		for( t = 0;  t < ( bytes / 2 ); ++t ) {
			if ( lc_word( addr, &tm_buffer[t] ) != OK ) {
				tm.tmer |= TM_NXM;
				return;
			}
			addr += 2;
		}
#ifdef SWAB
		swab((char *) tm_buffer, (char *) tm_swab,
			sizeof( tm_swab ));
		if ( fwrite( tm_swab, bytes, 1, tm.fp ) == 0 ) {
#else
		if ( fwrite( tm_buffer, bytes, 1, tm.fp ) == 0 ) {
#endif
			tm.tmer |= TM_EOF;
			tm.number++;
			tm_next_file();
			break;
		}
	}
	if ( tm.fp != NULL )
		fflush( tm.fp );
	tm.tmbc = ( 0177777 - count ) + 1;
}

/*
 * tm_do_weof() - Perform a end-of-file write.
 */

void
tm_do_weof()
{
	tm.tmer &= ~TM_EOF;
	tm.number++;
	tm_next_file();
}
