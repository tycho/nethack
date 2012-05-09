/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NitroHack may be freely redistributed.  See license for details. */

/*
 * This file contains the various functions that are related to the special
 * levels.
 * It contains also the special level loader.
 *
 */

#include "hack.h"
#include "dlb.h"

#include "sp_lev.h"
#include "rect.h"
#include "epri.h"
#include "eshk.h"

static void get_room_loc(struct level *lev, schar *, schar *, struct mkroom *);
static void get_free_room_loc(struct level *lev, schar *x, schar *y,
			      struct mkroom *croom);
static void create_trap(struct level *lev, trap *t, struct mkroom *croom);
static int noncoalignment(aligntyp);
static void create_monster(struct level *lev, monster *, struct mkroom *);
static void create_object(struct level *lev, object *, struct mkroom *);
static void create_engraving(struct level *lev, engraving *,struct mkroom *);
static void create_stairs(struct level *lev, stair *, struct mkroom *);
static void create_altar(struct level *lev, altar *, struct mkroom *);
static void create_gold(struct level *lev, gold *, struct mkroom *);
static void create_feature(struct level *lev, int,int,struct mkroom *,int);
static boolean search_door(struct mkroom *, xchar *, xchar *,
					xchar, int);
static void fix_stair_rooms(struct level *lev);
static void create_corridor(struct level *lev, corridor *);
static void count_features(struct level *lev);

static boolean create_subroom(struct level *lev, struct mkroom *, xchar, xchar,
					xchar, xchar, xchar, xchar);

#define LEFT	1
#define H_LEFT	2
#define CENTER	3
#define H_RIGHT	4
#define RIGHT	5

#define TOP	1
#define BOTTOM	5

#define XLIM	4
#define YLIM	3

#define Fread(ptr, size, count, stream)  if (dlb_fread(ptr,size,count,stream) != count) goto err_out;
#define Fgetc	(schar)dlb_fgetc
#define Free(ptr)		if (ptr) free((ptr))

extern int min_rx, max_rx, min_ry, max_ry; /* from mkmap.c */

char SpLev_Map[COLNO][ROWNO];
static char robjects[MAX_REGISTERS], rmonst[MAX_REGISTERS];
static char rloc_x[MAX_REGISTERS], rloc_y[MAX_REGISTERS];
static const aligntyp init_ralign[3] = { AM_CHAOTIC, AM_NEUTRAL, AM_LAWFUL };
static aligntyp ralign[3];
static xchar xstart, ystart;
static char xsize, ysize;
static int n_rloc, n_robj, n_rmon; /* # or random registers */

static void set_wall_property(struct level *lev, xchar,xchar,xchar,xchar,int);
static int rnddoor(void);
static int rndtrap(struct level *lev);
static void get_location(struct level *lev, schar *x, schar *y, int humidity, struct mkroom *croom);
static boolean is_ok_location(struct level *lev, schar, schar, int);
static void sp_lev_shuffle(char *,char *,int);
static void light_region(struct level *lev, region *tmpregion);
static void load_one_monster(dlb *,monster *);
static void load_one_object(dlb *,object *);
static void load_one_engraving(dlb *,engraving *);
static void maze1xy(struct level *lev, coord *m, int humidity);
static boolean sp_level_loader(struct level *lev, dlb *fp, sp_lev *lvl);
static void create_door(struct level *lev, room_door *, struct mkroom *);
static struct mkroom *build_room(struct level *lev, room *, struct mkroom *);

char *lev_message = 0;
lev_region *lregions = 0;
int num_lregions = 0;

struct obj *container_obj[MAX_CONTAINMENT];
int container_idx = 0;


static void lvlfill_maze_grid(struct level *lev, int x1, int y1, int x2, int y2,
			      schar filling)
{
	int x, y;

	for (x = x1; x <= x2; x++)
	    for (y = y1; y <= y2; y++)
		lev->locations[x][y].typ =
			(y < 2 || (x % 2 && y % 2)) ? STONE : filling;
}

static void lvlfill_solid(struct level *lev, schar filling)
{
	int x,y;

	for (x = 2; x <= x_maze_max; x++)
	    for (y = 0; y <= y_maze_max; y++)
		lev->locations[x][y].typ = filling;
}

static void flip_drawbridge_horizontal(struct rm *loc)
{
	if (IS_DRAWBRIDGE(loc->typ)) {
	    if ((loc->drawbridgemask & DB_DIR) == DB_WEST) {
		loc->drawbridgemask &= ~DB_WEST;
		loc->drawbridgemask |=  DB_EAST;
	    } else if ((loc->drawbridgemask & DB_DIR) == DB_EAST) {
		loc->drawbridgemask &= ~DB_EAST;
		loc->drawbridgemask |=  DB_WEST;
	    }
	}
}

static void flip_drawbridge_vertical(struct rm *loc)
{
	if (IS_DRAWBRIDGE(loc->typ)) {
	    if ((loc->drawbridgemask & DB_DIR) == DB_NORTH) {
		loc->drawbridgemask &= ~DB_NORTH;
		loc->drawbridgemask |=  DB_SOUTH;
	    } else if ((loc->drawbridgemask & DB_DIR) == DB_SOUTH) {
		loc->drawbridgemask &= ~DB_SOUTH;
		loc->drawbridgemask |=  DB_NORTH;
	    }
	}
}

static void flip_level(struct level *lev, int flp)
{
	int x2 = COLNO - 1;
	int y2 = ROWNO - 1;

	int x, y, i;

	struct rm trm;

	struct trap *ttmp;
	struct obj *otmp;
	struct monst *mtmp;
	struct engr *etmp;
	struct mkroom *sroom;

	/* stairs and ladders */
	if (flp & 1) {
	    lev->upstair.sy = y2 - lev->upstair.sy;
	    lev->dnstair.sy = y2 - lev->dnstair.sy;
	    lev->upladder.sy = y2 - lev->upladder.sy;
	    lev->dnladder.sy = y2 - lev->dnladder.sy;
	}
	if (flp & 2) {
	    lev->upstair.sx = x2 - lev->upstair.sx;
	    lev->dnstair.sx = x2 - lev->dnstair.sx;
	    lev->upladder.sx = x2 - lev->upladder.sx;
	    lev->dnladder.sx = x2 - lev->dnladder.sx;
	}

	/* traps */
	for (ttmp = lev->lev_traps; ttmp; ttmp = ttmp->ntrap) {
	    if (flp & 1) {
		ttmp->ty = y2 - ttmp->ty;
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
		    ttmp->launch.y = y2 - ttmp->launch.y;
		    ttmp->launch2.y = y2 - ttmp->launch2.y;
		}
	    }
	    if (flp & 2) {
		ttmp->tx = x2 - ttmp->tx;
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
		    ttmp->launch.x = x2 - ttmp->launch.x;
		    ttmp->launch2.x = x2 - ttmp->launch2.x;
		}
	    }
	}

	/* objects */
	for (otmp = lev->objlist; otmp; otmp = otmp->nobj) {
	    if (flp & 1)
		otmp->oy = y2 - otmp->oy;
	    if (flp & 2)
		otmp->ox = x2 - otmp->ox;
	}

	/* monsters */
	for (mtmp = lev->monlist; mtmp; mtmp = mtmp->nmon) {
	    if (flp & 1) {
		mtmp->my = y2 - mtmp->my;
		if (mtmp->ispriest) {
		    EPRI(mtmp)->shrpos.y = y2 - EPRI(mtmp)->shrpos.y;
		} else if (mtmp->isshk) {
		    ESHK(mtmp)->shk.y = y2 - ESHK(mtmp)->shk.y;
		    ESHK(mtmp)->shd.y = y2 - ESHK(mtmp)->shd.y;
		}
	    }
	    if (flp & 2) {
		mtmp->mx = x2 - mtmp->mx;
		if (mtmp->ispriest) {
		    EPRI(mtmp)->shrpos.x = x2 - EPRI(mtmp)->shrpos.x;
		} else if (mtmp->isshk) {
		    ESHK(mtmp)->shk.x = x2 - ESHK(mtmp)->shk.x;
		    ESHK(mtmp)->shd.x = x2 - ESHK(mtmp)->shd.x;
		}
	    }
	}

	/* engravings */
	for (etmp = lev->lev_engr; etmp; etmp = etmp->nxt_engr) {
	    if (flp & 1)
		etmp->engr_y = y2 - etmp->engr_y;
	    if (flp & 2)
		etmp->engr_x = x2 - etmp->engr_x;
	}

	/* regions */
	for (i = 0; i < num_lregions; i++) {
	    if (flp & 1) {
		lregions[i].inarea.y1 = y2 - lregions[i].inarea.y1;
		lregions[i].inarea.y2 = y2 - lregions[i].inarea.y2;
		if (lregions[i].inarea.y1 > lregions[i].inarea.y2) {
		    int tmp = lregions[i].inarea.y1;
		    lregions[i].inarea.y1 = lregions[i].inarea.y2;
		    lregions[i].inarea.y2 = tmp;
		}

		lregions[i].delarea.y1 = y2 - lregions[i].delarea.y1;
		lregions[i].delarea.y2 = y2 - lregions[i].delarea.y2;
		if (lregions[i].delarea.y1 > lregions[i].delarea.y2) {
		    int tmp = lregions[i].delarea.y1;
		    lregions[i].delarea.y1 = lregions[i].delarea.y2;
		    lregions[i].delarea.y2 = tmp;
		}
	    }
	    if (flp & 2) {
		lregions[i].inarea.x1 = x2 - lregions[i].inarea.x1;
		lregions[i].inarea.x2 = x2 - lregions[i].inarea.x2;
		if (lregions[i].inarea.x1 > lregions[i].inarea.x2) {
		    int tmp = lregions[i].inarea.x1;
		    lregions[i].inarea.x1 = lregions[i].inarea.x2;
		    lregions[i].inarea.x2 = tmp;
		}

		lregions[i].delarea.x1 = x2 - lregions[i].delarea.x1;
		lregions[i].delarea.x2 = x2 - lregions[i].delarea.x2;
		if (lregions[i].delarea.x1 > lregions[i].delarea.x2) {
		    int tmp = lregions[i].delarea.x1;
		    lregions[i].delarea.x1 = lregions[i].delarea.x2;
		    lregions[i].delarea.x2 = tmp;
		}
	    }
	}

	/* rooms */
	for (sroom = &lev->rooms[0]; ; sroom++) {
	    if (sroom->hx < 0) break;

	    if (flp & 1) {
		sroom->ly = y2 - sroom->ly;
		sroom->hy = y2 - sroom->hy;
		if (sroom->ly > sroom->hy) {
		    int tmp = sroom->ly;
		    sroom->ly = sroom->hy;
		    sroom->hy = tmp;
		}
	    }
	    if (flp & 2) {
		sroom->lx = x2 - sroom->lx;
		sroom->hx = x2 - sroom->hx;
		if (sroom->lx > sroom->hx) {
		    int tmp = sroom->lx;
		    sroom->lx = sroom->hx;
		    sroom->hx = tmp;
		}
	    }

	    if (sroom->nsubrooms) {
		for (i = 0; i < sroom->nsubrooms; i++) {
		    struct mkroom *rroom = sroom->sbrooms[i];
		    if (flp & 1) {
			rroom->ly = y2 - rroom->ly;
			rroom->hy = y2 - rroom->hy;
			if (rroom->ly > rroom->hy) {
			    int tmp = rroom->ly;
			    rroom->ly = rroom->hy;
			    rroom->hy = tmp;
			}
		    }
		    if (flp & 2) {
			rroom->lx = x2 - rroom->lx;
			rroom->hx = x2 - rroom->hx;
			if (rroom->lx > rroom->hx) {
			    int tmp = rroom->lx;
			    rroom->lx = rroom->hx;
			    rroom->hx = tmp;
			}
		    }
		}
	    }
	}

	/* doors */
	for (i = 0; i < lev->doorindex; i++) {
	    if (flp & 1)
		lev->doors[i].y = y2 - lev->doors[i].y;
	    if (flp & 2)
		lev->doors[i].x = x2 - lev->doors[i].x;
	}

	/* the map */
	if (flp & 1) {
	    for (x = 0; x <= x2; x++) {
		for (y = 0; y <= (y2 / 2); y++) {
		    flip_drawbridge_vertical(&lev->locations[x][y]);
		    flip_drawbridge_vertical(&lev->locations[x][y2-y]);

		    trm = lev->locations[x][y];
		    lev->locations[x][y] = lev->locations[x][y2-y];
		    lev->locations[x][y2-y] = trm;

		    otmp = lev->objects[x][y];
		    lev->objects[x][y] = lev->objects[x][y2-y];
		    lev->objects[x][y2-y] = otmp;

		    mtmp = lev->monsters[x][y];
		    lev->monsters[x][y] = lev->monsters[x][y2-y];
		    lev->monsters[x][y2-y] = mtmp;
		}
	    }
	}
	if (flp & 2) {
	    for (x = 0; x <= (x2 / 2); x++) {
		for (y = 0; y <= y2; y++) {
		    flip_drawbridge_horizontal(&lev->locations[x][y]);
		    flip_drawbridge_horizontal(&lev->locations[x2-x][y]);

		    trm = lev->locations[x][y];
		    lev->locations[x][y] = lev->locations[x2-x][y];
		    lev->locations[x2-x][y] = trm;

		    otmp = lev->objects[x][y];
		    lev->objects[x][y] = lev->objects[x2-x][y];
		    lev->objects[x2-x][y] = otmp;

		    mtmp = lev->monsters[x][y];
		    lev->monsters[x][y] = lev->monsters[x2-x][y];
		    lev->monsters[x2-x][y] = mtmp;
		}
	    }
	}

	wall_extends(lev, 1, 0, COLNO - 1, ROWNO - 1);
}

static void flip_level_rnd(struct level *lev, int flp)
{
	int c = 0;
	if ((flp & 1) && rn2(2)) c |= 1;
	if ((flp & 2) && rn2(2)) c |= 2;
	flip_level(lev, c);
}

/*
 * Make walls of the area (x1, y1, x2, y2) non diggable/non passwall-able
 */

static void set_wall_property(struct level *lev, xchar x1, xchar y1,
			      xchar x2, xchar y2, int prop)
{
	xchar x, y;

	for (y = y1; y <= y2; y++)
	    for (x = x1; x <= x2; x++)
		if (IS_STWALL(lev->locations[x][y].typ))
		    lev->locations[x][y].wall_info |= prop;
}

static void shuffle_alignments(void)
{
	int i;
	aligntyp atmp;
	/* shuffle 3 alignments; can't use sp_lev_shuffle() on aligntyp's */
	ralign[0] = init_ralign[0]; ralign[1] = init_ralign[1]; ralign[2] = init_ralign[2];
	i = rn2(3);   atmp = ralign[2]; ralign[2] = ralign[i]; ralign[i] = atmp;
	if (rn2(2)) { atmp = ralign[1]; ralign[1] = ralign[0]; ralign[0] = atmp; }
}

/*
 * Count the different features (sinks, fountains) in the level.
 */
static void count_features(struct level *lev)
{
	xchar x, y;
	lev->flags.nfountains = lev->flags.nsinks = 0;
	for (y = 0; y < ROWNO; y++) {
	    for (x = 0; x < COLNO; x++) {
		int typ = lev->locations[x][y].typ;
		if (typ == FOUNTAIN)
		    lev->flags.nfountains++;
		else if (typ == SINK)
		    lev->flags.nsinks++;
	    }
	}
}

/*
 * Choose randomly the state (nodoor, open, closed or locked) for a door
 */
static int rnddoor(void)
{
	int i = 1 << rn2(5);
	i >>= 1;
	return i;
}

/*
 * Select a random trap
 */
static int rndtrap(struct level *lev)
{
	int rtrap;

	do {
	    rtrap = rnd(TRAPNUM-1);
	    switch (rtrap) {
	     case HOLE:		/* no random holes on special levels */
             case VIBRATING_SQUARE:
	     case MAGIC_PORTAL:	rtrap = NO_TRAP;
				break;
	     case TRAPDOOR:	if (!can_dig_down(lev)) rtrap = NO_TRAP;
				break;
	     case LEVEL_TELEP:
	     case TELEP_TRAP:	if (lev->flags.noteleport) rtrap = NO_TRAP;
				break;
	     case ROLLING_BOULDER_TRAP:
	     case ROCKTRAP:	if (In_endgame(&lev->z)) rtrap = NO_TRAP;
				break;
	    }
	} while (rtrap == NO_TRAP);
	return rtrap;
}

/*
 * Coordinates in special level files are handled specially:
 *
 *	if x or y is -(MAX_REGISTERS+1), we generate a random coordinate.
 *	if x or y is between -1 and -MAX_REGISTERS, we read one from the corresponding
 *	register (x0, x1, ... x9).
 *	if x or y is nonnegative, we convert it from relative to the local map
 *	to global coordinates.
 *	The "humidity" flag is used to insure that engravings aren't
 *	created underwater, or eels on dry land.
 */
#define DRY	0x1
#define WET	0x2


static void get_location(struct level *lev, schar *x, schar *y, int humidity, struct mkroom *croom)
{
	int cpt = 0;
	int mx, my, sx, sy;

	if (croom) {
	    mx = croom->lx;
	    my = croom->ly;
	    sx = croom->hx - mx + 1;
	    sy = croom->hy - my + 1;
	} else {
	    mx = xstart;
	    my = ystart;
	    sx = xsize;
	    sy = ysize;
	}

	if (*x >= 0) {			/* normal locations */
		*x += mx;
		*y += my;
	} else if (*x > -(MAX_REGISTERS+1)) {		/* special locations */
		*y = my + rloc_y[ - *y - 1];
		*x = mx + rloc_x[ - *x - 1];
	} else {			/* random location */
	    do {
		*x = mx + rn2((int)sx);
		*y = my + rn2((int)sy);
		if (is_ok_location(lev, *x,*y,humidity)) break;
	    } while (++cpt < 100);
	    if (cpt >= 100) {
		int xx, yy;
		/* last try */
		for (xx = 0; xx < sx; xx++) {
		    for (yy = 0; yy < sy; yy++) {
			*x = mx + xx;
			*y = my + yy;
			if (is_ok_location(lev, *x,*y,humidity)) goto found_it;
		    }
		}
		panic("get_location:  can't find a place!");
	    }
	}
found_it:

	if (!isok(*x,*y)) {
	    warning("get_location:  (%d,%d) out of bounds", *x, *y);
	    *x = x_maze_max; *y = y_maze_max;
	}
}

static boolean is_ok_location(struct level *lev, schar x, schar y, int humidity)
{
	int typ;

	if (Is_waterlevel(&lev->z)) return TRUE;	/* accept any spot */

	if (humidity & DRY) {
	    typ = lev->locations[x][y].typ;
	    if (typ == ROOM || typ == AIR ||
		    typ == CLOUD || typ == ICE || typ == CORR)
		return TRUE;
	}
	if (humidity & WET) {
	    if (is_pool(lev, x,y) || is_lava(lev, x,y))
		return TRUE;
	}
	return FALSE;
}

/*
 * Shuffle the registers for locations, objects or monsters
 */
static void sp_lev_shuffle(char list1[], char list2[], int n)
{
	int i, j;
	char k;

	for (i = n - 1; i > 0; i--) {
		if ((j = rn2(i + 1)) == i) continue;
		k = list1[j];
		list1[j] = list1[i];
		list1[i] = k;
		if (list2) {
			k = list2[j];
			list2[j] = list2[i];
			list2[i] = k;
		}
	}
}

/*
 * Get a relative position inside a room.
 * negative values for x or y means RANDOM!
 */

static void get_room_loc(struct level *lev, schar *x, schar *y, struct mkroom *croom)
{
	coord c;

	if (*x <0 && *y <0) {
		if (somexy(lev, croom, &c)) {
			*x = c.x;
			*y = c.y;
		} else
		    panic("get_room_loc : can't find a place!");
	} else {
		if (*x < 0)
		    *x = rn2(croom->hx - croom->lx + 1);
		if (*y < 0)
		    *y = rn2(croom->hy - croom->ly + 1);
		*x += croom->lx;
		*y += croom->ly;
	}
}

/*
 * Get a relative position inside a room.
 * negative values for x or y means RANDOM!
 */
static void get_free_room_loc(struct level *lev, schar *x, schar *y,
			      struct mkroom *croom)
{
	schar try_x, try_y;
	int trycnt = 0;

	try_x = *x,  try_y = *y;

	get_location(lev, &try_x, &try_y, DRY, croom);
	if (lev->locations[try_x][try_y].typ != ROOM) {
	    do {
		try_x = *x,  try_y = *y;
		get_room_loc(lev, &try_x, &try_y, croom);
	    } while (lev->locations[try_x][try_y].typ != ROOM && ++trycnt <= 100);

	    if (trycnt > 100)
		panic("get_free_room_loc : can't find a place!");
	}
	*x = try_x,  *y = try_y;
}

boolean check_room(struct level *lev, xchar *lowx, xchar *ddx,
		   xchar *lowy, xchar *ddy, boolean vault)
{
	int x,y,hix = *lowx + *ddx, hiy = *lowy + *ddy;
	struct rm *loc;
	int xlim, ylim, ymax;

	xlim = XLIM + (vault ? 1 : 0);
	ylim = YLIM + (vault ? 1 : 0);

	if (*lowx < 3)		*lowx = 3;
	if (*lowy < 2)		*lowy = 2;
	if (hix > COLNO-3)	hix = COLNO-3;
	if (hiy > ROWNO-3)	hiy = ROWNO-3;
chk:
	if (hix <= *lowx || hiy <= *lowy)	return FALSE;

	/* check area around room (and make room smaller if necessary) */
	for (x = *lowx - xlim; x<= hix + xlim; x++) {
		if (x <= 0 || x >= COLNO) continue;
		y = *lowy - ylim;
		ymax = hiy + ylim;
		if (y < 0)
		    y = 0;
		if (ymax >= ROWNO)
		    ymax = (ROWNO-1);
		loc = &lev->locations[x][y];
		
		for (; y <= ymax; y++) {
			if (loc++->typ) {
				if (!rn2(3))
				    return FALSE;

				if (x < *lowx)
				    *lowx = x + xlim + 1;
				else
				    hix = x - xlim - 1;
				if (y < *lowy)
				    *lowy = y + ylim + 1;
				else
				    hiy = y - ylim - 1;
				goto chk;
			}
		}
	}
	*ddx = hix - *lowx;
	*ddy = hiy - *lowy;
	return TRUE;
}

/*
 * Create a new room.
 */
boolean create_room(struct level *lev, xchar x, xchar y, xchar w, xchar h,
		    xchar xal, xchar yal, xchar rtype, xchar rlit)
{
	xchar	xabs, yabs;
	int	wtmp, htmp, xaltmp, yaltmp, xtmp, ytmp;
	struct nhrect *r1 = NULL, r2;
	int	trycnt = 0;
	boolean	vault = FALSE;
	int	xlim = XLIM, ylim = YLIM;

	if (rtype == -1)	/* Is the type random ? */
	    rtype = OROOM;

	if (rtype == VAULT) {
		vault = TRUE;
		xlim++;
		ylim++;
	}

	/* on low levels the room is lit (usually) */
	/* some other rooms may require lighting */

	/* is light state random ? */
	if (rlit == -1)
	    rlit = (rnd(1+abs(depth(&lev->z))) < 11 && rn2(77)) ? TRUE : FALSE;

	/*
	 * Here we will try to create a room. If some parameters are
	 * random we are willing to make several try before we give
	 * it up.
	 */
	do {
		xchar xborder, yborder;
		wtmp = w; htmp = h;
		xtmp = x; ytmp = y;
		xaltmp = xal; yaltmp = yal;

		/* First case : a totaly random room */

		if ((xtmp < 0 && ytmp <0 && wtmp < 0 && xaltmp < 0 &&
		   yaltmp < 0) || vault) {
			xchar hx, hy, lx, ly, dx, dy;
			r1 = rnd_rect(); /* Get a random rectangle */

			if (!r1) /* No more free rectangles ! */
				return FALSE;

			hx = r1->hx;
			hy = r1->hy;
			lx = r1->lx;
			ly = r1->ly;
			if (vault)
			    dx = dy = 1;
			else {
				dx = 2 + rn2((hx-lx > 28) ? 12 : 8);
				dy = 2 + rn2(4);
				if (dx*dy > 50)
				    dy = 50/dx;
			}
			xborder = (lx > 0 && hx < COLNO -1) ? 2*xlim : xlim+1;
			yborder = (ly > 0 && hy < ROWNO -1) ? 2*ylim : ylim+1;
			if (hx-lx < dx + 3 + xborder ||
			   hy-ly < dy + 3 + yborder) {
				r1 = 0;
				continue;
			}
			xabs = lx + (lx > 0 ? xlim : 3)
			    + rn2(hx - (lx>0?lx : 3) - dx - xborder + 1);
			yabs = ly + (ly > 0 ? ylim : 2)
			    + rn2(hy - (ly>0?ly : 2) - dy - yborder + 1);
			if (ly == 0 && hy >= (ROWNO-1) &&
			    (!lev->nroom || !rn2(lev->nroom)) && (yabs+dy > ROWNO/2)) {
			    yabs = rn1(3, 2);
			    if (lev->nroom < 4 && dy>1) dy--;
		        }
			if (!check_room(lev, &xabs, &dx, &yabs, &dy, vault)) {
				r1 = 0;
				continue;
			}
			wtmp = dx+1;
			htmp = dy+1;
			r2.lx = xabs-1; r2.ly = yabs-1;
			r2.hx = xabs + wtmp;
			r2.hy = yabs + htmp;
		} else {	/* Only some parameters are random */
			int rndpos = 0;
			if (xtmp < 0 && ytmp < 0) { /* Position is RANDOM */
				xtmp = rnd(5);
				ytmp = rnd(5);
				rndpos = 1;
			}
			if (wtmp < 0 || htmp < 0) { /* Size is RANDOM */
				wtmp = rn1(15, 3);
				htmp = rn1(8, 2);
			}
			if (xaltmp == -1) /* Horizontal alignment is RANDOM */
			    xaltmp = rnd(3);
			if (yaltmp == -1) /* Vertical alignment is RANDOM */
			    yaltmp = rnd(3);

			/* Try to generate real (absolute) coordinates here! */

			xabs = (((xtmp-1) * COLNO) / 5) + 1;
			yabs = (((ytmp-1) * ROWNO) / 5) + 1;
			switch (xaltmp) {
			      case LEFT:
				break;
			      case RIGHT:
				xabs += (COLNO / 5) - wtmp;
				break;
			      case CENTER:
				xabs += ((COLNO / 5) - wtmp) / 2;
				break;
			}
			switch (yaltmp) {
			      case TOP:
				break;
			      case BOTTOM:
				yabs += (ROWNO / 5) - htmp;
				break;
			      case CENTER:
				yabs += ((ROWNO / 5) - htmp) / 2;
				break;
			}

			if (xabs + wtmp - 1 > COLNO - 2)
			    xabs = COLNO - wtmp - 3;
			if (xabs < 2)
			    xabs = 2;
			if (yabs + htmp - 1> ROWNO - 2)
			    yabs = ROWNO - htmp - 3;
			if (yabs < 2)
			    yabs = 2;

			/* Try to find a rectangle that fit our room ! */

			r2.lx = xabs-1; r2.ly = yabs-1;
			r2.hx = xabs + wtmp + rndpos;
			r2.hy = yabs + htmp + rndpos;
			r1 = get_rect(&r2);
		}
	} while (++trycnt <= 100 && !r1);
	if (!r1) {	/* creation of room failed ? */
		return FALSE;
	}
	split_rects(r1, &r2);

	if (!vault) {
		smeq[lev->nroom] = lev->nroom;
		add_room(lev, xabs, yabs, xabs+wtmp-1, yabs+htmp-1,
			 rlit, rtype, FALSE);
	} else {
		lev->rooms[lev->nroom].lx = xabs;
		lev->rooms[lev->nroom].ly = yabs;
	}
	return TRUE;
}

/*
 * Create a subroom in room proom at pos x,y with width w & height h.
 * x & y are relative to the parent room.
 */
static boolean create_subroom(struct level *lev, struct mkroom *proom, xchar x, xchar y,
			      xchar w, xchar h, xchar rtype, xchar rlit)
{
	xchar width, height;

	width = proom->hx - proom->lx + 1;
	height = proom->hy - proom->ly + 1;

	/* There is a minimum size for the parent room */
	if (width < 4 || height < 4)
	    return FALSE;

	/* Check for random position, size, etc... */

	if (w == -1)
	    w = rnd(width - 3);
	if (h == -1)
	    h = rnd(height - 3);
	if (x == -1)
	    x = rnd(width - w - 1) - 1;
	if (y == -1)
	    y = rnd(height - h - 1) - 1;
	if (x == 1)
	    x = 0;
	if (y == 1)
	    y = 0;
	if ((x + w + 1) == width)
	    x++;
	if ((y + h + 1) == height)
	    y++;
	if (rtype == -1)
	    rtype = OROOM;
	if (rlit == -1)
	    rlit = (rnd(1+abs(depth(&lev->z))) < 11 && rn2(77)) ? TRUE : FALSE;
	add_subroom(lev, proom, proom->lx + x, proom->ly + y,
		    proom->lx + x + w - 1, proom->ly + y + h - 1,
		    rlit, rtype, FALSE);
	return TRUE;
}

/*
 * Create a new door in a room.
 * It's placed on a wall (north, south, east or west).
 */

static void create_door(struct level *lev, room_door *dd, struct mkroom *broom)
{
	int	x = 0, y = 0;
	int	trycnt = 0, walltry = 0, wtry = 0;

	if (dd->secret == -1)
	    dd->secret = rn2(2);

	if (dd->mask == -1) {
		/* is it a locked door, closed, or a doorway? */
		if (!dd->secret) {
			if (!rn2(3)) {
				if (!rn2(5))
				    dd->mask = D_ISOPEN;
				else if (!rn2(6))
				    dd->mask = D_LOCKED;
				else
				    dd->mask = D_CLOSED;
				if (dd->mask != D_ISOPEN && !rn2(25))
				    dd->mask |= D_TRAPPED;
			} else
			    dd->mask = D_NODOOR;
		} else {
			if (!rn2(5))	dd->mask = D_LOCKED;
			else		dd->mask = D_CLOSED;

			if (!rn2(20)) dd->mask |= D_TRAPPED;
		}
	}

	do {
		int dwall, dpos;

		dwall = dd->wall;
		if (dwall == -1)	/* The wall is RANDOM */
		    dwall = 1 << rn2(4);

		dpos = dd->pos;
		if (dpos == -1)	/* The position is RANDOM */
		    dpos = rn2(((dwall & (W_WEST|W_EAST)) ? 2 : 1) ?
			       (broom->hy - broom->ly + 1) :
			       (broom->hx - broom->lx + 1));

		/* Convert wall and pos into an absolute coordinate! */
		wtry = rn2(4);
		for (walltry = 0; walltry < 4; walltry++) {
		    switch ((wtry + walltry) % 4) {
			case 0:
			    if (!(dwall & W_NORTH)) break;
			    y = broom->ly - 1;
			    x = broom->lx + dpos;
			    goto outdirloop;
			case 1:
			    if (!(dwall & W_SOUTH)) break;
			    y = broom->hy + 1;
			    x = broom->lx + dpos;
			    goto outdirloop;
			case 2:
			    if (!(dwall & W_WEST)) break;
			    x = broom->lx - 1;
			    y = broom->ly + dpos;
			    goto outdirloop;
			case 3:
			    if (!(dwall & W_EAST)) break;
			    x = broom->hx + 1;
			    y = broom->ly + dpos;
			    goto outdirloop;
			default:
			    x = y = 0;
			    panic("create_door: No wall for door!");
			    goto outdirloop;
		    }
		}
outdirloop:
		if (okdoor(lev, x, y))
		    break;
	} while (++trycnt <= 100);
	if (trycnt > 100) {
		impossible("create_door: Can't find a proper place!");
		return;
	}
	add_door(lev, x,y,broom);
	lev->locations[x][y].typ = (dd->secret ? SDOOR : DOOR);
	lev->locations[x][y].doormask = dd->mask;
}

/*
 * Create a secret door in croom on any one of the specified walls.
 */
void create_secret_door(struct level *lev, 
			struct mkroom *croom,
			xchar walls) /* any of W_NORTH | W_SOUTH | W_EAST | W_WEST (or W_ANY) */
{
    xchar sx, sy; /* location of the secret door */
    int count;

    for (count = 0; count < 100; count++) {
	sx = rn1(croom->hx - croom->lx + 1, croom->lx);
	sy = rn1(croom->hy - croom->ly + 1, croom->ly);

	switch(rn2(4)) {
	case 0:  /* top */
	    if (!(walls & W_NORTH)) continue;
	    sy = croom->ly-1; break;
	case 1: /* bottom */
	    if (!(walls & W_SOUTH)) continue;
	    sy = croom->hy+1; break;
	case 2: /* left */
	    if (!(walls & W_EAST)) continue;
	    sx = croom->lx-1; break;
	case 3: /* right */
	    if (!(walls & W_WEST)) continue;
	    sx = croom->hx+1; break;
	}

	if (okdoor(lev, sx, sy)) {
	    lev->locations[sx][sy].typ = SDOOR;
	    lev->locations[sx][sy].doormask = D_CLOSED;
	    add_door(lev, sx, sy, croom);
	    return;
	}
    }

    impossible("couldn't create secret door on any walls 0x%x", walls);
}

/*
 * Create a trap in a room.
 */
static void create_trap(struct level *lev, trap *t, struct mkroom *croom)
{
    schar	x,y;
    coord	tm;

    if (rn2(100) < t->chance) {
	x = t->x;
	y = t->y;
	if (croom)
	    get_free_room_loc(lev, &x, &y, croom);
	else
	    get_location(lev, &x, &y, DRY, croom);

	tm.x = x;
	tm.y = y;

	mktrap(lev, t->type, 1, NULL, &tm);
    }
}

static void spill_terrain(struct level *lev, spill *sp, struct mkroom *croom)
{
	schar x, y, nx, ny, qx, qy;
	int j, k, lastdir, guard;
	boolean found = FALSE;

	if (sp->typ >= MAX_TYPE) return;

	/* This code assumes that you're going to spill one particular
	 * type of terrain from a wall into somewhere.
	 *
	 * If we were given a specific coordinate, though, it doesn't have
	 * to start from a wall... */
	if (sp->x <= -(MAX_REGISTERS+1) || sp->y <= -(MAX_REGISTERS+1)) {
	    for (j = 0; j < 500; j++) {
		x = sp->x;
		y = sp->y;
		get_location(lev, &x, &y, DRY|WET, croom);
		nx = x;  ny = y;
		switch (sp->direction) {
		/* backwards to make sure we're against a wall */
		case W_NORTH: ny++; break;
		case W_SOUTH: ny--; break;
		case W_WEST: nx++; break;
		case W_EAST: nx--; break;
		default: return; break;
		}
		if (!isok(nx,ny)) continue;
		if (IS_WALL(lev->locations[nx][ny].typ)) {
		    /* mark it as broken through */
		    lev->locations[nx][ny].typ = sp->typ;
		    lev->locations[nx][ny].lit = sp->lit;
		    found = TRUE;
		    break;
		}
	    }
	} else {
	    found = TRUE;
	    x = sp->x;
	    y = sp->y;
	    /* support random registers too */
	    get_location(lev, &x, &y, DRY|WET, croom);
	}

	if (!found) return;

	/* gloop! */
	lastdir = -1;  nx = x;  ny = y;
	for (j = sp->count; j > 0; j--) {
	    guard = 0;
	    lev->locations[nx][ny].typ = sp->typ;
	    lev->locations[nx][ny].lit = sp->lit;
	    do {
		guard++;
		do {
		    k = rn2(5);
		    qx = nx;  qy = ny;
		    if (k > 3)
			k = sp->direction;
		    else
			k = 1 << k;
		    switch(k) {
		    case W_NORTH: qy--; break;
		    case W_SOUTH: qy++; break;
		    case W_WEST: qx--; break;
		    case W_EAST: qx++; break;
		    }
		} while (!isok(qx,qy));
	    } while ((k == lastdir || lev->locations[qx][qy].typ == sp->typ) &&
		     guard < 200);
	    /* tend to not make rivers, but pools;
	     * and don't redo stuff of the same type! */

	    switch(k) {
	    case W_NORTH: ny--; break;
	    case W_SOUTH: ny++; break;
	    case W_WEST: nx--; break;
	    case W_EAST: nx++; break;
	    }
	    lastdir = k;
	}
}

/*
 * Create a monster in a room.
 */
static int noncoalignment(aligntyp alignment)
{
	int k;

	k = rn2(2);
	if (!alignment)
		return k ? -1 : 1;
	return k ? -alignment : 0;
}

static void create_monster(struct level *lev, monster *m, struct mkroom *croom)
{
    struct monst *mtmp;
    schar x, y;
    char class;
    aligntyp amask;
    coord cc;
    const struct permonst *pm;
    unsigned g_mvflags;

    if (rn2(100) < m->chance) {

	if (m->class >= 0)
	    class = (char) def_char_to_monclass((char)m->class);
	else if (m->class > -(MAX_REGISTERS+1))
	    class = (char) def_char_to_monclass(rmonst[- m->class - 1]);
	else
	    class = 0;

	if (class == MAXMCLASSES)
	    panic("create_monster: unknown monster class '%c'", m->class);

	amask = (m->align == AM_SPLEV_CO) ?
			Align2amask(u.ualignbase[A_ORIGINAL]) :
		(m->align == AM_SPLEV_NONCO) ?
			Align2amask(noncoalignment(u.ualignbase[A_ORIGINAL])) :
		(m->align <= -(MAX_REGISTERS+1)) ? induced_align(&lev->z, 80) :
		(m->align < 0 ? ralign[-m->align-1] : m->align);

	if (!class)
	    pm = NULL;
	else if (m->id != NON_PM) {
	    pm = &mons[m->id];
	    g_mvflags = (unsigned) mvitals[m->id].mvflags;
	    if ((pm->geno & G_UNIQ) && (g_mvflags & G_EXTINCT))
		return;
	    else if (g_mvflags & G_GONE)	/* genocided or extinct */
		pm = NULL;	/* make random monster */
	} else {
	    pm = mkclass(&lev->z, class, G_NOGEN);
	    /* if we can't get a specific monster type (pm == 0) then the
	       class has been genocided, so settle for a random monster */
	}
	if (In_mines(&lev->z) && pm && your_race(pm) &&
			(Race_if (PM_DWARF) || Race_if(PM_GNOME)) && rn2(3))
	    pm = NULL;

	x = m->x;
	y = m->y;
	if (!pm || !is_swimmer(pm))
	    get_location(lev, &x, &y, DRY, croom);
	else if (pm->mlet == S_EEL)
	    get_location(lev, &x, &y, WET, croom);
	else
	    get_location(lev, &x, &y, DRY|WET, croom);
	/* try to find a close place if someone else is already there */
	if (MON_AT(lev, x,y) && enexto(&cc, lev, x, y, pm))
	    x = cc.x,  y = cc.y;

	if (m->align != -(MAX_REGISTERS+2))
	    mtmp = mk_roamer(pm, Amask2align(amask), lev, x, y, m->peaceful);
	else if (PM_ARCHEOLOGIST <= m->id && m->id <= PM_WIZARD)
	         mtmp = mk_mplayer(pm, lev, x, y, FALSE);
	else mtmp = makemon(pm, lev, x, y, NO_MM_FLAGS);

	if (mtmp) {
	    /* handle specific attributes for some special monsters */
	    if (m->name.str) mtmp = christen_monst(mtmp, m->name.str);

	    /*
	     * This is currently hardwired for mimics only.  It should
	     * eventually be expanded.
	     */
	    if (m->appear_as.str && mtmp->data->mlet == S_MIMIC) {
		int i;

		switch (m->appear) {
		    case M_AP_NOTHING:
			warning(
		"create_monster: mon has an appearance, \"%s\", but no type",
				m->appear_as.str);
			break;

		    case M_AP_FURNITURE:
			for (i = 0; i < MAXPCHARS; i++)
			    if (!strcmp(defexplain[i], m->appear_as.str))
				break;
			if (i == MAXPCHARS) {
			    warning(
				"create_monster: can't find feature \"%s\"",
				m->appear_as.str);
			} else {
			    mtmp->m_ap_type = M_AP_FURNITURE;
			    mtmp->mappearance = i;
			}
			break;

		    case M_AP_OBJECT:
			for (i = 0; i < NUM_OBJECTS; i++)
			    if (OBJ_NAME(objects[i]) &&
				!strcmp(OBJ_NAME(objects[i]),m->appear_as.str))
				break;
			if (i == NUM_OBJECTS) {
			    warning(
				"create_monster: can't find object \"%s\"",
				m->appear_as.str);
			} else {
			    mtmp->m_ap_type = M_AP_OBJECT;
			    mtmp->mappearance = i;
			}
			break;

		    case M_AP_MONSTER:
			/* note: mimics don't appear as monsters! */
			/*	 (but chameleons can :-)	  */
		    default:
			warning(
		"create_monster: unimplemented mon appear type [%d,\"%s\"]",
				m->appear, m->appear_as.str);
			break;
		}
		if (does_block(lev, x, y))
		    block_point(x, y);
	    }

	    if (m->peaceful >= 0) {
		mtmp->mpeaceful = m->peaceful;
		/* changed mpeaceful again; have to reset malign */
		set_malign(mtmp);
	    }
	    if (m->asleep >= 0) {
		mtmp->msleeping = m->asleep;
	    }
	}

    }		/* if (rn2(100) < m->chance) */
}

/*
 * Create an object in a room.
 */
static void create_object(struct level *lev, object *o, struct mkroom *croom)
{
    struct obj *otmp;
    schar x, y;
    char c;
    boolean named;	/* has a name been supplied in level description? */

    if (rn2(100) < o->chance) {
	named = o->name.str ? TRUE : FALSE;

	x = o->x; y = o->y;
	get_location(lev, &x, &y, DRY, croom);

	if (o->class >= 0)
	    c = o->class;
	else if (o->class > -(MAX_REGISTERS+1))
	    c = robjects[ -(o->class+1)];
	else
	    c = 0;

	if (!c)
	    otmp = mkobj_at(RANDOM_CLASS, lev, x, y, !named);
	else if (o->id != -1)
	    otmp = mksobj_at(o->id, lev, x, y, TRUE, !named);
	else {
	    /*
	     * The special levels are compiled with the default "text" object
	     * class characters.  We must convert them to the internal format.
	     */
	    char oclass = (char) def_char_to_objclass(c);

	    if (oclass == MAXOCLASSES)
		panic("create_object:  unexpected object class '%c'",c);

	    /* KMH -- Create piles of gold properly */
	    if (oclass == COIN_CLASS)
		otmp = mkgold(0L, lev, x, y);
	    else
		otmp = mkobj_at(oclass, lev, x, y, !named);
	}

	if (o->spe != -127)	/* That means NOT RANDOM! */
	    otmp->spe = (schar)o->spe;

	switch (o->curse_state) {
	      case 1:	bless(otmp); break; /* BLESSED */
	      case 2:	unbless(otmp); uncurse(otmp); break; /* uncursed */
	      case 3:	curse(otmp); break; /* CURSED */
	      default:	break;	/* Otherwise it's random and we're happy
				 * with what mkobj gave us! */
	}

	/*	corpsenm is "empty" if -1, random if -2, otherwise specific */
	if (o->corpsenm == NON_PM - 1) otmp->corpsenm = rndmonnum(&lev->z);
	else if (o->corpsenm != NON_PM) otmp->corpsenm = o->corpsenm;

	/* assume we wouldn't be given an egg corpsenm unless it was
	   hatchable */
	if (otmp->otyp == EGG && otmp->corpsenm != NON_PM) {
	    if (dead_species(otmp->otyp, TRUE))
		kill_egg(otmp);	/* make sure nothing hatches */
	    else
		attach_egg_hatch_timeout(otmp);	/* attach new hatch timeout */
	}

	if (named)
	    otmp = oname(otmp, o->name.str);

	switch (o->containment) {
	    /* contents */
	    case 1:
		if (!container_idx) {
		    warning("create_object: no container");
		    break;
		}
		remove_object(otmp);
		add_to_container(container_obj[container_idx - 1], otmp);
		return;		/* don't stack */
	    /* container */
	    case 2:
		delete_contents(otmp);
		if (container_idx < MAX_CONTAINMENT) {
		    if (container_idx) {
			remove_object(otmp);
			add_to_container(container_obj[container_idx - 1], otmp);
		    }
		    container_obj[container_idx] = otmp;
		    container_idx++;
		} else warning("create_object: containers nested too deeply.");
		break;
	    /* nothing */
	    case 0: break;

	    default: warning("containment type %d?", (int) o->containment);
	}

	/* Medusa level special case: statues are petrified monsters, so they
	 * are not stone-resistant and have monster inventory.  They also lack
	 * other contents, but that can be specified as an empty container.
	 */
	if (o->id == STATUE && Is_medusa_level(&lev->z) &&
		    o->corpsenm == NON_PM) {
	    struct monst *was;
	    struct obj *obj;
	    int wastyp;

	    /* Named random statues are of player types, and aren't stone-
	     * resistant (if they were, we'd have to reset the name as well as
	     * setting corpsenm).
	     */
	    for (wastyp = otmp->corpsenm; ; wastyp = rndmonnum(&lev->z)) {
		/* makemon without rndmonst() might create a group */
		was = makemon(&mons[wastyp], lev, 0, 0, NO_MM_FLAGS);
		if (!resists_ston(was)) break;
		mongone(was);
	    }
	    otmp->corpsenm = wastyp;
	    while (was->minvent) {
		obj = was->minvent;
		obj->owornmask = 0;
		obj_extract_self(obj);
		add_to_container(otmp, obj);
	    }
	    otmp->owt = weight(otmp);
	    mongone(was);
	}

	stackobj(otmp);

    }		/* if (rn2(100) < o->chance) */
}

/*
 * Randomly place a specific engraving, then release its memory.
 */
static void create_engraving(struct level *lev, engraving *e, struct mkroom *croom)
{
	xchar x, y;

	x = e->x,  y = e->y;
	get_location(lev, &x, &y, DRY, croom);

	make_engr_at(lev, x, y, e->engr.str, 0L, e->etype);
}

/*
 * Create stairs in a room.
 */
static void create_stairs(struct level *lev, stair *s, struct mkroom *croom)
{
	schar x,y;

	x = s->x; y = s->y;
	get_location(lev, &x, &y, DRY, croom);
	mkstairs(lev, x,y,(char)s->up, croom);
}

/*
 * Create an altar in a room.
 */
static void create_altar(struct level *lev, altar *a, struct mkroom *croom)
{
	schar		sproom,x,y;
	aligntyp	amask;
	boolean		croom_is_temple = TRUE;
	int oldtyp; 

	x = a->x; y = a->y;

	if (croom) {
	    get_free_room_loc(lev, &x, &y, croom);
	    if (croom->rtype != TEMPLE)
		croom_is_temple = FALSE;
	} else {
	    get_location(lev, &x, &y, DRY, croom);
	    if ((sproom = (schar) *in_rooms(lev, x, y, TEMPLE)) != 0)
		croom = &lev->rooms[sproom - ROOMOFFSET];
	    else
		croom_is_temple = FALSE;
	}

	/* check for existing features */
	oldtyp = lev->locations[x][y].typ;
	if (oldtyp == STAIRS || oldtyp == LADDER)
	    return;

	a->x = x;
	a->y = y;

	/* Is the alignment random ?
	 * If so, it's an 80% chance that the altar will be co-aligned.
	 *
	 * The alignment is encoded as amask values instead of alignment
	 * values to avoid conflicting with the rest of the encoding,
	 * shared by many other parts of the special level code.
	 */

	amask = (a->align == AM_SPLEV_CO) ?
			Align2amask(u.ualignbase[A_ORIGINAL]) :
		(a->align == AM_SPLEV_NONCO) ?
			Align2amask(noncoalignment(u.ualignbase[A_ORIGINAL])) :
		(a->align == -(MAX_REGISTERS+1)) ? induced_align(&lev->z, 80) :
		(a->align < 0 ? ralign[-a->align-1] : a->align);

	lev->locations[x][y].typ = ALTAR;
	lev->locations[x][y].altarmask = amask;

	if (a->shrine < 0) a->shrine = rn2(2);	/* handle random case */

	if (!croom_is_temple || !a->shrine) return;

	if (a->shrine) {	/* Is it a shrine  or sanctum? */
	    priestini(lev, croom, x, y, (a->shrine > 1));
	    lev->locations[x][y].altarmask |= AM_SHRINE;
	    lev->flags.has_temple = TRUE;
	}
}

/*
 * Create a gold pile in a room.
 */
static void create_gold(struct level *lev, gold *g, struct mkroom *croom)
{
	schar		x,y;

	x = g->x; y= g->y;
	get_location(lev, &x, &y, DRY, croom);

	if (g->amount == -1)
	    g->amount = rnd(200);
	mkgold((long) g->amount, lev, x, y);
}

/*
 * Create a feature (e.g a fountain) in a room.
 */
static void create_feature(struct level *lev, int fx, int fy, struct mkroom *croom, int typ)
{
	schar	x,y;

	x = fx;  y = fy;
	get_location(lev, &x, &y, DRY, croom);
	/* Don't cover up an existing feature (particularly randomly
	   placed stairs).  However, if the _same_ feature is already
	   here, it came from the map drawing and we still need to
	   update the special counters. */
	if (IS_FURNITURE(lev->locations[x][y].typ) && lev->locations[x][y].typ != typ)
	    return;

	lev->locations[x][y].typ = typ;
}

static void replace_terrain(struct level *lev, replaceterrain *terr, struct mkroom *croom)
{
	schar x, y, x1, y1, x2, y2;

	if (terr->toter >= MAX_TYPE) return;

	x1 = terr->x1;  y1 = terr->y1;
	get_location(lev, &x1, &y1, DRY|WET, croom);

	x2 = terr->x2;  y2 = terr->y2;
	get_location(lev, &x2, &y2, DRY|WET, croom);

	for (x = x1; x <= x2; x++) {
	    for (y = y1; y <= y2; y++) {
		if (lev->locations[x][y].typ == terr->fromter &&
			rn2(100) < terr->chance) {
		    lev->locations[x][y].typ = terr->toter;
		    lev->locations[x][y].lit = terr->tolit;
		}
	    }
	}
}

static void set_terrain(struct level *lev, terrain *terr, struct mkroom *croom)
{
	schar x, y, x1, y1, x2, y2;

	if (terr->ter >= MAX_TYPE) return;

	if (rn2(100) >= terr->chance) return;

	x1 = terr->x1;  y1 = terr->y1;
	get_location(lev, &x1, &y1, DRY|WET, croom);

	switch (terr->areatyp) {
	case 0: /* point */
	default:
	    lev->locations[x1][y1].typ = terr->ter;
	    lev->locations[x1][y1].lit = terr->tlit;
	    /* handle doors and secret doors */
	    if (lev->locations[x1][y1].typ == SDOOR ||
		    IS_DOOR(lev->locations[x1][y1].typ)) {
		if (lev->locations[x1][y1].typ == SDOOR)
		    lev->locations[x1][y1].doormask = D_CLOSED;
		if (x1 && (IS_WALL(lev->locations[x1-1][y1].typ) ||
			lev->locations[x1-1][y1].horizontal))
		    lev->locations[x1][y1].horizontal = 1;
	    }
	    break;
	case 1: /* horiz line */
	    for (x = 0; x < terr->x2; x++) {
		lev->locations[x + x1][y1].typ = terr->ter;
		lev->locations[x + x1][y1].lit = terr->tlit;
	    }
	    break;
	case 2: /* vert line */
	    for (y = 0; y < terr->y2; y++) {
		lev->locations[x1][y + y1].typ = terr->ter;
		lev->locations[x1][y + y1].lit = terr->tlit;
	    }
	    break;
	case 3: /* filled rectangle */
	    x2 = terr->x2;  y2 = terr->y2;
	    get_location(lev, &x2, &y2, DRY|WET, croom);
	    for (x = x1; x <= x2; x++) {
		for (y = y1; y <= y2; y++) {
		    lev->locations[x][y].typ = terr->ter;
		    lev->locations[x][y].lit = terr->tlit;
		}
	    }
	    break;
	case 4: /* rectangle */
	    x2 = terr->x2;  y2 = terr->y2;
	    get_location(lev, &x2, &y2, DRY|WET, croom);
	    for (x = x1; x <= x2; x++) {
		lev->locations[x][y1].typ = terr->ter;
		lev->locations[x][y1].lit = terr->tlit;
		lev->locations[x][y2].typ = terr->ter;
		lev->locations[x][y2].lit = terr->tlit;
	    }
	    for (y = y1; y <= y2; y++) {
		lev->locations[x1][y].typ = terr->ter;
		lev->locations[x1][y].lit = terr->tlit;
		lev->locations[x2][y].typ = terr->ter;
		lev->locations[x2][y].lit = terr->tlit;
	    }
	    break;
	}
}

static void put_terr_spot(struct level *lev, schar x, schar y,
			  schar ter, schar lit, schar thick)
{
	int dx, dy;

	if (thick < 1) thick = 1;
	else if (thick > 10) thick = 10;

	for (dx = x - thick / 2; dx < x + (thick + 1) / 2; dx++) {
	    for (dy = y - thick / 2; dy < y + (thick+1) / 2; dy++) {
		if (!(dx >= COLNO - 1 || dx <= 0 || dy <= 0 || dy >= ROWNO - 1)) {
		    lev->locations[dx][dy].typ = ter;
		    lev->locations[dx][dy].lit = lit;
		}
	    }
	}
}

static void line_midpoint_core(struct level *lev,
			       schar x1, schar y1, schar x2, schar y2, schar rough,
			       schar ter, schar lit, schar thick, schar rec)
{
	int mx, my;
	int dx, dy;

	if (rec < 1)
	    return;

	if (x2 == x1 && y2 == y1) {
	    put_terr_spot(lev, x1, y1, ter, lit, thick);
	    return;
	}

	if (rough > max(abs(x2 - x1), abs(y2 - y1)))
	    rough = max(abs(x2 - x1), abs(y2 - y1));

	if (rough < 2) {
	    mx = (x1 + x2) / 2;
	    my = (y1 + y2) / 2;
	} else {
	    do {
		dx = rand() % rough - rough / 2;
		dy = rand() % rough - rough / 2;
		mx = (x1 + x2) / 2 + dx;
		my = (y1 + y2) / 2 + dy;
	    } while (mx > COLNO - 1 || mx < 0 || my < 0 || my > ROWNO - 1);
	}

	put_terr_spot(lev, mx, my, ter, lit, thick);

	rough = (rough * 2) / 3;

	rec--;

	line_midpoint_core(lev, x1, y1, mx, my, rough, ter, lit, thick, rec);
	line_midpoint_core(lev, mx, my, x2, y2, rough, ter, lit, thick, rec);
}

static void line_midpoint(struct level *lev, randline *rndline, struct mkroom *croom)
{
	schar x1, y1, x2, y2, thick;

	x1 = rndline->x1;  y1 = rndline->y1;
	get_location(lev, &x1, &y1, DRY|WET, croom);

	x2 = rndline->x2;  y2 = rndline->y2;
	get_location(lev, &x2, &y2, DRY|WET, croom);

	thick = rndline->thick;

	line_midpoint_core(lev, x1, y1, x2, y2, rndline->roughness,
			   rndline->fg, rndline->lit, thick, 12);
}

/*
 * Search for a door in a room on a specified wall.
 */
static boolean search_door(struct mkroom *croom, xchar *x, xchar *y,
			   xchar wall, int cnt)
{
	int dx, dy;
	int xx,yy;

	switch(wall) {
	      case W_NORTH:
		dy = 0; dx = 1;
		xx = croom->lx;
		yy = croom->hy + 1;
		break;
	      case W_SOUTH:
		dy = 0; dx = 1;
		xx = croom->lx;
		yy = croom->ly - 1;
		break;
	      case W_EAST:
		dy = 1; dx = 0;
		xx = croom->hx + 1;
		yy = croom->ly;
		break;
	      case W_WEST:
		dy = 1; dx = 0;
		xx = croom->lx - 1;
		yy = croom->ly;
		break;
	      default:
		dx = dy = xx = yy = 0;
		panic("search_door: Bad wall!");
		break;
	}
	while (xx <= croom->hx+1 && yy <= croom->hy+1) {
		if (IS_DOOR(level->locations[xx][yy].typ) || level->locations[xx][yy].typ == SDOOR) {
			*x = xx;
			*y = yy;
			if (cnt-- <= 0)
			    return TRUE;
		}
		xx += dx;
		yy += dy;
	}
	return FALSE;
}

/*
 * Dig a corridor between two points.
 */
boolean dig_corridor(struct level *lev, coord *org, coord *dest, boolean nxcor,
		     schar ftyp, schar btyp)
{
	int dx=0, dy=0, dix, diy, cct;
	struct rm *crm;
	int tx, ty, xx, yy;

	xx = org->x;  yy = org->y;
	tx = dest->x; ty = dest->y;
	if (xx <= 0 || yy <= 0 || tx <= 0 || ty <= 0 ||
	    xx > COLNO-1 || tx > COLNO-1 ||
	    yy > ROWNO-1 || ty > ROWNO-1)
		return FALSE;

	if (tx > xx)		dx = 1;
	else if (ty > yy)	dy = 1;
	else if (tx < xx)	dx = -1;
	else			dy = -1;

	xx -= dx;
	yy -= dy;
	cct = 0;
	while (xx != tx || yy != ty) {
	    /* loop: dig corridor at [xx,yy] and find new [xx,yy] */
	    if (cct++ > 500 || (nxcor && !rn2(35)))
		return FALSE;

	    xx += dx;
	    yy += dy;

	    if (xx >= COLNO-1 || xx <= 0 || yy <= 0 || yy >= ROWNO-1)
		return FALSE;		/* impossible */

	    crm = &lev->locations[xx][yy];
	    if (crm->typ == btyp) {
		if (ftyp != CORR || rn2(100)) {
			crm->typ = ftyp;
			if (nxcor && !rn2(50))
				mksobj_at(BOULDER, lev, xx, yy, TRUE, FALSE);
		} else {
			crm->typ = SCORR;
		}
	    } else
	    if (crm->typ != ftyp && crm->typ != SCORR) {
		/* strange ... */
		return FALSE;
	    }

	    /* find next corridor position */
	    dix = abs(xx-tx);
	    diy = abs(yy-ty);

	    /* do we have to change direction ? */
	    if (dy && dix > diy) {
		int ddx = (xx > tx) ? -1 : 1;

		crm = &lev->locations[xx+ddx][yy];
		if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
		    dx = ddx;
		    dy = 0;
		    continue;
		}
	    } else if (dx && diy > dix) {
		int ddy = (yy > ty) ? -1 : 1;

		crm = &lev->locations[xx][yy+ddy];
		if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
		    dy = ddy;
		    dx = 0;
		    continue;
		}
	    }

	    /* continue straight on? */
	    crm = &lev->locations[xx+dx][yy+dy];
	    if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
		continue;

	    /* no, what must we do now?? */
	    if (dx) {
		dx = 0;
		dy = (ty < yy) ? -1 : 1;
	    } else {
		dy = 0;
		dx = (tx < xx) ? -1 : 1;
	    }
	    crm = &lev->locations[xx+dx][yy+dy];
	    if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
		continue;
	    dy = -dy;
	    dx = -dx;
	}
	return TRUE;
}

/*
 * Disgusting hack: since special levels have their rooms filled before
 * sorting the rooms, we have to re-arrange the speed values upstairs_room
 * and dnstairs_room after the rooms have been sorted.  On normal levels,
 * stairs don't get created until _after_ sorting takes place.
 */
static void fix_stair_rooms(struct level *lev)
{
    int i;
    struct mkroom *croom;

    if (lev->dnstair.sx &&
       !((lev->dnstairs_room->lx <= lev->dnstair.sx && lev->dnstair.sx <= lev->dnstairs_room->hx) &&
	 (lev->dnstairs_room->ly <= lev->dnstair.sy && lev->dnstair.sy <= lev->dnstairs_room->hy))) {
	for (i=0; i < lev->nroom; i++) {
	    croom = &lev->rooms[i];
	    if ((croom->lx <= lev->dnstair.sx && lev->dnstair.sx <= croom->hx) &&
	       (croom->ly <= lev->dnstair.sy && lev->dnstair.sy <= croom->hy)) {
		lev->dnstairs_room = croom;
		break;
	    }
	}
	if (i == lev->nroom)
	    panic("Couldn't find dnstair room in fix_stair_rooms!");
    }
    if (lev->upstair.sx &&
       !((lev->upstairs_room->lx <= lev->upstair.sx && lev->upstair.sx <= lev->upstairs_room->hx) &&
	 (lev->upstairs_room->ly <= lev->upstair.sy && lev->upstair.sy <= lev->upstairs_room->hy))) {
	for (i=0; i < lev->nroom; i++) {
	    croom = &lev->rooms[i];
	    if ((croom->lx <= lev->upstair.sx && lev->upstair.sx <= croom->hx) &&
	       (croom->ly <= lev->upstair.sy && lev->upstair.sy <= croom->hy)) {
		lev->upstairs_room = croom;
		break;
	    }
	}
	if (i == lev->nroom)
	    panic("Couldn't find upstair room in fix_stair_rooms!");
    }
}

/*
 * Corridors always start from a door. But it can end anywhere...
 * Basically we search for door coordinates or for endpoints coordinates
 * (from a distance).
 */
static void create_corridor(struct level *lev, corridor *c)
{
	coord org, dest;

	if (c->src.room == -1) {
		fix_stair_rooms(lev);
		makecorridors(lev);
		return;
	}

	if ( !search_door(&lev->rooms[c->src.room], &org.x, &org.y, c->src.wall,
			 c->src.door))
	    return;

	if (c->dest.room != -1) {
		if (!search_door(&lev->rooms[c->dest.room], &dest.x, &dest.y,
				c->dest.wall, c->dest.door))
		    return;
		switch(c->src.wall) {
		      case W_NORTH: org.y--; break;
		      case W_SOUTH: org.y++; break;
		      case W_WEST:  org.x--; break;
		      case W_EAST:  org.x++; break;
		}
		switch(c->dest.wall) {
		      case W_NORTH: dest.y--; break;
		      case W_SOUTH: dest.y++; break;
		      case W_WEST:  dest.x--; break;
		      case W_EAST:  dest.x++; break;
		}
		dig_corridor(lev, &org, &dest, FALSE, CORR, STONE);
	}
}


/*
 * Fill a room (shop, zoo, etc...) with appropriate stuff.
 */
void fill_room(struct level *lev, struct mkroom *croom, boolean prefilled)
{
	if (!croom || croom->rtype == OROOM)
	    return;

	if (!prefilled) {
	    int x,y;

	    /* Shop ? */
	    if (croom->rtype >= SHOPBASE) {
		    stock_room(croom->rtype - SHOPBASE, lev, croom);
		    lev->flags.has_shop = TRUE;
		    return;
	    }

	    switch (croom->rtype) {
		case VAULT:
		    for (x=croom->lx;x<=croom->hx;x++)
			for (y=croom->ly;y<=croom->hy;y++)
			    mkgold((long)rn1(abs(depth(&lev->z))*100, 51), lev, x, y);
		    break;
		case GARDEN:
		case COURT:
		case ZOO:
		case BEEHIVE:
		case LEMUREPIT:
		case MORGUE:
		case BARRACKS:
		    fill_zoo(lev, croom);
		    break;
	    }
	}
	switch (croom->rtype) {
	    case VAULT:
		lev->flags.has_vault = TRUE;
		break;
	    case ZOO:
		lev->flags.has_zoo = TRUE;
		break;
	    case GARDEN:
		lev->flags.has_garden = TRUE;
		break;
	    case COURT:
		lev->flags.has_court = TRUE;
		break;
	    case MORGUE:
		lev->flags.has_morgue = TRUE;
		break;
	    case BEEHIVE:
		lev->flags.has_beehive = TRUE;
		break;
	    case LEMUREPIT:
		lev->flags.has_lemurepit = TRUE;
		break;
	    case BARRACKS:
		lev->flags.has_barracks = TRUE;
		break;
	    case TEMPLE:
		lev->flags.has_temple = TRUE;
		break;
	    case SWAMP:
		lev->flags.has_swamp = TRUE;
		break;
	}
}

static struct mkroom *build_room(struct level *lev, room *r, struct mkroom *mkr)
{
	boolean okroom;
	struct mkroom	*aroom;
	xchar rtype = (!r->chance || rn2(100) < r->chance) ? r->rtype : OROOM;

	if (mkr) {
		aroom = &lev->subrooms[lev->nsubroom];
		okroom = create_subroom(lev, mkr, r->x, r->y, r->w, r->h,
					rtype, r->rlit);
	} else {
		aroom = &lev->rooms[lev->nroom];
		okroom = create_room(lev, r->x, r->y, r->w, r->h, r->xalign,
				     r->yalign, rtype, r->rlit);
	}

	if (okroom) {
		topologize(lev, aroom);			/* set roomno */
		aroom->needfill = ((aroom->rtype != OROOM) && r->filled);
		return aroom;
	}
	return NULL;
}

/*
 * set lighting in a region that will not become a room.
 */
static void light_region(struct level *lev, region *tmpregion)
{
    boolean litstate = tmpregion->rlit ? 1 : 0;
    int hiy = tmpregion->y2;
    int x, y;
    struct rm *loc;
    int lowy = tmpregion->y1;
    int lowx = tmpregion->x1, hix = tmpregion->x2;

    if (litstate) {
	/* adjust region size for walls, but only if lighted */
	lowx = max(lowx-1,1);
	hix = min(hix+1,COLNO-1);
	lowy = max(lowy-1,0);
	hiy = min(hiy+1, ROWNO-1);
    }
    for (x = lowx; x <= hix; x++) {
	loc = &lev->locations[x][lowy];
	for (y = lowy; y <= hiy; y++) {
	    if (loc->typ != LAVAPOOL) /* this overrides normal lighting */
		loc->lit = litstate;
	    loc++;
	}
    }
}

static void load_one_monster(dlb *fd, monster *m)
{
	int size;

	Fread(m, 1, sizeof *m, fd);
	if ((size = m->name.len) != 0) {
	    m->name.str = malloc((unsigned)size + 1);
	    Fread(m->name.str, 1, size, fd);
	    m->name.str[size] = '\0';
	} else
	    m->name.str = NULL;
	if ((size = m->appear_as.len) != 0) {
	    m->appear_as.str = malloc((unsigned)size + 1);
	    Fread(m->appear_as.str, 1, size, fd);
	    m->appear_as.str[size] = '\0';
	} else
	    m->appear_as.str = NULL;
	
	return;
err_out:
	fprintf(stderr, "read error in load_one_monster\n");
}

static void load_one_object(dlb *fd, object *o)
{
	int size;

	Fread(o, 1, sizeof *o, fd);
	if ((size = o->name.len) != 0) {
	    o->name.str = malloc((unsigned)size + 1);
	    Fread(o->name.str, 1, size, fd);
	    o->name.str[size] = '\0';
	} else
	    o->name.str = NULL;

	return;
err_out:
	fprintf(stderr, "read error in load_one_object\n");
}

static void load_one_engraving(dlb *fd, engraving *e)
{
	int size;

	Fread(e, 1, sizeof *e, fd);
	size = e->engr.len;
	e->engr.str = malloc((unsigned)size+1);
	Fread(e->engr.str, 1, size, fd);
	e->engr.str[size] = '\0';
	
	return;
err_out:
	fprintf(stderr, "read error in load_one_engraving\n");
}

static void load_one_room(struct level *lev, dlb *fd, room *r)
{
	int size;

	Fread(r, 1, sizeof *r, fd);
	size = r->name.len;
	if (size > 0) {
	    r->name.str = malloc((unsigned)size+1);
	    Fread(r->name.str, 1, size, fd);
	    r->name.str[size] = '\0';
	}
	size = r->parent.len;
	if (size > 0) {
	    r->parent.str = malloc((unsigned)size+1);
	    Fread(r->parent.str, 1, size, fd);
	    r->parent.str[size] = '\0';
	}
	return;
err_out:
	fprintf(stderr, "read error in load_one_room\n");
}

static void wallify_map(struct level *lev)
{
	int x, y, xx, yy, lo_xx, lo_yy, hi_xx, hi_yy;

	for (y = ystart; y <= ystart + ysize; y++) {
	    lo_yy = (y > 0) ? y - 1 : 0;
	    hi_yy = (y < ystart + ysize) ? y + 1 : ystart + ysize;
	    for (x = xstart; x <= xstart + xsize; x++) {
		if (lev->locations[x][y].typ != STONE) continue;
		lo_xx = (x > 0) ? x - 1 : 0;
		hi_xx = (x < xstart + xsize) ? x + 1 : xstart + xsize;
		for (yy = lo_yy; yy <= hi_yy; yy++) {
		    for (xx = lo_xx; xx <= hi_xx; xx++) {
			if (IS_ROOM(lev->locations[xx][yy].typ) ||
				lev->locations[xx][yy].typ == CROSSWALL) {
			    lev->locations[x][y].typ = (yy != y) ? HWALL : VWALL;
			    yy = hi_yy;		/* end 'yy' loop */
			    break;		/* end 'xx' loop */
			}
		    }
		}
	    }
	}
}

/*
 * Select a random coordinate in the maze.
 *
 * We want a place not 'touched' by the loader.  That is, a place in
 * the maze outside every part of the special level.
 */
static void maze1xy(struct level *lev, coord *m, int humidity)
{
	int x, y, tryct = 2000;
	/* tryct:  normally it won't take more than ten or so tries due
	   to the circumstances under which we'll be called, but the
	   `humidity' screening might drastically change the chances */

	do {
	    x = rn1(x_maze_max - 3, 3);
	    y = rn1(y_maze_max - 3, 3);
	    if (--tryct < 0) break;	/* give up */
	} while (!(x % 2) || !(y % 2) || !SpLev_Map[x][y] ||
		 !is_ok_location(lev, (schar)x, (schar)y, humidity));

	m->x = (xchar)x,  m->y = (xchar)y;
}

/*
 * If there's a significant portion of maze unused by the special level,
 * we don't want it empty.
 *
 * Makes the number of traps, monsters, etc. proportional
 * to the size of the maze.
 */
static void fill_empty_maze(struct level *lev)
{
    int mapcountmax, mapcount, mapfact;
    xchar x, y;
    coord mm;

    mapcountmax = mapcount = (x_maze_max - 2) * (y_maze_max - 2);
    mapcountmax = mapcountmax / 2;

    for (x = 2; x < x_maze_max; x++) {
	for (y = 0; y < y_maze_max; y++)
	    if (!SpLev_Map[x][y]) mapcount--;
    }

    if (mapcount > (int)(mapcountmax / 10)) {
	mapfact = (int)((mapcount * 100L) / mapcountmax);
	for (x = rnd((int)(20 * mapfact) / 100); x; x--) {
	    maze1xy(lev, &mm, DRY);
	    mkobj_at(rn2(2) ? GEM_CLASS : RANDOM_CLASS,
		    lev, mm.x, mm.y, TRUE);
	}
	for (x = rnd((int)(12 * mapfact) / 100); x; x--) {
	    maze1xy(lev, &mm, DRY);
	    mksobj_at(BOULDER, lev, mm.x, mm.y, TRUE, FALSE);
	}
	for (x = rn2(2); x; x--) {
	    maze1xy(lev, &mm, DRY);
	    makemon(&mons[PM_MINOTAUR], lev, mm.x, mm.y, NO_MM_FLAGS);
	}
	for (x = rnd((int)(12 * mapfact) / 100); x; x--) {
	    maze1xy(lev, &mm, WET|DRY);
	    makemon(NULL, lev, mm.x, mm.y, NO_MM_FLAGS);
	}
	for (x = rn2((int)(15 * mapfact) / 100); x; x--) {
	    maze1xy(lev, &mm, DRY);
	    mkgold(0L, lev, mm.x, mm.y);
	}
	for (x = rn2((int)(15 * mapfact) / 100); x; x--) {
	    int trytrap;

	    maze1xy(lev, &mm, DRY);
	    trytrap = rndtrap(lev);
	    if (sobj_at(BOULDER, lev, mm.x, mm.y)) {
		while (trytrap == PIT || trytrap == SPIKED_PIT ||
			trytrap == TRAPDOOR || trytrap == HOLE) {
		    trytrap = rndtrap(lev);
		}
	    }
	    maketrap(lev, mm.x, mm.y, trytrap);
	}
    }
}

/*
 * Special level loader.
 */
static boolean sp_level_loader(struct level *lev, dlb *fd, sp_lev *lvl)
{
    long n_opcode = 0;
    long i, j;
    void *opdat;
    unsigned char n;
    int size, opcode;
    lev_region *tmplregion;
    mazepart *tmpmazepart;

    /* Read the level initialization data. */
    Fread(&lvl->init_lev, 1, sizeof(lev_init), fd);

    lvl->opcodes = malloc(sizeof(_opcode) * lvl->init_lev.n_opcodes);

    while (n_opcode < lvl->init_lev.n_opcodes) {

	Fread(&lvl->opcodes[n_opcode].opcode, 1,
		sizeof(lvl->opcodes[n_opcode].opcode), fd);
	opcode = lvl->opcodes[n_opcode].opcode;

	opdat = NULL;

	switch (opcode) {
	case SPO_POP_CONTAINER:
	case SPO_NULL:
	case SPO_EXIT:
	case SPO_WALLIFY:
	case SPO_ENDROOM:
	    break;
	case SPO_MESSAGE:
	    Fread(&n, 1, sizeof(n), fd);
	    if (n) {
		char *msg;
		opdat = malloc(n + 1);
		Fread(opdat, 1, n, fd);
		msg = (char *)opdat;
		msg[n] = '\0';
	    }
	    break;
	case SPO_MONSTER:
	    opdat = malloc(sizeof(monster));
	    load_one_monster(fd, opdat);
	    break;
	case SPO_OBJECT:
	    opdat = malloc(sizeof(object));
	    load_one_object(fd, opdat);
	    break;
	case SPO_ENGRAVING:
	    opdat = malloc(sizeof(engraving));
	    load_one_engraving(fd, opdat);
	    break;
	case SPO_SUBROOM:
	case SPO_ROOM:
	    opdat = malloc(sizeof(room));
	    load_one_room(lev, fd, opdat);
	    break;
	case SPO_DOOR:
	    opdat = malloc(sizeof(door));
	    Fread(opdat, 1, sizeof(door), fd);
	    break;
	case SPO_STAIR:
	    opdat = malloc(sizeof(stair));
	    Fread(opdat, 1, sizeof(stair), fd);
	    break;
	case SPO_LADDER:
	    opdat = malloc(sizeof(lad));
	    Fread(opdat, 1, sizeof(lad), fd);
	    break;
	case SPO_ALTAR:
	    opdat = malloc(sizeof(altar));
	    Fread(opdat, 1, sizeof(altar), fd);
	    break;
	case SPO_FOUNTAIN:
	    opdat = malloc(sizeof(fountain));
	    Fread(opdat, 1, sizeof(fountain), fd);
	    break;
	case SPO_SINK:
	    opdat = malloc(sizeof(sink));
	    Fread(opdat, 1, sizeof(sink), fd);
	    break;
	case SPO_POOL:
	    opdat = malloc(sizeof(pool));
	    Fread(opdat, 1, sizeof(pool), fd);
	    break;
	case SPO_TRAP:
	    opdat = malloc(sizeof(trap));
	    Fread(opdat, 1, sizeof(trap), fd);
	    break;
	case SPO_GOLD:
	    opdat = malloc(sizeof(gold));
	    Fread(opdat, 1, sizeof(gold), fd);
	    break;
	case SPO_CORRIDOR:
	    opdat = malloc(sizeof(corridor));
	    Fread(opdat, 1, sizeof(corridor), fd);
	    break;
	case SPO_LEVREGION:
	    opdat = malloc(sizeof(lev_region));
	    tmplregion = (lev_region *)opdat;
	    Fread(opdat, sizeof(lev_region), 1, fd);
	    size = tmplregion->rname.len;
	    if (size != 0) {
		tmplregion->rname.str = malloc((unsigned)size + 1);
		Fread(tmplregion->rname.str, size, 1, fd);
		tmplregion->rname.str[size] = '\0';
	    } else {
		tmplregion->rname.str = NULL;
	    }
	    break;
	case SPO_REGION:
	    opdat = malloc(sizeof(region));
	    Fread(opdat, 1, sizeof(region), fd);
	    break;
	case SPO_RANDOM_OBJECTS:
	    Fread(&n, 1, sizeof(n), fd);
	    if (n > 0 && n <= MAX_REGISTERS) {
		char *msg;
		opdat = malloc(n+1);
		Fread(opdat, 1, n, fd);
		msg = (char *)opdat;
		msg[n] = '\0';
	    } else panic("sp_level_loader: rnd_objs idx out-of-bounds (%i)", n);
	    break;
	case SPO_RANDOM_PLACES:
	    Fread(&n, 1, sizeof(n), fd);
	    if (n > 0 && n <= (2 * MAX_REGISTERS)) {
		char *tmpstr = malloc(n+1);
		Fread(tmpstr, 1, n, fd);
		tmpstr[n] = '\0';
		opdat = tmpstr;
	    } else panic("sp_level_loader: rnd_places idx out-of-bounds (%i)", n);
	    break;
	case SPO_RANDOM_MONSTERS:
	    Fread(&n, 1, sizeof(n), fd);
	    if (n > 0 && n <= MAX_REGISTERS) {
		char *tmpstr = malloc(n+1);
		Fread(tmpstr, 1, n, fd);
		tmpstr[n] = '\0';
		opdat = tmpstr;
	    } else panic("sp_level_loader: rnd_mons idx out-of-bounds (%i)", n);
	    break;
	case SPO_DRAWBRIDGE:
	    opdat = malloc(sizeof(drawbridge));
	    Fread(opdat, 1, sizeof(drawbridge), fd);
	    break;
	case SPO_MAZEWALK:
	    opdat = malloc(sizeof(walk));
	    Fread(opdat, 1, sizeof(walk), fd);
	    break;
	case SPO_NON_DIGGABLE:
	case SPO_NON_PASSWALL:
	    opdat = malloc(sizeof(digpos));
	    Fread(opdat, 1, sizeof(digpos), fd);
	    break;
	case SPO_ROOM_DOOR:
	    opdat = malloc(sizeof(room_door));
	    Fread(opdat, 1, sizeof(room_door), fd);
	    break;
	case SPO_CMP:
	    opdat = malloc(sizeof(opcmp));
	    Fread(opdat, 1, sizeof(opcmp), fd);
	    break;
	case SPO_JMP:
	case SPO_JL:
	case SPO_JG:
	    opdat = malloc(sizeof(opjmp));
	    Fread(opdat, 1, sizeof(opjmp), fd);
	    break;
	case SPO_REPLACETERRAIN:
	    opdat = malloc(sizeof(replaceterrain));
	    Fread(opdat, 1, sizeof(replaceterrain), fd);
	    break;
	case SPO_TERRAIN:
	    opdat = malloc(sizeof(terrain));
	    Fread(opdat, 1, sizeof(terrain), fd);
	    break;
	case SPO_RANDLINE:
	    opdat = malloc(sizeof(randline));
	    Fread(opdat, 1, sizeof(randline), fd);
	    break;
	case SPO_SPILL:
	    opdat = malloc(sizeof(spill));
	    Fread(opdat, 1, sizeof(spill), fd);
	    break;
	case SPO_MAP:
	    opdat = malloc(sizeof(mazepart));
	    tmpmazepart = (mazepart *)opdat;
	    Fread(&tmpmazepart->zaligntyp, 1, sizeof(tmpmazepart->zaligntyp), fd);
	    Fread(&tmpmazepart->keep_region, 1, sizeof(tmpmazepart->keep_region), fd);
	    Fread(&tmpmazepart->halign, 1, sizeof(tmpmazepart->halign), fd);
	    Fread(&tmpmazepart->valign, 1, sizeof(tmpmazepart->valign), fd);
	    Fread(&tmpmazepart->xsize, 1, sizeof(tmpmazepart->xsize), fd);
	    Fread(&tmpmazepart->ysize, 1, sizeof(tmpmazepart->ysize), fd);
	    if (tmpmazepart->xsize > 0 && tmpmazepart->ysize > 0) {
		tmpmazepart->map = malloc(tmpmazepart->ysize * sizeof(char *));
		for (i = 0; i < tmpmazepart->ysize; i++) {
		    tmpmazepart->map[i] = malloc(tmpmazepart->xsize);
		    for (j = 0; j < tmpmazepart->xsize; j++)
			tmpmazepart->map[i][j] = Fgetc(fd);
		}
	    }
	    break;
	default:
	    panic("sp_level_loader: Unknown opcode %i", opcode);
	}

	lvl->opcodes[n_opcode].opdat = opdat;
	n_opcode++;
    } /* while */

    return TRUE;

err_out:
    fprintf(stderr, "read error in sp_level_loader\n");
    return FALSE;
}

/*
 * Free the memory allocated for special level creation structs.
 */
static boolean sp_level_free(sp_lev *lvl)
{
    long n_opcode = 0;
    monster *tmpmons;
    object *tmpobj;
    engraving *tmpengraving;
    room *tmproom;
    mazepart *mpart;

    while (n_opcode < lvl->init_lev.n_opcodes) {
	int opcode = lvl->opcodes[n_opcode].opcode;
	void *opdat = lvl->opcodes[n_opcode].opdat;

	switch (opcode) {
	case SPO_CMP:
	case SPO_JMP:
	case SPO_JL:
	case SPO_JG:
	case SPO_NULL:
	case SPO_EXIT:
	case SPO_POP_CONTAINER:
	case SPO_MESSAGE:
	case SPO_DOOR:
	case SPO_ENDROOM:
	case SPO_STAIR:
	case SPO_LADDER:
	case SPO_ALTAR:
	case SPO_FOUNTAIN:
	case SPO_SINK:
	case SPO_POOL:
	case SPO_TRAP:
	case SPO_GOLD:
	case SPO_CORRIDOR:
	case SPO_REGION:
	case SPO_RANDOM_OBJECTS:
	case SPO_RANDOM_PLACES:
	case SPO_RANDOM_MONSTERS:
	case SPO_DRAWBRIDGE:
	case SPO_MAZEWALK:
	case SPO_NON_DIGGABLE:
	case SPO_NON_PASSWALL:
	case SPO_ROOM_DOOR:
	case SPO_WALLIFY:
	case SPO_TERRAIN:
	case SPO_REPLACETERRAIN:
	case SPO_RANDLINE:
	case SPO_SPILL:
	    /* nothing extra to free here */
	    break;
	case SPO_SUBROOM:
	case SPO_ROOM:
	    tmproom = (room *)opdat;
	    if (tmproom) {
		Free(tmproom->name.str);
		Free(tmproom->parent.str);
	    }
	    break;
	case SPO_LEVREGION:
	    /* lev_regions are moved to lregions, and used in mkmaze.c,
	       so do not free them here! */
	    break;
	case SPO_MONSTER:
	    tmpmons = (monster *)opdat;
	    if (tmpmons) {
		Free(tmpmons->name.str);
		Free(tmpmons->appear_as.str);
	    }
	    break;
	case SPO_OBJECT:
	    tmpobj = (object *)opdat;
	    if (tmpobj) {
		Free(tmpobj->name.str);
	    }
	    break;
	case SPO_ENGRAVING:
	    tmpengraving = (engraving *)opdat;
	    if (tmpengraving) {
		Free(tmpengraving->engr.str);
	    }
	    break;
	case SPO_MAP:
	    mpart = (mazepart *)opdat;
	    if (mpart) {
		int j;
		if (mpart->xsize > 0 && mpart->ysize > 0) {
		    for (j = 0; j < mpart->ysize; j++) {
			Free(mpart->map[j]);
		    }
		    Free(mpart->map);
		}
	    }
	    break;
	default:
	    panic("sp_level_free: Unknown opcode %i", opcode);
	}
	Free(opdat);
	n_opcode++;
    } /* while */

    free(lvl->opcodes);

    return TRUE;
}

/*
 * Special level coder; creates the special level from the sp_lev codes.
 * Does not free the allocated memory.
 */
static boolean sp_level_coder(struct level *lev, sp_lev *lvl)
{
    long n_opcode = 0;
    long exec_opcodes = 0;
    boolean exit_script = FALSE;
    int tmpidx;
    int cpu_flags = 0;
    char *tmpstr;
    monster *tmpmons;
    object *tmpobj;
    engraving *tmpengraving;
    door *tmpdoor;
    stair *tmpstair;
    stair prevstair;
    lad *tmplad;
    altar *tmpaltar;
    fountain *tmpfountain;
    trap *tmptrap;
    gold *tmpgold;
    lev_region *tmplregion;
    region *tmpregion;
    drawbridge *tmpdb;
    walk *tmpwalk;
    digpos *tmpdig;
    mazepart *tmpmazepart;
    sink *tmpsink;
    pool *tmppool;
    corridor *tmpcorridor;
    terrain *tmpterrain;
    randline *tmprandline;
    replaceterrain *tmpreplaceterrain;
    spill *tmpspill;
    room *tmproom, *tmpsubroom;
    room_door *tmproomdoor;
    struct mkroom *croom,
	    *mkr = NULL,
	    *mkrsub = NULL;

    xchar x, y, typ;
    boolean prefilled, room_not_needed;

    char n = '\0';
    schar halign, valign;

    int xi, dir;
    int tmpi;
    int allow_flips = 3;
    int room_build_fail = 0;

    xchar tmpxstart, tmpystart, tmpxsize, tmpysize;

    struct trap *badtrap;
    boolean has_bounds = FALSE;
    boolean premapped = FALSE;

    prevstair.x = prevstair.y = 0;
    tmproom = tmpsubroom = NULL;

    shuffle_alignments();

    for (xi = 0; xi < MAX_CONTAINMENT; xi++)
	container_obj[xi] = NULL;
    container_idx = 0;

    memset(&SpLev_Map[0][0], 0, sizeof(SpLev_Map));

    lev->flags.is_maze_lev = 0;

    switch (lvl->init_lev.init_style) {
	case LVLINIT_NONE:
	    break;
	case LVLINIT_SOLIDFILL:
	    lvlfill_solid(lev, lvl->init_lev.filling);
	    xstart = 1;
	    ystart = 0;
	    xsize = COLNO - 1;
	    ysize = ROWNO;
	    break;
	case LVLINIT_MAZEGRID:
	    lvlfill_maze_grid(lev, 2,0, x_maze_max,y_maze_max, lvl->init_lev.filling);
	    xstart = 1;
	    ystart = 0;
	    xsize = COLNO - 1;
	    ysize = ROWNO;
	    break;
	case LVLINIT_MINES:
	    if (lvl->init_lev.lit < 0)
		lvl->init_lev.lit = rn2(2);
	    if (lvl->init_lev.filling > -1)
		lvlfill_solid(lev, lvl->init_lev.filling);
	    mkmap(lev, &(lvl->init_lev));
	    xstart = 1;
	    ystart = 0;
	    xsize = COLNO - 1;
	    ysize = ROWNO;
	    break;
	default:
	    impossible("Unrecognized level init style.");
	    break;
    }

    if (lvl->init_lev.flags & NOTELEPORT)   lev->flags.noteleport = 1;
    if (lvl->init_lev.flags & HARDFLOOR)    lev->flags.hardfloor = 1;
    if (lvl->init_lev.flags & NOMMAP)	    lev->flags.nommap = 1;
    if (lvl->init_lev.flags & SHORTSIGHTED) lev->flags.shortsighted = 1;
    if (lvl->init_lev.flags & ARBOREAL)	    lev->flags.arboreal = 1;
    if (lvl->init_lev.flags & NOFLIPX)	    allow_flips &= ~1;
    if (lvl->init_lev.flags & NOFLIPY)	    allow_flips &= ~2;
    if (lvl->init_lev.flags & MAZELEVEL)    lev->flags.is_maze_lev = 1;
    if (lvl->init_lev.flags & PREMAPPED)    premapped = TRUE;
    if (lvl->init_lev.flags & SHROUD)	    lev->flags.hero_memory = 0;

    while (n_opcode < lvl->init_lev.n_opcodes && !exit_script) {
	int opcode = lvl->opcodes[n_opcode].opcode;
	void *opdat = lvl->opcodes[n_opcode].opdat;

	if (exec_opcodes++ > SPCODER_MAX_RUNTIME) {
	    impossible("Level script is taking too much time, stopping.");
	    exit_script = TRUE;
	}

	croom = mkrsub ? mkrsub : mkr;

	if (room_build_fail &&
	    opcode != SPO_ENDROOM &&
	    opcode != SPO_ROOM &&
	    opcode != SPO_SUBROOM)
	    goto next_opcode;

	switch (opcode) {
	case SPO_NULL:
	    break;
	case SPO_EXIT:
	    exit_script = TRUE;
	    break;
	case SPO_POP_CONTAINER:
	    if (container_idx > 0) {
		container_idx--;
		container_obj[container_idx] = NULL;
	    }
	    break;
	case SPO_MESSAGE:
	    if (opdat) {
		char *msg = (char *)opdat;
		char *levmsg;
		int old_n = lev_message ? (strlen(lev_message) + 1) : 0;
		n = strlen(msg);
		levmsg = malloc(old_n + n + 1);
		if (old_n)
		    levmsg[old_n - 1] = '\n';
		if (lev_message)
		    memcpy(levmsg, lev_message, old_n - 1);
		memcpy(&levmsg[old_n], opdat, n);
		levmsg[old_n + n] = '\0';
		Free(lev_message);
		lev_message = levmsg;
	    }
	    break;
	case SPO_MONSTER:
	    tmpmons = (monster *)opdat;
	    if (tmpmons) create_monster(lev, tmpmons, croom);
	    break;
	case SPO_OBJECT:
	    tmpobj = (object *)opdat;
	    if (tmpobj) create_object(lev, tmpobj, croom);
	    break;
	case SPO_ENGRAVING:
	    tmpengraving = (engraving *)opdat;
	    if (tmpengraving)
		create_engraving(lev, tmpengraving, croom);
	    break;
	case SPO_SUBROOM:
	    if (!room_build_fail) {
		tmpsubroom = (room *)opdat;
		if (!mkr) {
		    panic("Subroom without a parent room?!");
		} else if (!tmpsubroom) panic("Subroom without data?");
		croom = build_room(lev, tmpsubroom, mkr);
		if (croom) mkrsub = croom;
		else room_build_fail++;
	    } else room_build_fail++; /* room failed to get built, fail subroom too */
	    break;
	case SPO_ROOM:
	    if (!room_build_fail) {
		tmproom = (room *)opdat;
		tmpsubroom = NULL;
		mkrsub = NULL;
		if (!tmproom) panic("Room without data?");
		croom = build_room(lev, tmproom, NULL);
		if (croom) mkr = croom;
		else room_build_fail++;
	    } else room_build_fail++; /* one room failed alreaedy, fail this one too */
	    break;
	case SPO_ENDROOM:
	    if (mkrsub) {
		mkrsub = NULL; /* get out of subroom */
	    } else if (mkr) {
		mkr = NULL; /* no subroom, get out of top-level room */
		/* Need to ensure xstart/ystart/xsize/ysize have something sensible,
		 * in case there's some stuff to be created outside the outermost room,
		 * and there's no MAP. */
		if (xsize <= 1 && ysize <= 1) {
		    xstart = 1;
		    ystart = 0;
		    xsize = COLNO - 1;
		    ysize = ROWNO;
		}
	    }
	    if (room_build_fail > 0) room_build_fail--;
	    break;
	case SPO_DOOR:
	    croom = &lev->rooms[0];

	    tmpdoor = (door *)opdat;
	    x = tmpdoor->x;
	    y = tmpdoor->y;
	    typ = tmpdoor->mask == -1 ? rnddoor() : tmpdoor->mask;

	    get_location(lev, &x, &y, DRY, NULL);
	    if (lev->locations[x][y].typ != SDOOR) {
		lev->locations[x][y].typ = DOOR;
	    } else {
		if (typ < D_CLOSED)
		    typ = D_CLOSED; /* force it to be closed */
	    }
	    lev->locations[x][y].doormask = typ;

	    /* Now the complicated part: list it with each subroom.
	     * The dog move and mail daemon routines use this. */
	    while (croom->hx >= 0 && lev->doorindex < DOORMAX) {
		if (croom->hx >= x - 1 && croom->lx <= x + 1 &&
		    croom->hy >= y - 1 && croom->ly <= y + 1) {
		    /* Found it! */
		    add_door(lev, x, y, croom);
		}
		croom++;
	    }
	    break;
	case SPO_STAIR:
	    tmpstair = (stair *)opdat;
	    if (croom) {
		create_stairs(lev, tmpstair, croom);
	    } else {
		xi = 0;
		do {
		    x = tmpstair->x;  y = tmpstair->y;
		    get_location(lev, &x, &y, DRY, croom);
		} while (prevstair.x && xi++ < 100 &&
			 distmin(x, y, prevstair.x, prevstair.y) <= 8);
		if ((badtrap = t_at(lev, x, y)) != 0) deltrap(lev, badtrap);
		mkstairs(lev, x, y, (char)tmpstair->up, croom);
		prevstair.x = x;
		prevstair.y = y;
	    }
	    break;
	case SPO_LADDER:
	    tmplad = (lad *)opdat;

	    x = tmplad->x;  y = tmplad->y;
	    get_location(lev, &x, &y, DRY, croom);

	    lev->locations[x][y].typ = LADDER;
	    if (tmplad->up == 1) {
		lev->upladder.sx = x;  lev->upladder.sy = y;
		lev->locations[x][y].ladder = LA_UP;
	    } else {
		lev->dnladder.sx = x;  lev->dnladder.sy = y;
		lev->locations[x][y].ladder = LA_DOWN;
	    }
	    break;
	case SPO_ALTAR:
	    tmpaltar = (altar *)opdat;
	    create_altar(lev, tmpaltar, croom);
	    break;
	case SPO_FOUNTAIN:
	    tmpfountain = (fountain *)opdat;
	    create_feature(lev, tmpfountain->x, tmpfountain->y, croom, FOUNTAIN);
	    break;
	case SPO_SINK:
	    tmpsink = (sink *)opdat;
	    create_feature(lev, tmpsink->x, tmpsink->y, croom, SINK);
	    break;
	case SPO_POOL:
	    tmppool = (pool *)opdat;
	    create_feature(lev, tmppool->x, tmppool->y, croom, POOL);
	    break;
	case SPO_TRAP:
	    tmptrap = (trap *)opdat;
	    create_trap(lev, tmptrap, croom);
	    break;
	case SPO_GOLD:
	    tmpgold = (gold *)opdat;
	    create_gold(lev, tmpgold, croom);
	    break;
	case SPO_CORRIDOR:
	    tmpcorridor = (corridor *)opdat;
	    create_corridor(lev, tmpcorridor);
	    break;
	case SPO_TERRAIN:
	    tmpterrain = (terrain *)opdat;
	    set_terrain(lev, tmpterrain, croom);
	    break;
	case SPO_REPLACETERRAIN:
	    tmpreplaceterrain = (replaceterrain *)opdat;
	    replace_terrain(lev, tmpreplaceterrain, croom);
	    break;
	case SPO_RANDLINE:
	    tmprandline = (randline *)opdat;
	    line_midpoint(lev, tmprandline, croom);
	    break;
	case SPO_SPILL:
	    tmpspill = (spill *)opdat;
	    spill_terrain(lev, tmpspill, croom);
	    break;
	case SPO_LEVREGION:
	    tmplregion = (lev_region *)opdat;
	    if (!tmplregion->in_islev) {
		get_location(lev, &tmplregion->inarea.x1, &tmplregion->inarea.y1,
			     DRY|WET, NULL);
		get_location(lev, &tmplregion->inarea.x2, &tmplregion->inarea.y2,
			     DRY|WET, NULL);
	    }
	    if (!tmplregion->del_islev) {
		get_location(lev, &tmplregion->delarea.x1, &tmplregion->delarea.y1,
			     DRY|WET, NULL);
		get_location(lev, &tmplregion->delarea.x2, &tmplregion->delarea.y2,
			     DRY|WET, NULL);
	    }
	    if (num_lregions) {
		/* realloc the lregion space to add the new one */
		lev_region *newl = malloc(sizeof(lev_region) *
			(unsigned)(1 + num_lregions));
		memcpy(newl, lregions, sizeof(lev_region) * num_lregions);
		Free(lregions);
		num_lregions += 1;
		lregions = newl;
	    } else {
		num_lregions = 1;
		lregions = malloc(sizeof(lev_region) * (unsigned)1);
	    }
	    memcpy(&lregions[num_lregions - 1], tmplregion, sizeof(lev_region));
	    break;
	case SPO_REGION:
	    {
		struct mkroom *troom;
		tmpregion = (region *)opdat;
		if (tmpregion->rtype > MAXRTYPE) {
		    tmpregion->rtype -= MAXRTYPE + 1;
		    prefilled = TRUE;
		} else {
		    prefilled = FALSE;
		}

		if (tmpregion->rlit < 0) {
		    tmpregion->rlit = (rnd(1 + abs(depth(&lev->z))) < 11 && rn2(77)) ?
			    TRUE : FALSE;
		}

		get_location(lev, &tmpregion->x1, &tmpregion->y1, DRY|WET, NULL);
		get_location(lev, &tmpregion->x2, &tmpregion->y2, DRY|WET, NULL);

		/* For an ordinary room, 'prefilled' is a flag to force
		   an actual room to be created (such rooms are used to
		   control placement of migrating monster arrivals). */
		room_not_needed = (tmpregion->rtype == OROOM &&
				   !tmpregion->rirreg && !prefilled);
		if (room_not_needed || lev->nroom >= MAXNROFROOMS) {
		    if (!room_not_needed)
			warning("Too many rooms on new level!");
		    light_region(lev, tmpregion);
		    goto next_opcode;
		}

		troom = &lev->rooms[lev->nroom];

		/* mark rooms that must be filled, but do it later */
		if (tmpregion->rtype != OROOM)
		    troom->needfill = (prefilled ? 2 : 1);

		if (tmpregion->rirreg) {
		    min_rx = max_rx = tmpregion->x1;
		    min_ry = max_ry = tmpregion->y1;
		    flood_fill_rm(lev, tmpregion->x1, tmpregion->y1,
				  lev->nroom + ROOMOFFSET, tmpregion->rlit, TRUE);
		    add_room(lev, min_rx, min_ry, max_rx, max_ry,
			     FALSE, tmpregion->rtype, TRUE);
		    troom->rlit = tmpregion->rlit;
		    troom->irregular = TRUE;
		} else {
		    add_room(lev, tmpregion->x1, tmpregion->y1,
			     tmpregion->x2, tmpregion->y2,
			     tmpregion->rlit, tmpregion->rtype, TRUE);
		    topologize(lev, troom);
		}
	    }
	    break;
	case SPO_RANDOM_OBJECTS:
	    tmpstr = (char *)opdat;
	    n_robj = strlen(tmpstr);
	    if (n_robj <= 0 || n_robj > MAX_REGISTERS)
		panic("sp_level_coder: rnd_objs idx out-of-bounds (%i)", n_robj);
	    memcpy(robjects, tmpstr, n_robj);
	    sp_lev_shuffle(robjects, NULL, n_robj);
	    break;
	case SPO_RANDOM_PLACES:
	    tmpstr = (char *)opdat;
	    n_rloc = strlen(tmpstr);
	    if (n_rloc <= 0 || n_rloc > 2 * MAX_REGISTERS)
		panic("sp_level_coder: rnd_places idx out-of-bounds (%i)", n_rloc);
	    n_rloc = n_rloc / 2;
	    for (tmpidx = 0; tmpidx < n_rloc; tmpidx++) {
		rloc_x[tmpidx] = tmpstr[tmpidx * 2] - 1;
		rloc_y[tmpidx] = tmpstr[tmpidx * 2 + 1] - 1;
	    }
	    sp_lev_shuffle(rloc_x, rloc_y, n_rloc);
	    break;
	case SPO_RANDOM_MONSTERS:
	    tmpstr = (char *)opdat;
	    n_rmon = strlen(tmpstr);
	    if (n_rmon <= 0 || n_rmon > MAX_REGISTERS)
		panic("sp_level_coder: rnd_mons idx out-of-bounds (%i)", n_rmon);
	    memcpy(rmonst, tmpstr, n_rmon);
	    sp_lev_shuffle(rmonst, NULL, n_rmon);
	    break;
	case SPO_DRAWBRIDGE:
	    tmpdb = (drawbridge *)opdat;

	    x = tmpdb->x;  y = tmpdb->y;
	    get_location(lev, &x, &y, DRY|WET, croom);

	    if (!create_drawbridge(lev, x, y, tmpdb->dir, tmpdb->db_open))
		impossible("Cannot create drawbridge.");
	    break;
	case SPO_MAZEWALK:
	    tmpwalk = (walk *)opdat;

	    get_location(lev, &tmpwalk->x, &tmpwalk->y, DRY|WET, NULL);

	    x = (xchar)tmpwalk->x;  y = (xchar)tmpwalk->y;
	    dir = tmpwalk->dir;

	    if (tmpwalk->typ < 1)
		tmpwalk->typ = ROOM;

	    /* don't use move() - it doesn't use W_NORTH, etc. */
	    switch (dir) {
	    case W_NORTH: --y; break;
	    case W_SOUTH: y++; break;
	    case W_EAST:  x++; break;
	    case W_WEST:  --x; break;
	    default: panic("sp_level_coder: bad MAZEWALK direction");
	    }

	    if (!IS_DOOR(lev->locations[x][y].typ)) {
		lev->locations[x][y].typ = tmpwalk->typ;
		lev->locations[x][y].flags = 0;
	    }

	    /*
	     * We must be sure that the parity of the coordinates for
	     * walkfrom() is odd.  But we must also take into account
	     * what direction was chosen.
	     */
	    if (!(x % 2)) {
		if (dir == W_EAST)
		    x++;
		else
		    x--;

		/* no need for IS_DOOR check; out of map bounds */
		lev->locations[x][y].typ = tmpwalk->typ;
		lev->locations[x][y].flags = 0;
	    }

	    if (!(y % 2)) {
		if (dir == W_SOUTH)
		    y++;
		else
		    y--;
	    }

	    walkfrom(lev, x, y, tmpwalk->typ);
	    if (tmpwalk->stocked) fill_empty_maze(lev);
	    break;
	case SPO_NON_DIGGABLE:
	    tmpdig = (digpos *)opdat;

	    get_location(lev, &tmpdig->x1, &tmpdig->y1, DRY|WET, NULL);
	    get_location(lev, &tmpdig->x2, &tmpdig->y2, DRY|WET, NULL);

	    set_wall_property(lev, tmpdig->x1, tmpdig->y1,
			      tmpdig->x2, tmpdig->y2, W_NONDIGGABLE);
	    break;
	case SPO_NON_PASSWALL:
	    tmpdig = (digpos *)opdat;

	    get_location(lev, &tmpdig->x1, &tmpdig->y1, DRY|WET, NULL);
	    get_location(lev, &tmpdig->x2, &tmpdig->y2, DRY|WET, NULL);

	    set_wall_property(lev, tmpdig->x1, tmpdig->y1,
			      tmpdig->x2, tmpdig->y2, W_NONPASSWALL);
	    break;
	case SPO_ROOM_DOOR:
	    tmproomdoor = (room_door *)opdat;
	    if (!croom) impossible("Room_door without room?");
	    create_door(lev, tmproomdoor, croom);
	    break;
	case SPO_WALLIFY:
	    wallify_map(lev);
	    break;
	case SPO_CMP:
	    {
		opcmp *tmpcmp = (opcmp *)opdat;
		int tmpval = 0;
		if (tmpcmp->cmp_what == 0) tmpval = rn2(100);
		cpu_flags = 0;
		if (tmpval < tmpcmp->cmp_val) cpu_flags += SP_CPUFLAG_LT;
		if (tmpval > tmpcmp->cmp_val) cpu_flags += SP_CPUFLAG_GT;
		if (tmpval == tmpcmp->cmp_val) cpu_flags += SP_CPUFLAG_EQ;
	    }
	    break;
	case SPO_JMP:
	    {
		opjmp *tmpjmp = (opjmp *)opdat;
		if (tmpjmp->jmp_target >= 0 &&
		    tmpjmp->jmp_target < lvl->init_lev.n_opcodes)
		    n_opcode = tmpjmp->jmp_target;
	    }
	    break;
	case SPO_JL:
	    {
		opjmp *tmpjmp = (opjmp *)opdat;
		if ((cpu_flags & SP_CPUFLAG_LT) &&
		    tmpjmp->jmp_target >= 0 &&
		    tmpjmp->jmp_target < lvl->init_lev.n_opcodes)
		    n_opcode = tmpjmp->jmp_target;
	    }
	    break;
	case SPO_JG:
	    {
		opjmp *tmpjmp = (opjmp *)opdat;
		if ((cpu_flags & SP_CPUFLAG_GT) &&
		    tmpjmp->jmp_target >= 0 &&
		    tmpjmp->jmp_target < lvl->init_lev.n_opcodes)
		    n_opcode = tmpjmp->jmp_target;
	    }
	    break;
	case SPO_MAP:
	    tmproom = tmpsubroom = NULL;
	    tmpmazepart = (mazepart *)opdat;

	    tmpxsize = xsize;  tmpysize = ysize;
	    tmpxstart = xstart;  tmpystart = ystart;

	    halign = tmpmazepart->halign;
	    valign = tmpmazepart->valign;
	    xsize = tmpmazepart->xsize;
	    ysize = tmpmazepart->ysize;

	    switch (tmpmazepart->zaligntyp) {
	    default:
	    case 0:
		break;
	    case 1:
		switch ((int)halign) {
		case LEFT:    xstart = 3;					break;
		case H_LEFT:  xstart = 2 + (x_maze_max - 2 - xsize) / 4;	break;
		case CENTER:  xstart = 2 + (x_maze_max - 2 - xsize) / 2;	break;
		case H_RIGHT: xstart = 2 + (x_maze_max - 2 - xsize) * 3 / 4;	break;
		case RIGHT:   xstart = x_maze_max - xsize - 1;			break;
		}
		switch ((int)valign) {
		case TOP:    ystart = 3;				break;
		case CENTER: ystart = 2 + (y_maze_max - 2 - ysize) / 2; break;
		case BOTTOM: ystart = y_maze_max - ysize - 1;		break;
		}
		if (!(xstart % 2)) xstart++;
		if (!(ystart % 2)) ystart++;
		break;
	    case 2:
		get_location(lev, &halign, &valign, DRY|WET, croom);
		xstart = halign;
		ystart = valign;
		break;
	    }

	    if (ystart < 0 || ystart + ysize > ROWNO) {
		/* try to move the start a bit */
		ystart += (ystart > 0) ? -2 : 2;
		if (ysize == ROWNO) ystart = 0;
		if (ystart < 0 || ystart + ysize > ROWNO)
		    panic("reading special level with ysize too large");
	    }

	    if (xsize <= 1 && ysize <= 1) {
		xstart = 1;
		ystart = 0;
		xsize = COLNO - 1;
		ysize = ROWNO;
	    } else {
		/* Load the map. */
		for (y = ystart; y < ystart + ysize; y++) {
		    for (x = xstart; x < xstart + xsize; x++) {
			if (tmpmazepart->map[y - ystart][x - xstart] >= MAX_TYPE)
			    continue;
			lev->locations[x][y].typ =
				tmpmazepart->map[y - ystart][x - xstart];
			lev->locations[x][y].lit = FALSE;
			/* clear out lev->locations: load_common_data may set them */
			lev->locations[x][y].flags = 0;
			lev->locations[x][y].horizontal = 0;
			lev->locations[x][y].roomno = 0;
			lev->locations[x][y].edge = 0;

			/*
			 *  Set secret doors to closed (why not trapped too?).
			 *  Set the horizontal bit.
			 */
			if (lev->locations[x][y].typ == SDOOR ||
				IS_DOOR(lev->locations[x][y].typ)) {
			    if (lev->locations[x][y].typ == SDOOR)
				lev->locations[x][y].doormask = D_CLOSED;
			    /*
			     *  If there is a wall to the left that connects to a
			     *  (secret) door, then it is horizontal.  This does
			     *  not allow (secret) doors to be corners of rooms.
			     */
			    if (x != xstart && (IS_WALL(lev->locations[x - 1][y].typ) ||
						lev->locations[x - 1][y].horizontal))
				lev->locations[x][y].horizontal = 1;
			} else if (lev->locations[x][y].typ == HWALL ||
				   lev->locations[x][y].typ == IRONBARS) {
			    lev->locations[x][y].horizontal = 1;
			} else if (lev->locations[x][y].typ == LAVAPOOL) {
			    lev->locations[x][y].lit = 1;
			} else if (lev->locations[x][y].typ == CROSSWALL) {
			    has_bounds = TRUE;
			}
		    }
		}
		if (lvl->init_lev.joined)
		    remove_rooms(lev, xstart, ystart, xstart + xsize, ystart + ysize);
	    }

	    if (!tmpmazepart->keep_region) {
		/* should use a stack for this stuff... */
		xstart = tmpxstart;  ystart = tmpystart;
		xsize = tmpxsize;  ysize = tmpysize;
	    }

	    break;
	default:
	    panic("sp_level_coder: Unknown opcode %i", opcode);
	}
    next_opcode:
	n_opcode++;
    } /* while */

    /* Now that we have rooms _and_ associated doors, fill the rooms. */
    for (tmpi = 0; tmpi < lev->nroom; tmpi++) {
	int m;
	if (lev->rooms[tmpi].needfill)
	    fill_room(lev, &lev->rooms[tmpi],
		      (lev->rooms[tmpi].needfill == 2) ? TRUE : FALSE);
	for (m = 0; m < lev->rooms[tmpi].nsubrooms; m++) {
	    if (lev->rooms[tmpi].sbrooms[m]->needfill)
		fill_room(lev, lev->rooms[tmpi].sbrooms[m], FALSE);
	}
    }

    /*
     * If any CROSSWALLs are found, must change to ROOM after REGION's
     * are laid out.  CROSSWALLS are used to specify "invisible"
     * boundaries where DOOR syms look bad or aren't desirable.
     */
    /* if special boundary syms (CROSSWALL) in map, remove them now */
    if (has_bounds) {
	for (x = 0; x < x_maze_max; x++) {
	    for (y = 0; y < y_maze_max; y++) {
		if (lev->locations[x][y].typ == CROSSWALL && !SpLev_Map[x][y])
		    lev->locations[x][y].typ = ROOM;
	    }
	}
    }

    wallification(lev, 1, 0, COLNO - 1, ROWNO - 1);

    /* disable level flipping for some levels */
    if (!Is_astralevel(&lev->z) &&
	!Is_blackmarket(&lev->z) &&
	!Is_knox(&lev->z) &&
	!Is_oracle_level(&lev->z) &&
	!Is_minetown_level(&lev->z) &&
	!Is_town_level(&lev->z) &&
	/* When returning from the Valley the player gets
	 * placed on the right side of the screen, regardless
	 * of flipped state. */
	!Is_stronghold(&lev->z) &&
	/* Up and down ladders should be in the same position. */
	!In_V_tower(&lev->z)) {
	    flip_level_rnd(lev, allow_flips);
    }

    count_features(lev);

    if (premapped) sokoban_detect(lev);

    return TRUE;
}

/*
 * General loader
 */
boolean load_special(struct level *lev, const char *name)
{
	dlb *fd;
	sp_lev lvl;
	boolean result = FALSE;
	struct version_info vers_info;

	fd = dlb_fopen(name, RDBMODE);
	if (!fd) return FALSE;

	Fread(&vers_info, sizeof vers_info, 1, fd);
	if (!check_version(&vers_info, name, TRUE))
	    goto give_up;

	result = sp_level_loader(lev, fd, &lvl);
	if (result) result = sp_level_coder(lev, &lvl);
	sp_level_free(&lvl);
give_up:
	dlb_fclose(fd);
	return result;

err_out:
	fprintf(stderr, "read error in load_special\n");
	return FALSE;
}

/*sp_lev.c*/
