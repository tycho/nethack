/* Write live game progress changes to a log file
 * Needs xlog-v3 patch. */

#include "hack.h"

#ifdef LIVELOGFILE

#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif

/* Encodes the current xlog "achieve" status to an integer */
long
encodeachieve(void)
{
  /* Achievement bitfield:
   * bit  meaning
   *  0   obtained the Bell of Opening
   *  1   entered gehennom (by any means)
   *  2   obtained the Candelabrum of Invocation
   *  3   obtained the Book of the Dead
   *  4   performed the invocation ritual
   *  5   obtained the amulet
   *  6   entered elemental planes
   *  7   entered astral plane
   *  8   ascended (not "escaped in celestial disgrace!")
   *  9   obtained the luckstone from the Mines
   *  10  obtained the sokoban prize
   *  11  killed medusa
   */

  long r;

  r = 0;

  if(achieve.get_bell)           r |= 1L << 0;
  if(achieve.enter_gehennom)     r |= 1L << 1;
  if(achieve.get_candelabrum)    r |= 1L << 2;
  if(achieve.get_book)           r |= 1L << 3;
  if(achieve.perform_invocation) r |= 1L << 4;
  if(achieve.get_amulet)         r |= 1L << 5;
  if(In_endgame(&u.uz))          r |= 1L << 6;
  if(Is_astralevel(&u.uz))       r |= 1L << 7;
  if(achieve.ascended)           r |= 1L << 8;
  if(achieve.get_luckstone)      r |= 1L << 9;
  if(achieve.finish_sokoban)     r |= 1L << 10;
  if(achieve.killed_medusa)      r |= 1L << 11;

  return r;
}

/* Keep the last xlog "achieve" value to be able to compare */
long last_achieve_int;

/* Generic buffer for snprintf */
#define STRBUF_LEN (4096)
char strbuf[STRBUF_LEN];
char prefixbuf[STRBUF_LEN];

/* Open the live log file */
boolean livelog_start() {


	last_achieve_int = encodeachieve();

	return TRUE;
}

/* Locks the live log file and writes 'buffer' */
void livelog_write_string(char* buffer) {
	FILE* livelogfile;
	if(lock_file(LIVELOGFILE, SCOREPREFIX, 10)) {
		if(!(livelogfile = fopen_datafile(LIVELOGFILE, "a", SCOREPREFIX))) {
			pline("Cannot open live log file!");
		} else {
			fprintf(livelogfile, buffer);
			(void) fclose(livelogfile);
		}
		unlock_file(LIVELOGFILE);
	}
}

/** Returns
 role, race, gender
currenttime
*/
static
char *livelog_prefix() {
	snprintf(prefixbuf, STRBUF_LEN,
			"version=%s-%d.%d.%d:"
			"player=%s:turns=%ld:starttime=%ld:"
			"currenttime=%ld:"
			"dnum=%d:dlev=%d:maxlvl=%d:"
			"hp=%d:maxhp=%d:deaths=%d:"
#ifdef RECORD_REALTIME
			"realtime=%ld:"
#endif
			"conduct=0x%lx:"
			"role=%s:race=%s:"
			"gender=%s:align=%s:"
			"gender0=%s:align0=%s",
			GAME_SHORT_NAME, VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL,
			plname,
			moves,
			(long)u.ubirthday,
			(long)current_epoch(),
			u.uz.dnum, depth(&u.uz), deepest_lev_reached(TRUE),
			u.uhp, u.uhpmax, u.umortality,
#ifdef RECORD_REALTIME
			(long)realtime_data.realtime,
#endif
			encodeconduct(),
			urole.filecode, urace.filecode,
			genders[flags.female].filecode, aligns[1-u.ualign.type].filecode,
			genders[flags.initgend].filecode, aligns[1-u.ualignbase[A_ORIGINAL]].filecode);
	return prefixbuf;
}

/* Writes changes in the achieve structure to the live log.
 * Called from various places in the NetHack source,
 * usually where xlog's achieve is set. */
void livelog_achieve_update() {
	long achieve_int, achieve_diff;

	achieve_int = encodeachieve();
	achieve_diff = last_achieve_int ^ achieve_int;

	/* livelog_achieve_update is sometimes called when there's
	 * no actual change. */
	if(achieve_diff == 0) {
		return;
	}

	snprintf(strbuf, STRBUF_LEN,
		"%s:achieve=0x%lx:achieve_diff=0x%lx\n",
		livelog_prefix(),
		achieve_int,
		achieve_diff);
	livelog_write_string(strbuf);

	last_achieve_int = achieve_int;
}

/* Reports wishes */
void
livelog_wish(item)
char *item;
{
	snprintf(strbuf, STRBUF_LEN,
		"%s:wish=%s:wish_count=%ld\n",
		livelog_prefix(),
		item,
		u.uconduct.wishes);
	livelog_write_string(strbuf);
}

/* Shout */
#ifdef LIVELOG_SHOUT

int 
doshout()
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	char* p;
	
	Sprintf(qbuf,"Shout what?");
	getlin(qbuf, buf);
	
	You("shout into the void: %s", buf);

	/* filter livelog delimiter */
	for (p = buf; *p != 0; p++)
		if( *p == ':' )
			*p = ' ';

	livelog_generic("shout", buf);

	return 0;
}

#endif /* LIVELOG_SHOUT */

#ifdef LIVELOG_BONES_KILLER
void
livelog_bones_killed(mtmp)
struct monst *mtmp;
{
	char *name = NAME(mtmp);

	if (name &&
	    mtmp->former_rank && strlen(mtmp->former_rank) > 0) {
		/* $player killed the $bones_monst of $bones_killed the former
		 * $bones_rank on $turns on dungeon level $dlev! */
		snprintf(strbuf, STRBUF_LEN,
				"%s:bones_killed=%s:bones_rank=%s:bones_monst=%s\n",
				livelog_prefix(),
				name,
				mtmp->former_rank,
				mtmp->data->mname);
		livelog_write_string(strbuf);
	} else if ((mtmp->data->geno & G_UNIQ)
#ifdef BLACKMARKET
	           || (mtmp->data == &mons[PM_BLACK_MARKETEER])
#endif
		  ) {
		char *n = noit_mon_nam(mtmp);
		/* $player killed a uniq monster */
		livelog_generic("killed_uniq", n);
	}
}
#endif /* LIVELOG_BONES_KILLER */

/** Reports shoplifting */
void
livelog_shoplifting(shk_name, shop_name, total)
const char* shk_name;
const char* shop_name;
long total;
{
	/* shopkeeper: Name of the shopkeeper (e.g. Kopasker)
	   shop:       Name of the shop (e.g. general store)
	   shoplifted: Merchandise worth this many Zorkmids was stolen */
	snprintf(strbuf, STRBUF_LEN,
		"%s:shopkeeper=%s:shop=%s:shoplifted=%ld\n",
		livelog_prefix(),
		shk_name,
		shop_name,
		total);
	livelog_write_string(strbuf);
}

/** Livelog method for reporting the starting/resuming of a game. */
void
livelog_game_started(verb, alignment_sex, race, role)
const char* verb;
const char* alignment_sex;
const char* race;
const char* role;
{
	snprintf(strbuf, STRBUF_LEN,
		"%s:game_action=%s:character=%s %s %s\n",
		livelog_prefix(),
		verb,
		alignment_sex,
		race,
		role);
	livelog_write_string(strbuf);
}

/** Livelog method for reporting saving, quitting, etc. */
void
livelog_game_action(verb)
const char* verb;
{
	snprintf(strbuf, STRBUF_LEN,
		"%s:game_action=%s\n",
		livelog_prefix(),
		verb);
	livelog_write_string(strbuf);
}

/** Livelog method for reporting generic events with one customizable field. */
void
livelog_generic(field, text)
const char* field;
const char* text;
{
	snprintf(strbuf, STRBUF_LEN,
		"%s:%s=%s\n",
		livelog_prefix(),
		field,
		text);
	livelog_write_string(strbuf);
}

#endif /* LIVELOGFILE */
