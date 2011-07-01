/*	SCCS Id: @(#)flag.h	3.2	96/02/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you change this file make sure you increment EDITLEVEL in   */
/* patchlevel.h if needed.					  */

#ifndef FLAG_H
#define FLAG_H

struct flag {	
#ifdef AMIFLUSH
	boolean  altmeta;	/* use ALT keys as META */
	boolean  amiflush;	/* kill typeahead */
#endif
#ifdef	MFLOPPY
	boolean  asksavedisk;
#endif
	boolean  beginner;
#ifdef MAIL
	boolean  biff;		/* enable checking for mail */
#endif
#ifdef MICRO
	boolean  BIOS;		/* use IBM or ST BIOS calls when appropriate */
#endif
	boolean  botl;		/* partially redo status line */
	boolean  botlx;		/* print an entirely new bottom line */
	boolean  cbreak;	/* in cbreak mode, rogue format */
	boolean  confirm;	/* confirm before hitting tame monsters */
	boolean  debug;		/* in debugging mode */
#define wizard	 flags.debug
	boolean  DECgraphics;	/* use DEC VT-xxx extended character set */
	boolean  echo;		/* 1 to echo characters */
	boolean  end_own;	/* list all own scores */
	boolean  explore;	/* in exploration mode */
#ifdef OPT_DISPMAP
	boolean  fast_map;	/* use optimized, less flexible map display */
#endif
#define discover flags.explore
	boolean  female;
	boolean  friday13;	/* it's Friday the 13th */
	boolean  help;		/* look in data file for info about stuff */
	boolean  IBMgraphics;	/* use IBM extended character set */
	boolean  ignintr;	/* ignore interrupts */
#ifdef INSURANCE
	boolean  ins_chkpt;	/* checkpoint as appropriate */
#endif
	boolean  invlet_constant; /* let objects keep their inventory symbol */
	boolean  legacy;	/* print game entry "story" */
	boolean  lit_corridor;	/* show a dark corr as lit if it is in sight */
	boolean  made_amulet;
	boolean  mon_moving;	/* monsters' turn to move */
	boolean  move;
	boolean  mv;
#ifdef TIMED_DELAY
	boolean  nap;		/* `timed_delay' option for display effects */
#endif
	boolean  news;		/* print news */
	boolean  nopick;	/* do not pickup objects (as when running) */
	boolean  null;		/* OK to send nulls to the terminal */
	boolean  num_pad;	/* use numbers for movement commands */
#ifdef MAC
	boolean  page_wait;	/* put up a --More-- after a page of messages */
#endif
	boolean  perm_invent;	/* keep full inventories up until dismissed */
	boolean  pickup;	/* whether you pickup or move and look */
#ifdef MAC
	boolean  popup_dialog;	/* put queries in pop up dialogs instead of
				   in the message window */
#endif
#ifdef MICRO
	boolean  rawio;		/* Whether can use rawio (IOCTL call) */
#endif
	boolean  rest_on_space;	/* space means rest */
	boolean  safe_dog;	/* give complete protection to the dog */
#ifdef WIZARD
	boolean	 sanity_check;	/* run sanity checks */
#endif
#ifdef EXP_ON_BOTL
	boolean  showexp;	/* show experience points */
#endif
#ifdef SCORE_ON_BOTL
	boolean  showscore;	/* show score */
#endif
	boolean  silent;	/* whether the bell rings or not */
	boolean  sortpack;	/* sorted inventory */
	boolean  soundok;	/* ok to tell about sounds heard */
	boolean  standout;	/* use standout for --More-- */
	boolean  time;		/* display elapsed 'time' */
	boolean  tombstone;	/* print tombstone */
	boolean  toptenwin;	/* ending list in window instead of stdout */
#ifdef TEXTCOLOR
	boolean  use_color;	/* use color graphics */
	boolean  hilite_pet;	/* hilight pets on monochome displays */
#endif
	boolean  verbose;	/* max battle info */

	boolean  window_inited;	/* true if init_nhwindows() completed */
	int	 end_top, end_around;	/* describe desired score list */
	unsigned ident;		/* social security number for each monster */
	unsigned moonphase;
#define NEW_MOON	0
#define FULL_MOON	4
	unsigned msg_history;	/* hint: # of top lines to save */
	unsigned no_of_wizards;	/* 0, 1 or 2 (wizard and his shadow) */
	unsigned run;		/* 0: h (etc), 1: H (etc), 2: fh (etc) */
				/* 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF- */
	int	 djinni_count, ghost_count;	/* potion effect tuning */
	char	 inv_order[MAXOCLASSES];
	char	 pickup_types[MAXOCLASSES];
	char	 end_disclose[5];	/* disclose various info upon exit */
	char	 menu_style;	/* User interface style setting */
#ifdef MAC_GRAPHICS_ENV
	boolean  large_font;	/* draw in larger fonts (say, 12pt instead
				   of 9pt) */
	boolean  MACgraphics;	/* use Macintosh extended character set, as
				   as defined in the special font HackFont */
#endif
#ifdef AMII_GRAPHICS
	int numcols;
	unsigned short amii_dripens[ 20 ]; /* DrawInfo Pens currently there are 13 in v39 */
	AMII_COLOR_TYPE amii_curmap[ AMII_MAXCOLORS ]; /* colormap */
#endif
#ifdef MSDOS
	boolean	hasvga;			/* has a vga adapter                 */
	boolean usevga;			/* use the vga adapter               */
	boolean has8514;
	boolean use8514;
	boolean hasvesa;
	boolean usevesa;
	boolean grmode;			/* currently in graphics mode        */
#endif

#if defined(MSDOS) || defined(WIN32)
	boolean hassound;		/* has a sound card                  */
	boolean usesound;		/* use the sound card                */
# ifdef PCMUSIC
	boolean usepcspeaker;		/* use the pc speaker		     */
# endif
#endif
};


extern NEARDATA struct flag flags;

#endif /* FLAG_H */
