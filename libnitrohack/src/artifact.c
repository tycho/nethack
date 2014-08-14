/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"
#include "artilist.h"
/*
 * Note:  both artilist[] and artiexist[] have a dummy element #0,
 *	  so loops over them should normally start at #1.  The primary
 *	  exception is the save & restore code, which doesn't care about
 *	  the contents, just the total size.
 */

extern boolean notonhead;	/* for long worms */
extern struct obj *thrownobj;	/* from dothrow.c to clear for detonations */

#define get_artifact(o) \
		(((o)&&(o)->oartifact) ? &artilist[(int) (o)->oartifact] : 0)

static int spec_applies(const struct artifact *,struct monst *);
static int arti_invoke(struct obj*);
static boolean magicbane_hit(struct monst *magr,struct monst *mdef,
				  struct obj *,int *,int,boolean,char *);
static long spec_m2(struct obj *);

/* The amount added to the victim's total hit points to insure that the
   victim will be killed even after damage bonus/penalty adjustments.
   Most such penalties are small, and 200 is plenty; the exception is
   half physical damage.  3.3.1 and previous versions tried to use a very
   large number to account for this case; now, we just compute the fatal
   damage by adding it to 2 times the total hit points instead of 1 time.
   Note: this will still break if they have more than about half the number
   of hit points that will fit in a 15 bit integer. */
#define FATAL_DAMAGE_MODIFIER 200

/* coordinate effects from spec_dbon() with messages in artifact_hit() */
static int spec_dbon_applies = 0;
/*
 * Which damage types/effects was the monster actually affected by in spec_dbon()
 * and spec_applies()?  Used by cases in artifact_hit() and friends that need
 * more detail than what spec_dbon_applies provides.
 *
 * Add more, set them in spec_dbon()/spec_applies() if needed in artifact_hit().
 */
static boolean spec_applies_ad_fire,
	       spec_applies_ad_cold,
	       spec_applies_ad_elec,
	       spec_applies_ad_magm,
	       spec_applies_ad_stun,
	       spec_applies_ad_drli,
	       spec_applies_spfx_behead;

/* flags including which artifacts have already been created */
static boolean artiexist[1+NROFARTIFACTS+1];
/* and a discovery list for them (no dummy first entry here) */
static xchar artidisco[NROFARTIFACTS];

static void hack_artifacts(void);
static void strip_oprops(struct obj *);
static boolean attacks(int, struct obj *, struct obj *, boolean);


void init_artilist(void)
{
    artilist = malloc(sizeof(const_artilist));
    memcpy(artilist, const_artilist, sizeof(const_artilist));
}

/* handle some special cases; must be called after u_init() */
static void hack_artifacts(void)
{
	struct artifact *art;
	int alignmnt = aligns[u.initalign].value;

	/* Fix up the alignments of "gift" artifacts */
	for (art = artilist+1; art->otyp; art++)
	    if (art->role == Role_switch && art->alignment != A_NONE)
		art->alignment = alignmnt;

	/* Excalibur can be used by any lawful character, not just knights */
	if (!Role_if (PM_KNIGHT))
	    artilist[ART_EXCALIBUR].role = NON_PM;

	/* Fix up the quest artifact */
	if (urole.questarti) {
	    artilist[urole.questarti].alignment = alignmnt;
	    artilist[urole.questarti].role = Role_switch;
	}
	return;
}

/* zero out the artifact existence list */
void init_artifacts(void)
{
	memset(artiexist, 0, sizeof artiexist);
	memset(artidisco, 0, sizeof artidisco);
	hack_artifacts();
}

void save_artifacts(struct memfile *mf)
{
	/* artiexist and artidisco are arrays of bytes, so writing them in one
	 * go is safe and portable */
	mtag(mf, 0, MTAG_ARTIFACT);
	mwrite(mf, artiexist, sizeof artiexist);
	mwrite(mf, artidisco, sizeof artidisco);
}

void restore_artifacts(struct memfile *mf)
{
	mread(mf, artiexist, sizeof artiexist);
	mread(mf, artidisco, sizeof artidisco);
	hack_artifacts();	/* redo non-saved special cases */
}

const char *artiname(int artinum)
{
	if (artinum <= 0 || artinum > NROFARTIFACTS) return "";
	return artilist[artinum].name;
}

/*
   Make an artifact.  If a specific alignment is specified, then an object of
   the appropriate alignment is created from scratch, or 0 is returned if
   none is available.  (If at least one aligned artifact has already been
   given, then unaligned ones also become eligible for this.)
   If no alignment is given, then 'otmp' is converted
   into an artifact of matching type, or returned as-is if that's not possible.
   For the 2nd case, caller should use ``obj = mk_artifact(lev, obj, A_NONE);''
   for the 1st, ``obj = mk_artifact(lev, NULL, some_alignment);''.
 */
struct obj *mk_artifact(
    struct level *lev,	/* level to create artifact on if otmp not given */
    struct obj *otmp,	/* existing object; ignored if alignment specified */
    aligntyp alignment	/* target alignment, or A_NONE */
    )
{
	const struct artifact *a;
	int n, m;
	boolean by_align = (alignment != A_NONE);
	short o_typ = (by_align || !otmp) ? 0 : otmp->otyp;
	boolean unique = !by_align && otmp && objects[o_typ].oc_unique;
	short eligible[NROFARTIFACTS];

	/* Don't artifactize already special weapons */
	if (otmp && otmp->oartifact) return otmp;

	/* Strip properties: they're mutually exclusive with artifacts. */
	if (otmp) strip_oprops(otmp);

	/* gather eligible artifacts */
	for (n = 0, a = artilist+1, m = 1; a->otyp; a++, m++)
	    if ((!by_align ? a->otyp == o_typ :
		    (a->alignment == alignment ||
			(a->alignment == A_NONE && u.ugifts > 0))) &&
		(!(a->spfx & SPFX_NOGEN) || unique) && !artiexist[m]) {
		if (by_align && a->race != NON_PM && race_hostile(&mons[a->race]))
		    continue;	/* skip enemies' equipment */
		else if (by_align && Role_if (a->role))
		    goto make_artif;	/* 'a' points to the desired one */
		else
		    eligible[n++] = m;
	    }

	if (n) {		/* found at least one candidate */
	    m = eligible[rn2(n)];	/* [0..n-1] */
	    a = &artilist[m];

	    /* make an appropriate object if necessary, then christen it */
make_artif: if (by_align) otmp = mksobj(lev, (int)a->otyp, TRUE, FALSE);
	    otmp = oname(otmp, a->name);
	    otmp->oartifact = m;
	    artiexist[m] = TRUE;
	} else {
	    /* spruce up the object if there's no matching artifact for it */
	    otmp = create_oprop(level, otmp, FALSE);
	}
	return otmp;
}

/* Given a skill, find a suitable weapon for the current race and role. */
/* Returns the object type or 0 if nothing is found. */
static int suitable_obj_for_skill(int skill)
{
	switch (skill) {
	case P_DAGGER:
	    if (Race_if(PM_ELF)) return ELVEN_DAGGER;
	    if (Race_if(PM_ORC)) return ORCISH_DAGGER;
	    if (Role_if(PM_WIZARD)) return ATHAME;
	    break;
	case P_KNIFE:
	    if (Role_if(PM_HEALER)) return SCALPEL;
	    break;
	case P_SHORT_SWORD:
	    if (Race_if(PM_ELF)) return ELVEN_SHORT_SWORD;
	    if (Race_if(PM_ORC)) return ORCISH_SHORT_SWORD;
	    if (Race_if(PM_DWARF)) return DWARVISH_SHORT_SWORD;
	    if (Role_if(PM_SAMURAI)) return SHORT_SWORD; /* wakizashi */
	    break;
	case P_BROAD_SWORD:
	    if (Race_if(PM_ELF)) return ELVEN_BROADSWORD;
	    break;
	case P_LONG_SWORD:
	    if (Role_if(PM_SAMURAI)) return KATANA;
	    break;
	case P_TWO_HANDED_SWORD:
	    if (Role_if(PM_SAMURAI)) return TSURUGI;
	    break;
	case P_BOW:
	    if (Race_if(PM_ELF)) return rn2(2) ? ELVEN_BOW : ELVEN_ARROW;
	    if (Race_if(PM_ORC)) return rn2(2) ? ORCISH_BOW : ORCISH_ARROW;
	    if (Role_if(PM_SAMURAI)) return rn2(2) ? YUMI : YA;
	    break;
	default:
	    break;
	}

	return 0;
}

/* Apply restrictions to special properties to an object. */
long restrict_oprops(struct obj *otmp, long objprops)
{
	enum ro_objtypes {
	    RO_OTHER = -1,
	    RO_WEP_BLADE, RO_WEP_LAUNCH, RO_WEP_PROJ, RO_WEP_OTHER,
	    RO_ARM_HGF, RO_ARM_OTHER, RO_AMULET, RO_RING,
	    RO_MAX
	} ot;
	static const struct {
	    long oprop;
	    boolean objtypes[RO_MAX];
	} otyp_oprops[] = {
	    /* most objects */
	    {ITEM_FIRE,		{1, 1, 1, 1,	1, 1, 1, 1}},
	    {ITEM_FROST,	{1, 1, 1, 1,	1, 1, 1, 1}},
	    {ITEM_DRLI,		{1, 1, 1, 1,	1, 1, 1, 1}},
	    {ITEM_REFLECTION,	{1, 1, 0, 1,	0, 1, 1, 1}},
	    {ITEM_ESP,		{1, 1, 0, 1,	1, 1, 1, 1}},
	    {ITEM_SEARCHING,	{1, 1, 0, 1,	1, 1, 1, 1}},
	    {ITEM_WARNING,	{1, 1, 0, 1,	1, 1, 1, 1}},
	    {ITEM_STEALTH,	{1, 1, 1, 1,	1, 1, 1, 1}},
	    {ITEM_FUMBLING,	{1, 1, 0, 1,	1, 1, 1, 1}},
	    {ITEM_HUNGER,	{1, 1, 0, 1,	1, 1, 1, 1}},
	    {ITEM_AGGRAVATE,	{1, 1, 0, 1,	1, 1, 1, 1}},
	    /* weapons only */
	    {ITEM_VORPAL,	{1, 0, 0, 0,	0, 0, 0, 0}},
	    {ITEM_DETONATIONS,	{0, 1, 1, 0,	0, 0, 0, 0}},
	    /* worn armor, amulets and rings only */
	    {ITEM_SPEED,	{0, 0, 0, 0,	1, 1, 1, 1}},
	    {ITEM_OILSKIN,	{0, 0, 0, 0,	1, 1, 1, 1}},
	    {ITEM_POWER,	{0, 0, 0, 0,	1, 1, 1, 1}},
	    {ITEM_DEXTERITY,	{0, 0, 0, 0,	1, 1, 0, 1}},
	    {ITEM_BRILLIANCE,	{0, 0, 0, 0,	1, 1, 0, 1}},
	    {ITEM_DISPLACEMENT,	{0, 0, 0, 0,	1, 1, 1, 1}},
	    {ITEM_CLAIRVOYANCE,	{0, 0, 0, 0,	1, 1, 1, 1}},
	};

	short otyp = otmp->otyp;
	int i;

	/* classify object */
	if (otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) {
	    if (is_blade(otmp))
		ot = RO_WEP_BLADE;
	    else if (is_launcher(otmp))
		ot = RO_WEP_LAUNCH;
	    else if (is_ammo(otmp) || is_missile(otmp))
		ot = RO_WEP_PROJ;
	    else
		ot = RO_WEP_OTHER;
	} else if (otmp->oclass == ARMOR_CLASS) {
	    if (is_helmet(otmp) || is_gloves(otmp) || is_boots(otmp))
		ot = RO_ARM_HGF;
	    else
		ot = RO_ARM_OTHER;
	} else if (otmp->oclass == AMULET_CLASS) {
	    ot = RO_AMULET;
	} else if (otmp->oclass == RING_CLASS) {
	    ot = RO_RING;
	} else {
	    ot = RO_OTHER;
	}

	/* no properties for objects outside of the above for now */
	if (ot == RO_OTHER) return 0L;

	/* apply object type restrictions */
	for (i = 0; i < SIZE(otyp_oprops); i++) {
	    if (!otyp_oprops[i].objtypes[ot])
		objprops &= ~(otyp_oprops[i].oprop);
	}

	/* no oilskin for non-cloth objects */
	if (objects[otyp].oc_material != CLOTH)
	    objprops &= ~ITEM_OILSKIN;

	/* dexterity and brilliance need enchantment to have an effect */
	if (!objects[otyp].oc_charged)
	    objprops &= ~(ITEM_DEXTERITY|ITEM_BRILLIANCE);

	/* avoid doubling-up with the object's base abilities */
	switch (objects[otyp].oc_oprop) {
	case FIRE_RES: objprops &= ~ITEM_FIRE; break;
	case COLD_RES: objprops &= ~ITEM_FROST; break;
	case DRAIN_RES: objprops &= ~ITEM_DRLI; break;
	/* nothing for ITEM_VORPAL */
	case REFLECTING: objprops &= ~ITEM_REFLECTION; break;
	case FAST: objprops &= ~ITEM_SPEED; break;
	/* ITEM_OILSKIN handled below */
	/* ITEM_POWER handled below */
	/* ITEM_DEXTERITY handled below */
	/* ITEM_BRILLIANCE handled below */
	case TELEPAT: objprops &= ~ITEM_ESP; break;
	case DISPLACED: objprops &= ~ITEM_DISPLACEMENT; break;
	case SEARCHING: objprops &= ~ITEM_SEARCHING; break;
	case WARNING: objprops &= ~ITEM_WARNING; break;
	case STEALTH: objprops &= ~ITEM_STEALTH; break;
	case FUMBLING: objprops &= ~ITEM_FUMBLING; break;
	case CLAIRVOYANT: objprops &= ~ITEM_CLAIRVOYANCE; break;
	/* nothing for ITEM_DETONATIONS */
	case HUNGER: objprops &= ~ITEM_HUNGER; break;
	case AGGRAVATE_MONSTER: objprops &= ~ITEM_AGGRAVATE; break;
	}

	if (otyp == OILSKIN_CLOAK || otyp == OILSKIN_SACK)
	    objprops &= ~ITEM_OILSKIN;
	if (otyp == GAUNTLETS_OF_POWER)
	    objprops &= ~ITEM_POWER;
	if (otyp == GAUNTLETS_OF_DEXTERITY)
	    objprops &= ~ITEM_DEXTERITY;
	if (otyp == HELM_OF_BRILLIANCE)
	    objprops &= ~ITEM_BRILLIANCE;

	return objprops;
}

/*
 * Create an item with special properties, or grant the item those properties.
 */
struct obj *create_oprop(struct level *lev, struct obj *otmp,
			 boolean allow_detrimental)
{
	if (!otmp) {
	    int type = 0;
	    int skill = P_NONE;
	    int candidates[128];
	    int ccount;
	    int threshold = P_EXPERT;
	    int i;

	    /* This probably is only ever done for weapons, y'know?
	     * Find an appropriate type of weapon */
	    while (threshold > P_UNSKILLED) {
		ccount = 0;
		for (i = P_FIRST_WEAPON; i < P_LAST_WEAPON; i++) {
		    if (P_MAX_SKILL(i) >= max(threshold, P_BASIC) &&
			P_SKILL(i) >= threshold)
			candidates[ccount++] = i;
		    if (ccount >= 128) break;
		}
		if (ccount == 0) {
		    threshold--;
		    continue;
		}
		skill = candidates[rn2(ccount)];
		if ((type = suitable_obj_for_skill(skill)))
		    break;

		ccount = 0;
		for (i = ARROW; i <= CROSSBOW; i++) {
		    if (abs(objects[i].oc_skill) == skill)
			candidates[ccount++] = i;
		    if (ccount == 128) break;
		}
		if (!ccount) {
		    impossible("found no weapons for skill %d?", skill);
		    threshold--;
		    continue;
		}
		type = candidates[rn2(ccount)];
		break;
	    }

	    /* Now make one, if we can */
	    if (type != 0)
		otmp = mksobj(lev, type, TRUE, FALSE);
	    else
		otmp = mkobj(lev, WEAPON_CLASS, FALSE);
	}

	/* Regardless if it's special or not, fix it up as necessary */
	if (!allow_detrimental) {
	    if (otmp->cursed) uncurse(otmp);
	    else bless(otmp);

	    if (otmp->oclass == WEAPON_CLASS ||
		otmp->oclass == ARMOR_CLASS ||
		(otmp->oclass == RING_CLASS && objects[otmp->otyp].oc_charged)) {
		if (otmp->spe < 0) otmp->spe = 0;
		else if (otmp->spe == 0) otmp->spe = rn1(2, 1);
	    }
	}

	/* Don't spruce up artifacts */
	if (otmp->oartifact) return otmp;
	else if (objects[otmp->otyp].oc_unique) return otmp;

	if (otmp->oclass != WEAPON_CLASS && !is_weptool(otmp) &&
	    otmp->oclass != AMULET_CLASS &&
	    otmp->oclass != RING_CLASS &&
	    otmp->oclass != ARMOR_CLASS)
	    return otmp;

	while (!otmp->oprops || !rn2(250)) {
	    int j = 1 << rn2(MAX_ITEM_PROPS); /* pick an item property */

	    if (otmp->oprops & j) continue;

	    /* don't randomly put insta-death weapons in monsters' hands */
	    if (j & ITEM_VORPAL) continue;

	    if ((j & (ITEM_FUMBLING|ITEM_HUNGER|ITEM_AGGRAVATE)) &&
		!allow_detrimental)
		continue;

	    j = restrict_oprops(otmp, j);
	    otmp->oprops |= j;
	}

	if ((otmp->oprops & (ITEM_FUMBLING|ITEM_HUNGER|ITEM_AGGRAVATE)) &&
	    allow_detrimental && rn2(10)) {
	    curse(otmp);
	}

	return otmp;
}

/*
 * Strip any object properties on a potentially hero-wielded/worn object.
 * Will NOT unset property effects on monster-equipped objects.
 */
static void strip_oprops(struct obj *otmp)
{
	if (!otmp || !otmp->oprops)
	    return;

	if (otmp == uwep || otmp == uswapwep) {
	    set_artifact_intrinsic(otmp, FALSE, otmp->owornmask, TRUE);
	} else {
	    unsigned int which = 0;
	    if (otmp == uarm) which = W_ARM;
	    if (otmp == uarmu) which = W_ARMU;
	    if (otmp == uarmc) which = W_ARMC;
	    if (otmp == uarmh) which = W_ARMH;
	    if (otmp == uarms) which = W_ARMS;
	    if (otmp == uarmg) which = W_ARMG;
	    if (otmp == uarmf) which = W_ARMF;
	    if (otmp == uamul) which = W_AMUL;
	    if (otmp == uleft) which = W_RINGL;
	    if (otmp == uright) which = W_RINGR;
	    if (which != 0) Oprops_off(otmp, which);
	}

	otmp->oprops = otmp->oprops_known = 0L;
}

/*
 * Set property the player knows on object/stack/launcher from an attack.
 */
void oprop_id(long oprop, struct obj *otmp, struct obj *ostack, struct obj *olaunch)
{
	if ((otmp->oprops & oprop)) {
	    otmp->oprops_known |= oprop;
	    if (ostack)
		ostack->oprops_known |= oprop;
	} else if (ammo_and_launcher(otmp, olaunch) &&
		   (olaunch->oprops & oprop)) {
	    olaunch->oprops_known |= oprop;
	}
}

/*
 * Decide if an attack property applies for an object/launcher.
 */
static boolean oprop_attack(long oprop, const struct obj *otmp,
			    const struct obj *olaunch, boolean thrown)
{
	if (thrown) {
	    if (ammo_and_launcher(otmp, olaunch)) {
		return (otmp->oprops & oprop) || (olaunch->oprops & oprop);
	    } else {
		/* any weapon except for thrown launchers */
		return (otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) &&
		       !is_launcher(otmp) && (otmp->oprops & oprop);
	    }
	} else {
	    /* wielded melee hit: weapons, but not missiles/ammo/launchers */
	    return (otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) &&
		   !is_missile(otmp) && !is_ammo(otmp) && !is_launcher(otmp) &&
		   (otmp->oprops & oprop);
	}
}

/*
 * Will attacking with this object/launcher be silent?
 */
boolean oprop_stealth_attack(const struct obj *otmp,
			     const struct obj *ostack,
			     const struct obj *olaunch,
			     boolean thrown)
{
	return oprop_attack(ITEM_STEALTH, otmp, olaunch, thrown);
}

/*
 * Explode a projectile if the detonation property applies.
 * The explosion and scattering occur at bhitpos.
 * Returns TRUE if the projectile was destroyed; it is freed here.
 */
boolean detonate_obj(struct obj *otmp, struct obj *ostack, struct obj *olaunch,
		     int x, int y, boolean thrown)
{
	boolean is_artifact;
	int dmg;

	/* Don't destroy vital objects, but allow artifacts,
	 * e.g. the Heart of Ahriman fired from a sling of detonation . */
	is_artifact = !!otmp->oartifact;
	if (obj_resists(otmp, 0, 100) && !is_artifact)
	    return FALSE;

	if (!oprop_attack(ITEM_DETONATIONS, otmp, olaunch, thrown) &&
	    /* ammo and missiles may detonate even if dropped */
	    !((otmp->oprops & ITEM_DETONATIONS) &&
	      (is_ammo(otmp) || is_missile(otmp))))
	    return FALSE;

	pline("The %s explodes!", xname(otmp));
	oprop_id(ITEM_DETONATIONS, otmp, ostack, olaunch);

	if (!is_artifact) {
	    thrownobj = NULL;
	    obfree(otmp, NULL);
	    otmp = NULL;
	}

	/* otmp->ox and otmp->oy may not be set correctly, or even at all! */
	dmg = dice(4, 4);
	explode(x, y, ZT_SPELL(ZT_FIRE), dmg, WEAPON_CLASS, EXPL_FIERY);
	scatter(x, y, dmg, VIS_EFFECTS|MAY_HIT|MAY_DESTROY|MAY_FRACTURE, NULL);

	return !is_artifact;
}

/*
 * Returns the full name (with articles and correct capitalization) of an
 * artifact named "name" if one exists, or NULL, it not.
 * The given name must be rather close to the real name for it to match.
 * The object type of the artifact is returned in otyp if the return value
 * is non-NULL.
 */
const char *artifact_name(const char *name, short *otyp)
{
    const struct artifact *a;
    const char *aname;

    if (!strncmpi(name, "the ", 4)) name += 4;

    for (a = artilist+1; a->otyp; a++) {
	aname = a->name;
	if (!strncmpi(aname, "the ", 4)) aname += 4;
	if (!strcmpi(name, aname)) {
	    *otyp = a->otyp;
	    return a->name;
	}
    }

    return NULL;
}

boolean exist_artifact(int otyp, const char *name)
{
	const struct artifact *a;
	boolean *arex;

	if (otyp && *name)
	    for (a = artilist+1,arex = artiexist+1; a->otyp; a++,arex++)
		if ((int) a->otyp == otyp && !strcmp(a->name, name))
		    return *arex;
	return FALSE;
}

void artifact_exists(struct obj *otmp, const char *name, boolean mod)
{
	const struct artifact *a;

	if (otmp && *name)
	    for (a = artilist+1; a->otyp; a++)
		if (a->otyp == otmp->otyp && !strcmp(a->name, name)) {
		    int m = a - artilist;
		    if (mod) /* artifacts can't have properties */
			strip_oprops(otmp);
		    otmp->oartifact = (char)(mod ? m : 0);
		    otmp->age = 0;
		    if (otmp->otyp == RIN_INCREASE_DAMAGE)
			otmp->spe = 0;
		    artiexist[m] = mod;
		    break;
		}
	return;
}

int nartifact_exist(void)
{
    int a = 0;
    int n = SIZE(artiexist);

    while (n > 1)
	if (artiexist[--n]) a++;

    return a;
}


boolean spec_ability(struct obj *otmp, unsigned long abil)
{
	const struct artifact *arti = get_artifact(otmp);

	return (boolean)(arti && (arti->spfx & abil));
}

/* used so that callers don't need to known about SPFX_ codes */
boolean confers_luck(struct obj *obj)
{
    /* might as well check for this too */
    if (obj->otyp == LUCKSTONE) return TRUE;

    return obj->oartifact && spec_ability(obj, SPFX_LUCK);
}

/* used to check whether a monster is getting reflection from an artifact */
boolean arti_reflects(struct obj *obj)
{
    const struct artifact *arti = get_artifact(obj);

    if (arti) {      
	/* while being worn */
	if ((obj->owornmask & ~W_ART) && (arti->spfx & SPFX_REFLECT))
	    return TRUE;
	/* just being carried */
	if (arti->cspfx & SPFX_REFLECT) return TRUE;
    }
    return FALSE;
}


/* returns 1 if name is restricted */
boolean restrict_name(struct obj *otmp, const char *name, boolean restrict_typ)
{
	const struct artifact *a;
	const char *aname;

	if (!*name) return FALSE;
	if (!strncmpi(name, "the ", 4)) name += 4;

		/* Since almost every artifact is SPFX_RESTR, it doesn't cost
		   us much to do the string comparison before the spfx check.
		   Bug fix:  don't name multiple elven daggers "Sting".
		 */
	for (a = artilist+1; a->otyp; a++) {
	    if (restrict_typ && a->otyp != otmp->otyp) continue;
	    aname = a->name;
	    if (!strncmpi(aname, "the ", 4)) aname += 4;
	    if (!strcmpi(aname, name))
		return ((boolean)((a->spfx & (SPFX_NOGEN|SPFX_RESTR)) != 0 ||
			otmp->quan > 1L));
	}

	return FALSE;
}

static boolean attacks(int adtyp, struct obj *otmp, struct obj *olaunch,
		       boolean thrown)
{
	const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return (boolean)(weap->attk.adtyp == adtyp);

	if (!weap && (otmp->oprops ||
		      (ammo_and_launcher(otmp, olaunch) &&
		       olaunch->oprops))) {
	    if (adtyp == AD_FIRE &&
		oprop_attack(ITEM_FIRE, otmp, olaunch, thrown))
		return TRUE;
	    if (adtyp == AD_COLD &&
		oprop_attack(ITEM_FROST, otmp, olaunch, thrown))
		return TRUE;
	    if (adtyp == AD_DRLI &&
		oprop_attack(ITEM_DRLI, otmp, olaunch, thrown))
		return TRUE;
	}

	return FALSE;
}

boolean defends(int adtyp, struct obj *otmp)
{
	const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return (boolean)(weap->defn.adtyp == adtyp);
	return FALSE;
}

/* used for monsters */
boolean protects(int adtyp, struct obj *otmp)
{
	const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return (boolean)(weap->cary.adtyp == adtyp);
	return FALSE;
}

/*
 * a potential artifact has just been worn/wielded/picked-up or
 * unworn/unwielded/dropped.  Pickup/drop only set/reset the W_ART mask.
 */
void set_artifact_intrinsic(struct obj *otmp, boolean on, long wp_mask,
			    boolean side_effects)
{
	unsigned int *mask = 0;
	const struct artifact *oart = get_artifact(otmp);
	uchar dtyp;
	long spfx;
	boolean spfx_stlth = FALSE;	/* we ran out of SPFX_* bits, so... */
	boolean spfx_fumbling = FALSE;
	boolean spfx_hunger = FALSE;
	boolean spfx_aggravate = FALSE;

	if (!oart && !otmp->oprops)
	    return;

	/* effects from the defn field */
	if (oart)
	    dtyp = (wp_mask != W_ART) ? oart->defn.adtyp : oart->cary.adtyp;
	else
	    dtyp = 0;

	if (dtyp == AD_FIRE)
	    mask = &EFire_resistance;
	else if (dtyp == AD_COLD)
	    mask = &ECold_resistance;
	else if (dtyp == AD_ELEC)
	    mask = &EShock_resistance;
	else if (dtyp == AD_MAGM)
	    mask = &EAntimagic;
	else if (dtyp == AD_DISN)
	    mask = &EDisint_resistance;
	else if (dtyp == AD_DRST)
	    mask = &EPoison_resistance;

	if (mask && wp_mask == W_ART && !on) {
	    /* find out if some other artifact also confers this intrinsic */
	    /* if so, leave the mask alone */
	    struct obj* obj;
	    for (obj = invent; obj; obj = obj->nobj)
		if (obj != otmp && obj->oartifact) {
		    const struct artifact *art = get_artifact(obj);
		    if (art->cary.adtyp == dtyp ||
			(dtyp == AD_MAGM && is_quest_artifact(obj))) {
			mask = NULL;
			break;
		    }
		}
	}
	if (mask) {
	    if (on) *mask |= wp_mask;
	    else *mask &= ~wp_mask;
	}

	/* quest artifact grants magic resistance when carried by their role */
	if (is_quest_artifact(otmp) && wp_mask == W_ART) {
	    if (on) {
		EAntimagic |= W_ART;
	    } else {
		/* don't take away magic resistance if another artifact grants it */
		struct obj *obj;
		for (obj = invent; obj; obj = obj->nobj) {
		    if (obj != otmp && obj->oartifact) {
			const struct artifact *art = get_artifact(obj);
			if (art->cary.adtyp == AD_MAGM) break;
		    }
		}
		if (!obj) EAntimagic &= ~W_ART;
	    }
	}

	/* intrinsics from the spfx field; there could be more than one */
	spfx = 0;
	if (oart) {
	    spfx = (wp_mask != W_ART) ? oart->spfx : oart->cspfx;
	} else if ((wp_mask == W_WEP ||
		    (wp_mask == W_SWAPWEP && (u.twoweap || !on))) &&
		   (otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) &&
		   !is_ammo(otmp) && !is_missile(otmp)) {
	    if (otmp->oprops & ITEM_REFLECTION)
		spfx |= SPFX_REFLECT;
	    if (otmp->oprops & ITEM_ESP)
		spfx |= SPFX_ESP;
	    if (otmp->oprops & ITEM_SEARCHING)
		spfx |= SPFX_SEARCH;
	    if (otmp->oprops & ITEM_WARNING)
		spfx |= SPFX_WARN;
	    if (otmp->oprops & ITEM_STEALTH)
		spfx_stlth = TRUE;
	    if (otmp->oprops & ITEM_FUMBLING)
		spfx_fumbling = TRUE;
	    if (otmp->oprops & ITEM_HUNGER)
		spfx_hunger = TRUE;
	    if (otmp->oprops & ITEM_AGGRAVATE)
		spfx_aggravate = TRUE;
	}

	if (spfx && wp_mask == W_ART && !on) {
	    /* don't change any spfx also conferred by other artifacts */
	    struct obj* obj;
	    for (obj = invent; obj; obj = obj->nobj)
		if (obj != otmp && obj->oartifact) {
		    const struct artifact *art = get_artifact(obj);
		    spfx &= ~art->cspfx;
		}
	}

	if (spfx & SPFX_SEARCH) {
	    if (on) ESearching |= wp_mask;
	    else ESearching &= ~wp_mask;
	}
	if (spfx & SPFX_HALRES) {
	    /* make_hallucinated must (re)set the mask itself to get
	     * the display right */
	    /* hide message when restoring a game */
	    make_hallucinated((long)!on, side_effects, wp_mask);
	}
	if (spfx & SPFX_ESP) {
	    if (on) ETelepat |= wp_mask;
	    else ETelepat &= ~wp_mask;
	    see_monsters();
	}
	if (spfx_stlth) {
	    if (on) {
		if (side_effects && !EStealth && !HStealth && !BStealth) {
		    pline("You move very quietly.");
		    if (otmp->oprops & ITEM_STEALTH)
			otmp->oprops_known |= ITEM_STEALTH;
		}
		EStealth |= wp_mask;
	    } else {
		EStealth &= ~wp_mask;
		if (side_effects && !EStealth && !HStealth && !BStealth) {
		    pline("You sure are noisy.");
		    if (otmp->oprops & ITEM_STEALTH)
			otmp->oprops_known |= ITEM_STEALTH;
		}
	    }
	}
	if (spfx_fumbling) {
	    if (on) {
		if (!EFumbling && !(HFumbling & ~TIMEOUT))
		    incr_itimeout(&HFumbling, rnd(20));
		EFumbling |= wp_mask;
	    } else {
		EFumbling &= ~wp_mask;
		if (!EFumbling && ~(HFumbling & ~TIMEOUT))
		    HFumbling = EFumbling = 0;
	    }
	}
	if (spfx_hunger) {
	    if (on) EHunger |= wp_mask;
	    else EHunger &= ~wp_mask;
	}
	if (spfx_aggravate) {
	    if (on) EAggravate_monster |= wp_mask;
	    else EAggravate_monster &= ~wp_mask;
	}
	if (spfx & SPFX_DISPL) {
	    if (on) EDisplaced |= wp_mask;
	    else EDisplaced &= ~wp_mask;
	}
	if (spfx & SPFX_REGEN) {
	    if (on) ERegeneration |= wp_mask;
	    else ERegeneration &= ~wp_mask;
	}
	if (spfx & SPFX_TCTRL) {
	    if (on) ETeleport_control |= wp_mask;
	    else ETeleport_control &= ~wp_mask;
	}
	if (spfx & SPFX_WARN) {
	    if (spec_m2(otmp)) {
		if (on) {
			EWarn_of_mon |= wp_mask;
			flags.warntype |= spec_m2(otmp);
		} else {
			EWarn_of_mon &= ~wp_mask;
			flags.warntype &= ~spec_m2(otmp);
		}
	    } else {
		if (on) EWarning |= wp_mask;
		else EWarning &= ~wp_mask;
	    }
	    see_monsters();
	}
	if (spfx & SPFX_WARN_S) {
	    if (oart->mtype) {
		if (on) {
		    EWarn_of_mon |= wp_mask;
		} else {
		    EWarn_of_mon &= ~wp_mask;
		}
	    } else {
		if (on) EWarning |= wp_mask;
	    	else EWarning &= ~wp_mask;
	    }
	    see_monsters();
	}
	if (spfx & SPFX_EREGEN) {
	    if (on) EEnergy_regeneration |= wp_mask;
	    else EEnergy_regeneration &= ~wp_mask;
	}
	if (spfx & SPFX_HSPDAM) {
	    if (on) EHalf_spell_damage |= wp_mask;
	    else EHalf_spell_damage &= ~wp_mask;
	}
	if (spfx & SPFX_HPHDAM) {
	    if (on) EHalf_physical_damage |= wp_mask;
	    else EHalf_physical_damage &= ~wp_mask;
	}
	if (spfx & SPFX_XRAY) {
	    /* this assumes that no one else is using xray_range */
	    if (on) u.xray_range = 3;
	    else u.xray_range = -1;
	    vision_full_recalc = 1;
	}
	if ((spfx & SPFX_REFLECT) && (wp_mask & (W_WEP|W_SWAPWEP))) {
	    if (on) EReflecting |= wp_mask;
	    else EReflecting &= ~wp_mask;
	}

	if (wp_mask == W_ART && !on && oart && oart->inv_prop) {
	    /* might have to turn off invoked power too */
	    if (oart->inv_prop <= LAST_PROP &&
		(u.uprops[oart->inv_prop].extrinsic & W_ARTI))
		arti_invoke(otmp);
	}
}

/*
 * creature (usually player) tries to touch (pick up or wield) an artifact obj.
 * Returns 0 if the object refuses to be touched.
 * This routine does not change any object chains.
 * Ignores such things as gauntlets, assuming the artifact is not
 * fooled by such trappings.
 */
int touch_artifact(struct obj *obj, struct monst *mon)
{
    const struct artifact *oart = get_artifact(obj);
    boolean badclass, badalign, self_willed, yours;

    if (!oart) return 1;

    /* [ALI] Thiefbane has a special affinity with shopkeepers */
    if (mon->isshk && obj->oartifact == ART_THIEFBANE) return 1;

    yours = (mon == &youmonst);
    /* all quest artifacts are self-willed; it this ever changes, `badclass'
       will have to be extended to explicitly include quest artifacts */
    self_willed = ((oart->spfx & SPFX_INTEL) != 0);
    if (yours) {
	badclass = self_willed &&
		   ((oart->role != NON_PM && !Role_if (oart->role)) ||
		    (oart->race != NON_PM && !Race_if (oart->race)));
	badalign = (oart->spfx & SPFX_RESTR) && oart->alignment != A_NONE &&
		   (oart->alignment != u.ualign.type || u.ualign.record < 0);
    } else if (!is_covetous(mon->data) && !is_mplayer(mon->data)) {
	badclass = self_willed &&
		   oart->role != NON_PM && oart != &artilist[ART_EXCALIBUR];
	badalign = (oart->spfx & SPFX_RESTR) && oart->alignment != A_NONE &&
		   (oart->alignment != sgn(mon->data->maligntyp));
    } else {    /* an M3_WANTSxxx monster or a fake player */
	/* special monsters trying to take the Amulet, invocation tools or
	   quest item can touch anything except for `spec_applies' artifacts */
	badclass = badalign = FALSE;
    }
    /* weapons which attack specific categories of monsters are
       bad for them even if their alignments happen to match */
    if (!badalign && (oart->spfx & SPFX_DBONUS) != 0) {
	struct artifact tmp;

	tmp = *oart;
	tmp.spfx &= SPFX_DBONUS;
	badalign = !!spec_applies(&tmp, mon);
    }

    if (((badclass || badalign) && self_willed) ||
       (badalign && (!yours || !rn2(4))))  {
	int dmg;
	char buf[BUFSZ];

	if (!yours) return 0;
	pline("You are blasted by %s power!", s_suffix(the(xname(obj))));
	dmg = dice((Antimagic ? 2 : 4), (self_willed ? 10 : 4));
	sprintf(buf, "touching %s", oart->name);
	losehp(dmg, buf, KILLED_BY);
	exercise(A_WIS, FALSE);
    }

    /* can pick it up unless you're totally non-synch'd with the artifact */
    if (badclass && badalign && self_willed) {
	if (yours) pline("%s your grasp!", Tobjnam(obj, "evade"));
	return 0;
    }

    /* The Iron Ball of Liberation removes any mundane heavy iron ball. */
    if (oart == &artilist[ART_IRON_BALL_OF_LIBERATION] &&
	Punished && obj != uball) {
	pline("You are liberated from %s!", yname(uball));
	unpunish();
    }

    return 1;
}


/* decide whether an artifact's special attacks apply against mtmp */
static int spec_applies(const struct artifact *weap, struct monst *mtmp)
{
	const struct permonst *ptr;
	boolean yours;

	if (!(weap->spfx & (SPFX_DBONUS | SPFX_ATTK)))
	    return weap->attk.adtyp == AD_PHYS;

	/* spec_applies_* variables are only reset when this is called
	 * from spec_dbon(), so don't trust them to be fresh if you're
	 * calling this in any other context! */

	yours = (mtmp == &youmonst);
	ptr = mtmp->data;

	if (weap->spfx & SPFX_DMONS) {
	    return ptr == &mons[(int)weap->mtype];
	} else if (weap->spfx & SPFX_DCLAS) {
	    return weap->mtype == (unsigned long)ptr->mlet;
	} else if (weap->spfx & SPFX_DFLAG1) {
	    return (ptr->mflags1 & weap->mtype) != 0L;
	} else if (weap->spfx & SPFX_DFLAG2) {
	    return ((ptr->mflags2 & weap->mtype) || (yours &&
			((!Upolyd && (urace.selfmask & weap->mtype)) ||
			 ((weap->mtype & M2_WERE) && u.ulycn >= LOW_PM))));
	} else if (weap->spfx & SPFX_DALIGN) {
	    return yours ? (u.ualign.type != weap->alignment) :
			   (ptr->maligntyp == A_NONE ||
				sgn(ptr->maligntyp) != weap->alignment);
	} else if (weap->spfx & SPFX_ATTK) {
	    struct obj *defending_weapon = (yours ? uwep : MON_WEP(mtmp));

	    if (defending_weapon && defending_weapon->oartifact &&
		    defends((int)weap->attk.adtyp, defending_weapon))
		return FALSE;
	    switch(weap->attk.adtyp) {
		case AD_FIRE:
			spec_applies_ad_fire = !(yours ? FFire_resistance :
							 resists_fire(mtmp));
			return spec_applies_ad_fire;
		case AD_COLD:
			spec_applies_ad_cold = !(yours ? FCold_resistance :
							 resists_cold(mtmp));
			return spec_applies_ad_cold;
		case AD_ELEC:
			spec_applies_ad_elec = !(yours ? FShock_resistance :
							 resists_elec(mtmp));
			return spec_applies_ad_elec;
		case AD_MAGM:
			spec_applies_ad_magm = !(yours ? Antimagic :
							 (rn2(100) < ptr->mr));
			return spec_applies_ad_magm;
		case AD_STUN:
			spec_applies_ad_stun = !(yours ? Antimagic :
							 (rn2(100) < ptr->mr));
			return spec_applies_ad_stun;
		case AD_DRST:
			return !(yours ? FPoison_resistance : resists_poison(mtmp));
		case AD_DRLI:
			spec_applies_ad_drli = !(yours ? Drain_resistance :
							 resists_drli(mtmp));
			return spec_applies_ad_drli;
		case AD_STON:
			return !(yours ? Stone_resistance : resists_ston(mtmp));
		default:	warning("Weird weapon special attack.");
	    }
	}
	return 0;
}

/* return the M2 flags of monster that an artifact's special attacks apply against */
static long spec_m2(struct obj *otmp)
{
	const struct artifact *artifact = get_artifact(otmp);
	if (artifact)
		return artifact->mtype;
	return 0L;
}

/* special attack bonus */
int spec_abon(struct obj *otmp, struct monst *mon)
{
	const struct artifact *weap = get_artifact(otmp);

	/* no need for an extra check for `NO_ATTK' because this will
	   always return 0 for any artifact which has that attribute */

	if (weap && weap->attk.damn && spec_applies(weap, mon))
	    return rnd((int)weap->attk.damn);
	return 0;
}

/* special damage bonus */
int spec_dbon(struct obj *otmp, struct obj *olaunch, boolean thrown,
	      struct monst *mon, int tmp)
{
	const struct artifact *weap = get_artifact(otmp);
	boolean yours = (mon == &youmonst);

	spec_dbon_applies = FALSE;

	spec_applies_ad_fire = FALSE;
	spec_applies_ad_cold = FALSE;
	spec_applies_ad_elec = FALSE;
	spec_applies_ad_magm = FALSE;
	spec_applies_ad_stun = FALSE;
	spec_applies_ad_drli = FALSE;
	spec_applies_spfx_behead = spec_ability(otmp, SPFX_BEHEAD);

	if (!weap) {
	    /* Object must have properties, which might stack. */
	    int propdmg = 0;
	    struct obj *defwep = (yours ? uwep : MON_WEP(mon));

	    if ((attacks(AD_FIRE, otmp, olaunch, thrown) &&
		!(yours ? FFire_resistance : resists_fire(mon)) &&
		!(defwep && defwep->oartifact && defends(AD_FIRE, defwep)))) {
		spec_dbon_applies = TRUE;
		spec_applies_ad_fire = TRUE;
		propdmg += rnd((yours && PFire_resistance) ? 3 : 6);
	    }

	    if ((attacks(AD_COLD, otmp, olaunch, thrown) &&
		!(yours ? FCold_resistance : resists_cold(mon)) &&
		!(defwep && defwep->oartifact && defends(AD_COLD, defwep)))) {
		spec_dbon_applies = TRUE;
		spec_applies_ad_cold = TRUE;
		propdmg += rnd((yours && PCold_resistance) ? 3 : 6);
	    }

	    /* No AD_MAGM/spec_applies_ad_magm (magic missile) property effect. */

	    /* No AD_STUN/spec_applies_ad_stun (stun) property effect. */

	    /* Life-draining is handled specially in artifact_hit(). */
	    if (attacks(AD_DRLI, otmp, olaunch, thrown) &&
		!(yours ? Drain_resistance : resists_drli(mon)) &&
		!(defwep && defwep->oartifact && defends(AD_DRLI, defwep))) {
		spec_dbon_applies = TRUE;
		spec_applies_ad_drli = TRUE;
	    }

	    /* [SPFX_BEHEAD] This *may* apply; only artifact_hit_behead()
	     * truly decides that. */
	    if (oprop_attack(ITEM_VORPAL, otmp, olaunch, thrown)) {
		spec_dbon_applies = TRUE;
		spec_applies_spfx_behead = TRUE;
	    }

	    return propdmg;
	}

	if (!weap || (weap->attk.adtyp == AD_PHYS && /* check for `NO_ATTK' */
			weap->attk.damn == 0 && weap->attk.damd == 0))
	    spec_dbon_applies = FALSE;
	else
	    spec_dbon_applies = spec_applies(weap, mon);

	if (spec_dbon_applies) {
	    int dmg_bonus = weap->attk.damd ? rnd(weap->attk.damd) : max(tmp, 1);
	    /* halve artifact bonus damage for partial resistances */
	    if (yours && (weap->spfx & SPFX_ATTK)) {
		uchar adtyp = weap->attk.adtyp;
		if ((adtyp == AD_FIRE && PFire_resistance) ||
		    (adtyp == AD_COLD && PCold_resistance) ||
		    (adtyp == AD_ELEC && PShock_resistance) ||
		    (adtyp == AD_DRST && PPoison_resistance)) {
		    dmg_bonus = (dmg_bonus + 1) / 2;
		}
	    }
	    return dmg_bonus;
	}
	return 0;
}

/* add identified artifact to discoveries list */
void discover_artifact(xchar m)
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's not present, so add it */
    for (i = 0; i < NROFARTIFACTS; i++)
	if (artidisco[i] == 0 || artidisco[i] == m) {
	    artidisco[i] = m;
	    return;
	}
    /* there is one slot per artifact, so we should never reach the
       end without either finding the artifact or an empty slot... */
    warning("couldn't discover artifact (%d)", (int)m);
}


/* used to decide whether an artifact has been fully identified */
boolean undiscovered_artifact(xchar m)
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's undiscovered */
    for (i = 0; i < NROFARTIFACTS; i++)
	if (artidisco[i] == m)
	    return FALSE;
	else if (artidisco[i] == 0)
	    break;
    return TRUE;
}


/* display a list of discovered artifacts; return their count */
int disp_artifact_discoveries(
    struct menulist *menu /* supplied by dodiscover() */
    )
{
    int i, m, otyp;
    char buf[BUFSZ];

    for (i = 0; i < NROFARTIFACTS; i++) {
	if (artidisco[i] == 0) break;	/* empty slot implies end of list */
	if (i == 0)
	    add_menuheading(menu, "Artifacts");
	m = artidisco[i];
	otyp = artilist[m].otyp;
	sprintf(buf, "  %s [%s %s]", artiname(m),
		align_str(artilist[m].alignment), simple_typename(otyp));
	add_menutext(menu, buf);
    }
    return i;
}


	/*
	 * Magicbane's intrinsic magic is incompatible with normal
	 * enchantment magic.  Thus, its effects have a negative
	 * dependence on spe.  Against low mr victims, it typically
	 * does "double athame" damage, 2d4.  Occasionally, it will
	 * cast unbalancing magic which effectively averages out to
	 * 4d4 damage (3d4 against high mr victims), for spe = 0.
	 *
	 * Prior to 3.4.1, the cancel (aka purge) effect always
	 * included the scare effect too; now it's one or the other.
	 * Likewise, the stun effect won't be combined with either
	 * of those two; it will be chosen separately or possibly
	 * used as a fallback when scare or cancel fails.
	 *
	 * [Historical note: a change to artifact_hit() for 3.4.0
	 * unintentionally made all of Magicbane's special effects
	 * be blocked if the defender successfully saved against a
	 * stun attack.  As of 3.4.1, those effects can occur but
	 * will be slightly less likely than they were in 3.3.x.]
	 */
#define MB_MAX_DIEROLL		8	/* rolls above this aren't magical */
static const char * const mb_verb[2][4] = {
	{ "probe", "stun", "scare", "cancel" },
	{ "prod", "amaze", "tickle", "purge" },
};
#define MB_INDEX_PROBE		0
#define MB_INDEX_STUN		1
#define MB_INDEX_SCARE		2
#define MB_INDEX_CANCEL		3

/* called when someone is being hit by Magicbane */
static boolean magicbane_hit(
    struct monst *magr,		/* attacker */
    struct monst *mdef,		/* defender */
    struct obj *mb,		/* Magicbane */
    int *dmgptr,		/* extra damage target will suffer */
    int dieroll,		/* d20 that has already scored a hit */
    boolean vis,		/* whether the action can be seen */
    char *hittee		/* target's name: "you" or mon_nam(mdef) */
    )
{
    const struct permonst *old_uasmon;
    const char *verb;
    boolean youattack = (magr == &youmonst),
	    youdefend = (mdef == &youmonst),
	    resisted = FALSE, do_stun, do_confuse, result;
    int attack_indx, scare_dieroll = MB_MAX_DIEROLL / 2;

    result = FALSE;		/* no message given yet */
    /* the most severe effects are less likely at higher enchantment */
    if (mb->spe >= 3)
	scare_dieroll /= (1 << (mb->spe / 3));
    /* if target successfully resisted the artifact damage bonus,
       reduce overall likelihood of the assorted special effects */
    if (!spec_applies_ad_stun) dieroll += 1;

    /* might stun even when attempting a more severe effect, but
       in that case it will only happen if the other effect fails;
       extra damage will apply regardless; 3.4.1: sometimes might
       just probe even when it hasn't been enchanted */
    do_stun = (max(mb->spe,0) < rn2(spec_applies_ad_stun ? 11 : 7));

    /* the special effects also boost physical damage; increments are
       generally cumulative, but since the stun effect is based on a
       different criterium its damage might not be included; the base
       damage is either 1d4 (athame) or 2d4 (athame+spec_dbon) depending
       on target's resistance check against AD_STUN (handled by caller)
       [note that a successful save against AD_STUN doesn't actually
       prevent the target from ending up stunned] */
    attack_indx = MB_INDEX_PROBE;
    *dmgptr += rnd(4);			/* (2..3)d4 */
    if (do_stun) {
	attack_indx = MB_INDEX_STUN;
	*dmgptr += rnd(4);		/* (3..4)d4 */
    }
    if (dieroll <= scare_dieroll) {
	attack_indx = MB_INDEX_SCARE;
	*dmgptr += rnd(4);		/* (3..5)d4 */
    }
    if (dieroll <= (scare_dieroll / 2)) {
	attack_indx = MB_INDEX_CANCEL;
	*dmgptr += rnd(4);		/* (4..6)d4 */
    }

    /* give the hit message prior to inflicting the effects */
    verb = mb_verb[!!Hallucination][attack_indx];
    if (youattack || youdefend || vis) {
	result = TRUE;
	pline("The magic-absorbing blade %s %s!",
		  vtense(NULL, verb), hittee);
	/* assume probing has some sort of noticeable feedback
	   even if it is being done by one monster to another */
	if (attack_indx == MB_INDEX_PROBE && !canspotmon(level, mdef))
	    map_invisible(mdef->mx, mdef->my);
    }

    /* now perform special effects */
    switch (attack_indx) {
    case MB_INDEX_CANCEL:
	old_uasmon = youmonst.data;
	/* No mdef->mcan check: even a cancelled monster can be polymorphed
	 * into a golem, and the "cancel" effect acts as if some magical
	 * energy remains in spellcasting defenders to be absorbed later.
	 */
	if (!cancel_monst(mdef, mb, youattack, FALSE, FALSE)) {
	    resisted = TRUE;
	} else {
	    do_stun = FALSE;
	    if (youdefend) {
		if (youmonst.data != old_uasmon)
		    *dmgptr = 0;    /* rehumanized, so no more damage */
		if (u.uenmax > 0) {
		    pline("You lose magical energy!");
		    u.uenmax--;
		    if (u.uen > 0) u.uen--;
		    iflags.botl = 1;
		}
	    } else {
		if (mdef->data == &mons[PM_CLAY_GOLEM])
		    mdef->mhp = 1;	/* cancelled clay golems will die */
		if (youattack && attacktype(mdef->data, AT_MAGC)) {
		    pline("You absorb magical energy!");
		    u.uenmax++;
		    u.uen++;
		    iflags.botl = 1;
		}
	    }
	}
	break;

    case MB_INDEX_SCARE:
	if (youdefend) {
	    if (Antimagic) {
		resisted = TRUE;
	    } else {
		nomul(-3, "being scared stiff");
		nomovemsg = "";
		if (magr && magr == u.ustuck && sticks(youmonst.data)) {
		    u.ustuck = NULL;
		    u.uwilldrown = 0;
		    pline("You release %s!", mon_nam(magr));
		}
	    }
	} else {
	    if (rn2(2) && resist(mdef, WEAPON_CLASS, 0, NOTELL))
		resisted = TRUE;
	    else
		monflee(mdef, 3, FALSE, (mdef->mhp > *dmgptr));
	}
	if (!resisted) do_stun = FALSE;
	break;

    case MB_INDEX_STUN:
	do_stun = TRUE;		/* (this is redundant...) */
	break;

    case MB_INDEX_PROBE:
	if (youattack && (mb->spe == 0 || !rn2(3 * abs(mb->spe)))) {
	    pline("The %s is insightful.", verb);
	    /* pre-damage status */
	    probe_monster(mdef);
	}
	break;
    }
    /* stun if that was selected and a worse effect didn't occur */
    if (do_stun) {
	if (youdefend)
	    make_stunned((HStun + 3), FALSE);
	else
	    mdef->mstun = 1;
	/* avoid extra stun message below if we used mb_verb["stun"] above */
	if (attack_indx == MB_INDEX_STUN) do_stun = FALSE;
    }
    /* lastly, all this magic can be confusing... */
    do_confuse = !rn2(12);
    if (do_confuse) {
	if (youdefend)
	    make_confused(HConfusion + 4, FALSE);
	else
	    mdef->mconf = 1;
    }

    if (youattack || youdefend || vis) {
	upstart(hittee);	/* capitalize */
	if (resisted) {
	    pline("%s %s!", hittee, vtense(hittee, "resist"));
	    shieldeff(youdefend ? u.ux : mdef->mx,
		      youdefend ? u.uy : mdef->my);
	}
	if ((do_stun || do_confuse) && flags.verbose) {
	    char buf[BUFSZ];

	    buf[0] = '\0';
	    if (do_stun) strcat(buf, "stunned");
	    if (do_stun && do_confuse) strcat(buf, " and ");
	    if (do_confuse) strcat(buf, "confused");
	    pline("%s %s %s%c", hittee, vtense(hittee, "are"),
		  buf, (do_stun && do_confuse) ? '!' : '.');
	}
    }

    return result;
}


/* the tsurugi of muramasa or vorpal blade hit someone */
static boolean artifact_hit_behead(struct monst *magr, struct monst *mdef,
			           struct obj *otmp, struct obj *ostack,
				   struct obj *olaunch,
				   int *dmgptr, int dieroll,
				   boolean prior_damage, boolean *instakilled)
{
    boolean youattack = (magr == &youmonst);
    boolean youdefend = (mdef == &youmonst);
    boolean vis = (!youattack && magr && cansee(magr->mx, magr->my))
	|| (!youdefend && cansee(mdef->mx, mdef->my))
	|| (youattack && u.uswallow && mdef == u.ustuck && !Blind);
    const char *wepdesc;
    char hittee[BUFSZ];
    char wepbuf[BUFSZ];

    snprintf(wepbuf, BUFSZ, "%s %s", youattack ? "Your" : "The",
				     distant_name(otmp, xname));
    wepdesc = wepbuf;

    strcpy(hittee, youdefend ? "you" : mon_nam(mdef));

    *instakilled = FALSE;

    /* We really want "on a natural 20" but Nethack does it in reverse from AD&D. */
    if (bimanual(otmp) && otmp->oartifact != ART_THIEFBANE && dieroll == 1) {
	if (otmp->oartifact == ART_TSURUGI_OF_MURAMASA)
	    wepdesc = "The razor-sharp blade";

	if (!otmp->oartifact)
	    oprop_id(ITEM_VORPAL, otmp, ostack, olaunch);

	/* not really beheading, but so close, why add another SPFX */
	if (youattack && u.uswallow && mdef == u.ustuck) {
	    pline("You slice %s wide open!", mon_nam(mdef));
	    *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
	    return TRUE;
	}

	if (!youdefend) {
		/* allow normal cutworm() call to add extra damage */
		if (notonhead)
		    return FALSE;

		if (bigmonst(mdef->data)) {
			if (youattack)
				pline("You slice deeply into %s!",
					mon_nam(mdef));
			else if (vis)
				pline("%s cuts deeply into %s!",
					Monnam(magr), hittee);
			*dmgptr *= 2;
			return TRUE;
		}
		*dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
		pline("%s cuts %s in half!", wepdesc, mon_nam(mdef));
		*instakilled = TRUE;
		if (otmp->oartifact) otmp->dknown = TRUE;
		return TRUE;
	} else {
		if (bigmonst(youmonst.data)) {
			pline("%s cuts deeply into you!",
				magr ? Monnam(magr) : wepdesc);
			*dmgptr *= 2;
			return TRUE;
		}

		/* Players with negative AC's take less damage instead
		    * of just not getting hit.  We must add a large enough
		    * value to the damage so that this reduction in
		    * damage does not prevent death.
		    */
		*dmgptr = 2 * (Upolyd ? u.mh : u.uhp) + FATAL_DAMAGE_MODIFIER;
		pline("%s cuts you in half!", wepdesc);
		*instakilled = TRUE;
		if (otmp->oartifact) otmp->dknown = TRUE;
		return TRUE;
	}

    } else if ((!bimanual(otmp) &&
		(dieroll == 1 || mdef->data->mlet == S_JABBERWOCK)) ||
	       (otmp->oartifact == ART_THIEFBANE && dieroll < 3)) {
	static const char * const behead_msg[2] = {
		"%s beheads %s!",
		"%s decapitates %s!"
	};

	if (youattack && u.uswallow && mdef == u.ustuck)
		return FALSE;

	if (otmp->oartifact == ART_VORPAL_BLADE ||
	    otmp->oartifact == ART_THIEFBANE)
		wepdesc = artilist[(int)otmp->oartifact].name;

	if (!otmp->oartifact)
		oprop_id(ITEM_VORPAL, otmp, ostack, olaunch);

	if (!youdefend) {
		if (!has_head(mdef->data) || notonhead || u.uswallow) {
		    if (prior_damage) {
			/* Don't nullify prior damage, but don't do
			 * anything else either. */
			return FALSE;
		    } else {
			if (youattack)
				pline("Somehow, you miss %s wildly.",
					mon_nam(mdef));
			else if (vis)
				pline("Somehow, %s misses wildly.",
					mon_nam(magr));
			*dmgptr = 0;
			return (boolean)(youattack || vis);
		    }
		}
		if (noncorporeal(mdef->data) || amorphous(mdef->data)) {
			pline("%s slices through %s %s.", wepdesc,
				s_suffix(mon_nam(mdef)),
				mbodypart(mdef,NECK));
			return TRUE;
		}
		*dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
		pline(behead_msg[rn2(SIZE(behead_msg))],
			wepdesc, mon_nam(mdef));
		*instakilled = TRUE;
		if (otmp->oartifact) otmp->dknown = TRUE;
		return TRUE;
	} else {
		if (!has_head(youmonst.data)) {
		    if (prior_damage) {
			/* Don't nullify prior damage, but don't do
			 * anything else either. */
			return FALSE;
		    } else {
			pline("Somehow, %s misses you wildly.",
				magr ? mon_nam(magr) : wepdesc);
			*dmgptr = 0;
			return TRUE;
		    }
		}
		if (noncorporeal(youmonst.data) || amorphous(youmonst.data)) {
			pline("%s slices through your %s.",
				wepdesc, body_part(NECK));
			return TRUE;
		}
		*dmgptr = 2 * (Upolyd ? u.mh : u.uhp)
			    + FATAL_DAMAGE_MODIFIER;
		pline(behead_msg[rn2(SIZE(behead_msg))],
			wepdesc, "you");
		*instakilled = TRUE;
		if (otmp->oartifact) otmp->dknown = TRUE;
		/* Should amulets fall off? */
		return TRUE;
	}
    }
    return FALSE;
}


static boolean artifact_hit_drainlife(struct monst *magr, struct monst *mdef,
				      struct obj *otmp, struct obj *ostack,
				      struct obj *olaunch,
				      int *dmgptr)
{
    boolean youattack = (magr == &youmonst);
    boolean youdefend = (mdef == &youmonst);
    boolean vis = (!youattack && magr && cansee(magr->mx, magr->my))
	|| (!youdefend && cansee(mdef->mx, mdef->my))
	|| (youattack && u.uswallow && mdef == u.ustuck && !Blind);

    if (!youdefend) {
	    if (vis) {
		if (otmp->oartifact == ART_STORMBRINGER) {
		    pline("The %s blade draws the life from %s!",
			    hcolor("black"),
			    mon_nam(mdef));
		} else {
		    pline("%s draws the life from %s!",
			    The(distant_name(otmp, xname)),
			    mon_nam(mdef));
		    oprop_id(ITEM_DRLI, otmp, ostack, olaunch);
		}
	    }
	    if (mdef->m_lev == 0) {
		*dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
	    } else {
		int drain = rnd(8);
		*dmgptr += drain;
		mdef->mhpmax -= drain;
		mdef->m_lev--;
		drain /= 2;
		if (drain) healup(drain, 0, FALSE, FALSE);
	    }
	    return vis;
    } else { /* youdefend */
	    int oldhpmax = u.uhpmax;

	    if (Blind) {
		    pline("You feel an %s drain your life!",
			otmp->oartifact == ART_STORMBRINGER ?
			"unholy blade" : "object");
	    } else if (otmp->oartifact == ART_STORMBRINGER) {
		    pline("The %s blade drains your life!",
			    hcolor("black"));
	    } else {
		    pline("%s drains your life!",
			    The(distant_name(otmp, xname)));
		    oprop_id(ITEM_DRLI, otmp, ostack, olaunch);
	    }
	    losexp("life drainage");
	    if (magr && magr->mhp < magr->mhpmax) {
		magr->mhp += (oldhpmax - u.uhpmax)/2;
		if (magr->mhp > magr->mhpmax) magr->mhp = magr->mhpmax;
	    }
	    return TRUE;
    }
    return FALSE;
}


/* Function used when someone attacks someone else with an artifact
 * weapon.  Only adds the special (artifact) damage, and returns a 1 if it
 * did something special (in which case the caller won't print the normal
 * hit message).  This should be called once upon every artifact attack;
 * dmgval() no longer takes artifact bonuses into account.  Possible
 * extension: change the killer so that when an orc kills you with
 * Stormbringer it's "killed by Stormbringer" instead of "killed by an orc".
 */
boolean artifact_hit(
    struct monst *magr,
    struct monst *mdef,
    struct obj *otmp,
    struct obj *ostack,
    boolean thrown,
    int *dmgptr,
    int dieroll /* needed for Magicbane and vorpal blades */
    )
{
	boolean youattack = (magr == &youmonst);
	boolean youdefend = (mdef == &youmonst);
	boolean vis = (!youattack && magr && cansee(magr->mx, magr->my))
	    || (!youdefend && cansee(mdef->mx, mdef->my))
	    || (youattack && u.uswallow && mdef == u.ustuck && !Blind);
	boolean realizes_damage;
	boolean msg_printed = FALSE;
	const char *wepdesc;
	char hittee[BUFSZ];
	char wepbuf[BUFSZ];
	struct obj *olaunch = NULL;

	if (thrown) {
	    /* Launcher properties stack with fired ammunition,
	     * so get the launcher here if that's the case. */
	    if (youattack)
		olaunch = uwep;
	    else if (magr)
		olaunch = MON_WEP(magr);

	    if (olaunch && (!is_launcher(olaunch) ||
			    !ammo_and_launcher(otmp, olaunch)))
		olaunch = NULL;
	}

	/* Exit early if not an artifact or absent of attack properties. */
	if (!otmp->oartifact &&
	    !oprop_attack(ITEM_FIRE|ITEM_FROST|ITEM_DRLI|ITEM_VORPAL,
			  otmp, olaunch, thrown))
	    return FALSE;

	snprintf(wepbuf, BUFSZ, "%s %s", youattack ? "Your" : "The",
					 distant_name(otmp, xname));
	wepdesc = wepbuf;

	strcpy(hittee, youdefend ? "you" : mon_nam(mdef));

	/* The following takes care of most of the damage, but not all--
	 * the exception being for level draining, which is specially
	 * handled.  Messages are done in this function, however.
	 */
	*dmgptr += spec_dbon(otmp, olaunch, thrown, mdef, *dmgptr);

	if (youattack && youdefend) {
	    warning("attacking yourself with weapon?");
	    return FALSE;
	}

	realizes_damage = (youdefend || vis || 
			   /* feel the effect even if not seen */
			   (youattack && mdef == u.ustuck));

	/* the four basic attacks: fire, cold, shock and missiles */
	if (attacks(AD_FIRE, otmp, olaunch, thrown)) {
	    if (realizes_damage && (otmp->oartifact || spec_applies_ad_fire)) {
		if (otmp->oartifact == ART_FIRE_BRAND)
		    wepdesc = "The fiery blade";
		pline("%s %s %s%c",
			wepdesc,
			!spec_applies_ad_fire ? "hits" :
			(mdef->data == &mons[PM_WATER_ELEMENTAL]) ?
			"vaporizes part of" : "burns",
			hittee, !spec_applies_ad_fire ? '.' : '!');
		msg_printed = TRUE;
		if (spec_applies_ad_fire)
		    oprop_id(ITEM_FIRE, otmp, ostack, olaunch);
	    }
	    if (!rn2(4)) {
		if (destroy_mitem(mdef, POTION_CLASS, AD_FIRE, dmgptr)) {
		    if (realizes_damage)
			oprop_id(ITEM_FIRE, otmp, ostack, olaunch);
		}
	    }
	    if (!rn2(4)) {
		if (destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE, dmgptr)) {
		    if (realizes_damage)
			oprop_id(ITEM_FIRE, otmp, ostack, olaunch);
		}
	    }
	    if (!rn2(7)) {
		if (destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE, dmgptr)) {
		    if (realizes_damage)
			oprop_id(ITEM_FIRE, otmp, ostack, olaunch);
		}
	    }
	    if (youdefend && Slimed) {
		burn_away_slime();
		oprop_id(ITEM_FIRE, otmp, ostack, olaunch);
	    }
	}

	if (attacks(AD_COLD, otmp, olaunch, thrown)) {
	    if (realizes_damage && (otmp->oartifact || spec_applies_ad_cold)) {
		if (otmp->oartifact == ART_FROST_BRAND)
		    wepdesc = "The ice-cold blade";
		pline("%s %s %s%c",
			wepdesc,
			!spec_applies_ad_cold ? "hits" : "freezes",
			hittee, !spec_applies_ad_cold ? '.' : '!');
		msg_printed = TRUE;
		if (spec_applies_ad_cold)
		    oprop_id(ITEM_FROST, otmp, ostack, olaunch);
	    }
	    if (!rn2(4)) {
		if (destroy_mitem(mdef, POTION_CLASS, AD_COLD, dmgptr)) {
		    if (realizes_damage)
			oprop_id(ITEM_FROST, otmp, ostack, olaunch);
		}
	    }
	}

	if (attacks(AD_ELEC, otmp, olaunch, thrown)) {
	    if (realizes_damage && (otmp->oartifact || spec_applies_ad_elec)) {
		pline("The massive hammer hits%s %s%c",
			  !spec_applies_ad_elec ? "" : "!  Lightning strikes",
			  hittee, !spec_applies_ad_elec ? '.' : '!');
		msg_printed = TRUE;
	    }
	    if (!rn2(5)) destroy_mitem(mdef, RING_CLASS, AD_ELEC, dmgptr);
	    if (!rn2(5)) destroy_mitem(mdef, WAND_CLASS, AD_ELEC, dmgptr);
	}

	if (attacks(AD_MAGM, otmp, olaunch, thrown)) {
	    if (realizes_damage && (otmp->oartifact || spec_applies_ad_magm)) {
		pline("The imaginary widget hits%s %s%c",
			  !spec_applies_ad_magm ? "" :
				"!  A hail of magic missiles strikes",
			  hittee, !spec_applies_ad_magm ? '.' : '!');
		msg_printed = TRUE;
	    }
	}

	if (attacks(AD_STUN, otmp, olaunch, thrown) && dieroll <= MB_MAX_DIEROLL) {
	    /* Magicbane's special attacks (possibly modifies hittee[]) */
	    msg_printed = magicbane_hit(magr, mdef, otmp, dmgptr, dieroll, vis,
					hittee) || msg_printed;
	}

	if (otmp->oartifact != ART_THIEFBANE || !youdefend) {
	    if (!spec_dbon_applies &&
		!spec_applies_ad_drli && !spec_applies_spfx_behead) {
		/* since damage bonus didn't apply, nothing more to do;
		   no further attacks have side-effects on inventory */
		return msg_printed;
	    }
	}

	/* Stormbringer or thirsty weapon */
	if (spec_applies_ad_drli &&
	    (spec_ability(otmp, SPFX_DRLI) ||
	     oprop_attack(ITEM_DRLI, otmp, olaunch, thrown))) {
	    msg_printed = artifact_hit_drainlife(magr, mdef, otmp, ostack, olaunch,
						 dmgptr) || msg_printed;
	}

	/* Thiefbane, Tsurugi of Muramasa, Vorpal Blade or vorpal weapon */
	if (spec_applies_spfx_behead &&
	    (spec_ability(otmp, SPFX_BEHEAD) ||
	     oprop_attack(ITEM_VORPAL, otmp, olaunch, thrown))) {
	    boolean beheaded;
	    msg_printed = artifact_hit_behead(magr, mdef, otmp, ostack, olaunch,
					      dmgptr, dieroll, msg_printed,
					      &beheaded) || msg_printed;
	    /* No further effects if the monster was instakilled. */
	    if (beheaded)
		return msg_printed;
	}

	/* WAC -- 1/6 chance of cancellation with foobane weapons */
	if (otmp->oartifact == ART_ORCRIST ||
	    otmp->oartifact == ART_DRAGONBANE ||
	    otmp->oartifact == ART_DEMONBANE ||
	    otmp->oartifact == ART_WEREBANE ||
	    otmp->oartifact == ART_TROLLSBANE ||
	    otmp->oartifact == ART_THIEFBANE ||
	    otmp->oartifact == ART_OGRESMASHER) {
		if (!mdef->mcan && dieroll < 4) {
		    if (realizes_damage) {
			pline("%s %s as it strikes %s!",
			      The(distant_name(otmp, xname)),
			      Blind ? "roars deafeningly" : "shines brilliantly",
			      hittee);
		    }
		    cancel_monst(mdef, otmp, youattack, TRUE, magr == mdef);
		    msg_printed = TRUE;
		}
	}

	return msg_printed;
}

static const char recharge_type[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
static const char invoke_types[] = { ALL_CLASSES, 0 };
		/* #invoke: an "ugly check" filters out most objects */

int doinvoke(struct obj *obj)
{
    if (obj && !validate_object(obj, invoke_types, "invoke, break, or rub"))
	return 0;
    else if (!obj)
	obj = getobj(invoke_types, "invoke, break, or rub", NULL);
    if (!obj) return 0;
    
    if (obj->oartifact && !touch_artifact(obj, &youmonst))
	return 1;
    return arti_invoke(obj);
}

static int arti_invoke(struct obj *obj)
{
    const struct artifact *oart = get_artifact(obj);

    if (!oart || !oart->inv_prop) {
        if (obj->oclass == WAND_CLASS)
            return do_break_wand(obj);
        else if (obj->oclass == GEM_CLASS || obj->oclass == TOOL_CLASS)
            return dorub(obj);
	else if (obj->otyp == CRYSTAL_BALL)
	    use_crystal_ball(obj);
	else if (obj->otyp == AMULET_OF_YENDOR ||
		 obj->otyp == FAKE_AMULET_OF_YENDOR)
	    /* The Amulet is not technically an artifact
	     * in the usual sense... */
	    return invoke_amulet(obj);
	else
	    pline("Nothing happens.");
	return 1;
    }

    if (oart->inv_prop > LAST_PROP) {
	/* It's a special power, not "just" a property */
	if (obj->age > moves) {
	    /* the artifact is tired :-) */
	    pline("You feel that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
	    /* and just got more so; patience is essential... */
	    obj->age += (long) dice(3,10);
	    return 1;
	}
	obj->age = moves + rnz(100);

	switch(oart->inv_prop) {
	case TAMING: {
	    struct obj pseudo;
	    boolean unused_known;

	    pseudo = zeroobj;	/* neither cursed nor blessed */
	    pseudo.otyp = SCR_TAMING;
	    seffects(&pseudo, &unused_known);
	    break;
	  }
	case HEALING: {
	    int healamt = (u.uhpmax + 1 - u.uhp) / 2;
	    long creamed = (long)u.ucreamed;

	    if (Upolyd) healamt = (u.mhmax + 1 - u.mh) / 2;
	    if (healamt || Sick || Slimed || Blinded > creamed)
		pline("You feel better.");
	    else
		goto nothing_special;
	    if (healamt > 0) {
		if (Upolyd) u.mh += healamt;
		else u.uhp += healamt;
	    }
	    if (Sick) make_sick(0L,NULL,FALSE,SICK_ALL);
	    if (Slimed) Slimed = 0L;
	    if (Blinded > creamed) make_blinded(creamed, FALSE);
	    iflags.botl = 1;
	    break;
	  }
	case ENERGY_BOOST: {
	    int epboost = (u.uenmax + 1 - u.uen) / 2;
	    if (epboost > 120) epboost = 120;		/* arbitrary */
	    else if (epboost < 12) epboost = u.uenmax - u.uen;
	    if (epboost) {
		pline("You feel re-energized.");
		u.uen += epboost;
		iflags.botl = 1;
	    } else
		goto nothing_special;
	    break;
	  }
	case UNTRAP: {
	    if (!untrap(TRUE)) {
		obj->age = 0; /* don't charge for changing their mind */
		return 0;
	    }
	    break;
	  }
	case CHARGE_OBJ: {
	    struct obj *otmp = getobj(recharge_type, "charge", NULL);
	    boolean b_effect;

	    if (!otmp) {
		obj->age = 0;
		return 0;
	    }
	    b_effect = obj->blessed &&
		(Role_switch == oart->role || !oart->role);
	    recharge(otmp, b_effect ? 1 : obj->cursed ? -1 : 0);
	    update_inventory();
	    break;
	  }
	case LEV_TELE:
	    level_tele();
	    break;
	case CREATE_PORTAL: {
	    int i, num_ok_dungeons, last_ok_dungeon = 0;
	    d_level newlev;
	    extern int n_dgns; /* from dungeon.c */
	    struct nh_menuitem *items;

	    /* Prevent branchporting from the Black Market shop. */
	    if (Is_blackmarket(&u.uz) && *u.ushops) {
		pline("You feel very disoriented for a moment.");
		break;
	    }

	    items = malloc(n_dgns * sizeof(struct nh_menuitem));

	    num_ok_dungeons = 0;
	    for (i = 0; i < n_dgns; i++) {
		if (!dungeons[i].dunlev_ureached)
		    continue;
		items[num_ok_dungeons].id = i+1;
		items[num_ok_dungeons].accel = 0;
		items[num_ok_dungeons].role = MI_NORMAL;
		items[num_ok_dungeons].selected = FALSE;
		strcpy(items[num_ok_dungeons].caption, dungeons[i].dname);
		num_ok_dungeons++;
		last_ok_dungeon = i;
	    }

	    if (num_ok_dungeons > 1) {
		/* more than one entry; display menu for choices */
		int n;
		int selected[1];

		n = display_menu(items, num_ok_dungeons, "Open a portal to which dungeon?", PICK_ONE, selected);
		free(items);
		if (n <= 0)
		    goto nothing_special;
		
		i = selected[0] - 1;
	    } else {
		free(items);
		i = last_ok_dungeon;	/* also first & only OK dungeon */
	    }

	    /*
	     * i is now index into dungeon structure for the new dungeon.
	     * Find the closest level in the given dungeon, open
	     * a use-once portal to that dungeon and go there.
	     * The closest level is either the entry or dunlev_ureached.
	     */
	    newlev.dnum = i;
	    if (dungeons[i].depth_start >= depth(&u.uz))
		newlev.dlevel = dungeons[i].entry_lev;
	    else
		newlev.dlevel = dungeons[i].dunlev_ureached;
	    if (u.uhave.amulet || In_endgame(&u.uz) || In_endgame(&newlev) ||
	       newlev.dnum == u.uz.dnum) {
		pline("You feel very disoriented for a moment.");
	    } else {
		if (!Blind) pline("You are surrounded by a shimmering sphere!");
		else pline("You feel weightless for a moment.");
		goto_level(&newlev, FALSE, FALSE, FALSE);
	    }
	    break;
	  }
	case ENLIGHTENING:
	    enlightenment(0);
	    break;
	case CREATE_AMMO: {
	    struct obj *otmp = mksobj(level, ARROW, TRUE, FALSE);

	    if (!otmp) goto nothing_special;
	    otmp->blessed = obj->blessed;
	    otmp->cursed = obj->cursed;
	    otmp->bknown = obj->bknown;
	    if (obj->blessed) {
		if (otmp->spe < 0) otmp->spe = 0;
		otmp->quan += rnd(10);
	    } else if (obj->cursed) {
		if (otmp->spe > 0) otmp->spe = 0;
	    } else
		otmp->quan += rnd(5);
	    otmp->owt = weight(otmp);
	    hold_another_object(otmp, "Suddenly %s out.", aobjnam(otmp, "fall"), NULL);
	    break;
	  }
	case PHASING:	/* walk through walls and stone like a xorn */
	    if (Passes_walls) goto nothing_special;
	    if (oart == &artilist[ART_IRON_BALL_OF_LIBERATION]) {
		if (Punished && obj != uball) {
		    pline("You are liberated from %s!", yname(uball));
		    unpunish(); /* remove a mundane heavy iron ball */
		}

		if (!Punished) {
		    setworn(mkobj(level, CHAIN_CLASS, TRUE), W_CHAIN);
		    setworn(obj, W_BALL);
		    uball->spe = 1;	/* attach the Iron Ball of Liberation */
		    if (!u.uswallow) {
			placebc();
			if (Blind) set_bc(1);	/* set up ball and chain variables */
			newsym(u.ux, u.uy);	/* see ball&chain if can't see self */
		    }
		    pline("Your %s chains itself to you!", xname(obj));
		}
	    }
	    if (!Hallucination)
		pline("Your body begins to feel less solid.");
	    else
		pline("You feel one with the spirit world.");
	    incr_itimeout(&Phasing, (50 + rnd(100)));
	    obj->age += Phasing; /* Time begins after phasing ends */
	    break;
	}
    } else {
	long eprop = (u.uprops[oart->inv_prop].extrinsic ^= W_ARTI),
	     iprop = u.uprops[oart->inv_prop].intrinsic;
	boolean on = (eprop & W_ARTI) != 0; /* true if invoked prop just set */

	if (on && obj->age > moves) {
	    /* the artifact is tired :-) */
	    u.uprops[oart->inv_prop].extrinsic ^= W_ARTI;
	    pline("You feel that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
	    /* can't just keep repeatedly trying */
	    obj->age += (long) dice(3,10);
	    return 1;
	} else if (!on) {
	    /* when turning off property, determine downtime */
	    /* arbitrary for now until we can tune this -dlc */
	    obj->age = moves + rnz(100);
	}

	if ((eprop & ~W_ARTI) || iprop) {
nothing_special:
	    /* you had the property from some other source too */
	    if (carried(obj))
		pline("You feel a surge of power, but nothing seems to happen.");
	    return 1;
	}
	switch(oart->inv_prop) {
	case CONFLICT:
	    if (on) pline("You feel like a rabble-rouser.");
	    else pline("You feel the tension decrease around you.");
	    break;
	case LEVITATION:
	    if (on) {
		float_up();
		spoteffects(FALSE);
	    } else float_down(I_SPECIAL|TIMEOUT, W_ARTI);
	    break;
	case INVIS:
	    if (BInvis || Blind) goto nothing_special;
	    newsym(u.ux, u.uy);
	    if (on)
		pline("Your body takes on a %s transparency...",
		     Hallucination ? "normal" : "strange");
	    else
		pline("Your body seems to unfade...");
	    break;
	}
    }

    return 1;
}


/* WAC return TRUE if artifact is always lit */
boolean artifact_light(const struct obj *obj)
{
    return get_artifact(obj) && obj->oartifact == ART_SUNSWORD;
}

/* KMH -- Talking artifacts are finally implemented */
void arti_speak(struct obj *obj)
{
	const struct artifact *oart = get_artifact(obj);
	const char *line;
	char buf[BUFSZ];


	/* Is this a speaking artifact? */
	if (!oart || !(oart->spfx & SPFX_SPEAK))
		return;

	line = getrumor(bcsign(obj), buf, TRUE);
	if (!*line)
		line = "Rumors file closed for renovation.";
	pline("%s:", Tobjnam(obj, "whisper"));
	verbalize("%s", line);
	return;
}

boolean artifact_has_invprop(struct obj *otmp, uchar inv_prop)
{
	const struct artifact *arti = get_artifact(otmp);

	return (boolean)(arti && (arti->inv_prop == inv_prop));
}

/* Return the price sold to the hero of a given artifact or unique item */
long arti_cost(const struct obj *otmp)
{
	if (!otmp->oartifact)
	    return (long)objects[otmp->otyp].oc_cost;
	else if (artilist[(int) otmp->oartifact].cost)
	    return artilist[(int) otmp->oartifact].cost;
	else
	    return 100L * (long)objects[otmp->otyp].oc_cost;
}

boolean match_warn_of_mon(const struct monst *mon)
{
	/* warned of S_MONSTER? */
	if (uwep && uwep->oartifact) {
	    const struct artifact *arti = get_artifact(uwep);
	    if (arti->spfx & SPFX_WARN_S &&
		arti->mtype && arti->mtype == mon->data->mlet) {
		return TRUE;
	    }
	}

	return Warn_of_mon && flags.warntype &&
	       (flags.warntype & mon->data->mflags2);
}

/* Return the plural name of the monster the player is warned about
 * with SPFX_WARN_S.
 */
const char *get_warned_of_monster(const struct obj *otmp)
{
	if (otmp && otmp->oartifact) {
	    const struct artifact *arti = get_artifact(otmp);
	    if (arti->spfx & SPFX_WARN_S && arti->mtype) {
		switch (arti->mtype) {
		case S_TROLL: return "trolls"; break;
		case S_DRAGON: return "dragons"; break;
		case S_OGRE: return "ogres"; break;
		case S_JABBERWOCK: return "jabberwocks"; break;
		default: return "something"; break;
		}
	    }
	}

	return NULL;
}

/*artifact.c*/
