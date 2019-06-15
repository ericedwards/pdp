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
 * access.c - Access routines to read and write loactions on the UNIBUS
 * and main memory.  
 */


#include "defines.h"


/*
 * The PDP main memory.
 */

d_word mem[PDP_MEM_SIZE];


/*
 * The address translation cache.
 */

typedef union {
	long	valid;
	c_addr	offset;
} atc_entry;

atc_entry atc[4][1024];


/*
 * The UNIBUS memory map.
 */

struct pdp_ubmap {
	c_addr start;
	c_addr size;
	int (*ifunc)();
	int (*rfunc)();
	int (*wfunc)();
	int (*bwfunc)();
} ubmap[MMAP_SIZE] = {
	{ PDP_PS, PDP_PS_SIZE, pdp_init, pdp_read, pdp_write, pdp_bwrite },
	{ PDP_SR, PDP_SR_SIZE, ub_null, pdp_read, pdp_write, pdp_bwrite },
	{ MMU_UISD, MMU_UISD_SIZE, ub_null, pdp_read, pdp_write, pdp_bwrite },
	{ MMU_UISA, MMU_UISA_SIZE, ub_null, pdp_read, pdp_write, pdp_bwrite },
	{ MMU_KISD, MMU_KISD_SIZE, ub_null, pdp_read, pdp_write, pdp_bwrite },
	{ MMU_KISA, MMU_KISA_SIZE, ub_null, pdp_read, pdp_write, pdp_bwrite },
	{ MMU_MMR0, MMU_MMR0_SIZE, ub_null, pdp_read, pdp_write, pdp_bwrite },
	{ MMU_MMR2, MMU_MMR2_SIZE, ub_null, pdp_read, pdp_write, pdp_bwrite },
	{ KL11_CON, KL11_CON_SIZE, kl_init, kl_read, kl_write, kl_bwrite },
	{ KW11, KW11_SIZE, kw_init, kw_read, kw_write, kw_bwrite },
	{ RL11, RL11_SIZE, rl_init, rl_read, rl_write, rl_bwrite },
	{ KL11_TTY, KL11_TTY_SIZE, kl_init, kl_read, kl_write, kl_bwrite },
	{ TM11, TM11_SIZE, tm_init, tm_read, tm_write, tm_bwrite },
	{ LP11, LP11_SIZE, lp_init, lp_read, lp_write, lp_bwrite },
	{ BOOT, BOOT_SIZE, ub_null, boot_read, boot_write, boot_bwrite },
	{ RTC, RTC_SIZE, ub_null, rtc_read, rtc_write, rtc_bwrite },
	{ WD, WD_SIZE, wd_init, wd_read, wd_write, wd_bwrite },
	{ 0, 0, 0, 0, 0, 0 }
};


/*
 * lc_word() - Load a word from the given core address.
 */

int
lc_word( addr, word )
c_addr addr;
d_word *word;
{
	int i;

	if ( addr & 1 ) {
		return ODD_ADDRESS;
	}

	if ( addr < (PDP_MEM_SIZE * 2)) {
		*word = mem[addr >> 1];
		return OK;
	}

	for ( i = 0; ubmap[i].size; ++i ) {
		if (( addr >= ubmap[i].start ) &&
		 ( addr < ( ubmap[i].start + (ubmap[i].size *2 )))) {
			return (ubmap[i].rfunc)( addr, word );
		}
	}
	return BUS_ERROR;
}


/*
 * sc_word() - Store a word at the given core address.
 */

int
sc_word( addr, word )
c_addr addr;
d_word word;
{
	int i;

	if ( addr & 1 ) {
		return ODD_ADDRESS;
	}

	if ( addr < (PDP_MEM_SIZE * 2)) {
		mem[addr >> 1] = word;
		return OK;
	}

	for ( i = 0; ubmap[i].size; ++i ) {
		if (( addr >= ubmap[i].start ) &&
		 ( addr < ( ubmap[i].start + ( ubmap[i].size * 2 )))) {
			return (ubmap[i].wfunc)( addr, word );
		}
	}
	return BUS_ERROR;
}


/*
 * ll_word() - Load a word from the given logical address.
 */

int
ll_word( p, laddr, word )
register pdp_regs *p;
d_word laddr;
d_word *word;
{
	register c_addr caddr;
	register d_word desc;
	unsigned index;
	unsigned block;
	d_word addr;
	d_word mode;
	int i;
	atc_entry *a;

	a = &atc[ p->psw >> 14 ][ laddr >> 6 ];

	if ( a->valid >= 0 ) {
		caddr = a->offset + ( laddr & 017777 );
	} else if (( p->mmr0 & 1 ) == 0 ) {
		if ( laddr >= 0160000 ) {
			caddr = 0760000 + ( laddr & 017777 );
		} else {
			caddr = laddr;
		}
	} else {

		index = ( laddr >> 13 ) & 7;
		block = ( laddr >> 6 ) & 0177;

		if ( KERNEL( p->psw )) {
			addr = p->kisa[index];
			desc = p->kisd[index];
			mode = 0;
		} else {
			addr = p->uisa[index];
			desc = p->uisd[index];
			mode = 0140;
		}

		if (( desc & 2 ) == 0 ) {
			p->mmr0 |= 0100000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}

		if ( desc & 010 ) {	/* downward expanding */
			if ( block < ((desc >> 8) & 0177)) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		} else {		/* upward expanding */
			if ( block > ((desc >> 8) & 0177)) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		}

		caddr = addr;
		caddr <<= 6;
		a->offset = caddr;
		caddr += ( laddr & 017777 );
	}

	if ( caddr & 1 ) {
		return ODD_ADDRESS;
	}

	if ( caddr < (PDP_MEM_SIZE * 2)) {
		*word = mem[caddr >> 1];
		return OK;
	}

	for ( i = 0; ubmap[i].size; ++i ) {
		if (( caddr >= ubmap[i].start ) &&
		 ( caddr < ( ubmap[i].start + (ubmap[i].size *2 )))) {
			return (ubmap[i].rfunc)( caddr, word );
		}
	}

	return BUS_ERROR;
}


/*
 * sl_word() - Store a word at the given logical address.
 */

int
sl_word( p, laddr, word )
register pdp_regs *p;
d_word laddr;
d_word word;
{
	register c_addr caddr;
	register d_word desc;
	atc_entry *a;
	unsigned short plf;
	d_word *desc_p;
	unsigned index;
	unsigned block;
	d_word addr;
	d_word mode;
	int i;

	a = &atc[ p->psw >> 14 ][ laddr >> 6 ];
	if ( a->valid >= 0 ) {
		caddr = a->offset + ( laddr & 017777 );
		index = ( laddr >> 13 ) & 7;
		if ( KERNEL( p->psw )) {
			desc_p = &p->kisd[index];
		} else {
			desc_p = &p->uisd[index];
		}
		*desc_p |= 0100;
	} else if (( p->mmr0 & 1 ) == 0 ) {
		if ( laddr >= 0160000 ) {
			caddr = 0760000 + ( laddr & 017777 );
		} else {
			caddr = laddr;
		}
	} else {

		index = ( laddr >> 13 ) & 7;
		block = ( laddr >> 6 ) & 0177;

		if ( KERNEL( p->psw )) {
			addr = p->kisa[index];
			desc_p = &p->kisd[index];
			mode = 0;
		} else {
			addr = p->uisa[index];
			desc_p = &p->uisd[index];
			mode = 0140;
		}

		desc = *desc_p;

		if (( desc & 2 ) == 0 ) {
			p->mmr0 |= 0100000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}

		plf = (desc >> 8) & 0177;

		if ( desc & 010 ) {	/* downward expanding */
			if ( block < plf ) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		} else {		/* upward expanding */
			if ( block > plf ) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		}

		if ((desc & 4) == 0 ) {
			p->mmr0 |= 020000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}
		*desc_p |= 0100;

		caddr = addr;
		caddr <<= 6;
		a->offset = caddr;
		caddr += ( laddr & 017777 );
	}

	if ( caddr & 1 ) {
		return ODD_ADDRESS;
	}

	if ( caddr < (PDP_MEM_SIZE * 2)) {
		mem[caddr >> 1] = word;
		return OK;
	}

	for ( i = 0; ubmap[i].size; ++i ) {
		if (( caddr >= ubmap[i].start ) &&
		 ( caddr < ( ubmap[i].start + (ubmap[i].size *2 )))) {
			return (ubmap[i].wfunc)( caddr, word );
		}
	}

	return BUS_ERROR;
}


/*
 * ll_byte() - Load a byte from the given logical address.
 * The PDP can't really do byte reads, so do a word read and
 * get the proper piece.
 */

int
ll_byte( p, baddr, byte )
register pdp_regs *p;
d_word baddr;
d_byte *byte;
{
	d_word word;
	d_word laddr;

	register c_addr caddr;
	register d_word desc;
	atc_entry *a;
	unsigned short plf;
	d_word *desc_p;
	unsigned index;
	unsigned block;
	d_word addr;
	d_word mode;
	int i;

	/*
	 * Get the word address.
	 */

	laddr = baddr & 0177776;

	/*
	 * Address translation.
	 */

	a = &atc[ p->psw >> 14 ][ laddr >> 6 ];

	if ( a->valid >= 0 ) {
		caddr = a->offset + ( laddr & 017777 );
	} else if (( p->mmr0 & 1 ) == 0 ) {
		if ( laddr >= 0160000 ) {
			caddr = 0760000 + ( laddr & 017777 );
		} else {
			caddr = laddr;
		}
	} else {

		index = ( laddr >> 13 ) & 7;
		block = ( laddr >> 6 ) & 0177;

		if ( KERNEL( p->psw )) {
			addr = p->kisa[index];
			desc_p = &p->kisd[index];
			mode = 0;
		} else {
			addr = p->uisa[index];
			desc_p = &p->uisd[index];
			mode = 0140;
		}

		desc = *desc_p;

		if (( desc & 2 ) == 0 ) {
			p->mmr0 |= 0100000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}

		plf = (desc >> 8) & 0177;

		if ( desc & 010 ) {	/* downward expanding */
			if ( block < plf ) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		} else {		/* upward expanding */
			if ( block > plf ) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		}

		caddr = addr;
		caddr <<= 6;
		a->offset = caddr;
		caddr += ( laddr & 017777 );
	}

	if ( caddr < (PDP_MEM_SIZE * 2)) {
		word = mem[caddr >> 1];
	} else {
		for ( i = 0; ubmap[i].size; ++i ) {
			if (( caddr >= ubmap[i].start ) &&
		 	( caddr < ( ubmap[i].start + (ubmap[i].size *2 )))) {
				(ubmap[i].rfunc)( caddr, &word );
				break;
			}
		}
		if ( !ubmap[i].size )
			return BUS_ERROR;
	}

	if ( baddr & 1 ) {
		word = (word >> 8) & 0377;
	} else {
		word = word & 0377;
	}

	*byte = word;

	return OK;
}


/*
 * sl_byte() - Store a byte at the given logical address.
 */

int
sl_byte( p, laddr, byte )
register pdp_regs *p;
d_word laddr;
d_byte byte;
{
	register c_addr caddr;
	register d_word desc;
	atc_entry *a;
	unsigned short plf;
	d_word *desc_p;
	unsigned index;
	unsigned block;
	d_word t, s;
	d_word addr;
	d_word mode;
	int i;

	a = &atc[ p->psw >> 14 ][ laddr >> 6 ];

	if ( a->valid >= 0 ) {
		caddr = a->offset + ( laddr & 017777 );
		index = ( laddr >> 13 ) & 7;
		if ( KERNEL( p->psw )) {
			desc_p = &p->kisd[index];
		} else {
			desc_p = &p->uisd[index];
		}
		*desc_p |= 0100;
	} else if (( p->mmr0 & 1 ) == 0 ) {

		if ( laddr >= 0160000 ) {
			caddr = 0760000 + ( laddr & 017777 );
		} else {
			caddr = laddr;
		}

	} else {

		index = ( laddr >> 13 ) & 7;
		block = ( laddr >> 6 ) & 0177;

		if ( KERNEL( p->psw )) {
			addr = p->kisa[index];
			desc_p = &p->kisd[index];
			mode = 0;
		} else {
			addr = p->uisa[index];
			desc_p = &p->uisd[index];
			mode = 0140;
		}

		desc = *desc_p;

		if (( desc & 2 ) == 0 ) {
			p->mmr0 |= 0100000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}

		plf = (desc >> 8) & 0177;

		if ( desc & 010 ) {	/* downward expanding */
			if ( block < plf ) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		} else {		/* upward expanding */
			if ( block > plf ) {
				p->mmr0 |= 040000;
				p->mmr0 |= mode;
				p->mmr0 |= index << 1;
				return MMU_ERROR;
			}
		}

		if ((desc & 4) == 0 ) {
			p->mmr0 |= 020000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}
		*desc_p |= 0100;

		caddr = addr;
		caddr <<= 6;
		a->offset = caddr;
		caddr += ( laddr & 017777 );
	}

	if ( caddr < (PDP_MEM_SIZE * 2)) {
		t = mem[caddr >> 1];
		if (caddr & 1) {
			t &= 0x00ff;
			s = byte;
			s = s << 8;
		} else {
			t &= 0xff00;
			s = byte;
		}
		t += s;
		mem[caddr >> 1] = t;
		return OK;
	}

	for ( i = 0; ubmap[i].size; ++i ) {
		if (( caddr >= ubmap[i].start ) &&
		 ( caddr < ( ubmap[i].start + ( ubmap[i].size * 2 )))) {
			return (ubmap[i].bwfunc)( caddr, byte );
		}
	}

	return BUS_ERROR;
}


/*
 * llp_word() - Load a word from the given logical address.  Use
 * the previous mode's mmu setup.
 */

int
llp_word( p, addr, word )
register pdp_regs *p;
d_word addr;
d_word *word;
{
	int status;
	c_addr caddr;

	if (( status = map( p, addr, &caddr, MMU_PREVIOUS, 0 )) != OK )
		return status;
	else
		return lc_word( caddr, word );
}


/*
 * slp_word() - Store a word at the given logical address.  Use
 * the previous mode's mmu setup.
 */

int
slp_word( p, addr, word )
register pdp_regs *p;
d_word addr;
d_word word;
{
	int status;
	c_addr caddr;

	if (( status = map( p, addr, &caddr, MMU_PREVIOUS, 1 )) != OK )
		return status;
	else
		return sc_word( caddr, word );
}




/*
 * mem_init() - Initialize the memory.
 */

mem_init()
{
	int x;

	for ( x = 0; x < PDP_MEM_SIZE; ++x ) {
		mem[x] = (d_word) 0xaaaa;	/* alternate bits */
	}
}


/*
 * ub_null() - Null UNIBUS device switch handler.
 */

ub_null()
{
	return OK;
}


/*
 * ub_reset() - Reset the UNIBUS devices.
 */

ub_reset()
{
	int i;

	for ( i = 0; ubmap[i].size; ++i ) {
		(ubmap[i].ifunc)();
	}
}


/*
 * map() - MMU map routine for abnormal accesses.
 */

int
map( p, laddr, caddr, flags, write )
register pdp_regs *p;
d_word laddr;
c_addr *caddr;
unsigned flags;
unsigned write;
{
	unsigned index;
	unsigned block;
	d_word addr;
	d_word mode;
	d_word *desc_p;
	register d_word desc;
	unsigned short plf;

	/*
	 * First, check to see if the mmu is off, if so do
	 * the simple mapping.
	 */

	if (( p->mmr0 & 1 ) == 0 ) {
		if ( laddr >= 0160000 ) {
			*caddr = 0760000 + ( laddr & 017777 );
		} else {
			*caddr = laddr;
		}
		return OK;
	}

	index = ( laddr >> 13 ) & 7;
	block = ( laddr >> 6 ) & 0177;

	switch( flags ) {
	case MMU_NORMAL:
		switch(( p->psw & 0140000 ) >> 14 ) {
		case 0:
			addr = p->kisa[index];
			desc_p = &p->kisd[index];
			mode = 0;
			break;
		case 1:
			return CPU_NOT_IMPL;
			/*NOTREACHED*/
			break;
		case 2:
			return CPU_NOT_IMPL;
			/*NOTREACHED*/
			break;
		case 3:
			addr = p->uisa[index];
			desc_p = &p->uisd[index];
			mode = 0140;
			break;
		}
		break;
	case MMU_PREVIOUS:
		switch(( p->psw & 030000 ) >> 12 ) {
		case 0:
			addr = p->kisa[index];
			desc_p = &p->kisd[index];
			mode = 0;
			break;
		case 1:
			return CPU_NOT_IMPL;
			/*NOTREACHED*/
			break;
		case 2:
			return CPU_NOT_IMPL;
			/*NOTREACHED*/
			break;
		case 3:
			addr = p->uisa[index];
			desc_p = &p->uisd[index];
			mode = 0140;
			break;
		}
		break;
	case MMU_KERNEL: 
		addr = p->kisa[index];
		desc_p = &p->kisd[index];
		mode = 0;
		break;
	default:
		return CPU_NOT_IMPL;
		/*NOTREACHED*/
		break;
	}

	desc = *desc_p;

	if (( desc & 2 ) == 0 ) {
		p->mmr0 |= 0100000;
		p->mmr0 |= mode;
		p->mmr0 |= index << 1;
		return MMU_ERROR;
	}

	plf = (desc >> 8) & 0177;

	if ( desc & 010 ) {		/* downward expanding */
		if ( block < plf ) {
			p->mmr0 |= 040000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}
	} else {			/* upward expanding */
		if ( block > plf ) {
			p->mmr0 |= 040000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}
	}

	if ( write ) {
		if ((desc & 4) == 0 ) {
			p->mmr0 |= 020000;
			p->mmr0 |= mode;
			p->mmr0 |= index << 1;
			return MMU_ERROR;
		}
		*desc_p |= 0100;
	}

	*caddr = addr;
	*caddr <<= 6;
	*caddr += ( laddr & 017777 );

	return OK;
}


/*
 * flush_atc() and flush_atc_page() -
 */

flush_atc()
{
	memset((char*) atc, (int) -1, sizeof( atc ));
}


flush_atc_page( mode, page )
unsigned mode;
unsigned page;
{
	register unsigned i;
	register atc_entry *a;

	a = &atc[ mode ][ page * 128 ];
	for ( i = 0; i < 128; ++i ) {
		a->valid = -1;
		++a;
	}
}
