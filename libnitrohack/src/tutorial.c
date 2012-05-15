/*	Copyright 2009, Alex Smith		  */
/* NitroHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "quest.h"
#include "qtext.h"


static char pl_tutorial[QT_T_MAX - QT_T_FIRST + 1];
#define CHECK_TUTORIAL_COMMAND_BUFSIZE 100
static int check_tutorial_command_message;
static char check_tutorial_command_buffer[CHECK_TUTORIAL_COMMAND_BUFSIZE];
static int check_tutorial_command_pointer;
static int check_tutorial_command_count;

static struct attribs tutorial_old_attribs;
#define TUTORIAL_IMPOSSIBLE_AC 300
static int tutorial_old_ac;
/* So tutorial doesn't detect two combats if a monster disappears. */
static int tutorial_last_combat;


void init_tutorial(void)
{
	int i;

	memset(pl_tutorial, 0, sizeof(pl_tutorial));
	check_tutorial_command_message = 0;
	check_tutorial_command_pointer = 0;
	check_tutorial_command_count = 0;

	for (i = 0; i < A_MAX; i++)
	    tutorial_old_attribs.a[i] = 0;
	tutorial_old_ac = TUTORIAL_IMPOSSIBLE_AC;
	tutorial_last_combat = 0;
}

void save_tutorial(struct memfile *mf)
{
	int i;

	mwrite(mf, pl_tutorial, sizeof(pl_tutorial));
	mwrite32(mf, check_tutorial_command_message);
	mwrite(mf, check_tutorial_command_buffer,
	       sizeof(check_tutorial_command_buffer));
	mwrite32(mf, check_tutorial_command_pointer);
	mwrite32(mf, check_tutorial_command_count);

	for (i = 0; i < A_MAX; i++)
	    mwrite8(mf, tutorial_old_attribs.a[i]);
	mwrite32(mf, tutorial_old_ac);
	mwrite32(mf, tutorial_last_combat);
}

void restore_tutorial(struct memfile *mf)
{
	int i;

	mread(mf, pl_tutorial, sizeof(pl_tutorial));
	check_tutorial_command_message = mread32(mf);
	mread(mf, check_tutorial_command_buffer,
	      sizeof(check_tutorial_command_buffer));
	check_tutorial_command_pointer = mread32(mf);
	check_tutorial_command_count = mread32(mf);

	for (i = 0; i < A_MAX; i++)
	    tutorial_old_attribs.a[i] = mread8(mf);
	tutorial_old_ac = mread32(mf);
	tutorial_last_combat = mread32(mf);
}

/* Display a tutorial message, if it hasn't been displayed before.
   Returns TRUE if a tutorial message is output. */
boolean check_tutorial_message(int msgnum)
{
	if (!flags.tutorial) return FALSE;
	if (pl_tutorial[msgnum - QT_T_FIRST] > 0) return FALSE;

	pl_tutorial[msgnum - QT_T_FIRST] = 1;

	/* Show matching vi-keys message in tutorial review menu
	   when a numpad tutorial message is shown. */
	switch (msgnum) {
	case QT_T_CURSOR_NUMPAD:
	    pl_tutorial[QT_T_CURSOR_VIKEYS - QT_T_FIRST] = 1;
	    break;
	case QT_T_FARMOVE_NUMPAD:
	    pl_tutorial[QT_T_FARMOVE_VIKEYS - QT_T_FIRST] = 1;
	    break;
	case QT_T_DIAGONALS_NUM:
	    pl_tutorial[QT_T_DIAGONALS_VI - QT_T_FIRST] = 1;
	    break;
	case QT_T_REPEAT_NUMPAD:
	    pl_tutorial[QT_T_REPEAT_VIKEYS - QT_T_FIRST] = 1;
	    break;
	}

	com_pager(msgnum);
	return TRUE;
}

/* Displays a tutorial message pertaining to object class oclass, if
   it hasn't been shown already. Returns 1 if the object class is one
   for which tutorial messages exist, regardless of whether the
   message is shown or not. This assumes that the object classes haven't
   been customized too heavily (possibly a custom boulder, and that's it). */
boolean check_tutorial_oclass(int oclass)
{
	switch(oclass) {
	case WEAPON_CLASS: check_tutorial_message(QT_T_ITEM_WEAPON);	return TRUE;
	case FOOD_CLASS:   check_tutorial_message(QT_T_ITEM_FOOD);	return TRUE;
	case GEM_CLASS:    check_tutorial_message(QT_T_ITEM_GEM);	return TRUE;
	case TOOL_CLASS:   check_tutorial_message(QT_T_ITEM_TOOL);	return TRUE;
	case AMULET_CLASS: check_tutorial_message(QT_T_ITEM_AMULET);	return TRUE;
	case POTION_CLASS: check_tutorial_message(QT_T_ITEM_POTION);	return TRUE;
	case SCROLL_CLASS: check_tutorial_message(QT_T_ITEM_SCROLL);	return TRUE;
	case SPBOOK_CLASS: check_tutorial_message(QT_T_ITEM_BOOK);	return TRUE;
	case ARMOR_CLASS:  check_tutorial_message(QT_T_ITEM_ARMOR);	return TRUE;
	case WAND_CLASS:   check_tutorial_message(QT_T_ITEM_WAND);	return TRUE;
	case RING_CLASS:   check_tutorial_message(QT_T_ITEM_RING);	return TRUE;
	case ROCK_CLASS:   check_tutorial_message(QT_T_ITEM_STATUE);	return TRUE;
	case COIN_CLASS:   check_tutorial_message(QT_T_ITEM_GOLD);	return TRUE;
	default: return FALSE; /* venom/ball/chain/mimic don't concern us */
	}
}

/* Displays a tutorial message pertaining to the location at (lx, ly),
   if there is a message and it hasn't been shown already. Returns TRUE
   if a message is shown. */
static boolean check_tutorial_location(int lx, int ly, boolean from_farlook)
{
	const struct rm *l = &level->locations[lx][ly];

	if (!flags.tutorial) return FALSE; /* short-circuit */

	if (l->mem_trap) /* seen traps only */
	    if (check_tutorial_message(QT_T_TRAP)) return TRUE;

	if (IS_DOOR(l->typ) && l->doormask >= D_ISOPEN)
	    if (check_tutorial_message(QT_T_DOORS)) return TRUE;

	if (l->typ == CORR)
	    if (check_tutorial_message(QT_T_CORRIDOR)) return TRUE;

	/* A freebie: we give away the location of a secret door or
	   corridor, once. This is so that the advice to search will
	   always end up coming good, to avoid confusing new players;
	   it also deals with the horrific possibility of a player's
	   first game having no visible exits from the first room (it
	   can happen!) */
	if (l->typ == SCORR || l->typ == SDOOR)
	    if (check_tutorial_message(QT_T_SECRETDOOR)) return TRUE;

	if (l->typ == POOL || l->typ == MOAT)
	    if (check_tutorial_message(QT_T_POOLORMOAT)) return TRUE;

	if (l->typ == LAVAPOOL)
	    if (check_tutorial_message(QT_T_LAVA)) return TRUE;

	if (l->typ == STAIRS) {
	    /* In which direction? */
	    if ((lx == level->upstair.sx && ly == level->upstair.sy) ||
		(lx == level->upladder.sx && ly == level->upladder.sy) ||
		(lx == level->sstairs.sx && ly == level->sstairs.sy &&
		 level->sstairs.up)) {
		if (u.uz.dlevel > 1) {
		    if (check_tutorial_message(QT_T_STAIRS)) return TRUE;
		} else if (from_farlook) {
		    if (check_tutorial_message(QT_T_L1UPSTAIRS)) return TRUE;
		}
	    } else if ((lx == level->dnstair.sx && ly == level->dnstair.sy) ||
		       (lx == level->dnladder.sx && ly == level->dnladder.sy) ||
		       (lx == level->sstairs.sx && ly == level->sstairs.sy &&
		        !level->sstairs.up)) {
		if (check_tutorial_message(QT_T_STAIRS)) return TRUE;
	    } else warning("Stairs go neither up nor down?");
	}

	if (l->typ == FOUNTAIN)
	    if (check_tutorial_message(QT_T_FOUNTAIN)) return TRUE;

	if (l->typ == THRONE)
	    if (check_tutorial_message(QT_T_THRONE)) return TRUE;

	if (l->typ == SINK)
	    if (check_tutorial_message(QT_T_SINK)) return TRUE;

	if (l->typ == GRAVE)
	    if (check_tutorial_message(QT_T_GRAVE)) return TRUE;

	if (l->typ == ALTAR)
	    if (check_tutorial_message(QT_T_ALTAR)) return TRUE;

	if (IS_DRAWBRIDGE(l->typ))
	    if (check_tutorial_message(QT_T_DRAWBRIDGE)) return TRUE;

	return FALSE;
}

/* Display tutorial messages that may result from farlook data. */
void check_tutorial_farlook(int x, int y)
{
	const struct rm *l = &level->locations[x][y];
	int monid;
	const struct monst *mtmp;
	const struct obj *otmp;

	if (!flags.tutorial) return; /* short-circuit */

	/* Monsters */
	if (l->mem_invis) {
	    check_tutorial_message(QT_T_LOOK_INVISIBLE);
	    return;
	}

	monid = dbuf_get_mon(x, y) - 1;
	if (monid != -1 && (mtmp = m_at(level, x, y))) {
	    mtmp = m_at(level, x, y);
	    if (mtmp &&
		((mtmp->mtame && !Hallucination) ||
		 /* riding implies tameness */
		 (x == u.ux && y == u.uy && u.usteed))) {
		check_tutorial_message(QT_T_LOOK_TAME);
	    } else if (mtmp && mtmp->mpeaceful && !Hallucination) {
		check_tutorial_message(QT_T_LOOK_PEACEFUL);
	    } else if (x == u.ux && y == u.uy) {
		/* looking at yourself, no message yet */
	    } else {
		check_tutorial_message(QT_T_LOOK_HOSTILE);
	    }
	    return;
	}

	/* Items */
	otmp = vobj_at(x, y);
	if (otmp) {
	    check_tutorial_oclass(otmp->oclass);
	    return;
	}

	/* Terrain */
	if (l->mem_bg || l->mem_trap) {
	    check_tutorial_location(x, y, TRUE);
	    return;
	}
}

/* Convert commands into chars to store in the tutorial command buffer. */
static char tutorial_translate_command(int command, const struct nh_cmd_arg *arg)
{
	/* Borrowing this like allmain.c to translate commands. */
	extern const struct cmd_desc cmdlist[];

	const char *cmd_name = cmdlist[command].name;
	if (cmd_name == NULL)
	    return '\0'; /* Interesting... */

	if (!strcmp(cmd_name, "move")) {
	    /* Detect cardinal versus diagonal movement. */
	    if ((cmdlist[command].flags & CMD_ARG_FLAGS) == CMD_ARG_DIR) {
		schar dx, dy, dz;
		if (!dir_to_delta(arg->d, &dx, &dy, &dz))
		    return '\0'; /* Bad direction. */
		/* 'y' == diagonal, 'b' == cardinal
		   Presumably they were chosen because they'd never be
		   confused with command keys in either the classic
		   vi-keys or numpad configurations.
		   This tutorial doesn't track anything not in this
		   function anyway, so that won't be a problem here. */
		return (dx && dy) ? 'y' : 'b';
	    }
	    return '\0'; /* Move without direction? */
	} else if (!strcmp(cmd_name, "run") || !strcmp(cmd_name, "go2")) {
	    return 'G';
	} else if (!strcmp(cmd_name, "remove") || !strcmp(cmd_name, "takeoff")) {
	    return 'T';
	} else if (!strcmp(cmd_name, "farlook")) {
	    return ';';
	} else if (!strcmp(cmd_name, "wield")) {
	    return 'w';
	} else if (!strcmp(cmd_name, "search")) {
	    return 's';
	} else if (!strcmp(cmd_name, "drop")) {
	    return 'd';
	} else if (!strcmp(cmd_name, "menuinv")) {
	    return 'I';
	} else if (!strcmp(cmd_name, "throw")) {
	    return 't';
	}

	/* If check_tutorial_command doesn't check it, it doesn't matter. */
	return '\0';
}

void check_tutorial_command(int command, const struct nh_cmd_arg *arg)
{
	int i, r;
	char c, lc;

	boolean travel = TRUE;
	boolean farmove = TRUE;
	boolean repeat = TRUE;
	boolean massunequip = TRUE;
	boolean look_reminder = TRUE;
	int secondwield = 0;

	if (!flags.tutorial) return; /* short-circuit */

	c = tutorial_translate_command(command, arg);

	check_tutorial_command_buffer[check_tutorial_command_pointer] = c;
	i = check_tutorial_command_pointer;
	check_tutorial_command_pointer++;
	check_tutorial_command_pointer %= CHECK_TUTORIAL_COMMAND_BUFSIZE;
	if (check_tutorial_command_count < CHECK_TUTORIAL_COMMAND_BUFSIZE)
	    check_tutorial_command_count++; /* any higher has no other meaning */
	check_tutorial_command_message = 0;

	r = 0;
	lc = c;
	do {
	    c = check_tutorial_command_buffer[i];
	    if (lc != c) repeat = FALSE;
	    if (c != 'y' && c != 'b' && c != 'G') travel = FALSE;
	    if (c != 'y' && c != 'b') farmove = FALSE;
	    if (c != 'R' && c != 'T') massunequip = FALSE;
	    if (c == ';') look_reminder = FALSE;
	    if (c == 'w') secondwield++;
	    r++;
	    if (r > check_tutorial_command_count) break;
	    if (moves > 125 && r > 5 && farmove) {
		check_tutorial_command_message = QT_T_FARMOVE_NUMPAD;
		break;
	    }
	    if (moves > 125 && r > 30 && travel) {
		check_tutorial_command_message = QT_T_TRAVEL;
		break;
	    }
	    if (moves > 80 && r > 20 && c == 'b') {
		check_tutorial_command_message = QT_T_DIAGONALS_NUM;
		break;
	    }
	    if (repeat && r > 5 && c == 's') {
		check_tutorial_command_message = QT_T_REPEAT_NUMPAD;
		break;
	    }
	    if (moves > 45 && r >= 2 && massunequip) {
		check_tutorial_command_message = QT_T_MASSUNEQUIP;
		break;
	    }
	    if (moves > 45 && r >= 2 && repeat && c == 'd') {
		check_tutorial_command_message = QT_T_MULTIDROP;
		break;
	    }
	    if (moves > 45 && r >= 2 && repeat && c == 'I') {
		check_tutorial_command_message = QT_T_MASSINVENTORY;
		break;
	    }
	    if (moves > 45 && secondwield >= 3 && r == 50) {
		check_tutorial_command_message = QT_T_SECONDWIELD;
		break;
	    }
	    if (r >= 3 && repeat && c == 't') {
		check_tutorial_command_message = QT_T_FIRE;
		break;
	    }
	    i--;
	    if (i == -1) i = CHECK_TUTORIAL_COMMAND_BUFSIZE - 1;
	    lc = c;
	} while (r < CHECK_TUTORIAL_COMMAND_BUFSIZE);

	if (check_tutorial_command_message == 0 && look_reminder &&
	    check_tutorial_command_count >= CHECK_TUTORIAL_COMMAND_BUFSIZE)
	    check_tutorial_command_message = QT_T_LOOK_REMINDER;
}

/* These are #defined in every file that they're used!
   There has to be a better way... */
#define SATIATED	0
#define NOT_HUNGRY	1
#define HUNGRY		2
#define WEAK		3
#define FAINTING	4
#define FAINTED		5
#define STARVED		6

/* Display tutorial messages based on the state of the character. */
void maybe_tutorial(void)
{
	int old_tutorial_last_combat;
	int i;
	const struct monst *mtmp;

	if (!flags.tutorial) return; /* short-circuit */

	/* Check to see if any tutorial triggers have occured.
	   Stop checking once one message is output. */

	/* Welcome message: show immediately */
	if (check_tutorial_message(QT_T_WELCOME)) return;

	/* Check surroundings; these only occur if at least 3 turns
	   have elapsed, to avoid overwhelming the player early on. */
	if (moves > 3) {
	    int dx, dy;
	    int monnum = -1;
	    for (dx = -1; dx <= 1; dx++) {
		for (dy = -1; dy <= 1; dy++) {
		    if (isok(u.ux + dx, u.uy + dy)) {
			int lx = u.ux + dx;
			int ly = u.uy + dy;
			const struct rm *l = &level->locations[lx][ly];

			/* Terrain checks */
			if (check_tutorial_location(lx, ly, FALSE)) return;

			/* Adjacent invisible monsters */
			if (l->mem_invis)
			    if (check_tutorial_message(QT_T_LOOK_INVISIBLE)) return;

			/* More than one of the same monster adjacent */
			if (dbuf_get_mon(lx, ly)) {
			    if (monnum == (dbuf_get_mon(lx, ly) - 1))
				if (check_tutorial_message(QT_T_CALLMONSTER)) return;
			    monnum = dbuf_get_mon(lx, ly) - 1;
			}
		    }
		}
	    }
	}

	/* Check to see if we're in combat. */
	tutorial_last_combat++;
	old_tutorial_last_combat = tutorial_last_combat;
	for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp) && cansee(mtmp->mx, mtmp->my) && !mtmp->mtame)
		tutorial_last_combat = 0;

	/* Ambient messages that only come up during combat, and only one
	   message per combat */
	if (!tutorial_last_combat && old_tutorial_last_combat > 5) {
	    if (u.uz.dlevel >= 3)
		if (flags.elbereth_enabled &&
		    check_tutorial_message(QT_T_ELBERETH)) return;
	    for (i = 0; i < MAXSPELL && spellid(i) != NO_SPELL; i++) {
		if (spellid(i) == SPE_FORCE_BOLT)
		    if (check_tutorial_message(QT_T_SPELLS)) return;
	    }
	    if (check_tutorial_message(QT_T_MELEE)) return;
	}

	/* Events! */

	if (spellid(1)) {
	    if ((uarm && is_metallic(uarm)) ||
		(uarms && !Role_if(PM_PRIEST)) || /* priests start with shields */
		(uarmh && is_metallic(uarmh) && uarmh->otyp != HELM_OF_BRILLIANCE) ||
		(uarmg && is_metallic(uarmg)) ||
		(uarmf && is_metallic(uarmf)))
		if (check_tutorial_message(QT_T_CASTER_ARMOR)) return;
	}

	if (uwep && uwep != &zeroobj && weapon_dam_bonus(uwep) < 0 && !u.twoweap)
	    if (check_tutorial_message(QT_T_WEAPON_SKILL)) return;
	if (u.ulevel >= 2)
	    if (check_tutorial_message(QT_T_LEVELUP)) return;
	if (u.ulevel >= 3)
	    if (check_tutorial_message(QT_T_RANKUP)) return;
	{
	    int incdec = 0;
	    boolean firstturn = tutorial_old_attribs.a[0] == 0;
	    for (i = 0; i < A_MAX; i++) {
		if (u.acurr.a[i] > tutorial_old_attribs.a[i]) incdec = 1;
		if (u.acurr.a[i] < tutorial_old_attribs.a[i]) incdec = -1;
		tutorial_old_attribs.a[i] = u.acurr.a[i];
	    }
	    if (!firstturn) {
		if (incdec > 0)
		    if (check_tutorial_message(QT_T_ABILUP)) return;
		if (incdec < 0)
		    if (check_tutorial_message(QT_T_ABILDOWN)) return;
	    }
	}
	if (u.uac < tutorial_old_ac && tutorial_old_ac != TUTORIAL_IMPOSSIBLE_AC)
	    if (check_tutorial_message(QT_T_ACIMPROVED)) return;
	tutorial_old_ac = u.uac;

	if (Confusion || Sick || Blind || Stunned || Hallucination || Slimed)
	    if (check_tutorial_message(QT_T_STATUS)) return;

	if (u.uz.dlevel >= 2)
	    if (check_tutorial_message(QT_T_DLEVELCHANGE)) return;

	if (u.uhp < u.uhpmax)
	    if (check_tutorial_message(QT_T_DAMAGED)) return;
	if (u.uen < u.uenmax)
	    if (check_tutorial_message(QT_T_PWUSED)) return;
	if (u.uen < 5 && u.uenmax > 10)
	    if (check_tutorial_message(QT_T_PWEMPTY)) return;

	if (u.umonster != u.umonnum)
	    if (check_tutorial_message(QT_T_POLYSELF)) return;

	if (u.uexp > 0)
	    if (check_tutorial_message(QT_T_GAINEDEXP)) return;

	if (u.uhs >= HUNGRY)
	    if (check_tutorial_message(QT_T_HUNGER)) return;
	if (u.uhs <= SATIATED)
	    if (check_tutorial_message(QT_T_SATIATION)) return;

	if (can_advance_something())
	    if (check_tutorial_message(QT_T_ENHANCE)) return;

	if (u.uswallow)
	    if (check_tutorial_message(QT_T_ENGULFED)) return;

	if (near_capacity() > UNENCUMBERED)
	    if (check_tutorial_message(QT_T_BURDEN)) return;

	if (in_trouble() > 0 && can_pray(0) &&
	    !IS_ALTAR(level->locations[u.ux][u.uy].typ))
	    if (check_tutorial_message(QT_T_MAJORTROUBLE)) return;

	if (inside_shop(level, u.ux, u.uy))
	    if (check_tutorial_message(QT_T_SHOPENTRY)) return;

	if (u.uz.dnum == mines_dnum)
	    if (check_tutorial_message(QT_T_MINES)) return;
	if (u.uz.dnum == sokoban_dnum)
	    if (check_tutorial_message(QT_T_SOKOBAN)) return;
	if (check_tutorial_command_message == QT_T_FIRE)
	    if (check_tutorial_message(QT_T_FIRE)) return;

	/* Item-dependent events. */

	{
	    int projectile_groups = 0;
	    int launcher_groups = 0;
	    struct obj *otmp;

	    for (otmp = invent; otmp; otmp = otmp->nobj) {
		if (otmp->bknown && otmp->cursed)
		    if (check_tutorial_message(QT_T_EQUIPCURSE)) return;

		if (otmp->oartifact)
		    if (check_tutorial_message(QT_T_ARTIFACT)) return;

		if (otmp->unpaid)
		    if (check_tutorial_message(QT_T_SHOPBUY)) return;

		if (!objects[otmp->otyp].oc_name_known) {
		    switch (objects[otmp->otyp].oc_class) {
		    case POTION_CLASS:
			if (otmp->otyp == POT_WATER) break;
			/* fall through */
		    case SCROLL_CLASS:
			if (otmp->otyp == SCR_BLANK_PAPER) break;
			/* fall through */
		    case WAND_CLASS:
		    case SPBOOK_CLASS:
			if (otmp->otyp == SPE_BLANK_PAPER) break;
			if (otmp->otyp == SPE_BOOK_OF_THE_DEAD) break;
			/* fall through */
		    case RING_CLASS:
		    case AMULET_CLASS:
			if (check_tutorial_message(QT_T_RANDAPPEARANCE)) return;
			break;
		    default:
			break;
		    }
		}

		/* Containers; minor spoiler here, in that it doesn't trigger
		   off a bag of tricks and a savvy player might notice that, but
		   that's not a freebie I'm worried about. */
		switch (otmp->otyp) {
		case SACK:
		    /* starting inventory for arcs/rogues, and we don't want to
		       give the message until a second container's listed */
		    if (Role_if(PM_ARCHEOLOGIST)) break;
		    if (Role_if(PM_ROGUE)) break;
		    /* otherwise fall through */
		case LARGE_BOX:
		case CHEST:
		case ICE_BOX:
		case OILSKIN_SACK:
		case BAG_OF_HOLDING:
		    if (check_tutorial_message(QT_T_ITEM_CONTAINER)) return;
		    break;
		default:
		    break;
		}

		/* Requiring a specific item during combat... */
		if (!tutorial_last_combat && old_tutorial_last_combat > 5) {
		    switch (otmp->otyp) {
		    /* Projectiles. */
		    case ARROW:
		    case ELVEN_ARROW:
		    case ORCISH_ARROW:
		    case SILVER_ARROW:
		    case YA:
			projectile_groups |= 0x1;
			break;
		    case CROSSBOW_BOLT:
			projectile_groups |= 0x2;
			break;
		    case FLINT:
		    case ROCK:
			projectile_groups |= 0x4;
			break;

		    /* Launchers. */
		    case BOW:
		    case ELVEN_BOW:
		    case ORCISH_BOW:
		    case YUMI:
			launcher_groups |= 0x1;
			break;
		    case CROSSBOW:
			launcher_groups |= 0x2;
			break;
		    case SLING:
			launcher_groups |= 0x4;
			break;

		    /* Thrown weapons. Don't count our wielded weapon in this. */
		    case DART:
		    case SHURIKEN:
		    case BOOMERANG:
		    case SPEAR:
		    case ELVEN_SPEAR:
		    case ORCISH_SPEAR:
		    case DWARVISH_SPEAR:
		    case JAVELIN:
		    case DAGGER:
		    case ELVEN_DAGGER:
		    case ORCISH_DAGGER:
		    case SILVER_DAGGER:
			if (otmp == uwep) break;
			if (check_tutorial_message(QT_T_THROWNWEAPONS)) return;
			break;

		    default:
			break;
		    }
		}
	    }
	    if (projectile_groups & launcher_groups)
		if (check_tutorial_message(QT_T_PROJECTILES)) return;
	}

	/* Items on the current square. */
	{
	    struct obj *otmp;
	    for (otmp = level->objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
		switch (otmp->otyp) {
		case LARGE_BOX:
		case CHEST:
		case ICE_BOX:
		    if (check_tutorial_message(QT_T_ITEM_CONTAINER)) return;
		    break;
		default:
		    break;
		}
	    }
	}

	/* Ambient messages that only come up outside combat */
	if (tutorial_last_combat > 5) {
	    if (check_tutorial_command_message > 0)
		if (check_tutorial_message(check_tutorial_command_message)) return;
	    if (moves >= 10)
		if (check_tutorial_message(QT_T_VIEWTUTORIAL)) return;
	    if (moves >= 30)
		if (check_tutorial_message(QT_T_CHECK_ITEMS)) return;
	    if (moves >= 60)
		if (check_tutorial_message(QT_T_OBJECTIVE)) return;
	    if (moves >= 100)
		if (check_tutorial_message(QT_T_SAVELOAD)) return;
	    if (moves >= 150)
		if (check_tutorial_message(QT_T_MESSAGERECALL)) return;
	}
}

/* Redisplay tutorial messages. */
void tutorial_redisplay(void)
{
	struct menulist menu;
	int i, n;
	int selected[1];

	init_menulist(&menu);
	for (i = QT_T_FIRST; i <= QT_T_MAX; i++) {
	    if (pl_tutorial[i - QT_T_FIRST] > 0) {
		char namebuf[80];
		char *name;
		qt_com_firstline(i, namebuf);
		/* add 10 to namebuf to remove the 'Tutorial: ' at the start */
		name = *namebuf ? namebuf + 10 : "(not found)";
		add_menuitem(&menu, i, name, 0, FALSE);
	    }
	}
	n = display_menu(menu.items, menu.icount, "Review which tutorial?",
			 PICK_ONE, selected);
	free(menu.items);

	if (n <= 0) return;
	com_pager(selected[0]);
}

/* tutorial.c */
