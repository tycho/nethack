/*	SCCS Id: @(#)fountain.c	3.0	88/12/22
/* Code for drinking from fountains.   */
/* Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef FOUNTAINS
static void NDECL(dowatersnakes);
static void NDECL(dowaterdemon);
static void NDECL(dowaternymph);
static void FDECL(dogushforth,(int));	
static void NDECL(dofindgem);

static
void
dowatersnakes() /* Fountain of snakes! */ {
	register int num = rnd(6);
	if (!(mons[PM_WATER_MOCCASIN].geno & G_GENOD)) {
		if (!Blind)
			pline("An endless stream of %s pours forth!",
				Hallucination ? makeplural(rndmonnam())
				: "snakes");
		else
			You("hear something hissing!");
		while(num-- > 0) (void) makemon(&mons[PM_WATER_MOCCASIN],u.ux,u.uy);
	} else
		pline("The fountain bubbles furiously for a moment, then calms.");
}

static
void
dowaterdemon() /* Water demon */ {
register struct monst *mtmp;

	if((mtmp = makemon(&mons[PM_WATER_DEMON],u.ux,u.uy))) {
	    if (!Blind)
	    	You("have unleashed %s!", defmonnam(mtmp));
	    else
		You("feel the presence of evil.");

	/* Give those on low levels a (slightly) better chance of survival */
	    if ( rnd(100) > (80 + dlevel)) {
		pline("Grateful for %s release, %s grants you a wish!",
			Blind ? "its" : "his", Blind ? "it" : "he" );
		makewish();
		mongone(mtmp);
	    }
	}
}

static
void
dowaternymph() /* Water Nymph */ {
	register struct monst *mtmp;
	if((mtmp = makemon(&mons[PM_WATER_NYMPH],u.ux,u.uy))) {
		if (!Blind)
		   You("have attracted %s!", defmonnam(mtmp));
		else
		   You("hear a seductive voice.");
		mtmp->msleep = 0;
	} else
		if (!Blind)
		   pline("A large bubble rises to the surface and pops.");
		else
		   You("hear a loud pop.");
}


static
void
dogushforth(drinking) /* Gushing forth in this room */
int drinking;
{
register int num = rnd(10);
register xchar mx,my;
register int tryct = 0;
register int uroom = inroom(u.ux, u.uy);
register struct mkroom *croom = &rooms[uroom];
register int madepool = 0;

	if(croom->hx < 0 || has_upstairs(croom) ||
	   has_dnstairs(croom))  {
		if (drinking) Your("thirst is quenched.");
		else pline("Water sprays all over you.");
		return;
	}
	while(num--) {
	    do {
		if(++tryct > 200)  {
		    if(madepool)
			pline("Water gushes forth from the overflowing fountain!");
		    else if (drinking) Your("thirst is quenched.");
		else pline("Water sprays all over you.");
		    return;
		}
		mx = somex(croom);
		my = somey(croom);
	    } while(nexttodoor(mx,my) || !((mx+my)%2) ||
		    (mx == u.ux && my == u.uy) ||
		    (IS_POOL(levl[mx][my].typ)));
		       
	    /* Put a pool at mx, my */
		     
	    levl[mx][my].typ = POOL;
	    levl[mx][my].doormask = 0;
	    mnewsym(mx,my);
	    if (cansee(mx, my))
		prl(mx, my);
	    madepool = 1;
	}

	pline("Water gushes forth from the overflowing fountain!");
}

static
void
dofindgem() /* Find a gem in the sparkling waters. */ {

	if (!Blind) You("spot a gem in the sparkling waters!");
	(void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE-1), u.ux, u.uy);
	levl[u.ux][u.uy].looted = T_LOOTED;
}

void
dryup(){
	if (!rn2(3) && IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
		if (!Blind) pline("The fountain dries up!");
		levl[u.ux][u.uy].typ = ROOM;
		levl[u.ux][u.uy].looted = 0;
		if(Invisible) newsym(u.ux, u.uy);
		fountsound--;
	}
}

void
drinkfountain() {

	/* What happens when you drink from a fountain? */
	register int fate = rnd(30);

	if(Levitation)	{
		You("are floating high above the fountain.");
		return;
	}
	else if (fate < 10) {
		pline("The cool draught refreshes you.");
		u.uhunger += rnd(10); /* don't choke on water */
	} else {
	    switch (fate) {

		case 19: /* Self-knowledge */

			You("feel self-knowledgeable...");
			more();
			enlightenment();
			pline("The feeling subsides.");
			break;

		case 20: /* Foul water */

			pline("The water is foul!  You gag and vomit.");
			morehungry(rnd(20)+10);
			vomit();
			break;

		case 21: /* Poisonous */

			pline("The water is contaminated!");
			if (Poison_resistance) {
#ifdef TUTTI_FRUTTI
	   pline("Perhaps it is runoff from the nearby %s farm.", pl_fruit);
#else
	   pline("Perhaps it is runoff from the nearby orange farm.");
#endif
			   losehp(rnd(4),"unrefrigerated sip of juice",
				KILLED_BY_AN);
			   break;
			}
			losestr(rn1(4,3));
			losehp(rnd(10),"contaminated water", KILLED_BY);
			break;
	
		case 22: /* Fountain of snakes! */

			dowatersnakes();
			break;

		case 23: /* Water demon */
			dowaterdemon();
			break;

		case 24: /* Curse an item... */ {
			register struct obj *obj;

			pline("This water's no good!");
			morehungry(rnd(20)+10);
			for(obj = invent; obj ; obj = obj->nobj)
				if (!rn2(5))	curse(obj);
			break;
			}
			 
		case 25: /* See invisible */

			You("see an image of someone stalking you.");
			pline("But it disappears.");
			HSee_invisible |= INTRINSIC;
			break;

		case 26: /* See Monsters */

			(void) monster_detect((struct obj *)0);
			break;

		case 27: /* Find a gem in the sparkling waters. */

			if (!levl[u.ux][u.uy].looted) {
				dofindgem();
				break;
			}

		case 28: /* Water Nymph */

			dowaternymph();
			break;

		case 29: /* Scare */ {
			register struct monst *mtmp;

			pline("This water gives you bad breath!");
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) 
				mtmp->mflee = 1;
			}
			break;

		case 30: /* Gushing forth in this room */

			dogushforth(TRUE);
			break;

		default:

			pline("This tepid water is tasteless.");
			break;
	    }
	}
	dryup();
}

void
dipfountain(obj)
register struct obj *obj;
{
	if (Levitation) {
		You("are floating high above the fountain.");
		return;
	}

	if (obj->otyp == LONG_SWORD && u.ulevel >= 5 && !rn2(6)
#ifdef NAMED_ITEMS
	    && !is_artifact(obj) && !exist_artifact(obj, "Excalibur")
#else
	    && !strcmp(ONAME(obj), "Excalibur")
#endif
	   ) {
		/* The lady of the lake acts! - Eric Backus */
		/* Be *REAL* nice */
	pline("A murky hand from the depths reaches up to bless the sword.");
	pline("As the hand retreats, the fountain disappears!");

#ifndef NAMED_ITEMS
		if(obj->spe < 5) obj->spe = 5;
#else
		/* otherwise +rnd(10) / +5 "Super"sword */
		obj = oname(obj, "Excalibur", 1);
#endif
		bless(obj);
		obj->rustfree = 1;
		levl[u.ux][u.uy].typ = ROOM;
                levl[u.ux][u.uy].looted = 0;
		if(Invisible) newsym(u.ux, u.uy);
		fountsound--;
		return;
	} else (void) get_wet(obj);

	switch (rnd(30)) {
		case 16: /* Curse the item */
			curse(obj);
			break;
		case 17:
		case 18:
		case 19:
		case 20: /* Uncurse the item */
			if(obj->cursed) {
			    if (!Blind)
				pline("The water glows for a moment.");
			    obj->cursed = 0;
			} else {
			    pline("A feeling of loss comes over you.");
			}
			break;
		case 21: /* Water Demon */
			dowaterdemon();
			break;
		case 22: /* Water Nymph */
			dowaternymph();
			break;
		case 23: /* an Endless Stream of Snakes */
			dowatersnakes();
			break;
		case 24: /* Find a gem */
			dofindgem();
			break;
		case 25: /* Water gushes forth */
			dogushforth(FALSE);
			break;
		case 26: /* Strange feeling */
			pline("A strange tingling runs up your %s.",
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			You("feel a sudden chill.");
			break;
		case 28: /* Strange feeling */
			pline("An urge to take a bath overwhelms you.");
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
			    You("lost some of your gold in the fountain!");
			    levl[u.ux][u.uy].looted = 0;
			}
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

			mkgold((long)(rnd((MAXLEVEL-dlevel)*2)+5), u.ux, u.uy);
			if (!Blind)
		pline("Far below you, you see coins glistening in the water.");
			break;
	}
	dryup();
	return;
}
#endif

#ifdef SINKS
void
drinksink()
{
	if (Levitation) {
		You("are floating high above the sink.");
		return;
	}
	switch(rn2(20)) {
		static struct obj NEARDATA *otmp;
		case 0: You("take a sip of very cold water.");
			break;
		case 1: You("take a sip of very warm water.");
			break;
		case 2: You("take a sip of scalding hot water.");
			if (Fire_resistance)
				pline("It seems quite tasty.");
			else losehp(rnd(6), "sipping boiling water", KILLED_BY);
			break;
		case 3: if (mons[PM_SEWER_RAT].geno & G_GENOD)
				pline("The sink seems quite dirty.");
			else {
				static struct monst NEARDATA *mtmp;

				mtmp = makemon(&mons[PM_SEWER_RAT], u.ux, u.uy);
				pline("Eek!  There's %s in the sink!",
					Blind ? "something squirmy" :
					defmonnam(mtmp));
			}
			break;
		case 4: do {
				otmp = mkobj(POTION_SYM,FALSE);
			} while(otmp->otyp == POT_WATER);
			otmp->cursed = otmp->blessed = 0;
			if (Blind)
				pline("The sink emits some odd liquid.");
			else
				pline("The sink emits a stream of %s water.",
				    Hallucination ? hcolor() :
				    objects[otmp->otyp].oc_descr);
			otmp->dknown = !(Blind || Hallucination);
			otmp->quan++; /* Avoid panic upon useup() */
			otmp->corpsenm = 1; /* kludge for docall() */
			(void) dopotion(otmp);
			obfree(otmp, (struct obj *)0);
			break;
		case 5: if (!levl[u.ux][u.uy].looted) {
			    You("find a ring in the sink!");
			    (void) mkobj_at(RING_SYM, u.ux, u.uy, TRUE);
			    levl[u.ux][u.uy].looted = T_LOOTED;
			} else pline("Some dirty water backs up in the drain.");
			break;
		case 6: pline("The pipes break!  Water spurts out!");
			sinksound--;
			levl[u.ux][u.uy].doormask = 0;
#ifdef FOUNTAINS
			levl[u.ux][u.uy].typ = FOUNTAIN;
			fountsound++;
#else
			levl[u.ux][u.uy].typ = ROOM;
#endif
			if (Invisible) newsym(u.ux,u.uy);
			break;
		case 7: pline("The water moves as though of its own will!");
			if (!makemon(&mons[PM_WATER_ELEMENTAL], u.ux, u.uy))
				pline("But it quiets down.");
			break;
		case 8: pline("Yuk, this water tastes awful.");
			more_experienced(1,0);
			newexplevel();
			break;
		case 9: pline("Gaggg... this tastes like sewage!  You vomit.");
			morehungry(rnd(30-ACURR(A_CON))+10);
			vomit();
			break;
#ifdef POLYSELF
		case 10: pline("This water contains toxic wastes!");
			You("undergo a freakish metamorphosis!");
			polyself();
			break;
#endif
		/* more odd messages --JJB */
		case 11: You("hear clanking from the pipes....");
			break;
		case 12: You("hear snatches of song from among the sewers...");
			break;
		case 19: if (Hallucination) {
				pline("A murky hand reaches up out of the drain... --oops--");
				break;
			}
		default: You("take a sip of %s water.",
			rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot");
	}
}
#endif /* SINKS /**/
