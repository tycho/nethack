/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NitroHack may be freely redistributed.  See license for details. */

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

#ifndef O_WRONLY
#include <fcntl.h>
#endif
#ifndef O_CREAT	/* some older BSD systems do not define O_CREAT in <fcntl.h> */
#include <sys/file.h>
#endif
#ifndef O_BINARY	/* used for micros, no-op for others */
# define O_BINARY 0
#endif

#if defined(WIN32)
# define OMASK FCMASK
#else
# define OMASK 0644
#endif

#define ERR		(-1)

#define NewTab(type, size)	malloc(sizeof(type *) * size)
#define Free(ptr)		if (ptr) free((ptr))
#define Write(fd, item, size)	if (write(fd, (item), size) != size) return FALSE;

#define MAX_ERRORS	25

extern int  yyparse(void);
extern void init_yyin(FILE *);
extern void init_yyout(FILE *);

int  main(int, char **);
void yyerror(const char *);
void lc_error(const char *,...);
void yywarning(const char *);
void lc_warning(const char *,...);
int  yywrap(void);
struct opvar *set_opvar_int(struct opvar *,long);
struct opvar *set_opvar_coord(struct opvar *,long);
struct opvar *set_opvar_region(struct opvar *,long);
struct opvar *set_opvar_mapchar(struct opvar *,long);
struct opvar *set_opvar_monst(struct opvar *,long);
struct opvar *set_opvar_obj(struct opvar *,long);
struct opvar *set_opvar_str(struct opvar *,char *);
struct opvar *set_opvar_var(struct opvar *,char *);
void add_opvars(sp_lev *,const char *,...);
int get_floor_type(char);
int get_room_type(char *);
int get_trap_type(char *);
int get_monster_id(char *,char);
int get_object_id(char *,char);
boolean check_monster_char(char);
boolean check_object_char(char);
char what_map_char(char);
void scan_map(char *,sp_lev *);
boolean check_subrooms(sp_lev *);
boolean write_level_file(char *,sp_lev *);

struct lc_funcdefs *funcdef_new(long,char *);
void funcdef_free_all(struct lc_funcdefs *);
struct lc_funcdefs *funcdef_defined(struct lc_funcdefs *,const char *,int);

struct lc_vardefs *vardef_new(long,char *);
void vardef_free_all(struct lc_vardefs *);
struct lc_vardefs *vardef_defined(struct lc_vardefs *,char *, int);
void check_vardef_type(struct lc_vardefs *,char *,long);
struct lc_vardefs *add_vardef_type(struct lc_vardefs *,char *,long);

int reverse_jmp_opcode(int);

struct opvar *opvar_clone(struct opvar *);
void splev_add_from(sp_lev *,sp_lev *);

void add_opcode(sp_lev *,int,void *);

static boolean write_common_data(int,sp_lev *);
static boolean write_maze(int,sp_lev *);
static void init_obj_classes(void);

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
        { "vibrating square",   VIBRATING_SQUARE },
	{ "teleport",	TELEP_TRAP },
	{ "level teleport", LEVEL_TELEP },
	{ "magic portal",   MAGIC_PORTAL },
	{ "web",	WEB },
	{ "statue",	STATUE_TRAP },
	{ "magic",	MAGIC_TRAP },
	{ "anti magic",	ANTI_MAGIC },
	{ "polymorph",	POLY_TRAP },
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
	{ "lemurepit",	 LEMUREPIT },
	{ "anthole",	 ANTHOLE },
	{ "cocknest",	 COCKNEST },
	{ "garden",	 GARDEN },
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
	{ "tin shop",	 TINSHOP },
	{ "music shop",	 INSTRUMENTSHOP },
	{ "pet shop",	 PETSHOP },
	{ "candle shop", CANDLESHOP },
	{ "black market", BLACKSHOP },
	{ 0, 0 }
};

const char *fname = "(stdin)";
static char *outprefix = "";
int fatal_error = 0;
int got_errors = 0;
int be_verbose = 0;
int decompile = 0;

extern unsigned int max_x_map, max_y_map;

extern int line_number, colon_line_number;

struct lc_vardefs *variable_definitions = NULL;

int main(int argc, char **argv)
{
	FILE *fin;
	int i;
	boolean errors_encountered = FALSE;

	init_objlist();
	init_obj_classes();

	init_yyout(stdout);
	if (argc == 1) {		/* Read standard input */
	    init_yyin(stdin);
	    yyparse();
	    if (fatal_error > 0 || got_errors > 0) {
		    errors_encountered = TRUE;
	    }
	} else {
	    /* first two args may be "-o outprefix" */
	    i = 1;
	    if (!strcmp(argv[1], "-o") && argc > 3) {
		outprefix = argv[2];
		i = 3;
	    }
	    /* Otherwise every argument is a filename */
	    for (; i<argc; i++) {
		    fname = argv[i];
		    if (!strcmp(fname, "-v")) {
			be_verbose++;
			continue;
		    }
		    if (!strcmp(fname, "-d")) {
			decompile = 1;
			continue;
		    }

		    fin = freopen(fname, "r", stdin);
		    if (!fin) {
			fprintf(stderr,"Can't open \"%s\" for input.\n",
						fname);
			perror(fname);
			errors_encountered = TRUE;
		    } else {
			init_yyin(fin);
			yyparse();
			line_number = 1;
			if (fatal_error > 0 || got_errors > 0) {
				errors_encountered = TRUE;
				fatal_error = 0;
			}
		    }
		    fclose(fin);
	    }
	}
	
	free(objects);
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
void yyerror(const char *s)
{
	fprintf(stderr, "%s: line %d : %s\n", fname,
		(*s >= 'A' && *s <= 'Z') ? colon_line_number : line_number, s);
	if (++fatal_error > MAX_ERRORS) {
		fprintf(stderr,"Too many errors, good bye!\n");
		exit(EXIT_FAILURE);
	}
}

void lc_error(const char *fmt, ...)
{
	char buf[512];
	va_list argp;

	va_start(argp, fmt);
	vsnprintf(buf, 511, fmt, argp);
	va_end(argp);

	yyerror(buf);
}

/*
 * Just display a warning (that is : a non fatal error)
 */
void yywarning(const char *s)
{
	fprintf(stderr, "%s: line %d : WARNING : %s\n",
				fname, colon_line_number, s);
}

void lc_warning(const char *fmt, ...)
{
	char buf[512];
	va_list argp;

	va_start(argp, fmt);
	vsnprintf(buf, 511, fmt, argp);
	va_end(argp);

	yywarning(buf);
}

struct opvar *set_opvar_int(struct opvar *ov, long val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_INT;
	    ov->vardata.l = val;
	}
	return ov;
}

struct opvar *set_opvar_coord(struct opvar *ov, long val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_COORD;
	    ov->vardata.l = val;
	}
	return ov;
}

struct opvar *set_opvar_region(struct opvar *ov, long val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_REGION;
	    ov->vardata.l = val;
	}
	return ov;
}

struct opvar *set_opvar_mapchar(struct opvar *ov, long val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_MAPCHAR;
	    ov->vardata.l = val;
	}
	return ov;
}

struct opvar *set_opvar_monst(struct opvar *ov, long val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_MONST;
	    ov->vardata.l = val;
	}
	return ov;
}

struct opvar *set_opvar_obj(struct opvar *ov, long val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_OBJ;
	    ov->vardata.l = val;
	}
	return ov;
}

struct opvar *set_opvar_str(struct opvar *ov, char *val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_STRING;
	    ov->vardata.str = val ? strdup(val) : NULL;
	}
	return ov;
}

struct opvar *set_opvar_var(struct opvar *ov, char *val)
{
	if (ov) {
	    ov->spovartyp = SPOVAR_VARIABLE;
	    ov->vardata.str = val ? strdup(val) : NULL;
	}
	return ov;
}

#define New(type)	memset(malloc(sizeof(type)), 0, sizeof(type))

void add_opvars(sp_lev *sp, const char *fmt, ...)
{
	const char *p;
	va_list argp;

	va_start(argp, fmt);

	for (p = fmt; *p != '\0'; p++) {
	    switch(*p) {
	    case ' ': break;
	    case 'i':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_int(ov, va_arg(argp, long));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 'c':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_coord(ov, va_arg(argp, long));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 'r':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_region(ov, va_arg(argp, long));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 'm':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_mapchar(ov, va_arg(argp, long));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 'M':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_monst(ov, va_arg(argp, long));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 'O':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_obj(ov, va_arg(argp, long));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 's':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_str(ov, va_arg(argp, char *));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 'v':
		{
		    struct opvar *ov = New(struct opvar);
		    set_opvar_var(ov, va_arg(argp, char *));
		    add_opcode(sp, SPO_PUSH, ov);
		    break;
		}
	    case 'o':
		{
		    long i = va_arg(argp, int);
		    if (i < 0 || i >= MAX_SP_OPCODES)
			fprintf(stderr, "add_opvars: unknown opcode '%li'.\n", i);
		    add_opcode(sp, i, NULL);
		    break;
		}
	    default:
		fprintf(stderr, "add_opvars: illegal format character '%c'.\n", *p);
		break;
	    }
	}

	va_end(argp);
}

struct lc_funcdefs *funcdef_new(long addr, char *name)
{
	struct lc_funcdefs *f = New(struct lc_funcdefs);
	if (!f) {
	    lc_error("Could not alloc function definition for '%s'.", name);
	    return NULL;
	}
	f->next = NULL;
	f->addr = addr;
	f->name = strdup(name);
	f->n_called = 0;
	f->code.opcodes = NULL;
	f->code.n_opcodes = 0;
	return f;
}

void funcdef_free_all(struct lc_funcdefs *fchain)
{
	struct lc_funcdefs *tmp = fchain;
	struct lc_funcdefs *nxt;
	while (tmp) {
	    nxt = tmp->next;
	    Free(tmp->name);
	    Free(tmp->code.opcodes);
	    Free(tmp);
	    tmp = nxt;
	}
}

struct lc_funcdefs *funcdef_defined(struct lc_funcdefs *f, const char *name,
				    int casesense)
{
	while (f) {
	    if (casesense) {
		if (!strcmp(name, f->name)) return f;
	    } else {
		if (!strcasecmp(name, f->name)) return f;
	    }
	    f = f->next;
	}
	return NULL;
}

struct lc_vardefs *vardef_new(long typ, char *name)
{
	struct lc_vardefs *f = New(struct lc_vardefs);
	if (!f) {
	    lc_error("Could not alloc variable definition for '%s'.", name);
	    return NULL;
	}
	f->next = NULL;
	f->var_type = typ;
	f->name = strdup(name);
	return f;
}

void vardef_free_all(struct lc_vardefs *fchain)
{
	struct lc_vardefs *tmp = fchain;
	struct lc_vardefs *nxt;
	while (tmp) {
	    nxt = tmp->next;
	    Free(tmp->name);
	    Free(tmp);
	    tmp = nxt;
	}
}

struct lc_vardefs *vardef_defined(struct lc_vardefs *f, char *name, int casesense)
{
	while (f) {
	    if (casesense) {
		if (!strcmp(name, f->name))
		    return f;
	    } else {
		if (!strcasecmp(name, f->name))
		    return f;
	    }
	    f = f->next;
	}
	return NULL;
}

static const char *spovar2str(long spovar)
{
	static int togl = 0;
	static char buf[2][128];
	char *n = NULL;
	int is_array = (spovar & SPOVAR_ARRAY);
	spovar &= ~SPOVAR_ARRAY;

	switch (spovar) {
	default:		lc_error("spovar2str(%li)", spovar); break;
	case SPOVAR_INT:	n = "integer"; break;
	case SPOVAR_STRING:	n = "string"; break;
	case SPOVAR_VARIABLE:	n = "variable"; break;
	case SPOVAR_COORD:	n = "coordinate"; break;
	case SPOVAR_REGION:	n = "region"; break;
	case SPOVAR_MAPCHAR:	n = "mapchar"; break;
	case SPOVAR_MONST:	n = "monster"; break;
	case SPOVAR_OBJ:	n = "object"; break;
	}

	togl = (togl + 1) % 2;

	snprintf(buf[togl], 127, "%s%s", n, (is_array ? " array" : ""));
	return buf[togl];
}

void check_vardef_type(struct lc_vardefs *vd, char *varname, long vartype)
{
	struct lc_vardefs *tmp;
	tmp = vardef_defined(vd, varname, 1);
	if (tmp) {
	    if (tmp->var_type != vartype) {
		lc_error("Trying to use variable '%s' as %s, when it is %s.",
			 varname, spovar2str(vartype), spovar2str(tmp->var_type));
	    }
	} else {
	    lc_error("Variable '%s' not defined.", varname);
	}
}

struct lc_vardefs *add_vardef_type(struct lc_vardefs *vd, char *varname,
				   long vartype)
{
	struct lc_vardefs *tmp;
	tmp = vardef_defined(vd, varname, 1);
	if (tmp) {
	    if (tmp->var_type != vartype)
		lc_error("Trying to redefine variable '%s' as %s, when it is %s.",
			 varname, spovar2str(vartype), spovar2str(tmp->var_type));
	} else {
	    tmp = vardef_new(vartype, varname);
	    tmp->next = vd;
	    return tmp;
	}
	return vd;
}

int reverse_jmp_opcode(int opcode)
{
	switch (opcode) {
	case SPO_JE:	return SPO_JNE;
	case SPO_JNE:	return SPO_JE;
	case SPO_JL:	return SPO_JGE;
	case SPO_JG:	return SPO_JLE;
	case SPO_JLE:	return SPO_JG;
	case SPO_JGE:	return SPO_JL;
	default:
	    lc_error("Cannot reverse comparison jmp opcode %i.", opcode);
	    return SPO_NULL;
	}
}

/*
 * Basically copied from src/sp_lev.c.
 */
struct opvar *opvar_clone(struct opvar *ov)
{
	if (ov) {
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
		{
		    lc_error("Unknown opvar_clone value type (%i)!", ov->spovartyp);
		}
	    }
	    return tmpov;
	}

	return NULL;
}


void splev_add_from(sp_lev *splev, sp_lev *from_splev)
{
	int i;

	if (splev && from_splev) {
	    for (i = 0; i < from_splev->n_opcodes; i++) {
		add_opcode(splev, from_splev->opcodes[i].opcode,
			   opvar_clone(from_splev->opcodes[i].opdat));
	    }
	}
}

/*
 * Find the type of floor, knowing its char representation.
 */
int get_floor_type(char c)
{
	int val;

	val = what_map_char(c);
	if (val == INVALID_TYPE) {
	    val = ERR;
	    lc_warning("Invalid fill character '%c' in MAZE declaration", c);
	}
	return val;
}

/*
 * Find the type of a room in the table, knowing its name.
 */
int get_room_type(char *s)
{
	int i;

	for (i=0; room_types[i].name; i++)
	    if (!strcmp(s, room_types[i].name))
		return (int) room_types[i].type;
	return ERR;
}

/*
 * Find the type of a trap in the table, knowing its name.
 */
int get_trap_type(char *s)
{
	int i;

	for (i=0; trap_types[i].name; i++)
	    if (!strcmp(s,trap_types[i].name))
		return trap_types[i].type;
	return ERR;
}

/*
 * Find the index of a monster in the table, knowing its name.
 */
int get_monster_id(char *s, char c)
{
	int i, class;

	class = c ? def_char_to_monclass(c) : 0;
	if (class == MAXMCLASSES) return ERR;

	for (i = LOW_PM; i < NUMMONS; i++)
	    if (!class || class == mons[i].mlet)
		if (!strcmp(s, mons[i].mname)) return i;
	/* didn't find it; lets try case insensitive search */
	for (i = LOW_PM; i < NUMMONS; i++) {
	    if (!class || class == mons[i].mlet) {
		if (!strcasecmp(s, mons[i].mname)) {
		    if (be_verbose)
			lc_warning("Monster type \"%s\" matches \"%s\".",
				   s, mons[i].mname);
		    return i;
		}
	    }
	}
	return ERR;
}

/*
 * Find the index of an object in the table, knowing its name.
 */
int get_object_id(char *s, char c)
{
	int i, class;
	const char *objname;

	class = (c > 0) ? def_char_to_objclass(c) : 0;
	if (class == MAXOCLASSES) return ERR;

	for (i = class ? bases[class] : 0; i < NUM_OBJECTS; i++) {
	    if (class && objects[i].oc_class != class) break;
	    objname = obj_descr[i].oc_name;
	    if (objname && !strcmp(s, objname))
		return i;
	}

	for (i = class ? bases[class] : 0; i < NUM_OBJECTS; i++) {
	    if (class && objects[i].oc_class != class) break;
	    objname = obj_descr[i].oc_name;
	    if (objname && !strcasecmp(s, objname)) {
		if (be_verbose)
		    lc_warning("Object type \"%s\" matches \"%s\".", s, objname);
		return i;
	    }
	}

	return ERR;
}

static void init_obj_classes(void)
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
boolean check_monster_char(char c)
{
	return def_char_to_monclass(c) != MAXMCLASSES;
}

/*
 * Is the character 'c' a valid object class ?
 */
boolean check_object_char(char c)
{
	return def_char_to_objclass(c) != MAXOCLASSES;
}

/*
 * Convert .des map letter into floor type.
 */
char what_map_char(char c)
{
	switch(c) {
		  case ' '  : return STONE;
		  case '#'  : return CORR;
		  case '.'  : return ROOM;
		  case '-'  : return HWALL;
		  case '|'  : return VWALL;
		  case '+'  : return DOOR;
		  case 'A'  : return AIR;
		  case 'B'  : return CROSSWALL; /* hack: boundary location */
		  case 'C'  : return CLOUD;
		  case 'S'  : return SDOOR;
		  case 'H'  : return SCORR;
		  case '{'  : return FOUNTAIN;
		  case '\\' : return THRONE;
		  case 'K'  : return SINK;
		  case '}'  : return MOAT;
		  case 'P'  : return POOL;
		  case 'L'  : return LAVAPOOL;
		  case 'I'  : return ICE;
		  case 'W'  : return WATER;
		  case 'T'  : return TREE;
		  case 't'  : return DEADTREE;
		  case 'F'  : return IRONBARS; /* Fe = iron */
		  case 'M'  : return BOG;
		  case 'x'  : return MAX_TYPE; /* 'see-through' */
	    }
	return INVALID_TYPE;
}

void add_opcode(sp_lev *sp, int opc, void *dat)
{
	long nop = sp->n_opcodes;
	_opcode *tmp;

	if (opc < 0 || opc >= MAX_SP_OPCODES)
	    lc_error("Unknown opcode '%i'", opc);

	tmp = realloc(sp->opcodes, sizeof(_opcode) * (nop + 1));
	if (!tmp) {
	    lc_error("Could not alloc opcode space");
	    /* sp->opcodes would be freed here, if this were ever reached */
	}

	sp->opcodes = tmp;

	sp->opcodes[nop].opcode = opc;
	sp->opcodes[nop].opdat = dat;

	sp->n_opcodes++;
}

/*
 * Yep! LEX gives us the map in a raw mode.
 * Just analyze it here.
 */
void scan_map(char *map, sp_lev *sp)
{
	int i, len;
	char *s1, *s2;
	long max_len = 0;
	long max_hig = 0;
	char *tmpmap[ROWNO];
	int dx, dy;
	char *mbuf;

	/* First, strip out digits 0-9 (line numbering) */
	for (s1 = s2 = map; *s1; s1++)
	    if (*s1 < '0' || *s1 > '9')
		*s2++ = *s1;
	*s2 = '\0';

	/* Second, find the max width of the map */
	s1 = map;
	while (s1 && *s1) {
		s2 = strchr(s1, '\n');
		if (s2) {
			len = (int) (s2 - s1);
			s1 = s2 + 1;
		} else {
			len = (int) strlen(s1);
			s1 = NULL;
		}
		if (len > max_len) max_len = len;
	}

	/* Then parse it now */
	while (map && *map) {
		tmpmap[max_hig] = malloc(max_len);
		s1 = strchr(map, '\n');
		if (s1) {
			len = (int) (s1 - map);
			s1++;
		} else {
			len = (int) strlen(map);
			s1 = map + len;
		}
		for (i=0; i<len; i++)
		  if ((tmpmap[max_hig][i] = what_map_char(map[i])) == INVALID_TYPE) {
		      lc_warning("Invalid character '%c' @ (%ld, %d) "
				 "- replacing with stone", map[i], max_hig, i);
		      tmpmap[max_hig][i] = STONE;
		    }
		while (i < max_len)
		    tmpmap[max_hig][i++] = STONE;
		map = s1;
		max_hig++;
	}

	/* Memorize boundaries */
	max_x_map = max_len - 1;
	max_y_map = max_hig - 1;

	if (max_len > MAP_X_LIM || max_hig > MAP_Y_LIM) {
	    lc_error("Map too large at (%d x %d), max is (%d x %d)",
		     max_len, max_hig, MAP_X_LIM, MAP_Y_LIM);
	}

	mbuf = malloc((max_hig - 1) * max_len + (max_len - 1) + 2);
	for (dy = 0; dy < max_hig; dy++)
	    for (dx = 0; dx < max_len; dx++)
		mbuf[dy * max_len + dx] = tmpmap[dy][dx] + 1;
	mbuf[(max_hig - 1) * max_len + (max_len - 1) + 1] = '\0';

	add_opvars(sp, "siio", mbuf, max_hig, max_len, SPO_MAP);

	for (dy = 0; dy < max_hig; dy++)
	    Free(tmpmap[dy]);
	Free(mbuf);
}

/*
 * Output some info common to all special levels.
 */
static boolean write_common_data(int fd, sp_lev *lvl)
{
	static struct version_info version_data = {
			VERSION_NUMBER, VERSION_FEATURES,
			VERSION_SANITY1
	};

	Write(fd, &version_data, sizeof version_data);
	return TRUE;
}

/*
 * Here we write the sp_lev structure in the specified file (fd).
 * Also, we have to free the memory allocated via malloc().
 */
static boolean write_maze(int fd, sp_lev *maze)
{
	long i;

	if (!write_common_data(fd, maze))
	    return FALSE;

	Write(fd, &maze->n_opcodes, sizeof(maze->n_opcodes));

	for (i = 0; i < maze->n_opcodes; i++) {
	    _opcode tmpo = maze->opcodes[i];
	    Write(fd, &(tmpo.opcode), sizeof(tmpo.opcode));

	    if (tmpo.opcode < SPO_NULL || tmpo.opcode >= MAX_SP_OPCODES)
		panic("write_maze: unknown opcode (%i).", tmpo.opcode);

	    if (tmpo.opcode == SPO_PUSH) {
		void *opdat = tmpo.opdat;
		if (opdat) {
		    struct opvar *ov = opdat;
		    int size;
		    Write(fd, &(ov->spovartyp), sizeof(ov->spovartyp));
		    switch (ov->spovartyp) {
		    case SPOVAR_NULL: break;
		    case SPOVAR_COORD:
		    case SPOVAR_REGION:
		    case SPOVAR_MAPCHAR:
		    case SPOVAR_MONST:
		    case SPOVAR_OBJ:
		    case SPOVAR_INT:
			Write(fd, &(ov->vardata.l), sizeof(ov->vardata.l));
			break;
		    case SPOVAR_VARIABLE:
		    case SPOVAR_STRING:
			if (ov->vardata.str)
			    size = strlen(ov->vardata.str);
			else
			    size = 0;
			Write(fd, &size, sizeof(size));
			if (size) {
			    Write(fd, ov->vardata.str, size);
			    Free(ov->vardata.str);
			}
			break;
		    default:
			panic("write_maze: unknown data type (%i).", ov->spovartyp);
		    }
		} else panic("write_maze: PUSH with no data.");
	    } else {
		/* sanity check */
		void *opdat = tmpo.opdat;
		if (opdat)
		    panic("write_maze: opcode (%i) has data.", tmpo.opcode);
	    }
	    Free(tmpo.opdat);
	}
	/* clear the struct for the next user */
	Free(maze->opcodes);
	maze->opcodes = NULL;

	return TRUE;
}

static boolean decompile_maze(int fd, const sp_lev *maze)
{
	long i;
	char debuf[128];
	const char *opcodestr[MAX_SP_OPCODES] = {
	    "null",
	    "message",
	    "monster",
	    "object",
	    "engraving",
	    "room",
	    "subroom",
	    "door",
	    "stair",
	    "ladder",
	    "altar",
	    "fountain",
	    "sink",
	    "pool",
	    "trap",
	    "gold",
	    "corridor",
	    "levregion",
	    "drawbridge",
	    "mazewalk",
	    "non_diggable",
	    "non_passwall",
	    "wallify",
	    "map",
	    "room_door",
	    "region",
	    "cmp",
	    "jmp",
	    "jl",
	    "jle",
	    "jg",
	    "jge",
	    "je",
	    "jne",
	    "spill",
	    "terrain",
	    "replaceterrain",
	    "exit",
	    "endroom",
	    "pop_container",
	    "push",
	    "pop",
	    "rn2",
	    "dec",
	    "inc",
	    "add",
	    "sub",
	    "mul",
	    "div",
	    "mod",
	    "copy",
	    "mon_generation",
	    "end_moninvent",
	    "grave",
	    "frame_push",
	    "frame_pop",
	    "call",
	    "return",
	    "init_map",
	    "flags",
	    "sounds",
	    "wallwalk",
	    "var_init",
	    "shuffle_array",
	    "dice",
	    "selection_add",
	    "selection_point",
	    "selection_rect",
	    "selection_fillrect",
	    "selection_line",
	    "selection_rndline",
	    "selection_grow",
	    "selection_flood",
	    "selection_rndcoord",
	    "selection_ellipse",
	    "selection_filter",
	};

	/* don't bother with the header stuff */

	for (i = 0; i < maze->n_opcodes; i++) {
	    const _opcode tmpo = maze->opcodes[i];

	    if (tmpo.opcode < SPO_NULL || tmpo.opcode >= MAX_SP_OPCODES)
		panic("decompile_maze: unknown opcode (%i).", tmpo.opcode);

	    if (tmpo.opcode == SPO_PUSH) {
		const void *opdat = tmpo.opdat;
		if (opdat) {
		    const struct opvar *ov = opdat;
		    int size;
		    switch (ov->spovartyp) {
		    case SPOVAR_NULL: break;
		    case SPOVAR_COORD:
			snprintf(debuf, 127, "%li:\t%s\tcoord:(%li,%li)\n",
				 i, opcodestr[tmpo.opcode],
				 SP_COORD_X(ov->vardata.l),
				 SP_COORD_Y(ov->vardata.l));
			Write(fd, debuf, strlen(debuf));
			break;
		    case SPOVAR_REGION:
			snprintf(debuf, 127, "%li:\t%s\tregion:(%li,%li,%li,%li)\n",
				 i, opcodestr[tmpo.opcode],
				 SP_REGION_X1(ov->vardata.l),
				 SP_REGION_Y1(ov->vardata.l),
				 SP_REGION_X2(ov->vardata.l),
				 SP_REGION_Y2(ov->vardata.l));
			Write(fd, debuf, strlen(debuf));
			break;
		    case SPOVAR_OBJ:
			snprintf(debuf, 127, "%li:\t%s\tobj:(id=%li,class=\'%c\')\n",
				 i, opcodestr[tmpo.opcode],
				 SP_OBJ_TYP(ov->vardata.l),
				 (char)SP_OBJ_CLASS(ov->vardata.l));
			Write(fd, debuf, strlen(debuf));
			break;
		    case SPOVAR_MONST:
			snprintf(debuf, 127, "%li:\t%s\tmonst:(pm=%li,class='%c')\n",
				 i, opcodestr[tmpo.opcode],
				 SP_MONST_PM(ov->vardata.l),
				 (char)SP_MONST_CLASS(ov->vardata.l));
			Write(fd, debuf, strlen(debuf));
			break;
		    case SPOVAR_MAPCHAR:
			snprintf(debuf, 127, "%li:\t%s\tmapchar:(%i,%i)\n",
				 i, opcodestr[tmpo.opcode],
				 (int)SP_MAPCHAR_TYP(ov->vardata.l),
				 (schar)SP_MAPCHAR_LIT(ov->vardata.l));
			Write(fd, debuf, strlen(debuf));
			break;
		    case SPOVAR_INT:
			if (ov->vardata.l >= ' ' && ov->vardata.l <= '~') {
			    snprintf(debuf, 127, "%li:\t%s\tint:%li\t# '%c'\n",
				     i, opcodestr[tmpo.opcode], ov->vardata.l,
				     (char)ov->vardata.l);
			} else {
			    snprintf(debuf, 127, "%li:\t%s\tint:%li\n",
				     i, opcodestr[tmpo.opcode], ov->vardata.l);
			}
			Write(fd, debuf, strlen(debuf));
			break;
		    case SPOVAR_VARIABLE:
		    case SPOVAR_STRING:
			if (ov->vardata.str)
			    size = strlen(ov->vardata.str);
			else
			    size = 0;
			if (size) {
			    int x;
			    int ok = (size > 127) ? 0 : 1;
			    if (ok) {
				for (x = 0; x < size; x++) {
				    if (ov->vardata.str[x] < ' ' ||
					ov->vardata.str[x] > '~') {
					ok = 0;
					break;
				    }
				}
			    }
			    if (ok) {
				if (ov->spovartyp == SPOVAR_VARIABLE) {
				    snprintf(debuf, 127, "%li:\t%s\tvar:$%s\n",
					     i, opcodestr[tmpo.opcode],
					     ov->vardata.str);
				} else {
				    snprintf(debuf, 127, "%li:\t%s\tstr:\"%s\"\n",
					     i, opcodestr[tmpo.opcode],
					     ov->vardata.str);
				}
				Write(fd, debuf, strlen(debuf));
			    } else {
				snprintf(debuf, 127, "%li:\t%s\tstr:",
					 i, opcodestr[tmpo.opcode]);
				Write(fd, debuf, strlen(debuf));
				for (x = 0; x < size; x++) {
				    snprintf(debuf, 127, "%02x ", ov->vardata.str[x]);
				    Write(fd, debuf, strlen(debuf));
				}
				snprintf(debuf, 127, "\n");
				Write(fd, debuf, strlen(debuf));
			    }
			}
			break;
		    default:
			panic("decompile_maze: unknown data type (%i).",
			      ov->spovartyp);
		    }
		} else panic("decompile_maze: PUSH with no data.");
	    } else {
		/* sanity check */
		const void *opdat = tmpo.opdat;
		if (opdat)
		    panic("decompile_maze: opcode (%i) has data.", tmpo.opcode);
		snprintf(debuf, 127, "%li:\t%s\n", i, opcodestr[tmpo.opcode]);
		Write(fd, debuf, strlen(debuf));
	    }
	}
	return TRUE;
}

/*
 * Open and write special level file.
 * Return TRUE on success, FALSE on failure.
 */
boolean write_level_file(char *filename, sp_lev *lvl)
{
	int fout;
	char lbuf[60];

	if (decompile) {
	    lbuf[0] = '\0';
	    strcat(lbuf, outprefix);
	    strcat(lbuf, filename);
	    strcat(lbuf, "_lev.txt");
	    fout = open(lbuf, O_TRUNC|O_WRONLY|O_CREAT, OMASK);
	    if (fout < 0) return FALSE;
	    decompile_maze(fout, lvl);
	    close(fout);
	}

	lbuf[0] = '\0';
	strcat(lbuf, outprefix);
	strcat(lbuf, filename);
	strcat(lbuf, LEV_EXT);

	fout = open(lbuf, O_WRONLY|O_CREAT|O_BINARY, OMASK);
	if (fout < 0) return FALSE;

	if (!lvl) panic("write_level_file");

	if (be_verbose)
	    fprintf(stdout, "File: '%s', opcodes: %li\n", lbuf, lvl->n_opcodes);

	if (!write_maze(fout, lvl))
	    return FALSE;

	close(fout);

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

/* termcap.c */
struct tc_lcl_data tc_lcl_data;
char *hilites[CLR_MAX];
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
