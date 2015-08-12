xlogfile patch for DynaHack
===========================

The xlogfile patch is a simplified version of the same patch for NetHack 4, main differences being:

 * The `conduct` field encodes extra DynaHack-specific bits, e.g. wishing for magical objects and breaking Sokoban rules.
 * Player username (recorded in the `name` field) is set with the `DYNAHACKUSER` environment variable.
 * Extra info recorded by modern NetHack 4 (as of August 2015) is not recorded by DynaHack, such as random number seed, birth options, intrinsics, extrinsics, and microsecond-accurate start and end times.

Here is an example of an xlogfile line produced by DynaHack:

    version=0.5.5:points=4046:deathdnum=0:deathlev=9:maxlvl=9:hp=-4:maxhp=35:deaths=1:deathdate=20150804:birthdate=20150804:uid=1000:role=Mon:race=Hum:gender=Fem:align=Neu:name=blahblah:charname=wizard:death=killed by an Uruk-hai:conduct=737176:turns=872:event=0:carried=0:starttime=1438664109:endtime=1438664323:gender0=Mal:align0=Neu:xplevel=5:exp=294:mode=debug:dumplog=2015-08-04 14_58_43, wizard-Mon-Hum-Fem-Neu, died.txt


version
-------

Version number of the game in "major.minor.revision" format, so "1.2.3" is major version 1, minor version 2 and revision 3.


points
------

Character score at time of game end.


deathdnum
---------

Dungeon number at the time of game end.

 * 0 = Dungeons of Doom (and also Fort Ludios for some reason)
 * 1 = Gehennom
 * 2 = Gnomish Mines
 * 3 = Quest
 * 4 = Sokoban
 * 5 = Town
 * 7 = Black Market
 * 8 = Vlad's Tower
 * 9 = Dragon Caves
 * 10 = Elemental Planes or Astral Plane

I'm not sure what `deathdnum` the Advent Calendar branch is, but since it's a disconnected branch like Fort Ludios it probably gets 0 as well.


deathlev
--------

Dungeon level at the time of game end, relative to the top of the dungeon.  In particular, `deathlev` == 1 is the first level of the dungeon,

Note that the order of the Elemental Planes is randomized, so `deathlev` values from -1 to -4 inclusive may not reflect the player's progress through the planes; this does NOT apply to the Astral Plane, which is always -5.


maxlvl
------

Roughly, this is the deepest dungeon level reached in the dungeon over the course of the game.

From `fill_topten_entry` in `src/topten.c`:

    /* deepest_lev_reached() is in terms of depth(), and reporting the
     * deepest level reached in the dungeon death occurred in doesn't
     * seem right, so we have to report the death level in depth() terms
     * as well (which also seems reasonable since that's all the player
     * sees on the screen anyway)
     */

From `deepest_lev_reached` in `src/dungeon.c`:

    /* this function is used for three purposes: to provide a factor
     * of difficulty in monster generation; to provide a factor of
     * difficulty in experience calculations (botl.c and end.c); and
     * to insert the deepest level reached in the game in the topten
     * display.  the 'noquest' arg switch is required for the latter.
     *
     * from the player's point of view, going into the Quest is _not_
     * going deeper into the dungeon -- it is going back "home", where
     * the dungeon starts at level 1.  given the setup in dungeon.def,
     * the depth of the Quest (thought of as starting at level 1) is
     * never lower than the level of entry into the Quest, so we exclude
     * the Quest from the topten "deepest level reached" display
     * calculation.  _However_ the Quest is a difficult dungeon, so we
     * include it in the factor of difficulty calculations.
     */


hp and maxhp
------------

Hit points and maximum hitpoints at the time of game end.


deaths
------

Number of deaths at the time of death.  Usually this is 1, but if the character had been life-saved over the course of the game this number will be higher.


deathdate and birthdate
-----------------------

Date of game end and game start respectively, in YYYYMMDD format.


uid
---

Unix user ID of the game process.


role
----

Three-letter character role:

 * Arc - archaelogist
 * Bar - barbarian
 * Cav - caveman/cavewoman
 * Con - convict
 * Hea - healer
 * Kni - knight
 * Mon - monk
 * Pri - priest
 * Ran - ranger
 * Rog - rogue
 * Sam - samurai
 * Tou - tourist
 * Val - valkyrie
 * Wiz - wizard


race
----

Three-letter character race:

 * Hum - human
 * Elf - elf
 * Dwa - dwarf
 * Gno - gnome
 * Orc - orc
 * Vam - vampire


gender
------

Gender of the character the time of game end, either "Mal" for males or "Fem" for females.


align
-----

Alignment of the character at the time of game end:

 * Law - lawful
 * Neu - neutral
 * Cha - chaotic


name
----

Player username; this should be set by the `DYNAHACKUSER` environment variable.  This falls back on the `USER` environment variable if `DYNAHACKUSER` is not set, or an empty string if that isn't set either for some reason.


charname
--------

Character name, entered by the player when they create their character.


death
-----

Cause of death or game end, e.g. "killed by an Uruk-hai" or "quit".


conduct
-------

An unsigned long in decimal form that encodes conducts adhered to, one bit per conduct:

 * 0x00001 - foodless
 * 0x00002 - vegan
 * 0x00004 - vegetarian
 * 0x00008 - athiest
 * 0x00010 - weaponless
 * 0x00020 - pacifist
 * 0x00040 - illiterate
 * 0x00080 - polypileless
 * 0x00100 - polyselfless
 * 0x00200 - wishless (any)
 * 0x00400 - wishless (artifacts)
 * 0x00800 - genocideless
 * 0x01000 - Elberethless
 * 0x02000 - wishless (magical objects)
 * 0x04000 - armorless
 * 0x08000 - zen (selected zen conduct at game start and remained blind)
 * 0x10000 - robber (never killed holders of quest artifact, invocation items or Amulet of Yendor)
 * 0x20000 - bonesless (never encountered a bones file)
 * 0x40000 - only ever wore racial armor
 * 0x80000 - never broke any Sokoban rules


turns
-----

Number of turns elapsed at the time of game end.


event
-----

An unsigned long in decimal form that encodes special events accomplished by the player, one bit per event:

 * 0x0001 - any Oracle consultation
 * 0x0002 - reached quest portal level
 * 0x0004 - was accepted for quest
 * 0x0008 - completed quest (showed quest artifact to leader)
 * 0x0010 - opened or destroyed Castle drawbridge
 * 0x0020 - entered Gehennom (the front way)
 * 0x0040 - provoked Rodney's wrath (i.e. Rodney is in his harrassment phase)
 * 0x0080 - performed the invocation
 * 0x0100 - ascended
 * 0x0200 - crowned
 * 0x0400 - defeated the quest nemesis
 * 0x0800 - defeated Croesus
 * 0x1000 - defeated Medusa
 * 0x2000 - defeated Vlad the Impaler
 * 0x4000 - defeated the Rodney at least once
 * 0x8000 - defeated Cthulhu (holder of the Amulet of Yendor)


carried
-------

An unsigned long in decimal form that encodes which objects are carried at the time of game end, one bit per object:

 * 0x0001 - real Amulet of Yendor
 * 0x0002 - Bell of Opening
 * 0x0004 - Book of the Dead
 * 0x0008 - Candelabrum of Invocation
 * 0x0010 - the character's quest artifact


starttime and endtime
---------------------

Decimal timestamp of the time of game start and game end respectively.


gender0
-------

Character gender at the start of the game, encoded the same way as the `gender` field.


align0
------

Character alignment at the start of the game, encoded the same way as the `align` field.


xplevel
-------

Character experience level at the time of game end.


exp
---

Character experience points at the time of game end.


mode
----

Game mode chosen at the start of the game:

 * debug - debug/wizard mode
 * explore - explore mode
 * normal - standard game mode, which should be most of them


dumplog
-------

Filename of the dumplog text file that summarizes the game at the time of game end.
