/*	SCCS Id: @(#)pcconf.h	3.1	93/06/28  	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef PCCONF_H
#define PCCONF_H

#define MICRO		/* always define this! */

#ifdef MSDOS		/* some of this material is MS-DOS specific */

/*
 *  The following options are somewhat configurable depending on
 *  your compiler.
 *  __GO32__ is defined automatically by the djgpp port of gcc.
 *  Manually define MOVERLAY if you are using Microsoft C version 7
 *  or greater.
 */

# if !defined (__GO32__) && !defined(__BORLANDC__) && !defined(AMIGA)
/* #  define OVERLAY         /* MS DOS overlay manager - PGM */
#define MOVERLAY /* Microsoft's MOVE overlay system (MSC >= 7.0) */
# endif


# ifndef __GO32__
#  define MFLOPPY         /* Support for floppy drives and ramdisks by dgk */
# endif

# define SHELL           /* via exec of COMMAND.COM */
/* # define TERMLIB        /* enable use of termcap file /etc/termcap */
			/* or ./termcap for MSDOS (SAC) */
			/* compile and link in Fred Fish's termcap library, */
			/* enclosed in TERMCAP.ARC, to use this */

/* # define ANSI_DEFAULT    /* allows NetHack to run without a ./termcap */

# define NO_TERMS	/* Allows Nethack to run without ansi.sys by linking */
			/* screen routines into the .exe     */

# ifdef NO_TERMS	/* if NO_TERMS select one screen package below */
#define SCREEN_BIOS		/* Use bios calls for all screen control */
/* #define SCREEN_DJGPPFAST	/* Use djgpp fast screen routines       */
# endif

/*# define PC9801	/* Allows NetHack to run on NEC PC-9801 machines */
			/* Yamamoto Keizo */

# define RANDOM		/* have Berkeley random(3) */

#endif /* MSDOS configuration stuff */


#define PATHLEN		64	/* maximum pathlength */
#define FILENAME	80	/* maximum filename length (conservative) */
#ifndef MICRO_H
# include "micro.h"      /* contains necessary externs for [os_name].c */
#endif

#ifdef MFLOPPY
# define FROMPERM        1      /* for ramdisk use */
# define TOPERM          2      /* for ramdisk use */
# define ACTIVE          1
# define SWAPPED         2

struct finfo {
	int	where;
	long	time;
	long	size;
};
extern struct finfo fileinfo[];
# define ZFINFO  { 0, 0L, 0L }

#endif /* MFLOPPY */

/*
 *  The remaining code shouldn't need modification.
 */

#ifndef SYSTEM_H
# include "system.h"
#endif

#ifndef index
# define index	strchr
#endif
#ifndef rindex
# define rindex	strrchr
#endif

#ifndef AMIGA
# include <time.h>
#endif

#ifdef RANDOM
/* Use the high quality random number routines. */
# define Rand()	random()
#else
# define Rand()	rand()
#endif

#ifndef TOS
# define FCMASK	0660	/* file creation mask */
#endif

#include <fcntl.h>

#define exit	msexit		/* do chdir first */

#ifndef REDO
# undef	Getchar
# define Getchar nhgetch
#endif

#ifdef MSDOS
# define TEXTCOLOR /* */
# define PORT_HELP "msdos.hlp"	/* msdos port specific help file */
#endif

#ifdef NO_TERMS		/* Sanity check, do not modify this block */
# ifdef TERMLIB
#  ifdef _MSC_VER
#   pragma message("Warning -- TERMLIB defined with NO_TERMS in pcconf.h")
#   pragma message("           Forcing undef of TERMLIB") 
#  endif
# undef TERMLIB
# endif
# ifdef ANSI_DEFAULT
#  ifdef _MSC_VER
#   pragma message("Warning -- ANSI_DEFAULT defined with NO_TERMS in pcconf.h")
#   pragma message("           Forcing undef of ANSI_DEFAULT") 
#  endif
# undef ANSI_DEFAULT
# endif
/* only one screen package is allowed */
# if defined(SCREEN_BIOS) && defined(SCREEN_DJGPPFAST)
#  ifdef _MSC_VER
#   pragma message("Warning -- More than one screen package defined in pcconf.h")
#   pragma message("           Forcing undef of SCREEN_DJGPPFAST") 
#   undef SCREEN_DJGPPFAST	/* Can't use djgpp fast with MSC anyway */
#  else
#   undef SCREEN_BIOS
#  endif
# endif
# define ASCIIGRAPH
# ifdef TEXTCOLOR
# define VIDEOSHADES
# endif
#endif 			/* End of sanity check block */

#ifdef MSC7_WARN	/* define with cl /DMSC7_WARN	*/
#pragma warning(disable:4131)
#endif

#endif /* PCCONF_H */
