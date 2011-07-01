/*	SCCS Id: @(#)dog.c	3.1	93/06/19	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"

#define domestic(mtmp)	(mtmp->data->msound == MS_BARK || \
			 mtmp->data->msound == MS_MEW)

#ifdef OVLB

static int NDECL(pet_type);

void
initedog(mtmp)
register struct monst *mtmp;
{
	mtmp->mtame = domestic(mtmp) ? 10 : 5;
	mtmp->mpeaceful = 1;
	set_malign(mtmp); /* recalc alignment now that it's tamed */
	mtmp->mleashed = 0;
	mtmp->meating = 0;
	EDOG(mtmp)->droptime = 0;
	EDOG(mtmp)->dropdist = 10000;
	EDOG(mtmp)->apport = 10;
	EDOG(mtmp)->whistletime = 0;
	EDOG(mtmp)->hungrytime = 1000 + moves;
}

static int
pet_type()
{
	register int pettype;

	switch (pl_character[0]) {
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
	register struct monst *mtmp;
	register struct permonst *pm;

top:
	if (otmp) pm = &mons[otmp->corpsenm]; /* Figurine; otherwise spell */
	else if (rn2(3)) {
	    if (!(pm = rndmonst())) {
		pline("There seems to be nothing available for a familiar.");
		return;
	    }
	}
	else pm = &mons[pet_type()];

	pm->pxlth += sizeof(struct edog);
	mtmp = makemon(pm, x, y);
	pm->pxlth -= sizeof(struct edog);
	if (!mtmp) { /* monster was genocided */
	    if (otmp)
		pline("The figurine writhes and then shatters into pieces!");
	    else goto top;
		/* rndmonst() returns something not genocided always, so this
		 * means it was a cat or dog; loop back to try again until
		 * either rndmonst() is called, or if only one of cat/dog
		 * was genocided, they get the other.
		 */
	    return;
	}
	initedog(mtmp);
	mtmp->msleep = 0;
	if (otmp && otmp->cursed) { /* cursed figurine */
		You("get a bad feeling about this.");
		mtmp->mtame = mtmp->mpeaceful = 0;
		newsym(mtmp->mx, mtmp->my);
	}
	set_malign(mtmp); /* more alignment changes */
#ifdef MUSE
	/* must wield weapon immediately since pets will otherwise drop it */
	if (mtmp->mtame && attacktype(mtmp->data, AT_WEAP)) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp);
	}
#endif
}

struct monst *
makedog()
{
	register struct monst *mtmp;
	char *petname;
	int   pettype;
	static int petname_used = 0;

	pettype = pet_type();
	if (pettype == PM_LITTLE_DOG)
		petname = dogname;
	else
		petname = catname;

	mons[pettype].pxlth = sizeof(struct edog);
	mtmp = makemon(&mons[pettype], u.ux, u.uy);
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
	int num_segs;

	while ((mtmp = mydogs) != 0) {
		mydogs = mtmp->nmon;
		mtmp->nmon = fmon;
		fmon = mtmp;
		if (mtmp->isshk)
		    set_residency(mtmp, FALSE);

		num_segs = mtmp->wormno;
		/* baby long worms have no tail so don't use is_longworm() */
		if ( (mtmp->data == &mons[PM_LONG_WORM]) &&
		     (mtmp->wormno = get_wormno()) ) {
		    initworm(mtmp, num_segs);
		    /* tail segs are not yet initialized or displayed */
		} else mtmp->wormno = 0;
		mnexto(mtmp);
	}

	for(mtmp = migrating_mons; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if(mtmp->mx == u.uz.dnum && mtmp->mux == u.uz.dlevel) {
		    mtmp->mx = 0;		   /* save xyloc in mtmp->my */
		    mtmp->mux = u.ux; mtmp->muy = u.uy; /* not really req'd */
		    if(mtmp == migrating_mons)
			migrating_mons = mtmp->nmon;
		    else
			mtmp0->nmon = mtmp->nmon;
		    mtmp->nmon = fmon;
		    fmon = mtmp;
		    num_segs = mtmp->wormno;
		    if ( (mtmp->data == &mons[PM_LONG_WORM]) &&
			 (mtmp->wormno = get_wormno()) ) {
			initworm(mtmp, num_segs);
			/* tail segs are not yet initialized or displayed */
		    } else mtmp->wormno = 0;

		    if(mtmp->mlstmv < monstermoves-1) {
			/* heal monster for time spent in limbo */
			long nmv = monstermoves - mtmp->mlstmv - 1;

			/* might stop being afraid, blind or frozen */
			/* set to 1 and allow final decrement in movemon() */
			if(nmv >= (long)mtmp->mblinded) mtmp->mblinded = 1;
			else mtmp->mblinded -= nmv;
			if(nmv >= (long)mtmp->mfrozen) mtmp->mfrozen = 1;
			else mtmp->mfrozen -= nmv;
			if(nmv >= (long)mtmp->mfleetim) mtmp->mfleetim = 1;
			else mtmp->mfleetim -= nmv;

			/* might be able to use special ability again */
			if(nmv > (long)mtmp->mspec_used) mtmp->mspec_used = 0;
			else mtmp->mspec_used -= nmv;

			/* reduce tameness for every 150 moves you are away */
			if (mtmp->mtame > nmv/150) mtmp->mtame -= nmv/150;
			else mtmp->mtame = 0;

			/* let monster move a bit on the new level */
			/* see placement code below */
			mtmp->mx = min(nmv, 8);
#ifdef MULDGN
			/*	temporary hack...
			 * Ft.Ludios' entry chamber isn't classified as a
			 * room by lev_comp and any movement greater than
			 * 1 can result in placement beyond solid walls.
			 */
			if (Is_knox(&u.uz)) mtmp->mx = 1;
#endif

			if(!regenerates(mtmp->data)) nmv /= 20;
			if((long)mtmp->mhp + nmv >= (long)mtmp->mhpmax)
			    mtmp->mhp = mtmp->mhpmax;
			else mtmp->mhp += nmv;
			mtmp->mlstmv = monstermoves-1;
		    }

		    if (mtmp->data->geno & G_GENOD) {
#ifdef KOPS
			allow_kops = FALSE;
#endif
			mondead(mtmp);	/* must put in fmon list first */
#ifdef KOPS
			allow_kops = TRUE;
#endif
		    } else if (mtmp->isshk && mtmp->mpeaceful)
			home_shk(mtmp, TRUE);
		    else switch(mtmp->my) {
			xchar xlocale, ylocale;
			register struct trap *t;

			case 1: xlocale = xupstair; ylocale = yupstair;
				goto common;
			case 2: xlocale = xdnstair; ylocale = ydnstair;
				goto common;
			case 3: xlocale = xupladder; ylocale = yupladder;
				goto common;
			case 4: xlocale = xdnladder; ylocale = ydnladder;
				goto common;
			case 5: xlocale = sstairs.sx; ylocale = sstairs.sy;
				goto common;
			case 6:		/* portal */
				for (t = ftrap; t; t = t->ntrap)
				    if (t->ttyp == MAGIC_PORTAL) break;
				if (t) {
				    xlocale = t->tx;
				    ylocale = t->ty;
				} else {
				    if (!In_endgame(&u.uz))
			      impossible("losedogs: no corresponding portal?");
				    xlocale = ylocale = 0;
				}
				goto common;
common:
				if (xlocale && mtmp->mx) {
				    /* monster moved a bit */
				    /* pick a nearby location */
				    /* mnearto() deals w/stone, et al */
				    char *r = in_rooms(xlocale, ylocale, 0);
				    if (r && *r) {
					coord c;
					/* somexy() handles irregular rooms */
					if (somexy(&rooms[*r - ROOMOFFSET], &c))
					    xlocale = c.x,  ylocale = c.y;
					else
					    xlocale = ylocale = 0;
				    } else {
					int i, j;
					i = max(1, xlocale - mtmp->mx);
					j = min(COLNO-1, xlocale + mtmp->mx);
					xlocale = rn1(j - i, i);
					i = max(0, ylocale - mtmp->mx);
					j = min(ROWNO-1, ylocale + mtmp->mx);
					ylocale = rn1(j - i, i);
				    }
				}
				if (xlocale) {
				    (void) mnearto(mtmp,
						   xlocale, ylocale, FALSE);
				    break;
				} /* else fall through */
			default: 
				rloc(mtmp);
				break;
			}
		} else
		    mtmp0 = mtmp;
		if (mtmp->isshk)
		    set_residency(mtmp, FALSE);
	}
}

#endif /* OVLB */
#ifdef OVL2

void
keepdogs()
{
	register struct monst *mtmp;
	register struct obj *obj;
	int num_segs = 0;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if(((monnear(mtmp, u.ux, u.uy) && levl_follower(mtmp)) ||
		/* the wiz will level t-port from anywhere to chase
		   the amulet; if you don't have it, will chase you
		   only if in range. -3. */
			(u.uhave.amulet && mtmp->iswiz))
			&& !mtmp->msleep && mtmp->mcanmove) {

		/* long worms can now change levels! - Norm */

		if (mtmp->mtame && mtmp->meating && canseemon(mtmp)) {
			pline("%s is still eating.", Monnam(mtmp));
			goto merge;
		}
		if (mon_has_amulet(mtmp)) {
			pline("%s seems very disoriented for a moment.",
				Monnam(mtmp));
		merge:
#ifdef WALKIES
			if (mtmp->mleashed) {
				pline("%s leash suddenly comes loose.",
					humanoid(mtmp->data)
					    ? (mtmp->female ? "Her" : "His")
					    : "Its");
				m_unleash(mtmp);
			}
#endif
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
		}

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
		keepdogs();	/* we destroyed the link, so use recursion */
		return;		/* (admittedly somewhat primitive) */
	}
}

#endif /* OVL2 */
#ifdef OVLB

void
migrate_to_level(mtmp, tolev, xyloc) 
	register struct monst *mtmp; 
	xchar tolev;  	/* destination level */
	xchar xyloc;	/* destination xy location: */
			/* 	0: rnd,
			 *	1: <,
			 *	2: >,
			 *	3: < ladder,
			 *	4: > ladder,
			 *	5: sstairs
			 *	6: portal
			 */ 
{
        register struct obj *obj;
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
#ifdef WALKIES
	if (mtmp->mleashed)  {
		m_unleash(mtmp);
		mtmp->mtame--;
		pline("The leash comes off!");
	}
#endif
	newsym(mtmp->mx,mtmp->my);
	/* make sure to reset mtmp->[mx,my] to 0 when releasing, */
	/* so rloc() on next level doesn't affect MON_AT() state */
	mtmp->mx = ledger_to_dnum((xchar)tolev); 
	mtmp->mux = ledger_to_dlev((xchar)tolev); 
	mtmp->my = xyloc;
	mtmp->muy = 0;
	mtmp->wormno = num_segs;
	mtmp->mlstmv = monstermoves;
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

	switch(obj->oclass) {
	case FOOD_CLASS:
	    if (obj->otyp == CORPSE &&
		((obj->corpsenm == PM_COCKATRICE && !resists_ston(mon->data))
		 || is_rider(fptr)))
		    return TABU;

	    if (!carni && !herbi)
		    return (obj->cursed ? UNDEF : APPORT);

	    switch (obj->otyp) {
		case TRIPE_RATION:
		    return (carni ? DOGFOOD : MANFOOD);
		case EGG:
		    if (obj->corpsenm == PM_COCKATRICE &&
						!resists_ston(mon->data))
			return POISON;
		    return (carni ? CADAVER : MANFOOD);
		case CORPSE:
		    if ((obj->age+50 <= monstermoves
					    && obj->corpsenm != PM_LIZARD
					    && mon->data->mlet != S_FUNGUS) ||
			(acidic(&mons[obj->corpsenm]) &&
						!resists_acid(mon->data)) ||
			(poisonous(&mons[obj->corpsenm]) &&
						!resists_poison(mon->data)))
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
#ifdef TUTTI_FRUTTI
		    return (obj->otyp > SLIME_MOLD ?
#else
		    return (obj->otyp > CLOVE_OF_GARLIC ?
#endif
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
#ifdef MULDGN
	    || (mtmp->data->mflags3 & M3_WANTSARTI)
#endif
	    )
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
	/* feeding it treats makes it tamer */
	if (mtmp->mtame && mtmp->mtame < 20 && 
	    obj && dogfood(mtmp, obj) > ACCFOOD)
	    mtmp->mtame++;
	if(mtmp->mtame || !mtmp->mcanmove ||
	   /* monsters with conflicting structures cannot be tamed */
	   mtmp->isshk || mtmp->isgd || mtmp->ispriest || mtmp->isminion ||
#ifdef POLYSELF
	   is_human(mtmp->data) || (is_demon(mtmp->data) && !is_demon(uasmon))
#else
	   is_human(mtmp->data) || is_demon(mtmp->data)
#endif
	   || (mtmp->data->mflags3 &
	       (M3_WANTSAMUL|M3_WANTSBELL|M3_WANTSBOOK|M3_WANTSCAND))
	   )
		return((struct monst *)0);
	if(obj) {
		if(dogfood(mtmp, obj) >= MANFOOD) return((struct monst *)0);
		if(cansee(mtmp->mx,mtmp->my))
			pline("%s devours %s.", Monnam(mtmp), the(xname(obj)));
		obfree(obj, (struct obj *)0);
	}
	if (u.uswallow && mtmp == u.ustuck)
		expels(mtmp, mtmp->data, TRUE);
	else if (mtmp == u.ustuck
#ifdef POLYSELF
		 && !(u.mtimedone && sticks(uasmon))
#endif
		 )
		unstuck(mtmp);
	mtmp2 = newmonst(sizeof(struct edog) + mtmp->mnamelth);
	*mtmp2 = *mtmp;
	mtmp2->mxlth = sizeof(struct edog);
	if(mtmp->mnamelth) Strcpy(NAME(mtmp2), NAME(mtmp));
	initedog(mtmp2);
	replmon(mtmp,mtmp2);
	newsym(mtmp2->mx, mtmp2->my);
#ifdef MUSE
	if (attacktype(mtmp2->data, AT_WEAP)) {
		mtmp2->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp2);
	}
#endif
	return(mtmp2);
}

void
abuse_dog(mtmp)
struct monst *mtmp;
{
	if (!mtmp->mtame) return;

	if (Aggravate_monster || Conflict) mtmp->mtame /=2;
	else mtmp->mtame--;

#ifdef SOUNDS
	if (mtmp->mtame && rn2(mtmp->mtame)) yelp(mtmp);
	else growl(mtmp);	/* give them a moment's worry */
#endif
	if (!mtmp->mtame) newsym(mtmp->mx, mtmp->my);
}

#endif /* OVLB */

/*dog.c*/
