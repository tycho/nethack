/*	SCCS Id: @(#)monst.h	3.0	88/04/15
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONST_H
#define MONST_H

struct monst {
	struct monst *nmon;
	struct permonst *data;
	unsigned m_id;
	uchar m_lev;		/* adjusted difficulty level of monster */
	schar malign;		/* alignment of this monster, relative to the
				   player (positive = good to kill) */
	xchar mx, my;
	xchar mux, muy;		/* where the monster thinks you are */
	xchar mdx, mdy;		/* if mdispl then pos where last displayed */
#define	MTSZ	4
	coord mtrack[MTSZ];	/* monster track */
	int mhp, mhpmax;
	unsigned mappearance;	/* for undetected mimics and the wiz */
	uchar	 m_ap_type;	/* what mappearance is describing: */
#define M_AP_NOTHING	0	/* mappearance is unused */
#define M_AP_FURNITURE	1	/* stairs, a door, an altar, etc. */
#define M_AP_OBJECT	2	/* an object */
#define M_AP_MONSTER	3	/* a monster */
#define M_AP_GOLD	4	/* a quantity (mappearance holds it) of gold */

	schar mtame;		/* level of tameness, implies peaceful */
	int mspec_used; 	/* monster's special ability attack timeout */

	Bitfield(mimic,1);	/* undetected mimic */
	Bitfield(mdispl,1);	/* mdx,mdy valid */
	Bitfield(minvis,1);	/* invisible */
	Bitfield(cham,1);	/* shape-changer */
	Bitfield(mhide,1);	/* hides beneath objects */
	Bitfield(mundetected,1);	/* not seen in present hiding place */
	Bitfield(mspeed,2);

	Bitfield(mflee,1);	/* fleeing */
	Bitfield(mfleetim,7);	/* timeout for mflee */

	Bitfield(mcansee,1);	/* cansee 1, temp.blinded 0, blind 0 */
	Bitfield(mblinded,7);	/* cansee 0, temp.blinded n, blind 0 */

	Bitfield(mcanmove,1);	/* paralysis, similar to mblinded */
	Bitfield(mfrozen,7);

	Bitfield(msleep,1);	/* sleeping */
	Bitfield(mstun,1);	/* stunned (off balance) */
	Bitfield(mconf,1);	/* confused */
	Bitfield(mcan,1);	/* has been cancelled */
	Bitfield(mpeaceful,1);	/* does not attack unprovoked */
	Bitfield(mtrapped,1);	/* trapped in a pit or bear trap */
	Bitfield(mleashed,1);	/* monster is on a leash */
	Bitfield(isshk,1);	/* is shopkeeper */

	Bitfield(isgd,1);	/* is guard */
#if defined(ALTARS) && defined(THEOLOGY)
	Bitfield(ispriest,1);	/* is a priest */
#endif
	Bitfield(iswiz,1);	/* is the Wizard of Yendor */
#ifdef WORM
	Bitfield(wormno,5);	/* at most 31 worms on any level */
#endif

	long mtrapseen;		/* bitmap of traps we've been trapped in */
	long mlstmv;		/* prevent two moves at once */
	struct obj *minvent;
	long mgold;
	uchar mnamelth;		/* length of name (following mxlth) */
	short mxlth;		/* length of following data */
	/* in order to prevent alignment problems mextra should
	   be (or follow) a long int */
	int meating;		/* monster is eating timeout */
	long mextra[1]; 	/* monster dependent info */
};

#define newmonst(xl)	(struct monst *) alloc((unsigned)(xl) + sizeof(struct monst))

extern struct monst *mydogs, *fallen_down;

/* these are in mspeed */
#define MSLOW 1 /* slow monster */
#define MFAST 2 /* speeded monster */

#define	NAME(mtmp)	(((char *) mtmp->mextra) + mtmp->mxlth)

#endif /* MONST_H /**/
