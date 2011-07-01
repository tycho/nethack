/*	SCCS Id: @(#)dungeon.c	3.1	93/06/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dgn_file.h"

#ifdef OVL1

#define	DUNGEON_FILE	"dungeon"
#if (defined(MICRO) && !defined(AMIGA)) || defined(THINK_C)
# define RDMODE "rb"
#else
# define RDMODE "r"
#endif

#ifdef MULDGN
#define	X_START		"x-start"
#define X_LOCATE	"x-locate"
#define	X_GOAL		"x-goal"
#endif

struct proto_dungeon {
	struct	tmpdungeon tmpdungeon[MAXDUNGEON];
	struct	tmplevel   tmplevel[LEV_LIMIT];
	s_level *final_lev[LEV_LIMIT];	/* corresponding level pointers */
	struct	tmpbranch  tmpbranch[BRANCH_LIMIT];

	int	start;	/* starting index of current dungeon sp levels */
	int	n_levs;	/* number of tmplevel entries */
	int	n_brs;	/* number of tmpbranch entries */
};

int n_dgns;				/* number of dungeons (used here,  */
					/*   and mklev.c)		   */
static branch *branches = (branch *) 0;	/* dungeon branch list		   */

static void FDECL(Fread, (genericptr_t, int, int, FILE *));
static xchar FDECL(dname_to_dnum, (const char *));
static int FDECL(find_branch, (const char *, struct proto_dungeon *));
static xchar FDECL(parent_dnum, (const char *, struct proto_dungeon *));
static int FDECL(level_range, (XCHAR_P,int,int,int,struct proto_dungeon *,int *));
static xchar FDECL(parent_dlevel, (const char *, struct proto_dungeon *));
static int FDECL(correct_branch_type, (struct tmpbranch *));
static branch *FDECL(add_branch, (int, int, struct proto_dungeon *));
static void FDECL(add_level, (s_level *));
static void FDECL(init_level, (int,int,struct proto_dungeon *));
static int FDECL(possible_places, (int, boolean *, struct proto_dungeon *));
static xchar FDECL(pick_level, (boolean *, int));
static boolean FDECL(place_level, (int, struct proto_dungeon *));
#ifdef WIZARD
static const char *FDECL(br_string, (int));
static void FDECL(print_branch, (winid, int, int, int));
#endif

#ifdef DEBUG
#define DD	dungeons[i]
static void NDECL(dumpit);

static void
dumpit()
{
	int	i;
	s_level	*x;
	branch *br;

	for(i = 0; i < n_dgns; i++)  {
	    fprintf(stderr, "\n#%d \"%s\" (%s):\n", i,
				DD.dname, DD.proto);
	    fprintf(stderr, "    num_dunlevs %d, dunlev_ureached %d\n",
				DD.num_dunlevs, DD.dunlev_ureached);
	    fprintf(stderr, "    depth_start %d, ledger_start %d\n",
				DD.depth_start, DD.ledger_start);
	    fprintf(stderr, "    flags:%s%s%s\n",
		    DD.flags.rogue_like ? " rogue_like" : "",
		    DD.flags.maze_like  ? " maze_like"  : "",
		    DD.flags.hellish    ? " hellish"    : "");
	    getchar();
	}
	fprintf(stderr,"\nSpecial levels:\n");
	for(x = sp_levchn; x; x = x->next) {
	    fprintf(stderr, "%s (%d): ", x->proto, x->rndlevs);
	    fprintf(stderr, "on %d, %d; ", x->dlevel.dnum, x->dlevel.dlevel);
	    fprintf(stderr, "flags:%s%s%s%s\n",
		    x->flags.rogue_like	? " rogue_like" : "",
		    x->flags.maze_like  ? " maze_like"  : "",
		    x->flags.hellish    ? " hellish"    : "",
		    x->flags.town       ? " town"       : "");
	    getchar();
	}
	fprintf(stderr,"\nBranches:\n");
	for (br = branches; br; br = br->next) {
	    fprintf(stderr, "%d: %s, end1 %d %d, end2 %d %d, %s\n",
		br->id,
		br->type == BR_STAIR ? "stair" :
		    br->type == BR_NO_END1 ? "no end1" :
		    br->type == BR_NO_END2 ? "no end2" :
		    br->type == BR_PORTAL  ? "portal"  :
					     "unknown",
		br->end1.dnum, br->end1.dlevel,
		br->end2.dnum, br->end2.dlevel,
		br->end1_up ? "end1 up" : "end1 down");
	}
	getchar();
	fprintf(stderr,"\nDone\n");
	getchar();
}
#endif

/* Save the dungeon structures. */
void
save_dungeon(fd)
    int fd;
{
    branch *curr;
    int    count;

    bwrite(fd, (genericptr_t) &n_dgns, sizeof(n_dgns));
    bwrite(fd, (genericptr_t) dungeons, sizeof(dungeon) * (unsigned)n_dgns);
    bwrite(fd, (genericptr_t) &dungeon_topology, sizeof dungeon_topology);
    bwrite(fd, (genericptr_t) tune, sizeof tune);

    for (count = 0, curr = branches; curr; curr = curr->next)
	count++;

    bwrite(fd, (genericptr_t) &count, sizeof(count));
    for (curr = branches; curr; curr = curr->next)
	bwrite(fd, (genericptr_t) curr, sizeof(branch));
}

/* Restore the dungeon structures. */
void
restore_dungeon(fd)
    int fd;
{
    branch *curr, *last;
    int    count, i;

    mread(fd, (genericptr_t) &n_dgns, sizeof(n_dgns));
    mread(fd, (genericptr_t) dungeons, sizeof(dungeon) * (unsigned)n_dgns);
    mread(fd, (genericptr_t) &dungeon_topology, sizeof dungeon_topology);
    mread(fd, (genericptr_t) tune, sizeof tune);

    last = branches = (branch *) 0;

    mread(fd, (genericptr_t) &count, sizeof(count));
    for (i = 0; i < count; i++) {
	curr = (branch *) alloc(sizeof(branch));
	mread(fd, (genericptr_t) curr, sizeof(branch));
	curr->next = (branch *) 0;
	if (last)
	    last->next = curr;
	else
	    branches = curr;
	last = curr;
    }
}

static void
Fread(ptr, size, nitems, stream)
	genericptr_t	ptr;
	int	size, nitems;
	FILE	*stream;
{
	int cnt;

	if((cnt = fread(ptr, size, nitems, stream)) != nitems) {

	    panic("PREMATURE EOF ON DUNGEON DESCRIPTION FILE!\nExpected %d bytes - got %d\n",
		    (size * nitems), (size * cnt));
	    terminate(1);
	}
}

static xchar
dname_to_dnum(s)
const char	*s;
{
	xchar	i;

	for (i = 0; i < n_dgns; i++)
	    if (!strcmp(dungeons[i].dname, s)) return i;

	panic("Couldn't resolve dungeon number for name \"%s\".", s);
	/*NOT REACHED*/
	return (xchar)0;
}

s_level *
find_level(s)
	const char *s;
{
	s_level *curr;
	for(curr = sp_levchn; curr; curr = curr->next)
	    if(!strcmp(s, curr->proto)) break;
	return curr;
}

/* Find the branch that links the named dungeon. */
static int
find_branch(s, pd)
	const char *s;		/* dungeon name */
	struct proto_dungeon *pd;
{
	int i;
	for (i = 0; i < pd->n_brs; i++)
	    if (!strcmp(pd->tmpbranch[i].name, s)) break;
	if (i == pd->n_brs) panic("find_branch: can't find %s", s);
	return i;
}


/*
 * Find the "parent" by searching the prototype branch list for the branch
 * listing, then figuring out to which dungeon it belongs.
 */
static xchar
parent_dnum(s, pd)
const char	   *s;	/* dungeon name */
struct proto_dungeon *pd;
{
	int	i;
	xchar	pdnum;

	i = find_branch(s, pd);
	/*
	 * Got branch, now find parent dungeon.  Stop if we have reached
	 * "this" dungeon (if we haven't found it by now it is an error).
	 */
	for (pdnum = 0; strcmp(pd->tmpdungeon[pdnum].name, s); pdnum++)
	    if ((i -= pd->tmpdungeon[pdnum].branches) < 0)
		return(pdnum);

	panic("parent_dnum: couldn't resolve branch.");
	/*NOT REACHED*/
	return (xchar)0;
}

/*
 * Return a starting point and number of successive positions a level
 * or dungeon entrance can occupy.
 *
 * Note: This follows the acouple (instead of the rcouple) rules for a
 *	 negative random component (rand < 0).  These rules are found
 *	 in dgn_comp.y.  The acouple [absolute couple] section says that
 *	 a negative random component means from the (adjusted) base to the
 *	 end of the dungeon.
 */
static int
level_range(dgn, base, rand, chain, pd, adjusted_base)
	xchar	dgn;
	int	base, rand, chain;
	struct proto_dungeon *pd;
	int *adjusted_base;
{
	int lmax = dungeons[dgn].num_dunlevs;

	if (chain >= 0) {		 /* relative to a special level */
	    s_level *levtmp = pd->final_lev[chain];
	    if (!levtmp) panic("level_range: empty chain level!");

	    base += levtmp->dlevel.dlevel;
	} else {			/* absolute in the dungeon */
	    /* from end of dungeon */
	    if (base < 0) base = (lmax + base + 1);
	}

	if (base < 1 || base > lmax)
	    panic("level_range: base value out of range");

	*adjusted_base = base;

	if (rand == -1) {	/* from base to end of dungeon */
	    return (lmax - base + 1);
	} else if (rand) {
	    /* make sure we don't run off the end of the dungeon */
	    return (((base + rand - 1) > lmax) ? lmax-base+1 : rand);
	} /* else only one choice */
	return 1;
}

static xchar
parent_dlevel(s, pd)
	const char	*s;
	struct proto_dungeon *pd;
{
	int i, num, base;

	i = find_branch(s, pd);
	num = level_range(parent_dnum(s, pd), pd->tmpbranch[i].lev.base,
					      pd->tmpbranch[i].lev.rand,
					      pd->tmpbranch[i].chain,
					      pd, &base);
	return (xchar) rn1(num,base);
}

/* Convert from the temporary branch type to the dungeon branch type. */
static int
correct_branch_type(tbr)
    struct tmpbranch *tbr;
{
    switch (tbr->type) {
	case TBR_STAIR:		return BR_STAIR;
	case TBR_NO_UP:		return tbr->up ? BR_NO_END1 : BR_NO_END2;
	case TBR_NO_DOWN:	return tbr->up ? BR_NO_END2 : BR_NO_END1;
	case TBR_PORTAL:	return BR_PORTAL;
    }
    impossible("correct_branch_type: unknown branch type");
    return BR_STAIR;
}

/*
 * Add the given branch to the branch list.  The branch list is ordered
 * by end1 dungeon and level followed by end2 dungeon and level.  If
 * extract_first is true, then the branch is already part of the list
 * but needs to be repositioned.
 */
void
insert_branch(new_branch, extract_first)
   branch *new_branch;
   boolean extract_first;
{
    branch *curr, *prev;
    long new_val, curr_val, prev_val;

    if (extract_first) {
	for (prev = 0, curr = branches; curr; prev = curr, curr = curr->next)
	    if (curr == new_branch) break;

	if (!curr) panic("insert_branch: not found");
	if (prev)
	    prev->next = curr->next;
	else
	    branches = curr->next;
    }
    new_branch->next = (branch *) 0;

/* Convert the branch into a unique number so we can sort them. */
#define branch_val(bp) ((((long)(bp)->end1.dnum * (MAXLEVEL+1) + (long)(bp)->end1.dlevel) * (MAXDUNGEON+1) * (MAXLEVEL+1)) + ((long)(bp)->end2.dnum * (MAXLEVEL+1) + (long)(bp)->end2.dlevel))

    /*
     * Insert the new branch into the correct place in the branch list.
     */
    prev = (branch *) 0;
    prev_val = -1;
    new_val = branch_val(new_branch);
    for (curr = branches; curr;
		    prev_val = curr_val, prev = curr, curr = curr->next) {
	curr_val = branch_val(curr);
	if (prev_val < new_val && new_val <= curr_val) break;
    }
    if (prev) {
	new_branch->next = curr;
	prev->next = new_branch;
    } else {
	new_branch->next = branches;
	branches = new_branch;
    }
}

/* Add a dungeon branch to the branch list. */
static branch *
add_branch(dgn, child_entry_level, pd)
    int dgn;
    int child_entry_level;
    struct proto_dungeon *pd;
{
    static int branch_id = 0;
    int branch_num;
    branch *new_branch;

    branch_num = find_branch(dungeons[dgn].dname,pd);
    new_branch = (branch *) alloc(sizeof(branch));
    new_branch->next = (branch *) 0;
    new_branch->id = branch_id++;
    new_branch->type = correct_branch_type(&pd->tmpbranch[branch_num]);
    new_branch->end1.dnum = parent_dnum(dungeons[dgn].dname, pd);
    new_branch->end1.dlevel = parent_dlevel(dungeons[dgn].dname, pd);
    new_branch->end2.dnum = dgn;
    new_branch->end2.dlevel = child_entry_level;
    new_branch->end1_up = pd->tmpbranch[branch_num].up ? TRUE : FALSE;

    insert_branch(new_branch, FALSE);
    return new_branch;
}

/*
 * Add new level to special level chain.  Insert it in level order with the
 * other levels in this dungeon.  This assumes that we are never given a
 * level that has a dungeon number less than the dungeon number of the
 * last entry.
 */
static void
add_level(new_lev)
    s_level *new_lev;
{
	s_level *prev, *curr;

	prev = (s_level *) 0;
	for (curr = sp_levchn; curr; curr = curr->next) {
	    if (curr->dlevel.dnum == new_lev->dlevel.dnum &&
		    curr->dlevel.dlevel > new_lev->dlevel.dlevel)
		break;
	    prev = curr;
	}
	if (!prev) {
	    new_lev->next = sp_levchn;
	    sp_levchn = new_lev;
	} else {
	    new_lev->next = curr;
	    prev->next = new_lev;
	}
}

static void
init_level(dgn, proto_index, pd)
	int dgn, proto_index;
	struct proto_dungeon *pd;
{
	s_level	*new_level;
	struct tmplevel *tlevel = &pd->tmplevel[proto_index];

	pd->final_lev[proto_index] = (s_level *) 0; /* no "real" level */
#ifdef WIZARD
	if (!wizard)
#endif
	    if (tlevel->chance <= rn2(100)) return;

	pd->final_lev[proto_index] = new_level =
					(s_level *) alloc(sizeof(s_level));
	/* load new level with data */
	Strcpy(new_level->proto, tlevel->name);
	new_level->boneid = tlevel->boneschar;
	new_level->dlevel.dnum = dgn;
	new_level->dlevel.dlevel = 0;	/* for now */

	new_level->flags.town = !!(tlevel->flags & TOWN);
	new_level->flags.hellish = !!(tlevel->flags & HELLISH);
	new_level->flags.maze_like = !!(tlevel->flags & MAZELIKE);
	new_level->flags.rogue_like = !!(tlevel->flags & ROGUELIKE);
	new_level->flags.align = ((tlevel->flags & D_ALIGN_MASK) >> 4);

	new_level->rndlevs = tlevel->rndlevs;
	new_level->next    = (s_level *) 0;
}

static int
possible_places(idx, map, pd)
    int idx;		/* prototype index */
    boolean *map;	/* array MAXLEVEL+1 in length */
    struct proto_dungeon *pd;
{
    int i, start, count;
    s_level *lev = pd->final_lev[idx];

    /* init level possibilities */
    for (i = 0; i <= MAXLEVEL; i++) map[i] = FALSE;

    /* get base and range and set those entried to true */
    count = level_range(lev->dlevel.dnum, pd->tmplevel[idx].lev.base,
					pd->tmplevel[idx].lev.rand,
					pd->tmplevel[idx].chain,
					pd, &start);
    for (i = start; i < start+count; i++)
	map[i] = TRUE;

    /* mark off already placed levels */
    for (i = pd->start; i < idx; i++) {
	if (pd->final_lev[i] && map[pd->final_lev[i]->dlevel.dlevel]) {
	    map[pd->final_lev[i]->dlevel.dlevel] = FALSE;
	    --count;
	}
    }

    return count;
}

/* Pick the nth TRUE entry in the given boolean array. */
static xchar
pick_level(map, nth)
    boolean *map;	/* an array MAXLEVEL+1 in size */
    int nth;
{
    int i;
    for (i = 1; i <= MAXLEVEL; i++)
	if (map[i] && !nth--) return (xchar) i;
    panic("pick_level:  ran out of valid levels");
    return 0;
}

#ifdef DDEBUG
static void FDECL(indent,(int));

static void
indent(d)
int d;
{
    while (d-- > 0) fputs("    ", stderr);
}
#endif

/*
 * Place a level.  First, find the possible places on a dungeon map
 * template.  Next pick one.  Then try to place the next level.  If
 * sucessful, we're done.  Otherwise, try another (and another) until
 * all possible places have been tried.  If all possible places have
 * been exausted, return false.
 */
static boolean
place_level(proto_index, pd)
    int proto_index;
    struct proto_dungeon *pd;
{
    boolean map[MAXLEVEL+1];	/* valid levels are 1..MAXLEVEL inclusive */
    s_level *lev;
    int npossible;
#ifdef DDEBUG
    int i;
#endif

    if (proto_index == pd->n_levs) return TRUE;	/* at end of proto levels */

    lev = pd->final_lev[proto_index];

    /* No level created for this prototype, goto next. */
    if (!lev) return place_level(proto_index+1, pd);

    npossible = possible_places(proto_index, map, pd);

    for (; npossible; --npossible) {
	lev->dlevel.dlevel = pick_level(map, rn2(npossible));
#ifdef DDEBUG
	indent(proto_index-pd->start);
	fprintf(stderr,"%s: trying %d [ ", lev->proto, lev->dlevel.dlevel);
	for (i = 1; i <= MAXLEVEL; i++)
	    if (map[i]) fprintf(stderr,"%d ", i);
	fprintf(stderr,"]\n");
#endif
	if (place_level(proto_index+1, pd)) return TRUE;
	map[lev->dlevel.dlevel] = FALSE;	/* this choice didn't work */
    }
#ifdef DDEBUG
    indent(proto_index-pd->start);
    fprintf(stderr,"%s: failed\n", lev->proto);
#endif
    return FALSE;
}


struct level_map {
	const char *lev_name;
	d_level *lev_spec;
} level_map[] = {
	{ "air",	&air_level },
	{ "asmodeus",	&asmodeus_level },
	{ "astral",	&astral_level },
	{ "baalz",	&baalzebub_level },
	{ "bigroom",	&bigroom_level },
	{ "castle",	&stronghold_level },
	{ "earth",	&earth_level },
	{ "fakewiz1",	&portal_level },
	{ "fire",	&fire_level },
	{ "juiblex",	&juiblex_level },
#ifdef MULDGN
	{ "knox",	&knox_level },
#endif
	{ "medusa",	&medusa_level },
	{ "oracle",	&oracle_level },
	{ "orcus",	&orcus_level },
#ifdef REINCARNATION
	{ "rogue",	&rogue_level },
#endif
	{ "sanctum",	&sanctum_level },
	{ "valley",	&valley_level },
	{ "water",	&water_level },
	{ "wizard1",	&wiz1_level },
	{ "wizard2",	&wiz2_level },
	{ "wizard3",	&wiz3_level },
#ifdef MULDGN
	{ X_START,	&qstart_level },
	{ X_LOCATE,	&qlocate_level },
	{ X_GOAL,	&nemesis_level },
#endif
	{ "",		(d_level *)0 }
};

void
init_dungeons()		/* initialize the "dungeon" structs */
{
	FILE	*dgn_file;
	register int i, cl = 0, cb = 0;
	register s_level *x;
	struct proto_dungeon pd;
	struct level_map *lev_map;

	pd.n_levs = pd.n_brs = 0;

	dgn_file = fopen_datafile(DUNGEON_FILE, RDMODE);
	if (!dgn_file)
	    panic("\rCANNOT OPEN DUNGEON DESCRIPTION FILE %s.", DUNGEON_FILE);

	/*
	 * Read in each dungeon and transfer the results to the internal
	 * dungeon arrays.
	 */
	sp_levchn = (s_level *) 0;
	Fread((genericptr_t)&n_dgns, sizeof(int), 1, dgn_file);
	if (n_dgns >= MAXDUNGEON)
	    panic("init_dungeons: too many dungeons");

	for (i = 0; i < n_dgns; i++) {
	    Fread((genericptr_t)&pd.tmpdungeon[i],
				    sizeof(struct tmpdungeon), 1, dgn_file);
#ifdef WIZARD
	    if(!wizard)
#endif
	      if(pd.tmpdungeon[i].chance && (pd.tmpdungeon[i].chance <= rn2(100))) {
		int j;

		/* skip over any levels or branches */
		for(j = 0; j < pd.tmpdungeon[i].levels; j++)
		    Fread((genericptr_t)&pd.tmplevel[cl], sizeof(struct tmplevel),
							1, dgn_file);

		for(j = 0; j < pd.tmpdungeon[i].branches; j++)
		    Fread((genericptr_t)&pd.tmpbranch[cb],
					sizeof(struct tmpbranch), 1, dgn_file);
		n_dgns--; i--;
		continue;
	      }

	    Strcpy(dungeons[i].dname, pd.tmpdungeon[i].name);
	    Strcpy(dungeons[i].proto, pd.tmpdungeon[i].protoname);
	    dungeons[i].boneid = pd.tmpdungeon[i].boneschar;

	    if(pd.tmpdungeon[i].lev.rand)
		dungeons[i].num_dunlevs = rn1(pd.tmpdungeon[i].lev.rand,
					      pd.tmpdungeon[i].lev.base);
	    else dungeons[i].num_dunlevs = pd.tmpdungeon[i].lev.base;

	    if(!i) {
		dungeons[i].ledger_start = 0;
		dungeons[i].depth_start = 1;
		dungeons[i].dunlev_ureached = 1;
	    } else {
		dungeons[i].ledger_start = dungeons[i-1].ledger_start +
					      dungeons[i-1].num_dunlevs;
		dungeons[i].dunlev_ureached = 0;
	    }

	    dungeons[i].flags.hellish = !!(pd.tmpdungeon[i].flags & HELLISH);
	    dungeons[i].flags.maze_like = !!(pd.tmpdungeon[i].flags & MAZELIKE);
	    dungeons[i].flags.rogue_like = !!(pd.tmpdungeon[i].flags & ROGUELIKE);
	    dungeons[i].flags.align = ((pd.tmpdungeon[i].flags & D_ALIGN_MASK) >> 4);
	    /*
	     * Set the entry level for this dungeon.  The pd.tmpdungeon entry
	     * value means:
	     *		< 0	from bottom (-1 == bottom level)
	     *		  0	default (top)
	     *		> 0	actual level (1 = top)
	     *
	     * Note that the entry_lev field in the dungeon structure is
	     * redundant.  It is used only here and in print_dungeon().
	     */
	    if (pd.tmpdungeon[i].entry_lev < 0) {
		dungeons[i].entry_lev = dungeons[i].num_dunlevs +
						pd.tmpdungeon[i].entry_lev + 1;
		if (dungeons[i].entry_lev <= 0) dungeons[i].entry_lev = 1;
	    } else if (pd.tmpdungeon[i].entry_lev > 0) {
		dungeons[i].entry_lev = pd.tmpdungeon[i].entry_lev;
		if (dungeons[i].entry_lev > dungeons[i].num_dunlevs)
		    dungeons[i].entry_lev = pd.tmpdungeon[i].entry_lev;
	    } else { /* default */
		dungeons[i].entry_lev = 1;	/* defaults to top level */
	    }

	    if (i) {	/* set depth */
		branch *br;
		schar from_depth;
		boolean from_up;

		br = add_branch(i, dungeons[i].entry_lev, &pd);

		/* Get the depth of the connecting end. */
		if (br->end1.dnum == i) {
		    from_depth = depth(&br->end2);
		    from_up = !br->end1_up;
		} else {
		    from_depth = depth(&br->end1);
		    from_up = br->end1_up;
		}

		/*
		 * Calculate the depth of the top of the dungeon via
		 * its branch.  First, the depth of the entry point:
		 *
		 *	depth of branch from "parent" dungeon
		 *	+ -1 or 1 depending on a up or down stair or
		 *	  0 if portal
		 *
		 * Followed by the depth of the top of the dungeon:
		 *
		 *	- (entry depth - 1)
		 *
		 * We'll say that portals stay on the same depth.
		 */
		dungeons[i].depth_start = from_depth
					+ (br->type == BR_PORTAL ? 0 :
							(from_up ? -1 : 1))
					- (dungeons[i].entry_lev - 1);
	    }

	    /* this is redundant - it should have been flagged by dgn_comp */
	    if(dungeons[i].num_dunlevs > MAXLEVEL)
		dungeons[i].num_dunlevs = MAXLEVEL;

	    pd.start = pd.n_levs;	/* save starting point */
	    pd.n_levs += pd.tmpdungeon[i].levels;
	    if (pd.n_levs > LEV_LIMIT)
		panic("init_dungeon: too many special levels");
	    /*
	     * Read in the prototype special levels.  Don't add generated
	     * special levels until they are all placed.
	     */
	    for(; cl < pd.n_levs; cl++) {
		Fread((genericptr_t)&pd.tmplevel[cl],
					sizeof(struct tmplevel), 1, dgn_file);
		init_level(i, cl, &pd);
	    }
	    /*
	     * Recursively place the generated levels for this dungeon.  This
	     * routine will attempt all possible combinations before giving
	     * up.
	     */
	    if (!place_level(pd.start, &pd))
		panic("init_dungeon:  couldn't place levels");
#ifdef DDEBUG
	    fprintf(stderr, "--- end of dungeon %d ---\n", i);
	    fflush(stderr);
	    getchar();
#endif
	    for (; pd.start < pd.n_levs; pd.start++)
		if (pd.final_lev[pd.start]) add_level(pd.final_lev[pd.start]);


	    pd.n_brs += pd.tmpdungeon[i].branches;
	    if (pd.n_brs > BRANCH_LIMIT)
		panic("init_dungeon: too many branches");
	    for(; cb < pd.n_brs; cb++)
		Fread((genericptr_t)&pd.tmpbranch[cb],
					sizeof(struct tmpbranch), 1, dgn_file);
	}
	(void) fclose(dgn_file);

	for (i = 0; i < 5; i++) tune[i] = 'A' + rn2(7);
	tune[5] = 0;

	/*
	 * Find most of the special levels and dungeons so we can access their
	 * locations quickly.
	 */
	for (lev_map = level_map; lev_map->lev_name[0]; lev_map++) {
		x = find_level(lev_map->lev_name);
		if (x) {
			assign_level(lev_map->lev_spec, &x->dlevel);
#ifdef MULDGN
			if (!strncmp(lev_map->lev_name, "x-", 2)) {
				/* This is where the name substitution on the
				 * levels of the quest dungeon occur.
				 */
				x->proto[0] = pl_character[0];
			} else if (lev_map->lev_spec == &knox_level) {
				branch *br;
				/*
				 * Kludge to allow floating Knox entrance.  We
				 * specify a floating entrance by the fact that
				 * its entrance (end1) has a bogus dnum, namely
				 * n_dgns.
				 */
				for (br = branches; br; br = br->next)
				    if (on_level(&br->end2, &knox_level)) break;

				if (br) br->end1.dnum = n_dgns;
				/* adjust the branch's position on the list */
				insert_branch(br, TRUE);
			}
#endif
		}
	}
#ifdef MULDGN
/*
 *	I hate hardwiring these names. :-(
 */
	quest_dnum = dname_to_dnum("The Quest");
	mines_dnum = dname_to_dnum("The Gnomish Mines");
#endif
	tower_dnum = dname_to_dnum("Vlad's Tower");

#ifdef DEBUG
	dumpit();
#endif
}

xchar
dunlev(lev)	/* return the level number for lev in *this* dungeon */
d_level	*lev;
{
	return(lev->dlevel);
}

xchar
dunlevs_in_dungeon(lev)	/* return the lowest level number for *this* dungeon*/
d_level	*lev;
{
	return(dungeons[lev->dnum].num_dunlevs);
}

xchar
deepest_lev_reached(noquest) /* return the lowest level explored in the game*/
boolean noquest;
{
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
	register int i;
	d_level tmp;
	register schar ret = 0;

	for(i = 0; i < n_dgns; i++) {
	    if((tmp.dlevel = dungeons[i].dunlev_ureached) == 0) continue;
	    if(!strcmp(dungeons[i].dname, "The Quest") && noquest) continue;

	    tmp.dnum = i;
	    if(depth(&tmp) > ret) ret = depth(&tmp);
	}
	return((xchar) ret);
}

/* return a bookkeeping level number for purpose of comparisons and
 * save/restore */
xchar
ledger_no(lev)
d_level	*lev;
{
	return((xchar)(lev->dlevel + dungeons[lev->dnum].ledger_start));
}

/*
 * The last level in the bookkeeping list of level is the bottom of the last
 * dungeon in the dungeons[] array.
 *
 * Maxledgerno() -- which is the max number of levels in the bookkeeping
 * list, should not be confused with dunlevs_in_dungeon(lev) -- which
 * returns the max number of levels in lev's dungeon, and both should
 * not be confused with deepest_lev_reached() -- which returns the lowest
 * depth visited by the player.
 */
xchar
maxledgerno()
{
    return (xchar) (dungeons[n_dgns-1].ledger_start +
				dungeons[n_dgns-1].num_dunlevs);
}

/* return the dungeon that this ledgerno exists in */
xchar
ledger_to_dnum(ledgerno)
xchar	ledgerno;
{
	register int i;

	/* find i such that (i->base + 1) <= ledgerno <= (i->base + i->count) */
	for (i = 0; i < n_dgns; i++)
	    if (dungeons[i].ledger_start < ledgerno &&
		ledgerno <= dungeons[i].ledger_start + dungeons[i].num_dunlevs)
		return (xchar)i;

	panic("level number out of range [ledger_to_dnum(%d)]", (int)ledgerno);
	/*NOT REACHED*/
	return (xchar)0;
}

/* return the level of the dungeon this ledgerno exists in */
xchar
ledger_to_dlev(ledgerno)
xchar	ledgerno;
{
	return((xchar)(ledgerno - dungeons[ledger_to_dnum(ledgerno)].ledger_start));
}

#endif /* OVL1 */
#ifdef OVL0

/* returns the depth of a level, in floors below the surface	*/
/* (note levels in different dungeons can have the same depth).	*/
schar
depth(lev)
d_level	*lev;
{
	return((schar)( dungeons[lev->dnum].depth_start + lev->dlevel - 1));
}

boolean
on_level(lev1, lev2)	/* are "lev1" and "lev2" actually the same? */
d_level	*lev1, *lev2;
{
	return((boolean)((lev1->dnum == lev2->dnum) && (lev1->dlevel == lev2->dlevel)));
}

#endif /* OVL0 */
#ifdef OVL1

s_level *
/* is this level referenced in the special level chain? */
Is_special(lev)
d_level	*lev;
{
	s_level *levtmp;

	for (levtmp = sp_levchn; levtmp; levtmp = levtmp->next)
	    if (on_level(lev, &levtmp->dlevel)) return(levtmp);

	return((s_level *)0);
}

/*
 * Is this a multi-dungeon branch level?  If so, return a pointer to the
 * branch.  Otherwise, return NULL.
 */
branch *
Is_branchlev(lev)
	d_level	*lev;
{
	branch *curr;

	for (curr = branches; curr; curr = curr->next) {
	    if (on_level(lev, &curr->end1) || on_level(lev, &curr->end2))
		return curr;
	}
	return (branch *) 0;
}

/* goto the next level (or appropriate dungeon) */
void
next_level(at_stairs )
boolean	at_stairs;
{
	if (at_stairs && u.ux == sstairs.sx && u.uy == sstairs.sy) {
		/* Taking a down dungeon branch. */
		goto_level(&sstairs.tolev, at_stairs, FALSE, FALSE);
	} else {
		/* Going down a stairs or jump in a trap door. */
		d_level	newlevel;

		newlevel.dnum = u.uz.dnum;
		newlevel.dlevel = u.uz.dlevel + 1;
		goto_level(&newlevel, at_stairs, !at_stairs, FALSE);
	}
}

/* goto the previous level (or appropriate dungeon) */
void
prev_level(at_stairs)
boolean	at_stairs;
{
	if (at_stairs && u.ux == sstairs.sx && u.uy == sstairs.sy) {
		/* Taking an up dungeon branch. */
		if(!u.uz.dnum  && !u.uhave.amulet) done(ESCAPED);
		else goto_level(&sstairs.tolev, at_stairs, FALSE, FALSE);
	} else {
		/* Going up a stairs or rising through the ceiling. */
		d_level	newlevel;
		newlevel.dnum = u.uz.dnum;
		newlevel.dlevel = u.uz.dlevel - 1;
		goto_level(&newlevel, at_stairs, FALSE, FALSE);
	}
}

void
u_on_sstairs() {	/* place you on the special staircase */

	if (sstairs.sx && sstairs.sy) {
	    u.ux = sstairs.sx;
	    u.uy = sstairs.sy;
	} else {
	    /* code stolen from goto_level */
	    int try = 0;
#ifdef DEBUG
	    pline("u_on_sstairs: picking random spot");
#endif
#define badspot(x,y) ((levl[x][y].typ != ROOM && levl[x][y].typ != CORR) || MON_AT(x, y))
	    do {
		u.ux = rnd(COLNO-1);
		u.uy = rn2(ROWNO);
	    } while(try++ < 100 && badspot(u.ux, u.uy));
	    if (try >= 100)
		panic("u_on_sstairs: could not relocate player!");
#undef badspot
	}
}

void
u_on_upstairs()	/* place you on upstairs (or special equivalent) */
{
	if(xupstair && yupstair)  {

		u.ux = xupstair;
		u.uy = yupstair;
	}
	else u_on_sstairs();
}

void
u_on_dnstairs()	/* place you on dnstairs (or special equivalent) */
{
	if(xdnstair && ydnstair)  {

		u.ux = xdnstair;
		u.uy = ydnstair;
	}
	else u_on_sstairs();
}

boolean
On_stairs(x, y)
xchar x, y;
{
	return((boolean)((x == xupstair && y == yupstair) ||
	       (x == xdnstair && y == ydnstair) ||
	       (x == xdnladder && y == ydnladder) ||
	       (x == xupladder && y == yupladder) ||
	       (x == sstairs.sx && y == sstairs.sy)));
}

boolean
Is_botlevel(lev)
d_level *lev;
{
	return((boolean)(lev->dlevel == dungeons[lev->dnum].num_dunlevs));
}

boolean
Can_dig_down(lev)
d_level *lev;
{
	return((boolean)(!level.flags.hardfloor
	    && !Is_botlevel(lev) && !Invocation_lev(lev)));
}

/*
 * Like Can_dig_down (above), but also allows falling through on the
 * stronghold level.  Normally, the bottom level of a dungeon resists
 * both digging and falling.
 */
boolean
Can_fall_thru(lev)
d_level *lev;
{
	return((boolean)(Can_dig_down(lev) || Is_stronghold(lev)));
}

/*
 * True if one can rise up a level (e.g. cursed gain level).
 * This happens on intermediate dungeon levels or on any top dungeon
 * level that has a stairwell style branch to the next higher dungeon.
 * Checks for amulets and such must be done elsewhere.
 */
boolean
Can_rise_up(lev)
d_level *lev;
{
    return((boolean)(!In_endgame(lev) &&
	(lev->dlevel > 1 ||
	 (dungeons[lev->dnum].entry_lev == 1 && ledger_no(lev) != 1 &&
	  sstairs.sx && sstairs.up))));
}

/*
 * It is expected that the second argument of get_level is a depth value,
 * either supplied by the user (teleport control) or randomly generated.
 * But more than one level can be at the same depth.  If the target level
 * is "above" the present depth location, get_level must trace "up" from
 * the player's location (through the ancestors dungeons) the dungeon
 * within which the target level is located.  With only one exception
 * which does not pass through this routine (see level_tele), teleporting
 * "down" is confined to the current dungeon.  At present, level teleport
 * in dungeons that build up is confined within them.
 */
void
get_level(newlevel, levnum)
d_level *newlevel;
int levnum;
{
	branch *br;
	xchar dgn = u.uz.dnum;

	if (levnum <= 0) {
	    /* can only currently happen in endgame */
	    levnum = u.uz.dlevel;
	} else if (levnum > dungeons[dgn].depth_start
			    + dungeons[dgn].num_dunlevs - 1) {
	    /* beyond end of dungeon, jump to last level */
	    levnum = dungeons[dgn].num_dunlevs;
	} else {
	    /* The desired level is in this dungeon or a "higher" one. */

	    /*
	     * Branch up the tree until we reach a dungeon that contains the
	     * levnum.
	     */
	    if (levnum < dungeons[dgn].depth_start) {

		do {
		    /*
		     * Find the parent dungeon of this dungeon.
		     *
		     * This assumes that end2 is always the "child" and it is
		     * unique.
		     */
		    for (br = branches; br; br = br->next)
			if (br->end2.dnum == dgn) break;
		    if (!br)
			panic("get_level: can't find parent dungeon");

		    dgn = br->end1.dnum;
		} while (levnum < dungeons[dgn].depth_start);
	    }

	    /* We're within the same dungeon; calculate the level. */
	    levnum = levnum - dungeons[dgn].depth_start + 1;
	}

	newlevel->dnum = dgn;
	newlevel->dlevel = levnum;
}

#endif /* OVL1 */
#ifdef OVL0

#ifdef MULDGN
boolean
In_quest(lev)	/* are you in the quest dungeon? */
d_level *lev;
{
	return((boolean)(lev->dnum == quest_dnum));
}
#endif /* MULDGN */

#endif /* OVL0 */
#ifdef OVL1

#ifdef MULDGN
boolean
In_mines(lev)	/* are you in the mines dungeon? */
d_level	*lev;
{
	return((boolean)(lev->dnum == mines_dnum));
}

/*
 * Return the branch for the given dungeon.
 *
 * This function assumes:
 *	+ This is not called with "Dungeons of Doom".
 *	+ There is only _one_ branch to a given dungeon.
 *	+ Field end2 is the "child" dungeon.
 */
branch *
dungeon_branch(s)
    const char *s;
{
    branch *br;
    xchar  dnum;

    dnum = dname_to_dnum(s);

    /* Find the branch that connects to dungeon i's branch. */
    for (br = branches; br; br = br->next)
	if (br->end2.dnum == dnum) break;

    if (!br) panic("dgn_entrance: can't find entrance to %s", s);

    return br;
}

/*
 * This returns true if the hero is on the same level as the entrance to
 * the named dungeon.
 *
 * Called from do.c and mklev.c.
 *
 * Assumes that end1 is always the "parent".
 */
boolean
at_dgn_entrance(s)
    const char *s;
{
    branch *br;

    br = dungeon_branch(s);
    return((boolean)(on_level(&u.uz, &br->end1) ? TRUE : FALSE));
}
#endif /* MULDGN */

boolean
In_tower(lev)	/* are you inside the tower? */
d_level	*lev;
{
	return((boolean)(lev->dnum == tower_dnum));
}

#endif /* OVL1 */
#ifdef OVL0

boolean
In_hell(lev)	/* are you in one of the Hell levels? */
d_level	*lev;
{
	return((boolean)(dungeons[lev->dnum].flags.hellish));
}

#endif /* OVL0 */
#ifdef OVL1

void
goto_hell(at_stairs, falling)	/* go directly to hell... */
boolean	at_stairs, falling;
{
	d_level lev;

	lev.dnum = wiz1_level.dnum;
	lev.dlevel = 1;
	goto_level(&lev, at_stairs, falling, FALSE);
}

void
assign_level(dest, src)		/* equivalent to dest = source */
d_level	*dest, *src;
{
	dest->dnum = src->dnum;
	dest->dlevel = src->dlevel;
}

void
assign_rnd_level(dest, src, range)	/* dest = src + rn1(range) */
d_level	*dest, *src;
int range;
{
	dest->dnum = src->dnum;
	dest->dlevel = src->dlevel + ((range > 0) ? rnd(range) : -rnd(-range)) ;

	if(dest->dlevel > dunlevs_in_dungeon(dest))
		dest->dlevel = dunlevs_in_dungeon(dest);
	else if(dest->dlevel < 1)
		dest->dlevel = 1;
}

#endif /* OVL1 */
#ifdef OVL0

int
induced_align(pct)
int	pct;
{
	s_level	*lev = Is_special(&u.uz);
	aligntyp al;

	if (lev && lev->flags.align)
		if(rn2(100) < pct) return(lev->flags.align);

	if(dungeons[u.uz.dnum].flags.align)
		if(rn2(100) < pct) return(dungeons[u.uz.dnum].flags.align);

	al = rn2(3) - 1;
	return(Align2amask(al));
}

#endif /* OVL0 */
#ifdef OVL1

boolean
Invocation_lev(lev)
d_level *lev;
{
	return((boolean)(In_hell(lev) &&
		lev->dlevel == (dungeons[lev->dnum].num_dunlevs - 1)));
}

/* use instead of depth() wherever a degree of difficulty is made
 * dependent on the location in the dungeon (eg. monster creation).
 */
xchar
level_difficulty()
{
	if (In_endgame(&u.uz))
		return((xchar)(depth(&sanctum_level) + u.ulevel/2));
	else
		if (u.uhave.amulet)
			return(deepest_lev_reached(FALSE));
		else
			return((xchar) depth(&u.uz));
}


#ifdef WIZARD

/* Convert a branch type to a string usable by print_dungeon(). */
static const char *
br_string(type)
    int type;
{
    switch (type) {
	case BR_PORTAL:	 return "Portal";
	case BR_NO_END1: return "Connection";
	case BR_NO_END2: return "One way stair";
	case BR_STAIR:	 return "Stair";
    }
    return " (unknown)";
}

/* Print all child branches between the lower and upper bounds. */
static void
print_branch(win, dnum, lower_bound, upper_bound)
    winid win;
    int   dnum;
    int   lower_bound;
    int   upper_bound;
{
    branch *br;
    char buf[BUFSZ];

    /* This assumes that end1 is the "parent". */
    for (br = branches; br; br = br->next) {
	if (br->end1.dnum == dnum && lower_bound < br->end1.dlevel &&
					br->end1.dlevel <= upper_bound) {
	    Sprintf(buf,"   %s to %s: %d",
		    br_string(br->type),
		    dungeons[br->end2.dnum].dname,
		    depth(&br->end1));
	    putstr(win, 0, buf);
	}
    }
}

/* Print available dungeon information. */
void
print_dungeon()
{
    int     i, last_level, nlev;
    char    buf[BUFSZ];
    boolean first;
    s_level *slev;
    dungeon *dptr;
    branch  *br;
    winid   win = create_nhwindow(NHW_MENU);

    for (i = 0, dptr = dungeons; i < n_dgns; i++, dptr++) {
	nlev = dptr->num_dunlevs;
	if (nlev > 1)
	    Sprintf(buf, "%s: levels %d to %d", dptr->dname, dptr->depth_start,
						dptr->depth_start + nlev - 1);
	else
	    Sprintf(buf, "%s: level %d", dptr->dname, dptr->depth_start);

	/* Most entrances are uninteresting. */
	if (dptr->entry_lev != 1) {
	    if (dptr->entry_lev == nlev)
		Strcat(buf, ", entrance from below");
	    else
		Sprintf(eos(buf), ", entrance on %d",
			dptr->depth_start + dptr->entry_lev - 1);
	}
	putstr(win, 0, buf);

	/*
	 * Circle through the special levels to find levels that are in
	 * this dungeon.
	 */
	for (slev = sp_levchn, last_level = 0; slev; slev = slev->next) {
	    if (slev->dlevel.dnum != i) continue;

	    /* print any branches before this level */
	    print_branch(win, i, last_level, slev->dlevel.dlevel);

	    Sprintf(buf, "   %s: %d", slev->proto, depth(&slev->dlevel));
	    if (Is_stronghold(&slev->dlevel))
		Sprintf(eos(buf), " (tune %s)", tune);
	    putstr(win, 0, buf);

	    last_level = slev->dlevel.dlevel;
	}
	/* print branches after the last special level */
	print_branch(win, i, last_level, MAXLEVEL);
    }

    /* Print out floating branches (if any). */
    for (first = TRUE, br = branches; br; br = br->next) {
	if (br->end1.dnum == n_dgns) {
	    if (first) {
		putstr(win, 0, "");
		putstr(win, 0, "Floating branches");
		first = FALSE;
	    }
	    Sprintf(buf, "   %s to %s",
			br_string(br->type), dungeons[br->end2.dnum].dname);
	    putstr(win, 0, buf);
	}
    }

    /* I hate searching for the invocation pos while debugging. -dean */
    if (Invocation_lev(&u.uz)) {
	putstr(win, 0, "");
	Sprintf(buf, "Invocation position @ (%d,%d), hero @ (%d,%d)",
		inv_pos.x, inv_pos.y, u.ux, u.uy);
	putstr(win, 0, buf);
    }
    /*
     * The following is based on the assumption that the inter-level portals
     * created by the level compiler (not the dungeon compiler) only exist
     * one per level (currently true, of course).
     */
    else if (Is_earthlevel(&u.uz) || Is_waterlevel(&u.uz)
				|| Is_firelevel(&u.uz) || Is_airlevel(&u.uz)) {
	struct trap *trap;
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if (trap->ttyp == MAGIC_PORTAL) break;

	putstr(win, 0, "");
	if (trap)
	    Sprintf(buf, "Portal @ (%d,%d), hero @ (%d,%d)",
		trap->tx, trap->ty, u.ux, u.uy);
	else
	    Sprintf(buf, "No portal found.");
	putstr(win, 0, buf);
    }

    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
}

#endif /* WIZARD */
#endif /* OVL1 */

/*dungeon.c*/
