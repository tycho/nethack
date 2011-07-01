/*	SCCS Id: @(#)dog.c	3.2	96/02/11	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"

#ifdef OVLB

static int NDECL(pet_type);

void
initedog(mtmp)
register struct monst *mtmp;
{
	mtmp->mtame = is_domestic(mtmp->data) ? 10 : 5;
	mtmp->mpeaceful = 1;
	set_malign(mtmp); /* recalc alignment now that it's tamed */
	mtmp->mleashed = 0;
	mtmp->meating = 0;
	EDOG(mtmp)->droptime = 0;
	EDOG(mtmp)->dropdist = 10000;
	EDOG(mtmp)->apport = 10;
	EDOG(mtmp)->whistletime = 0;
	EDOG(mtmp)->hungrytime = 1000 + moves;
	EDOG(mtmp)->ogoal.x = -1;	/* force error if used before set */
	EDOG(mtmp)->ogoal.y = -1;
}

static int
pet_type()
{
	register int pettype;

	switch (u.role) {
		/* some character classes have restricted ideas of pets */
		case 'C':
		case 'S':
			pettype = PM_LITTLE_DOG;
			break;
		case 'W':
			pettype = PM_KITTEN;
			break;
		/* otherwise, see if the player has a preference */
		default:
			if (preferred_pet == 'c')
				pettype = PM_KITTEN;
			else if (preferred_pet == 'd')
				pettype = PM_LITTLE_DOG;
			else	pettype = rn2(2) ? PM_KITTEN : PM_LITTLE_DOG;
			break;
	}
	return pettype;
}

void
make_familiar(otmp,x,y)
register struct obj *otmp;
xchar x, y;
{
	struct permonst *pm;
	struct monst *mtmp = 0;
	int chance, trycnt = 100;

	do {
	    if (otmp) {
		pm = &mons[otmp->corpsenm]; /* Figurine; otherwise spell */
	    } else if (!rn2(3)) {
		pm = &mons[pet_type()];
	    } else {
		pm = rndmonst();
		if (!pm) {
		    pline("There seems to be nothing available for a familiar.");
		    break;
		}
	    }

	    pm->pxlth += sizeof (struct edog);
	    mtmp = makemon(pm, x, y, NO_MM_FLAGS);
	    pm->pxlth -= sizeof (struct edog);
	    if (otmp && !mtmp) { /* monster was genocided or square occupied */
		pline_The("figurine writhes and then shatters into pieces!");
		break;
	    }
	} while (!mtmp && --trycnt > 0);

	if (!mtmp) return;

	initedog(mtmp);
	mtmp->msleep = 0;
	if (otmp) { /* figurine; resulting monster might not become a pet */
	    chance = rn2(10);	/* 0==tame, 1==peaceful, 2==hostile */
	    if (chance > 2) chance = otmp->blessed ? 0 : !otmp->cursed ? 1 : 2;
	    /* 0,1,2:  b=80%,10,10; nc=10%,80,10; c=10%,10,80 */
	    if (chance > 0) {
		mtmp->mtame = 0;	/* not tame after all */
		if (chance == 2) { /* hostile (cursed figurine) */
		    You("get a bad feeling about this.");
		    mtmp->mpeaceful = 0;
		}
	    }
	}
	set_malign(mtmp); /* more alignment changes */
	newsym(mtmp->mx, mtmp->my);

	/* must wield weapon immediately since pets will otherwise drop it */
	if (mtmp->mtame && attacktype(mtmp->data, AT_WEAP)) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp);
	}
}

struct monst *
makedog()
{
	register struct monst *mtmp;
	const char *petname;
	int   pettype;
	static int petname_used = 0;

	pettype = pet_type();
	if (pettype == PM_LITTLE_DOG)
		petname = dogname;
	else
		petname = catname;

	/* default pet names */
	if (!*petname) {
	    if(Role_is('C')) petname = "Slasher";   /* The Warrior */
	    if(Role_is('S')) petname = "Hachi";     /* Shibuya Station */
	    if(Role_is('B')) petname = "Idefix";    /* Obelix */
	}

	mons[pettype].pxlth = sizeof(struct edog);
	mtmp = makemon(&mons[pettype], u.ux, u.uy, NO_MM_FLAGS);
	mons[pettype].pxlth = 0;

	if(!mtmp) return((struct monst *) 0); /* pets were genocided */

	if (!petname_used++ && *petname)
		mtmp = christen_monst(mtmp, petname);

	initedog(mtmp);
	return(mtmp);
}

void
losedogs()
{
	register struct monst *mtmp, *mtmp0 = 0, *mtmp2;

	while ((mtmp = mydogs) != 0) {
		mydogs = mtmp->nmon;
		mon_arrive(mtmp, TRUE);
	}

	for(mtmp = migrating_mons; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (mtmp->mux == u.uz.dnum && mtmp->muy == u.uz.dlevel) {
		    if(mtmp == migrating_mons)
			migrating_mons = mtmp->nmon;
		    else
			mtmp0->nmon = mtmp->nmon;
		    mon_arrive(mtmp, FALSE);
		} else
		    mtmp0 = mtmp;
	}
}

/* called from resurrect() in addition to losedogs() */
void
mon_arrive(mtmp, with_you)
struct monst *mtmp;
boolean with_you;
{
	struct trap *t;
	xchar xlocale, ylocale, xyloc, xyflags, wander;
	int num_segs;

	mtmp->nmon = fmon;
	fmon = mtmp;
	if (mtmp->isshk)
	    set_residency(mtmp, FALSE);

	num_segs = mtmp->wormno;
	/* baby long worms have no tail so don't use is_longworm() */
	if ((mtmp->data == &mons[PM_LONG_WORM]) &&
#ifdef DCC30_BUG
	    (mtmp->wormno = get_wormno(), mtmp->wormno != 0)) {
#else
	    (mtmp->wormno = get_wormno()) != 0) {
#endif
	    initworm(mtmp, num_segs);
	    /* tail segs are not yet initialized or displayed */
	} else mtmp->wormno = 0;

	/* some monsters might need to do something special upon arrival
	   _after_ the current level has been fully set up; see dochug() */
	mtmp->mstrategy |= STRAT_ARRIVE;

	if (with_you) {
	    mnexto(mtmp);
	    return;
	}
	/*
	 * The monster arrived on this level independently of the player.
	 * Its coordinate fields were overloaded for use as flags that
	 * specify its final destination.
	 */

	xyloc	= mtmp->mtrack[0].x;
	xyflags = mtmp->mtrack[0].y;
	xlocale = mtmp->mtrack[1].x;
	ylocale = mtmp->mtrack[1].y;
	mtmp->mtrack[0].x = mtmp->mtrack[0].y = 0;
	mtmp->mtrack[1].x = mtmp->mtrack[1].y = 0;
	mtmp->mux = u.ux,  mtmp->muy = u.uy;	/* not really req'd */

	if (mtmp->mlstmv < monstermoves - 1L) {
	    /* heal monster for time spent in limbo */
	    long nmv = monstermoves - 1L - mtmp->mlstmv;

	    mon_catchup_elapsed_time(mtmp, nmv);
	    mtmp->mlstmv = monstermoves - 1L;

	    /* let monster move a bit on new level (see placement code below) */
	    wander = (xchar) min(nmv, 8);
	} else
	    wander = 0;

	switch (xyloc) {
	 case MIGR_APPROX_XY:	/* {x,y}locale set above */
		break;
	 case MIGR_EXACT_XY:	wander = 0;
		break;
	 case MIGR_NEAR_PLAYER:	xlocale = u.ux,  ylocale = u.uy;
		break;
	 case MIGR_STAIRS_UP:	xlocale = xupstair,  ylocale = yupstair;
		break;
	 case MIGR_STAIRS_DOWN:	xlocale = xdnstair,  ylocale = ydnstair;
		break;
	 case MIGR_LADDER_UP:	xlocale = xupladder,  ylocale = yupladder;
		break;
	 case MIGR_LADDER_DOWN:	xlocale = xdnladder,  ylocale = ydnladder;
		break;
	 case MIGR_SSTAIRS:	xlocale = sstairs.sx,  ylocale = sstairs.sy;
		break;
	 case MIGR_PORTAL:
		for (t = ftrap; t; t = t->ntrap)
		    if (t->ttyp == MAGIC_PORTAL) break;
		if (t) {
		    xlocale = t->tx,  ylocale = t->ty;
		    break;
		} else {
		    if (!In_endgame(&u.uz))
			impossible("mon_arrive: no corresponding portal?");
		} /*FALLTHRU*/
	 default:
	 case MIGR_RANDOM:	xlocale = ylocale = 0;
		    break;
	    }

	if (xlocale && wander) {
	    /* monster moved a bit; pick a nearby location */
	    /* mnearto() deals w/stone, et al */
	    char *r = in_rooms(xlocale, ylocale, 0);
	    if (r && *r) {
		coord c;
		/* somexy() handles irregular rooms */
		if (somexy(&rooms[*r - ROOMOFFSET], &c))
		    xlocale = c.x,  ylocale = c.y;
		else
		    xlocale = ylocale = 0;
	    } else {		/* not in a room */
		int i, j;
		i = max(1, xlocale - wander);
		j = min(COLNO-1, xlocale + wander);
		xlocale = rn1(j - i, i);
		i = max(0, ylocale - wander);
		j = min(ROWNO-1, ylocale + wander);
		ylocale = rn1(j - i, i);
	    }
	}	/* moved a bit */

	mtmp->mx = 0;	/*(already is 0)*/
	mtmp->my = xyflags;
	if (xlocale)
	    (void) mnearto(mtmp, xlocale, ylocale, FALSE);
	else
	    rloc(mtmp);
}

/* heal monster for time spent elsewhere */
void
mon_catchup_elapsed_time(mtmp, nmv)
struct monst *mtmp;
long nmv;		/* number of moves */
{
	int imv = 0;	/* avoid zillions of casts and lint warnings */

#if defined(DEBUG) || defined(BETA)
	if (nmv < 0L) {			/* crash likely... */
	    panic("catchup from future time?");
	    /*NOTREACHED*/
	    return;
	} else if (nmv == 0L) {		/* safe, but should'nt happen */
	    impossible("catchup from now?");
	} else
#endif
	if (nmv >= LARGEST_INT)		/* paranoia */
	    imv = LARGEST_INT - 1;
	else
	    imv = (int)nmv;

	/* might stop being afraid, blind or frozen */
	/* set to 1 and allow final decrement in movemon() */
	if (mtmp->mblinded) {
	    if (imv >= (int) mtmp->mblinded) mtmp->mblinded = 1;
	    else mtmp->mblinded -= imv;
	}
	if (mtmp->mfrozen) {
	    if (imv >= (int) mtmp->mfrozen) mtmp->mfrozen = 1;
	    else mtmp->mfrozen -= imv;
	}
	if (mtmp->mfleetim) {
	    if (imv >= (int) mtmp->mfleetim) mtmp->mfleetim = 1;
	    else mtmp->mfleetim -= imv;
	}

	/* might recover from temporary trouble */
	if (mtmp->mtrapped && rn2(imv + 1) > 40/2) mtmp->mtrapped = 0;
	if (mtmp->mconf    && rn2(imv + 1) > 50/2) mtmp->mconf = 0;
	if (mtmp->mstun    && rn2(imv + 1) > 10/2) mtmp->mstun = 0;

	/* might finish eating or be able to use special ability again */
	if (imv > mtmp->meating) mtmp->meating = 0;
	else mtmp->meating -= imv;
	if (imv > mtmp->mspec_used) mtmp->mspec_used = 0;
	else mtmp->mspec_used -= imv;

	/* reduce tameness for every 150 moves you are separated */
	if (mtmp->mtame) {
	    int wilder = (imv + 75) / 150;
	    if (mtmp->mtame > wilder) mtmp->mtame -= wilder;	/* less tame */
	    else if (mtmp->mtame > rn2(wilder)) mtmp->mtame = 0;  /* untame */
	    else mtmp->mtame = mtmp->mpeaceful = 0;		/* hostile! */
	}

	/* recover lost hit points */
	if (!regenerates(mtmp->data)) imv /= 20;
	if (mtmp->mhp + imv >= mtmp->mhpmax)
	    mtmp->mhp = mtmp->mhpmax;
	else mtmp->mhp += imv;
}

#endif /* OVLB */
#ifdef OVL2

/* called when you move to another level */
void
keepdogs(pets_only)
boolean pets_only;	/* true for ascension or final escape */
{
	register struct monst *mtmp, *mtmp2;
	register struct obj *obj;
	int num_segs;
	boolean stay_behind;

	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;
	    if (pets_only && !mtmp->mtame) continue;
	    if (((monnear(mtmp, u.ux, u.uy) && levl_follower(mtmp)) ||
		/* the wiz will level t-port from anywhere to chase
		   the amulet; if you don't have it, will chase you
		   only if in range. -3. */
			(u.uhave.amulet && mtmp->iswiz))
			&& !mtmp->msleep && mtmp->mcanmove) {
		stay_behind = FALSE;
		if (mtmp->mtame && mtmp->meating) {
			if (canseemon(mtmp))
			    pline("%s is still eating.", Monnam(mtmp));
			stay_behind = TRUE;
		} else if (mon_has_amulet(mtmp)) {
			if (canseemon(mtmp))
			    pline("%s seems very disoriented for a moment.",
				Monnam(mtmp));
			stay_behind = TRUE;
		}
		if (stay_behind) {
			if (mtmp->mleashed) {
				pline("%s leash suddenly comes loose.",
					humanoid(mtmp->data)
					    ? (mtmp->female ? "Her" : "His")
					    : "Its");
				m_unleash(mtmp);
			}
			continue;
		}
		if (mtmp->isshk)
			set_residency(mtmp, TRUE);

		if (mtmp->wormno) {
		    register int cnt;
		    /* NOTE: worm is truncated to # segs = max wormno size */
		    cnt = count_wsegs(mtmp);
		    num_segs = min(cnt, MAX_NUM_WORMS - 1);
		    wormgone(mtmp);
		} else num_segs = 0;

		/* set minvent's obj->no_charge to 0 */
		for(obj = mtmp->minvent; obj; obj = obj->nobj) {
		    if (Has_contents(obj))
			picked_container(obj);	/* does the right thing */
		    obj->no_charge = 0;
		}

		relmon(mtmp);
		newsym(mtmp->mx,mtmp->my);
		mtmp->mx = mtmp->my = 0; /* avoid mnexto()/MON_AT() problem */
		mtmp->wormno = num_segs;
		mtmp->nmon = mydogs;
		mydogs = mtmp;
	    } else if (mtmp->iswiz) {
		/* we want to be able to find him when his next resurrection
		   chance comes up, but have him resume his present location
		   if player returns to this level before that time */
		migrate_to_level(mtmp, ledger_no(&u.uz),
				 MIGR_EXACT_XY, (coord *)0);
	    } else if (mtmp->mleashed) {
		/* this can happen if your quest leader ejects you from the
		   "home" level while a leashed pet isn't next to you */
		pline("%s leash goes slack.", s_suffix(Monnam(mtmp)));
		m_unleash(mtmp);
	    }
	}
}

#endif /* OVL2 */
#ifdef OVLB

void
migrate_to_level(mtmp, tolev, xyloc, cc)
	register struct monst *mtmp;
	xchar tolev;	/* destination level */
	xchar xyloc;	/* MIGR_xxx destination xy location: */
	coord *cc;	/* optional destination coordinates */
{
	register struct obj *obj;
	d_level new_lev;
	xchar xyflags;
	int num_segs = 0;	/* count of worm segments */

	if (mtmp->isshk)
	    set_residency(mtmp, TRUE);

	if (mtmp->wormno) {
	    register int cnt;
	  /* **** NOTE: worm is truncated to # segs = max wormno size **** */
	    cnt = count_wsegs(mtmp);
	    num_segs = min(cnt, MAX_NUM_WORMS - 1);
	    wormgone(mtmp);
	}

	/* set minvent's obj->no_charge to 0 */
	for(obj = mtmp->minvent; obj; obj = obj->nobj) {
	    if (Has_contents(obj))
		picked_container(obj);	/* does the right thing */
	    obj->no_charge = 0;
	}

	relmon(mtmp);
	mtmp->nmon = migrating_mons;
	migrating_mons = mtmp;
	if (mtmp->mleashed)  {
		m_unleash(mtmp);
		mtmp->mtame--;
		pline_The("leash comes off!");
	}
	newsym(mtmp->mx,mtmp->my);

	new_lev.dnum = ledger_to_dnum((xchar)tolev);
	new_lev.dlevel = ledger_to_dlev((xchar)tolev);
	/* overload mtmp->[mx,my], mtmp->[mux,muy], and mtmp->mtrack[] as */
	/* destination codes (setup flag bits before altering mx or my) */
	xyflags = (depth(&new_lev) < depth(&u.uz));	/* 1 => up */
	if (In_W_tower(mtmp->mx, mtmp->my, &u.uz)) xyflags |= 2;
	mtmp->wormno = num_segs;
	mtmp->mlstmv = monstermoves;
	mtmp->mtrack[1].x = cc ? cc->x : mtmp->mx;
	mtmp->mtrack[1].y = cc ? cc->y : mtmp->my;
	mtmp->mtrack[0].x = xyloc;
	mtmp->mtrack[0].y = xyflags;
	mtmp->mux = new_lev.dnum;
	mtmp->muy = new_lev.dlevel;
	mtmp->mx = mtmp->my = 0;	/* this implies migration */
}

#endif /* OVLB */
#ifdef OVL1

/* return quality of food; the lower the better */
/* fungi will eat even tainted food */
int
dogfood(mon,obj)
struct monst *mon;
register struct obj *obj;
{
	boolean carni = carnivorous(mon->data);
	boolean herbi = herbivorous(mon->data);
	struct permonst *fptr = &mons[obj->corpsenm];

	if (is_quest_artifact(obj) || obj_resists(obj, 0, 95))
	    return (obj->cursed ? TABU : APPORT);

	switch(obj->oclass) {
	case FOOD_CLASS:
	    if (obj->otyp == CORPSE &&
		((obj->corpsenm == PM_COCKATRICE && !resists_ston(mon))
		 || is_rider(fptr)))
		    return TABU;

	    if (!carni && !herbi)
		    return (obj->cursed ? UNDEF : APPORT);

	    switch (obj->otyp) {
		case TRIPE_RATION:
		    return (carni ? DOGFOOD : MANFOOD);
		case EGG:
		    if (obj->corpsenm == PM_COCKATRICE && !resists_ston(mon))
			return POISON;
		    return (carni ? CADAVER : MANFOOD);
		case CORPSE:
		   if ((peek_at_iced_corpse_age(obj)+50 <= monstermoves
					    && obj->corpsenm != PM_LIZARD
					    && mon->data->mlet != S_FUNGUS) ||
			(acidic(&mons[obj->corpsenm]) && !resists_acid(mon)) ||
			(poisonous(&mons[obj->corpsenm]) &&
						!resists_poison(mon)))
			return POISON;
		    else if (fptr->mlet == S_FUNGUS)
			return (herbi ? CADAVER : MANFOOD);
		    else if (is_meaty(fptr))
		        return (carni ? CADAVER : MANFOOD);
		    else return (carni ? ACCFOOD : MANFOOD);
		case CLOVE_OF_GARLIC:
		    return (is_undead(mon->data) ? TABU :
			    (herbi ? ACCFOOD : MANFOOD));
		case TIN:
		    return (metallivorous(mon->data) ? ACCFOOD : MANFOOD);
		case APPLE:
		case CARROT:
		    return (herbi ? DOGFOOD : MANFOOD);
		case BANANA:
		    return ((mon->data->mlet == S_YETI) ? DOGFOOD :
			    (herbi ? ACCFOOD : MANFOOD));
		default:
		    return (obj->otyp > SLIME_MOLD ?
			    (carni ? ACCFOOD : MANFOOD) :
			    (herbi ? ACCFOOD : MANFOOD));
	    }
	default:
	    if (hates_silver(mon->data) &&
		objects[obj->otyp].oc_material == SILVER)
		return(TABU);
	    if (mon->data == &mons[PM_GELATINOUS_CUBE] && is_organic(obj))
		return(ACCFOOD);
	    if (metallivorous(mon->data) && is_metallic(obj))
		/* Ferrous based metals are preferred. */
		return(objects[obj->otyp].oc_material == IRON ? DOGFOOD :
		       ACCFOOD);
	    if(!obj->cursed && obj->oclass != BALL_CLASS &&
						obj->oclass != CHAIN_CLASS)
		return(APPORT);
	    /* fall into next case */
	case ROCK_CLASS:
	    return(UNDEF);
	}
}

#endif /* OVL1 */
#ifdef OVLB

struct monst *
tamedog(mtmp, obj)
register struct monst *mtmp;
register struct obj *obj;
{
	register struct monst *mtmp2;

	/* The Wiz, Medusa and the quest nemeses aren't even made peaceful. */
	if (mtmp->iswiz || mtmp->data == &mons[PM_MEDUSA]
				|| (mtmp->data->mflags3 & M3_WANTSARTI))
		return((struct monst *)0);

	/* worst case, at least it'll be peaceful. */
	mtmp->mpeaceful = 1;
	set_malign(mtmp);
	if(flags.moonphase == FULL_MOON && night() && rn2(6) && obj
						&& mtmp->data->mlet == S_DOG)
		return((struct monst *)0);

	/* If we cannot tame it, at least it's no longer afraid. */
	mtmp->mflee = 0;
	mtmp->mfleetim = 0;

	/* make grabber let go now, whether it becomes tame or not */
	if (mtmp == u.ustuck) {
	    if (u.uswallow)
		expels(mtmp, mtmp->data, TRUE);
	    else if (!(Upolyd && sticks(uasmon)))
		unstuck(mtmp);
	}

	/* feeding it treats makes it tamer */
	if (mtmp->mtame && obj) {
	    int tasty;

	    if (mtmp->mcanmove && !mtmp->mconf && !mtmp->meating &&
		((tasty = dogfood(mtmp, obj)) == DOGFOOD ||
		 (tasty <= ACCFOOD && EDOG(mtmp)->hungrytime <= moves))) {
		/* pet will "catch" and eat this thrown food */
		if (canseemon(mtmp)) {
		    boolean big_corpse = (obj->otyp == CORPSE &&
					  obj->corpsenm >= LOW_PM &&
				mons[obj->corpsenm].msize > mtmp->data->msize);
		    pline("%s catches %s%s",
			  Monnam(mtmp), the(xname(obj)),
			  !big_corpse ? "." : ", or vice versa!");
		} else if (cansee(mtmp->mx,mtmp->my))
		    pline("%s stops.", The(xname(obj)));
		/* dog_eat expects a floor object */
		place_object(obj, mtmp->mx, mtmp->my);
		(void) dog_eat(mtmp, obj, mtmp->mx, mtmp->my, FALSE);
		/* eating might have killed it, but that doesn't matter here;
		   a non-null result suppresses "miss" message for thrown
		   food and also implies that the object has been deleted */
		return mtmp;
	    } else
		return (struct monst *)0;
	}

	if (mtmp->mtame || !mtmp->mcanmove ||
	    /* monsters with conflicting structures cannot be tamed */
	    mtmp->isshk || mtmp->isgd || mtmp->ispriest || mtmp->isminion ||
	    is_covetous(mtmp->data) || is_human(mtmp->data) ||
	    (is_demon(mtmp->data) && !is_demon(uasmon)) ||
	    (obj && dogfood(mtmp, obj) >= MANFOOD)) return (struct monst *)0;

	/* make a new monster which has the pet extension */
	mtmp2 = newmonst(sizeof(struct edog) + mtmp->mnamelth);
	*mtmp2 = *mtmp;
	mtmp2->mxlth = sizeof(struct edog);
	if (mtmp->mnamelth) Strcpy(NAME(mtmp2), NAME(mtmp));
	initedog(mtmp2);
	replmon(mtmp, mtmp2);
	/* `mtmp' is now obsolete */

	if (obj) {		/* thrown food */
	    /* defer eating until the edog extension has been set up */
	    place_object(obj, mtmp2->mx, mtmp2->my);	/* put on floor */
	    /* devour the food (might grow into larger, genocided monster) */
	    if (dog_eat(mtmp2, obj, mtmp2->mx, mtmp2->my, TRUE) == 2)
		return mtmp2;		/* oops, it died... */
	    /* `obj' is now obsolete */
	}

	newsym(mtmp2->mx, mtmp2->my);
	if (attacktype(mtmp2->data, AT_WEAP)) {
		mtmp2->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp2);
	}
	return(mtmp2);
}

void
abuse_dog(mtmp)
struct monst *mtmp;
{
	if (!mtmp->mtame) return;

	if (Aggravate_monster || Conflict) mtmp->mtame /=2;
	else mtmp->mtame--;

	if (mtmp->mtame && rn2(mtmp->mtame)) yelp(mtmp);
	else growl(mtmp);	/* give them a moment's worry */

	if (!mtmp->mtame) newsym(mtmp->mx, mtmp->my);
}

#endif /* OVLB */

/*dog.c*/
