/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#ifndef RM_H
#define RM_H

/*
 * The dungeon presentation graphics code and data structures were rewritten
 * and generalized for NetHack's release 2 by Eric S. Raymond (eric@snark)
 * building on Don G. Kneller's MS-DOS implementation.	See drawing.c for
 * the code that permits the user to set the contents of the symbol structure.
 *
 * The door representation was changed by Ari Huttunen(ahuttune@niksula.hut.fi)
 */

/*
 * TLCORNER	TDWALL		TRCORNER
 * +-		-+-		-+
 * |		 |		 |
 *
 * TRWALL	CROSSWALL	TLWALL		HWALL
 * |		 |		 |
 * +-		-+-		-+		---
 * |		 |		 |
 *
 * BLCORNER	TUWALL		BRCORNER	VWALL
 * |		 |		 |		|
 * +-		-+-		-+		|
 */

/* Level location types */
enum location_types {
	STONE = 0,
	VWALL,
	HWALL,
	TLCORNER,
	TRCORNER,
	BLCORNER,
	BRCORNER,
	CROSSWALL,	/* For pretty mazes and special levels */
	TUWALL,
	TDWALL,
	TLWALL,
	TRWALL,
	DBWALL,
	TREE,		/* KMH */
	DEADTREE,	/* youkan */
	SDOOR,
	SCORR,
	POOL,
	MOAT,		/* pool that doesn't boil, adjust messages */
	WATER,
	DRAWBRIDGE_UP,
	LAVAPOOL,
	IRONBARS,	/* KMH */
	DOOR,
	CORR,
	ROOM,
	STAIRS,
	LADDER,
	FOUNTAIN,
	THRONE,
	SINK,
	GRAVE,
	ALTAR,
	ICE,
	BOG,
	DRAWBRIDGE_DOWN,
	AIR,
	CLOUD,

	MAX_TYPE
};
#define INVALID_TYPE	127

/*
 * Avoid using the level types in inequalities:
 * these types are subject to change.
 * Instead, use one of the macros below.
 */
#define IS_WALL(typ)	((typ) && (typ) <= DBWALL)
#define IS_STWALL(typ)	((typ) <= DBWALL)	/* STONE <= (typ) <= DBWALL */
#define IS_ROCK(typ)	((typ) < POOL)		/* absolutely nonaccessible */
#define IS_DOOR(typ)	((typ) == DOOR)
#define IS_TREE(lev,typ) ((typ) == TREE || \
			  ((lev)->flags.arboreal && (typ) == STONE))
#define IS_DEADTREE(typ) ((typ) == DEADTREE)
#define IS_TREES(lev,typ) (IS_TREE((lev),(typ)) || IS_DEADTREE(typ))
#define ACCESSIBLE(typ) ((typ) >= DOOR)		/* good position */
#define IS_ROOM(typ)	((typ) >= ROOM)		/* ROOM, STAIRS, furniture.. */
#define ZAP_POS(typ)	((typ) >= POOL)
#define SPACE_POS(typ)	((typ) > DOOR)
#define IS_POOL(typ)	((typ) >= POOL && (typ) <= DRAWBRIDGE_UP)
#define IS_THRONE(typ)	((typ) == THRONE)
#define IS_FOUNTAIN(typ) ((typ) == FOUNTAIN)
#define IS_SINK(typ)	((typ) == SINK)
#define IS_GRAVE(typ)	((typ) == GRAVE)
#define IS_ALTAR(typ)	((typ) == ALTAR)
#define IS_SWAMP(typ)	((typ) == BOG)
#define IS_DRAWBRIDGE(typ) ((typ) == DRAWBRIDGE_UP || (typ) == DRAWBRIDGE_DOWN)
#define IS_FURNITURE(typ) ((typ) >= STAIRS && (typ) <= ALTAR)
#define IS_AIR(typ)	((typ) == AIR || (typ) == CLOUD)
#define IS_SOFT(typ)	((typ) == AIR || (typ) == CLOUD || IS_POOL(typ) || (typ) == BOG)

/*
 * The screen symbols may be the default or defined at game startup time.
 * See drawing.c for defaults.
 * Note: {ibm|dec}_graphics[] arrays (also in drawing.c) must be kept in synch.
 */

/* begin dungeon characters */
/* Keep defexplain[] and defsyms[] in drawing.c in sync with this! */
enum dungeon_symbols {
/* 0*/	S_unexplored = 0,
	S_stone,
	S_vwall,
	S_hwall,
	S_tlcorn,
	S_trcorn,
	S_blcorn,
	S_brcorn,
	S_crwall,
	S_tuwall,
/*10*/	S_tdwall,
	S_tlwall,
	S_trwall,
	S_corr,
	S_litcorr,
	S_room,
	S_darkroom,
	S_pool,
	S_air,
	S_cloud,
/*20*/	S_water,
	S_ice,
	S_bog,
	S_lava,
	S_ndoor,
	/* "features" start here */
	S_vodoor,
	S_hodoor,
	S_vcdoor,	/* closed door, vertical wall */
	S_hcdoor,	/* closed door, horizontal wall */
	S_bars,		/* KMH -- iron bars */
	S_tree,		/* KMH */
	S_deadtree,	/* youkan */
	S_upstair,
	S_dnstair,
	S_upladder,
	S_dnladder,
	S_upsstair,
	S_dnsstair,
	S_altar,
	S_grave,
	S_throne,
	S_sink,
	S_fountain,
	S_vodbridge,
	S_hodbridge,
	S_vcdbridge,	/* closed drawbridge, vertical wall */
	S_hcdbridge,	/* closed drawbridge, horizontal wall */

	MAXPCHARS	/* maximum number of mapped characters */
};

#define DUNGEON_FEATURE_OFFSET S_vodoor

/* end dungeon characters, begin special effects */
/* Keep effectsyms[] in drawing.c in sync with this! */
enum effect_symbols {
	E_digbeam = 0,	/* dig beam symbol */
	E_flashbeam,	/* camera flash symbol */
	E_boomleft,	/* thrown boomerang, open left, e.g ')'    */
	E_boomright,	/* thrown boomerang, open right, e.g. '('  */
	E_ss1,		/* 4 magic shield glyphs */
	E_ss2,
	E_ss3,
	E_ss4,
	E_gascloud,
};

/* The 8 swallow symbols.  Do NOT separate.  To change order or add, see */
/* the function swallow_to_effect() in display.c.			 */
/* Keep swallowsyms[] in drawing.c in sync with this! */
enum swallow_symbols {
	S_sw_tl = 0,	/* swallow top left [1]			*/
	S_sw_tc,	/* swallow top center [2]	Order:	*/
	S_sw_tr,	/* swallow top right [3]		*/
	S_sw_ml,	/* swallow middle left [4]	1 2 3	*/
	S_sw_mr,	/* swallow middle right [6]	4 5 6	*/
	S_sw_bl,	/* swallow bottom left [7]	7 8 9	*/
	S_sw_bc,	/* swallow bottom center [8]		*/
	S_sw_br,	/* swallow bottom right [9]		*/
};

/* Keep explsyms[] in drawing.c in sync with this! */
enum explode_symbols {
	E_explode1 = 0,	/* explosion top left			*/
	E_explode2,	/* explosion top center			*/
	E_explode3,	/* explosion top right		 Ex.	*/
	E_explode4,	/* explosion middle left		*/
	E_explode5,	/* explosion middle center	 /-\	*/
	E_explode6,	/* explosion middle right	 |@|	*/
	E_explode7,	/* explosion bottom left	 \-/	*/
	E_explode8,	/* explosion bottom center		*/
	E_explode9,	/* explosion bottom right		*/
};

/* end effects */


extern const struct nh_symdef defsyms[];	/* defaults */

extern const char * const trapexplain[];
extern const char * const defexplain[];
extern const char * const warnexplain[];

/*
 * The 6 possible states of doors
 */

#define D_NODOOR	0
#define D_BROKEN	1
#define D_ISOPEN	2
#define D_CLOSED	4
#define D_LOCKED	8
#define D_TRAPPED	16
#define D_SECRET	32	/* only used by sp_lev.c, NOT in rm-struct */

/*
 * Some altars are considered as shrines, so we need a flag.
 */
#define AM_SHRINE	8

/*
 * Thrones should only be looted once.
 */
#define T_LOOTED	1

/*
 * Trees have more than one kick result.
 */
#define TREE_LOOTED	1
#define TREE_SWARM	2

/*
 * Fountains have limits, and special warnings.
 */
#define F_LOOTED	1
#define F_WARNED	2
#define FOUNTAIN_IS_WARNED(x,y)		(level->locations[x][y].looted & F_WARNED)
#define FOUNTAIN_IS_LOOTED(x,y)		(level->locations[x][y].looted & F_LOOTED)
#define SET_FOUNTAIN_WARNED(x,y)	level->locations[x][y].looted |= F_WARNED;
#define SET_FOUNTAIN_LOOTED(x,y)	level->locations[x][y].looted |= F_LOOTED;
#define CLEAR_FOUNTAIN_WARNED(x,y)	level->locations[x][y].looted &= ~F_WARNED;
#define CLEAR_FOUNTAIN_LOOTED(x,y)	level->locations[x][y].looted &= ~F_LOOTED;

/*
 * Doors are even worse :-) The special warning has a side effect
 * of instantly trapping the door, and if it was defined as trapped,
 * the guards consider that you have already been warned!
 */
#define D_WARNED	16

/*
 * Sinks have 3 different types of loot that shouldn't be abused
 */
#define S_LPUDDING	1
#define S_LDWASHER	2
#define S_LRING		4

/*
 * The four directions for a DrawBridge.
 */
#define DB_NORTH	0
#define DB_SOUTH	1
#define DB_EAST		2
#define DB_WEST		3
#define DB_DIR		3	/* mask for direction */

/*
 * What's under a drawbridge.
 */
#define DB_MOAT		0x00
#define DB_LAVA		0x04
#define DB_ICE		0x08
#define DB_FLOOR	0x0c
#define DB_BOG		0x10
#define DB_GROUND	0x14
#define DB_UNDER	28	/* mask for underneath */

/*
 * Wall information.
 */
#define WM_MASK		0x07	/* wall mode (bottom three bits) */
#define W_NONDIGGABLE	0x08
#define W_NONPASSWALL	0x10

/*
 * Ladders (in Vlad's tower) may be up or down.
 */
#define LA_UP		1
#define LA_DOWN		2

/*
 * Room areas may be iced pools
 */
#define ICED_BOG	4
#define ICED_POOL	8
#define ICED_MOAT	16

/*
 * The structure describing a coordinate position.
 * Before adding fields, remember that this will significantly affect
 * the size of temporary files and save files.
 */
struct rm {
	unsigned mem_bg:6;	/* remembered background */
	unsigned mem_trap:5;	/* remembered trap */
	unsigned mem_obj:10;	/* remembered object */
	unsigned mem_obj_mn:9;	/* monnum of remembered corpses, statues, figurines */
	unsigned mem_obj_stacks:1; /* remembered other stacks of objects */
	unsigned mem_obj_prize:1; /* remembered object is a level prize */

	schar typ;		/* what is really there */
	uchar seenv;		/* seen vector */

	unsigned mem_door_l:1;	/* player knows whether or not door is locked */
	unsigned mem_door_t:1;	/* player knows whether or not door is trapped */
	unsigned mem_stepped:1;	/* has this square been stepped on? */
	unsigned mem_invis:1;	/* remembered invisible monster encounter */
	unsigned flags:5;	/* extra information for typ */
	unsigned horizontal:1; /* wall/door/etc is horiz. (more typ info) */
	unsigned lit:1;	/* speed hack for lit rooms */
	unsigned waslit:1;	/* remember if a location was lit */
	unsigned roomno:6;	/* room # for special rooms */
	unsigned edge:1;	/* marks boundaries for special rooms*/
};

#define SET_TYPLIT(lev, x, y, ttyp, llit)			\
do {								\
	if ((ttyp) < MAX_TYPE)					\
	    (lev)->locations[(x)][(y)].typ = (ttyp);		\
	if ((ttyp) == LAVAPOOL) {				\
	    (lev)->locations[(x)][(y)].lit = 1;			\
	} else if ((schar)(llit) != -2) {			\
	    if ((schar)(llit) == -1)				\
		(lev)->locations[(x)][(y)].lit = rn2(2);	\
	    else						\
		(lev)->locations[(x)][(y)].lit = (llit);	\
	}							\
} while (0)

/*
 * Add wall angle viewing by defining "modes" for each wall type.  Each
 * mode describes which parts of a wall are finished (seen as as wall)
 * and which are unfinished (seen as rock).
 *
 * We use the bottom 3 bits of the flags field for the mode.  This comes
 * in conflict with secret doors, but we avoid problems because until
 * a secret door becomes discovered, we know what sdoor's bottom three
 * bits are.
 *
 * The following should cover all of the cases.
 *
 *	type	mode				Examples: R=rock, F=finished
 *	-----	----				----------------------------
 *	WALL:	0 none				hwall, mode 1
 *		1 left/top (1/2 rock)			RRR
 *		2 right/bottom (1/2 rock)		---
 *							FFF
 *
 *	CORNER: 0 none				trcorn, mode 2
 *		1 outer (3/4 rock)			FFF
 *		2 inner (1/4 rock)			F+-
 *							F|R
 *
 *	TWALL:	0 none				tlwall, mode 3
 *		1 long edge (1/2 rock)			F|F
 *		2 bottom left (on a tdwall)		-+F
 *		3 bottom right (on a tdwall)		R|F
 *
 *	CRWALL: 0 none				crwall, mode 5
 *		1 top left (1/4 rock)			R|F
 *		2 top right (1/4 rock)			-+-
 *		3 bottom left (1/4 rock)		F|R
 *		4 bottom right (1/4 rock)
 *		5 top left & bottom right (1/2 rock)
 *		6 bottom left & top right (1/2 rock)
 */

#define WM_W_LEFT 1			/* vertical or horizontal wall */
#define WM_W_RIGHT 2
#define WM_W_TOP WM_W_LEFT
#define WM_W_BOTTOM WM_W_RIGHT

#define WM_C_OUTER 1			/* corner wall */
#define WM_C_INNER 2

#define WM_T_LONG 1			/* T wall */
#define WM_T_BL   2
#define WM_T_BR   3

#define WM_X_TL   1			/* cross wall */
#define WM_X_TR   2
#define WM_X_BL   3
#define WM_X_BR   4
#define WM_X_TLBR 5
#define WM_X_BLTR 6

/*
 * Seen vector values.	The seen vector is an array of 8 bits, one for each
 * octant around a given center x:
 *
 *			0 1 2
 *			7 x 3
 *			6 5 4
 *
 * In the case of walls, a single wall square can be viewed from 8 possible
 * directions.	If we know the type of wall and the directions from which
 * it has been seen, then we can determine what it looks like to the hero.
 */
#define SV0 0x1
#define SV1 0x2
#define SV2 0x4
#define SV3 0x8
#define SV4 0x10
#define SV5 0x20
#define SV6 0x40
#define SV7 0x80
#define SVALL 0xFF



#define doormask	flags
#define altarmask	flags
#define wall_info	flags
#define ladder		flags
#define drawbridgemask	flags
#define looted		flags
#define icedpool	flags

#define blessedftn	horizontal  /* a fountain that grants attribs */
#define disturbed	horizontal  /* a grave that has been disturbed */

struct damage {
	struct damage *next;
	long when, cost;
	coord place;
	schar typ;
};

struct levelflags {
	uchar	nfountains;		/* number of fountains on level */
	uchar	nsinks;			/* number of sinks on the level */
	int	purge_monsters;	/* number of dead monsters still on level->monlist */

	/* Several flags that give hints about what's on the level */
	unsigned has_shop:1;
	unsigned has_vault:1;
	unsigned has_zoo:1;
	unsigned has_court:1;
	unsigned has_morgue:1;
	unsigned has_garden:1;
	unsigned has_beehive:1;
	unsigned has_barracks:1;
	unsigned has_temple:1;
	unsigned has_lemurepit:1;

	unsigned has_swamp:1;
	unsigned noteleport:1;
	unsigned hardfloor:1;
	unsigned nommap:1;
	unsigned hero_memory:1;		/* hero has memory */
	unsigned shortsighted:1;	/* monsters are shortsighted */
	unsigned graveyard:1;		/* has_morgue, but remains set */
	unsigned is_maze_lev:1;
	unsigned stormy:1;		/* thunderous clouds */

	unsigned is_cavernous_lev:1;
	unsigned arboreal:1;		/* Trees replace rock */
	unsigned sky:1;			/* map has sky instead of ceiling */

	unsigned forgotten:1;	/* previously visited but forgotten (amnesia) */
};

struct mon_gen_tuple {
	int freq;
	boolean is_sym;
	int monid;
	struct mon_gen_tuple *next;
};

struct mon_gen_override {
	int override_chance;
	int total_mon_freq;
	struct mon_gen_tuple *gen_chances;
};

#define LVLSND_HEARD	0	/* You_hear(msg); */
#define LVLSND_PLINED	1	/* pline(msg); */
#define LVLSND_VERBAL	2	/* verbalize(msg); */
#define LVLSND_FELT	3	/* pline("You feel %s", msg); */

struct lvl_sound_bite {
	int flags;	/* LVLSND_foo */
	char *msg;
};

struct lvl_sounds {
	int freq;
	int n_sounds;
	struct lvl_sound_bite *sounds;
};

/* worm segment structure */
struct wseg {
    struct wseg *nseg;
    xchar  wx, wy;	/* the segment's position */
};


struct ls_t;
struct level {
    char		levname[64]; /* as given by the player via donamelevel */
    struct rm		locations[COLNO][ROWNO];
    struct obj		*objects[COLNO][ROWNO];
    struct monst	*monsters[COLNO][ROWNO];
    struct obj		*objlist;
    struct obj		*buriedobjlist;
    struct obj		*billobjs; /* objects not yet paid for */
    struct monst	*monlist;
    struct damage	*damagelist;
    struct levelflags	flags;
    struct mon_gen_override *mon_gen;
    struct lvl_sounds	*sounds;

    timer_element	*lev_timers;
    struct ls_t		*lev_lights;
    struct trap 	*lev_traps;
    struct engr		*lev_engr;
    struct region 	**regions;

    coord 		doors[DOORMAX];
    struct mkroom	rooms[(MAXNROFROOMS+1)*2];
    struct mkroom	*subrooms;
    struct mkroom	*upstairs_room, *dnstairs_room, *sstairs_room;
    stairway		upstair, dnstair;
    stairway		upladder, dnladder;
    stairway		sstairs;
    dest_area		updest;
    dest_area		dndest;

    struct wseg 	*wheads[MAX_NUM_WORMS], *wtails[MAX_NUM_WORMS];
    int			wgrowtime[MAX_NUM_WORMS];
    int			lastmoves; /* when the level was last visited */
    int 		nroom;
    int			nsubroom;
    int			doorindex;
    int			n_regions;
    int			max_regions;

    d_level		z;
};

extern struct level *levels[MAXLINFO]; /* structure describing all levels */
extern struct level *level;		/* pointer to an entry in levels */


#define OBJ_AT(x,y)	(level->objects[x][y] != NULL)
#define OBJ_AT_LEV(lev, x,y)	((lev)->objects[x][y] != NULL)

/*
 * Macros for encapsulation of level->monsters references.
 */
#define MON_AT(lev,x,y)	((lev)->monsters[x][y] != NULL && \
			 !(lev)->monsters[x][y]->mburied)
#define MON_BURIED_AT(x,y)	(level->monsters[x][y] != NULL && \
				level->monsters[x][y]->mburied)
#define place_worm_seg(m,x,y)	(m)->dlevel->monsters[x][y] = m
#define m_at(lev,x,y)		(MON_AT(lev,x,y) ? (lev)->monsters[x][y] : \
						NULL)
#define m_buried_at(x,y)	(MON_BURIED_AT(x,y) ? level->monsters[x][y] : \
						       NULL)

#endif /* RM_H */
