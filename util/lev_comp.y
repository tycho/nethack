%{
/*	SCCS Id: @(#)lev_yacc.c	3.4	2000/01/17	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the Level Compiler code
 * It may handle special mazes & special room-levels
 */

/* In case we're using bison in AIX.  This definition must be
 * placed before any other C-language construct in the file
 * excluding comments and preprocessor directives (thanks IBM
 * for this wonderful feature...).
 *
 * Note: some cpps barf on this 'undefined control' (#pragma).
 * Addition of the leading space seems to prevent barfage for now,
 * and AIX will still see the directive.
 */
#ifdef _AIX
 #pragma alloca		/* keep leading space! */
#endif

#include "hack.h"
#include "sp_lev.h"

#define ERR		(-1)
/* many types of things are put in chars for transference to NetHack.
 * since some systems will use signed chars, limit everybody to the
 * same number for portability.
 */
#define MAX_OF_TYPE	128

#define MAX_NESTED_IFS	20

#define New(type)		\
	(type *) memset((genericptr_t)alloc(sizeof(type)), 0, sizeof(type))
#define NewTab(type, size)	(type **) alloc(sizeof(type *) * size)
#define Free(ptr)		free((genericptr_t)ptr)

extern void FDECL(yyerror, (const char *));
extern void FDECL(yywarning, (const char *));
extern int NDECL(yylex);
int NDECL(yyparse);

extern int FDECL(get_floor_type, (CHAR_P));
extern int FDECL(get_room_type, (char *));
extern int FDECL(get_trap_type, (char *));
extern int FDECL(get_monster_id, (char *,CHAR_P));
extern int FDECL(get_object_id, (char *,CHAR_P));
extern boolean FDECL(check_monster_char, (CHAR_P));
extern boolean FDECL(check_object_char, (CHAR_P));
extern char FDECL(what_map_char, (CHAR_P));
extern void FDECL(scan_map, (char *, sp_lev *));
extern void FDECL(add_opcode, (sp_lev *, int, genericptr_t));
extern genericptr_t FDECL(get_last_opcode_data1, (sp_lev *, int));
extern genericptr_t FDECL(get_last_opcode_data2, (sp_lev *, int,int));
extern boolean FDECL(check_subrooms, (sp_lev *));
extern boolean FDECL(write_level_file, (char *,sp_lev *));
extern struct opvar *FDECL(set_opvar_int, (struct opvar *, long));
extern struct opvar *FDECL(set_opvar_str, (struct opvar *, char *));

static struct reg {
	int x1, y1;
	int x2, y2;
}		current_region;

static struct coord {
	int x;
	int y;
}		current_coord, current_align;

static struct size {
	int height;
	int width;
}		current_size;

sp_lev splev;

static char olist[MAX_REGISTERS], mlist[MAX_REGISTERS];
static struct coord plist[MAX_REGISTERS];
static struct opvar *if_list[MAX_NESTED_IFS];

static short n_olist = 0, n_mlist = 0, n_plist = 0, n_if_list = 0;
static short on_olist = 0, on_mlist = 0, on_plist = 0;

static long lev_flags;

unsigned int max_x_map, max_y_map;
int obj_containment = 0;

extern int fatal_error;
extern const char *fname;

%}

%union
{
	int	i;
	char*	map;
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} corpos;
}


%token	<i> CHAR INTEGER BOOLEAN PERCENT SPERCENT
%token	<i> MAZE_GRID_ID SOLID_FILL_ID MINES_ID
%token	<i> MESSAGE_ID LEVEL_ID LEV_INIT_ID GEOMETRY_ID NOMAP_ID
%token	<i> OBJECT_ID COBJECT_ID MONSTER_ID TRAP_ID DOOR_ID DRAWBRIDGE_ID
%token	<i> MAZEWALK_ID WALLIFY_ID REGION_ID FILLING
%token	<i> RANDOM_OBJECTS_ID RANDOM_MONSTERS_ID RANDOM_PLACES_ID
%token	<i> ALTAR_ID LADDER_ID STAIR_ID NON_DIGGABLE_ID NON_PASSWALL_ID ROOM_ID
%token	<i> PORTAL_ID TELEPRT_ID BRANCH_ID LEV CHANCE_ID RANDLINE_ID
%token	<i> CORRIDOR_ID GOLD_ID ENGRAVING_ID FOUNTAIN_ID POOL_ID SINK_ID NONE
%token	<i> RAND_CORRIDOR_ID DOOR_STATE LIGHT_STATE CURSE_TYPE ENGRAVING_TYPE
%token	<i> DIRECTION RANDOM_TYPE O_REGISTER M_REGISTER P_REGISTER A_REGISTER
%token	<i> ALIGNMENT LEFT_OR_RIGHT CENTER TOP_OR_BOT ALTAR_TYPE UP_OR_DOWN
%token	<i> SUBROOM_ID NAME_ID FLAGS_ID FLAG_TYPE MON_ATTITUDE MON_ALERTNESS
%token	<i> MON_APPEARANCE ROOMDOOR_ID IF_ID ELSE_ID
%token	<i> SPILL_ID TERRAIN_ID HORIZ_OR_VERT REPLACE_TERRAIN_ID
%token	<i> EXIT_ID
%token	<i> ',' ':' '(' ')' '[' ']' '{' '}'
%token	<map> STRING MAP_ID
%type	<i> h_justif v_justif trap_name room_type door_state light_state
%type	<i> alignment altar_type a_register roomfill door_pos
%type	<i> door_wall walled secret amount chance
%type	<i> dir_list
%type	<i> engraving_type flags flag_list prefilled lev_region lev_init
%type	<i> monster monster_c m_register object object_c o_register
%type	<i> comparestmt
%type	<map> string level_def m_name o_name
%type	<corpos> corr_spec
%start	file

%%
file		: /* nothing */
		| levels
		;

levels		: level
		| level levels
		;

level		: level_def flags lev_init levstatements
		  {
			if (fatal_error > 0) {
				(void) fprintf(stderr,
				"%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
			        splev.init_lev.init_style = (xchar) $3;
				splev.init_lev.flags = (long) $2;
				if (!write_level_file($1, &splev)) {
				    yyerror("Can't write output file!!");
				    exit(EXIT_FAILURE);
				}
			}
			Free($1);
		  }
		;

level_def	: LEVEL_ID ':' string
		  {
			if (index($3, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if ((int) strlen($3) > 8)
			    yyerror("Level names limited to 8 characters.");
			n_plist = n_mlist = n_olist = 0;
			$$ = $3;
		  }
		;

lev_init	: /* nothing */
		  {
			$$ = LVLINIT_NONE;
		  }
		| LEV_INIT_ID ':' SOLID_FILL_ID ',' CHAR
		  {
		      splev.init_lev.filling = what_map_char((char) $5);
		      if (splev.init_lev.filling == INVALID_TYPE ||
			  splev.init_lev.filling >= MAX_TYPE)
			    yyerror("INIT_MAP: Invalid fill char type.");
		      $$ = LVLINIT_SOLIDFILL;
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
		| LEV_INIT_ID ':' MAZE_GRID_ID ',' CHAR
		  {
		      splev.init_lev.filling = what_map_char((char) $5);
		      if (splev.init_lev.filling == INVALID_TYPE ||
			  splev.init_lev.filling >= MAX_TYPE)
			    yyerror("INIT_MAP: Invalid fill char type.");
		      $$ = LVLINIT_MAZEGRID;
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
		| LEV_INIT_ID ':' MINES_ID ',' CHAR ',' CHAR ',' BOOLEAN ',' BOOLEAN ',' light_state ',' walled opt_fillchar
		  {
			splev.init_lev.fg = what_map_char((char) $5);
			if (splev.init_lev.fg == INVALID_TYPE ||
			  splev.init_lev.filling >= MAX_TYPE)
			    yyerror("INIT_MAP: Invalid foreground type.");
			splev.init_lev.bg = what_map_char((char) $7);
			if (splev.init_lev.bg == INVALID_TYPE ||
			  splev.init_lev.filling >= MAX_TYPE)
			    yyerror("INIT_MAP: Invalid background type.");
			splev.init_lev.smoothed = $9;
			splev.init_lev.joined = $11;
			if (splev.init_lev.joined &&
			    splev.init_lev.fg != CORR && splev.init_lev.fg != ROOM)
			    yyerror("INIT_MAP: Invalid foreground type for joined map.");
			splev.init_lev.lit = $13;
			splev.init_lev.walled = $15;

			splev.init_lev.filling = $<i>16;
			if (splev.init_lev.filling == INVALID_TYPE)
			    yyerror("INIT_MAP: Invalid fill char type.");

			$$ = LVLINIT_MINES;
			max_x_map = COLNO-1;
			max_y_map = ROWNO;
		  }
		;

opt_fillchar	: /* nothing */
		  {
		      $<i>$ = -1;
		  }
		| ',' CHAR
		  {
		      $<i>$ = what_map_char((char) $2);
		  }
		;


walled		: BOOLEAN
		| RANDOM_TYPE
		;

flags		: /* nothing */
		  {
			$$ = 0;
		  }
		| FLAGS_ID ':' flag_list
		  {
			$$ = lev_flags;
			lev_flags = 0;	/* clear for next user */
		  }
		;

flag_list	: FLAG_TYPE ',' flag_list
		  {
			lev_flags |= $1;
		  }
		| FLAG_TYPE
		  {
			lev_flags |= $1;
		  }
		;

levstatements	: /* nothing */
		| levstatement levstatements
		;

levstatement 	: message
		| altar_detail
		| branch_region
		| corridor
		| diggable_detail
		| door_detail
		| drawbridge_detail
		| engraving_detail
		| fountain_detail
		| gold_detail
		| ifstatement
		| exitstatement
		| init_reg
		| ladder_detail
		| map_definition
		| mazewalk_detail
		| monster_detail
		| object_detail
		| passwall_detail
		| pool_detail
		| portal_region
		| random_corridors
		| region_detail
		| room_def
		| subroom_def
		| room_chance
		| room_name
		| sink_detail
		| terrain_detail
		| replace_terrain_detail
		| spill_detail
		| randline_detail
		| stair_detail
		| stair_region
		| teleprt_region
		| trap_detail
		| wallify_detail
		;

exitstatement	: EXIT_ID
		  {
		      add_opcode(&splev, SPO_EXIT, NULL);
		  }
		;


comparestmt     : PERCENT
                  {
                     /* val > rn2(100) */
                     struct opvar *tmppush = New(struct opvar);
                     struct opvar *tmprn2 = New(struct opvar);

                     set_opvar_int(tmprn2, 100);

                     add_opcode(&splev, SPO_PUSH, tmprn2);
                     add_opcode(&splev, SPO_RN2, NULL);

                     set_opvar_int(tmppush, (long) $1);
                     add_opcode(&splev, SPO_PUSH, tmppush);

                     $$ = SPO_JGE; /* TODO: shouldn't this be SPO_JG? */
                  }
		;

ifstatement 	: IF_ID comparestmt
		  {
		      struct opvar *tmppush2 = New(struct opvar);

		      if (n_if_list >= MAX_NESTED_IFS) {
			  yyerror("IF: Too deeply nested IFs.");
			  n_if_list = MAX_NESTED_IFS - 1;
		      }

		      add_opcode(&splev, SPO_CMP, NULL);

		      set_opvar_int(tmppush2, -1);

		      if_list[n_if_list++] = tmppush2;

		      add_opcode(&splev, SPO_PUSH, tmppush2);

		      add_opcode(&splev, $2, NULL);
		  }
		 if_ending
		  {
		     /* do nothing */
		  }
		;

if_ending	: '{' levstatements '}'
		  {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev.init_lev.n_opcodes-1);
		      } else yyerror("IF: Huh?!  No start address?");
		  }
		| '{' levstatements '}'
		  {
		      if (n_if_list > 0) {
			  struct opvar *tmppush = New(struct opvar);
			  struct opvar *tmppush2;

			  set_opvar_int(tmppush, -1);
			  add_opcode(&splev, SPO_PUSH, tmppush);

			  add_opcode(&splev, SPO_JMP, NULL);

			  tmppush2 = (struct opvar *) if_list[--n_if_list];

			  set_opvar_int(tmppush2, splev.init_lev.n_opcodes-1);
			  if_list[n_if_list++] = tmppush;
		      } else yyerror("IF: Huh?!  No else-part address?");
		  }
		 ELSE_ID '{' levstatements '}'
		  {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev.init_lev.n_opcodes-1);
		      } else yyerror("IF: Huh?! No end address?");
		  }
		;

message		: MESSAGE_ID ':' STRING
		  {
		      if (strlen($3) > 254)
			  yyerror("Message string > 255 characters.");
		      else {
			  struct opvar *tmppush = New(struct opvar);
			  set_opvar_str(tmppush, $3);
			  add_opcode(&splev, SPO_PUSH, tmppush);
			  add_opcode(&splev, SPO_MESSAGE, NULL);
		      }
		  }
		;

cobj_ifstatement : IF_ID '[' comparestmt ']'
		  {
		      struct opvar *tmppush2 = New(struct opvar);

		      if (n_if_list >= MAX_NESTED_IFS) {
			  yyerror("IF: Too deeply nested IFs.");
			  n_if_list = MAX_NESTED_IFS - 1;
		      }

		      add_opcode(&splev, SPO_CMP, NULL);

		      set_opvar_int(tmppush2, -1);

		      if_list[n_if_list++] = tmppush2;

		      add_opcode(&splev, SPO_PUSH, tmppush2);

		      add_opcode(&splev, $3, NULL);
		  }
		 cobj_if_ending
		  {
		     /* do nothing */
		  }
		;

cobj_if_ending	: '{' cobj_statements '}'
		  {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev.init_lev.n_opcodes-1);
		      } else yyerror("IF: Huh?!  No start address?");
		  }
		| '{' cobj_statements '}'
		  {
		      if (n_if_list > 0) {
			  struct opvar *tmppush = New(struct opvar);
			  struct opvar *tmppush2;

			  set_opvar_int(tmppush, -1);
			  add_opcode(&splev, SPO_PUSH, tmppush);

			  add_opcode(&splev, SPO_JMP, NULL);

			  tmppush2 = (struct opvar *) if_list[--n_if_list];

			  set_opvar_int(tmppush2, splev.init_lev.n_opcodes-1);
			  if_list[n_if_list++] = tmppush;
		      } else yyerror("IF: Huh?!  No else-part address?");
		  }
		 ELSE_ID '{' cobj_statements '}'
		  {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev.init_lev.n_opcodes-1);
		      } else yyerror("IF: Huh?! No end address?");
		  }
		;



random_corridors: RAND_CORRIDOR_ID
		  {
		      struct opvar *srcroom = New(struct opvar);
		      struct opvar *srcwall = New(struct opvar);
		      struct opvar *srcdoor = New(struct opvar);
		      struct opvar *desroom = New(struct opvar);
		      struct opvar *deswall = New(struct opvar);
		      struct opvar *desdoor = New(struct opvar);

		      set_opvar_int(srcroom, -1);
		      set_opvar_int(srcwall, -1);
		      set_opvar_int(srcdoor, -1);
		      set_opvar_int(desroom, -1);
		      set_opvar_int(deswall, -1);
		      set_opvar_int(desdoor, -1);

		      add_opcode(&splev, SPO_PUSH, srcroom);
		      add_opcode(&splev, SPO_PUSH, srcdoor);
		      add_opcode(&splev, SPO_PUSH, srcwall);
		      add_opcode(&splev, SPO_PUSH, desroom);
		      add_opcode(&splev, SPO_PUSH, desdoor);
		      add_opcode(&splev, SPO_PUSH, deswall);

		      add_opcode(&splev, SPO_CORRIDOR, NULL);
		  }
		;

corridor	: CORRIDOR_ID ':' corr_spec ',' corr_spec
		  {
		      struct opvar *srcroom = New(struct opvar);
		      struct opvar *srcwall = New(struct opvar);
		      struct opvar *srcdoor = New(struct opvar);
		      struct opvar *desroom = New(struct opvar);
		      struct opvar *deswall = New(struct opvar);
		      struct opvar *desdoor = New(struct opvar);

		      set_opvar_int(srcroom, $3.room);
		      set_opvar_int(srcwall, $3.wall);
		      set_opvar_int(srcdoor, $3.door);

		      set_opvar_int(desroom, $5.room);
		      set_opvar_int(deswall, $5.wall);
		      set_opvar_int(desdoor, $5.door);

		      add_opcode(&splev, SPO_PUSH, srcroom);
		      add_opcode(&splev, SPO_PUSH, srcdoor);
		      add_opcode(&splev, SPO_PUSH, srcwall);
		      add_opcode(&splev, SPO_PUSH, desroom);
		      add_opcode(&splev, SPO_PUSH, desdoor);
		      add_opcode(&splev, SPO_PUSH, deswall);

		      add_opcode(&splev, SPO_CORRIDOR, NULL);
		  }
		| CORRIDOR_ID ':' corr_spec ',' INTEGER
		  {
		      struct opvar *srcroom = New(struct opvar);
		      struct opvar *srcwall = New(struct opvar);
		      struct opvar *srcdoor = New(struct opvar);
		      struct opvar *desroom = New(struct opvar);
		      struct opvar *deswall = New(struct opvar);
		      struct opvar *desdoor = New(struct opvar);

		      set_opvar_int(srcroom, $3.room);
		      set_opvar_int(srcwall, $3.wall);
		      set_opvar_int(srcdoor, $3.door);

		      set_opvar_int(desroom, -1);
		      set_opvar_int(deswall, $5);
		      set_opvar_int(desdoor, -1);

		      add_opcode(&splev, SPO_PUSH, srcroom);
		      add_opcode(&splev, SPO_PUSH, srcdoor);
		      add_opcode(&splev, SPO_PUSH, srcwall);
		      add_opcode(&splev, SPO_PUSH, desroom);
		      add_opcode(&splev, SPO_PUSH, desdoor);
		      add_opcode(&splev, SPO_PUSH, deswall);

		      add_opcode(&splev, SPO_CORRIDOR, NULL);
		  }
		;

corr_spec	: '(' INTEGER ',' DIRECTION ',' door_pos ')'
		  {
			$$.room = $2;
			$$.wall = $4;
			$$.door = $6;
		  }
		;

room_begin      : room_type chance ',' light_state
                  {
		      if (($2 == 1) && ($1 == OROOM))
			  yyerror("Only typed rooms can have a chance.");
		      else {
			  struct opvar *rtype = New(struct opvar);
			  struct opvar *rchance = New(struct opvar);
			  struct opvar *rlit = New(struct opvar);

			  set_opvar_int(rtype, $1);
			  set_opvar_int(rchance, $2);
			  set_opvar_int(rlit, $4);

			  add_opcode(&splev, SPO_PUSH, rtype);
			  add_opcode(&splev, SPO_PUSH, rchance);
			  add_opcode(&splev, SPO_PUSH, rlit);
		      }
                  }
                ;

subroom_def	: SUBROOM_ID ':' room_begin ',' subroom_pos ',' room_size roomfill
		  {
		      struct opvar *filled = New(struct opvar);
		      struct opvar *xalign = New(struct opvar);
		      struct opvar *yalign = New(struct opvar);
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *w = New(struct opvar);
		      struct opvar *h = New(struct opvar);

		      set_opvar_int(filled, $8);
		      set_opvar_int(xalign, ERR);
		      set_opvar_int(yalign, ERR);
		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(w, current_size.width);
		      set_opvar_int(h, current_size.height);

		      add_opcode(&splev, SPO_PUSH, filled);
		      add_opcode(&splev, SPO_PUSH, xalign);
		      add_opcode(&splev, SPO_PUSH, yalign);
		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, w);
		      add_opcode(&splev, SPO_PUSH, h);

		      add_opcode(&splev, SPO_SUBROOM, NULL);
		  }
		  '{' levstatements '}'
		  {
		      add_opcode(&splev, SPO_ENDROOM, NULL);
		  }
		;

room_def	: ROOM_ID ':' room_begin ',' room_pos ',' room_align ',' room_size roomfill
		  {
		      struct opvar *filled = New(struct opvar);
		      struct opvar *xalign = New(struct opvar);
		      struct opvar *yalign = New(struct opvar);
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *w = New(struct opvar);
		      struct opvar *h = New(struct opvar);

		      set_opvar_int(filled, $10);
		      set_opvar_int(xalign, current_align.x);
		      set_opvar_int(yalign, current_align.y);
		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(w, current_size.width);
		      set_opvar_int(h, current_size.height);

		      add_opcode(&splev, SPO_PUSH, filled);
		      add_opcode(&splev, SPO_PUSH, xalign);
		      add_opcode(&splev, SPO_PUSH, yalign);
		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, w);
		      add_opcode(&splev, SPO_PUSH, h);

		      add_opcode(&splev, SPO_ROOM, NULL);
		  }
		  '{' levstatements '}'
		  {
		      add_opcode(&splev, SPO_ENDROOM, NULL);
		  }
		;

roomfill	: /* nothing */
		  {
			$$ = 1;
		  }
		| ',' BOOLEAN
		  {
			$$ = $2;
		  }
		;

room_pos	: '(' INTEGER ',' INTEGER ')'
		  {
			if ( $2 < 1 || $2 > 5 ||
			    $4 < 1 || $4 > 5 ) {
			    yyerror("Room position should be between 1 & 5!");
			} else {
			    current_coord.x = $2;
			    current_coord.y = $4;
			}
		  }
		| RANDOM_TYPE
		  {
			current_coord.x = current_coord.y = ERR;
		  }
		;

subroom_pos	: '(' INTEGER ',' INTEGER ')'
		  {
			if ( $2 < 0 || $4 < 0) {
			    yyerror("Invalid subroom position !");
			} else {
			    current_coord.x = $2;
			    current_coord.y = $4;
			}
		  }
		| RANDOM_TYPE
		  {
			current_coord.x = current_coord.y = ERR;
		  }
		;

room_align	: '(' h_justif ',' v_justif ')'
		  {
			current_align.x = $2;
			current_align.y = $4;
		  }
		| RANDOM_TYPE
		  {
			current_align.x = current_align.y = ERR;
		  }
		;

room_size	: '(' INTEGER ',' INTEGER ')'
		  {
			current_size.width = $2;
			current_size.height = $4;
		  }
		| RANDOM_TYPE
		  {
			current_size.height = current_size.width = ERR;
		  }
		;

room_name	: NAME_ID ':' string
		  {
		      yyerror("NAME for rooms is not used anymore.");
		  }
		;

room_chance	: CHANCE_ID ':' INTEGER
		   {
		       yyerror("CHANCE for rooms is not used anymore.");
		   }
		;

door_detail	: ROOMDOOR_ID ':' secret ',' door_state ',' door_wall ',' door_pos
		  {
			/* ERR means random here */
			if ($7 == ERR && $9 != ERR) {
		     yyerror("If the door wall is random, so must be its pos!");
			} else {
			    struct opvar *secret = New(struct opvar);
			    struct opvar *mask = New(struct opvar);
			    struct opvar *wall = New(struct opvar);
			    struct opvar *pos = New(struct opvar);

			    set_opvar_int(secret, $3);
			    set_opvar_int(mask, $5);
			    set_opvar_int(wall, $7);
			    set_opvar_int(pos, $9);

			    add_opcode(&splev, SPO_PUSH, pos);
			    add_opcode(&splev, SPO_PUSH, mask);
			    add_opcode(&splev, SPO_PUSH, secret);
			    add_opcode(&splev, SPO_PUSH, wall);

			    add_opcode(&splev, SPO_ROOM_DOOR, NULL);
			}
		  }
		| DOOR_ID ':' door_state ',' coordinate
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *mask = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(mask, $<i>3);

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, mask);

		      add_opcode(&splev, SPO_DOOR, NULL);
		  }
		;

secret		: BOOLEAN
		| RANDOM_TYPE
		;

door_wall	: dir_list
		| RANDOM_TYPE
		;

dir_list	: DIRECTION
		  {
		      $$ = $1;
		  }
		| DIRECTION '|' dir_list
		  {
		      $$ = ($1 | $3);
		  }
		;

door_pos	: INTEGER
		| RANDOM_TYPE
		;

map_definition	: NOMAP_ID
		  {
		      struct opvar *zaligntyp = New(struct opvar);
		      struct opvar *keep_region = New(struct opvar);
		      struct opvar *halign = New(struct opvar);
		      struct opvar *valign = New(struct opvar);
		      struct opvar *xsize = New(struct opvar);
		      struct opvar *ysize = New(struct opvar);
		      struct opvar *mpart = New(struct opvar);

		      set_opvar_int(zaligntyp, 0);
		      set_opvar_int(keep_region, 1);
		      set_opvar_int(halign, 1);
		      set_opvar_int(valign, 1);
		      set_opvar_int(xsize, 0);
		      set_opvar_int(ysize, 0);
		      set_opvar_str(mpart, (char *)0);

		      add_opcode(&splev, SPO_PUSH, zaligntyp);
		      add_opcode(&splev, SPO_PUSH, keep_region);
		      add_opcode(&splev, SPO_PUSH, halign);
		      add_opcode(&splev, SPO_PUSH, valign);
		      add_opcode(&splev, SPO_PUSH, mpart);
		      add_opcode(&splev, SPO_PUSH, ysize);
		      add_opcode(&splev, SPO_PUSH, xsize);

		      add_opcode(&splev, SPO_MAP, NULL);

		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
		| map_geometry roomfill MAP_ID
		  {
		      struct opvar *zaligntyp = New(struct opvar);
		      struct opvar *keep_region = New(struct opvar);
		      struct opvar *halign = New(struct opvar);
		      struct opvar *valign = New(struct opvar);

		      set_opvar_int(zaligntyp, 1);
		      set_opvar_int(keep_region, $2);
		      set_opvar_int(halign, $<i>1 % 10);
		      set_opvar_int(valign, $<i>1 / 10);

		      add_opcode(&splev, SPO_PUSH, zaligntyp);
		      add_opcode(&splev, SPO_PUSH, keep_region);
		      add_opcode(&splev, SPO_PUSH, halign);
		      add_opcode(&splev, SPO_PUSH, valign);

		      scan_map($3, &splev);

		      Free($3);
		  }
		| GEOMETRY_ID ':' coordinate roomfill MAP_ID
		  {
		      struct opvar *zaligntyp = New(struct opvar);
		      struct opvar *keep_region = New(struct opvar);
		      struct opvar *halign = New(struct opvar);
		      struct opvar *valign = New(struct opvar);

		      set_opvar_int(zaligntyp, 2);
		      set_opvar_int(keep_region, $4);
		      set_opvar_int(halign, current_coord.x);
		      set_opvar_int(valign, current_coord.y);

		      add_opcode(&splev, SPO_PUSH, zaligntyp);
		      add_opcode(&splev, SPO_PUSH, keep_region);
		      add_opcode(&splev, SPO_PUSH, halign);
		      add_opcode(&splev, SPO_PUSH, valign);

		      scan_map($5, &splev);

		      Free($5);
		  }
		;

map_geometry	: GEOMETRY_ID ':' h_justif ',' v_justif
		  {
			$<i>$ = $<i>3 + ($<i>5 * 10);
		  }
		;

h_justif	: LEFT_OR_RIGHT
		| CENTER
		;

v_justif	: TOP_OR_BOT
		| CENTER
		;

init_reg	: RANDOM_OBJECTS_ID ':' object_list
		  {
		      struct opvar *ol = New(struct opvar);
		     char *tmp_olist;

		     tmp_olist = (char *) alloc(n_olist+1);
		     (void) memcpy((genericptr_t)tmp_olist,
				   (genericptr_t)olist, n_olist);
		     tmp_olist[n_olist] = 0;

		     set_opvar_str(ol, tmp_olist);
		     add_opcode(&splev, SPO_PUSH, ol);

		     add_opcode(&splev, SPO_RANDOM_OBJECTS, NULL);
		     on_olist = n_olist;
		     n_olist = 0;
		  }
		| RANDOM_PLACES_ID ':' place_list
		  {
		      struct opvar *coords = New(struct opvar);
		     char *tmp_plist;
		     int i;

		     tmp_plist = (char *) alloc(n_plist*2+1);

		     for (i=0; i<n_plist; i++) {
			tmp_plist[i*2] = plist[i].x+1;
			tmp_plist[i*2+1] = plist[i].y+1;
		     }
		     tmp_plist[n_plist*2] = 0;

		     set_opvar_str(coords, tmp_plist);
		     add_opcode(&splev, SPO_PUSH, coords);

		     add_opcode(&splev, SPO_RANDOM_PLACES, NULL);
		     on_plist = n_plist;
		     n_plist = 0;
		  }
		| RANDOM_MONSTERS_ID ':' monster_list
		  {
		      struct opvar *ml = New(struct opvar);
		     char *tmp_mlist;

		     tmp_mlist = (char *) alloc(n_mlist+1);
		     (void) memcpy((genericptr_t)tmp_mlist,
				   (genericptr_t)mlist, n_mlist);
		     tmp_mlist[n_mlist] = 0;

		     set_opvar_str(ml, tmp_mlist);
		     add_opcode(&splev, SPO_PUSH, ml);

		     add_opcode(&splev, SPO_RANDOM_MONSTERS, NULL);
		     on_mlist = n_mlist;
		     n_mlist = 0;
		  }
		;

object_list	: object
		  {
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = $<i>1;
			else
			    yyerror("Object list too long!");
		  }
		| object ',' object_list
		  {
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = $<i>1;
			else
			    yyerror("Object list too long!");
		  }
		;

monster_list	: monster
		  {
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = $<i>1;
			else
			    yyerror("Monster list too long!");
		  }
		| monster ',' monster_list
		  {
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = $<i>1;
			else
			    yyerror("Monster list too long!");
		  }
		;

place_list	: place
		  {
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
		| place
		  {
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
		 ',' place_list
		;

monster_detail	: MONSTER_ID chance ':' monster_desc
		  {
		      add_opcode(&splev, SPO_MONSTER, NULL);

		      if ( 1 == $2 ) {
			  if (n_if_list > 0) {
			      struct opvar *tmpjmp;
			      tmpjmp = (struct opvar *) if_list[--n_if_list];
			      set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			  } else yyerror("conditional creation of monster, but no jump point marker.");
		      }
		  }
		;

monster_desc	: monster_c ',' m_name ',' coordinate monster_infos
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *class = New(struct opvar);
		      struct opvar *id = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(class, $<i>1);
		      set_opvar_int(id, NON_PM);

		      if ($3) {
			  int token = get_monster_id($3, (char) $<i>1);
			  if (token == ERR)
			      yywarning("Invalid monster name!  Making random monster.");
			  else
			      set_opvar_int(id, token);
			  Free($3);
		      }

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, class);
		      add_opcode(&splev, SPO_PUSH, id);
		  }
		;

monster_infos	: /* nothing */
		  {
		      struct opvar *stopit = New(struct opvar);
		      set_opvar_int(stopit, SP_M_V_END);
		      add_opcode(&splev, SPO_PUSH, stopit);
		      $<i>$ = 0x00;
		  }
		| monster_infos monster_info
		  {
		      if (( $<i>1 & $<i>2 ))
			  yyerror("MONSTER extra info already used.");
		      $<i>$ = ( $<i>1 | $<i>2 );
		  }
		;

monster_info	: ',' string
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *monname = New(struct opvar);

		      set_opvar_str(monname, $2);
		      set_opvar_int(info, SP_M_V_NAME);

		      add_opcode(&splev, SPO_PUSH, monname);
		      add_opcode(&splev, SPO_PUSH, info);
		      $<i>$ = 0x01;
		  }
		| ',' MON_ATTITUDE
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *peaceful = New(struct opvar);

		      set_opvar_int(peaceful, $<i>2);
		      set_opvar_int(info, SP_M_V_PEACEFUL);

		      add_opcode(&splev, SPO_PUSH, peaceful);
		      add_opcode(&splev, SPO_PUSH, info);
		      $<i>$ = 0x02;
		  }
		| ',' MON_ALERTNESS
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *asleep = New(struct opvar);

		      set_opvar_int(asleep, $<i>2);
		      set_opvar_int(info, SP_M_V_ASLEEP);

		      add_opcode(&splev, SPO_PUSH, asleep);
		      add_opcode(&splev, SPO_PUSH, info);
		      $<i>$ = 0x04;
		  }
		| ',' alignment
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *malign = New(struct opvar);

		      set_opvar_int(malign, $<i>2);
		      set_opvar_int(info, SP_M_V_ALIGN);

		      add_opcode(&splev, SPO_PUSH, malign);
		      add_opcode(&splev, SPO_PUSH, info);
		      $<i>$ = 0x08;
		  }
		| ',' MON_APPEARANCE string
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *appear = New(struct opvar);
		      struct opvar *appearstr = New(struct opvar);

		      set_opvar_int(appear, $<i>2);
		      set_opvar_str(appearstr, $3);
		      set_opvar_int(info, SP_M_V_APPEAR);

		      add_opcode(&splev, SPO_PUSH, appearstr);
		      add_opcode(&splev, SPO_PUSH, appear);
		      add_opcode(&splev, SPO_PUSH, info);
		      $<i>$ = 0x10;
		  }
		;

cobj_statements	: /* nothing */
		  {
		  }
		| cobj_statement cobj_statements
		;

cobj_statement  : cobj_detail
		| cobj_ifstatement
		;

cobj_detail	: OBJECT_ID chance ':' cobj_desc
		  {
		      struct opvar *containment = New(struct opvar);
		      set_opvar_int(containment, SP_OBJ_CONTENT);
		      add_opcode(&splev, SPO_PUSH, containment);
		      add_opcode(&splev, SPO_OBJECT, NULL);
		      if ( 1 == $2 ) {
			  if (n_if_list > 0) {
			      struct opvar *tmpjmp;
			      tmpjmp = (struct opvar *) if_list[--n_if_list];
			      set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			  } else yyerror("conditional creation of obj, but no jump point marker.");
		      }
		  }
		| COBJECT_ID chance ':' cobj_desc
		  {
		      struct opvar *containment = New(struct opvar);

		      set_opvar_int(containment, SP_OBJ_CONTENT|SP_OBJ_CONTAINER);
		      add_opcode(&splev, SPO_PUSH, containment);
		      add_opcode(&splev, SPO_OBJECT, NULL);
			/* 1: is contents of preceeding object with 2 */
			/* 2: is a container */
			/* 0: neither */
		      $<i>$ = $2;
		  }
		'{' cobj_statements '}'
		  {
		      add_opcode(&splev, SPO_POP_CONTAINER, NULL);

		      if ( 1 == $<i>5 ) {
			  if (n_if_list > 0) {
			      struct opvar *tmpjmp;
			      tmpjmp = (struct opvar *) if_list[--n_if_list];
			      set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			  } else yyerror("conditional creation of obj, but no jump point marker.");
		      }
		  }
		;

object_detail	: OBJECT_ID chance ':' object_desc
		  {
		      struct opvar *containment = New(struct opvar);

		      set_opvar_int(containment, 0);
		      add_opcode(&splev, SPO_PUSH, containment);
		      add_opcode(&splev, SPO_OBJECT, NULL);

		      if ( 1 == $2 ) {
			  if (n_if_list > 0) {
			      struct opvar *tmpjmp;
			      tmpjmp = (struct opvar *) if_list[--n_if_list];
			      set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			  } else yyerror("conditional creation of obj, but no jump point marker.");
		      }
		  }
		| COBJECT_ID chance ':' object_desc
		  {
		      struct opvar *containment = New(struct opvar);

		      set_opvar_int(containment, SP_OBJ_CONTAINER);
		      add_opcode(&splev, SPO_PUSH, containment);
		      add_opcode(&splev, SPO_OBJECT, NULL);
			/* 1: is contents of preceeding object with 2 */
			/* 2: is a container */
			/* 0: neither */

		      $<i>$ = $2;
		  }
		'{' cobj_statements '}'
		 {
		     add_opcode(&splev, SPO_POP_CONTAINER, NULL);

		     if ( 1 == $<i>5 ) {
			 if (n_if_list > 0) {
			     struct opvar *tmpjmp;
			     tmpjmp = (struct opvar *) if_list[--n_if_list];
			     set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			 } else yyerror("conditional creation of obj, but no jump point marker.");
		     }
		 }
		;

cobj_desc	: object_c ',' o_name object_infos
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *id = New(struct opvar);
		      struct opvar *class = New(struct opvar);

		      set_opvar_int(x, -1);
		      set_opvar_int(y, -1);
		      set_opvar_int(class, $<i>1);
		      set_opvar_int(id, -1);
		      if ($3) {
			  int token = get_object_id($3, $<i>1);
			  if (token == ERR)
			      yywarning("Illegal object name!  Making random object.");
			  else
			      set_opvar_int(id, token);
			  Free($3);
		      }
		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, class);
		      add_opcode(&splev, SPO_PUSH, id);
		  }
		;

object_desc	: object_c ',' o_name ',' coordinate object_infos
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *id = New(struct opvar);
		      struct opvar *class = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(class, $<i>1);
		      set_opvar_int(id, -1);
		      if ($3) {
			  int token = get_object_id($3, $<i>1);
			  if (token == ERR)
			      yywarning("Illegal object name!  Making random object.");
			  else
			      set_opvar_int(id, token);
			  Free($3);
		      }

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, class);
		      add_opcode(&splev, SPO_PUSH, id);
		}
		;

object_infos	: /* nothing */
		  {
		      struct opvar *stopit = New(struct opvar);
		      set_opvar_int(stopit, SP_O_V_END);
		      add_opcode(&splev, SPO_PUSH, stopit);
		      $<i>$ = 0x00;
		  }
		| object_infos object_info
		  {
		      if (( $<i>1 & $<i>2 ))
			  yyerror("OBJECT extra info already used.");
		      $<i>$ = ( $<i>1 | $<i>2 );
		  }
		;

object_info	: ',' curse_state
		  {
		      $<i>$ = 0x01;
		  }
		| ',' monster_id
		  {
		      $<i>$ = 0x02;
		  }
		| ',' enchantment
		  {
		      $<i>$ = 0x04;
		  }
		| ',' optional_name
		  {
		      $<i>$ = 0x08;
		  }
		;

curse_state	: CURSE_TYPE
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *curse = New(struct opvar);

		      set_opvar_int(curse, $1);
		      set_opvar_int(info, SP_O_V_CURSE);

		      add_opcode(&splev, SPO_PUSH, curse);
		      add_opcode(&splev, SPO_PUSH, info);
		  }
		;

monster_id	: STRING
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *corpsenm = New(struct opvar);

		      int token = get_monster_id($1, (char)0);

		      if (token == ERR) {
			  /* "random" */
			  yywarning("Are you sure you didn't mean NAME:\"foo\"?");
			  token = NON_PM - 1;
		      }

		      set_opvar_int(corpsenm, token);
		      set_opvar_int(info, SP_O_V_CORPSENM);

		      add_opcode(&splev, SPO_PUSH, corpsenm);
		      add_opcode(&splev, SPO_PUSH, info);

		      Free($1);
		  }
		;

enchantment	: INTEGER
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *spe = New(struct opvar);

		      set_opvar_int(spe, $1);
		      set_opvar_int(info, SP_O_V_SPE);

		      add_opcode(&splev, SPO_PUSH, spe);
		      add_opcode(&splev, SPO_PUSH, info);
		  }
		;

optional_name	: NAME_ID ':' STRING
		  {
		      struct opvar *info = New(struct opvar);
		      struct opvar *name = New(struct opvar);

		      set_opvar_str(name, $3);
		      set_opvar_int(info, SP_O_V_NAME);

		      add_opcode(&splev, SPO_PUSH, name);
		      add_opcode(&splev, SPO_PUSH, info);
		  }
		;

trap_detail	: TRAP_ID chance ':' trap_name ',' coordinate
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *typ = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(typ, $<i>4);

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, typ);

		      add_opcode(&splev, SPO_TRAP, NULL);

		      if ( 1 == $2 ) {
			  if (n_if_list > 0) {
			      struct opvar *tmpjmp;
			      tmpjmp = (struct opvar *) if_list[--n_if_list];
			      set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			  } else yyerror("conditional creation of trap, but no jump point marker.");
		      }
		  }
		;

drawbridge_detail: DRAWBRIDGE_ID ':' coordinate ',' DIRECTION ',' door_state
		   {
		       struct opvar *dx = New(struct opvar);
		       struct opvar *dy = New(struct opvar);
		       struct opvar *dir = New(struct opvar);
		       struct opvar *state = New(struct opvar);

		       int x, y, d;

		       set_opvar_int(dx, current_coord.x);
		       set_opvar_int(dy, current_coord.y);

		       x = current_coord.x;
		       y = current_coord.y;
		       /* convert dir from a DIRECTION to a DB_DIR */
		       d = $5;
		       switch(d) {
		       case W_NORTH: d = DB_NORTH; y--; break;
		       case W_SOUTH: d = DB_SOUTH; y++; break;
		       case W_EAST:  d = DB_EAST;  x++; break;
		       case W_WEST:  d = DB_WEST;  x--; break;
		       default:
			   yyerror("Invalid drawbridge direction");
			   break;
		       }
		       set_opvar_int(dir, d);

		       if ( $<i>7 == D_ISOPEN )
			   set_opvar_int(state, 1);
		       else if ( $<i>7 == D_CLOSED )
			   set_opvar_int(state, 0);
		       else
			   yyerror("A drawbridge can only be open or closed!");

		       add_opcode(&splev, SPO_PUSH, dx);
		       add_opcode(&splev, SPO_PUSH, dy);
		       add_opcode(&splev, SPO_PUSH, state);
		       add_opcode(&splev, SPO_PUSH, dir);

		       add_opcode(&splev, SPO_DRAWBRIDGE, NULL);
		   }
		;

mazewalk_detail : MAZEWALK_ID ':' coordinate ',' DIRECTION
		  {
		       struct opvar *x = New(struct opvar);
		       struct opvar *y = New(struct opvar);
		       struct opvar *dir = New(struct opvar);
		       struct opvar *stocked = New(struct opvar);
		       struct opvar *typ = New(struct opvar);

		       set_opvar_int(x, current_coord.x);
		       set_opvar_int(y, current_coord.y);
		       set_opvar_int(dir, $5);
		       set_opvar_int(stocked, 1);
		       set_opvar_int(typ, 0);

		       add_opcode(&splev, SPO_PUSH, x);
		       add_opcode(&splev, SPO_PUSH, y);
		       add_opcode(&splev, SPO_PUSH, dir);
		       add_opcode(&splev, SPO_PUSH, stocked);
		       add_opcode(&splev, SPO_PUSH, typ);

		       add_opcode(&splev, SPO_MAZEWALK, NULL);
		  }
		| MAZEWALK_ID ':' coordinate ',' DIRECTION ',' BOOLEAN opt_fillchar
		  {
		       struct opvar *x = New(struct opvar);
		       struct opvar *y = New(struct opvar);
		       struct opvar *dir = New(struct opvar);
		       struct opvar *stocked = New(struct opvar);
		       struct opvar *typ = New(struct opvar);

		       set_opvar_int(x, current_coord.x);
		       set_opvar_int(y, current_coord.y);
		       set_opvar_int(dir, $5);
		       set_opvar_int(stocked, $<i>7);
		       set_opvar_int(typ, $<i>8);

		       add_opcode(&splev, SPO_PUSH, x);
		       add_opcode(&splev, SPO_PUSH, y);
		       add_opcode(&splev, SPO_PUSH, dir);
		       add_opcode(&splev, SPO_PUSH, stocked);
		       add_opcode(&splev, SPO_PUSH, typ);

		       add_opcode(&splev, SPO_MAZEWALK, NULL);
		  }
		;

wallify_detail	: WALLIFY_ID
		  {
		      add_opcode(&splev, SPO_WALLIFY, NULL);
		  }
		;

ladder_detail	: LADDER_ID ':' coordinate ',' UP_OR_DOWN
		  {
		       struct opvar *x = New(struct opvar);
		       struct opvar *y = New(struct opvar);
		       struct opvar *up = New(struct opvar);

		       set_opvar_int(x, current_coord.x);
		       set_opvar_int(y, current_coord.y);
		       set_opvar_int(up, $<i>5);

		       add_opcode(&splev, SPO_PUSH, x);
		       add_opcode(&splev, SPO_PUSH, y);
		       add_opcode(&splev, SPO_PUSH, up);

		       add_opcode(&splev, SPO_LADDER, NULL);
		  }
		;

stair_detail	: STAIR_ID ':' coordinate ',' UP_OR_DOWN
		  {
		       struct opvar *x = New(struct opvar);
		       struct opvar *y = New(struct opvar);
		       struct opvar *up = New(struct opvar);

		       set_opvar_int(x, current_coord.x);
		       set_opvar_int(y, current_coord.y);
		       set_opvar_int(up, $<i>5);

		       add_opcode(&splev, SPO_PUSH, x);
		       add_opcode(&splev, SPO_PUSH, y);
		       add_opcode(&splev, SPO_PUSH, up);

		       add_opcode(&splev, SPO_STAIR, NULL);
		  }
		;

stair_region	: STAIR_ID ':' lev_region
		  {
		       struct opvar *x1 = New(struct opvar);
		       struct opvar *y1 = New(struct opvar);
		       struct opvar *x2 = New(struct opvar);
		       struct opvar *y2 = New(struct opvar);
		       struct opvar *in_islev = New(struct opvar);

		       set_opvar_int(x1, current_region.x1);
		       set_opvar_int(y1, current_region.y1);
		       set_opvar_int(x2, current_region.x2);
		       set_opvar_int(y2, current_region.y2);
		       set_opvar_int(in_islev, $3);

		       add_opcode(&splev, SPO_PUSH, x1);
		       add_opcode(&splev, SPO_PUSH, y1);
		       add_opcode(&splev, SPO_PUSH, x2);
		       add_opcode(&splev, SPO_PUSH, y2);
		       add_opcode(&splev, SPO_PUSH, in_islev);
		  }
		  ',' lev_region ',' UP_OR_DOWN
		  {
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *del_islev = New(struct opvar);
		      struct opvar *rtype = New(struct opvar);
		      struct opvar *rname = New(struct opvar);
		      struct opvar *padding = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);
		      set_opvar_int(del_islev, $6);
		      set_opvar_int(rtype, ($8) ? LR_UPSTAIR : LR_DOWNSTAIR);
		      set_opvar_str(rname, (char *)0);
		      set_opvar_int(padding, 0);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, del_islev);
		      add_opcode(&splev, SPO_PUSH, rtype);
		      add_opcode(&splev, SPO_PUSH, padding);
		      add_opcode(&splev, SPO_PUSH, rname);

		      add_opcode(&splev, SPO_LEVREGION, NULL);
		  }
		;

portal_region	: PORTAL_ID ':' lev_region
		  {
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *in_islev = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);
		      set_opvar_int(in_islev, $3);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, in_islev);
		  }
		 ',' lev_region ',' string
		  {
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *del_islev = New(struct opvar);
		      struct opvar *rtype = New(struct opvar);
		      struct opvar *rname = New(struct opvar);
		      struct opvar *padding = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);
		      set_opvar_int(del_islev, $6);
		      set_opvar_int(rtype, LR_PORTAL);
		      set_opvar_str(rname, $8);
		      set_opvar_int(padding, 0);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, del_islev);
		      add_opcode(&splev, SPO_PUSH, rtype);
		      add_opcode(&splev, SPO_PUSH, padding);
		      add_opcode(&splev, SPO_PUSH, rname);

		      add_opcode(&splev, SPO_LEVREGION, NULL);
		  }
		;

teleprt_region	: TELEPRT_ID ':' lev_region
		  {
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *in_islev = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);
		      set_opvar_int(in_islev, $3);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, in_islev);
		  }
		 ',' lev_region
		  {
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *del_islev = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);
		      set_opvar_int(del_islev, $6);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, del_islev);
		  }
		teleprt_detail
		  {
		      struct opvar *rtype = New(struct opvar);
		      struct opvar *rname = New(struct opvar);
		      struct opvar *padding = New(struct opvar);

		      switch($<i>8) {
		      case -1: set_opvar_int(rtype, LR_TELE); break;
		      case  0: set_opvar_int(rtype, LR_DOWNTELE); break;
		      case  1: set_opvar_int(rtype, LR_UPTELE); break;
		      }
		      set_opvar_str(rname, (char *)0);
		      set_opvar_int(padding, 0);

		      add_opcode(&splev, SPO_PUSH, rtype);
		      add_opcode(&splev, SPO_PUSH, padding);
		      add_opcode(&splev, SPO_PUSH, rname);

		      add_opcode(&splev, SPO_LEVREGION, NULL);

		  }
		;

branch_region	: BRANCH_ID ':' lev_region
		  {
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *in_islev = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);
		      set_opvar_int(in_islev, $3);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, in_islev);
		  }
		 ',' lev_region
		  {
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *del_islev = New(struct opvar);
		      struct opvar *rtype = New(struct opvar);
		      struct opvar *rname = New(struct opvar);
		      struct opvar *padding = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);
		      set_opvar_int(del_islev, $6);
		      set_opvar_int(rtype, LR_BRANCH);
		      set_opvar_str(rname, (char *)0);
		      set_opvar_int(padding, 0);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, del_islev);
		      add_opcode(&splev, SPO_PUSH, rtype);
		      add_opcode(&splev, SPO_PUSH, padding);
		      add_opcode(&splev, SPO_PUSH, rname);

		      add_opcode(&splev, SPO_LEVREGION, NULL);
		  }
		;

teleprt_detail	: /* empty */
		  {
			$<i>$ = -1;
		  }
		| ',' UP_OR_DOWN
		  {
			$<i>$ = $2;
		  }
		;

fountain_detail : FOUNTAIN_ID ':' coordinate
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);

		      add_opcode(&splev, SPO_FOUNTAIN, NULL);
		  }
		;

sink_detail : SINK_ID ':' coordinate
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);

		      add_opcode(&splev, SPO_SINK, NULL);
		  }
		;

pool_detail : POOL_ID ':' coordinate
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);

		      add_opcode(&splev, SPO_POOL, NULL);
		  }
		;

replace_terrain_detail : REPLACE_TERRAIN_ID ':' region ',' CHAR ',' CHAR ',' light_state ',' SPERCENT
		  {
		      int c;
		      struct opvar *x1 = New(struct opvar);
		      struct opvar *y1 = New(struct opvar);
		      struct opvar *x2 = New(struct opvar);
		      struct opvar *y2 = New(struct opvar);
		      struct opvar *chance = New(struct opvar);
		      struct opvar *from_ter = New(struct opvar);
		      struct opvar *to_ter = New(struct opvar);
		      struct opvar *to_lit = New(struct opvar);

		      set_opvar_int(x1, current_region.x1);
		      set_opvar_int(y1, current_region.y1);
		      set_opvar_int(x2, current_region.x2);
		      set_opvar_int(y2, current_region.y2);

		      c = $11;
		      if (c < 0) c = 0;
		      else if (c > 100) c = 100;
		      set_opvar_int(chance, c);

		      c = what_map_char((char) $5);
		      set_opvar_int(from_ter, c);
		      if (c >= MAX_TYPE) yyerror("Replace terrain: illegal 'from' map char");

		      c = what_map_char((char) $7);
		      set_opvar_int(to_ter, c);
		      if (c >= MAX_TYPE) yyerror("Replace terrain: illegal 'to' map char");

		      set_opvar_int(to_lit, $9);

		      add_opcode(&splev, SPO_PUSH, x1);
		      add_opcode(&splev, SPO_PUSH, y1);
		      add_opcode(&splev, SPO_PUSH, x2);
		      add_opcode(&splev, SPO_PUSH, y2);
		      add_opcode(&splev, SPO_PUSH, from_ter);
		      add_opcode(&splev, SPO_PUSH, to_ter);
		      add_opcode(&splev, SPO_PUSH, to_lit);
		      add_opcode(&splev, SPO_PUSH, chance);

		      add_opcode(&splev, SPO_REPLACETERRAIN, NULL);
		  }
		;

terrain_detail : TERRAIN_ID chance ':' coordinate ',' CHAR ',' light_state
		 {
		     int c;
		     struct opvar *x1 = New(struct opvar);
		     struct opvar *y1 = New(struct opvar);
		     struct opvar *x2 = New(struct opvar);
		     struct opvar *y2 = New(struct opvar);
		     struct opvar *areatyp = New(struct opvar);
		     struct opvar *ter = New(struct opvar);
		     struct opvar *tlit = New(struct opvar);

		     set_opvar_int(areatyp, 0);
		     set_opvar_int(x1, current_coord.x);
		     set_opvar_int(y1, current_coord.y);
		     set_opvar_int(x2, -1);
		     set_opvar_int(y2, -1);
		     c = what_map_char((char) $6);
		     if (c >= MAX_TYPE) yyerror("Terrain: illegal map char");
		     set_opvar_int(ter, c);
		     set_opvar_int(tlit, $8);

		     add_opcode(&splev, SPO_PUSH, x1);
		     add_opcode(&splev, SPO_PUSH, y1);
		     add_opcode(&splev, SPO_PUSH, x2);
		     add_opcode(&splev, SPO_PUSH, y2);
		     add_opcode(&splev, SPO_PUSH, areatyp);
		     add_opcode(&splev, SPO_PUSH, ter);
		     add_opcode(&splev, SPO_PUSH, tlit);

		     add_opcode(&splev, SPO_TERRAIN, NULL);

		     if ( 1 == $2 ) {
			 if (n_if_list > 0) {
			     struct opvar *tmpjmp;
			     tmpjmp = (struct opvar *) if_list[--n_if_list];
			     set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			 } else yyerror("conditional terrain modification, but no jump point marker.");
		     }
		 }
	       |
	         TERRAIN_ID chance ':' coordinate ',' HORIZ_OR_VERT ',' INTEGER ',' CHAR ',' light_state
		 {
		     int c;
		     struct opvar *x1 = New(struct opvar);
		     struct opvar *y1 = New(struct opvar);
		     struct opvar *x2 = New(struct opvar);
		     struct opvar *y2 = New(struct opvar);
		     struct opvar *areatyp = New(struct opvar);
		     struct opvar *ter = New(struct opvar);
		     struct opvar *tlit = New(struct opvar);

		     c = $<i>6;
		     set_opvar_int(areatyp, c);
		     set_opvar_int(x1, current_coord.x);
		     set_opvar_int(y1, current_coord.y);
		     if (c == 1) {
			 set_opvar_int(x2, $8);
			 set_opvar_int(y2, -1);
		     } else {
			 set_opvar_int(x2, -1);
			 set_opvar_int(y2, $8);
		     }

		     c = what_map_char((char) $10);
		     if (c >= MAX_TYPE) yyerror("Terrain: illegal map char");
		     set_opvar_int(ter, c);
		     set_opvar_int(tlit, $12);

		     add_opcode(&splev, SPO_PUSH, x1);
		     add_opcode(&splev, SPO_PUSH, y1);
		     add_opcode(&splev, SPO_PUSH, x2);
		     add_opcode(&splev, SPO_PUSH, y2);
		     add_opcode(&splev, SPO_PUSH, areatyp);
		     add_opcode(&splev, SPO_PUSH, ter);
		     add_opcode(&splev, SPO_PUSH, tlit);

		     add_opcode(&splev, SPO_TERRAIN, NULL);

		     if ( 1 == $2 ) {
			 if (n_if_list > 0) {
			     struct opvar *tmpjmp;
			     tmpjmp = (struct opvar *) if_list[--n_if_list];
			     set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			 } else yyerror("conditional terrain modification, but no jump point marker.");
		     }
		 }
	       |
	         TERRAIN_ID chance ':' region ',' FILLING ',' CHAR ',' light_state
		 {
		     int c;
		     struct opvar *x1 = New(struct opvar);
		     struct opvar *y1 = New(struct opvar);
		     struct opvar *x2 = New(struct opvar);
		     struct opvar *y2 = New(struct opvar);
		     struct opvar *areatyp = New(struct opvar);
		     struct opvar *ter = New(struct opvar);
		     struct opvar *tlit = New(struct opvar);

		     set_opvar_int(areatyp, 3 + $<i>6 );
		     set_opvar_int(x1, current_region.x1);
		     set_opvar_int(y1, current_region.y1);
		     set_opvar_int(x2, current_region.x2);
		     set_opvar_int(y2, current_region.y2);

		     c = what_map_char((char) $8);
		     if (c >= MAX_TYPE) yyerror("Terrain: illegal map char");
		     set_opvar_int(ter, c);
		     set_opvar_int(tlit, $10);

		     add_opcode(&splev, SPO_PUSH, x1);
		     add_opcode(&splev, SPO_PUSH, y1);
		     add_opcode(&splev, SPO_PUSH, x2);
		     add_opcode(&splev, SPO_PUSH, y2);
		     add_opcode(&splev, SPO_PUSH, areatyp);
		     add_opcode(&splev, SPO_PUSH, ter);
		     add_opcode(&splev, SPO_PUSH, tlit);

		     add_opcode(&splev, SPO_TERRAIN, NULL);

		     if ( 1 == $2 ) {
			 if (n_if_list > 0) {
			     struct opvar *tmpjmp;
			     tmpjmp = (struct opvar *) if_list[--n_if_list];
			     set_opvar_int(tmpjmp, splev.init_lev.n_opcodes-1);
			 } else yyerror("conditional terrain modification, but no jump point marker.");
		     }
		 }
	       ;

randline_detail : RANDLINE_ID ':' lineends ',' CHAR ',' light_state ',' INTEGER opt_int
		  {
		      int c;
		     struct opvar *x1 = New(struct opvar);
		     struct opvar *y1 = New(struct opvar);
		     struct opvar *x2 = New(struct opvar);
		     struct opvar *y2 = New(struct opvar);
		     struct opvar *fg = New(struct opvar);
		     struct opvar *lit = New(struct opvar);
		     struct opvar *roughness = New(struct opvar);
		     struct opvar *thick = New(struct opvar);

		     set_opvar_int(x1, current_region.x1);
		     set_opvar_int(y1, current_region.y1);
		     set_opvar_int(x2, current_region.x2);
		     set_opvar_int(y2, current_region.y2);

		     c = what_map_char((char) $5);
		     if ((c == INVALID_TYPE) || (c >= MAX_TYPE)) yyerror("Terrain: illegal map char");
		     set_opvar_int(fg, c);
		     set_opvar_int(lit, $7);
		     set_opvar_int(roughness, $9);
		     set_opvar_int(thick, $<i>10);

		     add_opcode(&splev, SPO_PUSH, x1);
		     add_opcode(&splev, SPO_PUSH, y1);
		     add_opcode(&splev, SPO_PUSH, x2);
		     add_opcode(&splev, SPO_PUSH, y2);
		     add_opcode(&splev, SPO_PUSH, fg);
		     add_opcode(&splev, SPO_PUSH, lit);
		     add_opcode(&splev, SPO_PUSH, roughness);
		     add_opcode(&splev, SPO_PUSH, thick);

		     add_opcode(&splev, SPO_RANDLINE, NULL);
		  }

opt_int		: /* empty */
		  {
			$<i>$ = 0;
		  }
		| ',' INTEGER
		  {
			$<i>$ = $2;
		  }
		;

spill_detail : SPILL_ID ':' coordinate ',' CHAR ',' DIRECTION ',' INTEGER ',' light_state
		{
		    int c;
		    struct opvar *x = New(struct opvar);
		    struct opvar *y = New(struct opvar);
		    struct opvar *typ = New(struct opvar);
		    struct opvar *dir = New(struct opvar);
		    struct opvar *count = New(struct opvar);
		    struct opvar *lit = New(struct opvar);

		    set_opvar_int(x, current_coord.x);
		    set_opvar_int(y, current_coord.y);

		    c = what_map_char((char) $5);
		    if (c == INVALID_TYPE || c >= MAX_TYPE) {
			yyerror("SPILL: Invalid map character!");
		    }
		    set_opvar_int(typ, c);
		    set_opvar_int(dir, $7);
		    c = $9;
		    if (c < 1) yyerror("SPILL: Invalid count!");
		    set_opvar_int(count, c);
		    set_opvar_int(lit, $11);

		    add_opcode(&splev, SPO_PUSH, x);
		    add_opcode(&splev, SPO_PUSH, y);
		    add_opcode(&splev, SPO_PUSH, typ);
		    add_opcode(&splev, SPO_PUSH, dir);
		    add_opcode(&splev, SPO_PUSH, count);
		    add_opcode(&splev, SPO_PUSH, lit);

		    add_opcode(&splev, SPO_SPILL, NULL);
		}
		;

diggable_detail : NON_DIGGABLE_ID ':' region
		  {
		     struct opvar *x1 = New(struct opvar);
		     struct opvar *y1 = New(struct opvar);
		     struct opvar *x2 = New(struct opvar);
		     struct opvar *y2 = New(struct opvar);

		     set_opvar_int(x1, current_region.x1);
		     set_opvar_int(y1, current_region.y1);
		     set_opvar_int(x2, current_region.x2);
		     set_opvar_int(y2, current_region.y2);

		     add_opcode(&splev, SPO_PUSH, x1);
		     add_opcode(&splev, SPO_PUSH, y1);
		     add_opcode(&splev, SPO_PUSH, x2);
		     add_opcode(&splev, SPO_PUSH, y2);

		     add_opcode(&splev, SPO_NON_DIGGABLE, NULL);
		  }
		;

passwall_detail : NON_PASSWALL_ID ':' region
		  {
		     struct opvar *x1 = New(struct opvar);
		     struct opvar *y1 = New(struct opvar);
		     struct opvar *x2 = New(struct opvar);
		     struct opvar *y2 = New(struct opvar);

		     set_opvar_int(x1, current_region.x1);
		     set_opvar_int(y1, current_region.y1);
		     set_opvar_int(x2, current_region.x2);
		     set_opvar_int(y2, current_region.y2);

		     add_opcode(&splev, SPO_PUSH, x1);
		     add_opcode(&splev, SPO_PUSH, y1);
		     add_opcode(&splev, SPO_PUSH, x2);
		     add_opcode(&splev, SPO_PUSH, y2);

		     add_opcode(&splev, SPO_NON_PASSWALL, NULL);
		  }
		;

region_detail	: REGION_ID ':' region ',' light_state ',' room_type prefilled
		  {
		      int rt, irr;
		     struct opvar *x1 = New(struct opvar);
		     struct opvar *y1 = New(struct opvar);
		     struct opvar *x2 = New(struct opvar);
		     struct opvar *y2 = New(struct opvar);
		     struct opvar *rlit = New(struct opvar);
		     struct opvar *rtype = New(struct opvar);
		     struct opvar *rirreg = New(struct opvar);

		     set_opvar_int(x1, current_region.x1);
		     set_opvar_int(y1, current_region.y1);
		     set_opvar_int(x2, current_region.x2);
		     set_opvar_int(y2, current_region.y2);
		     set_opvar_int(rlit, $<i>5);
		     rt = $<i>7;
		     if (( $<i>8 ) & 1) rt += MAXRTYPE+1;
		     set_opvar_int(rtype, rt);
		     irr = ((( $<i>8 ) & 2) != 0);
		     set_opvar_int(rirreg, irr);

		     if(current_region.x1 > current_region.x2 ||
			current_region.y1 > current_region.y2)
		       yyerror("Region start > end!");

		     if (rt == VAULT &&	(irr ||
					 (current_region.x2 - current_region.x1 != 1) ||
					 (current_region.y2 - current_region.y1 != 1)))
			 yyerror("Vaults must be exactly 2x2!");

		     add_opcode(&splev, SPO_PUSH, x1);
		     add_opcode(&splev, SPO_PUSH, y1);
		     add_opcode(&splev, SPO_PUSH, x2);
		     add_opcode(&splev, SPO_PUSH, y2);
		     add_opcode(&splev, SPO_PUSH, rlit);
		     add_opcode(&splev, SPO_PUSH, rtype);
		     add_opcode(&splev, SPO_PUSH, rirreg);

		     add_opcode(&splev, SPO_REGION, NULL);
		  }
		;

altar_detail	: ALTAR_ID ':' coordinate ',' alignment ',' altar_type
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *align = New(struct opvar);
		      struct opvar *shrine = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(align, $<i>5);
		      set_opvar_int(shrine, $<i>7);

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, shrine);
		      add_opcode(&splev, SPO_PUSH, align);

		      add_opcode(&splev, SPO_ALTAR, NULL);
		  }
		;

gold_detail	: GOLD_ID ':' amount ',' coordinate
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *amount = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_int(amount, $<i>3);

		      add_opcode(&splev, SPO_PUSH, amount);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, x);

		      add_opcode(&splev, SPO_GOLD, NULL);
		  }
		;

engraving_detail: ENGRAVING_ID ':' coordinate ',' engraving_type ',' string
		  {
		      struct opvar *x = New(struct opvar);
		      struct opvar *y = New(struct opvar);
		      struct opvar *estr = New(struct opvar);
		      struct opvar *etype = New(struct opvar);

		      set_opvar_int(x, current_coord.x);
		      set_opvar_int(y, current_coord.y);
		      set_opvar_str(estr, $7);
		      set_opvar_int(etype, $<i>5);

		      add_opcode(&splev, SPO_PUSH, x);
		      add_opcode(&splev, SPO_PUSH, y);
		      add_opcode(&splev, SPO_PUSH, estr);
		      add_opcode(&splev, SPO_PUSH, etype);

		      add_opcode(&splev, SPO_ENGRAVING, NULL);
		  }
		;

monster_c	: monster
		| RANDOM_TYPE
		  {
			$<i>$ = - MAX_REGISTERS - 1;
		  }
		| m_register
		;

object_c	: object
		| RANDOM_TYPE
		  {
			$<i>$ = - MAX_REGISTERS - 1;
		  }
		| o_register
		;

m_name		: string
		| RANDOM_TYPE
		  {
			$$ = (char *) 0;
		  }
		;

o_name		: string
		| RANDOM_TYPE
		  {
			$$ = (char *) 0;
		  }
		;

trap_name	: string
		  {
			int token = get_trap_type($1);
			if (token == ERR)
				yyerror("Unknown trap type!");
			$<i>$ = token;
			Free($1);
		  }
		| RANDOM_TYPE
		;

room_type	: string
		  {
			int token = get_room_type($1);
			if (token == ERR) {
				yywarning("Unknown room type!  Making ordinary room...");
				$<i>$ = OROOM;
			} else
				$<i>$ = token;
			Free($1);
		  }
		| RANDOM_TYPE
		;

prefilled	: /* empty */
		  {
			$<i>$ = 0;
		  }
		| ',' FILLING
		  {
			$<i>$ = $2;
		  }
		| ',' FILLING ',' BOOLEAN
		  {
			$<i>$ = $2 + ($4 << 1);
		  }
		;

coordinate	: coord
		| p_register
		| RANDOM_TYPE
		  {
			current_coord.x = current_coord.y = -MAX_REGISTERS-1;
		  }
		;

door_state	: DOOR_STATE
		| RANDOM_TYPE
		;

light_state	: LIGHT_STATE
		| RANDOM_TYPE
		;

alignment	: ALIGNMENT
		| a_register
		| RANDOM_TYPE
		  {
			$<i>$ = - MAX_REGISTERS - 1;
		  }
		;

altar_type	: ALTAR_TYPE
		| RANDOM_TYPE
		;

p_register	: P_REGISTER '[' INTEGER ']'
		  {
		        if (on_plist == 0)
		                yyerror("No random places defined!");
			else if ( $3 >= on_plist )
				yyerror("Register Index overflow!");
			else
				current_coord.x = current_coord.y = - $3 - 1;
		  }
		;

o_register	: O_REGISTER '[' INTEGER ']'
		  {
		        if (on_olist == 0)
		                yyerror("No random objects defined!");
			else if ( $3 >= on_olist )
				yyerror("Register Index overflow!");
			else
				$<i>$ = - $3 - 1;
		  }
		;

m_register	: M_REGISTER '[' INTEGER ']'
		  {
		        if (on_mlist == 0)
		                yyerror("No random monsters defined!");
			if ( $3 >= on_mlist )
				yyerror("Register Index overflow!");
			else
				$<i>$ = - $3 - 1;
		  }
		;

a_register	: A_REGISTER '[' INTEGER ']'
		  {
			if ( $3 >= 3 )
				yyerror("Register Index overflow!");
			else
				$<i>$ = - $3 - 1;
		  }
		;

place		: coord
		;

monster		: CHAR
		  {
			if (check_monster_char((char) $1))
				$<i>$ = $1 ;
			else {
				yyerror("Unknown monster class!");
				$<i>$ = ERR;
			}
		  }
		;

object		: CHAR
		  {
			char c = $1;
			if (check_object_char(c))
				$<i>$ = c;
			else {
				yyerror("Unknown char class!");
				$<i>$ = ERR;
			}
		  }
		;

string		: STRING
		;

amount		: INTEGER
		| RANDOM_TYPE
		;

chance		: /* empty */
		  {
		      /* by default we just do it, unconditionally. */
		      $$ = 0;
		  }
		| comparestmt
		  {
		      /* otherwise we generate an IF-statement */
		      struct opvar *tmppush2 = New(struct opvar);
		      if (n_if_list >= MAX_NESTED_IFS) {
			  yyerror("IF: Too deeply nested IFs.");
			  n_if_list = MAX_NESTED_IFS - 1;
		      }
		      add_opcode(&splev, SPO_CMP, NULL);
		      set_opvar_int(tmppush2, -1);
		      if_list[n_if_list++] = tmppush2;
		      add_opcode(&splev, SPO_PUSH, tmppush2);
		      add_opcode(&splev, $1, NULL);
		      $$ = 1;
		  }
		;

engraving_type	: ENGRAVING_TYPE
		| RANDOM_TYPE
		;

coord		: '(' INTEGER ',' INTEGER ')'
		  {
		        if ($2 < 0 || $4 < 0 || $2 >= COLNO || $4 >= ROWNO)
		           yyerror("Coordinates out of map range!");
			current_coord.x = $2;
			current_coord.y = $4;
		  }
		;

lineends	: coordinate ','
		  {
			current_region.x1 = current_coord.x;
			current_region.y1 = current_coord.y;
		  }
		  coordinate
		  {
			current_region.x2 = current_coord.x;
			current_region.y2 = current_coord.y;
		  }
		;

lev_region	: region
		  {
			$$ = 0;
		  }
		| LEV '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'
		  {
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if ($3 <= 0 || $3 >= COLNO)
				yyerror("Region out of level range!");
			else if ($5 < 0 || $5 >= ROWNO)
				yyerror("Region out of level range!");
			else if ($7 <= 0 || $7 >= COLNO)
				yyerror("Region out of level range!");
			else if ($9 < 0 || $9 >= ROWNO)
				yyerror("Region out of level range!");
			current_region.x1 = $3;
			current_region.y1 = $5;
			current_region.x2 = $7;
			current_region.y2 = $9;
			$$ = 1;
		  }
		;

region		: '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'
		  {
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if ($2 < 0 || $2 > (int)max_x_map)
			  yyerror("Region out of map range (x1)!");
			else if ($4 < 0 || $4 > (int)max_y_map)
			  yyerror("Region out of map range (y1)!");
			else if ($6 < 0 || $6 > (int)max_x_map)
			  yyerror("Region out of map range (x2)!");
			else if ($8 < 0 || $8 > (int)max_y_map)
			  yyerror("Region out of map range (y2)!");
			current_region.x1 = $2;
			current_region.y1 = $4;
			current_region.x2 = $6;
			current_region.y2 = $8;
		  }
		;


%%

/*lev_comp.y*/
