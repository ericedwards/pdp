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
 * itab.c - Instruction decode table.
 */

#include "defines.h"

extern int illegal();
extern int adc();
extern int adcb();
extern int add();
extern int ash();
extern int ashc();
extern int asl();
extern int aslb();
extern int asr();
extern int asrb();
extern int bcc();
extern int bcs();
extern int beq();
extern int bge();
extern int bgt();
extern int bhi();
extern int bic();
extern int bicb();
extern int bis();
extern int bisb();
extern int bit();
extern int bitb();
extern int ble();
extern int blos();
extern int blt();
extern int bmi();
extern int bne();
extern int bpl();
extern int bpt();
extern int br();
extern int bvc();
extern int bvs();
extern int clr();
extern int clrb();
extern int ccc();
extern int cmp();
extern int cmpb();
extern int com();
extern int comb();
extern int dec();
extern int decb();
extern int divide();
extern int emt();
extern int halt();
extern int inc();
extern int incb();
extern int iot();
extern int jmp();
extern int jsr();
extern int mark();
extern int mfpd();
extern int mfpi();
extern int mfps();
extern int mov();
extern int movb();
extern int mtpd();
extern int mtpi();
extern int mtps();
extern int mul();
extern int neg();
extern int negb();
extern int busreset();
extern int rol();
extern int rolb();
extern int ror();
extern int rorb();
extern int rti();
extern int rts();
extern int rtt();
extern int sbc();
extern int sbcb();
extern int scc();
extern int sob();
extern int sub();
extern int swabi();
extern int sxt();
extern int trap();
extern int tst();
extern int tstb();
extern int waiti();
extern int xor();
extern int fis();

struct _itab sitab0[64] = {
	halt, waiti, rti, bpt, iot, busreset, rtt, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal
};

struct _itab sitab1[64] = {
	rts, rts, rts, rts, rts, rts, rts, rts,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	ccc, ccc, ccc, ccc, ccc, ccc, ccc, ccc,
	ccc, ccc, ccc, ccc, ccc, ccc, ccc, ccc,
	scc, scc, scc, scc, scc, scc, scc, scc,
	scc, scc, scc, scc, scc, scc, scc, scc
};

int dositab0( p )
register pdp_regs *p;
{
	return (sitab0[p->ir&077].func)( p );
}

int dositab1( p )
register pdp_regs *p;
{
	return (sitab1[p->ir&077].func)( p );
}

struct _itab itab[1024] = {
	dositab0, jmp, dositab1, swabi, br, br, br, br,
	bne, bne, bne, bne, beq, beq, beq, beq,
	bge, bge, bge, bge, blt, blt, blt, blt,
	bgt, bgt, bgt, bgt, ble, ble, ble, ble,
	jsr, jsr, jsr, jsr, jsr, jsr, jsr, jsr,
	clr, com, inc, dec, neg, adc, sbc, tst,
	ror, rol, asr, asl, mark, mfpi, mtpi, sxt,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	mov, mov, mov, mov, mov, mov, mov, mov,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	cmp, cmp, cmp, cmp, cmp, cmp, cmp, cmp,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bit, bit, bit, bit, bit, bit, bit, bit,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bic, bic, bic, bic, bic, bic, bic, bic,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	bis, bis, bis, bis, bis, bis, bis, bis,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	add, add, add, add, add, add, add, add,
	mul, mul, mul, mul, mul, mul, mul, mul,
	divide, divide, divide, divide, divide, divide, divide, divide,
	ash, ash, ash, ash, ash, ash, ash, ash,
	ashc, ashc, ashc, ashc, ashc, ashc, ashc, ashc,
	xor, xor, xor, xor, xor, xor, xor, xor,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	sob, sob, sob, sob, sob, sob, sob, sob,
	bpl, bpl, bpl, bpl, bmi, bmi, bmi, bmi,
	bhi, bhi, bhi, bhi, blos, blos, blos, blos,
	bvc, bvc, bvc, bvc, bvs, bvs, bvs, bvs,
	bcc, bcc, bcc, bcc, bcs, bcs, bcs, bcs,
	emt, emt, emt, emt, trap, trap, trap, trap,
	clrb, comb, incb, decb, negb, adcb, sbcb, tstb,
	rorb, rolb, asrb, aslb, mtps, mfpd, mtpd, mfps,
	illegal, illegal, illegal, illegal, illegal, illegal, illegal, illegal,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	movb, movb, movb, movb, movb, movb, movb, movb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb, cmpb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bitb, bitb, bitb, bitb, bitb, bitb, bitb, bitb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bicb, bicb, bicb, bicb, bicb, bicb, bicb, bicb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	bisb, bisb, bisb, bisb, bisb, bisb, bisb, bisb,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	sub, sub, sub, sub, sub, sub, sub, sub,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis,
	fis, fis, fis, fis, fis, fis, fis, fis
};
