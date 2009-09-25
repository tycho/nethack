/*	SCCS Id: @(#)lev_main.c	3.4	2002/03/27	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the main function for the parser
 * and some useful functions needed by yacc
 */
#define SPEC_LEV	/* for MPW */
/* although, why don't we move those special defines here.. and in dgn_main? */

#include "hack.h"
#include "date.h"
#include "sp_lev.h"
#ifdef STRICT_REF_DEF
#include "tcap.h"
#endif

#ifdef MAC
# if defined(__SC__) || defined(__MRC__)
#  define MPWTOOL
#  define PREFIX ":dungeon:"	/* place output files here */
#  include <CursorCtl.h>
# else
#  if !defined(__MACH__)
#   define PREFIX ":lib:"	/* place output files here */
#  endif
# endif
#endif

#ifdef WIN_CE
#define PREFIX "\\nethack\\dat\\"
#endif

#ifndef MPWTOOL
# define SpinCursor(x)
#endif

#if defined(AMIGA) && defined(DLB)
# define PREFIX "NH:slib/"
#endif

#ifndef O_WRONLY
#include <fcntl.h>
#endif
#ifndef O_CREAT	/* some older BSD systems do not define O_CREAT in <fcntl.h> */
#include <sys/file.h>
#endif
#ifndef O_BINARY	/* used for micros, no-op for others */
# define O_BINARY 0
#endif

#if defined(MICRO) || defined(WIN32)
# define OMASK FCMASK
#else
# define OMASK 0644
#endif

#define ERR		(-1)

#define NewTab(type, size)	(type **) alloc(sizeof(type *) * size)
#define Free(ptr)		if(ptr) free((genericptr_t) (ptr))
#define Write(fd, item, size)	if (write(fd, (genericptr_t)(item), size) != size) return FALSE;

#if defined(__BORLANDC__) && !defined(_WIN32)
extern unsigned _stklen = STKSIZ;
#endif
#define MAX_ERRORS	25

extern int  NDECL (yyparse);
extern void FDECL (init_yyin, (FILE *));
extern void FDECL (init_yyout, (FILE *));

int  FDECL (main, (int, char **));
void FDECL (yyerror, (const char *));
void FDECL (yywarning, (const char *));
int  NDECL (yywrap);
int FDECL(get_floor_type, (CHAR_P));
int FDECL(get_room_type, (char *));
int FDECL(get_trap_type, (char *));
int FDECL(get_monster_id, (char *,CHAR_P));
int FDECL(get_object_id, (char *,CHAR_P));
boolean FDECL(check_monster_char, (CHAR_P));
boolean FDECL(check_object_char, (CHAR_P));
char FDECL(what_map_char, (CHAR_P));
void FDECL(scan_map, (char *, sp_lev *));
boolean FDECL(check_subrooms, (sp_lev *));
boolean FDECL(write_level_file, (char *,sp_lev *));

extern void NDECL(monst_init);
extern void NDECL(objects_init);
extern void NDECL(decl_init);

void FDECL(add_opcode, (sp_lev *, int, genericptr_t));

static boolean FDECL(write_common_data, (int,sp_lev *));
static boolean FDECL(write_maze, (int,sp_lev *));
static void NDECL(init_obj_classes);

static struct {
	const char *name;
	int type;
} trap_types[] = {
	{ "arrow",	ARROW_TRAP },
	{ "dart",	DART_TRAP },
	{ "falling rock", ROCKTRAP },
	{ "board",	SQKY_BOARD },
	{ "bear",	BEAR_TRAP },
	{ "land mine",	LANDMINE },
	{ "rolling boulder",	ROLLING_BOULDER_TRAP },
	{ "sleep gas",	SLP_GAS_TRAP },
	{ "rust",	RUST_TRAP },
	{ "fire",	FIRE_TRAP },
	{ "pit",	PIT },
	{ "spiked pit",	SPIKED_PIT },
	{ "hole",	HOLE },
	{ "trap door",	TRAPDOOR },
	{ "teleport",	TELEP_TRAP },
	{ "level teleport", LEVEL_TELEP },
	{ "magic portal",   MAGIC_PORTAL },
	{ "web",	WEB },
	{ "statue",	STATUE_TRAP },
	{ "magic",	MAGIC_TRAP },
	{ "anti magic",	ANTI_MAGIC },
	{ "polymorph",	POLY_TRAP },
	{ "spear", SPEAR_TRAP },
	{ 0, 0 }
};

static struct {
	const char *name;
	int type;
} room_types[] = {
	/* for historical reasons, room types are not contiguous numbers */
	/* (type 1 is skipped) */
	{ "ordinary",	 OROOM },
	{ "throne",	 COURT },
	{ "swamp",	 SWAMP },
	{ "vault",	 VAULT },
	{ "beehive",	 BEEHIVE },
	{ "morgue",	 MORGUE },
	{ "barracks",	 BARRACKS },
	{ "zoo",	 ZOO },
	{ "delphi",	 DELPHI },
	{ "temple",	 TEMPLE },
	{ "anthole",	 ANTHOLE },
	{ "cocknest",	 COCKNEST },
	{ "leprehall",	 LEPREHALL },
	{ "shop",	 SHOPBASE },
	{ "armor shop",	 ARMORSHOP },
	{ "scroll shop", SCROLLSHOP },
	{ "potion shop", POTIONSHOP },
	{ "weapon shop", WEAPONSHOP },
	{ "food shop",	 FOODSHOP },
	{ "ring shop",	 RINGSHOP },
	{ "wand shop",	 WANDSHOP },
	{ "tool shop",	 TOOLSHOP },
	{ "book shop",	 BOOKSHOP },
	{ "candle shop", CANDLESHOP },
	{ 0, 0 }
};

const char *fname = "(stdin)";
int fatal_error = 0;
int be_verbose = 0;

#ifdef FLEX23_BUG
/* Flex 2.3 bug work around; not needed for 2.3.6 or later */
int yy_more_len = 0;
#endif

extern unsigned int max_x_map, max_y_map;

extern int line_number, colon_line_number;

int
main(argc, argv)
int argc;
char **argv;
{
	FILE *fin;
	int i;
	boolean errors_encountered = FALSE;
#if defined(MAC) && (defined(THINK_C) || defined(__MWERKS__))
	static char *mac_argv[] = {	"lev_comp",	/* dummy argv[0] */
				":dat:Arch.des",
				":dat:Barb.des",
				":dat:Caveman.des",
				":dat:Healer.des",
				":dat:Knight.des",
				":dat:Monk.des",
				":dat:Priest.des",
				":dat:Ranger.des",
				":dat:Rogue.des",
				":dat:Samurai.des",
				":dat:Tourist.des",
				":dat:Valkyrie.des",
				":dat:Wizard.des",
				":dat:bigroom.des",
				":dat:castle.des",
				":dat:endgame.des",
				":dat:gehennom.des",
				":dat:knox.des",
				":dat:medusa.des",
				":dat:mines.des",
				":dat:oracle.des",
				":dat:sokoban.des",
				":dat:tower.des",
				":dat:yendor.des"
				};

	argc = SIZE(mac_argv);
	argv = mac_argv;
#endif
	/* Note:  these initializers don't do anything except guarantee that
		we're linked properly.
	*/
	monst_init();
	objects_init();
	decl_init();
	/* this one does something... */
	init_obj_classes();

	init_yyout(stdout);
	if (argc == 1) {		/* Read standard input */
	    init_yyin(stdin);
	    (void) yyparse();
	    if (fatal_error > 0) {
		    errors_encountered = TRUE;
	    }
	} else {			/* Otherwise every argument is a filename */
	    for(i=1; i<argc; i++) {
		    fname = argv[i];
		    if(!strcmp(fname, "-v")) {
			be_verbose++;
			continue;
		    }
		    fin = freopen(fname, "r", stdin);
		    if (!fin) {
			(void) fprintf(stderr,"Can't open \"%s\" for input.\n",
						fname);
			perror(fname);
			errors_encountered = TRUE;
		    } else {
			init_yyin(fin);
			(void) yyparse();
			line_number = 1;
			if (fatal_error > 0) {
				errors_encountered = TRUE;
				fatal_error = 0;
			}
		    }
	    }
	}
	exit(errors_encountered ? EXIT_FAILURE : EXIT_SUCCESS);
	/*NOTREACHED*/
	return 0;
}

/*
 * Each time the parser detects an error, it uses this function.
 * Here we take count of the errors. To continue farther than
 * MAX_ERRORS wouldn't be reasonable.
 * Assume that explicit calls from lev_comp.y have the 1st letter
 * capitalized, to allow printing of the line containing the start of
 * the current declaration, instead of the beginning of the next declaration.
 */
void
yyerror(s)
const char *s;
{
	(void) fprintf(stderr, "%s: line %d : %s\n", fname,
		(*s >= 'A' && *s <= 'Z') ? colon_line_number : line_number, s);
	if (++fatal_error > MAX_ERRORS) {
		(void) fprintf(stderr,"Too many errors, good bye!\n");
		exit(EXIT_FAILURE);
	}
}

/*
 * Just display a warning (that is : a non fatal error)
 */
void
yywarning(s)
const char *s;
{
	(void) fprintf(stderr, "%s: line %d : WARNING : %s\n",
				fname, colon_line_number, s);
}

/*
 * Stub needed for lex interface.
 */
int
yywrap()
{
	return 1;
}

struct opvar *
set_opvar_int(ov, val)
struct opvar *ov;
long  val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_INT;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_str(ov, val)
struct opvar *ov;
char *val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_STRING;
        ov->vardata.str = val;
    }
    return ov;
}


/*
 * Find the type of floor, knowing its char representation.
 */
int
get_floor_type(c)
char c;
{
	int val;

	SpinCursor(3);
	val = what_map_char(c);
	if(val == INVALID_TYPE) {
	    val = ERR;
	    yywarning("Invalid fill character in MAZE declaration");
	}
	return val;
}

/*
 * Find the type of a room in the table, knowing its name.
 */
int
get_room_type(s)
char *s;
{
	register int i;

	SpinCursor(3);
	for(i=0; room_types[i].name; i++)
	    if (!strcmp(s, room_types[i].name))
		return ((int) room_types[i].type);
	return ERR;
}

/*
 * Find the type of a trap in the table, knowing its name.
 */
int
get_trap_type(s)
char *s;
{
	register int i;

	SpinCursor(3);
	for (i=0; trap_types[i].name; i++)
	    if(!strcmp(s,trap_types[i].name))
		return trap_types[i].type;
	return ERR;
}

/*
 * Find the index of a monster in the table, knowing its name.
 */
int
get_monster_id(s, c)
char *s;
char c;
{
	register int i, class;

	SpinCursor(3);
	class = c ? def_char_to_monclass(c) : 0;
	if (class == MAXMCLASSES) return ERR;

	for (i = LOW_PM; i < NUMMONS; i++)
	    if (!class || class == mons[i].mlet)
		if (!strcmp(s, mons[i].mname)) return i;
	return ERR;
}

/*
 * Find the index of an object in the table, knowing its name.
 */
int
get_object_id(s, c)
char *s;
char c;		/* class */
{
	int i, class;
	const char *objname;

	SpinCursor(3);
	class = (c > 0) ? def_char_to_objclass(c) : 0;
	if (class == MAXOCLASSES) return ERR;

	for (i = class ? bases[class] : 0; i < NUM_OBJECTS; i++) {
	    if (class && objects[i].oc_class != class) break;
	    objname = obj_descr[i].oc_name;
	    if (objname && !strcmp(s, objname))
		return i;
	}
	return ERR;
}

static void
init_obj_classes()
{
	int i, class, prev_class;

	prev_class = -1;
	for (i = 0; i < NUM_OBJECTS; i++) {
	    class = objects[i].oc_class;
	    if (class != prev_class) {
		bases[class] = i;
		prev_class = class;
	    }
	}
}

/*
 * Is the character 'c' a valid monster class ?
 */
boolean
check_monster_char(c)
char c;
{
	return (def_char_to_monclass(c) != MAXMCLASSES);
}

/*
 * Is the character 'c' a valid object class ?
 */
boolean
check_object_char(c)
char c;
{
	return (def_char_to_objclass(c) != MAXOCLASSES);
}

/*
 * Convert .des map letter into floor type.
 */
char
what_map_char(c)
char c;
{
	SpinCursor(3);
	switch(c) {
		  case ' '  : return(STONE);
		  case '#'  : return(CORR);
		  case '.'  : return(ROOM);
		  case '-'  : return(HWALL);
		  case '|'  : return(VWALL);
		  case '+'  : return(DOOR);
		  case 'A'  : return(AIR);
		  case 'B'  : return(CROSSWALL); /* hack: boundary location */
		  case 'C'  : return(CLOUD);
		  case 'S'  : return(SDOOR);
		  case 'H'  : return(SCORR);
		  case '{'  : return(FOUNTAIN);
		  case '\\' : return(THRONE);
		  case 'K'  :
#ifdef SINKS
		      return(SINK);
#else
		      yywarning("Sinks are not allowed in this version!  Ignoring...");
		      return(ROOM);
#endif
		  case '}'  : return(MOAT);
		  case 'P'  : return(POOL);
		  case 'L'  : return(LAVAPOOL);
		  case 'I'  : return(ICE);
		  case 'W'  : return(WATER);
		  case 'T'	: return (TREE);
		  case 'F'	: return (IRONBARS);	/* Fe = iron */
		  case 'x'  : return(MAX_TYPE); /* 'see-through' */
	    }
	return(INVALID_TYPE);
}

void
add_opcode(sp, opc, dat)
sp_lev *sp;
int opc;
genericptr_t dat;
{
   long nop = sp->init_lev.n_opcodes;
   _opcode *tmp;

   if ((opc < 0) || (opc >= MAX_SP_OPCODES))
     yyerror("Unknown opcode");

   tmp = (_opcode *)alloc(sizeof(_opcode)*(nop+1));
   if (sp->opcodes && nop) {
       (void) memcpy(tmp, sp->opcodes, sizeof(_opcode)*nop);
       free(sp->opcodes);
   } else if (!tmp)
       yyerror("Couldn't alloc opcode space");

   sp->opcodes = tmp;

   sp->opcodes[nop].opcode = opc;
   sp->opcodes[nop].opdat = dat;

   sp->init_lev.n_opcodes++;
}

/*
 * Yep! LEX gives us the map in a raw mode.
 * Just analyze it here.
 */
void
scan_map(map, sp)
char *map;
sp_lev *sp;
{
#define New(type)               \
        (type *) memset((genericptr_t)alloc(sizeof(type)), 0, sizeof(type))
   struct opvar *xs = New(struct opvar);
   struct opvar *ys = New(struct opvar);
   struct opvar *mp = New(struct opvar);

	register int i, len;
	register char *s1, *s2;
	int max_len = 0;
	int max_hig = 0;
	char msg[256];
	char *tmpmap[ROWNO];
   int dx,dy;
   char *mbuf;

	/* First, strip out digits 0-9 (line numbering) */
	for (s1 = s2 = map; *s1; s1++)
	    if (*s1 < '0' || *s1 > '9')
		*s2++ = *s1;
	*s2 = '\0';

	/* Second, find the max width of the map */
	s1 = map;
	while (s1 && *s1) {
		s2 = index(s1, '\n');
		if (s2) {
			len = (int) (s2 - s1);
			s1 = s2 + 1;
		} else {
			len = (int) strlen(s1);
			s1 = (char *) 0;
		}
		if (len > max_len) max_len = len;
	}

	/* Then parse it now */
	while (map && *map) {
		tmpmap[max_hig] = (char *) alloc(max_len);
		s1 = index(map, '\n');
		if (s1) {
			len = (int) (s1 - map);
			s1++;
		} else {
			len = (int) strlen(map);
			s1 = map + len;
		}
		for(i=0; i<len; i++)
		  if((tmpmap[max_hig][i] = what_map_char(map[i])) == INVALID_TYPE) {
		      Sprintf(msg,
			 "Invalid character @ (%d, %d) - replacing with stone",
			      max_hig, i);
		      yywarning(msg);
		      tmpmap[max_hig][i] = STONE;
		    }
		while(i < max_len)
		    tmpmap[max_hig][i++] = STONE;
		map = s1;
		max_hig++;
	}

	/* Memorize boundaries */

	max_x_map = max_len - 1;
	max_y_map = max_hig - 1;


	if(max_len > MAP_X_LIM || max_hig > MAP_Y_LIM) {
	    Sprintf(msg, "Map too large! (max %d x %d)", MAP_X_LIM, MAP_Y_LIM);
	    yyerror(msg);
	}

   set_opvar_int(xs, max_len);
   set_opvar_int(ys, max_hig);

   mbuf = (char *) alloc(((max_hig-1) * max_len) + (max_len-1) + 2);
   for (dy = 0; dy < max_hig; dy++)
     for (dx = 0; dx < max_len; dx++)
       mbuf[(dy * max_len) + dx] = (tmpmap[dy][dx] + 1);

   mbuf[((max_hig-1) * max_len) + (max_len-1) + 1] = '\0';

   set_opvar_str(mp, mbuf);

   add_opcode(sp, SPO_PUSH, mp);
   add_opcode(sp, SPO_PUSH, ys);
   add_opcode(sp, SPO_PUSH, xs);

   add_opcode(sp, SPO_MAP, NULL);
}


/*
 * Output some info common to all special levels.
 */
static boolean
write_common_data(fd, lvl)
int fd;
sp_lev *lvl;
{
	static struct version_info version_data = {
			VERSION_NUMBER, VERSION_FEATURES,
			VERSION_SANITY1, VERSION_SANITY2
	};

	Write(fd, &version_data, sizeof version_data);
	Write(fd, &lvl->init_lev, sizeof(lev_init));
	return TRUE;
}


/*
 * Here we write the sp_lev structure in the specified file (fd).
 * Also, we have to free the memory allocated via alloc().
 */
static boolean
write_maze(fd, maze)
int fd;
sp_lev *maze;
{
        int i;
        uchar len;

        if (!write_common_data(fd, maze))
            return FALSE;

        for (i=0;i<maze->init_lev.n_opcodes;i++) {
	   _opcode tmpo = maze->opcodes[i];

	   Write(fd, &(tmpo.opcode), sizeof(tmpo.opcode));

	   if (tmpo.opcode < SPO_NULL || tmpo.opcode >= MAX_SP_OPCODES)
	       panic("write_maze: unknown opcode (%i).", tmpo.opcode);

	   if (tmpo.opcode == SPO_PUSH) {
	       genericptr_t opdat = tmpo.opdat;
	       if (opdat) {
		   struct opvar *ov = (struct opvar *)opdat;
		   int size;
		   Write(fd, &(ov->spovartyp), sizeof(ov->spovartyp));
		   switch (ov->spovartyp) {
		   case SPOVAR_NULL: break;
		   case SPOVAR_INT:
		       Write(fd, &(ov->vardata.l), sizeof(ov->vardata.l));
		       break;
		   case SPOVAR_STRING:
		       /*fprintf(stderr, "wrote push with str: %s\n", ov->vardata.str);*/
		       if (ov->vardata.str)
			   size = strlen(ov->vardata.str);
		       else size = 0;
		       Write(fd, &size, sizeof(size));
		       if (size) {
			   Write(fd, ov->vardata.str, size);
			   free(ov->vardata.str);
		       }
		       break;
		   default: panic("write_maze: unknown data type (%i).", ov->spovartyp);
		   }
	       } else panic("write_maze: PUSH with no data.");
	   } else {
	       /* sanity check */
	       genericptr_t opdat = tmpo.opdat;
	       if (opdat)
		   panic("write_maze: opcode (%i) has data.", tmpo.opcode);
	   }

	   Free(tmpo.opdat);

	}
        /* clear the struct for next user */
	Free(maze->opcodes);
        (void) memset((genericptr_t) &maze->init_lev, 0, sizeof maze->init_lev);

	return TRUE;
}

/*
 * Open and write special level file.
 * Return TRUE on success, FALSE on failure.
 */
boolean
write_level_file(filename, lvl)
char *filename;
sp_lev *lvl;
{
	int fout;
	char lbuf[60];

	lbuf[0] = '\0';
#ifdef PREFIX
	Strcat(lbuf, PREFIX);
#endif
	Strcat(lbuf, filename);
	Strcat(lbuf, LEV_EXT);

	fout = open(lbuf, O_WRONLY|O_CREAT|O_BINARY, OMASK);
	if (fout < 0) return FALSE;

        if (!lvl) panic("write_level_file");

	if (be_verbose)
	    fprintf(stdout, "File: '%s', opcodes: %li\n", lbuf, lvl->init_lev.n_opcodes);

        if (!write_maze(fout, lvl))
          return FALSE;

	(void) close(fout);

	return TRUE;
}

#ifdef STRICT_REF_DEF
/*
 * Any globals declared in hack.h and descendents which aren't defined
 * in the modules linked into lev_comp should be defined here.  These
 * definitions can be dummies:  their sizes shouldn't matter as long as
 * as their types are correct; actual values are irrelevant.
 */
#define ARBITRARY_SIZE 1
/* attrib.c */
struct attribs attrmax, attrmin;
/* files.c */
const char *configfile;
char lock[ARBITRARY_SIZE];
char SAVEF[ARBITRARY_SIZE];
# ifdef MICRO
char SAVEP[ARBITRARY_SIZE];
# endif
/* termcap.c */
struct tc_lcl_data tc_lcl_data;
# ifdef TEXTCOLOR
#  ifdef TOS
const char *hilites[CLR_MAX];
#  else
char NEARDATA *hilites[CLR_MAX];
#  endif
# endif
/* trap.c */
const char *traps[TRAPNUM];
/* window.c */
struct window_procs windowprocs;
/* xxxtty.c */
# ifdef DEFINE_OSPEED
short ospeed;
# endif
#endif	/* STRICT_REF_DEF */

/*lev_main.c*/
