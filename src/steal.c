/*	SCCS Id: @(#)steal.c	3.1	93/05/30	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL int NDECL(stealarm);

#ifdef OVLB
static const char * FDECL(equipname, (struct obj *));

static const char *
equipname(otmp)
register struct obj *otmp;
{
	return (
#ifdef TOURIST
		(otmp == uarmu) ? "shirt" :
#endif
		(otmp == uarmf) ? "boots" :
		(otmp == uarms) ? "shield" :
		(otmp == uarmg) ? "gloves" :
		(otmp == uarmc) ? "cloak" :
		(otmp == uarmh) ? "helmet" : "armor");
}

long		/* actually returns something that fits in an int */
somegold()
{
#ifdef LINT	/* long conv. ok */
	return(0L);
#else
	return (long)( (u.ugold < 100) ? u.ugold :
		(u.ugold > 10000) ? rnd(10000) : rnd((int) u.ugold) );
#endif
}

void
stealgold(mtmp)
register struct monst *mtmp;
{
	register struct obj *gold = g_at(u.ux, u.uy);
	register long tmp;

	if (gold && ( !u.ugold || gold->quan > u.ugold || !rn2(5))) {
	    mtmp->mgold += gold->quan;
	    delobj(gold);
	    newsym(u.ux, u.uy);
	    pline("%s quickly snatches some gold from between your %s!",
		    Monnam(mtmp), makeplural(body_part(FOOT)));
	    if(!u.ugold || !rn2(5)) {
		if (!tele_restrict(mtmp)) rloc(mtmp);
		mtmp->mflee = 1;
	    }
	} else if(u.ugold) {
	    u.ugold -= (tmp = somegold());
	    Your("purse feels lighter.");
	    mtmp->mgold += tmp;
	    if (!tele_restrict(mtmp)) rloc(mtmp);
	    mtmp->mflee = 1;
	    flags.botl = 1;
	}
}

/* steal armor after you finish taking it off */
unsigned int stealoid;		/* object to be stolen */
unsigned int stealmid;		/* monster doing the stealing */

STATIC_OVL int
stealarm()
{
	register struct monst *mtmp;
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
	  if(otmp->o_id == stealoid) {
	    for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	      if(mtmp->m_id == stealmid) {
		  if(otmp->unpaid) 
		       subfrombill(otmp, shop_keeper(*u.ushops));
		  freeinv(otmp);
		  pline("%s steals %s!", Monnam(mtmp), doname(otmp));
		  mpickobj(mtmp,otmp);
		  mtmp->mflee = 1;
		  if (!tele_restrict(mtmp)) rloc(mtmp);
		break;
	      }
	    break;
	  }
	stealoid = 0;
	return 0;
}

/* Returns 1 when something was stolen (or at least, when N should flee now)
 * Returns -1 if the monster died in the attempt
 * Avoid stealing the object stealoid
 */
int
steal(mtmp)
struct monst *mtmp;
{
	register struct obj *otmp;
	register int tmp;
	register int named = 0;

	/* the following is true if successful on first of two attacks. */
	if(!monnear(mtmp, u.ux, u.uy)) return(0);

	if(!invent
#ifdef POLYSELF
		   || (inv_cnt() == 1 && uskin)
#endif
						){
	    /* Not even a thousand men in armor can strip a naked man. */
	    if(Blind)
	      pline("Somebody tries to rob you, but finds nothing to steal.");
	    else
	      pline("%s tries to rob you, but she finds nothing to steal!",
		Monnam(mtmp));
	    return(1);	/* let her flee */
	}

	if(Adornment & LEFT_RING) {
	    otmp = uleft;
	    goto gotobj;
	} else if(Adornment & RIGHT_RING) {
	    otmp = uright;
	    goto gotobj;
	}

	tmp = 0;
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if((!uarm || otmp != uarmc)
#ifdef POLYSELF
					&& otmp != uskin
#endif
							)
		tmp += ((otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_TOOL)) ? 5 : 1);
	tmp = rn2(tmp);
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if((!uarm || otmp != uarmc)
#ifdef POLYSELF
					&& otmp != uskin
#endif
							)
		if((tmp -= ((otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_TOOL)) ? 5 : 1)) < 0)
			break;
	if(!otmp) {
		impossible("Steal fails!");
		return(0);
	}
	/* can't steal gloves while wielding - so steal the wielded item. */
	if (otmp == uarmg && uwep)
	    otmp = uwep;
	/* can't steal armor while wearing cloak - so steal the cloak. */
	else if(otmp == uarm && uarmc) otmp = uarmc;
#ifdef TOURIST
	else if(otmp == uarmu && uarmc) otmp = uarmc;
	else if(otmp == uarmu && uarm) otmp = uarm;
#endif
gotobj:
	if(otmp->o_id == stealoid) return(0);

#ifdef WALKIES
	if(otmp->otyp == LEASH && otmp->leashmon) o_unleash(otmp);
#endif

	if((otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL))){
		switch(otmp->oclass) {
		case TOOL_CLASS:
			Blindf_off(otmp);
			break;
		case AMULET_CLASS:
			Amulet_off();
			break;
		case RING_CLASS:
			Ring_gone(otmp);
			break;
		case ARMOR_CLASS:
			/* Stop putting on armor which has been stolen. */
			if (donning(otmp)) {
			  cancel_don();
			  if (otmp == uarm)  (void) Armor_off();
			  /* else if (otmp == uarmc) (void) Cloak_off(); */
			  else if (otmp == uarmf) (void) Boots_off();
			  else if (otmp == uarmg) (void) Gloves_off();
			  else if (otmp == uarmh) (void) Helmet_off();
			  /* else if (otmp == uarms) (void) Shield_off(); */
			  else setworn((struct obj *)0, otmp->owornmask & W_ARMOR);
			  break;
			}
		    { int curssv = otmp->cursed;
			otmp->cursed = 0;
			stop_occupation();
			if(flags.female)
			    pline("%s charms you.  You gladly %s your %s.",
				  Blind ? "She" : Monnam(mtmp),
				  curssv ? "let her take" : "hand over",
				  equipname(otmp));
			else
			    pline("%s seduces you and %s off your %s.",
				  Blind ? "It" : Adjmonnam(mtmp, "beautiful"),
				  curssv ? "helps you to take" : "you start taking",
				  equipname(otmp));
			named++;
			/* the following is to set multi for later on */
			nomul(-objects[otmp->otyp].oc_delay);

			if (otmp == uarm)  (void) Armor_off();
			else if (otmp == uarmc) (void) Cloak_off();
			else if (otmp == uarmf) (void) Boots_off();
			else if (otmp == uarmg) (void) Gloves_off();
			else if (otmp == uarmh) (void) Helmet_off();
			else if (otmp == uarms) (void) Shield_off();
			else setworn((struct obj *)0, otmp->owornmask & W_ARMOR);
			otmp->cursed = curssv;
			if(multi < 0){
				/*
				multi = 0;
				nomovemsg = 0;
				afternmv = 0;
				*/
				stealoid = otmp->o_id;
				stealmid = mtmp->m_id;
				afternmv = stealarm;
				return(0);
			}
			break;
		    }
		default:
			impossible("Tried to steal a strange worn thing.");
		}
	}
	else if(otmp == uwep) uwepgone();

	if(otmp == uball) unpunish();

	freeinv(otmp);
	pline("%s stole %s.", named ? "She" : Monnam(mtmp), doname(otmp));
	(void) snuff_candle(otmp);
	mpickobj(mtmp,otmp);
	if (otmp->otyp == CORPSE && otmp->corpsenm == PM_COCKATRICE
	    && !resists_ston(mtmp->data)
#ifdef MUSE
	    && !(mtmp->misc_worn_check & W_ARMG)
#endif
		) {
	    pline("%s turns to stone.", Monnam(mtmp));
	    stoned = TRUE;
	    xkilled(mtmp, 0);
	    return -1;
	}
	return((multi < 0) ? 0 : 1);
}

#endif /* OVLB */
#ifdef OVL1

void
mpickobj(mtmp,otmp)
register struct monst *mtmp;
register struct obj *otmp;
{
    if (otmp->otyp == GOLD_PIECE) {	/* from floor etc. -- not inventory */
	mtmp->mgold += otmp->quan;
	obfree(otmp, (struct obj *)0);
    } else {
	otmp->nobj = mtmp->minvent;
	mtmp->minvent = otmp;
    }
}

#endif /* OVL1 */
#ifdef OVLB

void
stealamulet(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;
	register int	real, fake;

	/* select the artifact to steal */
	if(u.uhave.amulet) {
		real = AMULET_OF_YENDOR ;
		fake = FAKE_AMULET_OF_YENDOR ;
#ifdef MULDGN
	} else if(u.uhave.questart) {
	    real = fake = 0;		/* gcc -Wall lint */
	    for(otmp = invent; otmp; otmp = otmp->nobj)
	        if(is_quest_artifact(otmp)) goto snatch_it;
#endif
	} else if(u.uhave.bell) {
		real = BELL_OF_OPENING;
		fake = BELL;
	} else if(u.uhave.book) {
		real = SPE_BOOK_OF_THE_DEAD;
		fake = 0;
	} else if(u.uhave.menorah) {
		real = CANDELABRUM_OF_INVOCATION;
		fake = 0;
	} else return;	/* you have nothing of special interest */

/*	If we get here, real and fake have been set up. */
	for(otmp = invent; otmp; otmp = otmp->nobj) {
	    if(otmp->otyp == real || (otmp->otyp == fake && !mtmp->iswiz)) {
		/* might be an imitation one */
snatch_it:
#ifdef MULDGN
		if (otmp->oclass == ARMOR_CLASS) adj_abon(otmp, -(otmp->spe));
#endif
		setnotworn(otmp);
		freeinv(otmp);
		mpickobj(mtmp,otmp);
		pline("%s stole %s!", Monnam(mtmp), doname(otmp));
		if (can_teleport(mtmp->data) && !tele_restrict(mtmp))
			rloc(mtmp);
		return;
	    }
	}
}

#endif /* OVLB */
#ifdef OVL0

/* release the objects the creature is carrying */
void
relobj(mtmp,show,is_pet)
register struct monst *mtmp;
register int show;
boolean is_pet;		/* If true, pet should keep wielded/worn items */
{
	register struct obj *otmp, *otmp2;
	register int omx = mtmp->mx, omy = mtmp->my;
#ifdef MUSE
	struct obj *backobj = 0;
	struct obj *wep = MON_WEP(mtmp);
#endif

	otmp = mtmp->minvent;
	mtmp->minvent = 0;
	for (; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
#ifdef MUSE
		if (otmp->owornmask || otmp == wep) {
			if (is_pet) { /* skip worn/wielded item */
				if (!backobj) {
					mtmp->minvent = backobj = otmp;
				} else {
					backobj->nobj = otmp;
					backobj = backobj->nobj;
				}
				continue;
			}
			mtmp->misc_worn_check &= ~(otmp->owornmask);
			otmp->owornmask = 0L;
		}
		if (backobj) backobj->nobj = otmp->nobj;
#endif
		if (is_pet && cansee(omx, omy) && flags.verbose)
			pline("%s drops %s.", Monnam(mtmp),
					distant_name(otmp, doname));
		if (flooreffects(otmp, omx, omy, "fall")) continue;
		place_object(otmp, omx, omy);
		otmp->nobj = fobj;
		fobj = otmp;
		stackobj(fobj);
	}
	if (mtmp->mgold) {
		register long g = mtmp->mgold;
		mkgold(g, omx, omy);
		if (is_pet && cansee(omx, omy) && flags.verbose)
			pline("%s drops %ld gold piece%s.", Monnam(mtmp),
				g, plur(g));
		mtmp->mgold = 0L;
	}
	if (show & cansee(omx, omy))
		newsym(omx, omy);
}

#endif /* OVL0 */

/*steal.c*/
