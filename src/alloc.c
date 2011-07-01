/*	SCCS Id: @(#)alloc.c	3.1	90/22/02
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* to get the malloc() prototype from system.h */
#define ALLOC_C		/* comment line for pre-compiled headers */
/* since this file is also used in auxiliary programs, don't include all the 
 * function declarations for all of nethack
 */
#define EXTERN_H	/* comment line for pre-compiled headers */
#include "config.h"
long *FDECL(alloc,(unsigned int));

#ifdef LINT
/*
   a ridiculous definition, suppressing
	"possible pointer alignment problem" for (long *) malloc()
   from lint
*/
long *
alloc(n) unsigned int n; {
long dummy = ftell(stderr);
	if(n) dummy = 0;	/* make sure arg is used */
	return(&dummy);
}

#else
#ifndef __TURBOC__
extern void VDECL(panic, (const char *,...)) PRINTF_F(1,2);

long *
alloc(lth)
register unsigned int lth;
{
	register genericptr_t ptr;

	if(!(ptr = malloc(lth)))
		panic("Cannot get %d bytes", lth);
	return((long *) ptr);
}
#endif

#endif /* LINT /**/

/*alloc.c*/
