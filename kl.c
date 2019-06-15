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
 * kl.c - KL11 device simulator.
 */


#include "defines.h"

/*
 * Definitions for the KL11 simulation.
 *
 * The very short KL_DELAY makes console i/o more responsive on slow
 * host machines, but it may hammer the simulated code. Be warned.
 */

#define KL_DELAY	250	/* in microseconds, about 40 Kbps... */
#define KL_PRI		4	/* BR 4 */
#define KL_CON_RVECTOR	060	/* receiver vector for console */
#define KL_CON_TVECTOR	064	/* transmitter vector for console */
#define KL_TTY_RBASE	0300	/* receiver vector base for #1-#16 */
#define KL_TTY_TBASE	0304	/* transmitter vector base for #1-#16 */

/*
 * Defines for the registers.
 */

#define KL_IE		0100
#define	KL_DONE		0200

struct kl_regs {
	d_word rsr;		/* controller register images */
	d_word rdr;
	d_word tsr;
	d_word tdr;
	int rxfd;		/* receive and transmit file desc. */
	int txfd;
};

/*
 * The KL11 controller information and information about
 * the tty ports.
 */

struct kl_regs kl[NUM_KL];		/* register images */
fd_set kl_select_fds;			/* descriptors to poll */
struct sgttyb conoldstty, conrunstty;	/* console terminal i/o stuff */

/*
 *
 */

kl_open()
{
        struct sgttyb runstty;
	int i;

	FD_ZERO( &kl_select_fds );

	/*
	 * open up the console
	 */

	kl[0].rxfd = open( kl_gpath[0], O_RDWR );
	kl[0].txfd = kl[0].rxfd;
	dup2( kl[0].rxfd, 0 );
	dup2( kl[0].rxfd, 1 );
	FD_SET( kl[0].rxfd, &kl_select_fds );

	/*
	 * now the rest of the ports
	 */

	for ( i = 1; i < NUM_KL; ++i ) {
		kl[i].rxfd = -1;
		kl[i].txfd = -1;
		if ( strlen( kl_gpath[i] )) {
			kl[i].rxfd = open( kl_gpath[i], O_RDWR );
        		gtty( kl[i].rxfd, &runstty );
        		runstty.sg_flags |= CBREAK;
        		runstty.sg_flags &= ~ECHO;
        		stty( kl[i].rxfd, &runstty );
		}
		kl[i].txfd = kl[i].rxfd;
		if ( kl[i].rxfd != -1 ) {
			FD_SET( kl[i].rxfd, &kl_select_fds );
		}
	}
}

kl_con_grab()
{
	gtty( kl[0].rxfd, &conoldstty );
	gtty( kl[0].rxfd, &conrunstty );
	conrunstty.sg_flags |= CBREAK;
	conrunstty.sg_flags &= ~ECHO;
	stty( kl[0].rxfd, &conrunstty );
}

kl_con_release()
{
	stty( kl[0].rxfd, &conoldstty );
}

/*
 * kl_init() - Initialize the KL11 controller.
 */

int
kl_init()
{
	int i;

	for ( i = 0; i < NUM_KL; ++i ) {
		kl[i].rsr = 0;
		kl[i].rdr = 0177;
		kl[i].tsr = KL_DONE;
		kl[i].tdr = 0;
	}
}

/*
 * kl_read() - Handle the reading of an KL11 register.
 */

int
kl_read( addr, word )
c_addr addr;
d_word *word;
{
	struct kl_regs *k;
	d_word offset = addr & 07;

	if (( addr & 0777770 ) == KL11_CON )
		k = kl;
	else {
		unsigned n;

		n = (( addr & 070 ) >> 3 ) + 1;
		if (( n < 1 ) || (n >= NUM_KL ))
			return BUS_ERROR;
		k = &kl[n];
	}

	switch( offset ) {
	case 0:
		*word = k->rsr;
		kl_recv();
		break;
	case 2:
		*word = k->rdr;
		if ( k->rsr & KL_DONE )
			k->rsr &= ~KL_DONE;
		kl_recv();
		break;
	case 4:
		*word = k->tsr;
		break;
	case 6:
		*word = k->tdr;
		break;
	}
	return OK;
}

/*
 * kl_write() - Handle a write to one of the KL11
 * registers.
 */

int
kl_write( addr, word )
c_addr addr;
d_word word;
{
	int kl_finish();
	struct kl_regs *k;
	d_word offset = addr & 07;
	d_word vector;
	char c;

	if (( addr & 0777770 ) == KL11_CON ) {
		k = kl;
		vector = KL_CON_TVECTOR;
	} else {
		unsigned n;

		n = (( addr & 070 ) >> 3 ) + 1;
		if (( n < 1 ) || (n >= NUM_KL ))
			return BUS_ERROR;
		k = &kl[n];
		vector = KL_TTY_TBASE + ((n-1) * 010 );
	}

	switch( offset ) {
	case 0:
		k->rsr |= word & KL_IE;	/* only let them set IE */
		break;
	case 2:
		k->rdr = word;		/* clear done bit if set */
		k->rsr &= ~KL_DONE;
		break;
	case 4:
		if ((!( k->tsr & KL_IE )) && ( word & KL_IE ))
			ev_register( KL_PRI, kl_finish,
			 (unsigned long) KL_DELAY, vector );
		k->tsr |= word & KL_IE;	/* only let them set IE */
		break;
	case 6:
		k->tdr = word;
		c = word & 0177;
		if ( k->txfd != -1 )
			write( k->txfd, &c, 1 );
		if (  k->tsr & KL_IE ) {
			ev_register( KL_PRI, kl_finish,
				(unsigned long) KL_DELAY, vector );
		}
		break;
	}
	return OK;
}

/*
 * kl_bwrite() - Handle a byte write by doing a word write.
 */

int
kl_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	d_word word = byte;

	return kl_write( addr, word );
}

/*
 * kl_recv() - Called at various times during simulation to
 * set if the user typed somthing.  We use select, huhhuhh...
 */

kl_recv()
{
	struct kl_regs *k = kl;
	char c;
	struct timeval timeout;
	fd_set new_set;
	int num_ready, i = 0;
	int kl_finish();

	new_set = kl_select_fds;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	num_ready = select( 20, &new_set, (fd_set *)0, (fd_set *)0, &timeout );
	while ( num_ready && ( i < NUM_KL )) {
		if ( FD_ISSET( k->rxfd, &new_set )) {
			--num_ready;
			if ( ! (k->rsr & KL_DONE )) {
				read( k->rxfd, &c, 1 );
				k->rsr |= KL_DONE;
				k->rdr = c & 0177;
				if ( k->rsr & KL_IE ) {
					ev_register( KL_PRI, kl_finish,
					 (unsigned long) 0,
					 (( i == 0 ) ? KL_CON_RVECTOR :
					 KL_TTY_RBASE + ((i-1) * 010 )));
				}
			}
		}
		++i;
		++k;
	}
}

/*
 * kl_finish()
 */

kl_finish( vector )
d_word vector;
{
	service( vector );
}
