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
 * rtc.c - Fake Real Time Clock
 */


#include "defines.h"


char rtcbuffer[26];


/*
 * rtc_read() -
 */

int
rtc_read( addr, word )
c_addr addr;
d_word *word;
{
	struct timeval walltime;
	struct timezone tz;
	unsigned offset;

	if ( addr == RTC ) {
		gettimeofday( &walltime, &tz );
		strcpy( rtcbuffer, ctime( &walltime.tv_sec ));
	}
	offset = addr - RTC;
	*word = rtcbuffer[ offset + 1 ] << 8;
	*word += rtcbuffer[ offset ] ;

	return OK;
}


/*
 * rtc_write() -
 */

/*ARGSUSED*/
int
rtc_write( addr, word )
c_addr addr;
d_word word;
{
	return OK;
}


/*
 * rtc_bwrite() -
 */

/*ARGSUSED*/
int
rtc_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	return OK;
}
