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
 * ui.c - Simulator user interface.
 */


#include "defines.h"


#define DWIDTH	4
#define EMPTY	-1
#define FALSE	0
#define TRUE	1

#define isoct(c)	(((c) >= '0') && ((c) <= '7'))


int ui_done;

static char buf[BUFSIZ];


/*
 * ui() - The user interface main loop.
 */

ui()
{
	char *s;

	ui_done = 0;
	do {
		putchar( '%' );
		putchar( ' ' );
		fflush( stdout );
		s = gets( buf );
		if ( s ) {
			while( isspace( *s ))
				++s;
			switch( *s++ ) {
			case 'd':	/* dump memory */
				ui_dump( s );
				break;
			case 'e':	/* edit memory */
				ui_edit( s );
				break;
			case 'g':	/* go start execution */
				ui_start( s, 1 );
				break;
			case 'r':	/* register dump */
				ui_registers();
				break;
			case 's':	/* go run one intruction */
				ui_start( s, 0 );
				break;
			default:	/* invalid command */
				huh();
				break;
			}
		} else {
			ui_done = 1;	/* NULL means done */
		}
	} while( !ui_done );
}


/*
 * rd_c_addr() - Read an 18-bit core address from
 * the character buffer.
 */

char*
rd_c_addr( s, v, good )
char *s;
c_addr *v;
int *good;
{
	while( isspace( *s ))
		++s;
	*v = 0;
	*good = EMPTY;
	while(( *s != ' ' ) && ( *s != '\0' )) {
		if ( *good == EMPTY )
			*good = 0;
		if ( isoct( *s )) {
			*v = ( *v << 3 ) + ( *s - '0' );
			(*good)++;
		} else {
			*good = FALSE;
			break;
		}
		++s;
	}
	*v &= 0777777;		/* mask off to only 18 bits */
	return s;
}


/*
 * rd_d_word() - Read a 16-bit data word from
 * the character buffer.
 */

char*
rd_d_word( s, v, good )
char *s;
d_word *v;
int *good;
{
	while( isspace( *s ))
		++s;
	*v = 0;
	*good = EMPTY;
	while(( *s != ' ' ) && ( *s != '\0' )) {
		if ( *good == EMPTY )
			*good = 0;
		if ( isoct( *s )) {
			*v = ( *v << 3 ) + ( *s - '0' );
			(*good)++;
		} else {
			*good = FALSE;
			break;
		}
		++s;
	}
	return s;
}


/*
 * ui_dump() - Hex dump of memory.
 */

ui_dump( s )
char *s;
{
	c_addr addr;
	c_addr new;
	d_word word;
	static c_addr last = 0;
	int count = 0;
	int good;

	s = rd_c_addr( s, &new, &good );
	if ( good == FALSE ) {
		huh();
		return;
	}
	if ( good != EMPTY ) {
		addr = new;
		s = rd_c_addr( s, &new, &good );
		if ( good == FALSE ) {
			huh();
			return;
		}
		if ( good != EMPTY ) {
			last = new;
		} else {
			last = addr + (DWIDTH * 8);
		}
	} else {
		addr = last;
		last += (DWIDTH * 8);
	}
	
	addr &= 0777777;
	last &= 0777777;

	for( ; addr != last; addr += 2 ) {
		addr &= 0777777;
		if (( count % DWIDTH ) == 0 ) {
			printf( "%06o: ", addr );
		}
		if ( lc_word( addr, &word ) == OK ) {
			printf( "%06o ", word );
		} else {
			printf( "XXXXXX " );
		}
		if (( count % DWIDTH ) == ( DWIDTH - 1 )) {
			putchar( '\n' );
		}
		++count;
	}
}


/*
 * ui_edit() - Edit memory.
 */

ui_edit( s )
char *s;
{
	static c_addr addr;
	c_addr new;
	d_word word;
	int good;
	int done = 0;
	char *t;

	s = rd_c_addr( s, &new, &good );
	if ( good == FALSE ) {
		huh();
		return;
	} else if ( good != EMPTY ) {
		addr = new;
	}

	do {
		addr &= 0777777;
		printf( "%06o=", addr );
		if ( lc_word( addr, &word ) == OK ) {
			printf( "%06o ", word );
		} else {
			printf( "XXXXXX " );
		}
		fflush( stdout );
		t = gets( buf );
		if ( t == NULL ) {
			done = 1;	/* NULL means done */
			ui_done = 1;
		} else {
			switch( *t ) {
			case '+':	/* next addr */
			case '\0':
				addr += 2;
				break;
			case '.':	/* exit edit */
				done = 1;
				break;
			case '-':	/* previous addr */
				addr -= 2;
				break;
			case '0':	/* write data */
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				rd_d_word( t, &word, &good );
				if ( good == FALSE ) {
					huh();
				} else {
					if( sc_word( addr, word ) != OK ) {
						printf( "write error\n" );
					}
				}
				addr += 2;
				break;
			default:	/* invalid   command */
				huh();
				break;
			}
		}
	} while ( !done );
}


/*
 * huh() - Print the command garbled message.
 */

huh()
{
	puts( "can\'t grok" );
}


/*
 * ui_start() - Start execution or single stepping.
 */

ui_start( s, n )
char *s;
int n;
{
	d_word word;
	int good;

	s = rd_d_word( s, &word, &good );
	if ( good == FALSE ) {
		huh();
		return;
	}

	if ( good != EMPTY )
		pdp.regs[PC] = word;

	run( n );
}


/*
 * ui_registers() - Do a simple register dump.
 */

ui_registers()
{
	printf( "R0-R4: %06o %06o %06o %06o\n",
		pdp.regs[0], pdp.regs[1], pdp.regs[2], pdp.regs[3] );
	printf( "R5-R7: %06o %06o %06o %06o\n",
		pdp.regs[4], pdp.regs[5], pdp.regs[6], pdp.regs[7] );
	printf( "PSW: %06o [", pdp.psw );
	if ( pdp.psw & 010 ) putchar( 'N' );
	if ( pdp.psw & 04 ) putchar( 'Z' );
	if ( pdp.psw & 02 ) putchar( 'V' );
	if ( pdp.psw & 01 ) putchar( 'C' );
	printf( "]\n" );
}
