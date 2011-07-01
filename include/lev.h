/*	SCCS Id: @(#)lev.h	3.0	88/08/05
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*	Common include file for save.c and restore.c */

#ifndef LEV_H
#define LEV_H

#ifndef OLD_TOS
# ifdef MACOS
#define OMASK	0x8000	/* O_BINARY */
# else
#define OMASK	0
# endif
#else
#define msmsg	cprintf
#define OMASK	0x8000
#endif

#endif /* LEV_H /**/
