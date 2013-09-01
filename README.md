DynaHack
========

DynaHack is an experimental variant of NetHack.  It mainly merges the new
content and challenges of UnNetHack (up to version 4.1.1) onto NitroHack as a
base, with its new resizable ASCII interface.  It also includes the save system,
many bug fixes and selected interface changes from NetHack4, magical equipment
from GruntHack, and some original changes to add variety and fairness to the
gameplay.


Interface changes
-----------------

DynaHack adopts NitroHack's interface, allowing it to make better use of larger
terminal sizes, showing a multi-line message box and inventory sidebar.  Windows
players will want to change the win_width and win_height options (resizing works
automatically on Linux).  The msgheight option also changes the maximum height
of the message box.

Options in DynaHack are set in-game, unlike NetHack which uses a config file.
There are no means to import config files; instead, play some test games and
change options to taste.

Other interface changes include:

 * Item action menus.
 * New and improved quick command reference (type '??').
 * Colored status area and HP/Pw bars.
 * Colors for walls and floors of branches and special rooms.
 * msgtype option to force "--More--" prompts and hide messages.
 * Menus for choosing floor items when eating and looting.

Some interface changes provide more information than in NetHack:

 * Item weights and carrying capacity shown.
 * Red/green highlight when attributes change.
 * Highlighting for peacefuls, locked doors, item piles and covered stairs.
 * hp_notify option to show messages when damage is taken.
 * delay_msg option to show messages for multi-turn actions and delays.
 * Corpses show rotten/very rotten when unsafe to eat.
 * Required and remaining slots shown when viewing and enhancing skills.
 * Remaining memory for spells shown.

Several changes have been made to make the game more convenient to play:

 * Ctrl-O shows dungeon overview and previous levels.
 * Ctrl-X shows intrinsic resistances and properties.
 * Ctrl-E engraves "Elbereth", which scares monsters.
 * Auto-unlock for doors/containers, auto-loot after auto-unlocking containers.
 * Resting and searching stop when HP/Pw fully recover.
 * 'ff' fires in the last direction fired in.

For those used to playing on NAO, automatic travel can target stairs even when
covered, stops when becoming hungry or enemies come into view, routes around
peacefuls and even works in Sokoban.

Autopickup is much smarter and more useful than in NetHack thanks to the
autopickup rules system, offering fine-grained control over what it picks up or
leaves.  It works much like autopickup exceptions, but can filter not only by
name, but also item class and blessing/curses.  The default rules leave shop
items alone, but the sky's the limit in what it can do.  Autopickup also leaves
dropped items alone, and grabs thrown/fired items.

Item class filters are smarter: choosing "cursed" and "armor" will match only
cursed armor, whereas in NetHack this would show all cursed items mixed with all
armor.

DynaHack features fully remappable controls, which is especially useful for
people with QWERTZ keyboards.


New content
-----------

DynaHack was originally a merge of UnNetHack's gameplay with NitroHack as its
base, so all of this new content sources from UnNetHack 4.1.1:

 * Over 80 new special levels.
 * Revamped Gehennom with special segments to explore and open caverns for a
   faster and riskier descent.
 * New branches: Town, Black Market, Dragon Caves.
 * New special rooms: nymph gardens, dilapidated armories.
 * New shops: tin shops, instrument shops, pet stores.
 * New items: rings of gain intelligence/wisdom/dexterity, potions of blood and
   vampire blood, scroll of flood, iron safe, tinfoil hat, striped shirt,
   Thiefbane, Luck Blade.
 * New monsters: gold and chromatic dragons, snow ants, vorpal jabberwocks,
   disintegrators, giant turtles, wax golems, locusts, enormous rats, rodents of
   unusual size.


New challenges
--------------

Level difficulty in DynaHack differs from NetHack.  The maximum level of
monsters that are generated on a level in NetHack is the average of the level's
depth and your experience level.  In DynaHack, only depth counts towards level
difficulty.  Experience levels tend to plateau, so overall level difficulty
scales faster and higher than in NetHack.  This would seem to make the game
harder, but the change also exists in NetHack4, which has been won by multiple
people, so the challenge is definitely surmountable.

Like UnNetHack, wishing in DynaHack is divided into magical and non-magical
items.  Only the wand of wishing can grant wishes for magical items, while every
other source can only provide non-magical items.  Examples of useful non-magical
items include the shield of reflection and dragon scales (but not dragon scale
mail).  A magic lamp is permitted as an exception for non-magical wishes.
Unlike in NetHack, wands of wishing cannot be recharged.

Dragons in DynaHack work much like in UnNetHack: names and colors are randomized
every game, so in one game a red dragon may be a tatzelworm and breathe ice, and
in another it may be called a draken and breath disintegration.  This means
wishing for dragon armor no longer gives a free ticket to reflection or magic
resistance.  On top of this, dragon armor in general is heavier and provides
less AC than in NetHack; dragon scale mail is 2/3 the weight and the same AC as
elven mithril coats.

DynaHack introduces color alchemy from UnNetHack, meaning that potions will mix
by their colors instead of by their type, e.g. a red potion mixed with a yellow
potion makes an orange potion.  In some games this leads to interesting
combinations, but this also breaks the NetHack practice of alchemizing large
stacks of potions of full healing that could as much as double the max HP of
characters.

With its roots in UnNetHack, DynaHack also adds these new challenges:

 * Elbereth is ignored by quest nemeses, unique demons and Vlad the Impaler.
 * Mysterious force removed (downward levelport while ascending through
   Gehennom); Amulet now prevents all forms of teleportation.
 * Scrolls of gold detection can no longer be used to detect traps; crystal
   balls have been made easier to use to compensate.
 * Scrolls of genocide only affect a single monster type when blessed, or the
   same on only the current level when uncursed.
 * Fighting creates noise which wakes nearby monsters, depending on stealth and
   the size of the weapon.
 * Unicorn horns no longer restore lost attributes.

DynaHack addresses two common forms of farming in NetHack: pudding farming and
throne farming.  Both of these are still possible in limited forms, but as in
UnNetHack, puddings no longer split forever, nor do they drop any items when
killed.  Thrones take time to loot and may disappear in the process.


More fairness
-------------

DynaHack reduces the punishments for hazards players cannot or do not expect to
have to defend themselves against:

 * Warnings before walking into known traps, water and lava.
 * Poison, the most common unavoidable instadeath, instead drains max HP.
 * Floating eyes paralyze for half as long, capped further by wisdom.
 * Scroll of flood replaces the scroll of amnesia, making reading random scrolls
   less game-ruining.

DynaHack also reduces the spoiler advantage that long-time NetHack players have
over novices by providing a newer and more informative database of item
descriptions from NetHack4.  It is still incomplete and in time will also
include monster info.


More variety
------------

DynaHack features an extensively bug-fixed version of magical items from
GruntHack.  Any weapon, armor, amulet or ring can appear as "magical", which may
inflict or defend against fire or frost damage, drain life, grant stealth, speed
or reflection, or even detonate on impact.  These magical properties encourage
using equipment that would otherwise be junk in NetHack; some equipment may even
fill in essential ascension kit properties, freeing up equipment slots for
alternate gear that would otherwise be displaced by ascension kit mainstays.

Wishes for magical properties can only be granted with a wand of wishing.  Even
then, such wishes only work for equipment that doesn't already have its own
magical properties, e.g. no speed boots of fire resistance.  However, no such
restriction applies to randomly generated items, so players are encouraged to
explore to find gear that may exceed what could be wished for.

DynaHack encourages more weapon variety.  Weapons may reveal their enchantment
when used, based on skill and race.  Weapon skills also cross-train,
accelerating the training of related skills to allow players to experiment with
skills early without having to fully commit to them into the late game.  The
weapon skill groups are:

 * Short Blades: dagger, knife
 * Chopping Blades: axe, pick-axe
 * Swords: long sword, two-handed sword, scimitar, saber
 * Bludgeons: club, mace, hammer
 * Flails: whip
 * Polearms: polearm, trident, lance, unicorn horn
 * Launchers: bow, crossbow
 * Thrown: dart, shuriken, boomerang

Some weapon skills (not listed above) belong to more than one group:

 * short sword: Short Blades, Swords
 * broadsword: Chopping Blades, Swords
 * morning star: Bludgeons, Flails
 * quarterstaff: Bludgeons, Polearms
 * javelin: Polearms, Thrown
 * sling: Launchers, Thrown

DynaHack introduces the new heavyshot mechanic, complementing multishot.
Certain fired or thrown weapons, rather than launching multiple volleys, will
instead launch a single volley that inflicts multiplied damage.  This makes
crossbows more interesting than heavier bows with rarer ammo, and also applies
to shuriken, spears, javelins and boomerangs.  Spears of matching race and
javelins receive a bonus multiplier when thrown.

Ranged combat is enhanced in DynaHack.  Items, especially ammo, stacks much more
readily than in NetHack, ignoring differing knowledge of blessing and
enchantment.  Further, switching main and alternate weapons using 'x' no longer
costs a turn, making launchers more viable.

DynaHack makes two-handed weapons viable throughout the game:

 * Polearms and lances can be used in melee, making them easier to train early
   and usable in dark corridors.
 * Two-handed weapons double the strength damage bonus on hits.
 * New #tip command for containers, so players with hands stuck to a cursed
   two-handed weapon can still reach their holy water without having to
   backtrack out of Gehennom.
 * Hands stuck to a cursed quarterstaff can still cast spells unhindered.

DynaHack attempts to differentiate characters of different roles by improving
their quest artifact.  Specifically, the quest artifact cannot be stolen by the
Wizard of Yendor, making it a reliable source of resistances.  Also, the quest
artifact grants magic resistance when carried by their role, giving most
characters a good reason to make use of their unique powers.

DynaHack reduces incentives to stash and backtrack:

 * Scrolls of ID work on 3-6 items regardless of their blessing.
 * Wraiths can no longer be lured across levels.


Compiling from source
---------------------

DynaHack can be compiled to run natively in Windows with
[MinGW](doc/build-mingw.md), or under [Cygwin](doc/build-cygwin.md) for those
who prefer it.

DynaHack can also be compiled for [Linux](doc/build-linux.md) and [OS
X](doc/build-osx.md).


Bug reports
-----------

Bug reports should include a description (what you did, what you expected and
what happened), as well as the specific version of the game and a copy of the
save file if possible.

Bug reports can be submitted to the GitHub project page at:

    https://github.com/tung/NitroHack

Or they can be emailed to tungtn3+dynahack@gmail.com.
