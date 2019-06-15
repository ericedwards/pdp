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
 * wd.c - WD device simulator.
 */


#include "defines.h"


/*
 * WD Stuff.
 */

#define WD_DELAY	500	/* 5us */
#define WD_PRI		7	/* level 7, but doesn't really interrupt */
#define WD_DRIVES	2

int wd_finish();

#define WD_BUSY		0x80
#define WD_READY	0x40
#define WD_WRTFLT	0x20
#define WD_SEEKC	0x10
#define WD_DRQ		0x08
#define WD_ECC		0x04
#define WD_INDEX	0x02
#define WD_ERR		0x01

#define WD_READ		0x20
#define WD_WRITE	0x30
#define WD_RESTORE	0x10
#define WD_SETCHAR	0x91

#define WD_STEPR	0
#define WD_DSD		0xa0

struct wd_regs {
	d_word data;
	d_word error;
	d_word precomp;
	d_word seccnt;
	d_word sector;
	d_word cyl_lo;
	d_word cyl_hi;
	d_word sdh;
	d_word command;
	d_word status;
	d_word control;
	unsigned dbufptr;
	unsigned state;
	unsigned drive;
	long offset;
	struct {
		FILE *fp;
		unsigned sectors;
		unsigned heads;
		unsigned cylinders;
	} info[WD_DRIVES];
} wd;

d_word wd_dbuf[256];

#define WDSEC	17
#define WDHEAD	4
#define WDCYL	306
#define WDSIZE	20808

#define S_IDLE		0
#define S_READING	1
#define S_WRITING	2


/*
 * wd_init() - Initialize the WD controller.
 */

int
wd_init()
{
	if ( wd.info[0].fp != NULL )
		fclose( wd.info[0].fp );
	wd.info[0].fp = fopen( "./WD.0","r+" );
	wd.info[1].fp = NULL;
	wd.drive = 0;
	wd.offset = 0;
	wd.dbufptr = 0;
	wd.state = S_IDLE;
	wd.status = 0;
	wd.error = 0;
}


/*
 * wd_read() - Handle the reading of an WD register.
 */

int
wd_read( addr, word )
c_addr addr;
d_word *word;
{
	switch( addr ) {
	case WD:
		*word = wd_dbuf[wd.dbufptr++];
		if ( wd.dbufptr == 256 ) {
			wd.dbufptr = 0;
			if ( wd.state == S_READING ) {
				wd.status &= ~WD_DRQ;
				wd.status |= WD_BUSY;
				ev_register( WD_PRI, wd_finish,
				 WD_DELAY, (d_word) 0 );
			}
		}
		break;
	case WD+2:
		*word = wd.error;
		break;
	case WD+4:
		*word = wd.seccnt;
		break;
	case WD+6:
		*word = wd.sector;
		break;
	case WD+8:
		*word = wd.cyl_lo;
		break;
	case WD+10:
		*word = wd.cyl_hi;
		break;
	case WD+12:
		*word = wd.sdh;
		break;
	case WD+14:
		*word = wd.status;
		break;
	case WD+16:
		*word = wd.status;
		break;
	case WD+18:
		*word = 0;
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * wd_write() - Handle a write to one of the WD registers.
 */

int
wd_write( addr, word )
c_addr addr;
d_word word;
{

	word &= 0xff;	/* low byte only !!! */

	switch( addr ) {
	case WD:
		wd_dbuf[wd.dbufptr++] = word;
		if ( wd.dbufptr == 256 ) {
			wd.dbufptr = 0;
			if ( wd.state == S_WRITING ) {
				wd.status &= ~WD_DRQ;
				wd.status |= WD_BUSY;
				ev_register( WD_PRI, wd_finish,
				 WD_DELAY, (d_word) 0 );
			}
		}
		break;
	case WD+2:
		wd.precomp = word;
		break;
	case WD+4:
		wd.seccnt = word;
		break;
	case WD+6:
		wd.sector = word;
		if ( wd.sector == 0 )	/* force zero to 1 */
			wd.sector = 1;
		break;
	case WD+8:
		wd.cyl_lo = word;
		break;
	case WD+10:
		wd.cyl_hi = word;
		break;
	case WD+12:
		wd.sdh = word;
		wd.drive = (wd.sdh >> 4) & 1;
		if ( wd.info[wd.drive].fp != NULL ) {
			wd.status |= WD_READY;
		} else {
			wd.status &= ~WD_READY;
		}
		break;
	case WD+14:
		wd.command = word;
		wd_exec();
		break;
	case WD+16:
		wd.control = word;
		break;
	case WD+18:
		/* don't do anything */
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * wd_bwrite() - Handle a byte write by doing a word write.
 */

int
wd_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	d_word word = byte;

	if ( addr & 1 )
		return OK;
	else
		return wd_write( addr, word );
}


/*
 * wd_exec()
 */

wd_exec()
{
	wd.state = S_IDLE;
	wd.status &= ~(WD_BUSY|WD_ERR);

	switch( wd.command & 0xff ) {
	case WD_READ:
		wd.state = S_READING;
		wd.status |= WD_BUSY;
		wd.status &= ~WD_DRQ;
		ev_register( WD_PRI, wd_finish, WD_DELAY, (d_word) 0 );
		/* we'll be ready in a bit */
		break;
	case WD_WRITE:
		wd.status |= WD_DRQ;
		wd.status &= ~WD_BUSY;
		wd.state = S_WRITING;
		/* filling the buffer will kick us off */
		break;
	case WD_SETCHAR:
		wd.info[wd.drive].sectors = wd.sector;
		wd.info[wd.drive].heads = ( wd.sdh & 0xf ) + 1;
		wd.info[wd.drive].cylinders = wd.cyl_lo;
		wd.info[wd.drive].cylinders += ( wd.cyl_hi << 8 );
		/* return quickly */
		break;
	default:
		/* nothing for now */
		break;
	}
}


/*
 * wd_finish()
 */

wd_finish( info )
d_word info;
{
	switch( wd.state ) {
	case S_READING:
		if (wd_seek()) {
			wd.status |= WD_ERR;
		}
		if ( fread((char *) wd_dbuf, 512, 1,
                 wd.info[wd.drive].fp ) == 0 ) {
			wd.status |= WD_ERR;
		}
		wd.dbufptr = 0;
		wd_update();
		wd.status |= WD_DRQ;
		wd.status &= ~WD_BUSY;
		if ( --wd.seccnt ) {
			wd.state = S_READING;
		} else {
			wd.state = S_IDLE;
		}
		break;
	case S_WRITING:
		if (wd_seek()) {
			wd.status |= WD_ERR;
		}
		if ( fwrite((char *) wd_dbuf, 512, 1,
                 wd.info[wd.drive].fp ) == 0 ) {
			wd.status |= WD_ERR;
		}
		fflush( wd.info[wd.drive].fp );
		wd.dbufptr = 0;
		wd_update();
		if ( --wd.seccnt ) {
			wd.status |= WD_DRQ;
			wd.status &= ~WD_BUSY;
			wd.state = S_WRITING;
		} else {
			wd.status &= ~(WD_DRQ|WD_BUSY);
			wd.state = S_IDLE;
		}
		break;
	case S_IDLE:
		/* a stray? */
		break;
	}
}


wd_update()
{
	unsigned cylinder;
	unsigned head;
	if ( ++wd.sector > wd.info[wd.drive].sectors ) {
		wd.sector = 1;
		head = wd.sdh & 0xf;
		if ( ++head >= wd.info[wd.drive].heads ) {
			wd.sdh &= ~0xf;
			cylinder = ( wd.cyl_hi << 8 ) + wd.cyl_lo;
			cylinder++;
			wd.cyl_lo = cylinder & 0xff;
			wd.cyl_hi = ( cylinder >> 8 ) & 0xff;
		} else {
			wd.sdh &= ~0xf;
			wd.sdh |= ( head & 0xf );
		}
	}
}

wd_seek()
{
	if (( wd.sector < 1 ) || ( wd.sector > wd.info[wd.drive].sectors ))
		return -1;
	if (( wd.sdh & 0xf ) >= wd.info[wd.drive].heads )
		return -1;
	if ((( wd.cyl_hi << 8 ) + wd.cyl_lo ) >=  wd.info[wd.drive].cylinders )
		return -1;
	wd.offset = ( wd.cyl_hi << 8 ) + wd.cyl_lo; 
	wd.offset *= ( wd.sdh & 0xf );
	wd.offset *= ( wd.sector - 1 );
	wd.offset *= 512;
	return fseek( wd.info[wd.drive].fp, wd.offset, 0 );
}
