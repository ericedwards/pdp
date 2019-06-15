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
 * main.c -  Main routine and setup.
 */


#include "defines.h"


/*
 * Globals.
 */

char rl_gpath[BUFSIZ];
char lp_gpath[BUFSIZ];
char tm_gpath[BUFSIZ];
char kl_gpath[NUM_KL][BUFSIZ];


/*
 * Command line flags and variables.
 */

int aflag;		/* autoboot flag */
d_word aaddr = BOOT;	/* autoboot address */


/*
 * main()
 */

int
main( argc, argv )
int argc;
char **argv;
{
	
	aflag = 0;		/* no auto boot */

	strcpy( tm_gpath, "./TAPE" );		/* default tape pathname */
	strcpy( rl_gpath, "./DRIVE" );		/* default disk pathname */
	strcpy( lp_gpath, "./PRINTER" );	/* default printer pathname */
	strcpy( kl_gpath[0], "/dev/tty" );	/* default console pathname */

	if ( args( argc, argv ) < 0 ) {
		fprintf( stderr, "Usage: %s [options]\n", argv[0] );
		fprintf( stderr, "\t-a        autoboot\n" );
		fprintf( stderr, "\t-d<path>  set disk base pathname\n" );
		fprintf( stderr, "\t-t<path>  set tape base pathname\n" );
		fprintf( stderr, "\t-l<path>  set printer pathname\n" );
		fprintf( stderr, "\t-0<path>  set console port device\n" );
		fprintf( stderr, "\t-1<path>  set tty port 1 device\n" );
		exit( -1 );
	}

	kl_open();		/* initialize the tty stuff */
	ev_init();		/* initialize the event system */
	sim_init();		/* ...the simulated cpu */
	mem_init();		/* ...main memory */
	ub_reset();		/* ...any devices (MMU is here!) */

	if ( aflag ) {
		pdp.regs[PC] = aaddr;		/* go for it */
		run( 1 );
	} else {
		ui();				/* run the user interface */
	}

	return 0;		/* get out of here */
}


/*
 * args()
 */

args( argc, argv )
int argc;
char **argv;
{
	char *farg;
	char **narg;

	narg = argv;
	while ( --argc ) {
		narg++;
		farg = *narg;
		if ( *farg++ == '-' ) {
			switch( *farg ) {
			case 'a':
				aflag = 1;
				break;
			case 'd':
				strcpy( rl_gpath, ++farg );
				break;
			case 't':
				strcpy( tm_gpath, ++farg );
				break;
			case 'l':
				strcpy( lp_gpath, ++farg );
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				if (( *farg - '0' ) < NUM_KL )
					strcpy( kl_gpath[*farg - '0'], ++farg );
				break;
			default:
				return -1;
				/*NOTREACHED*/
				break;
			}
		}
	}
	return 0;
}


#define LOOK_NOW	50
#define INST_TIME	4


pdp_regs pdp;		/* internal processor registers */
int stop_it;		/* set when a SIGINT happens during execution */


/*
 * sim_init() - Initialize the cpu registers.
 */

int
sim_init()
{
	int x;

	for ( x = 0; x < 8; ++x ) {
		pdp.regs[x] = 0;
		pdp.uisd[x] = 0;
		pdp.uisa[x] = 0;
		pdp.kisd[x] = 0;
		pdp.kisa[x] = 0;
	}
	pdp.ir = 0;
	pdp.psw = 0340;
	pdp.mmr0 = 0;
	pdp.mmr2 = 0;
	pdp.sr = 0;

	flush_atc();
}


/*
 * run() - Run instructions (either just one or until something bad
 * happens).  Lots of nasty stuff to set up the terminal and the
 * execution timing.
 */

int
run( flag )
int flag;
{
	register pdp_regs *p = &pdp;	/* pointer to global struct */
	struct timeval start_time;	/* system time of simulation start */
	struct timeval stop_time;	/* system time of simulation end */
	struct timezone time_zone;	/* timezone, blah... */
	double expired, speed;		/* for statistics information */

	/*
	 * Set up the terminal cbreak i/o and start running.
	 */

	kl_con_grab();
	gettimeofday( &start_time, &time_zone );
	run_2( p, flag );
	gettimeofday( &stop_time, &time_zone );
	kl_con_release();

	/*
	 * Compute execution statistics and print it.
	 */

	expired = ((double) stop_time.tv_sec) +
		(((double) stop_time.tv_usec) / 1000000.0 );
	expired -= ((double) start_time.tv_sec) +
		(((double) start_time.tv_usec) / 1000000.0 );
	if ( expired != 0.0 )
		speed = (((double)p->total) / expired );
	else
		speed = 0.0;
	fprintf( stderr, "Instructions executed: %d\n", p->total );
	fprintf( stderr, "Simulation rate: %.5g instructions per second\n",
		speed );
	p->total = 0;
}


int
run_2( p, flag )
register pdp_regs *p;
int flag;
{
	register int result;		/* result of execution */
	int result2;			/* result of error handling */
	extern void intr_hand();	/* SIGINT handler */
	register unsigned priority;	/* current processor priority */
	int rtt = 0;			/* rtt don't trap yet flag */

	/*
	 * Clear execution stop flag and install SIGINT handler.
	 */

	stop_it = 0;
	signal( SIGINT, intr_hand );
	p->look_time = 0;

	/*
	 * Run until told to stop.
	 */

	do {

		/*
		 * Fetch and execute the instruction.
		 */
	
		if (( p->mmr0 & 0160000 ) == 0 )
			p->mmr2 = p->regs[PC];
		result = ll_word( p, p->regs[PC], &p->ir );
		p->regs[PC] += 2;
		if (result == OK)
			result = (itab[p->ir>>6].func)( p );

		/*
		 * Mop up the mess.
		 */

		if ( result != OK ) {
			switch( result ) {
			case BUS_ERROR:			/* vector 4 */
			case ODD_ADDRESS:
				result2 = service( (d_word) 04 );
				break;
			case MMU_ERROR:			/* vector 250 */
				result2 = service( (d_word) 0250 );
				break;
			case CPU_ILLEGAL:		/* vector 10 */
				result2 = service( (d_word) 010 );
				break;
			case CPU_BPT:			/* vector 14 */
				result2 = service( (d_word) 014 );
				break;
			case CPU_EMT:			/* vector 30 */
				result2 = service( (d_word) 030 );
				break;
			case CPU_TRAP:			/* vector 34 */
				result2 = service( (d_word) 034 );
				break;
			case CPU_IOT:			/* vector 20 */
				result2 = service( (d_word) 020 );
				break;
			case CPU_WAIT:		/* pause and look around */
				kl_recv();	/* hack for console i/o */
				do_waiti( p );	/* resync time */
				result2 = OK;
				break;
			case CPU_RTT:
				rtt = 1;
				result2 = OK;
				break;
			case CPU_HALT:
				fprintf( stderr, "\nProcessor halted @ %06o\n",
					p->regs[PC] );
				flag = 0;
				result2 = OK;
				break;
			default:
				fprintf( stderr, "\nUnexpected return.\n" );
				fprintf( stderr, "exec=%d pc=%06o ir=%06o\n",
					result, p->regs[PC], p->ir );
				flag = 0;
				result2 = OK;
				break;
			}
			if ( result2 != OK ) {
				fprintf( stderr, "\nDouble trap.\n" );
				flag = 0;
			}
		}

		if (( p->psw & 020) && (rtt == 0 )) {		/* trace bit */
			if ( service((d_word) 014 ) != OK ) {
				fprintf( stderr, "\nDouble trap.\n" );
				flag = 0;
			}
		}
		rtt = 0;
		p->total++;
		p->look_time++;

		/*
		 * See if execution should be stopped.  If so
		 * stop running, otherwise look for events
		 * to fire.
		 */

		if ( p->look_time > LOOK_NOW ) {
			if ( stop_it ) {
				fprintf( stderr, "\nExecution interrupted.\n" );
				flag = 0;
				goto out;
			}
			real_time.tv_usec += INST_TIME * p->look_time;
			if ( real_time.tv_usec >= 1000000 ) {
				real_time.tv_usec = 0;
				real_time.tv_sec++;
				kl_recv();
			}
			p->look_time = 0;
			priority = ( p->psw >> 5) & 7;
			if ( event_list[priority] ) {
				if ( event_list[priority]->when.tv_sec <
				 real_time.tv_sec )
					ev_fire( priority );
				else if (( event_list[priority]->when.tv_sec ==
				 real_time.tv_sec ) &&
				 ( event_list[priority]->when.tv_usec <=
				 real_time.tv_usec ))
					ev_fire( priority );
			}
		}
out:
#if defined(sparc) || defined(vax)
	priority = priority; 	/* compiler is stupid */
#endif
	} while( flag );

	real_time.tv_usec += INST_TIME * p->look_time;
	if ( real_time.tv_usec >= 1000000 ) {
		real_time.tv_usec = 0;
		real_time.tv_sec++;
	}

	signal( SIGINT, SIG_DFL );
}


/*
 * intr_hand() - Handle SIGINT during execution by breaking
 * back to user interface at the end of the current instruction.
 */

void intr_hand()
{
	stop_it = 1;
}


/*
 * do_waiti() - Do the mechanics of the wait instruction.  If any
 * events are queued up the the future, skip the real_time up
 * to them and fire then event.  Also attempt to do some clock
 * synchronization with the real system time. Every five times
 * through this routine see if the simulated time is at least a second
 * ahead of the real system time... if so, go to sleep for a
 * second.  Most of the time we will be behind (since simulation
 * is slow) so by executing the next event right away we can
 * catch up. On machines that have the library routine usleep(3), sleep
 * for only a quarter of a second, so terminal i/o is more responsive.
 */

do_waiti( p )
register pdp_regs *p;
{
	unsigned priority;
	static int resync = 0;
	unsigned long diff;
	struct timeval now;
	struct timezone huhhuhh;

	real_time.tv_usec += INST_TIME * p->look_time;
	if ( real_time.tv_usec >= 1000000 ) {
		real_time.tv_usec = 0;
		real_time.tv_sec++;
	}
	p->look_time = 0;

	priority = (p->psw >> 5) & 7;
	if ( event_list[priority] ) {
		real_time.tv_sec = event_list[priority]->when.tv_sec;
		real_time.tv_usec = event_list[priority]->when.tv_usec;
		ev_fire( priority );
	}

	if ( ++resync >= 5 ) {
		resync = 0;
		gettimeofday( &now, &huhhuhh );
		if (( now.tv_sec < real_time.tv_sec ) ||
		 (( now.tv_sec == real_time.tv_sec ) &&
		 ( now.tv_usec < real_time.tv_usec ))) {
			usleep( 100000 );
		}
	}
}
