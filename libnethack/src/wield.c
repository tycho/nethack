/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2013-09-21 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* KMH -- Differences between the three weapon slots.
 *
 * The main weapon (uwep):
 * 1.  Is filled by the (w)ield command.
 * 2.  Can be filled with any type of item.
 * 3.  May be carried in one or both hands.
 * 4.  Is used as the melee weapon and as the launcher for
 *     ammunition.
 * 5.  Only conveys intrinsics when it is a weapon, weapon-tool,
 *     or artifact.
 * 6.  Certain cursed items will weld to the hand and cannot be
 *     unwielded or dropped.  See erodeable_wep() and will_weld()
 *     below for the list of which items apply.
 *
 * The secondary weapon (uswapwep):
 * 1.  Is filled by the e(x)change command, which swaps this slot
 *     with the main weapon.  If the "pushweapon" option is set,
 *     the (w)ield command will also store the old weapon in the
 *     secondary slot.
 * 2.  Can be field with anything that will fit in the main weapon
 *     slot; that is, any type of item.
 * 3.  Is usually NOT considered to be carried in the hands.
 *     That would force too many checks among the main weapon,
 *     second weapon, shield, gloves, and rings; and it would
 *     further be complicated by bimanual weapons.  A special
 *     exception is made for two-weapon combat.
 * 4.  Is used as the second weapon for two-weapon combat, and as
 *     a convenience to swap with the main weapon.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld (see #3 for reasons), but they also
 *     prevent two-weapon combat.
 *
 * The quiver (uquiver):
 * 1.  Is filled by the (Q)uiver command.
 * 2.  Can be filled with any type of item.
 * 3.  Is considered to be carried in a special part of the pack.
 * 4.  Is used as the item to throw with the (f)ire command.
 *     This is a convenience over the normal (t)hrow command.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld; their effect is handled by the normal
 *     throwing code.
 *
 * No item may be in more than one of these slots.
 */


static int ready_weapon(struct obj *);

/* used by will_weld() */
/* probably should be renamed */
#define erodeable_wep(optr) ((optr)->oclass == WEAPON_CLASS \
                                || is_weptool(optr) \
                                || (optr)->otyp == HEAVY_IRON_BALL \
                                || (optr)->otyp == IRON_CHAIN)

/* used by welded(), and also while wielding */
#define will_weld(optr)     ((optr)->cursed \
                                && (erodeable_wep(optr) \
                                   || (optr)->otyp == TIN_OPENER))


/*** Functions that place a given item in a slot ***/
/* Proper usage includes:
 * 1.  Initializing the slot during character generation or a
 *     restore.
 * 2.  Setting the slot due to a player's actions.
 * 3.  If one of the objects in the slot are split off, these
 *     functions can be used to put the remainder back in the slot.
 * 4.  Putting an item that was thrown and returned back into the slot.
 * 5.  Emptying the slot, by passing a null object.  NEVER pass
 *     zeroobj!
 *
 * If the item is being moved from another slot, it is the caller's
 * responsibility to handle that.  It's also the caller's responsibility
 * to print the appropriate messages.
 */
void
setuwep(struct obj *obj)
{
    struct obj *olduwep = uwep;

    if (obj == uwep)
        return; /* necessary to not set unweapon */
    /* This message isn't printed in the caller because it happens *whenever*
       Sunsword is unwielded, from whatever cause. */
    setworn(obj, W_WEP);
    if (uwep == obj && artifact_light(olduwep) && olduwep->lamplit) {
        end_burn(olduwep, FALSE);
        if (!Blind)
            pline("%s glowing.", Tobjnam(olduwep, "stop"));
    }
    /* Note: Explicitly wielding a pick-axe will not give a "bashing" message.
       Wielding one via 'a'pplying it will. 3.2.2: Wielding arbitrary objects
       will give bashing message too. */
    if (obj) {
        unweapon = (obj->oclass == WEAPON_CLASS) ? is_launcher(obj) ||
            is_ammo(obj) || is_missile(obj) || (is_pole(obj)
                                                && !u.
                                                usteed) : !is_weptool(obj);
    } else
        unweapon = TRUE;        /* for "bare hands" message */
    update_inventory();
}

static int
ready_weapon(struct obj *wep)
{
    /* Separated function so swapping works easily */
    int res = 0;

    if (!wep) {
        /* No weapon */
        if (uwep) {
            pline("You are empty %s.", body_part(HANDED));
            setuwep(NULL);
            res++;
        } else
            pline("You are already empty %s.", body_part(HANDED));
    } else if (!uarmg && !Stone_resistance && wep->otyp == CORPSE &&
               touch_petrifies(&mons[wep->corpsenm])) {
        /* Prevent wielding cockatrice when not wearing gloves --KAA */
        char kbuf[BUFSZ];

        pline("You wield the %s corpse in your bare %s.",
              mons[wep->corpsenm].mname, makeplural(body_part(HAND)));
        sprintf(kbuf, "wielding %s corpse without gloves",
                an(mons[wep->corpsenm].mname));
        instapetrify(kbuf);
    } else if (uarms && bimanual(wep))
        pline("You cannot wield a two-handed %s while wearing a shield.",
              is_sword(wep) ? "sword" : wep->otyp ==
              BATTLE_AXE ? "axe" : "weapon");
    else if (wep->oartifact && !touch_artifact(wep, &youmonst)) {
        res++;  /* takes a turn even though it doesn't get wielded */
    } else {
        /* Weapon WILL be wielded after this point */
        res++;
        if (will_weld(wep)) {
            const char *tmp = xname(wep), *thestr = "The ";

            if (strncmp(tmp, thestr, 4) && !strncmp(The(tmp), thestr, 4))
                tmp = thestr;
            else
                tmp = "";
            pline("%s%s %s to your %s!", tmp,
                  aobjnam(wep, "weld"),
                  (wep->quan == 1L) ? "itself" : "themselves", /* a3 */
                  bimanual(wep) ? (const char *)makeplural(body_part(HAND))
                  : body_part(HAND));
            wep->bknown = TRUE;
        } else {
            /* The message must be printed before setuwep (since you might die
               and be revived from changing weapons), and the message must be
               before the death message and Lifesaved rewielding.  Yet we want
               the message to say "weapon in hand", thus this kludge. */
            long dummy = wep->owornmask;

            wep->owornmask |= W_WEP;
            prinv(NULL, wep, 0L);
            wep->owornmask = dummy;
        }
        setuwep(wep);

        /* KMH -- Talking artifacts are finally implemented */
        arti_speak(wep);

        if (artifact_light(wep) && !wep->lamplit) {
            begin_burn(wep, FALSE);
            if (!Blind)
                pline("%s to glow brilliantly!", Tobjnam(wep, "begin"));
        }

        if (wep->unpaid) {
            struct monst *this_shkp;

            if ((this_shkp =
                 shop_keeper(level, inside_shop(level, u.ux, u.uy))) != NULL) {
                pline("%s says \"You be careful with my %s!\"",
                      shkname(this_shkp), xname(wep));
            }
        }
    }
    return res;
}

void
setuqwep(struct obj *obj)
{
    setworn(obj, W_QUIVER);
    update_inventory();
}

void
setuswapwep(struct obj *obj)
{
    setworn(obj, W_SWAPWEP);
    update_inventory();
}


/*** Commands to change particular slot(s) ***/

static const char wield_objs[] =
    { ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, TOOL_CLASS, 0 };
static const char ready_objs[] = { ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, 0 };

static const char bullets[] =   /* (note: different from dothrow.c) */
{ ALL_CLASSES, ALLOW_NONE, GEM_CLASS, WEAPON_CLASS, 0 };

int
dowield(struct obj *wep)
{
    struct obj *oldwep;
    int result;

    /* May we attempt this? */
    multi = 0;
    if (cantwield(youmonst.data)) {
        pline("Don't be ridiculous!");
        return 0;
    }

    /* Prompt for a new weapon */
    if (wep && !validate_object(wep, wield_objs, "wield"))
        return 0;
    else if (!wep)
        wep = getobj(wield_objs, "wield");
    if (!wep)
        /* Cancelled */
        return 0;
    else if (wep == uwep) {
        pline("You are already wielding that!");
        if (is_weptool(wep))
            unweapon = FALSE;   /* [see setuwep()] */
        return 0;
    } else if (welded(uwep)) {
        weldmsg(uwep);
        /* previously interrupted armor removal mustn't be resumed */
        reset_remarm();
        return 0;
    }

    /* Handle no object, or object in other slot */
    if (wep == &zeroobj)
        wep = NULL;
    else if (wep == uswapwep)
        return doswapweapon();
    else if (wep == uquiver)
        setuqwep(NULL);
    else if (wep->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_SADDLE)) {
        pline("You cannot wield that!");
        return 0;
    }

    /* Set your new primary weapon */
    oldwep = uwep;
    result = ready_weapon(wep);
    if (flags.pushweapon && oldwep && uwep != oldwep)
        setuswapwep(oldwep);
    untwoweapon();

    return result;
}

int
doswapweapon(void)
{
    struct obj *oldwep, *oldswap;

    /* May we attempt this? */
    multi = 0;
    if (cantwield(youmonst.data)) {
        pline("Don't be ridiculous!");
        return 0;
    }
    if (welded(uwep)) {
        weldmsg(uwep);
        return 0;
    }

    /* Unwield your current secondary weapon */
    oldwep = uwep;
    oldswap = uswapwep;
    setuswapwep(NULL);

    /* Set your new primary weapon */
    ready_weapon(oldswap);

    /* Set your new secondary weapon */
    if (uwep == oldwep)
        /* Wield failed for some reason */
        setuswapwep(oldswap);
    else {
        setuswapwep(oldwep);
        if (uswapwep)
            prinv(NULL, uswapwep, 0L);
        else
            pline("You have no secondary weapon readied.");
    }

    if (u.twoweap && !can_twoweapon())
        untwoweapon();

    return 0;
}

int
dowieldquiver(struct obj *newquiver)
{
    const char *quivee_types = (uslinging() ||
                                (uswapwep &&
                                 objects[uswapwep->otyp].oc_skill ==
                                 P_SLING)) ? bullets : ready_objs;

    /* Since the quiver isn't in your hands, don't check cantwield(), */
    /* will_weld(), touch_petrifies(), etc. */
    multi = 0;

    if (newquiver && !validate_object(newquiver, quivee_types, "ready"))
        return 0;
    else if (!newquiver)
        /* Prompt for a new quiver */
        newquiver = getobj(quivee_types, "ready");
    if (!newquiver)
        /* Cancelled */
        return 0;

    /* Handle no object, or object in other slot */
    /* Any type is okay, since we give no intrinsics anyways */
    if (newquiver == &zeroobj) {
        /* Explicitly nothing */
        if (uquiver) {
            pline("You now have no ammunition readied.");
            setuqwep(newquiver = NULL);
        } else {
            pline("You already have no ammunition readied!");
            return 0;
        }
    } else if (newquiver == uquiver) {
        pline("That ammunition is already readied!");
        return 0;
    } else if (newquiver == uwep) {
        /* Prevent accidentally readying the main weapon */
        pline("%s already being used as a weapon!",
              !is_plural(uwep) ? "That is" : "They are");
        return 0;
    } else if (newquiver->
               owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_SADDLE)) {
        pline("You cannot ready that!");
        return 0;
    } else {
        long dummy;


        /* Check if it's the secondary weapon */
        if (newquiver == uswapwep) {
            setuswapwep(NULL);
            untwoweapon();
        }

        /* Okay to put in quiver; print it */
        dummy = newquiver->owornmask;
        newquiver->owornmask |= W_QUIVER;
        prinv(NULL, newquiver, 0L);
        newquiver->owornmask = dummy;
    }

    /* Finally, place it in the quiver */
    setuqwep(newquiver);
    /* Take no time since this is a convenience slot */
    return 0;
}

/* used for #rub and for applying pick-axe, whip, grappling hook, or polearm */
/* (moved from apply.c) */
boolean
wield_tool(struct obj * obj, const char *verb)
{       /* "rub",&c */
    const char *what;
    boolean more_than_1;

    if (obj == uwep)
        return TRUE;    /* nothing to do if already wielding it */

    if (!verb)
        verb = "wield";
    what = xname(obj);
    more_than_1 = (obj->quan > 1L || strstri(what, "pair of ") != 0 ||
                   strstri(what, "s of ") != 0);

    if (obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL)) {
        char yourbuf[BUFSZ];

        pline("You can't %s %s %s while wearing %s.", verb,
              shk_your(yourbuf, obj), what, more_than_1 ? "them" : "it");
        return FALSE;
    }
    if (welded(uwep)) {
        if (flags.verbose) {
            const char *hand = body_part(HAND);

            if (bimanual(uwep))
                hand = makeplural(hand);
            if (strstri(what, "pair of ") != 0)
                more_than_1 = FALSE;
            pline
                ("Since your weapon is welded to your %s, you cannot %s %s %s.",
                 hand, verb, more_than_1 ? "those" : "that", xname(obj));
        } else {
            pline("You can't do that.");
        }
        return FALSE;
    }
    if (cantwield(youmonst.data)) {
        pline("You can't hold %s strongly enough.",
              more_than_1 ? "them" : "it");
        return FALSE;
    }
    /* check shield */
    if (uarms && bimanual(obj)) {
        pline("You cannot %s a two-handed %s while wearing a shield.", verb,
              (obj->oclass == WEAPON_CLASS) ? "weapon" : "tool");
        return FALSE;
    }
    if (uquiver == obj)
        setuqwep(NULL);
    if (uswapwep == obj) {
        doswapweapon();
        /* doswapweapon might fail */
        if (uswapwep == obj)
            return FALSE;
    } else {
        pline("You now wield %s.", doname(obj));
        setuwep(obj);
    }
    if (uwep != obj)
        return FALSE;   /* rewielded old object after dying */
    /* applying weapon or tool that gets wielded ends two-weapon combat */
    if (u.twoweap)
        untwoweapon();
    if (obj->oclass != WEAPON_CLASS)
        unweapon = TRUE;
    return TRUE;
}

int
can_twoweapon(void)
{
    struct obj *otmp;

#define NOT_WEAPON(obj) (!is_weptool(obj) && obj->oclass != WEAPON_CLASS)
    if (!could_twoweap(youmonst.data)) {
        if (Upolyd)
            pline("You can't use two weapons in your current form.");
        else
            pline("%s aren't able to use two weapons at once.",
                  makeplural((flags.female &&
                              urole.name.f) ? urole.name.f : urole.name.m));
    } else if (!uwep || !uswapwep)
        pline("Your %s%s%s empty.", uwep ? "left " : uswapwep ? "right " : "",
              body_part(HAND), (!uwep && !uswapwep) ? "s are" : " is");
    else if (NOT_WEAPON(uwep) || NOT_WEAPON(uswapwep)) {
        otmp = NOT_WEAPON(uwep) ? uwep : uswapwep;
        pline("%s %s.", Yname2(otmp),
              is_plural(otmp) ? "aren't weapons" : "isn't a weapon");
    } else if (bimanual(uwep) || bimanual(uswapwep)) {
        otmp = bimanual(uwep) ? uwep : uswapwep;
        pline("%s isn't one-handed.", Yname2(otmp));
    } else if (uarms)
        pline("You can't use two weapons while wearing a shield.");
    else if (uswapwep->oartifact)
        pline("%s %s being held second to another weapon!", Yname2(uswapwep),
              otense(uswapwep, "resist"));
    else if (!uarmg && !Stone_resistance &&
             (uswapwep->otyp == CORPSE &&
              touch_petrifies(&mons[uswapwep->corpsenm]))) {
        char kbuf[BUFSZ];

        pline("You wield the %s corpse with your bare %s.",
              mons[uswapwep->corpsenm].mname, body_part(HAND));
        sprintf(kbuf, "wielding %s corpse without gloves",
                an(mons[uswapwep->corpsenm].mname));
        instapetrify(kbuf);
    } else if (Glib || uswapwep->cursed) {
        if (!Glib)
            uswapwep->bknown = TRUE;
        drop_uswapwep();
    } else
        return TRUE;
    return FALSE;
}

void
drop_uswapwep(void)
{
    char str[BUFSZ];
    struct obj *obj = uswapwep;

    /* Avoid trashing makeplural's static buffer */
    strcpy(str, makeplural(body_part(HAND)));
    pline("Your %s from your %s!", aobjnam(obj, "slip"), str);
    dropx(obj);
}

int
dotwoweapon(void)
{
    /* You can always toggle it off */
    if (u.twoweap) {
        pline("You switch to your primary weapon.");
        u.twoweap = 0;
        update_inventory();
        return 0;
    }

    /* May we use two weapons? */
    if (can_twoweapon()) {
        /* Success! */
        pline("You begin two-weapon combat.");
        u.twoweap = 1;
        update_inventory();
        return rnd(20) > ACURR(A_DEX);
    }
    return 0;
}

/*** Functions to empty a given slot ***/
/* These should be used only when the item can't be put back in
 * the slot by life saving.  Proper usage includes:
 * 1.  The item has been eaten, stolen, burned away, or rotted away.
 * 2.  Making an item disappear for a bones pile.
 */
void
uwepgone(void)
{
    if (uwep) {
        if (artifact_light(uwep) && uwep->lamplit) {
            end_burn(uwep, FALSE);
            if (!Blind)
                pline("%s glowing.", Tobjnam(uwep, "stop"));
        }
        setworn(NULL, W_WEP);
        unweapon = TRUE;
        update_inventory();
    }
}

void
uswapwepgone(void)
{
    if (uswapwep) {
        setworn(NULL, W_SWAPWEP);
        update_inventory();
    }
}

void
uqwepgone(void)
{
    if (uquiver) {
        setworn(NULL, W_QUIVER);
        update_inventory();
    }
}

void
untwoweapon(void)
{
    if (u.twoweap) {
        pline("You can no longer use two weapons at once.");
        u.twoweap = FALSE;
        update_inventory();
    }
    return;
}

/* Maybe rust object, or corrode it if acid damage is called for */
void
erode_obj(struct obj *target,   /* object (e.g. weapon or armor) to erode */
          boolean acid_dmg, boolean fade_scrolls)
{
    int erosion;
    struct monst *victim;
    boolean vismon;
    boolean visobj;

    if (!target)
        return;
    victim =
        carried(target) ? &youmonst : mcarried(target) ? target->ocarry : NULL;
    vismon = victim && (victim != &youmonst) && canseemon(victim);
    visobj = !victim && cansee(bhitpos.x, bhitpos.y);   /* assume thrown */

    erosion = acid_dmg ? target->oeroded2 : target->oeroded;

    if (target->greased) {
        grease_protect(target, NULL, victim);
    } else if (target->oclass == SCROLL_CLASS) {
        if (fade_scrolls && target->otyp != SCR_BLANK_PAPER) {
            if (!Blind) {
                if (victim == &youmonst)
                    pline("Your %s.", aobjnam(target, "fade"));
                else if (vismon)
                    pline("%s's %s.", Monnam(victim), aobjnam(target, "fade"));
                else if (visobj)
                    pline("The %s.", aobjnam(target, "fade"));
            }
            target->otyp = SCR_BLANK_PAPER;
            target->spe = 0;
        }
        return;
    } else if (target->oerodeproof ||
               (acid_dmg ? !is_corrodeable(target) : !is_rustprone(target))) {
        if (flags.verbose || !(target->oerodeproof && target->rknown)) {
            if (victim == &youmonst)
                pline("Your %s not affected.", aobjnam(target, "are"));
            else if (vismon)
                pline("%s's %s not affected.", Monnam(victim),
                      aobjnam(target, "are"));
            /* no message if not carried */
        }
        if (target->oerodeproof)
            target->rknown = TRUE;
        return;
    }
    if (target->unpaid)
        costly_damage_obj(target);
    if (erosion < MAX_ERODE) {
        if (victim == &youmonst)
            pline("Your %s%s!", aobjnam(target, acid_dmg ? "corrode" : "rust"),
                  erosion + 1 ==
                  MAX_ERODE ? " completely" : erosion ? " further" : "");
        else if (vismon)
            pline("%s's %s%s!", Monnam(victim),
                  aobjnam(target, acid_dmg ? "corrode" : "rust"),
                  erosion + 1 ==
                  MAX_ERODE ? " completely" : erosion ? " further" : "");
        else if (visobj)
            pline("The %s%s!", aobjnam(target, acid_dmg ? "corrode" : "rust"),
                  erosion + 1 ==
                  MAX_ERODE ? " completely" : erosion ? " further" : "");
        if (acid_dmg)
            target->oeroded2++;
        else
            target->oeroded++;
    } else {
        if (flags.verbose) {
            if (victim == &youmonst)
                pline("Your %s completely %s.",
                      aobjnam(target, Blind ? "feel" : "look"),
                      acid_dmg ? "corroded" : "rusty");
            else if (vismon)
                pline("%s's %s completely %s.", Monnam(victim),
                      aobjnam(target, "look"), acid_dmg ? "corroded" : "rusty");
            else if (visobj)
                pline("The %s completely %s.", aobjnam(target, "look"),
                      acid_dmg ? "corroded" : "rusty");
        }
    }
}

int
chwepon(struct obj *otmp, int amount)
{
    const char *color = hcolor((amount < 0) ? "black" : "blue");
    const char *xtime;
    int otyp = STRANGE_OBJECT;

    if (!uwep || (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))) {
        char buf[BUFSZ];

        sprintf(buf, "Your %s %s.", makeplural(body_part(HAND)),
                (amount >= 0) ? "twitch" : "itch");
        strange_feeling(otmp, buf);
        exercise(A_DEX, (boolean) (amount >= 0));
        return 0;
    }

    if (otmp && otmp->oclass == SCROLL_CLASS)
        otyp = otmp->otyp;

    if (uwep->otyp == WORM_TOOTH && amount >= 0) {
        uwep->otyp = CRYSKNIFE;
        uwep->oerodeproof = 0;
        pline("Your weapon seems sharper now.");
        uwep->cursed = 0;
        if (otyp != STRANGE_OBJECT)
            makeknown(otyp);
        goto weapon_unpaid_fixup;
    }

    if (uwep->otyp == CRYSKNIFE && amount < 0) {
        if (uwep->unpaid)
            costly_damage_obj(uwep);
        uwep->otyp = WORM_TOOTH;
        uwep->oerodeproof = 0;
        pline("Your weapon seems duller now.");
        if (otyp != STRANGE_OBJECT && otmp->bknown)
            makeknown(otyp);
        return 1;
    }

    if (amount < 0 && uwep->oartifact && restrict_name(uwep, ONAME(uwep))) {
        if (!Blind)
            pline("Your %s %s.", aobjnam(uwep, "faintly glow"), color);
        return 1;
    }
    if (amount < 0 && uwep->unpaid)
        costly_damage_obj(uwep);
    /* there is a (soft) upper and lower limit to uwep->spe */
    if (((uwep->spe > 5 && amount >= 0) || (uwep->spe < -5 && amount < 0))
        && rn2(3)) {
        if (!Blind)
            pline("Your %s %s for a while and then %s.",
                  aobjnam(uwep, "violently glow"), color,
                  otense(uwep, "evaporate"));
        else
            pline("Your %s.", aobjnam(uwep, "evaporate"));

        useupall(uwep); /* let all of them disappear */
        return 1;
    }
    if (!Blind) {
        xtime = (amount * amount == 1) ? "moment" : "while";
        pline("Your %s %s for a %s.",
              aobjnam(uwep, amount == 0 ? "violently glow" : "glow"), color,
              xtime);
        if (otyp != STRANGE_OBJECT && uwep->known &&
            (amount > 0 || (amount < 0 && otmp->bknown)))
            makeknown(otyp);
    }
    uwep->spe += amount;
    if (amount > 0)
        uwep->cursed = 0;

    /* 
     * Enchantment, which normally improves a weapon, has an
     * addition adverse reaction on Magicbane whose effects are
     * spe dependent.  Give an obscure clue here.
     */
    if (uwep->oartifact == ART_MAGICBANE && uwep->spe >= 0) {
        pline("Your right %s %sches!", body_part(HAND),
              (((amount > 1) && (uwep->spe > 1)) ? "flin" : "it"));
    }

    /* an elven magic clue, cookie@keebler */
    /* elven weapons vibrate warningly when enchanted beyond a limit */
    if ((uwep->spe > 5)
        && (is_elven_weapon(uwep) || uwep->oartifact || !rn2(7)))
        pline("Your %s unexpectedly.", aobjnam(uwep, "suddenly vibrate"));

weapon_unpaid_fixup:
    if (uwep->unpaid && (amount >= 0)) {
        adjust_bill_val(uwep);
    }

    return 1;
}

int
welded(struct obj *obj)
{
    if (obj && obj == uwep && will_weld(obj)) {
        obj->bknown = TRUE;
        return 1;
    }
    return 0;
}

void
weldmsg(struct obj *obj)
{
    long savewornmask;

    savewornmask = obj->owornmask;
    pline("Your %s %s welded to your %s!", xname(obj), otense(obj, "are"),
          bimanual(obj) ? (const char *)makeplural(body_part(HAND))
          : body_part(HAND));
    obj->owornmask = savewornmask;
}

/* Unwields all weapons silently. */
void
unwield_weapons_silently(void)
{
    setuwep(NULL);
    setuswapwep(NULL);
    setuqwep(NULL);
    u.twoweap = FALSE;
}

/*wield.c*/