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
 * pdp.c - Simulate the PDP registers that can be accessed via the
 * Unibus.
 */


#include "defines.h"


/*
 * pdp_init() -
 */

int
pdp_init()
{
	pdp.mmr0 = 0;
	pdp.mmr2 = 0;
	flush_atc();
	ev_init();	/* flush pending interrupts on processor reset */
}


/*
 * pdp_read() -
 */

int
pdp_read( addr, word )
c_addr addr;
d_word *word;
{
	unsigned j;

	switch( addr ) {
	case PDP_SR:
		*word = pdp.sr;
		break;
	case PDP_PS:
		*word = pdp.psw;
		break;
	case MMU_MMR0:
		*word = pdp.mmr0;
		break;
	case MMU_MMR2:
		*word = pdp.mmr2;
		break;
	case MMU_UISD:
	case MMU_UISD + 2:
	case MMU_UISD + 4:
	case MMU_UISD + 6:
	case MMU_UISD + 010:
	case MMU_UISD + 012:
	case MMU_UISD + 014:
	case MMU_UISD + 016:
		j = ( addr & 016 ) >> 1;
		*word = pdp.uisd[j];
		break;
	case MMU_UISA:
	case MMU_UISA + 2:
	case MMU_UISA + 4:
	case MMU_UISA + 6:
	case MMU_UISA + 010:
	case MMU_UISA + 012:
	case MMU_UISA + 014:
	case MMU_UISA + 016:
		j = ( addr & 016 ) >> 1;
		*word = pdp.uisa[j];
		break;
	case MMU_KISD:
	case MMU_KISD + 2:
	case MMU_KISD + 4:
	case MMU_KISD + 6:
	case MMU_KISD + 010:
	case MMU_KISD + 012:
	case MMU_KISD + 014:
	case MMU_KISD + 016:
		j = ( addr & 016 ) >> 1;
		*word = pdp.kisd[j];
		break;
	case MMU_KISA:
	case MMU_KISA + 2:
	case MMU_KISA + 4:
	case MMU_KISA + 6:
	case MMU_KISA + 010:
	case MMU_KISA + 012:
	case MMU_KISA + 014:
	case MMU_KISA + 016:
		j = ( addr & 016 ) >> 1;
		*word = pdp.kisa[j];
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * pdp_write() -
 */

int
pdp_write( addr, word )
c_addr addr;
d_word word;
{
	unsigned j;

	switch( addr ) {
	case PDP_SR:
		pdp.sr = word;
		break;
	case PDP_PS:
		pdp.psw = (word & ~020);	 /* no t-bit */
		break;
	case MMU_MMR0:
		pdp.mmr0 &= ~(0160157);		/* mask r/o bits */
		pdp.mmr0 |= (word & 0160157);	/* writeable bits only */
		flush_atc();
		break;
	case MMU_MMR2:
		/* doesn't accept writes */
		break;
	case MMU_UISD:
	case MMU_UISD + 2:
	case MMU_UISD + 4:
	case MMU_UISD + 6:
	case MMU_UISD + 010:
	case MMU_UISD + 012:
	case MMU_UISD + 014:
	case MMU_UISD + 016:
		j = ( addr & 016 ) >> 1;
		pdp.uisd[j] &= ~(077516);	/* mask r/o, and clear w-bit */
		pdp.uisd[j] |= (word & 077516);	/* writeable bits only */
		flush_atc_page( 3, j );
		break;
	case MMU_UISA:
	case MMU_UISA + 2:
	case MMU_UISA + 4:
	case MMU_UISA + 6:
	case MMU_UISA + 010:
	case MMU_UISA + 012:
	case MMU_UISA + 014:
	case MMU_UISA + 016:
		j = ( addr & 016 ) >> 1;
		pdp.uisd[j] &= ~(0100);		/* clear w-bit for segment */
		pdp.uisa[j] &= ~(07777);	/* mask r/o */
		pdp.uisa[j] |= (word & 07777);	/* writeable bits only */
		flush_atc_page( 3, j );
		break;
	case MMU_KISD:
	case MMU_KISD + 2:
	case MMU_KISD + 4:
	case MMU_KISD + 6:
	case MMU_KISD + 010:
	case MMU_KISD + 012:
	case MMU_KISD + 014:
	case MMU_KISD + 016:
		j = ( addr & 016 ) >> 1;
		pdp.kisd[j] &= ~(077516);	/* mask r/o, and clear w-bit */
		pdp.kisd[j] |= (word & 077516);	/* writeable bits only */
		flush_atc_page( 0, j );
		break;
	case MMU_KISA:
	case MMU_KISA + 2:
	case MMU_KISA + 4:
	case MMU_KISA + 6:
	case MMU_KISA + 010:
	case MMU_KISA + 012:
	case MMU_KISA + 014:
	case MMU_KISA + 016:
		j = ( addr & 016 ) >> 1;
		pdp.kisd[j] &= ~(0100);		/* clear w-bit for segment */
		pdp.kisa[j] &= ~(07777);	/* mask r/o */
		pdp.kisa[j] |= (word & 07777);	/* writeable bits only */
		flush_atc_page( 0, j );
		break;
	default:
		return BUS_ERROR;
		/*NOTREACHED*/
		break;
	}
	return OK;
}


/*
 * pdp_bwrite() -
 */

/*ARGSUSED*/
int
pdp_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	int result;
	d_word word;
	d_word word2;

	/*
	 * sleeze this.... look, some of it isn't even in octal!!!
	 */

	if ((result = pdp_read( addr & 0777776, &word )) != OK )
		return result;

	word2 = byte;
	if ( addr & 1 ) {
		word2 <<= 8;
		word = (word & 0x00ff) + word2;
	} else {
		word = (word & 0xff00) + word2;
	}

	return pdp_write( addr & 0777776, word );
}
