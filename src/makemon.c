/*	SCCS Id: @(#)makemon.c	3.0	89/11/22
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifdef REINCARNATION
# include <ctype.h>
#endif

STATIC_VAR struct monst NEARDATA zeromonst;

#ifdef OVL0
static int NDECL(cmnum);
static int FDECL(uncommon, (struct permonst *));
#endif /* OVL0 */
STATIC_DCL void FDECL(m_initgrp,(struct monst *,int,int,int));
STATIC_DCL void FDECL(m_initthrow,(struct monst *,int,int));
STATIC_DCL void FDECL(m_initweap,(struct monst *));
STATIC_DCL void FDECL(rloc_to,(struct monst *,int,int));
#ifdef OVL1
static void FDECL(m_initinv,(struct monst *));
static int FDECL(mstrength,(struct permonst *));
#endif /* OVL1 */

extern int monstr[];

#ifdef OVLB

int monstr[NUMMONS];

#endif /* OVLB */

#define m_initsgrp(mtmp, x, y)	m_initgrp(mtmp, x, y, 3)
#define m_initlgrp(mtmp, x, y)	m_initgrp(mtmp, x, y, 10)
#define toostrong(monindx, lev) (monstr[monindx] > lev)
#define tooweak(monindx, lev)	(monstr[monindx] < lev)

#ifdef OVLB

STATIC_OVL void
m_initgrp(mtmp, x, y, n)	/* make a group just like mtmp */
register struct monst *mtmp;
register int x, y, n;
{
	coord mm;
	register int cnt = rnd(n);
	struct monst *mon;

/*
 *	Temporary kludge to cut down on swarming at lower character levels
 *	till we can get this game a little more balanced. [mrs]
 */
	cnt /= (u.ulevel < 3) ? 4 : (u.ulevel < 5) ? 2 : 1;
	if(!cnt) cnt++;

	mm.x = x;
	mm.y = y;
	while(cnt--) {
		if (peace_minded(mtmp->data)) continue;
		/* Don't create groups of peaceful monsters since they'll get
		 * in our way.  If the monster has a percentage chance so some
		 * are peaceful and some are not, the result will just be a
		 * smaller group.
		 */
		if (enexto(&mm, mm.x, mm.y, mtmp->data)) {
		    mon = makemon(mtmp->data, mm.x, mm.y);
		    mon->mpeaceful = 0;
		    set_malign(mon);
		    /* Undo the second peace_minded() check in makemon(); if the
		     * monster turned out to be peaceful the first time we
		     * didn't create it at all; we don't want a second check.
		     */
		}
	}
}

STATIC_OVL
void
m_initthrow(mtmp,otyp,oquan)
struct monst *mtmp;
int otyp,oquan;
{
	register struct obj *otmp;

	otmp = mksobj(otyp,FALSE);
	otmp->quan = 2 + rnd(oquan);
	otmp->owt = weight(otmp);
#ifdef TOLKIEN
	if (otyp == ORCISH_ARROW) otmp->opoisoned = 1;
#endif
	mpickobj(mtmp, otmp);
}

#endif /* OVLB */
#ifdef OVL2

STATIC_OVL void
m_initweap(mtmp)
register struct monst *mtmp;
{
	register struct permonst *ptr = mtmp->data;
	register int mm = monsndx(ptr);
#ifdef REINCARNATION
	if (dlevel==rogue_level) return;
#endif
/*
 *	first a few special cases:
 *
 *		giants get a boulder to throw sometimes.
 *		ettins get clubs
 *		kobolds get darts to throw
 *		centaurs get some sort of bow & arrows or bolts
 *		soldiers get all sorts of things.
 *		kops get clubs & cream pies.
 */
	switch (mtmp->data->mlet) {
	    case S_GIANT:
		if (rn2(2)) (void)mongets(mtmp, (ptr != &mons[PM_ETTIN]) ?
				    BOULDER : CLUB);
		break;
	    case S_HUMAN:
		if(is_mercenary(ptr)) {
		    switch (mm) {

#ifdef ARMY
			case PM_SOLDIER:
			  (void) mongets(mtmp, rn2(2) ? SPEAR : SHORT_SWORD);
			  break;
			case PM_SERGEANT:
			  (void) mongets(mtmp, rn2(2) ? FLAIL : MACE);
			  break;
			case PM_LIEUTENANT:
			  (void) mongets(mtmp, rn2(2) ? GLAIVE : LONG_SWORD);
			  break;
			case PM_CAPTAIN:
			  (void) mongets(mtmp, rn2(2) ? LONG_SWORD : SCIMITAR);
			  break;
#endif
			default:    if (!rn2(4)) (void) mongets(mtmp, DAGGER);
				    if (!rn2(7)) (void) mongets(mtmp, SPEAR);
				    break;
		    }
#ifdef TOLKIEN
		} else if (is_elf(ptr)) {
		    (void)mongets(mtmp,
			rn2(2) ? ELVEN_MITHRIL_COAT : ELVEN_CLOAK);
		    if (rn2(2)) (void)mongets(mtmp, ELVEN_LEATHER_HELM);
		    if (rn2(3)) (void)mongets(mtmp, ELVEN_DAGGER);
		    switch (rn2(3)) {
			case 0:
			    if (!rn2(4)) (void)mongets(mtmp, ELVEN_SHIELD);
			    (void)mongets(mtmp, ELVEN_SHORT_SWORD);
			    (void)mongets(mtmp, ELVEN_BOW);
			    m_initthrow(mtmp, ELVEN_ARROW, 12);
			    break;
			case 1:
			    (void)mongets(mtmp, ELVEN_BROADSWORD);
			    if (rn2(2)) (void)mongets(mtmp, ELVEN_SHIELD);
			    break;
			case 2:
			    (void)mongets(mtmp, ELVEN_SPEAR);
			    (void)mongets(mtmp, ELVEN_SHIELD);
			    break;
		    }
#else /* TOLKIEN */
		} else if (is_elf(ptr)) {
		    (void)mongets(mtmp, ELVEN_CLOAK);
		    if (rn2(3)) (void)mongets(mtmp, DAGGER);
		    switch (rn2(3)) {
			case 0:
			    if (!rn2(4)) (void)mongets(mtmp, SMALL_SHIELD);
			    (void)mongets(mtmp, SHORT_SWORD);
			    (void)mongets(mtmp, BOW);
			    m_initthrow(mtmp, ARROW, 12);
			    break;
			case 1:
			    (void)mongets(mtmp, BROADSWORD);
			    if (rn2(2)) (void)mongets(mtmp, SMALL_SHIELD);
			    break;
			case 2:
			    (void)mongets(mtmp, SPEAR);
			    (void)mongets(mtmp, SMALL_SHIELD);
			    break;
		    }
#endif
		}
		break;

	    case S_HUMANOID:
#ifdef TOLKIEN
		if (mm == PM_HOBBIT) {
		    switch (rn2(3)) {
			case 0:
			    (void)mongets(mtmp, DAGGER);
			    break;
			case 1:
			    (void)mongets(mtmp, ELVEN_DAGGER);
			    break;
			case 2:
			    (void)mongets(mtmp, SLING);
			    break;
		      }
		    if (!rn2(10)) (void)mongets(mtmp, ELVEN_MITHRIL_COAT);
		} else if (is_dwarf(ptr)) {
		    (void)mongets(mtmp, DWARVISH_CLOAK);
		    (void)mongets(mtmp, IRON_SHOES);
		    if (!rn2(4)) {
			(void)mongets(mtmp, DWARVISH_SHORT_SWORD);
			/* note: you can't use a mattock with a shield */
			if (rn2(2)) (void)mongets(mtmp, DWARVISH_MATTOCK);
			else {
				(void)mongets(mtmp, AXE);
				(void)mongets(mtmp, DWARVISH_ROUNDSHIELD);
			}
			(void)mongets(mtmp, DWARVISH_IRON_HELM);
			if (!rn2(3))
			    (void)mongets(mtmp, DWARVISH_MITHRIL_COAT);
		    } else {
			(void)mongets(mtmp, PICK_AXE);
		    }
		}
#else /* TOLKIEN */
		if (is_dwarf(ptr)) {
		    (void)mongets(mtmp, IRON_SHOES);
		    if (rn2(4) == 0) {
			(void)mongets(mtmp, SHORT_SWORD);
			if (rn2(2)) (void)mongets(mtmp, TWO_HANDED_SWORD);
			else {
				(void)mongets(mtmp, AXE);
				(void)mongets(mtmp, LARGE_SHIELD);
			}
			if (rn2(3) == 0)
			    (void)mongets(mtmp, DWARVISH_MITHRIL_COAT);
		    } else {
			(void)mongets(mtmp, PICK_AXE);
		    }
		}
#endif /* TOLKIEN */
		break;
# ifdef KOPS
	    case S_KOP:		/* create Keystone Kops with cream pies to
				 * throw. As suggested by KAA.	   [MRS]
				 */
		if (!rn2(4)) m_initthrow(mtmp, CREAM_PIE, 2);
		if (!rn2(3)) (void)mongets(mtmp, (rn2(2)) ? CLUB : RUBBER_HOSE);
		break;
#endif
	    case S_ORC:
		if(rn2(2)) (void)mongets(mtmp, ORCISH_HELM);
#ifdef TOLKIEN
		switch (mm != PM_ORC_CAPTAIN ? mm :
			rn2(2) ? PM_MORDOR_ORC : PM_URUK_HAI) {
		    case PM_MORDOR_ORC:
			if(rn2(2)) (void)mongets(mtmp, SCIMITAR);
			if(rn2(2)) (void)mongets(mtmp, ORCISH_SHIELD);
			if(rn2(2)) (void)mongets(mtmp, KNIFE);
			if(rn2(2)) (void)mongets(mtmp, ORCISH_CHAIN_MAIL);
			break;
		    case PM_URUK_HAI:
			if(rn2(2)) (void)mongets(mtmp, ORCISH_CLOAK);
			if(rn2(2)) (void)mongets(mtmp, ORCISH_SHORT_SWORD);
			if(rn2(2)) (void)mongets(mtmp, IRON_SHOES);
			if(rn2(2)) {
			    (void)mongets(mtmp, ORCISH_BOW);
			    m_initthrow(mtmp, ORCISH_ARROW, 12);
			}
			if(rn2(2)) (void)mongets(mtmp, URUK_HAI_SHIELD);
			break;
		    default:
			if (mm != PM_ORC_SHAMAN)
			  (void)mongets(mtmp, (mm == PM_GOBLIN || rn2(2) == 0) ?
					      ORCISH_DAGGER : SCIMITAR);
#else /* TOLKIEN */
		switch (mm) {
		    case  PM_ORC_CAPTAIN:
			if(rn2(2)) {
			    if(rn2(2)) (void)mongets(mtmp, SCIMITAR);
			    if(rn2(2)) (void)mongets(mtmp, SMALL_SHIELD);
			    if(rn2(2)) (void)mongets(mtmp, KNIFE);
			    if(rn2(2)) (void)mongets(mtmp, CHAIN_MAIL);
			} else {
			    if(rn2(2)) (void)mongets(mtmp, SHORT_SWORD);
			    if(rn2(2)) (void)mongets(mtmp, IRON_SHOES);
			    if(rn2(2)) {
				(void)mongets(mtmp, BOW);
				m_initthrow(mtmp, ARROW, 12);
			    }
			    if(rn2(2)) (void)mongets(mtmp, SMALL_SHIELD);
			}
		    default:
			if (mm != PM_ORC_SHAMAN)
			  (void)mongets(mtmp, (mm == PM_GOBLIN || rn2(2) == 0) ?
					      DAGGER : SCIMITAR);
#endif /* TOLKIEN */
		}
		break;
	    case S_KOBOLD:
		if (!rn2(4)) m_initthrow(mtmp, DART, 12);
		break;

	    case S_CENTAUR:
		if (rn2(2)) {
		    if(ptr == &mons[PM_FOREST_CENTAUR]) {
			(void)mongets(mtmp, BOW);
			m_initthrow(mtmp, ARROW, 12);
		    } else {
			(void)mongets(mtmp, CROSSBOW);
			m_initthrow(mtmp, CROSSBOW_BOLT, 12);
		    }
		}
		break;
	    case S_WRAITH:
		(void)mongets(mtmp, KNIFE);
		(void)mongets(mtmp, LONG_SWORD);
		break;
	    case S_DEMON:
#ifdef INFERNO
		switch (mm) {
		    case PM_BALROG:
			(void)mongets(mtmp, BULLWHIP);
			(void)mongets(mtmp, BROADSWORD);
			break;
		    case PM_ORCUS:
			(void)mongets(mtmp, WAN_DEATH); /* the Wand of Orcus */
			break;
		    case PM_HORNED_DEVIL:
			(void)mongets(mtmp, rn2(4) ? TRIDENT : BULLWHIP);
			break;
		    case PM_ICE_DEVIL:
			if (!rn2(4)) (void)mongets(mtmp, SPEAR);
			break;
		    case PM_ASMODEUS:
			(void)mongets(mtmp, WAN_COLD);
			break;
		    case PM_DISPATER:
			(void)mongets(mtmp, WAN_STRIKING);
			break;
		    case PM_YEENOGHU:
			(void)mongets(mtmp, FLAIL);
			break;
		}
#endif
		/* prevent djinnis and mail daemons from leaving objects when
		 * they vanish
		 */
		if (!is_demon(ptr)) break;
		/* fall thru */
/*
 *	Now the general case, ~40% chance of getting some type
 *	of weapon. TODO: Add more weapons types (use bigmonst());
 */
	    default:
		switch(rnd(12)) {
		    case 1:
			m_initthrow(mtmp, DART, 12);
			break;
		    case 2:
			(void) mongets(mtmp, CROSSBOW);
			m_initthrow(mtmp, CROSSBOW_BOLT, 12);
			break;
		    case 3:
			(void) mongets(mtmp, BOW);
			m_initthrow(mtmp, ARROW, 12);
			break;
		    case 4:
			m_initthrow(mtmp, DAGGER, 3);
			break;
		    case 5:
			(void) mongets(mtmp, AKLYS);
			break;
		    default:
			break;
		}
		break;
	}
}

#endif /* OVL2 */
#ifdef OVL1

static void
m_initinv(mtmp)
register struct	monst	*mtmp;
{
	register int cnt;
	register struct obj *otmp;
	register struct permonst *ptr = mtmp->data;
#ifdef REINCARNATION
	if (dlevel==rogue_level) return;
#endif
/*
 *	Soldiers get armour & rations - armour approximates their ac.
 *	Nymphs may get mirror or potion of object detection.
 */
	switch(mtmp->data->mlet) {

	    case S_HUMAN:
		if(is_mercenary(ptr)) {
		    register int mac;

		    if((mac = ptr->ac) < -1)
			mac += 7 + mongets(mtmp, (rn2(5)) ?
					   PLATE_MAIL : CRYSTAL_PLATE_MAIL);
		    else if(mac < 3)
			mac += 6 + mongets(mtmp, (rn2(3)) ?
					   SPLINT_MAIL : BANDED_MAIL);
		    else
			mac += 3 + mongets(mtmp, (rn2(3)) ?
					   RING_MAIL : STUDDED_LEATHER_ARMOR);

		    if(mac < 10) {
			mac += 1 + mongets(mtmp, HELMET);
			if(mac < 10) {
			    mac += 1 + mongets(mtmp, SMALL_SHIELD);
			    if(mac < 10) {
				mac += 1 + mongets(mtmp, ELVEN_CLOAK);
				if(mac < 10)
				    mac += 1 +mongets(mtmp, LEATHER_GLOVES);
			    }
			}
		    }

		    if(mac != 10) {	/* make up the difference */
			otmp = mksobj(RIN_PROTECTION,FALSE);
			otmp->spe = (10 - mac);
			if(otmp->spe < 0) curse(otmp);
			mpickobj(mtmp, otmp);
		    }
#ifdef ARMY
		    if(ptr != &mons[PM_GUARD]) {
			if (!rn2(3)) (void) mongets(mtmp, K_RATION);
			if (!rn2(2)) (void) mongets(mtmp, C_RATION);
		    }
#endif
		} else if (ptr == &mons[PM_SHOPKEEPER]) {
		    (void) mongets(mtmp,SKELETON_KEY);
		}
		break;

	    case S_NYMPH:
#ifdef MEDUSA
		if(!rn2(2)) (void) mongets(mtmp, MIRROR);
#endif
		if(!rn2(2)) (void) mongets(mtmp, POT_OBJECT_DETECTION);
		break;

	    case S_GIANT:
		if(mtmp->data == &mons[PM_MINOTAUR])
		    (void) mongets(mtmp, WAN_DIGGING);
		else if (is_giant(mtmp->data)) {
		    for(cnt = rn2((int)(mtmp->m_lev / 2)); cnt; cnt--) {
			    do
				otmp = mkobj(GEM_SYM,FALSE);
			    while (otmp->otyp >= LAST_GEM+6);
			    otmp->quan = 2 + rnd(2);
			    otmp->owt = weight(otmp);
			    mpickobj(mtmp, otmp);
		    }
		}
		break;
#ifdef TOLKIEN
	    case S_WRAITH:
		if(mtmp->data == &mons[PM_NAZGUL]) {
			otmp = mksobj(RIN_INVISIBILITY, FALSE);
			curse(otmp);
			mpickobj(mtmp, otmp);
		}
		break;
#endif
	    default:
		break;
	}
}

/*
 * called with [x,y] = coordinates;
 *	[0,0] means anyplace
 *	[u.ux,u.uy] means: near player (if !in_mklev)
 *
 *	In case we make a monster group, only return the one at [x,y].
 */
struct monst *
makemon(ptr, x, y)
register struct permonst *ptr;
register int	x, y;
{
	register struct monst *mtmp;
	register int	ct;
	boolean anything = (!ptr);
	boolean byyou = (x == u.ux && y == u.uy);

	/* if caller wants random location, do it here */
	if(x == 0 && y == 0) {
		int uroom;
		int tryct = 0;	/* careful with bigrooms */
#ifdef __GNULINT__
		uroom = 0;	/* supress used before set warning */
#endif
		if(!in_mklev) uroom = inroom(u.ux, u.uy);

		do {
			x = rn1(COLNO-3,2);
			y = rn2(ROWNO);
		} while(!goodpos(x, y, ptr) ||
			(!in_mklev && tryct++ < 50 && inroom(x, y) == uroom));
	} else if (byyou && !in_mklev) {
		coord bypos;

		if(enexto(&bypos, u.ux, u.uy, ptr)) {
			x = bypos.x;
			y = bypos.y;
		} else
			return((struct monst *)0);
	}

	/* if a monster already exists at the position, return */
	if(MON_AT(x, y))
		return((struct monst *) 0);

	if(ptr){
		/* if you are to make a specific monster and it has
		   already been genocided, return */
		if(ptr->geno & G_GENOD) return((struct monst *) 0);
	} else {
		/* make a random (common) monster. */
		if(!(ptr = rndmonst()))
		{
#ifdef DEBUG
		    pline("Warning: no monster.");
#endif
		    return((struct monst *) 0);	/* no more monsters! */
		}
	}
	/* if it's unique, don't ever make it again */
	if (ptr->geno & G_UNIQ) ptr->geno |= G_GENOD;
/* gotmon:	/* label not referenced */
	mtmp = newmonst(ptr->pxlth);
	*mtmp = zeromonst;		/* clear all entries in structure */
	for(ct = 0; ct < ptr->pxlth; ct++)
		((char *) &(mtmp->mextra[0]))[ct] = 0;
	mtmp->nmon = fmon;
	fmon = mtmp;
	mtmp->m_id = flags.ident++;
	mtmp->data = ptr;
	mtmp->mxlth = ptr->pxlth;

	mtmp->m_lev = adj_lev(ptr);
#ifdef GOLEMS
	if (is_golem(ptr))
	    mtmp->mhpmax = mtmp->mhp = golemhp(monsndx(ptr));
	else
#endif /* GOLEMS */
	if(ptr->mlevel > 49) {
	    /* "special" fixed hp monster
	     * the hit points are encoded in the mlevel in a somewhat strange
	     * way to fit in the 50..127 positive range of a signed character
	     * above the 1..49 that indicate "normal" monster levels */
	    mtmp->mhpmax = mtmp->mhp = 2*(ptr->mlevel - 6);
	    mtmp->m_lev = mtmp->mhp / 4;	/* approximation */
	} else if((ptr->mlet == S_DRAGON) && (ptr >= &mons[PM_GRAY_DRAGON]))
	    mtmp->mhpmax = mtmp->mhp = 80;
	else if(!mtmp->m_lev) mtmp->mhpmax = mtmp->mhp = rnd(4);
	else mtmp->mhpmax = mtmp->mhp = d((int)mtmp->m_lev, 8);
	place_monster(mtmp, x, y);
	mtmp->mcansee = mtmp->mcanmove = 1;
	mtmp->mpeaceful = peace_minded(ptr);

	switch(ptr->mlet) {
		case S_MIMIC:
			set_mimic_sym(mtmp);
			break;
		case S_SPIDER:
		case S_SNAKE:
			mtmp->mhide = 1;
			if(in_mklev)
			    if(x && y)
				(void) mkobj_at(0, x, y, TRUE);
			if(OBJ_AT(x, y) || levl[x][y].gmask)
			    mtmp->mundetected = 1;
			break;
		case S_STALKER:
		case S_EEL:
			mtmp->minvis = 1;
			break;
		case S_LEPRECHAUN:
			mtmp->msleep = 1;
			break;
		case S_JABBERWOCK:
		case S_NYMPH:
			if(rn2(5) && !u.uhave_amulet) mtmp->msleep = 1;
			break;
		case S_UNICORN:
			if ((ptr==&mons[PM_WHITE_UNICORN] &&
				u.ualigntyp == U_LAWFUL) ||
			(ptr==&mons[PM_GRAY_UNICORN] &&
				u.ualigntyp == U_NEUTRAL) ||
			(ptr==&mons[PM_BLACK_UNICORN] &&
				u.ualigntyp == U_CHAOTIC))
				mtmp->mpeaceful = 1;
			break;
	}
	if (ptr == &mons[PM_CHAMELEON]) {
		/* If you're protected with a ring, don't create
		 * any shape-changing chameleons -dgk
		 */
		if (Protection_from_shape_changers)
			mtmp->cham = 0;
		else {
			mtmp->cham = 1;
			(void) newcham(mtmp, rndmonst());
		}
	} else if (ptr == &mons[PM_WIZARD_OF_YENDOR]) {
		mtmp->iswiz = 1;
		flags.no_of_wizards++;
	} else if (ptr == &mons[PM_QUANTUM_MECHANIC])
		mtmp = qname(mtmp);

	if(in_mklev) {
		if(((is_ndemon(ptr)) ||
		    (ptr == &mons[PM_WUMPUS]) ||
#ifdef WORM
		    (ptr == &mons[PM_LONG_WORM]) ||
#endif
		    (ptr == &mons[PM_GIANT_EEL])) && rn2(5))
			mtmp->msleep = 1;
	} else {
		if(byyou) {
			pmon(mtmp);
			set_apparxy(mtmp);
		}
	}
#ifdef INFERNO
	if(is_dprince(ptr)) {
	    mtmp->mpeaceful = mtmp->minvis = 1;
# ifdef NAMED_ITEMS
	    if(uwep)
		if(!strcmp(ONAME(uwep), "Excalibur"))
		    mtmp->mpeaceful = mtmp->mtame = 0;
# endif
	}
#endif
#ifdef WORM
	if(ptr == &mons[PM_LONG_WORM] && getwn(mtmp))  initworm(mtmp);
#endif
	set_malign(mtmp);		/* having finished peaceful changes */
	if(anything) {
	    if((ptr->geno & G_SGROUP) && rn2(2))
		m_initsgrp(mtmp, mtmp->mx, mtmp->my);
	    else if(ptr->geno & G_LGROUP) {
			if(rn2(3))  m_initlgrp(mtmp, mtmp->mx, mtmp->my);
			else	    m_initsgrp(mtmp, mtmp->mx, mtmp->my);
	    }
	}

	if(is_armed(ptr))
		m_initweap(mtmp);	/* equip with weapons / armour */
	m_initinv(mtmp);		/* add on a few special items */

	return(mtmp);
}

boolean
enexto(cc, xx, yy, mdat)
coord *cc;
register xchar xx, yy;
struct permonst *mdat;
{
	register xchar x,y;
	coord foo[15], *tfoo;
	int range, i;
	int xmin, xmax, ymin, ymax;

	tfoo = foo;
	range = 1;
	do {	/* full kludge action. */
		xmin = max(0, xx-range);
		xmax = min(COLNO, xx+range);
		ymin = max(0, yy-range);
		ymax = min(ROWNO, yy+range);

		for(x = xmin; x <= xmax; x++)
			if(goodpos(x, ymin, mdat)) {
				tfoo->x = x;
				(tfoo++)->y = ymin;
				if(tfoo == &foo[15]) goto foofull;
			}
		for(x = xmin; x <= xmax; x++)
			if(goodpos(x, ymax, mdat)) {
				tfoo->x = x;
				(tfoo++)->y = ymax;
				if(tfoo == &foo[15]) goto foofull;
			}
		for(y = ymin+1; y < ymax; y++)
			if(goodpos(xmin, y, mdat)) {
				tfoo->x = xmin;
				(tfoo++)->y = y;
				if(tfoo == &foo[15]) goto foofull;
			}
		for(y = ymin+1; y < ymax; y++)
			if(goodpos(xmax, y, mdat)) {
				tfoo->x = xmax;
				(tfoo++)->y = y;
				if(tfoo == &foo[15]) goto foofull;
			}
		range++;
		if(range > ROWNO && range > COLNO) return FALSE;
	} while(tfoo == foo);
foofull:
	i = rn2((int)(tfoo - foo));
	cc->x = foo[i].x;
	cc->y = foo[i].y;
	return TRUE;
}

int
goodpos(x, y, mdat)
int x,y;
struct permonst *mdat;
{
	if (x < 1 || x > COLNO-2 || y < 1 || y > ROWNO-2 || MON_AT(x, y))
		return 0;
	if (x == u.ux && y == u.uy) return 0;
	if (mdat) {
	    if (IS_POOL(levl[x][y].typ))
		if (mdat == &playermon && (HLevitation || Wwalking))
			return 1;
		else	return (is_flyer(mdat) || is_swimmer(mdat));
	    if (passes_walls(mdat)) return 1;
	}
	if (!ACCESSIBLE(levl[x][y].typ)) return 0;
	if (closed_door(x, y) && (!mdat || !amorphous(mdat)))
		return 0;
	if (sobj_at(BOULDER, x, y) && (!mdat || !throws_rocks(mdat)))
		return 0;
	return 1;
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL
void
rloc_to(mtmp, x, y)
struct monst *mtmp;
register int x,y;
{
#ifdef WORM		/* do not relocate worms */
	if(mtmp->wormno && mtmp->mx) return;
#endif
	if(mtmp->mx != 0 && mtmp->my != 0)
		remove_monster(mtmp->mx, mtmp->my);
	place_monster(mtmp, x, y);
	if(u.ustuck == mtmp){
		if(u.uswallow) {
			u.ux = x;
			u.uy = y;
			docrt();
		} else	u.ustuck = 0;
	}
	pmon(mtmp);
	set_apparxy(mtmp);
}

#endif /* OVLB */
#ifdef OVL2

void
rloc(mtmp)
struct monst *mtmp;
{
	register int x, y;

	/* if the wiz teleports away to heal, try the up staircase,
	   to block the player's escaping before he's healed */
	if(!mtmp->iswiz || !goodpos(x = xupstair, y = yupstair, mtmp->data))
	   do {
		x = rn1(COLNO-3,2);
		y = rn2(ROWNO);
	   } while(!goodpos(x,y,mtmp->data));
	rloc_to(mtmp, x, y);
}

#endif /* OVL2 */
#ifdef OVLB

void
vloc(mtmp)
struct monst *mtmp;
{
	register struct mkroom *croom;
	register int x, y;

	for(croom = &rooms[0]; croom->hx >= 0; croom++)
	    if(croom->rtype == VAULT) {
		x = rn2(2) ? croom->lx : croom->hx;
		y = rn2(2) ? croom->ly : croom->hy;
		if(goodpos(x, y, mtmp->data)) {
		    rloc_to(mtmp, x, y);
		    return;
		}
	    }
	rloc(mtmp);
}

#endif /* OVLB */
#ifdef OVL0

static int
cmnum()	{	/* return the number of "common" monsters */

	int	i, count;

	for(i = count = 0; mons[i].mlet; i++)
	   if(!uncommon(&mons[i]))  count++;

	return(count);
}

static int
uncommon(ptr)
struct	permonst *ptr;
{
	return (ptr->geno & (G_GENOD | G_NOGEN | G_UNIQ)) ||
		(!Inhell ? ptr->geno & G_HELL : ptr->maligntyp > 0);
}

#endif /* OVL0 */
#ifdef OVL1

/* This routine is designed to return an integer value which represents
 * an approximation of monster strength.  It uses a similar method of
 * determination as "experience()" to arrive at the strength.
 */
static int
mstrength(ptr)
struct permonst *ptr;
{
	int	i, tmp2, n, tmp = ptr->mlevel;

	if(tmp > 49)		/* special fixed hp monster */
	    tmp = 2*(tmp - 6) / 4;

/*	For creation in groups */
	n = (!!(ptr->geno & G_SGROUP));
	n += (!!(ptr->geno & G_LGROUP)) << 1;

/*	For ranged attacks */
	if (ranged_attk(ptr)) n++;

/*	For higher ac values */
	n += (ptr->ac < 0);

/*	For very fast monsters */
	n += (ptr->mmove >= 18);

/*	For each attack and "special" attack */
	for(i = 0; i < NATTK; i++) {

	    tmp2 = ptr->mattk[i].aatyp;
	    n += (tmp2 > 0);
	    n += (tmp2 == AT_MAGC);
	    n += (tmp2 == AT_WEAP && strongmonst(ptr));
	}

/*	For each "special" damage type */
	for(i = 0; i < NATTK; i++) {

	    tmp2 = ptr->mattk[i].adtyp;
	    if((tmp2 == AD_DRLI) || (tmp2 == AD_STON)
#ifdef POLYSELF
					|| (tmp2 == AD_WERE)
#endif
								) n += 2;
	    else if (ptr != &mons[PM_GRID_BUG]) n += (tmp2 != AD_PHYS);
	    n += ((ptr->mattk[i].damd * ptr->mattk[i].damn) > 23);
	}

/*	Leprechauns are special cases.  They have many hit dice so they
	can hit and are hard to kill, but they don't really do much damage. */
	if (ptr == &mons[PM_LEPRECHAUN]) n -= 2;

/*	Finally, adjust the monster level  0 <= n <= 24 (approx.) */
	if(n == 0) tmp--;
	else if(n >= 6) tmp += ( n / 2 );
	else tmp += ( n / 3 + 1);

	return((tmp >= 0) ? tmp : 0);
}

void
init_monstr()
{
	register int ct;

	for(ct = 0; mons[ct].mlet; ct++)
		monstr[ct] = mstrength(&(mons[ct]));
}

#endif /* OVL1 */
#ifdef OVL0

struct	permonst *
rndmonst()		/* select a random monster */
{
	register struct permonst *ptr;
	register int i, ct;
	register int zlevel;
	static int NEARDATA minmlev, NEARDATA maxmlev, NEARDATA accept;
	static long NEARDATA oldmoves = 0L;	/* != 1, starting value of moves */
#ifdef REINCARNATION
	static boolean NEARDATA upper;

	upper = (dlevel == rogue_level);
#endif

#ifdef __GNULINT__
	ptr = (struct permonst *)0; /* suppress "used uninitialized" warning */
#endif
	if(oldmoves != moves) {		/* must recalculate accept */
	    oldmoves = moves;
	    zlevel = u.uhave_amulet ? MAXLEVEL : dlevel;
	    if(cmnum() <= 0) {
#ifdef DEBUG
		pline("cmnum() fails!");
#endif
		return((struct permonst *) 0);
	    }

	    /* determine the level of the weakest monster to make. */
	    minmlev = zlevel/6;
	    /* determine the level of the strongest monster to make. */
	    maxmlev = (zlevel + u.ulevel)>>1;
/*
 *	Find out how many monsters exist in the range we have selected.
 */
	    accept = 0;
#ifdef REINCARNATION
	    for(ct = (upper ? PM_APE : 0);
			upper ? isupper(mons[ct].mlet) : mons[ct].mlet; ct++) {
#else
	    for(ct = 0 ; mons[ct].mlet; ct++) {
#endif
		ptr = &(mons[ct]);
		if(uncommon(ptr)) continue;
		if(tooweak(ct, minmlev) || toostrong(ct, maxmlev))
		    continue;
		accept += (ptr->geno & G_FREQ);
	    }
	}

	if(!accept) {
#ifdef DEBUG
		pline("no accept!");
#endif
		return((struct permonst *) 0);
	}
/*
 *	Now, select a monster at random.
 */
	ct = rnd(accept);
#ifdef REINCARNATION
	for(i = (upper ? PM_APE : 0);
	    (upper ? isupper(mons[i].mlet) : mons[i].mlet) && ct > 0; i++) {
#else
	for(i = 0; mons[i].mlet && ct > 0; i++) {
#endif
		ptr = &(mons[i]);
		if(uncommon(ptr)) continue;
		if(tooweak(i, minmlev) || toostrong(i, maxmlev))
		    continue;
		ct -= (ptr->geno & G_FREQ);
	}
	if(ct > 0) {
#ifdef DEBUG
		pline("no count!");
#endif
		return((struct permonst *) 0);
	}
	return(ptr);
}

#endif /* OVL0 */
#ifdef OVL1

/*	The routine below is used to make one of the multiple types
 *	of a given monster class.  It will return 0 if no monsters
 *	in that class can be made.
 */

struct permonst *
mkclass(mlet)
char	mlet;
{
	register int	first, last, num = 0;

	if(!mlet) {
	    impossible("mkclass called with null arg!");
	    return((struct permonst *) 0);
	}
/*	Assumption #1:	monsters of a given class are contiguous in the
 *			mons[] array.
 */
	for(first = 0; mons[first].mlet != mlet; first++)
		if(!mons[first].mlet)	return((struct permonst *) 0);

	for(last = first; mons[last].mlet && mons[last].mlet == mlet; last++)
	    if(!(mons[last].geno & (G_GENOD | G_NOGEN | G_UNIQ)))
		num += mons[last].geno & G_FREQ;

	if(!num) return((struct permonst *) 0);

/*	Assumption #2:	monsters of a given class are presented in ascending
 *			order of strength.
 */
	for(num = rnd(num); num > 0; first++)
	    if(!(mons[first].geno & (G_GENOD | G_NOGEN | G_UNIQ))) { /* consider it */
		/* skew towards lower value monsters at lower exp. levels */
		if(adj_lev(&mons[first]) > (u.ulevel*2)) num--;
		num -= mons[first].geno & G_FREQ;
	    }
	first--; /* correct an off-by-one error */

	return(&mons[first]);
}

int
adj_lev(ptr)	/* adjust strength of monsters based on dlevel and u.ulevel */
register struct permonst *ptr;
{
	int	tmp, tmp2;

	if((tmp = ptr->mlevel) > 49) return(50); /* "special" demons/devils */
	tmp2 = (dlevel - tmp);
	if(tmp2 < 0) tmp--;		/* if mlevel > dlevel decrement tmp */
	else tmp += (tmp2 / 5);		/* else increment 1 per five diff */

	tmp2 = (u.ulevel - ptr->mlevel);	/* adjust vs. the player */
	if(tmp2 > 0) tmp += (tmp2 / 4);		/* level as well */

	tmp2 = 3 * ptr->mlevel/ 2;		/* crude upper limit */
	return((tmp > tmp2) ? tmp2 : (tmp > 0 ? tmp : 0)); /* 0 lower limit */
}

#endif /* OVL1 */
#ifdef OVLB

struct permonst *
grow_up(mtmp)		/* mon mtmp "grows up" to a bigger version. */
register struct monst *mtmp;
{
	register int newtype;
	register struct permonst *ptr = mtmp->data;

	if (ptr->mlevel >= 50 || mtmp->mhpmax <= 8*mtmp->m_lev)
	    return ptr;
	newtype = little_to_big(monsndx(ptr));
	if (++mtmp->m_lev >= mons[newtype].mlevel
					&& newtype != monsndx(ptr)) {
		if (mons[newtype].geno & G_GENOD) {
			pline("As %s grows up into %s, %s dies!",
				mon_nam(mtmp),
				an(mons[newtype].mname),
				mon_nam(mtmp));
			mondied(mtmp);
			return (struct permonst *)0;
		}
		mtmp->data = &mons[newtype];
		mtmp->m_lev = mons[newtype].mlevel;
	}
	if (mtmp->m_lev > 3*mtmp->data->mlevel / 2)
		mtmp->m_lev = 3*mtmp->data->mlevel / 2;
	if (mtmp->mhp > mtmp->m_lev * 8)
		mtmp->mhp = mtmp->m_lev * 8;
	return(mtmp->data);
}

#endif /* OVLB */
#ifdef OVL1

int
mongets(mtmp, otyp)
register struct monst *mtmp;
register int otyp;
{
	register struct obj *otmp;

	if((otmp = (otyp) ? mksobj(otyp,FALSE) : mkobj(otyp,FALSE))) {
	    if (mtmp->data->mlet == S_DEMON) {
		/* demons always get cursed objects */
		curse(otmp);
	    }
	    mpickobj(mtmp, otmp);
	    return(otmp->spe);
	} else return(0);
}

#endif /* OVL1 */
#ifdef OVLB

#ifdef GOLEMS
int
golemhp(type)
int type;
{
	switch(type) {
		case PM_STRAW_GOLEM: return 20;
		case PM_ROPE_GOLEM: return 30;
		case PM_LEATHER_GOLEM: return 40;
		case PM_WOOD_GOLEM: return 50;
		case PM_FLESH_GOLEM: return 40;
		case PM_CLAY_GOLEM: return 50;
		case PM_STONE_GOLEM: return 60;
		case PM_IRON_GOLEM: return 80;
		default: return 0;
	}
}
#endif /* GOLEMS */

#endif /* OVLB */
#ifdef OVL1

/*
 *	Alignment vs. yours determines monster's attitude to you.
 *	( some "animal" types are co-aligned, but also hungry )
 */
boolean
peace_minded(ptr)
register struct permonst *ptr;
{
	schar mal = ptr->maligntyp, ual = u.ualigntyp;

	if (always_peaceful(ptr)) return TRUE;
	if (always_hostile(ptr)) return FALSE;

	/* the monster is hostile if its alignment is different from the
	 * player's */
	if (sgn(mal) != sgn(ual)) return FALSE;

	/* Negative monster hostile to player with Amulet. */
	if (mal < 0 && u.uhave_amulet) return FALSE;

	/* Last case:  a chance of a co-aligned monster being
	 * hostile.  This chance is greater if the player has strayed
	 * (u.ualign negative) or the monster is not strongly aligned.
	 */
	return !!rn2(16 + (u.ualign < -15 ? -15 : u.ualign)) &&
		!!rn2(2 + abs(mal));
}

/* Set malign to have the proper effect on player alignment if monster is
 * killed.  Negative numbers mean it's bad to kill this monster; positive
 * numbers mean it's good.  Since there are more hostile monsters than
 * peaceful monsters, the penalty for killing a peaceful monster should be
 * greater than the bonus for killing a hostile monster to maintain balance.
 * Rules:
 *   it's bad to kill peaceful monsters, potentially worse to kill always-
 *	peaceful monsters
 *   it's never bad to kill a hostile monster, although it may not be good
 */
void
set_malign(mtmp)
struct monst *mtmp;
{
	schar mal = mtmp->data->maligntyp;
	boolean coaligned = (sgn(mal) == sgn(u.ualigntyp));

	if (always_peaceful(mtmp->data))
		mtmp->malign = -3*max(5,abs(mal));
	else if (always_hostile(mtmp->data)) {
		if (coaligned)
			mtmp->malign = 0;
		else
			mtmp->malign = max(5,abs(mal));
	} else if (coaligned) {
		if (mtmp->mpeaceful)
			mtmp->malign = -3*max(3,abs(mal));
		else	/* renegade */
			mtmp->malign = max(3,abs(mal));
	} else	/* not coaligned and therefore hostile */
		mtmp->malign = abs(mal);
}

#endif /* OVL1 */
#ifdef OVLB

static char NEARDATA syms[] = { 0, 1, RING_SYM, WAND_SYM, WEAPON_SYM, FOOD_SYM, GOLD_SYM,
	SCROLL_SYM, POTION_SYM, ARMOR_SYM, AMULET_SYM, TOOL_SYM, ROCK_SYM,
	GEM_SYM,
#ifdef SPELLS
	SPBOOK_SYM,
#endif
	S_MIMIC_DEF, S_MIMIC_DEF, S_MIMIC_DEF,
};

void
set_mimic_sym(mtmp)		/* KAA, modified by ERS */
register struct monst *mtmp;
{
	int roomno, rt;
	unsigned appear, ap_type;
	int s_sym;
	struct obj *otmp;
	int mx, my;

	if (!mtmp) return;
	mx = mtmp->mx; my = mtmp->my;
	mtmp->mimic = 1;
	roomno = inroom(mx, my);
	if (levl[mx][my].gmask) {
		ap_type = M_AP_GOLD;
		if (g_at(mx, my)->amount <= 32767)
			appear = g_at(mx, my)->amount;
		else
			appear = 32000 + rnd(767);
	}
	else if (OBJ_AT(mx, my)) {
		ap_type = M_AP_OBJECT;
		appear = level.objects[mx][my]->otyp;
	}
	else if (IS_DOOR(levl[mx][my].typ) ||
		 IS_WALL(levl[mx][my].typ) ||
		 levl[mx][my].typ == SDOOR ||
		 levl[mx][my].typ == SCORR) {
		ap_type = M_AP_FURNITURE;
		appear = S_cdoor;
	}
	else if (is_maze_lev && rn2(2)) {
		ap_type = M_AP_OBJECT;
		appear = STATUE;
	}
	else if (roomno < 0) {
		ap_type = M_AP_OBJECT;
		appear = BOULDER;
	}
	else if ((rt = rooms[roomno].rtype) == ZOO || rt == VAULT) {
		ap_type = M_AP_GOLD;
		appear = rn2(100)+10;	/* number of gold pieces in pile */
	}
#ifdef ORACLE
	else if (rt == DELPHI) {
		if (rn2(2)) {
			ap_type = M_AP_OBJECT;
			appear = STATUE;
		}
		else {
			ap_type = M_AP_FURNITURE;
			appear = S_fountain;
		}
	}
#endif
#ifdef ALTARS
	else if (rt == TEMPLE) {
		ap_type = M_AP_FURNITURE;
		appear = S_altar;
	}
#endif
	/* We won't bother with beehives, morgues, barracks, throne rooms
	 * since they shouldn't contain too many mimics anyway...
	 */
	else if (rt >= SHOPBASE) {
		s_sym = get_shop_item(rt - SHOPBASE);
		if (s_sym < 0) {
			ap_type = M_AP_OBJECT;
			appear = -s_sym;
		}
		else {
			if (s_sym == RANDOM_SYM)
				s_sym = syms[rn2(sizeof(syms)-2) + 2];
			goto assign_sym;
		}
	}
	else {
		s_sym = syms[rn2(sizeof syms)];
assign_sym:
		if (s_sym < 2) {
			ap_type = M_AP_FURNITURE;
			appear = s_sym ? S_upstair : S_dnstair;
		}
		else if (s_sym == GOLD_SYM) {
			ap_type = M_AP_GOLD;
			appear = rn2(100)+100;
		}
		else {
			ap_type = M_AP_OBJECT;
			if (s_sym == S_MIMIC_DEF)
				appear = STRANGE_OBJECT;
			else {
				otmp = mkobj( (char) s_sym, FALSE );
				appear = otmp->otyp;
				free((genericptr_t) otmp);
			}
		}
	}
	mtmp->m_ap_type = ap_type;
	mtmp->mappearance = appear;
}

#endif /* OVLB */
