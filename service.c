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
 * service.c - Event handler system and interrupt service.
 */


#include "defines.h"


/*
 * Event lists and clock.
 */


struct timeval real_time;	/* current simulator time */
event events[NUM_EVENTS];	/* events */
event *free_events;		/* free list */
event *event_list[NUM_PRI];		/* pending event list */


/*
 * ev_init() - Initialize the event system.
 */

ev_init()
{
	int x;
	struct timezone huhhuhh;
	event *p;

	gettimeofday( &real_time, &huhhuhh );
	for ( x = 0; x < NUM_PRI; ++x ) {
		event_list[x] = (event *) 0;
	}
	free_events = (event *) 0;
	for ( x = 0; x < NUM_EVENTS; ++x ) {
		events[x].next[0] = free_events;
		free_events = &events[x];
	}
		
}

/*
 * ev_register() - Register an event.
 */

int
ev_register( priority, handler, delay, info )
unsigned priority;
int (*handler)(); 
unsigned long delay;	/* in microseconds */
d_word info;
{
	event *new;
	unsigned x;

	/*
	 * Allocate a new event buffer.
	 */

	if (( new = free_events ) == (event * ) 0 ) {
		fprintf( stderr, "ev_register(): no free events\n" );
		exit( 1 );
	}
	free_events = new->next[0];

	/*
	 * Fill in the info.
	 */

	new->handler = handler;
	new->priority = priority;
	new->info = info;
	new->when = real_time;
	new->when.tv_usec += delay;
	if ( new->when.tv_usec >= 1000000 ) {
		new->when.tv_usec -= 1000000;
		new->when.tv_sec++;
	}

	/*
	 * Insert into the priority lists.
	 */

	for ( x = 0; x < priority; ++x ) {
		ev_insert( x, new );
	}
}


/*
 * ev_fire() - Fire off the event at the head of the given
 * priority list.
 */

ev_fire( priority )
unsigned priority;
{
	event *fire;
	unsigned x;

	fire = event_list[priority];
	for( x = 0; x < fire->priority; ++x ) {
		ev_delete( x, fire );
	}
	(fire->handler)( fire->info );
	fire->next[0] = free_events;
	free_events = fire;
}


/*
 * ev_insert() and ev_delete() - Insert and delete from
 * event lists.
 */

ev_insert( list, item )
unsigned list;
event *item;
{
	event *back;
	event *temp;

	if ( event_list[list] == (event *) 0 ) {
		event_list[list] = item;
		item->next[list] = (event *) 0;
	} else {
		back = (event *) 0;
		temp = event_list[list];
		while( temp &&
			(( item->when.tv_sec > temp->when.tv_sec ) ||
			(( item->when.tv_sec == temp->when.tv_sec ) &&
			( item->when.tv_usec >= temp->when.tv_sec )))) {
				back = temp;
				temp = temp->next[list];
		}
		if ( back ) {
			back->next[list] = item;
			item->next[list] = temp;
		} else {
			item->next[list] = event_list[list];
			event_list[list] = item;
		}
	}
}

ev_delete( list, item )
unsigned list;
event *item;
{
	event *back;
	event *temp;

	back = (event *) 0;
	temp = event_list[list];
	while( temp != item ) {
		back = temp;
		temp = temp->next[list];
	}
	if ( back ) {
		back->next[list] = item->next[list];
	} else {
		event_list[list] = item->next[list];
	}
}


/*
 * service() - Handle a Trap.
 */

int
service( vector )
d_word vector;
{
	register pdp_regs *p = &pdp;
	int result;
	d_word oldpsw;
	d_word oldpc;
	d_word oldmode;
	d_word newmode;
	c_addr vaddr;

	oldmode = ( p->psw & 0140000 ) >> 14;

	oldpsw = p->psw;
	oldpc = p->regs[PC];

	if (( result = map( p, vector, &vaddr, MMU_KERNEL, 0 )) != OK )
		return result;
	if (( result = lc_word( vaddr, &( p->regs[PC]))) != OK)
		return result;
	if (( result = lc_word( vaddr + 2, &( p->psw ))) != OK)
		return result;

	newmode = ( p->psw & 0140000 ) >> 14;

	p->stacks[ oldmode ] = p->regs[SP];
	p->regs[SP] = p->stacks[ newmode ];
	p->psw = p->psw & 0147777;
	p->psw |= ( oldmode << 12 );

	if (( result = push( p, oldpsw )) != OK )
		return result;
	if (( result = push( p, oldpc )) != OK )
		return result;

	return OK;
}


/*
 * rti() - Return from Interrupt Instruction.
 */

rti( p )
register pdp_regs *p;
{
	d_word newpsw;
	d_word newpc;
	d_word oldmode;
	d_word newmode;
	int result;

	oldmode = ( p->psw & 0140000 ) >> 14;

	if (( result = pop( p, &newpc )) != OK )
		return result;
	if (( result = pop( p, &newpsw )) != OK )
		return result;

	if ( !(KERNEL( p->psw ))) {
		newpsw &= ~0340;
		newpsw |= ( p->psw & 0340 );
		p->psw = newpsw | ( p->psw & 0170000);
	} else {
		p->psw = newpsw;
	}

	newmode = ( p->psw & 0140000 ) >> 14;

	p->regs[PC] = newpc;
	p->stacks[ oldmode ] = p->regs[SP];
	p->regs[SP] = p->stacks[ newmode ];

	return OK;
}


/*
 * rtt() - Return from Interrupt Instruction.
 */

rtt( p )
register pdp_regs *p;
{
	d_word newpsw;
	d_word newpc;
	d_word oldmode;
	d_word newmode;
	int result;

	oldmode = ( p->psw & 0140000 ) >> 14;

	if (( result = pop( p, &newpc )) != OK )
		return result;
	if (( result = pop( p, &newpsw )) != OK )
		return result;

	if ( !(KERNEL( p->psw ))) {
		newpsw &= ~0340;
		newpsw |= ( p->psw & 0340 );
		p->psw = newpsw | ( p->psw & 0170000 );
	} else {
		p->psw = newpsw;
	}

	newmode = ( p->psw & 0140000 ) >> 14;

	p->regs[PC] = newpc;
	p->stacks[ oldmode ] = p->regs[SP];
	p->regs[SP] = p->stacks[ newmode ];

	return CPU_RTT;
}
