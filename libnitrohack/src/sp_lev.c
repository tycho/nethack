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
static void create_altar(struct level *lev, altar *, struct mkroom *);
static void create_gold(struct level *lev, gold *, struct mkroom *);
static void create_feature(struct level *lev, int,int,struct mkroom *,int);
static boolean search_door(struct mkroom *, struct level *lev,
			   xchar *, xchar *, xchar, int);
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
static const aligntyp init_ralign[3] = { AM_CHAOTIC, AM_NEUTRAL, AM_LAWFUL };
static aligntyp ralign[3];
static xchar xstart, ystart;
static char xsize, ysize;

static void set_wall_property(struct level *lev, xchar,xchar,xchar,xchar,int);
static int rnddoor(void);
static int rndtrap(struct level *lev);
static void get_location(struct level *lev, schar *x, schar *y, int humidity, struct mkroom *croom);
static boolean is_ok_location(struct level *lev, schar, schar, int);
static void light_region(struct level *lev, region *tmpregion);
static void maze1xy(struct level *lev, coord *m, int humidity);
static boolean sp_level_loader(struct level *lev, dlb *fp, sp_lev *lvl);
static void create_door(struct level *lev, room_door *, struct mkroom *);
static struct mkroom *build_room(struct level *lev, room *, struct mkroom *);

char *lev_message = 0;
lev_region *lregions = 0;
int num_lregions = 0;

struct obj *container_obj[MAX_CONTAINMENT];
int container_idx = 0;

struct monst *invent_carrying_monster = NULL;

#define SPLEV_STACK_RESERVE 128


static void splev_stack_init(struct splevstack *st)
{
	if (st) {
	    st->depth = 0;
	    st->depth_alloc = SPLEV_STACK_RESERVE;
	    st->stackdata = malloc(st->depth_alloc * sizeof(struct opvar *));
	}
}

static void splev_stack_done(struct splevstack *st)
{
	if (st) {
	    int i;

	    if (st->stackdata && st->depth) {
		for (i = 0; i < st->depth; i++) {
		    switch (st->stackdata[i]->spovartyp) {
		    default:
		    case SPOVAR_NULL:
		    case SPOVAR_COORD:
		    case SPOVAR_REGION:
		    case SPOVAR_MAPCHAR:
		    case SPOVAR_MONST:
		    case SPOVAR_OBJ:
		    case SPOVAR_INT:
			break;
		    case SPOVAR_VARIABLE:
		    case SPOVAR_STRING:
			if (st->stackdata[i]->vardata.str)
			    Free(st->stackdata[i]->vardata.str);
			st->stackdata[i]->vardata.str = NULL;
			break;
		    }
		    Free(st->stackdata[i]);
		    st->stackdata[i] = NULL;
		}
	    }

	    if (st->stackdata) free(st->stackdata);
	    st->stackdata = NULL;
	    st->depth = st->depth_alloc = 0;
	    Free(st);
	}
}

static void splev_stack_push(struct splevstack *st, struct opvar *v)
{
	if (!st || !st->stackdata)
	    panic("splev_stack_push: no stackdata allocated?");

	if (st->depth >= st->depth_alloc) {
	    struct opvar **tmp = malloc((st->depth_alloc + SPLEV_STACK_RESERVE) *
				        sizeof(struct opvar *));
	    memcpy(tmp, st->stackdata, st->depth_alloc * sizeof(struct opvar *));
	    Free(st->stackdata);
	    st->stackdata = tmp;
	    st->depth_alloc += SPLEV_STACK_RESERVE;
	}

	st->stackdata[st->depth] = v;
	st->depth++;
}

static struct opvar *splev_stack_pop(struct splevstack *st)
{
	struct opvar *ret = NULL;

	if (!st) return ret;
	if (!st->stackdata)
	    panic("splev_stack_pop: no stackdata allocated?");

	if (st->depth) {
	    st->depth--;
	    ret = st->stackdata[st->depth];
	    st->stackdata[st->depth] = NULL;
	    return ret;
	} else impossible("splev_stack_pop: empty stack?");

	return ret;
}

#define OV_typ(o)	((o)->spovartyp)
#define OV_i(o)		((o)->vardata.l)
#define OV_s(o)		((o)->vardata.str)

#define OV_pop_i(x)	((x) = splev_stack_getdat(coder, SPOVAR_INT))
#define OV_pop_c(x)	((x) = splev_stack_getdat(coder, SPOVAR_COORD))
#define OV_pop_r(x)	((x) = splev_stack_getdat(coder, SPOVAR_REGION))
#define OV_pop_s(x)	((x) = splev_stack_getdat(coder, SPOVAR_STRING))
#define OV_pop(x)	((x) = splev_stack_getdat_any(coder))
#define OV_pop_typ(x, typ) ((x) = splev_stack_getdat(coder, typ))

/*
static struct opvar *opvar_new_str(char *s)
{
	struct opvar *tmpov = malloc(sizeof(struct opvar));
	if (!tmpov) panic("could not alloc opvar struct");

	tmpov->spovartyp = SPOVAR_STRING;
	if (s) {
	    int len = strlen(s);
	    tmpov->vardata.str = malloc(len + 1);
	    memcpy(tmpov->vardata.str, s, len);
	    tmpov->vardata.str[len] = '\0';
	} else {
	    tmpov->vardata.str = NULL;
	}

	return tmpov;
}
*/

static struct opvar *opvar_new_int(long i)
{
	struct opvar *tmpov = malloc(sizeof(struct opvar));
	if (!tmpov) panic("could not alloc opvar struct");

	tmpov->spovartyp = SPOVAR_INT;
	tmpov->vardata.l = i;

	return tmpov;
}

static void opvar_free_x(struct opvar *ov)
{
	if (!ov) return;

	switch (ov->spovartyp) {
	case SPOVAR_COORD:
	case SPOVAR_REGION:
	case SPOVAR_MAPCHAR:
	case SPOVAR_MONST:
	case SPOVAR_OBJ:
	case SPOVAR_INT:
	    break;
	case SPOVAR_VARIABLE:
	case SPOVAR_STRING:
	    if (ov->vardata.str)
		Free(ov->vardata.str);
	    break;
	default:
	    impossible("Unknown opvar value type (%i)!", ov->spovartyp);
	}

	Free(ov);
}

#define opvar_free(ov)						\
do {								\
	if (ov) {						\
	    opvar_free_x(ov);					\
	    (ov) = NULL;					\
	} else {						\
	    impossible("opvar_free(), %s", __FUNCTION__);	\
	}							\
} while (0)

static struct opvar *opvar_clone(struct opvar *ov)
{
	struct opvar *tmpov = malloc(sizeof(struct opvar));
	if (!tmpov) panic("could not alloc opvar struct");

	switch (ov->spovartyp) {
	case SPOVAR_COORD:
	case SPOVAR_REGION:
	case SPOVAR_MAPCHAR:
	case SPOVAR_MONST:
	case SPOVAR_OBJ:
	case SPOVAR_INT:
	    {
		tmpov->spovartyp = ov->spovartyp;
		tmpov->vardata.l = ov->vardata.l;
	    }
	    break;
	case SPOVAR_VARIABLE:
	case SPOVAR_STRING:
	    {
		int len = strlen(ov->vardata.str);
		tmpov->spovartyp = ov->spovartyp;
		tmpov->vardata.str = malloc(len + 1);
		memcpy(tmpov->vardata.str, ov->vardata.str, len);
		tmpov->vardata.str[len] = '\0';
	    }
	    break;
	default:
	    impossible("Unknown push value type (%i)!", ov->spovartyp);
	}

	return tmpov;
}

static struct opvar *opvar_var_conversion(struct sp_coder *coder, struct opvar *ov)
{
	struct splev_var *tmp;
	struct opvar *tmpov;
	struct opvar *array_idx = NULL;

	if (!coder || !ov) return NULL;
	if (ov->spovartyp != SPOVAR_VARIABLE) return ov;

	tmp = coder->frame->variables;
	while (tmp) {
	    if (!strcmp(tmp->name, OV_s(ov))) {
		if (tmp->svtyp & SPOVAR_ARRAY) {
		    array_idx = opvar_var_conversion(coder,
						     splev_stack_pop(coder->stack));
		    if (!array_idx || OV_typ(array_idx) != SPOVAR_INT)
			panic("array idx not an int");
		    if (tmp->array_len < 1)
			panic("array len < 1");
		    OV_i(array_idx) = (OV_i(array_idx) % tmp->array_len);
		    tmpov = opvar_clone(tmp->data.arrayvalues[OV_i(array_idx)]);
		    return tmpov;
		} else {
		    tmpov = opvar_clone(tmp->data.value);
		    return tmpov;
		}
	    }
	    tmp = tmp->next;
	}

	return NULL;
}

static struct splev_var *opvar_var_defined(struct sp_coder *coder, char *name)
{
	struct splev_var *tmp;

	if (!coder) return NULL;

	tmp = coder->frame->variables;
	while (tmp) {
	    if (!strcmp(tmp->name, name)) return tmp;
	    tmp = tmp->next;
	}

	return NULL;
}

static struct opvar *splev_stack_getdat(struct sp_coder *coder, xchar typ)
{
	if (coder && coder->stack) {
	    struct opvar *tmp = splev_stack_pop(coder->stack);
	    if (tmp->spovartyp == SPOVAR_VARIABLE)
		tmp = opvar_var_conversion(coder, tmp);
	    if (tmp->spovartyp == typ)
		return tmp;
	}
	return NULL;
}

static struct opvar *splev_stack_getdat_any(struct sp_coder *coder)
{
	if (coder && coder->stack) {
	    struct opvar *tmp = splev_stack_pop(coder->stack);
	    if (tmp->spovartyp == SPOVAR_VARIABLE)
		tmp = opvar_var_conversion(coder, tmp);
	    return tmp;
	}
	return NULL;
}

static void variable_list_del(struct splev_var *varlist)
{
	struct splev_var *tmp = varlist;

	if (!tmp) return;

	while (tmp) {
	    free(tmp->name);
	    if (tmp->svtyp & SPOVAR_ARRAY) {
		long idx = tmp->array_len;
		while (idx-- > 0)
		    opvar_free(tmp->data.arrayvalues[idx]);
		free(tmp->data.arrayvalues);
	    } else {
		opvar_free(tmp->data.value);
	    }
	    tmp = varlist->next;
	    free(varlist);
	    varlist = tmp;
	}
}

#define SET_TYPLIT(lev, x, y, ttyp, llit)			\
{								\
	(lev)->locations[(x)][(y)].typ = (ttyp);		\
	if ((ttyp) == LAVAPOOL) {				\
	    (lev)->locations[(x)][(y)].lit = 1;			\
	} else if ((llit) != -2) {				\
	    if ((llit) == -1)					\
		(lev)->locations[(x)][(y)].lit = rn2(2);	\
	    else						\
		(lev)->locations[(x)][(y)].lit = (llit);	\
	}							\
}

static void lvlfill_maze_grid(struct level *lev, int x1, int y1, int x2, int y2,
			      schar filling)
{
	int x, y;

	for (x = x1; x <= x2; x++)
	    for (y = y1; y <= y2; y++)
		lev->locations[x][y].typ =
			(y < 2 || (x % 2 && y % 2)) ? STONE : filling;
}

static void lvlfill_solid(struct level *lev, schar filling, schar lit)
{
	int x, y;

	for (x = 2; x <= x_maze_max; x++) {
	    for (y = 0; y <= y_maze_max; y++) {
		SET_TYPLIT(lev, x, y, filling, lit);
	    }
	}
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

	for (otmp = lev->buriedobjlist; otmp; otmp = otmp->nobj) {
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
	/* shuffle 3 alignments */
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

static void remove_boundary_syms(struct level *lev)
{
	/*
	 * If any CROSSWALLs are found, must change to ROOM after REGION's
	 * are laid out.  CROSSWALLS are used to specify "invisible"
	 * boundaries where DOOR syms look bad or aren't desirable.
	 */
	xchar x, y;
	boolean has_bounds = FALSE;
	for (x = 0; x < COLNO - 1; x++) {
	    for (y = 0; y < ROWNO - 1; y++) {
		if (lev->locations[x][y].typ == CROSSWALL) {
		    has_bounds = TRUE;
		    break;
		}
	    }
	}
	if (has_bounds) {
	    for (x = 0; x < x_maze_max; x++) {
		for (y = 0; y < y_maze_max; y++) {
		    if (lev->locations[x][y].typ == CROSSWALL && !SpLev_Map[x][y])
			lev->locations[x][y].typ = ROOM;
		}
	    }
	}
}

static void fill_rooms(struct level *lev)
{
	int tmpi;
	for (tmpi = 0; tmpi < lev->nroom; tmpi++) {
	    int m;

	    if (lev->rooms[tmpi].needfill)
		fill_room(lev, &lev->rooms[tmpi], (lev->rooms[tmpi].needfill == 2));

	    for (m = 0; m < lev->rooms[tmpi].nsubrooms; m++)
		if (lev->rooms[tmpi].sbrooms[m]->needfill)
		    fill_room(lev, lev->rooms[tmpi].sbrooms[m], FALSE);
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
 *	If x or y is < 0, we generate a random coordinate.
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
	} else {			/* random location */
	    do {
		*x = mx + rn2((int)sx);
		*y = my + rn2((int)sy);
		if (is_ok_location(lev, *x, *y, humidity)) break;
	    } while (++cpt < 100);
	    if (cpt >= 100) {
		int xx, yy;
		/* last try */
		for (xx = 0; xx < sx; xx++) {
		    for (yy = 0; yy < sy; yy++) {
			*x = mx + xx;
			*y = my + yy;
			if (is_ok_location(lev, *x, *y, humidity)) goto found_it;
		    }
		}
		impossible("get_location:  can't find a place!");
	    }
	}
found_it:

	if (!isok(*x, *y)) {
	    warning("get_location:  (%d,%d) out of bounds", *x, *y);
	    *x = x_maze_max;
	    *y = y_maze_max;
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
	    if (is_pool(lev, x, y) || is_lava(lev, x, y))
		return TRUE;
	}
	return FALSE;
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
	int x, y, hix = *lowx + *ddx, hiy = *lowy + *ddy;
	struct rm *loc;
	int xlim, ylim, ymax;

	xlim = XLIM + (vault ? 1 : 0);
	ylim = YLIM + (vault ? 1 : 0);

	if (*lowx < 3)		*lowx = 3;
	if (*lowy < 2)		*lowy = 2;
	if (hix > COLNO-3)	hix = COLNO - 3;
	if (hiy > ROWNO-3)	hiy = ROWNO - 3;
chk:
	if (hix <= *lowx || hiy <= *lowy)	return FALSE;

	/* check area around room (and make room smaller if necessary) */
	for (x = *lowx - xlim; x <= hix + xlim; x++) {
		if (x <= 0 || x >= COLNO) continue;
		y = *lowy - ylim;
		ymax = hiy + ylim;
		if (y < 0)
		    y = 0;
		if (ymax >= ROWNO)
		    ymax = ROWNO - 1;
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
	xchar xabs, yabs;
	int wtmp, htmp, xaltmp, yaltmp, xtmp, ytmp;
	struct nhrect *r1 = NULL, r2;
	int trycnt = 0;
	boolean vault = FALSE;
	int xlim = XLIM, ylim = YLIM;

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
	int x = 0, y = 0;
	int trycnt = 0, wtry = 0;

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

		/* Convert wall and pos into an absolute coordinate! */
		wtry = rn2(4);
		switch (wtry) {
			case 0:
			    if (!(dwall & W_NORTH)) goto redoloop;
			    y = broom->ly - 1;
			    x = broom->lx + (dpos == -1 ?
					     rn2(broom->hx - broom->lx + 1) :
					     dpos);
			    if (IS_ROCK(lev->locations[x][y-1].typ)) goto redoloop;
			    goto outdirloop;
			case 1:
			    if (!(dwall & W_SOUTH)) goto redoloop;
			    y = broom->hy + 1;
			    x = broom->lx + (dpos == -1 ?
					     rn2(broom->hx - broom->lx + 1) :
					     dpos);
			    if (IS_ROCK(lev->locations[x][y+1].typ)) goto redoloop;
			    goto outdirloop;
			case 2:
			    if (!(dwall & W_WEST)) goto redoloop;
			    x = broom->lx - 1;
			    y = broom->ly + (dpos == -1 ?
					     rn2(broom->hy - broom->ly + 1) :
					     dpos);
			    if (IS_ROCK(lev->locations[x-1][y].typ)) goto redoloop;
			    goto outdirloop;
			case 3:
			    if (!(dwall & W_EAST)) goto redoloop;
			    x = broom->hx + 1;
			    y = broom->ly + (dpos == -1 ?
					     rn2(broom->hy - broom->ly + 1) :
					     dpos);
			    if (IS_ROCK(lev->locations[x+1][y].typ)) goto redoloop;
			    goto outdirloop;
			default:
			    x = y = 0;
			    panic("create_door: No wall for door!");
			    goto outdirloop;
		}
outdirloop:
		if (okdoor(lev, x, y))
		    break;
redoloop:	;
	} while (++trycnt <= 100);
	if (trycnt > 100) {
		impossible("create_door: Can't find a proper place!");
		return;
	}
	add_door(lev, x, y, broom);
	lev->locations[x][y].typ = (dd->secret ? SDOOR : DOOR);
	lev->locations[x][y].doormask = dd->mask;
}

/*
 * Create a secret door in croom on any one of the specified walls.
 */
void create_secret_door(struct level *lev,
			struct mkroom *croom,
			xchar walls) /* any of W_NORTH | W_SOUTH | W_EAST | W_WEST
					(or W_ANY) */
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
	schar x, y;
	coord tm;

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
	if (sp->x < 0 || sp->y < 0) {
	    for (j = 0; j < 500; j++) {
		x = sp->x;
		y = sp->y;
		get_location(lev, &x, &y, DRY|WET, croom);
		nx = x;
		ny = y;
		switch (sp->direction) {
		/* backwards to make sure we're against a wall */
		case W_NORTH: ny++; break;
		case W_SOUTH: ny--; break;
		case W_WEST: nx++; break;
		case W_EAST: nx--; break;
		default: return; break;
		}
		if (!isok(nx, ny)) continue;
		if (IS_WALL(lev->locations[nx][ny].typ)) {
		    /* mark it as broken through */
		    SET_TYPLIT(lev, nx, ny, sp->typ, sp->lit);
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
	lastdir = -1;
	nx = x;
	ny = y;
	for (j = sp->count; j > 0; j--) {
	    guard = 0;
	    SET_TYPLIT(lev, nx, ny, sp->typ, sp->lit);
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

	if (m->class >= 0)
	    class = (char) def_char_to_monclass((char)m->class);
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
	if (MON_AT(lev, x, y) && enexto(&cc, lev, x, y, pm))
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

	    if (m->seentraps) mtmp->mtrapseen = m->seentraps;
	    if (m->female) mtmp->female = 1;
	    if (m->cancelled) mtmp->mcan = 1;
	    if (m->revived) mtmp->mrevived = 1;
	    if (m->avenge) mtmp->mavenge = 1;
	    if (m->stunned) mtmp->mstun = 1;
	    if (m->confused) mtmp->mconf = 1;
	    if (m->invis) {
		mtmp->minvis = mtmp->perminvis = 1;
	    }
	    if (m->blinded) {
		mtmp->mcansee = 0;
		mtmp->mblinded = m->blinded % 127;
	    }
	    if (m->paralyzed) {
		mtmp->mcanmove = 0;
		mtmp->mfrozen = m->paralyzed % 127;
	    }
	    if (m->fleeing) {
		mtmp->mflee = 1;
		mtmp->mfleetim = m->fleeing % 127;
	    }

	    if (m->has_invent) {
		discard_minvent(mtmp);
		invent_carrying_monster = mtmp;
	    }
	}
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

	named = o->name.str ? TRUE : FALSE;

	x = o->x;
	y = o->y;
	get_location(lev, &x, &y, DRY, croom);

	if (o->class >= 0)
	    c = o->class;
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
	if (o->corpsenm == NON_PM - 1) otmp->corpsenm = rndmonnum(lev);
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

	if (o->eroded) {
	    if (o->eroded < 0) {
		otmp->oerodeproof = 1;
	    } else {
		otmp->oeroded = o->eroded % 4;
		otmp->oeroded2 = (o->eroded >> 2) % 4;
	    }
	}
	if (o->recharged) otmp->recharged = (o->recharged % 8);
	if (o->locked) {
	    otmp->olocked = 1;
	} else if (o->broken) {
	    otmp->obroken = 1;
	    otmp->olocked = 0; /* obj generation may set */
	}
	if (o->trapped) otmp->otrapped = 1;
	if (o->greased) otmp->greased = 1;
#ifdef INVISIBLE_OBJECTS
	if (o->invis) otmp->oinvis = 1;
#endif

	if (o->quan > 0 && objects[otmp->otyp].oc_merge) {
	    otmp->quan = o->quan;
	    otmp->owt = weight(otmp);
	}

	/* contents */
	if (o->containment & SP_OBJ_CONTENT) {
	    if (!container_idx) {
		if (!invent_carrying_monster) {
		    warning("create_object: no container");
		} else {
		    int c;
		    struct obj *objcheck = otmp;
		    int inuse = -1;
		    for (c = 0; c < container_idx; c++) {
			if (container_obj[c] == objcheck)
			    inuse = c;
		    }
		    remove_object(otmp);
		    if (mpickobj(invent_carrying_monster, otmp)) {
			if (inuse > -1) {
			    impossible("container given to monster "
				       "was merged or deallocated.");
			    for (c = inuse; c < container_idx - 1; c++)
				container_obj[c] = container_obj[c + 1];
			    container_obj[container_idx] = NULL;
			    container_idx--;
			}
			/* we lost track of it. */
			return;
		    }
		}
	    } else {
		remove_object(otmp);
		add_to_container(container_obj[container_idx - 1], otmp);
	    }
	}
	/* container */
	if (o->containment & SP_OBJ_CONTAINER) {
	    delete_contents(otmp);
	    if (container_idx < MAX_CONTAINMENT) {
		container_obj[container_idx] = otmp;
		container_idx++;
	    } else impossible("create_object: containers nested too deeply.");
	}

	/* Medusa level special case: statues are petrified monsters, so they
	 * are not stone-resistant and have monster inventory.  They also lack
	 * other contents, but that can be specified as an empty container.
	 */
	if (o->id == STATUE && Is_medusa_level(&lev->z) && o->corpsenm == NON_PM) {
	    struct monst *was;
	    struct obj *obj;
	    int wastyp;

	    /* Named random statues are of player types, and aren't stone-
	     * resistant (if they were, we'd have to reset the name as well as
	     * setting corpsenm).
	     */
	    for (wastyp = otmp->corpsenm; ; wastyp = rndmonnum(lev)) {
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

	if (o->lit) {
	    begin_burn(lev, otmp, FALSE);
	}

	if (o->buried && !o->containment) {
	    /* What if we'd want to bury a container?
	     * bury_an_obj() may dealloc obj.
	     */
	    bury_an_obj(otmp);
	}
}

/*
 * Randomly place a specific engraving, then release its memory.
 */
static void create_engraving(struct level *lev, engraving *e, struct mkroom *croom)
{
	xchar x, y;

	x = e->x;
	y = e->y;
	get_location(lev, &x, &y, DRY, croom);

	make_engr_at(lev, x, y, e->engr.str, 0L, e->etype);
}

/*
 * Create an altar in a room.
 */
static void create_altar(struct level *lev, altar *a, struct mkroom *croom)
{
	schar sproom, x, y;
	aligntyp amask;
	boolean croom_is_temple = TRUE;
	int oldtyp;

	x = a->x;
	y = a->y;

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
	schar x, y;

	x = g->x;
	y= g->y;
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
	schar x, y;

	x = fx;
	y = fy;
	get_location(lev, &x, &y, DRY, croom);
	/* Don't cover up an existing feature (particularly randomly
	   placed stairs). */
	if (IS_FURNITURE(lev->locations[x][y].typ))
	    return;

	lev->locations[x][y].typ = typ;
}

static void replace_terrain(struct level *lev, replaceterrain *terr, struct mkroom *croom)
{
	schar x, y, x1, y1, x2, y2;

	if (terr->toter >= MAX_TYPE) return;

	x1 = terr->x1;
	y1 = terr->y1;
	get_location(lev, &x1, &y1, DRY|WET, croom);

	x2 = terr->x2;
	y2 = terr->y2;
	get_location(lev, &x2, &y2, DRY|WET, croom);

	for (x = x1; x <= x2; x++) {
	    for (y = y1; y <= y2; y++) {
		if (lev->locations[x][y].typ == terr->fromter &&
		    rn2(100) < terr->chance) {
		    SET_TYPLIT(lev, x, y, terr->toter, terr->tolit);
		}
	    }
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
		    SET_TYPLIT(lev, dx, dy, ter, lit);
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

	x1 = rndline->x1;
	y1 = rndline->y1;
	get_location(lev, &x1, &y1, DRY|WET, croom);

	x2 = rndline->x2;
	y2 = rndline->y2;
	get_location(lev, &x2, &y2, DRY|WET, croom);

	thick = rndline->thick;

	line_midpoint_core(lev, x1, y1, x2, y2, rndline->roughness,
			   rndline->fg, rndline->lit, thick, 12);
}

static void set_terrain(struct level *lev, terrain *terr, struct mkroom *croom)
{
	schar x, y, x1, y1, x2, y2;

	if (terr->ter >= MAX_TYPE) return;

	x1 = terr->x1;
	y1 = terr->y1;
	get_location(lev, &x1, &y1, DRY|WET, croom);

	switch (terr->areatyp) {
	case 0: /* point */
	default:
	    SET_TYPLIT(lev, x1, y1, terr->ter, terr->tlit);
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
	    for (x = 0; x < terr->x2; x++)
		SET_TYPLIT(lev, x + x1, y1, terr->ter, terr->tlit);
	    break;
	case 2: /* vert line */
	    for (y = 0; y < terr->y2; y++)
		SET_TYPLIT(lev, x1, y + y1, terr->ter, terr->tlit);
	    break;
	case 3: /* filled rectangle */
	    x2 = terr->x2;
	    y2 = terr->y2;
	    get_location(lev, &x2, &y2, DRY|WET, croom);
	    for (x = x1; x <= x2; x++) {
		for (y = y1; y <= y2; y++) {
		    SET_TYPLIT(lev, x, y, terr->ter, terr->tlit);
		}
	    }
	    break;
	case 4: /* rectangle */
	    x2 = terr->x2;
	    y2 = terr->y2;
	    get_location(lev, &x2, &y2, DRY|WET, croom);
	    for (x = x1; x <= x2; x++) {
		SET_TYPLIT(lev, x, y1, terr->ter, terr->tlit);
		SET_TYPLIT(lev, x, y2, terr->ter, terr->tlit);
	    }
	    for (y = y1; y <= y2; y++) {
		SET_TYPLIT(lev, x1, y, terr->ter, terr->tlit);
		SET_TYPLIT(lev, x2, y, terr->ter, terr->tlit);
	    }
	    break;
	}
}

/*
 * Search for a door in a room on a specified wall.
 */
static boolean search_door(struct mkroom *croom, struct level *lev,
			   xchar *x, xchar *y, xchar wall, int cnt)
{
	int dx, dy;
	int xx, yy;

	switch(wall) {
	case W_NORTH:
	    dy = 0;
	    dx = 1;
	    xx = croom->lx;
	    yy = croom->hy + 1;
	    break;
	case W_SOUTH:
	    dy = 0;
	    dx = 1;
	    xx = croom->lx;
	    yy = croom->ly - 1;
	    break;
	case W_EAST:
	    dy = 1;
	    dx = 0;
	    xx = croom->hx + 1;
	    yy = croom->ly;
	    break;
	case W_WEST:
	    dy = 1;
	    dx = 0;
	    xx = croom->lx - 1;
	    yy = croom->ly;
	    break;
	default:
	    dx = dy = xx = yy = 0;
	    panic("search_door: Bad wall!");
	    break;
	}
	while (xx <= croom->hx + 1 && yy <= croom->hy + 1) {
	    if (IS_DOOR(lev->locations[xx][yy].typ) ||
		lev->locations[xx][yy].typ == SDOOR) {
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
	int dx = 0, dy = 0, dix, diy, cct;
	struct rm *crm;
	int tx, ty, xx, yy;

	xx = org->x;
	yy = org->y;
	tx = dest->x;
	ty = dest->y;
	if (xx <= 0 || yy <= 0 || tx <= 0 || ty <= 0 ||
	    xx > COLNO - 1 || tx > COLNO - 1 ||
	    yy > ROWNO - 1 || ty > ROWNO - 1)
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
	    if (cct++ > 500 || (nxcor && !rn2(35))) {
		if (!nxcor)
		    warning("dig_corridor: giving up after %d tries", cct - 1);
		return FALSE;
	    }

	    xx += dx;
	    yy += dy;

	    if (xx >= COLNO - 1 || xx <= 0 || yy <= 0 || yy >= ROWNO - 1) {
		/* should be impossible */
		if (!nxcor)
		    warning("dig_corridor: hit level edge [%d,%d]", xx, yy);
		return FALSE;
	    }

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
		if (!nxcor) {
		    warning("dig_corridor: trying to dig disallowed tile [%d,%d]",
			    xx, yy);
		}
		return FALSE;
	    }

	    /* find next corridor position */
	    dix = abs(xx-tx);
	    diy = abs(yy-ty);

	    /* do we have to change direction ? */
	    if (dy && (dix > diy ||
		       (dy < 0 && yy < ty) ||
		       (dy > 0 && yy > ty))) {
		int ddx = (xx > tx) ? -1 : 1;

		crm = &lev->locations[xx + ddx][yy];
		if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
		    dx = ddx;
		    dy = 0;
		    continue;
		}
	    } else if (dx && (diy > dix ||
			      (dx < 0 && xx < tx) ||
			      (dx > 0 && xx > tx))) {
		int ddy = (yy > ty) ? -1 : 1;

		crm = &lev->locations[xx][yy + ddy];
		if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
		    dy = ddy;
		    dx = 0;
		    continue;
		}
	    }

	    /* continue straight on? */
	    crm = &lev->locations[xx + dx][yy + dy];
	    if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
		continue;

	    /* no, what must we do now?? */
	    if (dx) {
		/* should we go up or down? */
		boolean upok = FALSE;
		boolean downok = FALSE;
		if (xx == tx) {
		    if (yy > ty) upok = TRUE;
		    else downok = TRUE;
		} else {
		    /* avoid corners made by rooms near level edges */
		    int i;
		    for (i = 1; i < ROWNO - 1; i++) {
			if (upok && downok)
			    break;
			if (yy - i > 0) {
			    crm = &lev->locations[xx + dx][yy - i];
			    if (!upok &&
				yy - i - 1 > 0 &&
				(crm->typ == btyp ||
				 crm->typ == ftyp ||
				 crm->typ == SCORR))
				upok = TRUE;
			}
			if (yy + i < ROWNO - 1) {
			    crm = &lev->locations[xx + dx][yy + i];
			    if (!downok &&
				yy + i + 1 < ROWNO - 1 &&
				(crm->typ == btyp ||
				 crm->typ == ftyp ||
				 crm->typ == SCORR))
				downok = TRUE;
			}
		    }
		}
		if (!nxcor && !upok && !downok)
		    warning("dig_corridor: horizontal corridor blocked");
		dx = 0;
		dy = ((yy > ty && upok) || !downok) ? -1 : 1;
	    } else {
		/* should we go left or right? */
		boolean leftok = FALSE;
		boolean rightok = FALSE;
		if (yy == ty) {
		    if (xx > tx) leftok = TRUE;
		    else rightok = TRUE;
		} else {
		    /* avoid corners made by rooms near level edges */
		    int i;
		    for (i = 1; i < COLNO - 1; i++) {
			if (leftok && rightok)
			    break;
			if (xx - i > 0) {
			    crm = &lev->locations[xx - i][yy + dy];
			    if (!leftok &&
				xx - i - 1 > 0 &&
				(crm->typ == btyp ||
				 crm->typ == ftyp ||
				 crm->typ == SCORR))
				leftok = TRUE;
			}
			if (xx + i < COLNO - 1) {
			    crm = &lev->locations[xx + i][yy + dy];
			    if (!rightok &&
				xx + i + 1 < COLNO - 1 &&
				(crm->typ == btyp ||
				 crm->typ == ftyp ||
				 crm->typ == SCORR))
				rightok = TRUE;
			}
		    }
		}
		if (!nxcor && !leftok && !rightok)
		    warning("dig_corridor: vertical corridor blocked");
		dx = ((xx > tx && leftok) || !rightok) ? -1 : 1;
		dy = 0;
	    }
	    crm = &lev->locations[xx + dx][yy + dy];
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
	for (i = 0; i < lev->nroom; i++) {
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
	for (i = 0; i < lev->nroom; i++) {
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
	    makecorridors(lev, c->src.door);
	    return;
	}

	if (!search_door(&lev->rooms[c->src.room], lev, &org.x, &org.y,
			 c->src.wall, c->src.door))
	    return;

	if (c->dest.room != -1) {
	    if (!search_door(&lev->rooms[c->dest.room], lev, &dest.x, &dest.y,
			     c->dest.wall, c->dest.door))
		return;
	    switch (c->src.wall) {
		case W_NORTH: org.y--; break;
		case W_SOUTH: org.y++; break;
		case W_WEST:  org.x--; break;
		case W_EAST:  org.x++; break;
	    }
	    switch (c->dest.wall) {
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
	    int x, y;

	    /* Shop ? */
	    if (croom->rtype >= SHOPBASE) {
		stock_room(croom->rtype - SHOPBASE, lev, croom);
		lev->flags.has_shop = TRUE;
		return;
	    }

	    switch (croom->rtype) {
		case VAULT:
		    for (x = croom->lx; x <= croom->hx; x++)
			for (y = croom->ly; y<=croom->hy; y++)
			    mkgold((long)rn1(abs(depth(&lev->z)) * 100, 51),
				   lev, x, y);
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
	struct mkroom *aroom;
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
	    lowx = max(lowx - 1, 1);
	    hix = min(hix + 1, COLNO - 1);
	    lowy = max(lowy - 1, 0);
	    hiy = min(hiy + 1, ROWNO - 1);
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

static void wallify_map(struct level *lev, int x1, int y1, int x2, int y2)
{
	int x, y, xx, yy, lo_xx, lo_yy, hi_xx, hi_yy;

	for (y = y1; y <= y2; y++) {
	    lo_yy = (y > 0) ? y - 1 : 0;
	    hi_yy = (y < y2) ? y + 1 : y2;
	    for (x = x1; x <= x2; x++) {
		if (lev->locations[x][y].typ != STONE) continue;
		lo_xx = (x > 0) ? x - 1 : 0;
		hi_xx = (x < x2) ? x + 1 : x2;
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

	m->x = (xchar)x;
	m->y = (xchar)y;
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
	struct opvar *opdat;
	int opcode;

	Fread(&lvl->n_opcodes, 1, sizeof(lvl->n_opcodes), fd);

	lvl->opcodes = malloc(sizeof(_opcode) * lvl->n_opcodes);

	while (n_opcode < lvl->n_opcodes) {
	    Fread(&lvl->opcodes[n_opcode].opcode, 1,
		  sizeof(lvl->opcodes[n_opcode].opcode), fd);
	    opcode = lvl->opcodes[n_opcode].opcode;

	    opdat = NULL;

	    if (opcode < SPO_NULL || opcode >= MAX_SP_OPCODES)
		panic("sp_level_loader: impossible opcode %i.", opcode);

	    if (opcode == SPO_PUSH) {
		struct opvar *ov = (opdat = malloc(sizeof(struct opvar)));
		int nsize;

		ov->spovartyp = SPO_NULL;
		ov->vardata.l = 0;
		Fread(&ov->spovartyp, 1, sizeof(ov->spovartyp), fd);

		switch (ov->spovartyp) {
		case SPOVAR_NULL: break;
		case SPOVAR_COORD:
		case SPOVAR_REGION:
		case SPOVAR_MAPCHAR:
		case SPOVAR_MONST:
		case SPOVAR_OBJ:
		case SPOVAR_INT:
		    Fread(&ov->vardata.l, 1, sizeof(ov->vardata.l), fd);
		    break;
		case SPOVAR_VARIABLE:
		case SPOVAR_STRING:
		    {
			char *opd;
			Fread(&nsize, 1, sizeof(nsize), fd);
			opd = malloc(nsize + 1);
			if (nsize) Fread(opd, 1, nsize, fd);
			opd[nsize] = 0;
			ov->vardata.str = opd;
		    }
		    break;
		default:
		    panic("sp_level_loader: Unknown opcode %i", opcode);
		}
	    }

	    lvl->opcodes[n_opcode].opdat = opdat;
	    n_opcode++;
	}

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

	while (n_opcode < lvl->n_opcodes) {
	    int opcode = lvl->opcodes[n_opcode].opcode;
	    struct opvar *opdat = lvl->opcodes[n_opcode].opdat;

	    if (opcode < SPO_NULL || opcode >= MAX_SP_OPCODES)
		panic("sp_level_free: unknown opcode %i", opcode);

	    if (opdat) opvar_free(opdat);
	    n_opcode++;
	}

	free(lvl->opcodes);
	return TRUE;
}

static void splev_initlev(struct level *lev, lev_init *linit)
{
	switch (linit->init_style) {
	default: impossible("Unrecognized level init style."); break;
	case LVLINIT_NONE: break;
	case LVLINIT_SOLIDFILL:
	    if (linit->lit == -1) linit->lit = rn2(2);
	    lvlfill_solid(lev, linit->filling, linit->lit);
	    break;
	case LVLINIT_MAZEGRID:
	    lvlfill_maze_grid(lev, 2, 0, x_maze_max, y_maze_max, linit->filling);
	    break;
	case LVLINIT_MINES:
	    if (linit->filling > -1) lvlfill_solid(lev, linit->filling, linit->lit);
	    mkmap(lev, linit);
	    break;
	}
}

static struct sp_frame *frame_new(long execptr)
{
	struct sp_frame *frame = malloc(sizeof(struct sp_frame));
	if (!frame) panic("could not create execution frame.");

	frame->next = NULL;
	frame->variables = NULL;
	frame->n_opcode = execptr;
	frame->stack = malloc(sizeof(struct splevstack));
	if (!frame->stack) panic("could not create execution frame stack.");

	splev_stack_init(frame->stack);
	return frame;
}

static void frame_del(struct sp_frame *frame)
{
	if (!frame) return;
	if (frame->stack) {
	    splev_stack_done(frame->stack);
	    frame->stack = NULL;
	}
	if (frame->variables) {
	    variable_list_del(frame->variables);
	    frame->variables = NULL;
	}
	Free(frame);
}

static void spo_frame_push(struct sp_coder *coder)
{
	struct sp_frame *tmpframe = frame_new(coder->frame->n_opcode);
	tmpframe->next = coder->frame;
	coder->frame = tmpframe;
}

static void spo_frame_pop(struct sp_coder *coder)
{
	if (coder->frame->next) {
	    struct sp_frame *tmpframe = coder->frame->next;
	    frame_del(coder->frame);
	    coder->frame = tmpframe;
	}
}

static long sp_code_jmpaddr(long curpos, long jmpaddr)
{
	return (curpos + jmpaddr);
}

static void spo_call(struct sp_coder *coder)
{
	struct opvar *addr;
	struct opvar *params;
	struct sp_frame *tmpframe;

	if (!OV_pop_i(addr) || !OV_pop_i(params)) return;
	if (OV_i(params) < 0) return;

	/* push a frame */
	tmpframe = frame_new(sp_code_jmpaddr(coder->frame->n_opcode, OV_i(addr) - 1));
	tmpframe->next = coder->frame;
	coder->frame = tmpframe;

	while (OV_i(params)-- > 0)
	    splev_stack_push(tmpframe->stack, splev_stack_pop(coder->stack));

	opvar_free(addr);
	opvar_free(params);
}

static void spo_return(struct sp_coder *coder)
{
	struct opvar *params;
	if (!coder->frame->next) return; /* something is seriously wrong */
	if (!OV_pop_i(params)) return;
	if (OV_i(params) < 0) return;

	while (OV_i(params)-- > 0) {
	    splev_stack_push(coder->frame->next->stack,
			     splev_stack_pop(coder->stack));
	}

	/* pop the frame */
	if (coder->frame->next) {
	    struct sp_frame *tmpframe = coder->frame->next;
	    frame_del(coder->frame);
	    coder->frame = tmpframe;
	    coder->stack = coder->frame->stack;
	}

	opvar_free(params);
}

static void spo_end_moninvent(struct sp_coder *coder, struct level *lev)
{
	if (invent_carrying_monster)
	    m_dowear(lev, invent_carrying_monster, TRUE);
	invent_carrying_monster = NULL;
}

static void spo_pop_container(struct sp_coder *coder)
{
	if (container_idx > 0) {
	    container_idx--;
	    container_obj[container_idx] = NULL;
	}
}

static void spo_message(struct sp_coder *coder)
{
	struct opvar *op;
	char *msg, *levmsg;
	int old_n, n;

	if (!OV_pop_s(op)) return;
	msg = string_subst(OV_s(op));
	if (!msg) return;

	old_n = lev_message ? strlen(lev_message) + 1 : 0;
	n = strlen(msg);

	levmsg = malloc(old_n + n + 1);
	if (old_n) levmsg[old_n - 1] = '\n';
	if (lev_message)
	    memcpy(levmsg, lev_message, old_n - 1);
	memcpy(&levmsg[old_n], msg, n);
	levmsg[old_n + n] = '\0';
	Free(lev_message);
	lev_message = levmsg;
	opvar_free(op);
}

static void spo_monster(struct sp_coder *coder, struct level *lev)
{
	int nparams = 0;

	struct opvar *varparam;
	struct opvar *id, *coord, *has_inv;
	monster tmpmons;

	tmpmons.peaceful = -1;
	tmpmons.asleep = -1;
	tmpmons.name.str = NULL;
	tmpmons.appear = 0;
	tmpmons.appear_as.str = NULL;
	tmpmons.align = -MAX_REGISTERS - 2;
	tmpmons.female = 0;
	tmpmons.invis = 0;
	tmpmons.cancelled = 0;
	tmpmons.revived = 0;
	tmpmons.avenge = 0;
	tmpmons.fleeing = 0;
	tmpmons.blinded = 0;
	tmpmons.paralyzed = 0;
	tmpmons.stunned = 0;
	tmpmons.confused = 0;
	tmpmons.seentraps = 0;
	tmpmons.has_invent = 0;

	if (!OV_pop_i(has_inv)) return;

	if (!OV_pop_i(varparam)) return;

	while (nparams++ < SP_M_V_END + 1 &&
	       OV_typ(varparam) == SPOVAR_INT &&
	       OV_i(varparam) >= 0 &&
	       OV_i(varparam) < SP_M_V_END) {
	    struct opvar *parm = NULL;
	    OV_pop(parm);
	    switch (OV_i(varparam)) {
	    case SP_M_V_NAME:
		if (OV_typ(parm) == SPOVAR_STRING && !tmpmons.name.str)
		    tmpmons.name.str = strdup(OV_s(parm));
		break;
	    case SP_M_V_APPEAR:
		if (OV_typ(parm) == SPOVAR_INT && !tmpmons.appear_as.str) {
		    tmpmons.appear = OV_i(parm);
		    opvar_free(parm);
		    OV_pop(parm);
		    tmpmons.appear_as.str = strdup(OV_s(parm));
		}
		break;
	    case SP_M_V_ASLEEP:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.asleep = OV_i(parm);
		break;
	    case SP_M_V_ALIGN:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.align = OV_i(parm);
		break;
	    case SP_M_V_PEACEFUL:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.peaceful = OV_i(parm);
		break;
	    case SP_M_V_FEMALE:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.female = OV_i(parm);
		break;
	    case SP_M_V_INVIS:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.invis = OV_i(parm);
		break;
	    case SP_M_V_CANCELLED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.cancelled = OV_i(parm);
		break;
	    case SP_M_V_REVIVED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.revived = OV_i(parm);
		break;
	    case SP_M_V_AVENGE:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.avenge = OV_i(parm);
		break;
	    case SP_M_V_FLEEING:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.fleeing = OV_i(parm);
		break;
	    case SP_M_V_BLINDED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.blinded = OV_i(parm);
		break;
	    case SP_M_V_PARALYZED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.paralyzed = OV_i(parm);
		break;
	    case SP_M_V_STUNNED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.stunned = OV_i(parm);
		break;
	    case SP_M_V_CONFUSED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.confused = OV_i(parm);
		break;
	    case SP_M_V_SEENTRAPS:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpmons.seentraps = OV_i(parm);
		break;
	    case SP_M_V_END:
		nparams = SP_M_V_END + 1;
		break;
	    default:
		impossible("MONSTER with unknown variable param type!");
		break;
	    }
	    opvar_free(parm);
	    if (OV_i(varparam) != SP_M_V_END) {
		opvar_free(varparam);
		OV_pop(varparam);
	    }
	}

	if (!OV_pop_c(coord)) panic("no monster coord?");

	if (!OV_pop_typ(id, SPOVAR_MONST)) panic("no mon type");

	tmpmons.id = SP_MONST_PM(OV_i(id));
	tmpmons.class = SP_MONST_CLASS(OV_i(id));
	tmpmons.x = SP_COORD_X(OV_i(coord));
	tmpmons.y = SP_COORD_Y(OV_i(coord));
	tmpmons.has_invent = OV_i(has_inv);

	create_monster(lev, &tmpmons, coder->croom);

	free(tmpmons.name.str);
	free(tmpmons.appear_as.str);

	opvar_free(id);
	opvar_free(coord);
	opvar_free(has_inv);
	opvar_free(varparam);
}

static void spo_object(struct sp_coder *coder, struct level *lev)
{
	int nparams = 0;

	struct opvar *varparam;
	struct opvar *id, *containment;

	object tmpobj;

	tmpobj.spe = -127;
	tmpobj.curse_state = -1;
	tmpobj.corpsenm = NON_PM;
	tmpobj.name.str = NULL;
	tmpobj.quan = -1;
	tmpobj.buried = 0;
	tmpobj.lit = 0;
	tmpobj.eroded = 0;
	tmpobj.locked = 0;
	tmpobj.trapped = 0;
	tmpobj.recharged = 0;
	tmpobj.invis = 0;
	tmpobj.greased = 0;
	tmpobj.broken = 0;
	tmpobj.x = tmpobj.y = -1;

	if (!OV_pop_i(containment)) return;

	if (!OV_pop_i(varparam)) return;

	while (nparams++ < SP_O_V_END + 1 &&
	       OV_typ(varparam) == SPOVAR_INT &&
	       OV_i(varparam) >= 0 &&
	       OV_i(varparam) < SP_O_V_END) {
	    struct opvar *parm = NULL;
	    OV_pop(parm);
	    switch (OV_i(varparam)) {
	    case SP_O_V_NAME:
		if (OV_typ(parm) == SPOVAR_STRING && !tmpobj.name.str)
		    tmpobj.name.str = strdup(OV_s(parm));
		break;
	    case SP_O_V_CORPSENM:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.corpsenm = OV_i(parm);
		break;
	    case SP_O_V_CURSE:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.curse_state = OV_i(parm);
		break;
	    case SP_O_V_SPE:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.spe = OV_i(parm);
		break;
	    case SP_O_V_QUAN:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.quan = OV_i(parm);
		break;
	    case SP_O_V_BURIED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.buried = OV_i(parm);
		break;
	    case SP_O_V_LIT:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.lit = OV_i(parm);
		break;
	    case SP_O_V_ERODED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.eroded = OV_i(parm);
		break;
	    case SP_O_V_LOCKED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.locked = OV_i(parm);
		break;
	    case SP_O_V_TRAPPED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.trapped = OV_i(parm);
		break;
	    case SP_O_V_RECHARGED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.recharged = OV_i(parm);
		break;
	    case SP_O_V_INVIS:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.invis = OV_i(parm);
		break;
	    case SP_O_V_GREASED:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.greased = OV_i(parm);
		break;
	    case SP_O_V_BROKEN:
		if (OV_typ(parm) == SPOVAR_INT)
		    tmpobj.broken = OV_i(parm);
		break;
	    case SP_O_V_COORD:
		if (OV_typ(parm) != SPOVAR_COORD)
		    panic("no coord for obj?");
		tmpobj.y = SP_COORD_Y(OV_i(parm));
		tmpobj.x = SP_COORD_X(OV_i(parm));
		break;
	    case SP_O_V_END:
		nparams = SP_O_V_END + 1;
		break;
	    default:
		impossible("OBJECT with unknown variable param type!");
		break;
	    }
	    opvar_free(parm);
	    if (OV_i(varparam) != SP_O_V_END) {
		opvar_free(varparam);
		OV_pop(varparam);
	    }
	}

	if (!OV_pop_typ(id, SPOVAR_OBJ))
	    panic("no obj type");

	tmpobj.id = SP_OBJ_TYP(OV_i(id));
	tmpobj.class = SP_OBJ_CLASS(OV_i(id));
	tmpobj.containment = OV_i(containment);

	create_object(lev, &tmpobj, coder->croom);

	Free(tmpobj.name.str);

	opvar_free(varparam);
	opvar_free(id);
	opvar_free(containment);
}

static void spo_level_flags(struct sp_coder *coder, struct level *lev)
{
	struct opvar *flagdata;
	long flags;

	if (!OV_pop_i(flagdata)) return;
	flags = OV_i(flagdata);

	if (flags & NOTELEPORT)		lev->flags.noteleport = 1;
	if (flags & HARDFLOOR)		lev->flags.hardfloor = 1;
	if (flags & NOMMAP)		lev->flags.nommap = 1;
	if (flags & SHORTSIGHTED)	lev->flags.shortsighted = 1;
	if (flags & ARBOREAL)		lev->flags.arboreal = 1;
	if (flags & NOFLIPX)		coder->allow_flips &= ~1;
	if (flags & NOFLIPY)		coder->allow_flips &= ~2;
	if (flags & MAZELEVEL)		lev->flags.is_maze_lev = 1;
	if (flags & PREMAPPED)		coder->premapped = TRUE;
	if (flags & SHROUD)		lev->flags.hero_memory = 0;
	if (flags & STORMY)		lev->flags.stormy = 1;
	if (flags & GRAVEYARD)		lev->flags.graveyard = 1;

	opvar_free(flagdata);
}

static void spo_initlevel(struct sp_coder *coder, struct level *lev)
{
	lev_init init_lev;
	struct opvar *init_style, *fg, *bg,
		     *smoothed, *joined, *lit, *walled, *filling;

	if (!OV_pop_i(fg) ||
	    !OV_pop_i(bg) ||
	    !OV_pop_i(smoothed) ||
	    !OV_pop_i(joined) ||
	    !OV_pop_i(lit) ||
	    !OV_pop_i(walled) ||
	    !OV_pop_i(filling) ||
	    !OV_pop_i(init_style)) return;

	init_lev.init_style = OV_i(init_style);
	init_lev.fg = OV_i(fg);
	init_lev.bg = OV_i(bg);
	init_lev.smoothed = OV_i(smoothed);
	init_lev.joined = OV_i(joined);
	init_lev.lit = OV_i(lit);
	init_lev.walled = OV_i(walled);
	init_lev.filling = OV_i(filling);

	coder->lvl_is_joined = OV_i(joined);

	splev_initlev(lev, &init_lev);

	opvar_free(init_style);
	opvar_free(fg);
	opvar_free(bg);
	opvar_free(smoothed);
	opvar_free(joined);
	opvar_free(lit);
	opvar_free(walled);
	opvar_free(filling);
}

static void spo_mon_generation(struct sp_coder *coder, struct level *lev)
{
	struct opvar *freq, *n_tuples;
	struct mon_gen_override *mg;
	struct mon_gen_tuple *mgtuple;

	if (lev->mon_gen) {
	    impossible("monster generation override already defined");
	    return;
	}

	if (!OV_pop_i(n_tuples) ||
	    !OV_pop_i(freq)) return;

	mg = malloc(sizeof(struct mon_gen_override));
	mg->override_chance = OV_i(freq);
	mg->total_mon_freq = 0;
	mg->gen_chances = NULL;
	while (OV_i(n_tuples)-- > 0) {
	    struct opvar *mfreq, *is_sym, *mon;
	    mgtuple = malloc(sizeof(struct mon_gen_tuple));

	    if (!OV_pop_i(is_sym) ||
		!OV_pop_i(mon) ||
		!OV_pop_i(mfreq))
		panic("oopsie when loading mon_gen chances.");

	    mgtuple->freq = OV_i(mfreq);
	    if (OV_i(mfreq) < 1) OV_i(mfreq) = 1;
	    mgtuple->is_sym = OV_i(is_sym);
	    if (OV_i(is_sym))
		mgtuple->monid = def_char_to_monclass(OV_i(mon));
	    else
		mgtuple->monid = OV_i(mon);
	    mgtuple->next = mg->gen_chances;
	    mg->gen_chances = mgtuple;
	    mg->total_mon_freq += OV_i(mfreq);
	    opvar_free(mfreq);
	    opvar_free(is_sym);
	    opvar_free(mon);
	}
	lev->mon_gen = mg;

	opvar_free(freq);
	opvar_free(n_tuples);
}

static void spo_level_sounds(struct sp_coder *coder, struct level *lev)
{
	struct opvar *freq, *n_tuples;
	struct lvl_sounds *ls;

	if (lev->sounds) {
	    impossible("level sounds already defined.");
	    return;
	}

	if (!OV_pop_i(n_tuples)) return;

	if (OV_i(n_tuples) < 1)
	    impossible("no level sounds attached to the sound opcode?");

	ls = malloc(sizeof(struct lvl_sounds));
	ls->n_sounds = OV_i(n_tuples);
	ls->sounds = malloc(sizeof(struct lvl_sound_bite) * ls->n_sounds);

	while (OV_i(n_tuples)-- > 0) {
	    struct opvar *flags, *msg;

	    if (!OV_pop_i(flags) ||
		!OV_pop_s(msg))
		panic("oopsie when loading lvl_sound_bite.");

	    ls->sounds[OV_i(n_tuples)].flags = OV_i(flags);
	    ls->sounds[OV_i(n_tuples)].msg = strdup(OV_s(msg));

	    opvar_free(flags);
	    opvar_free(msg);
	}

	if (!OV_pop_i(freq))
	    ls->freq = 1;
	else
	    ls->freq = OV_i(freq);
	if (ls->freq < 0)
	    ls->freq = -ls->freq;

	lev->sounds = ls;

	opvar_free(freq);
	opvar_free(n_tuples);
}

static void spo_engraving(struct sp_coder *coder, struct level *lev)
{
	struct opvar *etyp, *txt, *coord;
	engraving tmpe;

	if (!OV_pop_i(etyp) ||
	    !OV_pop_s(txt) ||
	    !OV_pop_c(coord)) return;

	tmpe.x = SP_COORD_X(OV_i(coord));
	tmpe.y = SP_COORD_Y(OV_i(coord));
	tmpe.engr.str = OV_s(txt);
	tmpe.etype = OV_i(etyp);

	create_engraving(lev, &tmpe, coder->croom);

	opvar_free(etyp);
	opvar_free(txt);
	opvar_free(coord);
}

static void spo_room(struct sp_coder *coder, struct level *lev)
{
	if (coder->n_subroom > MAX_NESTED_ROOMS) {
	    panic("Too deeply nested rooms?!");
	} else {
	    struct opvar *filled, *h, *w, *yalign, *xalign,
			 *y, *x, *rlit, *chance, *rtype;

	    room tmproom;
	    struct mkroom *tmpcr;

	    if (!OV_pop_i(h) ||
		!OV_pop_i(w) ||
		!OV_pop_i(y) ||
		!OV_pop_i(x) ||
		!OV_pop_i(yalign) ||
		!OV_pop_i(xalign) ||
		!OV_pop_i(filled) ||
		!OV_pop_i(rlit) ||
		!OV_pop_i(chance) ||
		!OV_pop_i(rtype)) return;

	    tmproom.x = OV_i(x);
	    tmproom.y = OV_i(y);
	    tmproom.w = OV_i(w);
	    tmproom.h = OV_i(h);
	    tmproom.xalign = OV_i(xalign);
	    tmproom.yalign = OV_i(yalign);
	    tmproom.rtype = OV_i(rtype);
	    tmproom.chance = OV_i(chance);
	    tmproom.rlit = OV_i(rlit);
	    tmproom.filled = OV_i(filled);

	    opvar_free(x);
	    opvar_free(y);
	    opvar_free(w);
	    opvar_free(h);
	    opvar_free(xalign);
	    opvar_free(yalign);
	    opvar_free(rtype);
	    opvar_free(chance);
	    opvar_free(rlit);
	    opvar_free(filled);

	    if (!coder->failed_room[coder->n_subroom - 1]) {
		tmpcr = build_room(lev, &tmproom, coder->croom);
		if (tmpcr) {
		    coder->tmproomlist[coder->n_subroom] = tmpcr;
		    coder->failed_room[coder->n_subroom] = FALSE;
		    coder->n_subroom++;
		    return;
		}
	    } /* failed to create parent room, so fail this too */
	}
	coder->tmproomlist[coder->n_subroom] = NULL;
	coder->failed_room[coder->n_subroom] = TRUE;
	coder->n_subroom++;
}

static void spo_endroom(struct sp_coder *coder)
{
	if (coder->n_subroom > 1) {
	    coder->n_subroom--;
	} else {
	    /* no subroom, get out of top-level room */
	    /* Need to ensure xstart/ystart/xsize/ysize have something sensible,
	     * in case there's some stuff to be created outside the outermost room,
	     * and there's no MAP.
	     */
	    if (xsize <= 1 && ysize <= 1) {
		xstart = 1;
		ystart = 0;
		xsize = COLNO - 1;
		ysize = ROWNO;
	    }
	}
}

static void spo_door(struct sp_coder *coder, struct level *lev)
{
	schar x, y;
	struct opvar *msk, *coord;
	struct mkroom *droom;
	xchar typ;

	if (!OV_pop_i(msk) ||
	    !OV_pop_c(coord)) return;

	droom = &lev->rooms[0];

	x = SP_COORD_X(OV_i(coord));
	y = SP_COORD_Y(OV_i(coord));
	typ = OV_i(msk) == -1 ? rnddoor() : (xchar)OV_i(msk);

	get_location(lev, &x, &y, DRY, NULL);
	if (!IS_DOOR(lev->locations[x][y].typ) && lev->locations[x][y].typ != SDOOR)
	    lev->locations[x][y].typ = (typ & D_SECRET) ? SDOOR : DOOR;
	if (typ & D_SECRET) {
	    typ &= ~D_SECRET;
	    if (typ < D_CLOSED)
		typ = D_CLOSED;
	}
	lev->locations[x][y].doormask = typ;
	/*SpLev_Map[x][y] = 1;*/

	/* Now the complicated part, list it with each subroom */
	/* The dog move and mail daemon routines use this */
	while (droom->hx >= 0 && lev->doorindex < DOORMAX) {
	    if (droom->hx >= x - 1 && droom->lx <= x + 1 &&
		droom->hy >= y - 1 && droom->ly <= y + 1) {
		/* Found it */
		add_door(lev, x, y, droom);
	    }
	    droom++;
	}

	opvar_free(coord);
	opvar_free(msk);
}

static void spo_stair(struct sp_coder *coder, struct level *lev)
{
	xchar x, y;
	struct opvar *up, *coord;
	struct trap *badtrap;

	if (!OV_pop_i(up) ||
	    !OV_pop_c(coord)) return;

	x = SP_COORD_X(OV_i(coord));
	y = SP_COORD_Y(OV_i(coord));

	if (coder->croom) {
	    get_location(lev, &x, &y, DRY, coder->croom);
	    mkstairs(lev, x, y, (char)OV_i(up), coder->croom);
	    SpLev_Map[x][y] = 1;
	} else {
	    get_location(lev, &x, &y, DRY, coder->croom);
	    if ((badtrap = t_at(lev, x, y)) != 0) deltrap(lev, badtrap);
	    mkstairs(lev, x, y, (char)OV_i(up), coder->croom);
	    SpLev_Map[x][y] = 1;
	}

	opvar_free(coord);
	opvar_free(up);
}

static void spo_ladder(struct sp_coder *coder, struct level *lev)
{
	xchar x, y;
	struct opvar *up, *coord;

	if (!OV_pop_i(up) ||
	    !OV_pop_c(coord)) return;

	x = SP_COORD_X(OV_i(coord));
	y = SP_COORD_Y(OV_i(coord));

	get_location(lev, &x, &y, DRY, coder->croom);

	lev->locations[x][y].typ = LADDER;
	SpLev_Map[x][y] = 1;
	if (OV_i(up)) {
	    lev->upladder.sx = x;
	    lev->upladder.sy = y;
	    lev->locations[x][y].ladder = LA_UP;
	} else {
	    lev->dnladder.sx = x;
	    lev->dnladder.sy = y;
	    lev->locations[x][y].ladder = LA_DOWN;
	}

	opvar_free(coord);
	opvar_free(up);
}

static void spo_grave(struct sp_coder *coder, struct level *lev)
{
	struct opvar *coord, *typ, *txt;
	schar x, y;

	if (!OV_pop_i(typ) ||
	    !OV_pop_s(txt) ||
	    !OV_pop_c(coord)) return;

	x = SP_COORD_X(OV_i(coord));
	y = SP_COORD_Y(OV_i(coord));
	get_location(lev, &x, &y, DRY, coder->croom);

	if (isok(x, y) && !t_at(lev, x, y)) {
	    lev->locations[x][y].typ = GRAVE;
	    switch (OV_i(typ)) {
	    case 2: make_grave(lev, x, y, OV_s(txt)); break;
	    case 1: make_grave(lev, x, y, NULL); break;
	    default: del_engr_at(lev, x, y); break;
	    }
	}

	opvar_free(coord);
	opvar_free(typ);
	opvar_free(txt);
}

static void spo_altar(struct sp_coder *coder, struct level *lev)
{
	struct opvar *al, *shrine, *coord;
	altar tmpaltar;

	if (!OV_pop_i(al) ||
	    !OV_pop_i(shrine) ||
	    !OV_pop_c(coord)) return;

	tmpaltar.x = SP_COORD_X(OV_i(coord));
	tmpaltar.y = SP_COORD_Y(OV_i(coord));
	tmpaltar.align = OV_i(al);
	tmpaltar.shrine = OV_i(shrine);

	create_altar(lev, &tmpaltar, coder->croom);

	opvar_free(coord);
	opvar_free(shrine);
	opvar_free(al);
}

static void spo_wallwalk(struct sp_coder *coder, struct level *lev)
{
	struct opvar *coord, *fgtyp, *bgtyp, *chance;
	xchar x, y;

	if (!OV_pop_i(chance) ||
	    !OV_pop_typ(bgtyp, SPOVAR_MAPCHAR) ||
	    !OV_pop_typ(fgtyp, SPOVAR_MAPCHAR) ||
	    !OV_pop_c(coord)) return;

	x = SP_COORD_X(OV_i(coord));
	y = SP_COORD_Y(OV_i(coord));
	get_location(lev, &x, &y, DRY|WET, coder->croom);

	if (OV_i(fgtyp) >= MAX_TYPE) return;
	if (OV_i(bgtyp) >= MAX_TYPE) return;

	wallwalk_right(lev, x, y, OV_i(fgtyp), OV_i(bgtyp), OV_i(chance));

	opvar_free(coord);
	opvar_free(chance);
	opvar_free(fgtyp);
	opvar_free(bgtyp);
}

static void spo_feature(struct sp_coder *coder, struct level *lev)
{
	struct opvar *coord;
	int typ;

	if (!OV_pop_c(coord)) return;

	switch (coder->opcode) {
	default:
	    impossible("spo_feature called with wrong opcode %i.", coder->opcode);
	    break;
	case SPO_FOUNTAIN:	typ = FOUNTAIN;	break;
	case SPO_SINK:		typ = SINK;	break;
	case SPO_POOL:		typ = POOL;	break;
	}

	create_feature(lev, SP_COORD_X(OV_i(coord)), SP_COORD_Y(OV_i(coord)),
		       coder->croom, typ);

	opvar_free(coord);
}

static void spo_trap(struct sp_coder *coder, struct level *lev)
{
	struct opvar *type, *coord;
	trap tmptrap;

	if (!OV_pop_i(type) ||
	    !OV_pop_c(coord)) return;

	tmptrap.x = SP_COORD_X(OV_i(coord));
	tmptrap.y = SP_COORD_Y(OV_i(coord));
	tmptrap.type = OV_i(type);

	create_trap(lev, &tmptrap, coder->croom);

	opvar_free(coord);
	opvar_free(type);
}

static void spo_gold(struct sp_coder *coder, struct level *lev)
{
	struct opvar *coord, *amt;
	gold tmpgold;

	if (!OV_pop_i(amt) ||
	    !OV_pop_c(coord)) return;

	tmpgold.x = SP_COORD_X(OV_i(coord));
	tmpgold.y = SP_COORD_Y(OV_i(coord));
	tmpgold.amount = OV_i(amt);

	create_gold(lev, &tmpgold, coder->croom);

	opvar_free(coord);
	opvar_free(amt);
}

static void spo_corridor(struct sp_coder *coder, struct level *lev)
{
	struct opvar *deswall, *desdoor, *desroom,
		     *srcwall, *srcdoor, *srcroom;
	corridor tc;

	if (!OV_pop_i(deswall) ||
	    !OV_pop_i(desdoor) ||
	    !OV_pop_i(desroom) ||
	    !OV_pop_i(srcwall) ||
	    !OV_pop_i(srcdoor) ||
	    !OV_pop_i(srcroom)) return;

	tc.src.room = OV_i(srcroom);
	tc.src.door = OV_i(srcdoor);
	tc.src.wall = OV_i(srcwall);
	tc.dest.room = OV_i(desroom);
	tc.dest.door = OV_i(desdoor);
	tc.dest.wall = OV_i(deswall);

	create_corridor(lev, &tc);

	opvar_free(deswall);
	opvar_free(desdoor);
	opvar_free(desroom);
	opvar_free(srcwall);
	opvar_free(srcdoor);
	opvar_free(srcroom);
}

static void spo_terrain(struct sp_coder *coder, struct level *lev)
{
	struct opvar *coord1, *x2, *y2, *areatyp, *ter, *tlit;
	terrain tmpterrain;

	if (!OV_pop_i(tlit) ||
	    !OV_pop_i(ter) ||
	    !OV_pop_i(areatyp) ||
	    !OV_pop_i(y2) ||
	    !OV_pop_i(x2) ||
	    !OV_pop_c(coord1)) return;

	tmpterrain.x1 = SP_COORD_X(OV_i(coord1));
	tmpterrain.y1 = SP_COORD_Y(OV_i(coord1));
	tmpterrain.x2 = OV_i(x2);
	tmpterrain.y2 = OV_i(y2);
	tmpterrain.areatyp = OV_i(areatyp);
	tmpterrain.ter = OV_i(ter);
	tmpterrain.tlit = OV_i(tlit);

	set_terrain(lev, &tmpterrain, coder->croom);

	opvar_free(coord1);
	opvar_free(x2);
	opvar_free(y2);
	opvar_free(areatyp);
	opvar_free(ter);
	opvar_free(tlit);
}

static void spo_replace_terrain(struct sp_coder *coder, struct level *lev)
{
	struct opvar *x1, *y1, *x2, *y2, *from_ter, *to_ter, *to_lit, *chance;
	replaceterrain rt;

	if (!OV_pop_i(chance) ||
	    !OV_pop_i(to_lit) ||
	    !OV_pop_i(to_ter) ||
	    !OV_pop_i(from_ter) ||
	    !OV_pop_i(y2) ||
	    !OV_pop_i(x2) ||
	    !OV_pop_i(y1) ||
	    !OV_pop_i(x1)) return;

	rt.chance = OV_i(chance);
	rt.tolit = OV_i(to_lit);
	rt.toter = OV_i(to_ter);
	rt.fromter = OV_i(from_ter);
	rt.x1 = OV_i(x1);
	rt.y1 = OV_i(y1);
	rt.x2 = OV_i(x2);
	rt.y2 = OV_i(y2);

	replace_terrain(lev, &rt, coder->croom);

	opvar_free(x1);
	opvar_free(y1);
	opvar_free(x2);
	opvar_free(y2);
	opvar_free(from_ter);
	opvar_free(to_ter);
	opvar_free(to_lit);
	opvar_free(chance);
}

static void spo_randline(struct sp_coder *coder, struct level *lev)
{
	struct opvar *x1, *y1, *x2, *y2, *fg, *lit, *roughness, *thick;
	randline rl;

	if (!OV_pop_i(thick) ||
	    !OV_pop_i(roughness) ||
	    !OV_pop_i(lit) ||
	    !OV_pop_i(fg) ||
	    !OV_pop_i(y2) ||
	    !OV_pop_i(x2) ||
	    !OV_pop_i(y1) ||
	    !OV_pop_i(x1)) return;

	rl.thick = OV_i(thick);
	rl.roughness = OV_i(roughness);
	rl.lit = OV_i(lit);
	rl.fg = OV_i(fg);
	rl.x1 = OV_i(x1);
	rl.y1 = OV_i(y1);
	rl.x2 = OV_i(x2);
	rl.y2 = OV_i(y2);

	line_midpoint(lev, &rl, coder->croom);

	opvar_free(x1);
	opvar_free(y1);
	opvar_free(x2);
	opvar_free(y2);
	opvar_free(fg);
	opvar_free(lit);
	opvar_free(roughness);
	opvar_free(thick);
}

static void spo_spill(struct sp_coder *coder, struct level *lev)
{
	struct opvar *coord, *typ, *dir, *count, *lit;
	spill sp;

	if (!OV_pop_i(lit) ||
	    !OV_pop_i(count) ||
	    !OV_pop_i(dir) ||
	    !OV_pop_i(typ) ||
	    !OV_pop_c(coord)) return;

	sp.x = SP_COORD_X(OV_i(coord));
	sp.y = SP_COORD_Y(OV_i(coord));

	sp.lit = OV_i(lit);
	sp.count = OV_i(count);
	sp.direction = OV_i(dir);
	sp.typ = OV_i(typ);

	spill_terrain(lev, &sp, coder->croom);

	opvar_free(coord);
	opvar_free(typ);
	opvar_free(dir);
	opvar_free(count);
	opvar_free(lit);
}

static void spo_levregion(struct sp_coder *coder, struct level *lev)
{
	struct opvar *rname, *padding, *rtype,
		     *del_islev, *dy2, *dx2, *dy1, *dx1,
		     *in_islev, *iy2, *ix2, *iy1, *ix1;
	lev_region *tmplregion;

	if (!OV_pop_s(rname) ||
	    !OV_pop_i(padding) ||
	    !OV_pop_i(rtype) ||
	    !OV_pop_i(del_islev) ||
	    !OV_pop_i(dy2) ||
	    !OV_pop_i(dx2) ||
	    !OV_pop_i(dy1) ||
	    !OV_pop_i(dx1) ||
	    !OV_pop_i(in_islev) ||
	    !OV_pop_i(iy2) ||
	    !OV_pop_i(ix2) ||
	    !OV_pop_i(iy1) ||
	    !OV_pop_i(ix1)) return;

	tmplregion = malloc(sizeof(lev_region));

	tmplregion->inarea.x1 = OV_i(ix1);
	tmplregion->inarea.y1 = OV_i(iy1);
	tmplregion->inarea.x2 = OV_i(ix2);
	tmplregion->inarea.y2 = OV_i(iy2);

	tmplregion->delarea.x1 = OV_i(dx1);
	tmplregion->delarea.y1 = OV_i(dy1);
	tmplregion->delarea.x2 = OV_i(dx2);
	tmplregion->delarea.y2 = OV_i(dy2);

	tmplregion->in_islev = OV_i(in_islev);
	tmplregion->del_islev = OV_i(del_islev);
	tmplregion->rtype = OV_i(rtype);
	tmplregion->padding = OV_i(padding);
	tmplregion->rname.str = strdup(OV_s(rname));

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
	    num_lregions++;
	    lregions = newl;
	} else {
	    num_lregions = 1;
	    lregions = malloc(sizeof(lev_region));
	}
	memcpy(&lregions[num_lregions - 1], tmplregion, sizeof(lev_region));

	opvar_free(dx1);
	opvar_free(dy1);
	opvar_free(dx2);
	opvar_free(dy2);

	opvar_free(ix1);
	opvar_free(iy1);
	opvar_free(ix2);
	opvar_free(iy2);

	opvar_free(del_islev);
	opvar_free(in_islev);
	opvar_free(rname);
	opvar_free(rtype);
	opvar_free(padding);
}

static void spo_region(struct sp_coder *coder, struct level *lev)
{
	struct opvar *rtype, *rlit, *rirreg, *area;
	xchar dx1, dy1, dx2, dy2;
	struct mkroom *troom;
	boolean prefilled, room_not_needed;

	if (!OV_pop_i(rirreg) ||
	    !OV_pop_i(rtype) ||
	    !OV_pop_i(rlit) ||
	    !OV_pop_r(area)) return;

	if (OV_i(rtype) > MAXRTYPE) {
	    OV_i(rtype) -= MAXRTYPE + 1;
	    prefilled = TRUE;
	} else {
	    prefilled = FALSE;
	}

	if (OV_i(rlit) < 0) {
	    OV_i(rlit) = (rnd(1 + abs(depth(&lev->z))) < 11 && rn2(77)) ?
			 TRUE : FALSE;
	}

	dx1 = SP_REGION_X1(OV_i(area));
	dy1 = SP_REGION_Y1(OV_i(area));
	dx2 = SP_REGION_X2(OV_i(area));
	dy2 = SP_REGION_Y2(OV_i(area));

	get_location(lev, &dx1, &dy1, DRY|WET, NULL);
	get_location(lev, &dx2, &dy2, DRY|WET, NULL);

	/* for an ordinary room, `prefilled' is a flag to force
	   an actual room to be created (such rooms are used to
	   control placement of migrating monster arrivals) */
	room_not_needed = (OV_i(rtype) == OROOM &&
			   !OV_i(rirreg) && !prefilled);
	if (room_not_needed || lev->nroom >= MAXNROFROOMS) {
	    region tmpregion;
	    if (!room_not_needed)
		impossible("Too many rooms on new level!");
	    tmpregion.rlit = OV_i(rlit);
	    tmpregion.x1 = dx1;
	    tmpregion.y1 = dy1;
	    tmpregion.x2 = dx2;
	    tmpregion.y2 = dy2;
	    light_region(lev, &tmpregion);

	    opvar_free(area);
	    opvar_free(rirreg);
	    opvar_free(rlit);
	    opvar_free(rtype);

	    return;
	}

	troom = &lev->rooms[lev->nroom];

	/* mark rooms that must be filled, but do it later */
	if (OV_i(rtype) != OROOM)
	    troom->needfill = prefilled ? 2 : 1;

	if (OV_i(rirreg)) {
	    min_rx = max_rx = dx1;
	    min_ry = max_ry = dy1;
	    flood_fill_rm(lev, dx1, dy1, lev->nroom + ROOMOFFSET,
			  OV_i(rlit), TRUE);
	    add_room(lev, min_rx, min_ry, max_rx, max_ry,
		     FALSE, OV_i(rtype), TRUE);
	    troom->rlit = OV_i(rlit);
	    troom->irregular = TRUE;
	} else {
	    add_room(lev, dx1, dy1, dx2, dy2,
		     OV_i(rlit), OV_i(rtype), TRUE);
	    topologize(lev, troom);	/* set roomno */
	}

	if (!room_not_needed) {
	    if (coder->n_subroom > 1) {
		impossible("spo_region: region as subroom");
	    } else {
		coder->tmproomlist[coder->n_subroom] = troom;
		coder->failed_room[coder->n_subroom] = FALSE;
		coder->n_subroom++;
	    }
	}

	opvar_free(area);
	opvar_free(rirreg);
	opvar_free(rlit);
	opvar_free(rtype);
}

static void spo_drawbridge(struct sp_coder *coder, struct level *lev)
{
	struct opvar *dir, *db_open, *coord;
	xchar x, y;

	if (!OV_pop_i(dir) ||
	    !OV_pop_i(db_open) ||
	    !OV_pop_c(coord)) return;

	x = SP_COORD_X(OV_i(coord));
	y = SP_COORD_Y(OV_i(coord));
	get_location(lev, &x, &y, DRY|WET, coder->croom);
	if (!create_drawbridge(lev, x, y, OV_i(dir), OV_i(db_open)))
	    impossible("Cannot create drawbridge.");
	SpLev_Map[x][y] = 1;

	opvar_free(coord);
	opvar_free(db_open);
	opvar_free(dir);
}

static void spo_mazewalk(struct sp_coder *coder, struct level *lev)
{
	struct opvar *ftyp, *fstocked, *fdir, *coord;
	xchar x, y;
	int dir;

	if (!OV_pop_i(ftyp) ||
	    !OV_pop_i(fstocked) ||
	    !OV_pop_i(fdir) ||
	    !OV_pop_c(coord)) return;

	dir = OV_i(fdir);
	x = SP_COORD_X(OV_i(coord));
	y = SP_COORD_Y(OV_i(coord));

	get_location(lev, &x, &y, DRY|WET, coder->croom);

	if (OV_i(ftyp) < 1)
	    OV_i(ftyp) = ROOM;

	/* don't use move() - it doesn't use W_NORTH, etc. */
	switch (dir) {
	case W_NORTH:	--y;	break;
	case W_SOUTH:	y++;	break;
	case W_EAST:	x++;	break;
	case W_WEST:	--x;	break;
	default:
	    impossible("spo_mazewalk: Bad MAZEWALK direction");
	}

	if (!IS_DOOR(lev->locations[x][y].typ)) {
	    lev->locations[x][y].typ = OV_i(ftyp);
	    lev->locations[x][y].flags = 0;
	}

	/* We must be sure that the parity of the coordinates for
	 * walkfrom() is odd.  But we must also take into account
	 * what direction was chosen.
	 */
	if (!(x % 2)) {
	    if (dir == W_EAST)
		x++;
	    else
		x--;

	    /* no need for IS_DOOR check; out of map bounds */
	    lev->locations[x][y].typ = OV_i(ftyp);
	    lev->locations[x][y].flags = 0;
	}

	if (!(y % 2)) {
	    if (dir == W_SOUTH)
		y++;
	    else
		y--;
	}

	walkfrom(lev, x, y, OV_i(ftyp));
	if (OV_i(fstocked)) fill_empty_maze(lev);

	opvar_free(coord);
	opvar_free(fdir);
	opvar_free(fstocked);
	opvar_free(ftyp);
}

static void spo_wall_property(struct sp_coder *coder, struct level *lev)
{
	struct opvar *x1, *y1, *x2, *y2;
	xchar dx1, dy1, dx2, dy2;
	int wprop = (coder->opcode == SPO_NON_DIGGABLE) ?
		    W_NONDIGGABLE : W_NONPASSWALL;

	if (!OV_pop_i(y2) ||
	    !OV_pop_i(x2) ||
	    !OV_pop_i(y1) ||
	    !OV_pop_i(x1)) return;

	dx1 = OV_i(x1);
	dy1 = OV_i(y1);
	dx2 = OV_i(x2);
	dy2 = OV_i(y2);

	get_location(lev, &dx1, &dy1, DRY|WET, NULL);
	get_location(lev, &dx2, &dy2, DRY|WET, NULL);

	set_wall_property(lev, dx1, dy1, dx2, dy2, wprop);

	opvar_free(x1);
	opvar_free(y1);
	opvar_free(x2);
	opvar_free(y2);
}

static void spo_room_door(struct sp_coder *coder, struct level *lev)
{
	struct opvar *wall, *secret, *mask, *pos;
	room_door tmpd;

	if (!OV_pop_i(wall) ||
	    !OV_pop_i(secret) ||
	    !OV_pop_i(mask) ||
	    !OV_pop_i(pos) ||
	    !coder->croom) return;

	tmpd.secret = OV_i(secret);
	tmpd.mask = OV_i(mask);
	tmpd.pos = OV_i(pos);
	tmpd.wall = OV_i(wall);

	create_door(lev, &tmpd, coder->croom);

	opvar_free(wall);
	opvar_free(secret);
	opvar_free(mask);
	opvar_free(pos);
}

static void spo_wallify(struct sp_coder *coder, struct level *lev)
{
	struct opvar *x1, *y1, *x2, *y2;
	int dx1, dy1, dx2, dy2;

	if (!OV_pop_i(y2) ||
	    !OV_pop_i(x2) ||
	    !OV_pop_i(y1) ||
	    !OV_pop_i(x1)) return;

	dx1 = (OV_i(x1) < 0) ? xstart : OV_i(x1);
	dy1 = (OV_i(y1) < 0) ? ystart : OV_i(y1);
	dx2 = (OV_i(x2) < 0) ? xstart + xsize : OV_i(x2);
	dy2 = (OV_i(y2) < 0) ? ystart + ysize : OV_i(y2);

	wallify_map(lev, dx1, dy1, dx2, dy2);

	opvar_free(x1);
	opvar_free(y1);
	opvar_free(x2);
	opvar_free(y2);
}

static void spo_map(struct sp_coder *coder, struct level *lev)
{
	struct opvar *mpxs, *mpys, *mpmap, *mpa, *mpkeepr, *mpzalign;
	mazepart tmpmazepart;
	xchar halign, valign;
	xchar tmpxstart, tmpystart, tmpxsize, tmpysize;

	if (!OV_pop_i(mpxs) ||
	    !OV_pop_i(mpys) ||
	    !OV_pop_s(mpmap) ||
	    !OV_pop_i(mpkeepr) ||
	    !OV_pop_i(mpzalign) ||
	    !OV_pop_c(mpa)) return;

	tmpmazepart.xsize = OV_i(mpxs);
	tmpmazepart.ysize = OV_i(mpys);
	tmpmazepart.zaligntyp = OV_i(mpzalign);
	tmpmazepart.halign = SP_COORD_X(OV_i(mpa));
	tmpmazepart.valign = SP_COORD_Y(OV_i(mpa));

	tmpxsize = xsize;
	tmpysize = ysize;
	tmpxstart = xstart;
	tmpystart = ystart;

	halign = tmpmazepart.halign;
	valign = tmpmazepart.valign;
	xsize = tmpmazepart.xsize;
	ysize = tmpmazepart.ysize;
	switch (tmpmazepart.zaligntyp) {
	default:
	case 0:
	    break;
	case 1:
	    switch((int)halign) {
	    case LEFT:	    xstart = 3;					break;
	    case H_LEFT:    xstart = 2 + (x_maze_max-2-xsize) / 4;	break;
	    case CENTER:    xstart = 2 + (x_maze_max-2-xsize) / 2;	break;
	    case H_RIGHT:   xstart = 2 + (x_maze_max-2-xsize) * 3 / 4;	break;
	    case RIGHT:	    xstart = x_maze_max-xsize-1;		break;
	    }
	    switch((int)valign) {
	    case TOP:	    ystart = 3;					break;
	    case CENTER:    ystart = 2 + (y_maze_max-2-ysize) / 2;	break;
	    case BOTTOM:    ystart = y_maze_max-ysize-1;		break;
	    }
	    if (!(xstart % 2)) xstart++;
	    if (!(ystart % 2)) ystart++;
	    break;
	case 2:
	    get_location(lev, &halign, &valign, DRY|WET, coder->croom);
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
	    xchar x, y;
	    /* Load the map */
	    for (y = ystart; y < ystart + ysize; y++) {
		for (x = xstart; x < xstart + xsize; x++) {
		    struct rm *loc;
		    xchar mptyp = mpmap->vardata.str[(y-ystart) * xsize +
						     (x-xstart)] - 1;
		    if (mptyp >= MAX_TYPE) continue;
		    loc = &lev->locations[x][y];
		    loc->typ = mptyp;
		    loc->lit = FALSE;
		    /* clear out lev->locations: load_common_data may set them */
		    loc->flags = 0;
		    loc->horizontal = 0;
		    loc->roomno = 0;
		    loc->edge = 0;
		    /*
		     * Set secret doors to closed (why not trapped too?).  Set
		     * the horizontal bit.
		     */
		    if (loc->typ == SDOOR || IS_DOOR(loc->typ)) {
			if (loc->typ == SDOOR)
			    loc->doormask = D_CLOSED;
			/*
			 *  If there is a wall to the left that connects to a
			 *  (secret) door, then it is horizontal.  This does
			 *  not allow (secret) doors to be corners of rooms.
			 */
			if (x != xstart && (IS_WALL(lev->locations[x-1][y].typ) ||
					    lev->locations[x-1][y].horizontal))
			    loc->horizontal = 1;
		    } else if (loc->typ == HWALL || loc->typ == IRONBARS) {
			loc->horizontal = 1;
		    } else if (loc->typ == LAVAPOOL) {
			loc->lit = 1;
		    }
		}
	    }
	    if (coder->lvl_is_joined)
		remove_rooms(lev, xstart, ystart, xstart + xsize, ystart + ysize);
	}
	if (!OV_i(mpkeepr)) {
	    xstart = tmpxstart;
	    ystart = tmpystart;
	    xsize = tmpxsize;
	    ysize = tmpysize;
	}

	opvar_free(mpxs);
	opvar_free(mpys);
	opvar_free(mpmap);
	opvar_free(mpa);
	opvar_free(mpkeepr);
	opvar_free(mpzalign);
}

static void spo_jmp(struct sp_coder *coder, sp_lev *lvl)
{
	struct opvar *tmpa;
	long a;

	if (!OV_pop_i(tmpa)) return;

	a = sp_code_jmpaddr(coder->frame->n_opcode, OV_i(tmpa) - 1);
	if (a >= 0 && a < lvl->n_opcodes &&
	    a != coder->frame->n_opcode)
	    coder->frame->n_opcode = a;

	opvar_free(tmpa);
}

static void spo_conditional_jump(struct sp_coder *coder, sp_lev *lvl)
{
	struct opvar *oa, *oc;
	long a, c;
	int test;

	if (!OV_pop_i(oa) ||
	    !OV_pop_i(oc)) return;

	a = sp_code_jmpaddr(coder->frame->n_opcode, OV_i(oa) - 1);
	c = OV_i(oc);

	switch (coder->opcode) {
	default: impossible("spo_conditional_jump: illegal opcode"); break;
	case SPO_JL:	test = (c <  0); break;
	case SPO_JLE:	test = (c <= 0); break;
	case SPO_JG:	test = (c >  0); break;
	case SPO_JGE:	test = (c >= 0); break;
	case SPO_JE:	test = (c == 0); break;
	case SPO_JNE:	test = (c != 0); break;
	}

	if (test && a >= 0 &&
	    a < lvl->n_opcodes &&
	    a != coder->frame->n_opcode)
	    coder->frame->n_opcode = a;

	opvar_free(oa);
	opvar_free(oc);
}

static void spo_var_init(struct sp_coder *coder)
{
	struct opvar *vname;
	struct opvar *arraylen;
	struct opvar *vvalue;
	struct splev_var *tmpvar;
	struct splev_var *tmp2;
	long idx;

	OV_pop_s(vname);
	OV_pop_i(arraylen);

	if (!vname || !arraylen)
	    panic("no values for SPO_VAR_INIT");

	tmpvar = opvar_var_defined(coder, OV_s(vname));

	if (tmpvar) {
	    /* variable redefinition */
	    if (OV_i(arraylen) < 0) {
		/* copy variable */
		if (tmpvar->array_len) {
		    idx = tmpvar->array_len;
		    while (idx-- > 0)
			opvar_free(tmpvar->data.arrayvalues[idx]);
		    free(tmpvar->data.arrayvalues);
		} else {
		    opvar_free(tmpvar->data.value);
		}
		tmpvar->data.arrayvalues = NULL;
		goto copy_variable;
	    } else if (OV_i(arraylen)) {
		/* redefined array */
		idx = tmpvar->array_len;
		while (idx-- > 0)
		    opvar_free(tmpvar->data.arrayvalues[idx]);
		free(tmpvar->data.arrayvalues);
		tmpvar->data.arrayvalues = NULL;
		goto create_new_array;
	    } else {
		/* redefined single value */
		OV_pop(vvalue);
		if (tmpvar->svtyp != vvalue->spovartyp)
		    panic("redefining variable as different type");
		opvar_free(tmpvar->data.value);
		tmpvar->data.value = vvalue;
		tmpvar->array_len = 0;
	    }
	} else {
	    /* new variable definition */
	    tmpvar = malloc(sizeof(struct splev_var));
	    if (!tmpvar) return;

	    tmpvar->next = coder->frame->variables;
	    tmpvar->name = strdup(OV_s(vname));
	    coder->frame->variables = tmpvar;

	    if (OV_i(arraylen) < 0) {
		/* copy variable */
copy_variable:
		OV_pop(vvalue);
		tmp2 = opvar_var_defined(coder, OV_s(vvalue));
		if (!tmp2)
		    panic("no copyable var");
		tmpvar->svtyp = tmp2->svtyp;
		tmpvar->array_len = tmp2->array_len;
		if (tmpvar->array_len) {
		    idx = tmpvar->array_len;
		    tmpvar->data.arrayvalues = malloc(sizeof(struct opvar *) * idx);
		    while (idx-- > 0) {
			tmpvar->data.arrayvalues[idx] =
				opvar_clone(tmp2->data.arrayvalues[idx]);
		    }
		} else {
		    tmpvar->data.value = opvar_clone(tmp2->data.value);
		}
		opvar_free(vvalue);
	    } else if (OV_i(arraylen)) {
		/* new array */
create_new_array:
		idx = OV_i(arraylen);
		tmpvar->array_len = idx;
		tmpvar->data.arrayvalues = malloc(sizeof(struct opvar *) * idx);
		if (!tmpvar->data.arrayvalues)
		    panic("malloc tmpvar->data.arrayvalues");
		while (idx-- > 0) {
		    OV_pop(vvalue);
		    if (!vvalue)
			panic("no value for arrayvariable");
		    tmpvar->data.arrayvalues[idx] = vvalue;
		}
		tmpvar->svtyp = SPOVAR_ARRAY;
	    } else {
		/* new single value */
		OV_pop(vvalue);
		if (!vvalue)
		    panic("no value for variable");
		tmpvar->svtyp = OV_typ(vvalue);
		tmpvar->data.value = vvalue;
		tmpvar->array_len = 0;
	    }
	}

	opvar_free(vname);
	opvar_free(arraylen);
}

static void spo_shuffle_array(struct sp_coder *coder)
{
	struct opvar *vname;
	struct splev_var *tmp;
	struct opvar *tmp2;
	long i, j;

	if (!OV_pop_s(vname)) return;

	tmp = opvar_var_defined(coder, OV_s(vname));
	if (!tmp || tmp->array_len < 1)
	    return;

	for (i = tmp->array_len - 1; i > 0; i--) {
	    if ((j = rn2(i + 1)) == i)
		continue;
	    tmp2 = tmp->data.arrayvalues[j];
	    tmp->data.arrayvalues[j] = tmp->data.arrayvalues[i];
	    tmp->data.arrayvalues[i] = tmp2;
	}

	opvar_free(vname);
}

/*
 * Special level coder; creates the special level from the sp_lev codes.
 * Does not free the allocated memory.
 */
static boolean sp_level_coder(struct level *lev, sp_lev *lvl)
{
	long exec_opcodes = 0;
	int tmpi;
	struct sp_coder *coder = malloc(sizeof(struct sp_coder));

	coder->frame = frame_new(0);
	coder->stack = NULL;
	coder->premapped = FALSE;
	coder->allow_flips = 3;
	coder->croom = NULL;
	coder->n_subroom = 1;
	coder->exit_script = FALSE;
	coder->lvl_is_joined = 0;

	for (tmpi = 0; tmpi <= MAX_NESTED_ROOMS; tmpi++) {
	    coder->tmproomlist[tmpi] = NULL;
	    coder->failed_room[tmpi] = FALSE;
	}

	shuffle_alignments();

	for (tmpi = 0; tmpi < MAX_CONTAINMENT; tmpi++)
	    container_obj[tmpi] = NULL;
	container_idx = 0;

	invent_carrying_monster = NULL;

	memset(&SpLev_Map[0][0], 0, sizeof(SpLev_Map));

	lev->flags.is_maze_lev = 0;

	xstart = 1;
	ystart = 0;
	xsize = COLNO - 1;
	ysize = ROWNO;

	while (coder->frame->n_opcode < lvl->n_opcodes && !coder->exit_script) {
	    coder->opcode = lvl->opcodes[coder->frame->n_opcode].opcode;
	    coder->opdat = lvl->opcodes[coder->frame->n_opcode].opdat;

	    coder->stack = coder->frame->stack;

	    if (exec_opcodes++ > SPCODER_MAX_RUNTIME) {
		impossible("Level script is taking too much time, stopping.");
		coder->exit_script = TRUE;
	    }

	    if (coder->failed_room[coder->n_subroom - 1] &&
		coder->opcode != SPO_ENDROOM &&
		coder->opcode != SPO_ROOM &&
		coder->opcode != SPO_SUBROOM) goto next_opcode;

	    coder->croom = coder->tmproomlist[coder->n_subroom - 1];

	    switch (coder->opcode) {
	    case SPO_NULL:		break;
	    case SPO_EXIT:		coder->exit_script = TRUE;	break;
	    case SPO_FRAME_PUSH:	spo_frame_push(coder);		break;
	    case SPO_FRAME_POP:		spo_frame_pop(coder);		break;
	    case SPO_CALL:		spo_call(coder);		break;
	    case SPO_RETURN:		spo_return(coder);		break;
	    case SPO_END_MONINVENT:	spo_end_moninvent(coder, lev);	break;
	    case SPO_POP_CONTAINER:	spo_pop_container(coder);	break;
	    case SPO_POP:
		{
		    struct opvar *ov = splev_stack_pop(coder->stack);
		    opvar_free(ov);
		}
		break;
	    case SPO_PUSH:
		splev_stack_push(coder->stack, opvar_clone(coder->opdat));
		break;
	    case SPO_MESSAGE:		spo_message(coder);		break;
	    case SPO_MONSTER:		spo_monster(coder, lev);	break;
	    case SPO_OBJECT:		spo_object(coder, lev);		break;
	    case SPO_LEVEL_FLAGS:	spo_level_flags(coder, lev);	break;
	    case SPO_INITLEVEL:		spo_initlevel(coder, lev);	break;
	    case SPO_MON_GENERATION:	spo_mon_generation(coder, lev);	break;
	    case SPO_LEVEL_SOUNDS:	spo_level_sounds(coder, lev);	break;
	    case SPO_ENGRAVING:		spo_engraving(coder, lev);	break;
	    case SPO_SUBROOM:		/* fall through */
	    case SPO_ROOM:		spo_room(coder, lev);		break;
	    case SPO_ENDROOM:		spo_endroom(coder);		break;
	    case SPO_DOOR:		spo_door(coder, lev);		break;
	    case SPO_STAIR:		spo_stair(coder, lev);		break;
	    case SPO_LADDER:		spo_ladder(coder, lev);		break;
	    case SPO_GRAVE:		spo_grave(coder, lev);		break;
	    case SPO_ALTAR:		spo_altar(coder, lev);		break;
	    case SPO_SINK:		/* fall through */
	    case SPO_POOL:		/* fall through */
	    case SPO_FOUNTAIN:		spo_feature(coder, lev);	break;
	    case SPO_WALLWALK:		spo_wallwalk(coder, lev);	break;
	    case SPO_TRAP:		spo_trap(coder, lev);		break;
	    case SPO_GOLD:		spo_gold(coder, lev);		break;
	    case SPO_CORRIDOR:		spo_corridor(coder, lev);	break;
	    case SPO_TERRAIN:		spo_terrain(coder, lev);	break;
	    case SPO_REPLACETERRAIN:	spo_replace_terrain(coder, lev); break;
	    case SPO_RANDLINE:		spo_randline(coder, lev);	break;
	    case SPO_SPILL:		spo_spill(coder, lev);		break;
	    case SPO_LEVREGION:		spo_levregion(coder, lev);	break;
	    case SPO_REGION:		spo_region(coder, lev);		break;
	    case SPO_DRAWBRIDGE:	spo_drawbridge(coder, lev);	break;
	    case SPO_MAZEWALK:		spo_mazewalk(coder, lev);	break;
	    case SPO_NON_PASSWALL:	/* fall through */
	    case SPO_NON_DIGGABLE:	spo_wall_property(coder, lev);	break;
	    case SPO_ROOM_DOOR:		spo_room_door(coder, lev);	break;
	    case SPO_WALLIFY:		spo_wallify(coder, lev);	break;
	    case SPO_COPY:
		{
		    struct opvar *a = splev_stack_pop(coder->stack);
		    splev_stack_push(coder->stack, opvar_clone(a));
		    splev_stack_push(coder->stack, opvar_clone(a));
		    opvar_free(a);
		}
		break;
	    case SPO_DEC:
		{
		    struct opvar *a;
		    if (!OV_pop_i(a)) break;
		    OV_i(a)--;
		    splev_stack_push(coder->stack, a);
		}
		break;
	    case SPO_CMP:
		{
		    struct opvar *a;
		    struct opvar *b;
		    struct opvar *c;

		    OV_pop(a);
		    OV_pop(b);

		    if (!a || !b) {
			impossible("spo_cmp: no values in stack");
			break;
		    }

		    if (OV_typ(a) != OV_typ(b)) {
			impossible("spo_cmp: trying to compare differing datatypes");
			break;
		    }

		    switch (OV_typ(a)) {
		    case SPOVAR_COORD:
		    case SPOVAR_REGION:
		    case SPOVAR_MAPCHAR:
		    case SPOVAR_MONST:
		    case SPOVAR_OBJ:
		    case SPOVAR_INT:
			c = opvar_new_int(OV_i(b) - OV_i(a));
			break;
		    case SPOVAR_STRING:
			c = opvar_new_int(strcmp(OV_s(b), OV_s(a)));
			break;
		    default:
			c = opvar_new_int(0);
			break;
		    }
		    splev_stack_push(coder->stack, c);
		    opvar_free(a);
		    opvar_free(b);
		}
		break;
	    case SPO_JMP:
		spo_jmp(coder, lvl);
		break;
	    case SPO_JL:
	    case SPO_JLE:
	    case SPO_JG:
	    case SPO_JGE:
	    case SPO_JE:
	    case SPO_JNE:
		spo_conditional_jump(coder, lvl);
		break;
	    case SPO_RN2:
		{
		    struct opvar *tmpv;
		    struct opvar *t;

		    if (!OV_pop_i(tmpv)) break;

		    t = opvar_new_int(OV_i(tmpv) > 1 ? rn2(OV_i(tmpv)) : 0);
		    splev_stack_push(coder->stack, t);
		    opvar_free(tmpv);
		}
		break;
	    case SPO_MAP:
		spo_map(coder, lev);
		break;
	    case SPO_VAR_INIT:
		spo_var_init(coder);
		break;
	    case SPO_SHUFFLE_ARRAY:
		spo_shuffle_array(coder);
		break;
	    default:
		panic("sp_level_coder: Unknown opcode %i", coder->opcode);
	    }

next_opcode:
	    coder->frame->n_opcode++;
	} /* while */

	fill_rooms(lev);
	remove_boundary_syms(lev);

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
		flip_level_rnd(lev, coder->allow_flips);
	}

	count_features(lev);

	if (coder->premapped) sokoban_detect(lev);

	if (coder->frame) {
	    struct sp_frame *tmpframe;
	    do {
		tmpframe = coder->frame->next;
		frame_del(coder->frame);
		coder->frame = tmpframe;
	    } while (coder->frame);
	}

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
