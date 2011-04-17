/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you change the flag structure make sure you increment EDITLEVEL in   */
/* patchlevel.h if needed.  Changing the instance_flags structure does	   */
/* not require incrementing EDITLEVEL.					   */

#ifndef FLAG_H
#define FLAG_H

/*
 * Persistent flags that are saved and restored with the game.
 *
 */

struct flag {
	boolean  autodig;       /* MRKR: Automatically dig */
	boolean  autoquiver;	/* Automatically fill quiver */
	boolean  beginner;
	boolean  confirm;	/* confirm before hitting tame monsters */
	boolean  debug;		/* in debugging mode */
#define wizard	 flags.debug
	boolean  end_own;	/* list all own scores */
	boolean  explore;	/* in exploration mode */
#define discover flags.explore
	boolean  female;
	boolean  forcefight;
	boolean  friday13;	/* it's Friday the 13th */
	boolean  help;		/* look in data file for info about stuff */
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
	boolean  bypasses;	/* bypass flag is set on at least one fobj */
	boolean  nap;		/* `timed_delay' option for display effects */
	boolean  nopick;	/* do not pickup objects (as when running) */
	boolean  null;		/* OK to send nulls to the terminal */
	boolean  perm_invent;	/* keep full inventories up until dismissed */
	boolean  pickup;	/* whether you pickup or move and look */

	boolean  pushweapon;	/* When wielding, push old weapon into second slot */
	boolean  rest_on_space; /* space means rest */
	boolean  safe_dog;	/* give complete protection to the dog */
	boolean  showexp;	/* show experience points */
#ifdef SCORE_ON_BOTL
	boolean  showscore;	/* show score */
#endif
	boolean  silent;	/* whether the bell rings or not */
	boolean  sortpack;	/* sorted inventory */
	boolean  soundok;	/* ok to tell about sounds heard */
	boolean  sparkle;	/* show "resisting" special FX (Scott Bigham) */
	boolean  time;		/* display elapsed 'time' */
	boolean  tombstone;	/* print tombstone */
	boolean  toptenwin;	/* ending list in window instead of stdout */
	boolean  verbose;	/* max battle info */
	boolean  prayconfirm;	/* confirm before praying */
	int	 end_top, end_around;	/* describe desired score list */
	unsigned ident;		/* social security number for each monster */
	unsigned moonphase;
	unsigned long suppress_alert;
#define NEW_MOON	0
#define FULL_MOON	4
	unsigned no_of_wizards; /* 0, 1 or 2 (wizard and his shadow) */
	boolean  travel;	/* find way automatically to u.tx,u.ty */
	unsigned run;		/* 0: h (etc), 1: H (etc), 2: fh (etc) */
				/* 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF- */
				/* 8: travel */
	unsigned long warntype; /* warn_of_mon monster type M2 */
	int	 warnlevel;
	int	 djinni_count, ghost_count;	/* potion effect tuning */
	int	 pickup_burden;		/* maximum burden before prompt */
	char	 inv_order[MAXOCLASSES];
	char	 pickup_types[MAXOCLASSES];
#define NUM_DISCLOSURE_OPTIONS		5
#define DISCLOSE_PROMPT_DEFAULT_YES	'y'
#define DISCLOSE_PROMPT_DEFAULT_NO	'n'
#define DISCLOSE_YES_WITHOUT_PROMPT	'+'
#define DISCLOSE_NO_WITHOUT_PROMPT	'-'
	char	 end_disclose[NUM_DISCLOSURE_OPTIONS + 1];  /* disclose various info
								upon exit */
	char	 menu_style;	/* User interface style setting */

	/* KMH, role patch -- Variables used during startup.
	 *
	 * If the user wishes to select a role, race, gender, and/or alignment
	 * during startup, the choices should be recorded here.  This
	 * might be specified through command-line options, environmental
	 * variables, a popup dialog box, menus, etc.
	 *
	 * These values are each an index into an array.  They are not
	 * characters or letters, because that limits us to 26 roles.
	 * They are not booleans, because someday someone may need a neuter
	 * gender.  Negative values are used to indicate that the user
	 * hasn't yet specified that particular value.	If you determine
	 * that the user wants a random choice, then you should set an
	 * appropriate random value; if you just left the negative value,
	 * the user would be asked again!
	 *
	 * These variables are stored here because the u structure is
	 * cleared during character initialization, and because the
	 * flags structure is restored for saved games.  Thus, we can
	 * use the same parameters to build the role entry for both
	 * new and restored games.
	 *
	 * These variables should not be referred to after the character
	 * is initialized or restored (specifically, after role_init()
	 * is called).
	 */
	int	 initrole;	/* starting role      (index into roles[])   */
	int	 initrace;	/* starting race      (index into races[])   */
	int	 initgend;	/* starting gender    (index into genders[]) */
	int	 initalign;	/* starting alignment (index into aligns[])  */
	int	 randomall;	/* randomly assign everything not specified */
	int	 pantheon;	/* deity selection for priest character */
};


/*
 * Old deprecated names
 */
#define hilite_pet wc_hilite_pet

extern struct flag flags;

/* runmode options */
#define RUN_TPORT	0	/* don't update display until movement stops */
#define RUN_LEAP	1	/* update display every 7 steps */
#define RUN_STEP	2	/* update display every single step */
#define RUN_CRAWL	3	/* walk w/ extra delay after each update */

#endif /* FLAG_H */
