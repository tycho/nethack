/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CONFIG1_H
#define CONFIG1_H

/*
 * MS DOS - compilers
 *
 * Microsoft C auto-defines MSDOS,
 * Borland C   auto-defines __MSDOS__,
 * DJGPP       auto-defines MSDOS.
 */

/* #define MSDOS */	/* use if not defined by compiler or cases below */

#ifdef __MSDOS__	/* for Borland C */
# ifndef MSDOS
# define MSDOS
# endif
#endif

#ifdef __TURBOC__
# define __MSC		/* increase Borland C compatibility in libraries */
#endif

#ifdef MSDOS
# undef UNIX
#endif

#if defined(__BEOS__)
# define NEED_VARARGS
# define DLB
# undef UNIX
#endif

#define NEARDATA

/*
 * Atari auto-detection
 */

#ifdef atarist
# undef UNIX
# ifndef TOS
# define TOS
# endif
#else
# ifdef __MINT__
#  undef UNIX
#  ifndef TOS
#  define TOS
#  endif
# endif
#endif

/*
 * Windows NT Autodetection
 */
#ifdef _WIN32_WCE
#define WIN_CE
# ifndef WIN32
# define WIN32
# endif
#endif

#ifdef WIN32
# undef UNIX
# undef MSDOS
# define NHSTDC
# define USE_STDARG
# define NEED_VARARGS

#ifndef WIN_CE
# define STRNCMPI
# define STRCMPI
#endif

#endif


#if defined(__linux__) && defined(__GNUC__) && !defined(_GNU_SOURCE)
/* ensure _GNU_SOURCE is defined before including any system headers */
# define _GNU_SOURCE
#endif

#ifdef KR1ED		/* For compilers which cannot handle defined() */
#define defined(x) (-x-1 != -1)
/* Because:
 * #define FOO => FOO={} => defined( ) => (-1 != - - 1) => 1
 * #define FOO 1 or on command-line -DFOO
 *	=> defined(1) => (-1 != - 1 - 1) => 1
 * if FOO isn't defined, FOO=0. But some compilers default to 0 instead of 1
 * for -DFOO, oh well.
 *	=> defined(0) => (-1 != - 0 - 1) => 0
 *
 * But:
 * defined("") => (-1 != - "" - 1)
 *   [which is an unavoidable catastrophe.]
 */
#endif

#endif	/* CONFIG1_H */
