/*	SCCS Id: @(#)mcastu.c	3.4	2003/01/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"
#include "epri.h"

/* monster mage spells */
#define MGC_PSI_BOLT	    0
#define MGC_CURE_SELF	 1
#define MGC_HASTE_SELF	 2
#define MGC_STUN_YOU	    3
#define MGC_DISAPPEAR	 4
#define MGC_WEAKEN_YOU	 5
#define MGC_DESTRY_ARMR	 6
#define MGC_CURSE_ITEMS	 7
#define MGC_AGGRAVATION	 8
#define MGC_SUMMON_MONS	 9
#define MGC_CLONE_WIZ	10
#define MGC_DEATH_TOUCH	11
#define MGC_FLY			12
#define MGC_ENRAGE		13
#define MGC_FIRE_BOLT   14
#define MGC_ICE_BOLT    15

/* monster cleric spells */
#define CLC_OPEN_WOUNDS	 0
#define CLC_CURE_SELF	 1
#define CLC_CONFUSE_YOU	 2
#define CLC_PARALYZE	 	 3
#define CLC_BLIND_YOU	 4
#define CLC_INSECTS		 5
#define CLC_CURSE_ITEMS	 6
#define CLC_LIGHTNING	 7
#define CLC_FIRE_PILLAR	 8
#define CLC_GEYSER		 9
#define CLC_FLY			10
#define CLC_VULN_YOU		11
#define CLC_SUMMON_ELM  12

STATIC_DCL void FDECL(cursetxt,(struct monst *,BOOLEAN_P));
STATIC_DCL int FDECL(choose_magic_spell, (struct monst *, int));
STATIC_DCL int FDECL(choose_clerical_spell, (struct monst *, int));
STATIC_DCL void FDECL(cast_wizard_spell,(struct monst *, int,int));
STATIC_DCL void FDECL(cast_cleric_spell,(struct monst *, int,int));
STATIC_DCL boolean FDECL(is_undirected_spell,(unsigned int,int));
STATIC_DCL boolean FDECL(spell_would_be_useless,(struct monst *,unsigned int,int));
STATIC_DCL void FDECL(cast_fly,(struct monst *));

#ifdef OVL0

extern const char * const flash_types[];	/* from zap.c */

/* feedback when frustrated monster couldn't cast a spell */
STATIC_OVL
void
cursetxt(mtmp, undirected)
struct monst *mtmp;
boolean undirected;
{
	if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
	    const char *point_msg;  /* spellcasting monsters are impolite */

	    if (undirected)
		point_msg = "all around, then curses";
	    else if ((Invis && !perceives(mtmp->data) &&
			(mtmp->mux != u.ux || mtmp->muy != u.uy)) ||
		    (youmonst.m_ap_type == M_AP_OBJECT &&
			youmonst.mappearance == STRANGE_OBJECT) ||
		    u.uundetected)
		point_msg = "and curses in your general direction";
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		point_msg = "and curses at your displaced image";
	    else
		point_msg = "at you, then curses";

	    pline("%s points %s.", Monnam(mtmp), point_msg);
	} else if ((!(moves % 4) || !rn2(4))) {
	    if (flags.soundok) Norep("You hear a mumbled curse.");
	}
}

#endif /* OVL0 */
#ifdef OVLB

/* convert a level based random selection into a specific mage spell;
   inappropriate choices will be screened out by spell_would_be_useless() */
STATIC_OVL int
choose_magic_spell(mtmp,spellval)
struct monst* mtmp;
int spellval;
{
	struct trap* tr;
	int i;

	/* If we're stuck in a pit, we know a way out */
	tr = t_at(mtmp->mx,mtmp->my);
	if (mtmp->mtrapped && tr && (tr->ttyp == PIT || tr->ttyp == SPIKED_PIT) 
			&& mtmp->m_lev > 5) { spellval = 5; }

	/* If we're hurt, seriously consider giving fixing ourselves priority */
	if ((mtmp->mhp * 4) <= mtmp->mhpmax) { spellval = 1; }

	switch (spellval) {
		case 22:
		case 21:
			return MGC_DEATH_TOUCH;
		case 20:
		case 19:
			return MGC_CLONE_WIZ;
		case 18:
		case 17:
			return MGC_ENRAGE;
		case 16:
		case 15:
			return MGC_SUMMON_MONS;
		case 14:
			return MGC_AGGRAVATION;
		case 13:
		case 12:
		case 11:
			return MGC_CURSE_ITEMS;
		case 10:
		case 9:
		case 8:
			return MGC_DESTRY_ARMR;
		case 7:
		case 6:
			return MGC_WEAKEN_YOU;
		case 5:
			return MGC_FLY;
		case 4:
			return MGC_DISAPPEAR;
		case 3:
			return MGC_STUN_YOU;
		case 2:
			return MGC_HASTE_SELF;
		case 1:
			return MGC_CURE_SELF;
		case 0:
		default:
			i = rn2(3);
			switch (i) {
				case 2:
					return MGC_FIRE_BOLT;
				case 1:
					return MGC_ICE_BOLT;
				case 0:
				default:
					return MGC_PSI_BOLT;
			}
	}
}

/* convert a level based random selection into a specific cleric spell */
STATIC_OVL int
choose_clerical_spell(mtmp,spellnum)
struct monst* mtmp;
int spellnum;
{
	struct trap* tr;

	/* If we're stuck in a pit, we know a way out */
	tr = t_at(mtmp->mx,mtmp->my);
	if (mtmp->mtrapped && tr && (tr->ttyp == PIT || tr->ttyp == SPIKED_PIT) 
			&& mtmp->m_lev > 3) { spellnum = 3; }

	/* If we're hurt, seriously consider giving fixing ourselves priority */
	if ((mtmp->mhp * 4) <= mtmp->mhpmax) { spellnum = 1; }

    switch (spellnum) {
		case 14:
			return CLC_SUMMON_ELM;
		case 13:
			return CLC_GEYSER;
		case 12:
			return CLC_FIRE_PILLAR;
		case 11:
		case 10:
			return CLC_PARALYZE;
		case 9:
			return CLC_LIGHTNING;
		case 8:
		case 7:
			return CLC_CURSE_ITEMS;
		case 6:
			return CLC_INSECTS;
		case 5:
			return CLC_BLIND_YOU;
		case 4:
			return CLC_FLY;
		case 3:
			return CLC_VULN_YOU;
		case 2:
			return CLC_CONFUSE_YOU;
		case 1:
			return CLC_CURE_SELF;
		case 0:
		default:
			return CLC_OPEN_WOUNDS;
    }
}

/* return values:
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmu(mtmp, mattk, thinks_it_foundyou, foundyou)
	register struct monst *mtmp;
	register struct attack *mattk;
	boolean thinks_it_foundyou;
	boolean foundyou;
{
	int	dmg, ml = mtmp->m_lev;
	int ret;
	int spellnum = 0;

	/* Three cases:
	 * -- monster is attacking you.  Search for a useful spell.
	 * -- monster thinks it's attacking you.  Search for a useful spell,
	 *    without checking for undirected.  If the spell found is directed,
	 *    it fails with cursetxt() and loss of mspec_used.
	 * -- monster isn't trying to attack.  Select a spell once.  Don't keep
	 *    searching; if that spell is not useful (or if it's directed),
	 *    return and do something else. 
	 * Since most spells are directed, this means that a monster that isn't
	 * attacking casts spells only a small portion of the time that an
	 * attacking monster does.
	 */
	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
	    int cnt = 40;

	    do {
			spellnum = rn2(ml);
			if (mattk->adtyp == AD_SPEL)
				spellnum = choose_magic_spell(mtmp,spellnum);
			else
				spellnum = choose_clerical_spell(mtmp,spellnum);
			/* not trying to attack?  don't allow directed spells */
			if (!thinks_it_foundyou) {
				if (!is_undirected_spell(mattk->adtyp, spellnum) || spell_would_be_useless(mtmp, mattk->adtyp, spellnum)) {
					if (foundyou)
						impossible("spellcasting monster found you and doesn't know it?");
					return 0;
				}
				break;
			}
			} while(--cnt > 0 &&
		    spell_would_be_useless(mtmp, mattk->adtyp, spellnum));
	    if (cnt == 0) return 0;
	}

	/* monster unable to cast spells? */
	if(mtmp->mcan || mtmp->mspec_used || !ml) {
	    cursetxt(mtmp, is_undirected_spell(mattk->adtyp, spellnum));
	    return(0);
	}

	if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
	    mtmp->mspec_used = 8 - mtmp->m_lev;
	    if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
		 /* your quest leader is a badass and does not need recharge time */
		 if (mtmp->data->msound == MS_LEADER) mtmp->mspec_used = 0;
	}

	/* monster can cast spells, but is casting a directed spell at the
	   wrong place?  If so, give a message, and return.  Do this *after*
	   penalizing mspec_used. */
	if (!foundyou && thinks_it_foundyou &&
		!is_undirected_spell(mattk->adtyp, spellnum)) {
	    pline("%s casts a spell at %s!",
		canseemon(mtmp) ? Monnam(mtmp) : "Something",
		levl[mtmp->mux][mtmp->muy].typ == WATER
		    ? "empty water" : "thin air");
	    return(0);
	}

	nomul(0);
	if(rn2(ml*10) < (mtmp->mconf ? 100 : 10)) {	/* fumbled attack */
	    if (flags.soundok) {
			 if (canseemon(mtmp)) {
				pline_The("air crackles around %s.", mon_nam(mtmp));
			 } else {
				 You("hear the air crackling with magical energy.");
			 }
		 }
	    return(0);
	}

	if (canspotmon(mtmp) || !is_undirected_spell(mattk->adtyp, spellnum)) {
	    pline("%s casts a spell%s!",
		  canspotmon(mtmp) ? Monnam(mtmp) : "Something",
		  is_undirected_spell(mattk->adtyp, spellnum) ? "" :
		  (Invisible && !perceives(mtmp->data) && 
		   (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  " at a spot near you" :
		  (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  " at your displaced image" :
		  " at you");
	}

/*
 *	As these are spells, the damage is related to the level
 *	of the monster casting the spell.
 */
	if (!foundyou) {
	    dmg = 0;
	    if (mattk->adtyp != AD_SPEL && mattk->adtyp != AD_CLRC) {
		impossible(
	      "%s casting non-hand-to-hand version of hand-to-hand spell %d?",
			   Monnam(mtmp), mattk->adtyp);
		return(0);
	    }
	} else if (mattk->damd)
	    dmg = d((int)((ml/2) + mattk->damn), (int)mattk->damd);
	else dmg = d((int)((ml/2) + 1), 6);
	if (Half_spell_damage) dmg = (dmg+1) / 2;

	ret = 1;

	switch (mattk->adtyp) {

	    case AD_FIRE:
		pline("You're enveloped in flames.");
		if (how_resistant(FIRE_RES) == 100) {
			shieldeff(u.ux, u.uy);
			pline("But you resist the effects.");
			monstseesu(M_SEEN_FIRE);
			dmg = 0;
		} else {
			dmg = resist_reduce(dmg,FIRE_RES);
		}
		burn_away_slime();
		break;
	    case AD_COLD:
		pline("You're covered in frost.");
		if(how_resistant(COLD_RES) == 100) {
			shieldeff(u.ux, u.uy);
			pline("But you resist the effects.");
			monstseesu(M_SEEN_COLD);
			dmg = 0;
		} else {
			dmg = resist_reduce(dmg,COLD_RES);
		}
		break;
	    case AD_MAGM:
		You("are hit by a shower of missiles!");
		dmg = d((int)mtmp->m_lev/2 + 1,6);
		if(Antimagic) {
			shieldeff(u.ux, u.uy);
			pline("Some of the missiles bounce off!");
			monstseesu(M_SEEN_MAGR);
			dmg /= 2;
		}
		break;
	    case AD_SPEL:	/* wizard spell */
	    case AD_CLRC:       /* clerical spell */
	    {
		if (mattk->adtyp == AD_SPEL)
		    cast_wizard_spell(mtmp, dmg, spellnum);
		else
		    cast_cleric_spell(mtmp, dmg, spellnum);
		dmg = 0; /* done by the spell casting functions */
		break;
	    }
	}
	if(dmg) mdamageu(mtmp, dmg);
	return(ret);
}

/* monster wizard and cleric spellcasting functions */
/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and spellnum should always be a valid
   undirected spell.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
STATIC_OVL
void
cast_wizard_spell(mtmp, dmg, spellnum)
struct monst *mtmp;
int dmg;
int spellnum;
{
	struct obj* oatmp;
	struct monst* mtmp2;
	int erodelvl;
	const char* desc = 0;
	int seen,count;
	struct edog* edog;

    if (dmg == 0 && !is_undirected_spell(AD_SPEL, spellnum)) {
	impossible("cast directed wizard spell (%d) with dmg=0?", spellnum);
	return;
    }

    switch (spellnum) {
    case MGC_DEATH_TOUCH:
	pline("Oh no, %s's using the touch of death!", mhe(mtmp));
	if (nonliving(youmonst.data) || is_demon(youmonst.data)) {
	    You("seem no deader than before.");
	} else {
	    if (Hallucination) {
			You("have an out of body experience.");
	    } else {
			dmg = d(8,6);
			/* Magic resistance or half spell damage will cut this in half... */
			if (Antimagic || Half_spell_damage) {
				shieldeff(u.ux, u.uy);
				monstseesu(M_SEEN_MAGR);
				dmg /= 2;
			}
			You("feel drained...");
			gainmaxhp(dmg/-2);
			losehp(dmg,"touch of death",KILLED_BY_AN);
	    }
	}
	dmg = 0;
	break;
    case MGC_CLONE_WIZ:
	if (mtmp->iswiz && flags.no_of_wizards == 1) {
	    pline("Double Trouble...");
	    clonewiz();
	    dmg = 0;
	} else
	    impossible("bad wizard cloning?");
	break;
	/* Inspire critters to fight a little more vigorously...
	 *
	 * -- Peaceful critters may become hostile.
	 * -- Hostile critters may become berserk.
	 * -- Borderline tame critters, or tame critters
	 *    who have been treated poorly may ALSO become hostile!
	 */
	 case MGC_ENRAGE:
		for (mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon) {
			if (m_cansee(mtmp,mtmp2->mx,mtmp2->my) && rn2(3) &&
					mtmp2 != mtmp && distu(mtmp2->mx,mtmp2->my) < 16) {
				seen++;
				if (mtmp2->mtame) {
					edog = (mtmp2->isminion) ? 0 : EDOG(mtmp2);
					if (mtmp2->mtame <= 3 || (edog && edog->abuse >= 5)) {
						mtmp2->mtame = mtmp2->mpeaceful = 0;
						if (mtmp2->mleashed) { m_unleash(mtmp2,FALSE); }
						count++;
					}
				} else if (mtmp2->mpeaceful) {
					mtmp2->mpeaceful = 0;
					count++;
				} else {
					mtmp2->mberserk = 1;
					count++;
				}
			}
		}
		/* Don't yell if we didn't see anyone to yell at. */
		if (seen && (!rn2(3) || mtmp->iswiz)) {
			verbalize("Get %s, you fools, or I'll have your figgin on a stick!",uhim());
		}
		if (count) {
			pline("It seems a little more dangerous here now...");
			doredraw();
		}
		dmg = 0;
		break;
    case MGC_SUMMON_MONS:
    {
	int count;

	count = nasty(mtmp,FALSE);	/* summon something nasty */
	if (mtmp->iswiz)
	    verbalize("Destroy the thief, my pet%s!", plur(count));
	else {
	    const char *mappear =
		(count == 1) ? "A monster appears" : "Monsters appear";

	    /* messages not quite right if plural monsters created but
	       only a single monster is seen */
	    if (Invisible && !perceives(mtmp->data) &&
				    (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("%s around a spot near you!", mappear);
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("%s around your displaced image!", mappear);
	    else
		pline("%s from nowhere!", mappear);
	}
	dmg = 0;
	break;
    }
    case MGC_AGGRAVATION:
	You_feel("that monsters are aware of your presence.");
	aggravate();
	dmg = 0;
	break;
    case MGC_CURSE_ITEMS:
	You_feel("as if you need some help.");
	rndcurse();
	dmg = 0;
	break;
    case MGC_DESTRY_ARMR:
	/* Magic resistance will reduce the amount by which your stuff is fiddled,
	 * but it won't actually stop things from being damaged */
	erodelvl = rnd(3);
	if (Antimagic) {
		shieldeff(u.ux, u.uy);
		monstseesu(M_SEEN_MAGR);
		erodelvl = 1;
	}
	oatmp = some_armor(&youmonst);
	if (oatmp) {
		if (oatmp->oerodeproof) {
			if (!Blind) {
				pline("Your %s glows brown for a moment.",xname(oatmp));
			}
			oatmp->oerodeproof = 0;
		}
		if (greatest_erosion(oatmp) == MAX_ERODE) {
			destroy_arm(oatmp);
		} else {
			while (erodelvl-- > 0) {
				_erode_obj(oatmp, AD_MAGM);
			}
		}
	} else {
		Your("skin itches.");
	}
	dmg = 0;
	break;
	 case MGC_FIRE_BOLT:
	 case MGC_ICE_BOLT:
		/* hotwire these to only go off if the critter can see you
		 * to avoid bugs WRT the Eyes and detect monsters */
		if (m_canseeu(mtmp)) {
			pline("%s blasts you with %s!",Monnam(mtmp), spellnum == MGC_FIRE_BOLT ? "fire" : "cold");
			explode(u.ux,u.uy,spellnum == MGC_FIRE_BOLT ? AD_FIRE-1 : AD_COLD-1,
					d((mtmp->m_lev/5)+1,8),WAND_CLASS,1);
		} else {
			if (canspotmon(mtmp)) {
				pline("%s blasts the %s with %s and curses!", Monnam(mtmp), rn2(2) ? "ceiling" : "floor",  
						spellnum == MGC_FIRE_BOLT ? "fire" : "cold");
			} else {
				You_hear("some cursing!");
			}
		}
		dmg = 0;
	break;
    case MGC_WEAKEN_YOU:		/* drain strength */
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
		 monstseesu(M_SEEN_MAGR);
	    You_feel("momentarily weakened.");
	} else {
	    You("suddenly feel weaker!");
	    dmg = mtmp->m_lev - 6;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    losestr(rnd(dmg));
	    if (u.uhp < 1)
		done_in_by(mtmp);
	}
	dmg = 0;
	break;
    case MGC_DISAPPEAR:		/* makes self invisible */
	if (!mtmp->minvis && !mtmp->invis_blkd) {
	    if (canseemon(mtmp))
		pline("%s suddenly %s!", Monnam(mtmp),
		      !See_invisible ? "disappears" : "becomes transparent");
	    mon_set_minvis(mtmp);
	    dmg = 0;
	} else
	    impossible("no reason for monster to cast disappear spell?");
	break;
	 case MGC_FLY:		 /* take off! */
		cast_fly(mtmp);
		break;
    case MGC_STUN_YOU:
	if (Antimagic || Free_action) {
	    shieldeff(u.ux, u.uy);
	    if (!Stunned)
		You_feel("momentarily disoriented.");
	    make_stunned(1L, FALSE);
	} else {
	    You(Stunned ? "struggle to keep your balance." : "reel...");
	    dmg = d(ACURR(A_DEX) < 12 ? 6 : 4, 4);
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    make_stunned(HStun + dmg, FALSE);
	}
	dmg = 0;
	break;
    case MGC_HASTE_SELF:
	mon_adjust_speed(mtmp, 1, (struct obj *)0);
	dmg = 0;
	break;
    case MGC_CURE_SELF:
	if (mtmp->mhp < mtmp->mhpmax) {
	    if (canseemon(mtmp))
		pline("%s looks better.", Monnam(mtmp));
	    /* note: player healing does 6d4; this used to do 1d8 */
	    if ((mtmp->mhp += d(3,6)) > mtmp->mhpmax)
		mtmp->mhp = mtmp->mhpmax;
	    dmg = 0;
	}
	break;
    case MGC_PSI_BOLT:
	/* prior to 3.4.0 Antimagic was setting the damage to 1--this
	   made the spell virtually harmless to players with magic res. */
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
		 monstseesu(M_SEEN_MAGR);
	    dmg = (dmg + 1) / 2;
	} 
	/* Tinfoil hat? ;) */
	if (uarmh) {
		desc = OBJ_DESCR(objects[uarmh->otyp]);
		if (desc && strstri(desc,"tinfoil")) {
			shieldeff(u.ux, u.uy);
			dmg = 0;
			Your("%s tingles.%s", body_part(HEAD), Hallucination ?
					" You think your dandruff might be clearing up." : "");
			break;
		}
	}
	if (dmg <= 5)
	    You("get a slight %sache.", body_part(HEAD));
	else if (dmg <= 10)
	    Your("brain is on fire!");
	else if (dmg <= 20)
	    Your("%s suddenly aches painfully!", body_part(HEAD));
	else
	    Your("%s suddenly aches very painfully!", body_part(HEAD));
	break;
    default:
	impossible("mcastu: invalid magic spell (%d)", spellnum);
	dmg = 0;
	break;
    }

    if (dmg) mdamageu(mtmp, dmg);
}

const char* vulntext[] = {
	"chartreuse polka-dot",
	"reddish-orange",
	"purplish-blue",
	"coppery-yellow",
	"greenish-mottled"
};

STATIC_OVL
void
cast_cleric_spell(mtmp, dmg, spellnum)
struct monst *mtmp;
int dmg;
int spellnum;
{
	int aligntype;

    if (dmg == 0 && !is_undirected_spell(AD_CLRC, spellnum)) {
	impossible("cast directed cleric spell (%d) with dmg=0?", spellnum);
	return;
    }

    switch (spellnum) {
	 case CLC_SUMMON_ELM:
		 if (mtmp->ispriest) {
			 aligntype = EPRI(mtmp)->shralign;
		 } else {
			 /* uh, who fed the orc shaman so many gain level potions? */
			 aligntype = (int)mtmp->data->maligntyp;
		 }
		 pline("A servant of %s appears!",aligns[1 - aligntype].noun);
		summon_minion(aligntype, TRUE);
		 break;
    case CLC_GEYSER:
	/* this is physical damage, not magical damage */
	pline("A sudden geyser slams into you from nowhere!");
	dmg = d(8, 6);
	if (Half_physical_damage) dmg = (dmg + 1) / 2;
	break;
    case CLC_FIRE_PILLAR:
	pline("A pillar of fire strikes all around you!");
	if (how_resistant(FIRE_RES) == 100) {
	    shieldeff(u.ux, u.uy);
		 monstseesu(M_SEEN_FIRE);
	    dmg = 0;
	} else
	    dmg = resist_reduce(d(8,6),FIRE_RES);
	if (Half_spell_damage) dmg = (dmg + 1) / 2;
	burn_away_slime();
	(void) burnarmor(&youmonst);
	destroy_item(SCROLL_CLASS, AD_FIRE);
	destroy_item(POTION_CLASS, AD_FIRE);
	destroy_item(SPBOOK_CLASS, AD_FIRE);
	(void) burn_floor_paper(u.ux, u.uy, TRUE, FALSE);
	break;
    case CLC_LIGHTNING:
    {
		boolean reflects;
		pline("A bolt of lightning strikes down at you from above!");
		reflects = ureflects("Some of it bounces off your %s%s.", "");
		if (reflects || (how_resistant(SHOCK_RES) == 100)) {
			shieldeff(u.ux, u.uy);
			if (reflects) {
				dmg = resist_reduce(d(4,6),SHOCK_RES);
				monstseesu(M_SEEN_REFL);
			}
			if (how_resistant(SHOCK_RES) == 100) {
				pline("You aren't shocked.");
				dmg = 0;
				monstseesu(M_SEEN_ELEC);
			}
		} else {
			dmg = resist_reduce(d(8,6),SHOCK_RES);
		}
		if (dmg && Half_spell_damage)  { dmg = (dmg + 1) / 2; }
		if (!reflects) {
			destroy_item(WAND_CLASS, AD_ELEC);	 /* reflection protects items */
			destroy_item(RING_CLASS, AD_ELEC);
		}
		break;
    }
    case CLC_CURSE_ITEMS:
	You_feel("as if you need some help.");
	rndcurse();
	dmg = 0;
	break;
    case CLC_INSECTS:
      {
	/* Try for insects, and if there are none
	   left, go for (sticks to) snakes.  -3. */
	struct permonst *pm = mkclass(S_ANT,0);
	struct monst *mtmp2 = (struct monst *)0;
	char let = (pm ? S_ANT : S_SNAKE);
	boolean success;
	int i;
	coord bypos;
	int quan;

	quan = (mtmp->m_lev < 2) ? 1 : rnd((int)mtmp->m_lev / 2);
	if (quan < 3) quan = 3;
	success = pm ? TRUE : FALSE;
	for (i = 0; i <= quan; i++) {
	    if (!enexto(&bypos, mtmp->mux, mtmp->muy, mtmp->data))
		break;
	    if ((pm = mkclass(let,0)) != 0 &&
		    (mtmp2 = makemon(pm, bypos.x, bypos.y, NO_MM_FLAGS)) != 0) {
		success = TRUE;
		mtmp2->msleeping = mtmp2->mpeaceful = mtmp2->mtame = 0;
		set_malign(mtmp2);
	    }
	}
	/* Not quite right:
         * -- message doesn't always make sense for unseen caster (particularly
	 *    the first message)
         * -- message assumes plural monsters summoned (non-plural should be
         *    very rare, unlike in nasty())
         * -- message assumes plural monsters seen
         */
	if (!success)
	    pline("%s casts at a clump of sticks, but nothing happens.",
		Monnam(mtmp));
	else if (let == S_SNAKE)
	    pline("%s transforms a clump of sticks into snakes!",
		Monnam(mtmp));
	else if (Invisible && !perceives(mtmp->data) &&
				(mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline("%s summons insects around a spot near you!",
		Monnam(mtmp));
	else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline("%s summons insects around your displaced image!",
		Monnam(mtmp));
	else
	    pline("%s summons insects!", Monnam(mtmp));
	dmg = 0;
	break;
      }
    case CLC_BLIND_YOU:
	/* note: resists_blnd() doesn't apply here */
	if (!Blinded) {
	    int num_eyes = eyecount(youmonst.data);
	    pline("Scales cover your %s!",
		  (num_eyes == 1) ?
		  body_part(EYE) : makeplural(body_part(EYE)));
	    make_blinded(Half_spell_damage ? 100L : 200L, FALSE);
	    if (!Blind) Your(vision_clears);
	    dmg = 0;
	} else
	    impossible("no reason for monster to cast blindness spell?");
	break;
    case CLC_PARALYZE:
	if (Antimagic || Free_action) {
	    shieldeff(u.ux, u.uy);
	    if (multi >= 0)
		You("stiffen briefly.");
	    nomul(-1);
	} else {
	    if (multi >= 0)
		You("are frozen in place!");
	    dmg = 4 + (int)mtmp->m_lev;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    nomul(-dmg);
	}
	dmg = 0;
	break;
    case CLC_CONFUSE_YOU:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
		 monstseesu(M_SEEN_MAGR);
	    You_feel("momentarily dizzy.");
	} else {
	    boolean oldprop = !!Confusion;

	    dmg = (int)mtmp->m_lev;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    make_confused(HConfusion + dmg, TRUE);
	    if (Hallucination)
		You_feel("%s!", oldprop ? "trippier" : "trippy");
	    else
		You_feel("%sconfused!", oldprop ? "more " : "");
	}
	dmg = 0;
	break;
    case CLC_CURE_SELF:
	if (mtmp->mhp < mtmp->mhpmax) {
	    if (canseemon(mtmp))
		pline("%s looks better.", Monnam(mtmp));
	    /* note: player healing does 6d4; this used to do 1d8 */
	    if ((mtmp->mhp += d(3,6)) > mtmp->mhpmax)
		mtmp->mhp = mtmp->mhpmax;
	    dmg = 0;
	}
	break;
	 case CLC_FLY:
		cast_fly(mtmp);
		break;
	 case CLC_VULN_YOU:
		dmg = rnd(4);
		pline("A %s film oozes over your skin!",Blind ? "slimy" : vulntext[dmg]);
		switch (dmg) {
			case 1:
				if (Vulnerable_fire) return;
				incr_itimeout(&HVulnerable_fire,rnd(100)+150);
				You_feel("more inflammable.");
				break;
			case 2:
				if (Vulnerable_cold) return;
				incr_itimeout(&HVulnerable_cold,rnd(100)+150);
				You_feel("like Jack Frost is out to get you.");
				break;
			case 3:
				if (Vulnerable_elec) return;
				incr_itimeout(&HVulnerable_elec,rnd(100)+150);
				You_feel("overly conductive.");
				break;
			case 4:
				if (Vulnerable_acid) return;
				incr_itimeout(&HVulnerable_acid,rnd(100)+150);
				You_feel("easily corrodable.");
				break;
			default:
				break;
		}
		break;
    case CLC_OPEN_WOUNDS:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
		 monstseesu(M_SEEN_MAGR);
	    dmg = (dmg + 1) / 2;
	}
	if (dmg <= 5)
	    Your("skin itches badly for a moment.");
	else if (dmg <= 10)
	    pline("Wounds appear on your body!");
	else if (dmg <= 20)
	    pline("Severe wounds appear on your body!");
	else
	    Your("body is covered with painful wounds!");
	break;
    default:
	impossible("mcastu: invalid clerical spell (%d)", spellnum);
	dmg = 0;
	break;
    }

    if (dmg) mdamageu(mtmp, dmg);
}

STATIC_DCL
boolean
is_undirected_spell(adtyp, spellnum)
unsigned int adtyp;
int spellnum;
{
    if (adtyp == AD_SPEL) {
	switch (spellnum) {
	case MGC_CLONE_WIZ:
	case MGC_SUMMON_MONS:
	case MGC_AGGRAVATION:
	case MGC_DISAPPEAR:
	case MGC_HASTE_SELF:
	case MGC_CURE_SELF:
	case MGC_FLY:
	case MGC_ENRAGE:
	case MGC_FIRE_BOLT:
	case MGC_ICE_BOLT:
	    return TRUE;
	default:
	    break;
	}
    } else if (adtyp == AD_CLRC) {
	switch (spellnum) {
	case CLC_INSECTS:
	case CLC_CURE_SELF:
	case CLC_FLY:
	    return TRUE;
	default:
	    break;
	}
    }
    return FALSE;
}

STATIC_DCL
void
cast_fly(mtmp)
struct monst* mtmp;
{
	struct trap* tr;
	char msg[200];

	if (!mtmp->mflying) {
		mtmp->mflying = TRUE;
		if (mtmp->mtrapped) {
			tr = t_at(mtmp->mx,mtmp->my);
			if (tr && (tr->ttyp == PIT || tr->ttyp == SPIKED_PIT)) {
				sprintf(msg,"%s rises up, out of the pit!",Monnam(mtmp));
				mtmp->mtrapped = FALSE;
			}
		} else {
			sprintf(msg,"%s rises into the air!",Monnam(mtmp));
		}
		if (canseemon(mtmp)) { pline(msg); }
	} else {
		impossible("No reason to cast 'fly' spell?");
	}
}

/* Some spells are useless under some circumstances. */
STATIC_DCL
boolean
spell_would_be_useless(mtmp, adtyp, spellnum)
struct monst *mtmp;
unsigned int adtyp;
int spellnum;
{
    /* Some spells don't require the player to really be there and can be cast
     * by the monster when you're invisible, yet still shouldn't be cast when
     * the monster doesn't even think you're there.
     * This check isn't quite right because it always uses your real position.
     * We really want something like "if the monster could see mux, muy".
     */
    boolean mcouldseeu = couldsee(mtmp->mx, mtmp->my);

    if (adtyp == AD_SPEL) {
		/* aggravate monsters, etc. won't be cast by peaceful monsters */
		if (mtmp->mpeaceful && (spellnum == MGC_AGGRAVATION ||
			spellnum == MGC_SUMMON_MONS || spellnum == MGC_CLONE_WIZ))
			return TRUE;
		/* haste self when already fast */
		if (mtmp->permspeed == MFAST && spellnum == MGC_HASTE_SELF)
			return TRUE;
		/* invisibility when already invisible */
		if ((mtmp->minvis || mtmp->invis_blkd) && spellnum == MGC_DISAPPEAR)
			return TRUE;
		/* peaceful monster won't cast invisibility if you can't see invisible,
			same as when monsters drink potions of invisibility.  This doesn't
			really make a lot of sense, but lets the player avoid hitting
			peaceful monsters by mistake */
		if (mtmp->mpeaceful && !See_invisible && spellnum == MGC_DISAPPEAR)
			return TRUE;
		/* healing when already healed */
		if (mtmp->mhp == mtmp->mhpmax && spellnum == MGC_CURE_SELF)
			return TRUE;
		/* don't summon monsters if it doesn't think you're around */
		if (!mcouldseeu && (spellnum == MGC_SUMMON_MONS ||
			(!mtmp->iswiz && spellnum == MGC_CLONE_WIZ)))
			return TRUE;
		if ((!mtmp->iswiz || flags.no_of_wizards > 1)
							&& spellnum == MGC_CLONE_WIZ)
			return TRUE;
		/* don't lift off if we're already in the air or just-peaceful
		* ...but go ahead and let pets do it */
		if ((is_flyer(mtmp->data) || is_flying(mtmp) || 
					(mtmp->mpeaceful && !mtmp->mtame)) && spellnum == MGC_FLY)
			return TRUE;
		/* Don't go making everything else bonkers if you're peaceful! */
		if (spellnum == MGC_ENRAGE && (mtmp->mpeaceful || mtmp->mtame)) {
			return TRUE;
		}
		/* Don't waste time zapping resisted spells at the player,
		* and don't blast ourselves with our own explosions */
		if ((m_seenres(mtmp,M_SEEN_FIRE) || distu(mtmp->mx,mtmp->my) < 2) && 
				spellnum == MGC_FIRE_BOLT) {
			return TRUE;
		}
		if ((m_seenres(mtmp,M_SEEN_COLD) || distu(mtmp->mx,mtmp->my) < 2) && 
				spellnum == MGC_ICE_BOLT) {
			return TRUE;
		}
		if ((spellnum == MGC_ICE_BOLT || spellnum == MGC_FIRE_BOLT) && mtmp->mpeaceful)
		{
			return TRUE;
		}
    } else if (adtyp == AD_CLRC) {
		/* summon insects/sticks to snakes won't be cast by peaceful monsters */
		if (mtmp->mpeaceful && spellnum == CLC_INSECTS)
			return TRUE;
		/* healing when already healed */
		if (mtmp->mhp == mtmp->mhpmax && spellnum == CLC_CURE_SELF)
			return TRUE;
		/* don't summon insects if it doesn't think you're around */
		if (!mcouldseeu && spellnum == CLC_INSECTS)
			return TRUE;
		/* blindness spell on blinded player */
		if (Blinded && spellnum == CLC_BLIND_YOU)
			return TRUE;
		/* don't lift off if we're already in the air or just-peaceful
		* ...but go ahead and let pets do it */
		if ((is_flyer(mtmp->data) || is_flying(mtmp) || 
					(mtmp->mpeaceful && !mtmp->mtame)) && spellnum == CLC_FLY)
			return TRUE;
    }
    return FALSE;
}

#endif /* OVLB */
#ifdef OVL0

/* convert 1..10 to 0..9; add 10 for second group (spell casting) */
#define ad_to_typ(k) (10 + (int)k - 1)

int
buzzmu(mtmp, mattk)		/* monster uses spell (ranged) */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	/* don't print constant stream of curse messages for 'normal'
	   spellcasting monsters at range */
	if (mattk->adtyp > AD_SPC2)
	    return(0);

	if (mtmp->mcan) {
	    cursetxt(mtmp, FALSE);
	    return(0);
	}
	if(lined_up(mtmp) && rn2(3)) {
	    nomul(0);
	    if(mattk->adtyp && (mattk->adtyp < 11)) { /* no cf unsigned >0 */
		if(canseemon(mtmp))
		    pline("%s zaps you with a %s!", Monnam(mtmp),
			  flash_types[ad_to_typ(mattk->adtyp)]);
		buzz(-ad_to_typ(mattk->adtyp), (int)mattk->damn,
		     mtmp->mx, mtmp->my, sgn(tbx), sgn(tby));
	    } else impossible("Monster spell %d cast", mattk->adtyp-1);
	}
	return(1);
}

#endif /* OVL0 */

/*mcastu.c*/
