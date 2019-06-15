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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


#define RL_CYL_RL01	256
#define RL_CYL_RL02	512
#define RL_NUM_SECT	40
#define RL_NUM_HEADS	2
#define RL_WORDS_SECTOR	128
#define RL_BYTES_SECTOR	(RL_WORDS_SECTOR * 2)
#define RL_SECT_RL01	(RL_CYL_RL01 * RL_NUM_HEADS * RL_NUM_SECT)
#define RL_SECT_RL02	(RL_CYL_RL02 * RL_NUM_HEADS * RL_NUM_SECT)

#define DEFAULT_NAME	"./DRIVE.0"


main( argc, argv )
int argc;
char *argv[];
{
	unsigned size = RL_SECT_RL01;

	if ( argc == 1 ) {
		make_it( DEFAULT_NAME, size );
	} else {
		while ( --argc ) {
			argv++;
			if ( **argv == '-' ) {
				switch( (*argv)[1] ) {
				case '1':
					size = RL_SECT_RL01;
					break;
				case '2':
					size = RL_SECT_RL02;
					break;
				default:
					fprintf( stderr, "unknown flag -%c\n",
						(*argv)[1] );
					exit( 1 );
					break;
				}
			} else {
				make_it( *argv, size );
			}
		}
	}
}


make_it( file, sectors )
char *file;
unsigned sectors;
{
	char buf[RL_BYTES_SECTOR];
	struct stat sbuf;
	FILE *fp;
	int x;
	
	if ( stat( file, &sbuf ) == 0 ) {
		fprintf( stderr, "file already exists: %s\n", file );
		exit( 1 );
	}
	
	for ( x = 0; x < RL_BYTES_SECTOR; ++x )
		buf[x] = x;
	if (( fp = fopen( file, "w" )) == NULL ) {
		fprintf( stderr, "can't open: %s\n", file );
		exit( 1 );
	}
	for ( x = 0; x < sectors; ++x )
		fwrite( buf, RL_BYTES_SECTOR, 1, fp );
	fclose( fp );
}
