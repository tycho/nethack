/*	SCCS Id: @(#)version.c	3.0	89/06/12
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include	"hack.h"
#include	"date.h"
#ifndef BETA
#include	"patchlevel.h"
#endif

int
doversion(){

#ifdef BETA
	pline("%s NetHack Beta Version %s - last build %s.",
#else
	pline("%s NetHack Version %s Patchlevel %d - last build %s.",
#endif
#if defined(MSDOS)
# if defined(TOS)
		"ST",
# else
#  if defined(AMIGA)
		"Amiga",
#  else
#   if defined(OS2)
		"OS/2",
#   else
		"PC",
#   endif
#  endif
# endif
#endif
#ifdef UNIX
		"Unix",
#endif
		VERSION,
#ifndef BETA
		PATCHLEVEL,
#endif
		datestring);
	return 0;
}
