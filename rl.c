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
 * rl.c - RL11/RL01/RL02 device simulator.  Just enough functionality
 * is provided to satisfy simple simulations, don't you dare try to run
 * DEC diagnostics...  For example, the bus and disk address registers
 * and the transfer word count aren't updated at all during or after a
 * transfer.
 */

#include "defines.h"

/*
 * Definitions for the RL11/RL01/RL02 simulation.
 */

#define RL_DELAY	5000		/* number of microseconds  = 5 ms */
#define RL_PRI		5		/* interrupt priority */
#define RL_VECTOR	0160		/* interrupt vector number */
#define MAX_RL		4		/* drives per controller */

/*
 * Control register bits.
 */

#define RL_DRDY		01
#define RL_NOP		0
#define RL_WRCK		02
#define RL_GSTAT	04
#define RL_SEEK		06
#define RL_RDHEAD	010
#define RL_WCOM		012
#define RL_RCOM		014
#define RL_RDNOCK	016
#define RL_IE		0100
#define RL_CRDY		0200
#define RL_OPI		02000
#define RL_DCRC		04000
#define RL_HNF		010000
#define RL_NXM		020000
#define RL_DE		040000
#define RL_CE		0100000

#define	RL_VC		01000
#define RL_BH		010
#define	RL_HO		020
#define RL_CO		040
#define	RL_LOCK		05
#define RL_02		0200

#define RL_RESET	010

/*
 * Internal info.
 */

#define RL_TYPE_NORL	0
#define RL_TYPE_RL01	1
#define RL_TYPE_RL02	2

/*
 * Drive geometry info.
 */

#define RL_CYL_RL01	256
#define RL_CYL_RL02	512
#define RL_NUM_SECT	40
#define RL_NUM_HEADS	2
#define RL_WORDS_SECTOR	128
#define RL_BYTES_SECTOR	(RL_WORDS_SECTOR * 2)
#define RL_BYTES_TRACK	(RL_BYTES_SECTOR * RL_NUM_SECT)
#define RL_BYTES_CYL	(RL_BYTES_TRACK * RL_NUM_HEADS)
#define RL_SIZE_RL01	(RL_BYTES_CYL * RL_CYL_RL01 )
#define RL_SIZE_RL02	(RL_BYTES_CYL * RL_CYL_RL02 )

/*
 * Info structure.
 */

struct rl_regs {
	d_word csr;			/* controller register images */
	d_word bar;
	d_word dar;
	d_word mpr;
	unsigned drive;			/* drive number for current oper */
	struct _info {			/* per drive information */
		unsigned exists;	/* does drive exist */
		unsigned cylinder;	/* current cylinder */
		unsigned head;		/* current head */
		unsigned error;		/* non-zero if drive in error */
		FILE *file;		/* where the data lives (file ptr) */
	} info[MAX_RL];
};

/*
 * The RL11/RL01/RL02 controller information.
 */

struct rl_regs rl;
d_word rl_buffer[RL_WORDS_SECTOR];

/*
 * rl_init() - Initialize the RL11 controller and
 * RL01/RL02 drive simulation.  The drives are simulated
 * with a simple unix file.
 */

int
rl_init()
{
	char fullpath[BUFSIZ];
	int x;

	rl.csr = RL_CRDY;		/* reset controller */
	rl.bar = 0;
	rl.dar = 0;
	rl.mpr = 0;
	rl.drive = 0;
	for( x = 0; x < MAX_RL; ++x ) {
		if ( rl.info[x].file != NULL )		/* close files */
			fclose( rl.info[x].file );
		rl.info[x].exists = RL_TYPE_NORL;
		rl.info[x].cylinder = 0;
		rl.info[x].head = 0;
		rl.info[x].error = 0;
		if ( rl_gpath != NULL ) {
			strcpy( fullpath, rl_gpath );
			sprintf( &fullpath[strlen(fullpath)], ".%d", x );
			if (( rl.info[x].file = fopen( fullpath, "r+" )) !=
			 NULL ) {
				fseek( rl.info[x].file, 0L, 2 );
				switch( ftell( rl.info[x].file )) {
				case RL_SIZE_RL01:
					rl.info[x].exists = RL_TYPE_RL01;
					fseek( rl.info[x].file, 0L, 0 );
					break;
				case RL_SIZE_RL02:
					rl.info[x].exists = RL_TYPE_RL02;
					fseek( rl.info[x].file, 0L, 0 );
					break;
				default:
					fclose( rl.info[x].file );
					break;
				}
			}
		}
	}
}

/*
 * rl_read() - Handle the reading of an RL11 register.  This is
 * simple since the RL11 doesn't change the registers as a result
 * of the read.  The multi-purpose register actually does change,
 * but we'll cheat by not fully implementing it yet.
 */

int
rl_read( addr, word )
c_addr addr;
d_word *word;
{
	switch( addr ) {
	case RL11:
		*word = rl.csr;
		break;
	case RL11 + 2:
		*word = rl.bar;
		break;
	case RL11 + 4:
		*word = rl.dar;
		break;
	case RL11 + 6:
		*word = rl.mpr;
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}

/*
 * rl_write() - Handle a write to one of the RL11
 * registers.  All registers except the csr are just
 * handled by saving the value.  If the csr is
 * written with the RL_CRDY bit cleared, rl_exec is
 * called to handle the command.
 */

int
rl_write( addr, word )
c_addr addr;
d_word word;
{
	switch( addr ) {
	case RL11:
		rl.csr = word;
		if (( rl.csr & RL_CRDY ) == 0 )
			rl_exec();
		break;
	case RL11 + 2:
		rl.bar = word & 0177776;	/* lowest bit always zero */
		break;
	case RL11 + 4:
		rl.dar = word;
		break;
	case RL11 + 6:
		rl.mpr = word;
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}

/*
 * rl_bwrite() - Ok, who's the retard?  Byte writes should
 * probably be allowed to the RL11, but I'm too lazy.
 */

/*ARGSUSED*/
int
rl_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	return BUS_ERROR;
}

/*
 * rl_exec() - Handle the incoming commmand.
 */

int
rl_exec()
{
	int rl_finish();
	unsigned long rl_delay = RL_DELAY;	/* default operation time */

	rl.drive = ( rl.csr >> 8 ) & 3;
	if ( rl.info[rl.drive].exists == RL_TYPE_NORL ) {
		rl.info[rl.drive].error = 1;
		rl_delay = 0;			/* finish quickly */
		if (( rl.csr & 016 ) == RL_GSTAT )
			rl.mpr = RL_CO|RL_BH;	/* the cover is open */
	} else {
		switch( rl.csr & 016 ) {
		case RL_NOP:
			rl_delay = 0;			/* finish quickly */
			/* null command, do nothing yet */
			break;
		case RL_GSTAT:
			rl.mpr = RL_HO|RL_BH|RL_LOCK;	/* drive is cool */
			if ( rl.info[rl.drive].exists == RL_TYPE_RL02 )
				rl.mpr |= RL_02;
			if ( rl.dar & RL_RESET )	/* clear drive error */
				rl.info[rl.drive].error = 0;
			rl_delay = 0;			/* finish quickly */
			break;
		case RL_SEEK:
			if ( rl_seek()) {		/* do the seek */
				rl.info[rl.drive].error = 1;
			}
			break;
		case RL_RDHEAD:
			rl.mpr = rl.info[rl.drive].cylinder << 7;
			rl.mpr |= rl.info[rl.drive].head << 6;
			break;
		case RL_RCOM:
			if ( rl_do_read()) {
				rl.info[rl.drive].error = 1;
			}
			break;
		case RL_WCOM:
			if ( rl_do_write()) {
				rl.info[rl.drive].error = 1;
			}
			break;
		case RL_WRCK:
		case RL_RDNOCK:
		default:
			/* unsupported commands, do nothing yet */
			rl_delay = 0;			/* finish quickly */
			break;
		}
	}

	/*
	 * Call rl_finish() to complete the command if someone
	 * is polling the device.  Hey, look, drive operations
	 * are wicked fast... If interupts are enabled, make it
	 * happen later.
	 */

	if ( rl.csr & RL_IE )
		ev_register( RL_PRI, rl_finish, rl_delay, (d_word) 0 );
	else
		rl_finish( (d_word) 0 );
}

/*
 * rl_finish() - Finish the current command.  Set the error bits and
 * mark the controller ready.
 */

int
rl_finish( info )
d_word info;
{
	if ( rl.info[rl.drive].error ) {
		rl.csr |= RL_DE;
		rl.csr &= ~RL_DRDY;
	} else {
		rl.csr &= ~RL_DE;
		rl.csr |= RL_DRDY;
	}
	if ( rl.csr & 0176000 ) {
		rl.csr |= RL_CE;
	}
	rl.csr |= RL_CRDY;

	if ( rl.csr & RL_IE ) {
		service( (d_word) RL_VECTOR );
	}
}

/*
 * rl_seek() - Do the mechanics of a controller seek command.  Return
 * non-zero on any kind of failure.
 */

int
rl_seek()
{
	unsigned diff;
	unsigned max;

	rl.info[rl.drive].head = ( rl.dar >> 4 ) & 1;
	diff = ( rl.dar >> 7 );
	if ( rl.info[rl.drive].exists == RL_TYPE_RL01 ) {
		max = RL_CYL_RL01;
	} else {
		max = RL_CYL_RL02;
	}
	if ( rl.dar & 04 ) {
		if (( rl.info[rl.drive].cylinder + diff ) >= max )
			return -1;
		else
			rl.info[rl.drive].cylinder += diff;
	} else {
		if ( diff > rl.info[rl.drive].cylinder )
			return -1;
		else
			rl.info[rl.drive].cylinder -= diff;
	}
	return 0;
}

/*
 * rl_do_read() - Perform the actual read of the disk and
 * simulate the DMA.
 */

int
rl_do_read()
{
	unsigned sector;
	unsigned offset;
	c_addr addr;
	unsigned count;
	unsigned t;

	/*
	 * Check to see if on proper cylinder.  We let the user
	 * slide by not checking for the right head.
	 */

	if ( rl.info[rl.drive].cylinder != ( rl.dar >> 7 )) {
		rl.csr |= RL_HNF;
		return -1;
	}

	/*
	 * Calculate head and first sector, also the core
	 * address and word count.
	 */

	rl.info[rl.drive].head = ( rl.dar >> 6 ) & 1;
	sector = rl.dar & 077;
	addr = rl.bar;
	addr += ((c_addr)(rl.csr & 060)) << 12;
	count = ( 0177777 - rl.mpr ) + 1;

	/*
	 * Make sure sector is in range.
	 */

	if ( sector >= RL_NUM_SECT ) {
		rl.csr |= RL_HNF;
		return -1;
	}
	
	offset = (( rl.info[rl.drive].cylinder * RL_BYTES_CYL ) +
		  ( rl.info[rl.drive].head * RL_BYTES_TRACK ) +
		  ( sector * RL_BYTES_SECTOR ));

	if ( fseek( rl.info[rl.drive].file, (long) offset, 0 ) != 0 ) {
		rl.csr |= RL_HNF;
		return -1;
	}

	while ( count ) {
		if ( fread((char *) rl_buffer, RL_BYTES_SECTOR, 1,
		 rl.info[rl.drive].file ) == 0 ) {
			rl.csr |= RL_DCRC;
			return -1;
		}
		for( t = 0; ( t < RL_WORDS_SECTOR ) && count; count-- ) {
			if ( sc_word( addr, rl_buffer[t] ) != OK ) {
				rl.csr |= RL_NXM;
				return -1;
			}
			addr += 2;
			t++;
		}
	}
	return 0;
}

/*
 * rl_do_write() - Perform the DMA then do the write to
 * the disk.
 */

int
rl_do_write()
{
	unsigned sector;
	unsigned offset;
	c_addr addr;
	unsigned count;
	unsigned t;

	/*
	 * Check to see if on proper cylinder.  We let the user
	 * slide by not checking for the right head.
	 */

	if ( rl.info[rl.drive].cylinder != ( rl.dar >> 7 )) {
		rl.csr |= RL_HNF;
		return -1;
	}

	/*
	 * Calculate head and first sector, also the core
	 * address and word count.
	 */

	rl.info[rl.drive].head = ( rl.dar >> 6 ) & 1;
	sector = rl.dar & 077;
	addr = rl.bar;
	addr += ((c_addr)(rl.csr & 060)) << 12;
	count = ( 0177777 - rl.mpr ) + 1;

	/*
	 * Make sure sector is in range.
	 */

	if ( sector >= RL_NUM_SECT ) {
		rl.csr |= RL_HNF;
		return -1;
	}
	
	offset = (( rl.info[rl.drive].cylinder * RL_BYTES_CYL ) +
		  ( rl.info[rl.drive].head * RL_BYTES_TRACK ) +
		  ( sector * RL_BYTES_SECTOR ));

	if ( fseek( rl.info[rl.drive].file, (long) offset, 0 ) != 0 ) {
		rl.csr |= RL_HNF;
		return -1;
	}

	while ( count ) {
		for( t = 0; ( t < RL_WORDS_SECTOR ) && count; count-- ) {
			if ( lc_word( addr, &rl_buffer[t] ) != OK ) {
				rl.csr |= RL_NXM;
				return -1;
			}
			addr += 2;
			t++;
		}


		if ( fwrite((char *) rl_buffer, RL_BYTES_SECTOR, 1,
		 rl.info[rl.drive].file ) == 0 ) {
			rl.csr |= RL_DCRC;
			return -1;
		}
	}
	fflush( rl.info[rl.drive].file );
	return 0;
}
