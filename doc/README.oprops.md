# Magical Object Properties (ported from GruntHack to NitroHack)

Normally in NetHack, objects have magical properties depending on their base
object type, e.g. speed boots grant speed.  GruntHack adds to NetHack the
ability for many other objects to randomly possess these kinds of magical
properties, e.g. water walking boots of speed, leather gloves of speed, amulet
of unchanging and speed, ring of adornment and speed, etc. similar to "ego"
items in Angband.

This document describes this feature as ported from GruntHack to this
development branch based on NitroHack, extracted and isolated from thousands of
lines of unrelated changes that GruntHack makes to NetHack.

Though this change is heavily based on code from GruntHack, it fixes major bugs
and oversights that Steve "Grunt" Melenchuk missed.  There are too many to
list, but the biggest problems with GruntHack's implementation are:

 * Stacked attack properties never inflicted stacked damage, even though they
   stacked the messages.

 * The automatic identification of properties of an object based on observed
   effects did not spread to the object stack from which it came in many cases.
   GruntHack caught a few, but missed many more.

 * Hunger, aggravation and fumbling properties had no effect on wielded
   weapons.

 * Several instances of armor destruction would grant their properties
   permanently to the player.

 * Cancelling and disenchantment of equipped objects with properties could
   grant permanent attribute bonuses and penalties.

 * Objects of reflection worn by monsters would protect the monster as if they
   had reflected it, but didn't actually continue beams in the opposite
   direction.

 * Explosions from objects of detonation could appear in a completely different
   place to the object's place of impact.

 * The wish parsing made it impossible to wish for certain kinds of objects,
   e.g. "cloak of magic resistance" would produce a random cloak, "shield of
   reflection" would grant a random shield with the reflection property.

 * Excessively-stacked properties on objects could lead to buffer overflows in
   object naming functions.


## Part 1: Gameplay Rules

This section describes how object properties work in gameplay terms.  In
general, properties only apply to weapons/weapon-tools, armors, amulets and
rings.

Properties only have an effect on weapons and weapon-tools (not ammunition or
missiles) when wielded, or in the off-hand when two-weaponing.  Properties for
armors, amulets and rings apply when worn (and not wielded).  Attack properties
only apply when used as they were designed, e.g. melee weapons apply on any
hit, ammunition and launchers apply only when firing, missiles apply only when
thrown.

Properties may stack, but it's very rare, as described below; however, the
properties of ammunition and their launchers will stack when fired, e.g. arrows
of fire shot from a bow of frost inflicts fire and cold effects on their
target.

Artifacts and object properties are mutually exclusive, for gameplay balance
and implementation reasons.  Objects with properties that become artifacts have
their properties stripped from them, e.g. a long sword of warning dipped to
become Excalibur loses its warning property.  If the game tries and fails to
create an artifact, random properties are applied to it.

An object with properties will lose them when polymorphed.

An object with properties will *not* lose them when cancelled.  Objects of
dexterity and brilliance have the correct effects when cancelled or
disenchanted.


### List of Object Properties

There are 20 properties in total.  These properties apply to most objects:

 * fire (resistance) - On weapons/weapon-tools inflicts +d6 extra fire damage
   and destroys fire-vulnerable inventory objects; on armor/amulets/rings
   confers fire resistance

 * frost (resistance) - Like fire, but with cold.

 * thirsty (drain resistance) - On weapons/weapon-tools drains life like
   Stormbringer (d8 HP vs. monsters, an experience level vs. the player), e.g.
   thirsty stiletto; on armor/amulets/rings confers drain resistance.

 * reflection - Grants reflection when wielded or worn.  Not permitted on
   helmets, gloves or boots.

 * telepathy (or ESP) - Grants extrinsic telepathy, like an amulet of ESP.

 * searching - Grants automatic searching.

 * warning - Grants warning.

 * stealth - Grants stealth.  With UnNetHack's rules, weapons of stealth also
   create no noise when hit with.

 * fumbling - Causes fumbling, like gauntlets of fumbling or fumble boots.

 * hunger - Causes hunger, like a ring of hunger.

 * aggravation - Aggravates monsters, causing them to generate awake, like a
   ring of aggravate monster.

The following properties apply only to weapons:

 * vorpal (blade weapons only) - One-handed blades will sometimes behead like
   Vorpal Blade, e.g. vorpal knife.  Two-handed weapons bisect or cut deeply
   like the Tsurugi of Muramasa, e.g. vorpal two-handed sword.  Never randomly
   generated.

 * detonation (missiles, launchers and ammo only) - Creates a fiery
   explosion that inflicts 4d4 damage and scatters object piles on impact.
   Destroys the thrown or fired object, unless it's an artifact or important
   item.

The following properties apply only to armors, amulets and rings:

 * speed - Grants extrinsic speed, like speed boots.

 * oilskin (cloth-material objects only) - Object is immune to water damage,
   e.g. oilskin mummy wrapping.  If on a cloak, protects against inventory
   water damage and grabbing attacks like a real oilskin cloak.  Oilskin
   containers would protect their contents like an oilskin sack, if they were
   ever allowed to be generated or wished for, which they currently aren't.

 * power - Grants 25 strength, like gauntlets of power.  Does *not* restrict
   spell-casting or firing accuracy like actual gauntlets of power.

 * dexterity - Grants bonus dexterity based on enchantment, like a ring of gain
   dexterity.

 * brilliance - Grants bonus intelligence and wisdom based on enchantment, like
   a helm of brilliance.

 * displacement - Grants displacement, like a cloak of displacement.

 * clairvoyance - Grants clairvoyance, i.e. occasional nearby magic mapping,
   like the donation effect or a cornuthaum worn by a wizard.


### Random Generation of Properties on Objects

As mentioned before, properties are only permitted on weapons, weapon-tools,
armors, amulets and rings.

If an object gets any properties, they will get at least one randomly, and have
a successive 1 in 250 chance of getting each extra random property.

Objects with bad properties, like hunger, aggravation and fumbling have a 90%
of being cursed.

Weapons and weapon-tools have a (difficulty)% chance of having a property,
where difficulty in NetHack and most of its variants is the average of the
current dungeon level and the player's experience level.

[Aside: The difficulty formula is true of NitroHack, but not of NetHack4, and
this development branch of NitroHack adopts NetHack4's save system, pulling in
the (IMO unnecessary) object migration chain removal and thus the change of
level difficulty to be simply the current dungeon level, without the player's
experience level.]

Armor has a flat 1 in 100 chance of having at least one random property.

Amulets (besides the Amulet of Yendor) and rings have a 1 in 250 chance of
having at least one random property.


### Identification of Objects with Properties

Any object with properties will be observed as "magical", e.g. a T-shirt of
warning will initially appear as "a magical T-shirt".

Knowledge of each property on an object is tracked separately, making it
possible to know that an object has, say, the speed property, while not
formally knowing that it also has fire resistance.

"Magical" will not appear as a prefix on objects that have any properties known
already, e.g. an orcish helm of fire resistance and speed would appear as "a
magical orcish helm" when neither property is known, but only "an orcish helm
of speed" if the speed is known but the fire resistance is not.

[Aside: In GruntHack, only wizards had this ability, and other characters
needed to use the new scroll of detect magic to sense the presence of
properties on objects on the current level.  The main problem with this is the
same as +5 armor hiding amongst piles of +0 junk: players would just skip them
unless they were willing to tediously ferry loads of armor to an altar on
another level.]

Many, but not all properties, will identify when they cause an obvious effect,
e.g. "magical studded leather armor" that reflects something will identify the
reflection, giving "studded leather armor of reflection".


### Wishing Rules

With UnNetHack wishing rules, wishes for magical objects are restricted to
wands of wishing.  Properties on objects are considered magical and are thus
only allowed from wands of wishing.

Unless you're in the debugging "wizard mode", only one property is allowed on
an object that is wished for, and no properties can be wished for on any object
whose base type is already magical (e.g. cloak of magic resistance) or has some
special ability (e.g. ring of increase damage).

[Aside: Oilskin cloaks are the exception to this, since they aren't flagged as
magical in the object list, so an "oilskin cloak of reflection" is a possible
wish. I'm unsure if they should have wished property restrictions for
consistency, since the oilskin effect is fairly mundane in terms of gameplay.]

Wishing for more than one property chooses one of them at random to be applied.

The following prefixes are recognized by the wish parser:

 * magical
 * vorpal
 * thirsty
 * oilskin

Wishing for a "magical" object without any other properties applies a random
property, like randomly-generated objects.  "Magical" will be ignored if any
other properties are given.  Wishing for an object without any properties will
never grant any properties.

Wishing for an "oilskin cloak of magic resistance" grants a cloak of magic
resistance with the oilskin property, assuming the cloak is made of cloth,
otherwise the "oilskin" is ignored.

For the ambiguous case of "oilskin cloak of displacement", where "oilskin" and
"displacement" are valid properties, this grants a cloak of displacement with
"oilskin" for consistency with the other cloaks; if you want to work around
this and get an oilskin cloak with "displacement", you can work around this by
exploiting the fact that the wish parser mostly considers "of" and "and" to be
the same, and wishing for an "oilskin cloak and displacement".  This wish
parsing case does *not* fall back on alternate interpretations if "oilskin" is
not valid for the cloak that is wished for.

Suffixes cover most of the properties, as long as they're preceded by an "of"
or "and":

 * fire (resistance)
 * frost/cold (resistance)
 * drain
 * reflection
 * speed
 * power
 * dexterity
 * brilliance
 * telepathy/ESP
 * displacement
 * warning
 * searching
 * stealth
 * fumbling
 * clairvoyance
 * detonation(s)
 * hunger
 * aggravation

The "resistance" in things like "dented pot of fire resistance" is optional.
In fact, it can technically be anything that doesn't contain "of" or "and" with
spaces around it; this part of the wish parsing is permissive like that.

Properties that may be prefixes or suffixes accept either, e.g. "thirsty
leather gloves" is equivalent to "leather gloves of drain resistance".


### Price of Properties on Objects

Properties add to the price of an object:

 * 1 property adds 100 zorkmids
 * 2 properties add 500 zorkmids
 * 3 properties add 1000 zorkmids
 * 4 properties add 5000 zorkmids
 * etc.

In general, the extra price is alternately multiplied by 2 and 5 for each
successive property.

[Aside: Any more than 2 properties on an object outside of wishing in wizard
mode would be absurdly rare.]

The price is independent of the player's knowledge of the properties.  In fact,
it's even independent of the properties themselves: shopkeepers are perfectly
happy to pay a the extra 1000 zorkmids (before deducting the base price for
selling) for a knife of fumbling and hunger and aggravation; all of this logic
comes from GruntHack, so I don't know if this was intended, or an area of
potential improvement.


### Objects with Properties as Sacrifice Gifts

A corpse sacrifice in NetHack, assuming no other effects, has a 1 in (10 +
gifts given + number of artifacts) chance of granting an artifact gift of the
altar's alignment.

With the introduction of random magical properties for objects, the player may
receive the gift of a weapon with random properties if the artifact gift fails.
The chance of this is 1 in (10 + 2 * gifts given).

[Aside: GruntHack grants weapons with properties with a chance of 1 in (10 + 5
* gifts given), and makes actual artifact gifts much rarer: the first artifact
is granted 1 in 25 times, successive ones are given only 1 in (100 * artifact
gifts + 2 * artifact gifts * number of artifacts) times.  GruntHack also has a
2 in 3 chance of granting spellbooks to wizards instead of a weapon with
properties.]

The weapon is randomly chosen from the highest weapon skills that have been
advanced.  The weapon itself may be tweaked according to race and/or role:

 * daggers: elven/orcish daggers for elves/orcs, athames for wizards
 * knives: scalpels for healers
 * short swords: elven/orcish/dwarvish for those races, wakizashi for samurai
 * broadswords: elven broadswords for elves
 * long swords: katana for samurai
 * two-handed swords: tsurugi for samurai
 * bows: elven/orcish bows for elves/orcs, yumi for samurai

No bad properties will ever be applied to such a gift, i.e. no fumbling, hunger
or aggravation.  The gift is uncursed if it was initially generated cursed, and
blessed otherwise, and if it was initially of negative enchantment it will be
made at least +0, or made +1 or +2 if it was already at +0.


### Objects with Properties generated with Monsters

In NetHack, Angels are created with a long sword, which has a chance to be
christened Sunsword or Demonbane.  The introduction of object properties will
apply random non-detrimental properties (almost always just one) to that long
sword if the artifact chosen already exists.

Player-monsters found on the Astral Plane in NetHack come with role-suitable
weapons, which have a 1 in 2 chance of being turned into an artifact if that's
possible.  The introduction of object properties changes this into a 1 in 5
chance; the other 4 in 5 applies non-detrimental properties (almost always just
one) to the weapon instead.

[Aside: This almost certainly reduces the chances of Vorpal Blade appearing.
Coupled with the fact that the vorpal property is never randomly applied and
the likelihood of being beheaded by a weapon is lower than it otherwise would
be.  I don't know if this is intended.]


### Thoughts and Further Ideas

The introduction of additional magical properties to equipment has the
potential to add an extra dimension of tactics and strategy to greatly enhance
the standard meta-game of NetHack.

One of the most important aspects to winning NetHack is the so-called
"Ascension Kit": a collection of objects that maximizes the chance of winning
the game.  This includes sources of reflection and magic resistance, with
extrinsic speed as a desirable extra.  The sources of these properties is very
constrained, leading to very few viable equipment combinations post-Castle:
reflection only comes from dragon armor/amulet/shield, magic resistance only
comes from dragon armor/cloak/some artifacts, and speed only comes from boots.
The current magical object properties permit speed and reflection to appear on
other equipment, allowing equipment slots once consumed by them to be freed to
explore NetHack's otherwise diverse roster of magical effects, e.g. alternative
cloaks and boots.

Astute readers will note that the knowledge of whether an object is magical
applies to all characters equally in this port, versus only for wizards and as
a new scroll effect in GruntHack; this is not an accident.  Random magical
properties to be found on equipment has the ability to make exploring the
dungeon more appealing than simply digging through it, but for that to be the
case, the discovery of such magical equipment needs to be efficient enough to
allow it.  As an anti-example, consider that most randomly highly-enchanted
armor gets left on the floor, because there's no way to sift it from the
average piece of junk armor besides hauling it all to an altar and wearing and
taking off each one.  Having a scroll that detects magic leads into a deeply
embedded problem in NetHack's design, which is that any consumable that can be
used immediately is better stashed, either to be efficiently blessed before use
(each blessing costs a holy water but works on a whole stack of objects, so you
want to do it as little as possible with as many objects stacked as possible),
or converted (blanked/diluted/polymorphed) into something more useful; a scroll
of detect magic would likely be hoarded rather than used, leading to magical
objects being overlooked like unknown highly-enchanted armor.  In any case, if
random magical properties are to influence exploration, the stock probabilities
to generate them will need play-testing and adjusting (they might be too rare
right now), otherwise the only awareness of this aspect of the game will be via
wishing.

Wishing in NetHack means that no item is truly rare or special: no matter the
item (quest artifact, Amulet of Yendor, invocation items and more wishes
notwithstanding), any character that reaches the guaranteed wand of wishing the
Castle can wish for it.  But with this port of magical object properties, the
rules for wishing for object properties are deliberately limited: to
successfully wish for a property on an object, the object base type must not
have any other magical effect, and even then only one property is allowed.  In
contrast, random object generation can apply magical properties regardless of
existing effects and in rare cases even apply more than one.  For the first
time in NetHack, randomly-generated objects can be more powerful than those
that can be wished for; in other words, this adds truly rare objects to the
game.

Randomly finding a weapon with a magical property puts pressure on the player
to take advantage of it, leading to incentives to raise weapon skills other
than the one for the artifact weapon they intend to take with them to the
end-game.  For example, bows are 30 weight and their arrows are extremely
common.  Crossbows are 50 weight and their bolts are not only less common, they
even do less damage (d4/d6 versus the arrow's d6/d6)!  However, a
randomly-found crossbow of fire inflicts +d6 fire damage to its bolts when
shooting fire-vulnerable monsters, easily out-damaging the average bow and
giving players a reason to advance the otherwise-ignored crossbow weapon skill.
Even without multi-shot, a thirsty short sword with the short sword skill
advanced is often more desirable than a mundane long sword.  Magical properties
adds an interesting and rewarding alternative to simply advancing the weapon
skill for the weapon intended for use in the late- and end-game.

Armors, amulets and rings may come with the fire/cold/drain resistance
properties, which are extrinsic resistances.  Another long-standing issue in
NetHack is that extrinsic resistances are functionally equivalent to intrinsic
resistances.  A leather cloak of fire resistance might be useful early, but
once intrinsic fire resistance is gained from, say, eating red molds and fire
ants, it becomes at best completely superfluous and at worst a waste of the
cloak slot.  Having extrinsic resistances remain relevant creates interesting
trade-offs in equipment strategy, perhaps by adding perks for extrinsics that
intrisics don't get, e.g. extrinsic fire resistance protects your inventory
from fire damage.  A more dramatic move would be to make acquired intrinsics
only halve damage instead of proving complete immunity, the latter only coming
from an extrinsic source; in this case, reflection would also need to be
adjusted by, say, reworking breaths so they are no longer beams and having
breaths not be reflectable, which would also make ranged elemental attackers in
the late-game distinct from the average bag of hit points.

Some magical properties on objects are clearly inferior to the original objects
that inspired them, e.g. a long sword of fire with its +d6 fire damage pales
next to Fire Brand's double damage.  However, a number of properties are in
fact the opposite, e.g. a vorpal silver saber is strictly superior to Vorpal
Blade, leather gloves of power have none of the downsides of gauntlets of
power, etc.  Base objects and artifacts should feel more distinct from their
property counterparts, and there are interesting ways to achieve this.  Vorpal
Blade could add +1% chance to behead for each point of enchantment (on top of
its base 5%).  Gauntlets of power could add their enchantment to damage like a
ring of increase damage.  A genuine ring of searching could instantly find all
adjacent hidden things.  There's a lot of room for creativity in this space.

Magical properties are currently restricted both in generation and in wishing
to weapons, weapon-tools, armors, amulets and rings.  There is support for
oilskin containers to protect their contents from water damage in the game
logic, but since containers are tools, they cannot receive the oilskin property
via random generation or wishing.


## Part 2: Implementation Guide

Code-wise, this is a really big change that covers an enormous scope:

 * object naming
 * wish parsing
 * the precise mechanics of wearing armors, amulets and rings
 * setting of extrinsics with armors, amulets and rings
 * setting of extrinsics with wielded artifact weapons
 * attack effects of hits from artifact weapons

This amounts to about 2200 lines of code added and about 450 removed.  This
section provides a high-level summary of how this all works.


### Data Representation of Object Properties

The properties of an object are defined as, in include/obj.h:

```c
struct obj {
    /* ... */
    long oprops;
    long oprops_known;
    /* ... */
};
```

Each bit of oprops represents a property on the object, and each matching bit
of oprops_known is set based on if the player formally knows that that property
exists on the object.  The C standard guarantees that the long type has at
least 32 bits.  The 20 properties themselves are set as follows, at the bottom
of include/obj.h:

```c
#define ITEM_FIRE         0x00000001L
#define ITEM_FROST        0x00000002L
#define ITEM_DRLI         0x00000004L /* drain life */
#define ITEM_VORPAL       0x00000008L
#define ITEM_REFLECTION   0x00000010L
#define ITEM_SPEED        0x00000020L
#define ITEM_OILSKIN      0x00000040L
#define ITEM_POWER        0x00000080L
#define ITEM_DEXTERITY    0x00000100L
#define ITEM_BRILLIANCE   0x00000200L
#define ITEM_ESP          0x00000400L /* telepathy */
#define ITEM_DISPLACEMENT 0x00000800L
#define ITEM_SEARCHING    0x00001000L
#define ITEM_WARNING      0x00002000L
#define ITEM_STEALTH      0x00004000L
#define ITEM_FUMBLING     0x00008000L
#define ITEM_CLAIRVOYANCE 0x00010000L
#define ITEM_DETONATIONS  0x00020000L /* fiery explosion when fired/thrown */
#define ITEM_HUNGER       0x00040000L
#define ITEM_AGGRAVATE    0x00080000L
```

ITEM_FIRE, ITEM_FROST and ITEM_DRLI are "x of fire", "x of frost" and "thirsty
x" on weapons and weapon-tools, and "fire/frost/drain resistance" on armors,
amulets and rings.

A special bit is set in oprops_known (as opposed to adding, say, an extra
"magic known"/mknown bit to struct obj) to mark if the player recognizes that
an object has properties, but doesn't know any of them yet:

```c
#define ITEM_MAGICAL      0x80000000L
```

You can compare each bit of oprops to one of the potential values of oc_oprop
in struct objclass, which was likely where GruntHack drew inspiration for the
field names.


### Restrictions of Properties on Objects

Whether randomly-generated or applied via a wish, there are restrictions to
which properties can apply to what objects, applied by restrict_oprops() in
src/artifact.c.

The very first thing to note is that properties can *never* apply to artifacts.
The code for dealing with attack properties shares function call points with
artifacts, but the actual code paths for artifact hits and attack property hits
are mutually exclusive.  Combined with thematic ("thirst Excalibur", really?)
and balance concerns, there is no good reason to address this.

The code that stops artifacts from receiving properties is actually in
create_oprop(), whose main purpose is to add one or more properties to an
existing object.  If it gets an artifact it simply returns early.  All forms of
random property application to objects use that function.  The only other
source of property application, wishes, also manually checks for artifacts
before acting.  Looking back, adding the artifact check to restrict_oprops() as
well or instead would be a potential improvement to this major feature port.

In any case, back to restrict_oprops().  Objects that can receive properties
are divided into eight categories:

 1. blades - bladed weapons, as considered by is_blade(); not to be confused
             with weapons that do slashing damage
 2. launchers - bows, slings and crossbows, as considered by is_launcher()
 3. projectiles - ammo for launchers (arrows, gems and bolts) and missiles
                  (darts, shuriken and boomerangs)
 4. other weapons - any weapon or weapon-tool that isn't counted in the above
 5. small armors - helmets, gauntlets and boots
 6. other armors - any armor that isn't a small armor
 7. amulets
 8. rings

No other objects are currently eligible to receive properties; the only case
this doesn't cover which perhaps should be are containers to permit things like
oilskin bags of holding.

Once classified, the properties are then permitted as follows:

 * fire, frost, drain life, stealth - all objects
 * telepathy, searching, warning, fumbling, hunger, aggravation - all objects
   except projectiles
 * reflection - all objects except projectiles and small armors
 * vorpal - blades only
 * detonations - launchers and projectiles only
 * speed, oilskin, power, displacement, clairvoyance - small armors, other
   armors, amulets and rings
 * dexterity, brilliance - small armors, other armors and rings (not amulets)

Furthermore, "oilskin" cannot apply to any object that is not made from cloth.
This can lead to game-to-game variation of what can or cannot be "oilskin", for
example, an "oilskin cloak of magic resistance" is allowed if its random
description is something like "opera cloak" (which is cloth), but not if it is
"leather cloak" (which is leather).

Objects that cannot be charged cannot get "dexterity" or "brilliance", which is
why amulets are excluded early.  This prevents rings that can't be charged from
receiving them.

Properties that overlap with the base property of an object (i.e. oc_oprop in
struct objclass) are not permitted, preventing cases like "shield of reflection
and reflection", "speed boots of speed" and so on.

Certain objects have properties that are handled specially by NetHack instead
of using a value set for oc_oprop in its struct objclass, e.g. oilskin cloaks
and bags, the gain attribute rings, gauntlets of power/dexterity and the helm
of brilliance.  restrict_oprops() special-cases these to prevent overlaps too,
catching duplicated properties like "oilskin oilskin cloak", "gauntlets of
power and power" and so on.

[Aside: GruntHack does nothing to prevent properties from overlapping like
this in either of the above cases.]

[Aside 2: This issue with objects that would seem to have properties but don't
have an oc_oprop set appears again when restricting properties on wishes.
There's a strong argument for consolidating the code that checks for properties
in a common function, but I haven't because I'm not sure how restricted
properties for wishes should be yet.]

Wishes apply further restrictions to properties, which are applied near the
bottom of the readobjnam() function in src/objnam.c, which is the main wish
parsing function in NetHack:

 1. Again, only weapons, weapon-tools, armors, amulets and ring may receive
    properties.

 2. Objects that are magical (i.e. have the oc_magic flag set in their struct
    objclass) cannot receive any properties.

 3. Objects that already possess some special property (i.e. have an oc_oprop
    set, or are handled specially without one as mentioned earlier) cannot
    receive any properties.

 4. Only one property is permitted.  Wishing for more than one results in one
    of them being applied at random.

If wishes are made in wizard mode, only the first restriction applies.  This
could lead to absurdly-long object names that lead to crashes, which is what
motivated special name-shortening logic that is described in detail later in
the object-naming section.

[Aside: In contrast, GruntHack allows as many of fumbling/hunger/aggravation as
the player asks for, limits objects to one good property per wish, and makes no
restrictions other than the first.]

Under UnNetHack's rules, one more restriction applies, checked in makewish() in
src/zap.c: objects with properties count as magical wishes, meaning that they
can only be obtained from a wand of wishing.


### Generation of Objects with Properties

Properties randomly found on objects are applied by create_oprop() in
src/artifact.c, which can be pseudo-code summarized like this:

```c
struct obj *create_oprop(struct obj *otmp, boolean allow_detrimental)
{
    if (otmp is null) {
        find highest weapon skill of the hero (random tiebreaking);
        otmp = random weapon made from chosen skill;
    }

    if (otmp is an artifact or is unique)
        return;

    if (otmp isn't a weapon/weapon-tool/amulet/ring/armor)
        return;

    while (otmp has no properties or random 1 in 250) {
        pick a property j at random;

        /* never randomly create vorpal weapons */
        if (j is vorpal) continue;

        if (allow_detrimental is true and j is fumbling/hunger/aggravation)
            continue;

        /* ensure j is compatible with otmp's existing properties
         * and apply it if so */
        j = restrict_oprops(otmp, j);
        add j to otmp's properties;
    }

    if (bad properties exist on otmp and 90% of the time)
        curse(otmp);

    return otmp;
}
```

[Aside: There's a struct level argument to create_oprop() that precedes the
two above, but it's specific to NitroHack.]

In other words, create_oprop() either adds properties to an existing object, or
creates one of its own and adds properties to that object.

Most places that randomly apply properties bring their own object; only
sacrifice gifts have create_oprop() make an object on its own, and perhaps
failed artifact creation.  All of the possible objects for the chosen skill
exist in the new suitable_obj_for_skill() function; any skill that isn't listed
there instead has create_oprop() generate a random weapon.

allow_detrimental creates two kinds of random property generation on objects:

 1. "one or more properties" - fumbling/hunger/aggravation are allowed
 2. "one or more good properties" - fumbling/hunger/aggravation are not allowed

If one of those bad properties are found, 90% of the time the object will also
be found cursed.

There are 10 cases where properties are randomly applied to objects:

  1. An object will be granted one or more good properties if mk_artifact()
     fails to find a suitable artifact to turn it into.

  2. Angels whose swords are intended to be christened Demonbane or Sunsword in
     m_initweap() but fail in doing so instead have one or more good properties
     applied to them.

  3. In mksobj(), an initialized weapon that isn't already made into an artifact
     has a (level difficulty / 2)% chance of being granted one or more
     properties.

  4. In mksobj(), an initialized weapon-tool has a (level difficulty / 2)%
     chance of being granted one or more properties.

  5. In mksobj(), an initialized amulet that is not the Amulet of Yendor has a 1
     in 250 chance of being granted one or more properties.

  6. In mksobj(), an initialized armor that isn't already made into an artifact
     has a 1 in 100 chance of being granted one or more properties.

  7. In mksobj(), an initialized ring has a 1 in 250 chance of being granted one
     or more properties.

  8. In mk_mplayer(), player-monsters created on the Astral Plane have their 50%
     chance of receiving an artifact made from a role-appropriate weapon
     replaced with a 20% chance, with the remaining 80% instead applying one or
     more good properties to it.

  9. In readobjnam(), if "magical" is given as a prefix to a wish and no other
     properties are given, one or more random properties will be applied to the
     object.  create_oprops() ensures this won't be done to objects other than
     weapons, weapon-tools, armors, amulets or rings.

 10. In dosacrifice(), if the chance to receive an artifact gift fails, the hero
     may instead be gifted with a weapon according to their highest skill(s),
     with one or more good properties applied and identified for them.  The
     chance of this is 1 in (10 + 2 * u.ugifts).  Like artifact gifts, such a
     weapon is guaranteed to be at least +0, not cursed and erodeproof.  This
     also increments u.ugifts, the sacrifice gift counter.

[Aside: GruntHack never randomly applies properties to weapon-tools, amulets or
rings.  The chance of getting a weapon with properties is lower: 1 in (10 + 5 *
u.ugifts) after a 2 in 3 chance to not get a spellbook instead as a wizard, but
the chance of an artifact gift is much different to NetHack: 1 in 25 for the
first artifact gift, and 1 in (100 * artigifts + (2 * artigifts * u.ugifts)).]


### Naming Objects with Properties

e.g. "oilskin fedora of fire resistance and telepathy"

Properties may add prefixes ("magical", "thirsty", "vorpal", "oilskin") and
suffixes (e.g. "of telepathy") to object names, which are used in messages and
displayed in the player's inventory.  This is done at the basic xname() level,
as opposed to the doname() level (which calls xname() and adds attributes like
quantity, blessed/cursed, enchantment, wielded or worn, etc.).

The starting point for all of the logic that adds these new prefixes and
suffixes is xname2() in src/objnam.c.

[Aside: xname2() is the common renaming of xname() to support quantity-less
object naming for the sortloot patch.]

However, the very first thing to note is how objects are marked to be prefixed
with "magical".  This is done by setting the special ITEM_MAGICAL bit in the
oprops_known of an object, the same way bknown is set when priests look at an
object, in examine_object().

examine_object() is a NitroHack-specific function that splits out the object
state mutating front matter of xname() so that it can take a const struct obj *
instead of just struct obj *.  Non-NitroHack NetHack forks can safely put this
wherever bknown is set for priests, usually the top of xname().

The general idea of xname2() is that it builds the basic name of an object as it
appears in the game by looking at its type and player knowledge of it,
concatenating pieces until it forms the whole name.

The DYWYPISI (do you want your possessions' ID-state identified) patch also
affects name-building, substituting for player knowledge by putting square
brackets around the unknown attributes of an object that the player didn't know
formally.  It does this with local variables all called dump_ID_flag in the
functions it affects, which is functionally equivalent to program_state.gameover
being set.

The presence of prefixes means many of the cases that assumed that they would
have first access to the name buffer (buf) no longer have it; this means that
many calls to strcpy(buf, ...) into strcat(buf, ...) or sprintf(eos(buf), "%s",
...).  This conversion is safe since buf is initialized to be an empty
null-terminated string.

The very first prefix that can be applied is "magical", which is added if the
object's oprops_known has the ITEM_MAGICAL bit set, and no other property
prefixes or suffixes will be added.

[Aside: There is support to also add "magical" if the object has no properties,
but its base object type is magical, i.e. oc_magic is set for its entry in the
master objects[] array.  This was used to mark objects as magical for
GruntHack's new scroll of detect magic that wasn't ported for reasons given
earlier.]

The "thirsty" and "vorpal" prefixes may be added to weapons and tools (and
venom, but that doesn't matter).  Like "magical", these are checked and added
directly in xname2().

[Aside: There's no check that a tool is a weapon-tool before adding either
prefix, but it shouldn't matter since the game won't allow those properties to
appear on non-weapon tools, nor do properties stay after being polymorphed.]

The "oilskin" prefix is added for armors, and apparently only armors.  Possible
support for "oilskin bag of holding" would need to extend this to tools as well.

Adding suffixes is trickier than prefixes: they need to appear after the
object's name or appearance, but before the called name given by the player to
that object's base type, e.g. "octagonal amulet of drain resistance called
woohoo ESP!"

UnNetHack also adds special cases for naming Sokoban prizes when they're not
directly seen: "sokoban amulet", "sokoban bag" and "sokoban cloak"; the issue
with those isn't so much the names as much as they change the naming logic
compared to GruntHack and NetHack.

Unlike prefixes, property suffixes for objects are centralized in the
propnames() helper function:

```c
static void propnames(char *buf, long props, long props_known,
                      boolean weapon, boolean has_of)
```

The arguments:

 * `buf` is the name buffer to append the suffixes to.

 * `props` consists of the oprops set for the object.

 * `props_known` consists of the oprops_known set for the object.

 * `weapon` changes how some properties are named, e.g. weapons are "of fire"
   but everything else is "of fire resistance", a drain-life weapon is "thirsty"
   but other things defend against it and are thus "of drain resistance", etc.

 * `has_of` is a flag that controls whether the object's name already has an
   "of" in it so, e.g. a ring of adornment with the speed property can be named
   "ring of adornment and speed and power" instead of "ring of adornment of
   speed and power".

propnames() chains properties with "of" and "and", so for a weapon with fire,
frost and stealth properties it will add "of fire and frost and stealth".

propnames() makes use of a multi-line helper macro, cutting the function down to
about a twentieth of its size in GruntHack, which heavily copy-pastes code for
each of its cases.  Much of the logic for these cases is to support DYWYPISI,
since each case may be surrounded by square brackets, like "of fire [and frost]
and stealth".

propnames() also detects when a certain number of properties is exceeded
(currently 3 or more), and falls back on a compact naming system and short names
to avoid a buffer overflow.  Stacked properties in a normal game are unlikely to
trigger this (they'd have to exceeed BUFSZ minus PREFIX characters), but absurd
numbers of properties are quite possible using wishes in wizard mode, which
would otherwise lead to a crash; another thing that GruntHack doesn't account
for.

Back to xname2(), propnames() is used by it to insert suffixes to objects.  They
need to be carefully added after the object's name, but before whatever the
player has called the base object type.  Recall "octagonal amulet of drain
resistance called woohoo ESP!" for instance: "octagonal amulet" is the name
given by the function, "of drain resistance" is added by propnames(), and
"called woohoo ESP!" is added after that by xname2().

The last tricky part of using propnames() in xname2() is figuring out when to
set the has_of argument.  It needs to be set of the object is already "of"
something, but there are some cases where that doesn't apply neatly:

 * For amulets, has_of can be set simply by checking for " of " in the name thus
   far.

 * For weapons and tools, lenses (as a tool) are prepended with "pair of ", so
   the naive approach would give "pair of lenses and warning", so in that case
   the search for " of " needs to start after the "pair of " to give "pair of
   lenses of warning".

 * For rings, " of " never appears in any ring name or description, so nn (name
   known, I guess) is a good value for the has_of argument.

Armors are the most complicated case, since the naming logic in xname2() has so
many special cases in it:

 * Unseen shields with known properties are suffixed with has_of set to FALSE.

 * The shield of reflection when described as a "smooth shield" is suffixed with
   properties with has_of set to FALSE.

 * If the actual name (i.e. not just the random description) of the armor is
   known, it will be used, so has_of can be set by searching just that part of
   the name.  This allows it to cater to boots and gauntlets, which are prefixed
   with "pair of" in buf but not in the actual name, leading to the desirable
   case of "pair of boots of fire resistance".

 * Armors whose base type has a player-provided called name will never be "of"
   anything, so propnames() can be safely used with has_of set to FALSE.

 * UnNetHack's "sokoban cloak" can safely call propnames() with has_of set to
   FALSE.

 * Armors named by a possibly random description have the worst time setting
   has_of: the "pair of" for boots and gauntlets needs to be skipped, as well as
   "set of" for dragon scales, only after those can the search for " of " be
   done properly.  Otherwise, appending suffixes goes through propnames(), just
   like all of the other cases.

Except for the conversion of strcpy(buf, ...) calls to strcat(buf, ...) and
sprintf(eos(buf), "%s", ...), this covers all of the major changes to xname2().

One last note is for stacks of multiple objects, xname2() calls pluralize() on
the name, which interacts badly with DYWYPISI, leading to things like "dart [of
frost]'s" instead of "darts [of frost]".  pluralize() needs to be detect the
where " [of " appears and handle it the same as it does " of ", which fixes this
issue.

[Aside: GruntHack adds a lot more cases where DYWYPISI might mess up pluralize
in the same way, but given that things like "foo called/labelled/etc." are never
unknown knowledge revealed by DYWYPISI, they make no practical difference, so I
left them out.]

A number of fixes need to be made to fully support the DYWYPISI patch as
implemented in UnNetHack.  DYWYPISI appends the actual name of an object to the
name as it appeared to the player if they didn't already know it, e.g. "old
gloves [gauntlets of dexterity]".  This is all done in doname_base().  However,
this interacts really badly with properties, giving something like "old gloves
[of stealth gauntlets of dexterity]".  Putting the property suffixes after the
actual object type name would involve replicating all of the intricate
special-casing in xname2() and putting it in doname(), so instead I put the true
name in parentheses, leading to "old gloves [of stealth (gauntlets of
dexterity)]".  In the case of "old gloves [(gauntlets of dexterity)]", there is
logic to collapse nested braces for wand charges which handles this, turning it
into just "old gloves [gauntlets of dexterity]", though it requires a small
modification to prevent turning "[(foo of bar) (n:n)]" into "[foo of bar)
(n:n]".

[Aside: GruntHack doesn't seem to do anything to prevent this DYWYPISI naming
ugliness.  It also seems to use a version of DYWYPISI that's closer to the patch
than the implementation used by UnNetHack.]

[Aside: doname_base() is the renamed version of doname() to support appending
the price of shop objects to their names, something imported into NitroHack from
UnNetHack a long time ago by Daniel Thaler, the author of NitroHack.]


### Parsing Wishes for Properties on Objects

Before getting into parsing wishes, a note about wishes themselves in this
development branch: under UnNetHack, wishes are divided into magical and
non-magical, the former coming from wands of wishing and the latter from every
other source of wishes.  Objects with properties on them count as magical under
these rules, as checked in makewish() in src/zap.c.

The main wish parsing logic lives in readobjnam() in src/objnam.c, a behemoth
1200 line function made almost entirely of string-reading special cases and
exceptions.  The good news is that the function itself is mostly self-contained,
which means that all the changes needed to support the new magical property
prefixes and suffixes can be made in that one function.

First we need a flag to track if the player asked for "magical" as a prefix,
which is much the same way things like "blessed" and "fixed" are already
handled:

```c
int magical;
/* ... */
magical = 0;
```

It says `int`, but it's treated like a boolean, the same as the `blessed` and
`erodeproof` flags, so the type is purely for consistency with the declarations
around it.

The properties that the player asks for also needs to be tracked in a bit field
not unlike oprops in struct obj:

```c
long objprops = 0L;
```

The "magical", "vorpal", "thirsty" (for life-draining weapons) and "oilskin"
prefixes are all handled in the prefix-handling section, again like "blessed"
and "fixed".

The "vorpal" prefix check needs to stop itself from eating the "vorpal" from
"Vorpal Blade"; the checks are case-insensitive, but more importantly, it
advances `bp`, which I presume stands for "base pointer" for the wish input
string.  As parts of the prefix are read, `bp` is advanced by that many letters
plus the space, effectively eating the input up until the next word or portion
of the wish input.

The "oilskin" prefix also needs to be careful for the same reason as "vorpal":
it needs to dodge "oilskin sack", otherwise it will try to grant a sack with the
oilskin property instead of the base object type "oilskin sack".

The "oilskin" prefix is also the first of multiple cases where object names and
properties conflict, e.g. what should the game try to give a player who asks for
an "oilskin cloak of displacement", where "oilskin" is a valid prefix,
"displacement" is a valid suffix, and "oilskin cloak" and "cloak of
displacement" are valid base object names?  First, readobjnam() needs to prevent
"oilskin" being eaten for a wish for an "oilskin cloak".  However, the "oilskin"
needs to be eaten in case what follows the "cloak" part is a valid name for any
of the four cloaks of foo:

 1. cloak of protection
 2. cloak of invisibility
 3. cloak of displacement
 4. cloak of magic resistance

This goes on to interpret "oilskin cloak of displacement" as a cloak of
displacement with the oilskin property; if it were done the other way around (an
oilskin cloak with the displacement property), the suffix reader later on would
need to deal with "of protection/invisibility/magic resistance", none of which
are valid suffixes, and were highly unlikely to be what the player meant when
they wished for their cloak.

With prefixes out of the way, suffixes need to be captured.  The way this is
done is with the portion of readobjnam() that deals with monster names for
corpses, tins, figurines and statues, i.e. the same logic that can detect that a
wish for a "tin of nurse meat" needs to set the object's monster ID to nurse can
be used to pick up the fire in "dagger of fire".

First, suffixes need to avoid eating the "fire" in "scroll(s) of fire", so
wishes at this point that are of "scroll" or "scrolls" at this point skip all of
this suffix logic.

The eight lines of logic that actually read monster names from "object of
monster" expands to about 50 lines to support the new magical prefixes.  This
goes through the wish string, looking for the next "of" or "and" and then
processes whatever monster name or prefix it stands for.

The very first "of" found for wishes for "amulet/cloak/shield/gauntlets/helm/
ring of foo" likely means the "of foo" is part of the actual object name and not
a magical suffix, so in those cases the very first suffix is ignored so later
wishing code can use it.

If the suffix itself turns out not to be a monster name, the each suffix is
checked in turn, setting the relevant bit in `objprops`, cutting the prefix
from the base object name in the wish by setting a null terminator at that
point, and advancing to the next "of" or "and".  The following suffixes are
recognized in this section:

 * fire ("resistance" optional)
 * frost/cold ("resistance" optional)
 * drain ("resistance" optional)
 * reflection
 * speed
 * power
 * dexterity
 * brilliance
 * telepathy (or "ESP")
 * displacement
 * warning
 * searching
 * stealth
 * fumbling
 * clairvoyance
 * detonation (or "detonations"; GruntHack may have once called it this)
 * hunger
 * aggravation

[Aside: The bits between each "of" or "and" aren't checked at all, meaning they
can technically contain any string that doesn't have "of" or "and" surrounded by
spaces.  Have fun.]

If the property name was recognized, the prefix is cut off from the base wish
name at that point by setting a null terminator at the beginning of where it was
found, advancing the "of"/"and" search past that property and repeating until
nothing more is found.

With prefixes and suffixes processed, we can finally do something with them.
The very first thing to do is to ensure that the fresh object created by mkobj()
or mksobj() doesn't have any random properties.  This prevents makewish() in
UnNetHack randomly though rarely refusing a non-magical wish because its
creation included random properties.

Finally, we come to the part of readobjnam() that tries to give the attributes
of the object that they asked for.  First, if "magical" was requested and no
other properties were asked for, one or more properties will be applied to the
object as if they had been randomly applied, using the create_oprops() rules.

If magical properties were requested, properties may be applied to the object,
with a number of restrictions:

 1. Artifacts can never receive properties.

 2. Only weapons, weapon-tools, amulets, rings and armors may receive
    properties.

 3. Objects whose base type already has a special ability of some sort cannot
    receive properties from a wish.  This is mostly based on if it has an
    oc_oprop set in its struct objclass entry in the objects[] array (this is
    similar but not identical to the things mentioned in Restrictions of
    Properties on Objects), but includes some objects that don't set an
    oc_oprop:

    * gauntlets of power
    * gauntlets of dexterity
    * helm of brilliance
    * ring of gain strength/constitution/intelligence/wisdom/dexterity
    * ring of increase accuracy/damage

The last restriction does not apply in wizard mode.

[Aside: The last restriction is original and does not feature in GruntHack.  It
currently doesn't apply to objects with oc_magic set in their struct objclass;
that feels like it might be too restrictive. Only play-testing can tell for sure
if it would be needed or not.]

If properties are to be applied to an object at all, the usual restrictions for
properties on objects are applied by a call to restrict_oprops().

There is a final restriction for objects on properties: if not playing in wizard
mode, only one property of those valid for that object can be applied to it.  If
more than one was asked for by the player, one of them is chosen at random and
then applied.

Slight diversion: to randomly choose a property, a new utility function
longbits() counts the number of bits set in objprops.  The way it works is by
performing what is called [sideways
addition](http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel),
which filters alternating sets of bits and adds them instead of iterating
through each bit, like so:

```
  01010101 01010101 01010101 01010101
+ 10101010 10101010 10101010 10101010 shifted right 1 bit
-------------------------------------
       sum of each bit in each 2 bits

  00110011 00110011 00110011 00110011
+ 11001100 11001100 11001100 11001100 shifted right 2 bits
-------------------------------------
    sum of each 2 bits in each 4 bits

  00001111 00001111 00001111 00001111
+ 11110000 11110000 11110000 11110000 shifted right 4 bits
-------------------------------------
    sum of each 4 bits in each 8 bits

  00000000 11111111 00000000 11111111
+ 11111111 00000000 11111111 00000000 shifted right 8 bits
-------------------------------------
   sum of each 8 bits in each 16 bits

  00000000 00000000 11111111 11111111
+ 11111111 11111111 00000000 00000000 shifted right 16 bits
-------------------------------------
   sum of all of the bits in the long
```

This is all that's needed to add support wishing for magical property prefixes
and suffixes in readobjnam().


### Property Effects for Player-worn Armors, Amulets and Rings

Before getting into how property effects work for armors, amulets and rings worn
by the player, it's worth knowing a bit about how NetHack handles players
wearing equipment:

 1. When the player wants to wear armor, the game calls dowear(); rings and
    amulets instead call doputon().  Both of these live in src/dowear.c.

 2. The game maintains global variables like uarm (body armor), uarmh (helmet),
    uarmg (gauntlets) and so on to represent player equipment.  dowear() and
    doputon() call setworn(), which sets one of these variables, as well as
    unsetting any prior extrinsic the old object had (oc_oprop of struct
    objclass) and setting the extrinsic of the new equipment.

 3. Equipment may have a non-zero delay, meaning that it takes several player
    actions to finish wearing it.  If so, schedule the relevant Foo_on()
    function to run after that delay, e.g. Armor_on() for body armor,
    Helmet_on() for helmets, Gloves_on() for gauntlets, etc.  Equipment with
    zero delay run their Foo_on() immediately instead.

 4. If the equipment needed time to be worn, the main loop passes time until the
    delay is done.

 5. Once the main loop finds that the delay is done, the Foo_on() function can
    then run.  This is usually used to print a suitable message after wearing
    equipment with obvious effects, but it also does things like automatically
    identify certain equipment when worn and covers special cases that the
    oc_oprop doesn't or can't handle, e.g. attribute bonuses.

The odd thing about this method is that magical armor that has some sort of
property confers its extrinsic ability (as set in oc_oprop) when the player
begins to wear it and not after they've finished, e.g. silver (reflecting)
dragon scale mail will reflect beams while it's being put on.

Taking off armors, amulets and rings is similar, but not identical:

 1. When the player wants to take off armor, the game calls dotakeoff(), though
    it delegates most of its work to armoroff(); rings, amulets and tools like
    lenses and blindfolds instead go through doremring().

 2. Equipment without a delay have Foo_off() called on them immediately.
    Equipment with a delay have Foo_off() scheduled to be called after the delay
    is finished.

 3. Time passes in the main loop until the delay is done.

 4. Once the delay is done, the scheduled Foo_off() is then run.  This is
    natural analog to Foo_on(), with one important exception: the corresponding
    uarm* global is set *for* Foo_on() by setworn(), but unsetting it when
    taking off equipment is done by Foo_off().  Otherwise, Foo_off() simply
    undoes whatever Foo_on() did.

For equipment with delays, it is possible for the object being worn or put on to
be stolen, disintegrated or destroyed.  In these cases, whatever scheduled
function is cancelled, and if an equipment with a delay was being put on, a
special `cancelled_don` flag is set.  Foo_off() is called right away, taking
care to check `cancelled_don` to make sure it doesn't undo anything that
Foo_on() was going to do but didn't due to being cancelled; this last detail is
especially important for attribute bonuses on equipment since, unlike most
extrinsics and abilities, setting and unsetting them are not idempotent
operations.

With the above in mind, the effects of armors, amulets and rings when worn by
the player can be divided into several broad categories:

 * extrinsics - set when worn, unset when taken off (confusingly, the game
   refers to equipment-acquired resistances and abilities as "intrinsics",
   reserving "extrinsic" for external influences like fumbling from ice; I'll
   use "extrinsic" to refer to abilities from equipment from now on, as it's the
   most common use of the term in the NetHack community.)

 * reflection - set and unset like most extrinsics, but needs some special
   support to work fully as expected

 * oilskin - only checked for in events that need to know it

 * attribute bonuses - dexterity and brillance, which if handled improperly can
   easily lead to accidental permanent attribute changes

The two major functions that handle applying and unapplying extrinsics provided
by the new magical object properties are Oprops_on() and Oprops_off(), both of
which can be found in src/do_wear.c.

The code within Oprops_on() and Oprops_off() is mostly self-explanatory: set any
extrinsics for properties that apply, identify those properties with obvious
effects, and show a suitable message for each.  Oprops_on() is called by the all
of the Foo_on() functions, and Oprops_off() is called by all of the Foo_off()
functions and also Armor_gone().

For most properties, this pair of functions is enough.  For reflection, the
ureflects() function in src/muse.c needs to be extended to handle the new
possible sources of reflection that the player could have wielded or worn, in
order from outermost to innermost equipped.

The oilskin property extends the cases where oilskin cloaks and oilskin sacks
are checked for to include objects the new property: in u_slip_free() to stop
monster grabbing attacks and in water_damage() to protect the contents of
oilskin containers (which currently aren't creatable or properly named, as
mentioned before) and body armor under cloaks with the oilskin property.

The power property grants 25 strength by extending the special case in acurr()
to return 25 for strength for gauntlets of power to other armors, amulets and
rings.

This leaves dexterity and brilliance, which provide attribute bonuses based on
the enchantment of the equipment they are on.  The way attribute bonuses from
equipment works in NetHack is by adding the bonuses when they're put on, and
taking those bonuses away when they're taken off.  This means that we have to be
really careful about when to add to attribute bonuses and when to take away, to
prevent game-affecting issues like the HoBug, where the enchantment and
attribute bonus provided by a piece of equipment fall out of sync.

Support for dexterity and brilliance starts in the adj_abon() function at the
bottom of src/do_wear.c; all that is needed there is to extend the cases for
gauntlets of dexterity and helms of brilliance.

Two other cases where dexterity and brilliance matter (which GruntHack doesn't
account for) are cancelling (in cancel_item()) and draining/disenchantment (in
drain_item()) of objects.  In both cases, the new equip_is_worn() function in
src/do_wear.c detects if a piece of equipment is really worn: recall the issue
where the uarm* globals are set before their Foo_on() has finished.  If the
equipment is indeed worn, only then is it safe to deduct attribute bonuses,
assuming the equipment was positively enchanted to begin with.

[Aside: I tried to extend cancellation to actually strip properties from
objects, which required a fair amount of code to ensure that the player was
indeed wearing their equipment before removing the extrinsics that they granted.
I later encountered the real problem, which was that the same thing needed to be
done for monsters, and the code for handling monster extrinsics on equipment
combined wearing, object oc_oprop and the new object properties required heavy
refactoring to do properly, so I ultimately scrapped it, since GruntHack didn't
do it either.]

You may notice that this dead-reckoning approach to attribute bonuses seems
error-prone, as noted by Daniel Thaler when he originally made NitroHack.  He
added calc_attr_bonus() to src/attrib.c: a function that wipes the attribute
bonuses clean to zero, and recalculates them based on the equipment worn by the
player.  Accounting for the new dexterity and brilliance properties is
straightforward.  If this code is being ported to a NetHack fork that is not
based on NitroHack, I highly recommend porting this function too to prevent
HoBug-like issues and attribute bonus miscalculations in the future.  It is
called in two places in the main movement loop: at the top of the monster
movement loop in you_moved() in src/allmain.c, and at the top of
pre_move_tasks() in the same file.

With any luck, this is all that needs to be known about handling magical
properties on armors, amulets and rings worn by the player.


### Extrinsics for Player-wielded Weapons

Extrinsics that are granted to the player when they wield a weapon with magical
properties works quite differently to those on armors, amulets and rings.  The
idea is that they are set and unset in set_artifact_intrinsic() in
src/artifact.c, and then several places where that function is called are
modified to accept objects with properties as well as artifacts.

The following properties apply extrinsics to the player when wielded:

 * reflection
 * telepathy
 * searching
 * warning
 * stealth
 * fumbling
 * hunger
 * aggravation

Though the new object properties piggy-back on set_artifact_intrinsic(), care is
taken to preserve the existing behavior of extrinsics applied by artifacts,
which is one of the technical reasons object properties are mutually exclusive
with artifacts.

The first thing to note in set_artifact_intrinsic() is the presence of several
flags:

```c
boolean spfx_stlth = FALSE;
boolean spfx_fumbling = FALSE;
boolean spfx_hunger = FALSE;
boolean spfx_aggravate = FALSE;
```

The special effects of artifacts are carried down the function to where they are
checked, one by one.  There are no SPFX_* constants for fumbling, hunger or
aggravation in NetHack, hence the presence of `spfx_fumbling`, `spfx_hunger` and
`spfx_aggravate`.

The enhancement that UnNetHack makes to the Heart of Ahriman (i.e. replacing its
stealth with displacement and fast energy regeneration) means that it no longer
has SPFX_STLTH; `spfx_stlth` is added in its place here, and the old code for
stealth support is reinstated lower in the function for this flag.

Support for wielded reflecting weapons is extended to off-hand weapons during
two-handed combat.  This has no effect for artifact weapons, since they can't be
wielded in the off-hand anyway, but this enables reflection for off-hand weapons
with the reflection property.  From there, complete support for reflection there
means adding a case for off-hand weapons in ureflects().

The effects for warning in NetHack only properly refresh the map when the player
starts or stops being warned against specific monster types and not for warning
in general, which is easy to fix but ensures the warning property works
correctly on weapons.

Finally, there are six call sites for set_artifact_intrinsic(), and we need to
change the three in setworn_core() and setnotworn() to accept objects with
properties along with the artifacts it normally passes through.

[Aside: setworn_core() is just a renaming of setworn() to fix a minor bug in
UnNetHack's worn armor tracking count incrementing every time a game was
restored.]


### Property Effects for Monster-worn Weapons, Armors, Amulets and Rings

There are two main parts to implementing the new magical object properties for
monsters: updating monster extrinsics, and adjusting their AI to prefer
equipment with properties over mundane equipment.

The majority of monster extrinsics are handled by update_mon_intrinsics() in
src/worn.c.  Object properties are translated into oc_oprop-like values and are
processed one-by-one with a goto loop not unlike a do-while loop.

[Aside: The decision to use goto is from GruntHack; refactoring it into a real
loop, perhaps also lifting the property setting and unsetting into a helper
function, would be an improvement.]

Monsters maintain resistances and properties differently from the hero.  The
hero has a bit set for each property and each bit stands for the source it comes
from, e.g. `EFire_resistance`, but monster resistances and properties all live
in a single bit set call `mintrinsics` and each bit stands for one of those
resistances or properties.  This means that unsetting a bit in `mintrinsics`
needs to ensure that nothing else the monster is wearing sets that bit, which
can be seen in action where the various resistances are toggled off near the
middle-bottom of update_mon_intrinsics().  To properly check if some piece of
worn equipment has a given property, the new obj_has_prop() helper function
checks its oc_oprop and any relevant properties.

One strange thing is that many object properties are translated into
oc_oprop-like values, but only a handful of them are actually implemented by
NetHack.  Therefore, the only properties granted to monsters from worn equipment
are:

 * fire resistance
 * cold resistance
 * reflection
 * speed

Fire and cold resistance line up with bits MR_FIRE and MR_COLD in `mintrinsics`.
Drain resistance on armors and amulets (monsters never wear rings) does nothing,
since `mintrinsics` doesn't have a bit for it.

[Aside: There are 8 MR_* constants, as defined in include/monflags.h: MR_FIRE,
MR_COLD, MR_SLEEP, MR_DISINT, MR_ELEC, MR_POISON, MR_ACID and MR_STONE.
However, `mintrinsics` actually has room for 16 bits, so what are the other 8
bits used for?  Apparently nothing; there are additional MR2_* constants in
include/monflags.h that occupy those bits: MR2_SEE_INVIS, MR2_LEVITATE,
MR2_WATERWALK, MR2_MAGBREATH, MR2_DISPLACED, MR2_STRENGTH, MR2_FUMBLING.  None
of those defines are found anywhere else in the source code.  Interestingly,
GruntHack implements all of these effects for monsters; I've not included them
in this feature port as they fall just outside the scope of object properties.]

Speed for monsters is handled by the mon_adjust_speed() helper function, which
itself needs minor adjustment to automatically identify the speed property when
the hero sees a monster's speed change due to something they were wearing or
taking off.

Reflection for monsters is handled not as a set resistance bit but rather
on-the-fly when the monster is struck by something reflectable, checked by
mon_reflects() in src/muse.c.  This monster-based counterpart to ureflects()
seems to have been overlooked by GruntHack; I had to add the new cases for
reflecting monster-used equipment since GruntHack made no modifications there.

Support for monsters to recognize and use equipment with properties is fairly
rudimentary, since GruntHack makes huge changes to monster AI, most of which has
nothing to do with the new object properties, and also implements property
effects for monsters that aren't implemented at all in NetHack.

[Aside: Steve "Grunt" Melenchuk, the creator of GruntHack, is as far as I know
also the author of the Intelligent Pets patch that extends and improves pet AI,
allowing them to, pick up and use good equipment right away and use their ranged
attacks, amongst other things.  He was arguably the most knowledgeable person in
the world about NetHack's monster AI until he left NetHack development behind to
work on other things.]

Nonetheless, monsters at least recognize that amulets with the reflection
property are just as good as a true amulet of reflection; the modification for
this is made in m_dowear_type().

Monster preference for equipment is also influenced by changes to extra_pref(),
helping monsters recognize that the speed and reflection properties are
desirable on any equipment, as well as other properties to a lesser extent.

This leaves the matter of monster preferences for properties on objects when
choosing weapons.  I do the same thing GruntHack does here: nothing.  Ideally,
good attack properties would count like positive enchantment, but of the code
I've seen in this area, there is a hard-coded list of weapons preferred by
monsters in order of preference with hard-coded logic, and I don't have enough
confidence to safely modify it to acknowledge the new object properties.


### Attack Effects for Properties on Weapons

Some properties have an effect when on weapons that are hit with, thrown or
fired.  These properties are:

 * fire
 * frost
 * draining
 * vorpal
 * detonation

Except for detonation, these properties are handled by extending artifact_hit()
and spec_dbon(), both of which can be found in src/artifact.c.

The major themes of the changes made to support these are:

 * If more than one attack property applies when hitting, the effects stack, so
   artifact_hit() and spec_dbon() need to be overhauled to inflict their effects
   accordingly.

 * The properties of ammunition and their launcher combine when firing them, so
   the throwing and firing code now needs to carry through the launcher to the
   hit code, not just the fired object.

 * If artifact_hit() gives an identifying message for a property, that property
   needs to be identified on the object it is on, including if it's on the
   launcher for a fired object.

All of this easily makes up about half of the count of all changed lines of
source code, and virtually all of the refactorings where function arguments were
changed.

A quick guide to dealing with all of the refactoring:

 * If you see `lev` and `level` passed around as arguments, they can be safely
   ignored for the most part: they're specific to NitroHack and a bunch of them
   are even specific to this development branch/fork of it.  If you're curious,
   the extra argument in all of these functions was to prevent crashes in
   NitroHack due to dereferencing a level that didn't exist at that point during
   runtime.

 * `magr` is the common name for the argument used to pass the aggressor monster
   in an attack, so their wielded launcher can be found to extract, use and
   possibly identify properties on them in an attack.

 * `olaunch` is the launcher object, in a more direct form, again used for its
   properties during ranged firing attacks.

 * `thrown` is a flag that signals to artifact_hit() that the object was thrown
   or fired as opposed to being hit with in melee; a distinction that now
   matters, since missiles, launchers and ammo only apply their properties in
   those kinds of attacks.

 * `ostack` is the stack from which an object was split from, so properties
   identified on a single thrown or fired object can be automatically identified
   on the stack too.

 * artifact_hit() now checks internally if it needs to do anything due to the
   attacking object being an artifact or having properties, so its callers no
   longer have to redundantly do it themselves.

When an artifact is used in an attack and it hits, it first calculates all of
the usual damage for the base object type, and then it calls artifact_hit() with
the artifact and the damage to determine if extra damage should be done, special
effects like boiling and exploding potions, draining life or beheading, and
printing a special artifact hit message if so.

Even though artifact_hit() is responsible for most of what happens when an
artifact is hit with, the true decision on whether an artifact has any effect at
all and whether it does extra damage and/or causes other special effects is made
by spec_dbon() called by artifact_hit(), and spec_applies() which is called by
spec_dbon().

Before going into overhauling these functions to support the new object
properties, I should note that except for finding all of the places to change,
the changes themselves in GruntHack were of almost no help, since they were so
buggy, e.g. hit messages would stack but not the damage, the hit messages in
when multiple properties applied incorrectly said that fire-resistant monsters
would burn, dropping one out of a stack of darts when levitating and seeing the
dart explode failed to identify the detonation property on the stack, and so on.
As such, nearly all of the logic is original work, and cross-comparing it to
code in GruntHack isn't very useful.

Anyway, the fire, frost, drain life and vorpal properties all go through
artifact_hit() and spec_dbon().  The big overhaul for these functions was to
support multiple artifact-like effects occuring in a single round; I was careful
as was possible to preserve the original effects of artifacts themselves,
despite the changes to the fundamental logic of this part of the system.

A very ingrained assumption lies in the interaction of artifact_hit() and
spec_dbon(), and it comes in the form of a file-wide static variable named
`spec_dbon_applies`: if spec_dbon() affects a target monster in any way, it sets
that flag, and artifact_hit() works backwards to figure out what property exists
on the object and assumes that the first one it finds is the one that took
effect.  In other words, if a monster were to be hit with a weapon of fire and
cold, but was only affected by the cold, spec_dbon() sets `spec_dbon_applies`,
and artifact_hit() sees it and sees the fire on the weapon, it will think the
monster was affected by the fire and not the cold.  The ideal way to handle this
is to not have spec_dbon() at all and inflict all special weapon effects in
artifact_hit(), but that would have been a huge change that would be difficult
to verify the correctness of.  Instead, `spec_dbon_applies` gets some new
friends:

```c
static boolean spec_applies_ad_fire,
               spec_applies_ad_cold,
               spec_applies_ad_elec,
               spec_applies_ad_magm,
               spec_applies_ad_stun,
               spec_applies_ad_drli,
               spec_applies_spfx_behead;
```

Only the first and last two are really important to support the new object
properties, but having all of these allows spec_dbon() to give the full picture
to artifact_hit() so that it can do its job properly.  Properties on objects set
these directly in spec_dbon(), whereas artifacts will tend to have them set by
spec_applies() instead.

[Aside: Getting rid of spec_dbon() is tricky because (1) it's the analog
function to spec_abon(), which doesn't need to go, and (2) it's used by the
dmgval() function to try to stop bonus damage from e.g. silver from being
doubled.  dmgval() doesn't even detect it properly for weapons that only double
damage conditionally, such as The Tsurugi of Muramasa cutting deeply into (read:
doing double damage to) large monsters.  It may deserve a function just to
detect if a weapon does double damage to free spec_dbon() of its burden, or even
better inflict bonus damage properly in a separate function, but that would be
almost impossible to do across every single instance of inflicted damage in
NetHack reliably, let along at all.]

The first thing that the revamped artifact_hit() does is, if the object was
thrown, check the wielded weapon of `magr` (the aggressor monster) to see if it
was the launcher for that object.  This then feeds into the next change to
artifact_hit(): centralizing the rules for whether it runs at all.  Obviously
this still includes artifacts, but the picture gets murkier with properties,
since the object itself may not have properties, but its launcher can, and those
properties still need to take effect.  The new helper function that determines
this is oprops_attack(), which not only decides if attack properties exist on an
object, but also if they will trigger at all under the circumstances:

 * weapons and weapon-tools in melee, but not missiles, launchers or ammo
 * weapons and weapon-tools when thrown, except for launchers
 * ammo and launcher when fired, assuming the ammo is for the launcher

Once artifact_hit() calls spec_dbon() it can find out what should actually occur
via the new spec_applies_* variables.  When a unique message is given or effect
occurs, the property it is associated with is supposed to be identified on the
object.  The rules to do this with properties get tricky, but they're all
consolidated in the new oprop_id() helper function, which correctly handles
objects, the stack they may have come from, firing and launchers.

The main return value of artifact_hit() is whether it printed its own message,
to prevent the calling code to redundantly print its own normal hit message.  In
using the new spec_applies_* variables, it cascades through rather than
returning at the end of each case.

Fire, cold and shock may destroy inventory items.  Since these events can now
identify properties on an object even if the monster itself isn't affected,
destroy_mitem() and destroy_item() are retooled to, instead of returning how
many inventory items were destroyed (which may not have been seen), it returns
how many items were seen being destroyed (i.e. a message was given), and the
actual number of items destroyed is instead stored in a caller-supplied int
pointer.

Before continuing, note that NitroHack splits out the effects for Vorpal
Blade/The Tsurugi of Muramasa (beheading and bisecting) and Stormbringer (life
draining) into helper functions called artifact_hit_drainlife() and
artifact_hit_behead() respectively.  I would recommend this change even to
NetHack forks that aren't based on NitroHack, because it makes them much easier
to deal with; in fact I swap the order in which they're called to maximize the
effects of multiple properties from a single hit.

With that in mind, there's one effect that doesn't trivially transform like
this: the beheading, or rather the missing of Vorpal Blade in
artifact_hit_behead() when it tries to behead headless monsters.  Rather than
artifact_hit() accumulating special damage into a number and sending it back, it
takes a damage pointer, which allows it to, amongst other things, to completely
negate all damage up to that point, counting it as though the hit had actually
missed.  With attack effects stacking, this could lead to successful-looking hit
messages for fire and frost, then suddenly seeing "Somehow, you miss foo
wildly" with the damage completely nullified.

That just about covers all of the important changes to artifact_hit(), which
handles fire, frost, draining and vorpal.  What about detonation?  Detonation is
handled rather separately of all of this, instead getting its own detonate_obj()
function in src/artifact.c.  This creates a 4d4 fiery explosion and scatters
objects.

The fiery explosion requires a constant (ZT_FIRE) and a macro (ZT_SPELL) that
until now have lived exclusively in src/zap.c.  This required moving the
zap-type macros and constants into a more global location; I chose
include/spell.c arbitrarily, even though that header is mainly for player
spell-casting and not really zap/explosion effects.  Another alternative would
be to not move those macros at all and simply put detonate_obj() in src/zap.c,
though this would split the handling of this property far from the rest of them,
which would have been a bit odd.

detonate_obj() usually destroys the object that it affects.  This only happens
if the object is not needed to win the game (nothing will happen) or if the
object is an artifact.  The latter is essentially a special case to allow The
Heart of Ahriman to be used as infinite ammo for a sling of detonation.

Finally, the places where detonate_obj() is called for an object:

 1. In hitfloor() when it hits the floor when, e.g. dropped while levitating.
 2. In throwit() when the hero throws or fires it and it lands on the floor.
 3. In thitmonst() when a thrown or fired object hits a monster.
 4. Twice in drop_throw() when a monster-thrown/fired object lands.

[Aside: GruntHack not only repeats the code I condensed into detonate_obj() in
these places, it also never even checks if the object should not be destroyed!]

As a bonus for UnNetHack, a small change in noisy_hit() in src/uhitm.c allows
weapons with the stealth property to not create noise when hitting, allowing
heros equipped with such a weapon to, say, deal with a zoo without waking its
inhabitants, just like in NetHack.

The attack effects for properties is easily the most complicated part of the
port, but it's also arguably the most interesting for players, so it's worth the
effort.  Of all of the parts of magical object properties that I ported from
GruntHack, this was easily the buggiest in its original form; this port manages
to fix bugs and implement most of the things that GruntHack missed.  It's not
perfect, though, which is a good segue into the final section.


### Known Bugs and Potential Enhancements

One quirk about artifact_hit() is that for whatever reason it can't cope with
the aggressor and defender being the hero; it raises a warning if it's called
like that.  Since the aggressor is needed to get the launcher with possible
properties, objects that are fired upwards and land on the hero again don't have
launcher properties apply at all, so arrows fired from a bow of frost won't have
their frost effect, even though they should.  The cases where artifact_hit()
actually does lead to self-hits are:

 1. tossing or firing an object upwards
 2. being hit by a returning boomerang
 3. being hit by returning Mjollnir

It shouldn't be too hard to rig artifact_hit() to get the right aggressor for
these self hits (the hero) and fudge the aggressor back to null for the rest of
the special effect logic, but I haven't done it here since this change is
already massive and complicated enough without it.

Another bug is that arrow and dart traps have a low but finite chance to fire an
arrow or dart with properties, doing extra damage in the process; in the extreme
it could be detonation, doing 4d4 fire damage plus subjecting the inventory to
fire!  This is another piece of low-hanging fruit which I'd deal with right now
if I wasn't already exhausted from the porting effort.

A NitroHack specific quirk is that the floor object sidebar doesn't update when
an object that is thrown by an explosion lands by the hero's feet.  Since that
sidebar is called in the same conditions as the (annoying) object pile prompt
that NetHack gives when moving, it's hard to imagine a solution for NitroHack
that would generalize well for NetHack forks that aren't based on it.

Object properties are supposed to make both players and monsters more powerful,
but right now they mostly just benefit the player, since monsters don't
recognize the power of a weapon with attack properties.  The monster AI for
choosing a weapon is quite fixed on the base object types of weapons; I'm not
even sure it considers enchantment, let alone a way to consider attack
properties.

NitroHack includes the sortloot patch which makes browsing large piles of
objects much easier.  It should probably give priority to objects that have
properties, especially given their current rarity; right now the only
distinction they get from mundane objects is their different name.

If a wish for an "oilskin cloak of displacement" fails due to the cloak being
leather and thus failing to apply the oilskin property, perhaps the wish parser
could reinterpret the wish as one for an oilskin cloak with the displacement
property instead.  This would likely require yet another nasty back-jump goto in
what is already goto-infested wish parsing code, so it's hard to justify.

The selection of properties to be randomly applied to an object is very
rudimentary: it picks a random property, and if it can apply, it will be, but if
it doesn't, it drops the property entirely and the object missed out on its
chance to get a property.  A better way to assign random properties would be to
consider a random property that is eligible for that object type, perhaps
further filtering based on what properties the object already has.  This would
change the balance of properties on objects quite a bit.

I originally played around with the idea of cancellation cancelling properties.
I ultimately failed, since doing so means removing whatever special effects that
were associated with those properties, which meant knowing if the player was
wearing or wielding the object, or if a monster was, and if the latter, which
monster, and cancel_item() receives none of that information.  Making this work
without accidentally giving the player or monsters accidental permanent
extrinsics or attribute bonuses is more effort than it's worth, in my opinion.

To wrap it up, a nice little idea to make martial arts slightly more appealing
than it is at the moment: gloves and boots with properties could inflict their
defenses as attack properties when punching or kicking.  I have no idea how such
armor would make it into artifact_hit() or how it would allow it with the weapon
rules, but it's not impossible.


### Final Thoughts

Objects with random magical properties is a feature that roguelikes other than
NetHack have had for decades, so seeing it in a NetHack fork is really
gratifying.  Handled correctly, this has the potential to affect gameplay in
NetHack forks in a similar way that the Wizard patch did, way back when NetHack
was actively developed.

Or it could just as easily fall on the floor and be ignored; I'm only one
person, and all of my play-testing will never be enough to say whether
properties occur too often or too rarely, whether the properties themselves are
worthwhile, whether they reward exploration or just wishing, whether they make
the game easier or harder, and ultimately whether they make the game more
interesting and fun to play.  I hope this isn't just ignored, since it's very
rare for a feature to appear in a NetHack fork that truly affects gameplay in a
fundamental way, as opposed to the many cosmetic changes, incremental spot
improvements and unimportant fridge-logic additions that are much more common.

-tung
