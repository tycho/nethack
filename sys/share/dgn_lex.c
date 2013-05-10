/* A lexical scanner for NetHack generated by flex */

/* Scanner skeleton version:
 * flexhack.skl 3.3.0 (from .../flex/RCS/flex.skl,v 2.85 95/04/24 10:48:47)
 */
#define FLEXHACK_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 5

#include "config.h"
#define yyconst const	/* some code inserted by flex will refer to yyconst */

/* Returned upon end-of-file. */
#define YY_NULL 0

/* Promotes a possibly negative, possibly signed char to an unsigned
 * integer for use as an array index.  If the signed char is negative,
 * we want to instead treat it as an 8-bit unsigned char, hence the
 * double cast.
 */
#define YY_SC_TO_UI(c) ((unsigned int) (unsigned char) c)

/* Enter a start condition.  This macro really ought to take a parameter,
 * but we do it the disgusting crufty way forced on us by the ()-less
 * definition of BEGIN.
 */
#define BEGIN yy_start = 1 + 2 *

/* Translate the current start state into a value that can be later handed
 * to BEGIN to return to the state.  The YYSTATE alias is for lex
 * compatibility.
 */
#define YY_START ((yy_start - 1) / 2)
#define YYSTATE YY_START

/* Action number for EOF rule of a given start state. */
#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)

/* Special action meaning "start processing a new file". */
#define YY_NEW_FILE yyrestart( yyin )

#define YY_END_OF_BUFFER_CHAR 0

/* Size of default input buffer. */
#define YY_BUF_SIZE 16384

typedef struct yy_buffer_state *YY_BUFFER_STATE;

extern int yyleng;
extern FILE *yyin, *yyout;

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

/* Return all but the first 'n' matched characters back to the input stream. */
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
		*yy_cp = yy_hold_char; \
		yy_c_buf_p = yy_cp = yy_bp + n - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up yytext again */ \
		} \
	while ( 0 )

#define unput(c) yyunput( c, yytext_ptr )

/* The following is because we cannot portably get our hands on size_t
 * (without autoconf's help, which isn't available because we want
 * flex-generated scanners to compile on their own).
 */
typedef unsigned int yy_size_t;


struct yy_buffer_state
	{
	FILE *yy_input_file;

	char *yy_ch_buf;		/* input buffer */
	char *yy_buf_pos;		/* current position in input buffer */

	/* Size of input buffer in bytes, not including room for EOB
	 * characters.
	 */
	yy_size_t yy_buf_size;

	/* Number of characters read into yy_ch_buf, not including EOB
	 * characters.
	 */
	int yy_n_chars;

	/* Whether we "own" the buffer - i.e., we know we created it,
	 * and can realloc() it to grow it, and should free() it to
	 * delete it.
	 */
	int yy_is_our_buffer;

	/* Whether this is an "interactive" input source; if so, and
	 * if we're using stdio for input, then we want to use getc()
	 * instead of fread(), to make sure we stop fetching input after
	 * each newline.
	 */
	int yy_is_interactive;

	/* Whether we're considered to be at the beginning of a line.
	 * If so, '^' rules will be active on the next match, otherwise
	 * not.
	 */
	int yy_at_bol;

	/* Whether to try to fill the input buffer when we reach the
	 * end of it.
	 */
	int yy_fill_buffer;

	int yy_buffer_status;
#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
	/* When an EOF's been seen but there's still some text to process
	 * then we mark the buffer as YY_EOF_PENDING, to indicate that we
	 * shouldn't try reading from the input source any more.  We might
	 * still have a bunch of tokens to match, though, because of
	 * possible backing-up.
	 *
	 * When we actually see the EOF, we change the status to "new"
	 * (via yyrestart()), so that the user can continue scanning by
	 * just pointing yyin at a new input file.
	 */
#define YY_BUFFER_EOF_PENDING 2
	};

static YY_BUFFER_STATE yy_current_buffer = 0;

/* We provide macros for accessing buffer states in case in the
 * future we want to put the buffer states in a more general
 * "scanner state".
 */
#define YY_CURRENT_BUFFER yy_current_buffer

/* yy_hold_char holds the character lost when yytext is formed. */
static char yy_hold_char;

static int yy_n_chars;		/* number of characters read into yy_ch_buf */

int yyleng;

/* Points to current character in buffer. */
static char *yy_c_buf_p = (char *) 0;
static int yy_init = 1;		/* whether we need to initialize */
static int yy_start = 0;	/* start state number */

/* Flag which is used to allow yywrap()'s to do buffer switches
 * instead of setting up a fresh yyin.  A bit of a hack ...
 */
static int yy_did_buffer_switch_on_eof;

void FDECL(yyrestart, (FILE *));

void FDECL(yy_switch_to_buffer, (YY_BUFFER_STATE));
void NDECL(yy_load_buffer_state);
YY_BUFFER_STATE FDECL(yy_create_buffer, (FILE *,int));
void FDECL(yy_delete_buffer, (YY_BUFFER_STATE));
void FDECL(yy_init_buffer, (YY_BUFFER_STATE,FILE *));
void FDECL(yy_flush_buffer, (YY_BUFFER_STATE));
#define YY_FLUSH_BUFFER yy_flush_buffer( yy_current_buffer )

static genericptr_t FDECL(yy_flex_alloc, (yy_size_t));
static genericptr_t FDECL(yy_flex_realloc2, (genericptr_t,yy_size_t,int));
static void FDECL(yy_flex_free, (genericptr_t));

#define yy_new_buffer yy_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! yy_current_buffer ) \
		yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE ); \
	yy_current_buffer->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! yy_current_buffer ) \
		yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE ); \
	yy_current_buffer->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (yy_current_buffer->yy_at_bol)

typedef unsigned char YY_CHAR;
FILE *yyin = (FILE *) 0, *yyout = (FILE *) 0;
typedef int yy_state_type;
extern char *yytext;
#define yytext_ptr yytext

static yy_state_type NDECL(yy_get_previous_state);
static yy_state_type FDECL(yy_try_NUL_trans, (yy_state_type));
static int NDECL(yy_get_next_buffer);
static void FDECL(yy_fatal_error, (const char *));

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up yytext.
 */
#define YY_DO_BEFORE_ACTION \
	yytext_ptr = yy_bp; \
	yyleng = (int) (yy_cp - yy_bp); \
	yy_hold_char = *yy_cp; \
	*yy_cp = '\0'; \
	yy_c_buf_p = yy_cp;

#define YY_NUM_RULES 35
#define YY_END_OF_BUFFER 36
static yyconst short int yy_accept[196] =
    {   0,
        0,    0,   36,   34,   33,   32,   34,   34,   29,   34,
       34,   34,   34,   34,   34,   34,   34,   34,   34,   34,
       34,   34,   34,   34,   34,   34,   34,   34,   34,   33,
       32,    0,   30,   29,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    2,    0,   31,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    3,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,   14,    0,    0,    0,    0,    0,    0,
        4,    0,   25,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    6,    0,    0,    0,    5,    0,    0,   23,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,   20,    0,    0,    0,    0,    8,    0,    0,    0,
        0,    0,    0,    1,    0,    0,    0,    0,    0,   22,
       15,    0,   21,    7,   19,    0,    0,    0,    0,    0,
        0,   13,    0,    0,    0,   26,   16,    0,    0,   12,
        0,    0,    0,   11,    9,    0,   17,   18,    0,   27,
        0,   28,   24,   10,    0

    } ;

static yyconst int yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        1,    1,    4,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    1,    5,    6,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    7,    1,    1,    7,    7,    7,
        7,    7,    7,    7,    7,    7,    7,    1,    1,    1,
        1,    1,    1,    1,    8,    9,   10,   11,   12,   13,
       14,   15,   16,    1,    1,   17,   18,   19,   20,   21,
        1,   22,   23,   24,   25,   26,    1,    1,   27,    1,
        1,    1,    1,    1,   28,    1,   29,    1,   30,   31,

       32,   33,   34,   35,   36,    1,   37,   38,   39,   40,
       41,   42,    1,   43,   44,   45,   46,    1,   47,    1,
        1,   48,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,

        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1
    } ;

static yyconst int yy_meta[49] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1
    } ;

static yyconst short int yy_base[198] =
    {   0,
        0,  213,  218,  220,  215,  220,  213,  210,  207,  196,
      190,  196,   37,  191,  197,  186,  188,  171,  164,  172,
      174,  173,   18,  160,  159,  154,  157,   11,  194,  194,
      220,  190,  220,  187,  177,  184,  183,  167,  170,  164,
      161,  166,  174,  155,  136,  144,  134,  132,  133,   26,
      135,  143,  147,  128,  145,  220,  170,  220,  158,  152,
      154,  159,  154,  145,   44,  142,   47,  124,  124,  125,
      129,  129,  115,   27,  121,  113,  111,  120,  115,  116,
      134,  142,  132,  128,  137,  121,  130,  129,  125,  129,
      131,   97,  220,  105,   94,  101,   95,   96,   94,   99,

      105,  101,   89,  220,   95,  112,  114,   51,  112,  107,
      220,  110,  114,  111,  106,   96,   85,   76,   81,   82,
       88,   69,  220,   81,   76,   75,  220,   78,   99,  220,
       88,   97,   87,   88,   92,   93,   88,   91,   90,   71,
       65,  220,   62,   60,   57,   56,  220,   59,   54,   74,
       84,   65,   66,  220,   70,   65,   70,   60,   68,  220,
      220,   52,  220,  220,  220,   46,   50,   57,   61,   67,
       62,  220,   67,   64,   63,  220,  220,   42,   41,  220,
       61,   53,   49,  220,  220,   50,  220,  220,   51,  220,
       46,  220,  220,  220,  220,   62,   60

    } ;

static yyconst short int yy_def[198] =
    {   0,
      195,    1,  195,  195,  195,  195,  195,  196,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  197,  195,
      195,  196,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  197,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,

      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,    0,  195,  195

    } ;

static yyconst short int yy_nxt[269] =
    {   0,
        4,    5,    6,    7,    8,    4,    9,   10,   11,   12,
       13,   14,    4,    4,    4,    4,   15,    4,    4,    4,
       16,   17,    4,    4,    4,    4,    4,    4,    4,   18,
       19,    4,    4,    4,   20,    4,    4,   21,   22,   23,
        4,   24,   25,   26,   27,   28,    4,    4,   38,   49,
       55,   87,   56,   74,   75,   88,   90,   98,   50,  131,
       57,   39,   32,   91,  194,  193,  192,  132,  191,  190,
      189,  188,   99,  187,  186,  185,  184,  183,  182,  181,
      180,  179,  178,  177,  176,  175,  174,  173,  172,  171,
      170,  169,  168,  167,  166,  165,  164,  163,  162,  161,

      160,  159,  158,  157,  156,  155,  154,  153,  152,  151,
      150,  149,  148,  147,  146,  145,  144,  143,  142,  141,
      140,  139,  138,  137,  136,  135,  134,  133,  130,  129,
      128,  127,  126,  125,  124,  123,  122,  121,  120,  119,
      118,  117,  116,  115,  114,  113,  112,  111,  110,  109,
      108,  107,  106,  105,  104,  103,  102,  101,  100,   97,
       96,   95,   94,   93,   92,   89,   86,   85,   84,   83,
       82,   81,   58,   80,   79,   78,   77,   76,   73,   72,
       71,   70,   69,   68,   67,   66,   65,   64,   63,   62,
       61,   60,   59,   34,   33,   30,   58,   54,   53,   52,

       51,   48,   47,   46,   45,   44,   43,   42,   41,   40,
       37,   36,   35,   34,   33,   31,   30,  195,   29,    3,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195
    } ;

static yyconst short int yy_chk[269] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,   13,   23,
       28,   65,   28,   50,   50,   65,   67,   74,   23,  108,
      197,   13,  196,   67,  191,  189,  186,  108,  183,  182,
      181,  179,   74,  178,  175,  174,  173,  171,  170,  169,
      168,  167,  166,  162,  159,  158,  157,  156,  155,  153,
      152,  151,  150,  149,  148,  146,  145,  144,  143,  141,

      140,  139,  138,  137,  136,  135,  134,  133,  132,  131,
      129,  128,  126,  125,  124,  122,  121,  120,  119,  118,
      117,  116,  115,  114,  113,  112,  110,  109,  107,  106,
      105,  103,  102,  101,  100,   99,   98,   97,   96,   95,
       94,   92,   91,   90,   89,   88,   87,   86,   85,   84,
       83,   82,   81,   80,   79,   78,   77,   76,   75,   73,
       72,   71,   70,   69,   68,   66,   64,   63,   62,   61,
       60,   59,   57,   55,   54,   53,   52,   51,   49,   48,
       47,   46,   45,   44,   43,   42,   41,   40,   39,   38,
       37,   36,   35,   34,   32,   30,   29,   27,   26,   25,

       24,   22,   21,   20,   19,   18,   17,   16,   15,   14,
       12,   11,   10,    9,    8,    7,    5,    3,    2,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195,  195,  195,
      195,  195,  195,  195,  195,  195,  195,  195
    } ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
char *yytext;
#define INITIAL 0
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/*	Copyright (c) 1990 by M. Stephenson	     */
/* NetHack may be freely redistributed.  See license for details. */

#define DGN_COMP

#include "config.h"
#include "dgn_comp.h"
#include "dgn_file.h"

/*
 * Most of these don't exist in flex, yywrap is macro and
 * yyunput is properly declared in flex.skel.
 */
#if !defined(FLEX_SCANNER) && !defined(FLEXHACK_SCANNER)
int FDECL(yyback, (int *,int));
int NDECL(yylook);
int NDECL(yyinput);
int NDECL(yywrap);
int NDECL(yylex);
	/* Traditional lexes let yyunput() and yyoutput() default to int;
	 * newer ones may declare them as void since they don't return
	 * values.  For even more fun, the lex supplied as part of the
	 * newer unbundled compiler for SunOS 4.x adds the void declarations
	 * (under __STDC__ or _cplusplus ifdefs -- otherwise they remain
	 * int) while the bundled lex and the one with the older unbundled
	 * compiler do not.  To detect this, we need help from outside --
	 * sys/unix/Makefile.utl.
	 *
	 * Digital UNIX is difficult and still has int in spite of all
	 * other signs.
	 */
# if defined(NeXT) || defined(SVR4) || defined(_AIX32)
#  define VOIDYYPUT
# endif
# if !defined(VOIDYYPUT) && defined(POSIX_TYPES)
#  if !defined(BOS) && !defined(HISX) && !defined(_M_UNIX) && !defined(VMS)
#   define VOIDYYPUT
#  endif
# endif
# if !defined(VOIDYYPUT) && defined(WEIRD_LEX)
#  if defined(SUNOS4) && defined(__STDC__) && (WEIRD_LEX > 1)
#   define VOIDYYPUT
#  endif
# endif
# if defined(VOIDYYPUT) && defined(__osf__)
#  undef VOIDYYPUT
# endif
# ifdef VOIDYYPUT
void FDECL(yyunput, (int));
void FDECL(yyoutput, (int));
# else
int FDECL(yyunput, (int));
int FDECL(yyoutput, (int));
# endif
#endif	/* !FLEX_SCANNER && !FLEXHACK_SCANNER */

#ifdef FLEX_SCANNER
#define YY_MALLOC_DECL \
	       genericptr_t FDECL(malloc, (size_t)); \
	       genericptr_t FDECL(realloc, (genericptr_t,size_t));
#endif


void FDECL(init_yyin, (FILE *));
void FDECL(init_yyout, (FILE *));

/* this doesn't always get put in dgn_comp.h
 * (esp. when using older versions of bison)
 */

extern YYSTYPE yylval;

int line_number = 1;


/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
extern int NDECL(yywrap);
#endif

#ifndef YY_NO_UNPUT
static void FDECL(yyunput, (int,char *));
#endif

#ifndef yytext_ptr
static void FDECL(yy_flex_strncpy, (char *,const char *,int));
#endif

#ifndef YY_NO_INPUT
static int NDECL(input);
#endif

/* Amount of stuff to slurp up with each read. */
#ifndef YY_READ_BUF_SIZE
#define YY_READ_BUF_SIZE 8192
#endif

/* Copy whatever the last rule matched to the standard output. */

#ifndef ECHO
/* This used to be an fputs(), but since the string might contain NUL's,
 * we now use fwrite().
 */
#define ECHO (void) fwrite( yytext, yyleng, 1, yyout )
#endif

/* Gets input and stuffs it into "buf".  number of characters read, or YY_NULL,
 * is returned in "result".
 */
#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( yy_current_buffer->yy_is_interactive ) \
		{ \
		int c = '*', n; \
		for ( n = 0; n < max_size && \
			     (c = getc( yyin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( yyin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else if ( ((result = fread( buf, 1, max_size, yyin )) == 0) \
		  && ferror( yyin ) ) \
		YY_FATAL_ERROR( "input in flex scanner failed" );
#endif

/* No semi-colon after return; correct usage is to write "yyterminate();" -
 * we don't want an extra ';' after the "return" because that will cause
 * some compilers to complain about unreachable statements.
 */
#ifndef yyterminate
#define yyterminate() return YY_NULL
#endif

/* Number of entries by which start-condition stack grows. */
#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif

/* Report a fatal error. */
#ifndef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) yy_fatal_error( msg )
#endif

/* Code executed at the beginning of each rule, after yytext and yyleng
 * have been set up.
 */
#ifndef YY_USER_ACTION
#define YY_USER_ACTION
#endif

/* Code executed at the end of each rule. */
#ifndef YY_BREAK
#define YY_BREAK break;
#endif

#define YY_RULE_SETUP \
	if ( yyleng > 0 ) \
		yy_current_buffer->yy_at_bol = \
				(yytext[yyleng - 1] == '\n'); \
	YY_USER_ACTION

int NDECL(yylex);
int yylex()
	{
	yy_state_type yy_current_state;
	char *yy_cp, *yy_bp;
	int yy_act;



	if ( yy_init )
		{
		yy_init = 0;

#ifdef YY_USER_INIT
		YY_USER_INIT;
#endif

		if ( ! yy_start )
			yy_start = 1;	/* first start state */

		if ( ! yyin )
			yyin = stdin;

		if ( ! yyout )
			yyout = stdout;

		if ( ! yy_current_buffer )
			yy_current_buffer =
				yy_create_buffer( yyin, YY_BUF_SIZE );

		yy_load_buffer_state();
		}

	while ( 1 )		/* loops until end-of-file is reached */
		{
		yy_cp = yy_c_buf_p;

		/* Support of yytext. */
		*yy_cp = yy_hold_char;

		/* yy_bp points to the position in yy_ch_buf of the start of
		 * the current run.
		 */
		yy_bp = yy_cp;

		yy_current_state = yy_start;
		yy_current_state += YY_AT_BOL();
yy_match:
		do
			{
			YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)];
			if ( yy_accept[yy_current_state] )
				{
				yy_last_accepting_state = yy_current_state;
				yy_last_accepting_cpos = yy_cp;
				}
			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
				{
				yy_current_state = (int) yy_def[yy_current_state];
				if ( yy_current_state >= 196 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 220 );

yy_find_action:
		yy_act = yy_accept[yy_current_state];
		if ( yy_act == 0 )
			{ /* have to back up */
			yy_cp = yy_last_accepting_cpos;
			yy_current_state = yy_last_accepting_state;
			yy_act = yy_accept[yy_current_state];
			}

		YY_DO_BEFORE_ACTION;


do_action:	/* This label is used only to access EOF actions. */


		switch ( yy_act )
	{ /* beginning of action switch */
			case 0: /* must back up */
			/* undo the effects of YY_DO_BEFORE_ACTION */
			*yy_cp = yy_hold_char;
			yy_cp = yy_last_accepting_cpos;
			yy_current_state = yy_last_accepting_state;
			goto yy_find_action;

case 1:
YY_RULE_SETUP
return(A_DUNGEON);
	YY_BREAK
case 2:
YY_RULE_SETUP
{ yylval.i=1; return(UP_OR_DOWN); }
	YY_BREAK
case 3:
YY_RULE_SETUP
{ yylval.i=0; return(UP_OR_DOWN); }
	YY_BREAK
case 4:
YY_RULE_SETUP
return(ENTRY);
	YY_BREAK
case 5:
YY_RULE_SETUP
return(STAIR);
	YY_BREAK
case 6:
YY_RULE_SETUP
return(NO_UP);
	YY_BREAK
case 7:
YY_RULE_SETUP
return(NO_DOWN);
	YY_BREAK
case 8:
YY_RULE_SETUP
return(PORTAL);
	YY_BREAK
case 9:
YY_RULE_SETUP
return(PROTOFILE);
	YY_BREAK
case 10:
YY_RULE_SETUP
return(DESCRIPTION);
	YY_BREAK
case 11:
YY_RULE_SETUP
return(LEVELDESC);
	YY_BREAK
case 12:
YY_RULE_SETUP
return(ALIGNMENT);
	YY_BREAK
case 13:
YY_RULE_SETUP
return(LEVALIGN);
	YY_BREAK
case 14:
YY_RULE_SETUP
{ yylval.i=TOWN ; return(DESCRIPTOR); }
	YY_BREAK
case 15:
YY_RULE_SETUP
{ yylval.i=HELLISH ; return(DESCRIPTOR); }
	YY_BREAK
case 16:
YY_RULE_SETUP
{ yylval.i=MAZELIKE ; return(DESCRIPTOR); }
	YY_BREAK
case 17:
YY_RULE_SETUP
{ yylval.i=ROGUELIKE ; return(DESCRIPTOR); }
	YY_BREAK
case 18:
YY_RULE_SETUP
{ yylval.i=D_ALIGN_NONE ; return(DESCRIPTOR); }
	YY_BREAK
case 19:
YY_RULE_SETUP
{ yylval.i=D_ALIGN_NONE ; return(DESCRIPTOR); }
	YY_BREAK
case 20:
YY_RULE_SETUP
{ yylval.i=D_ALIGN_LAWFUL ; return(DESCRIPTOR); }
	YY_BREAK
case 21:
YY_RULE_SETUP
{ yylval.i=D_ALIGN_NEUTRAL ; return(DESCRIPTOR); }
	YY_BREAK
case 22:
YY_RULE_SETUP
{ yylval.i=D_ALIGN_CHAOTIC ; return(DESCRIPTOR); }
	YY_BREAK
case 23:
YY_RULE_SETUP
return(BRANCH);
	YY_BREAK
case 24:
YY_RULE_SETUP
return(CHBRANCH);
	YY_BREAK
case 25:
YY_RULE_SETUP
return(LEVEL);
	YY_BREAK
case 26:
YY_RULE_SETUP
return(RNDLEVEL);
	YY_BREAK
case 27:
YY_RULE_SETUP
return(CHLEVEL);
	YY_BREAK
case 28:
YY_RULE_SETUP
return(RNDCHLEVEL);
	YY_BREAK
case 29:
YY_RULE_SETUP
{ yylval.i=atoi(yytext); return(INTEGER); }
	YY_BREAK
case 30:
YY_RULE_SETUP
{ yytext[yyleng-1] = 0; /* Discard the trailing \" */
		  yylval.str = (char *) alloc(strlen(yytext+1)+1);
		  Strcpy(yylval.str, yytext+1); /* Discard the first \" */
		  return(STRING); }
	YY_BREAK
case 31:
YY_RULE_SETUP
{ line_number++; }
	YY_BREAK
case 32:
YY_RULE_SETUP
{ line_number++; }
	YY_BREAK
case 33:
YY_RULE_SETUP
;	/* skip trailing tabs & spaces */
	YY_BREAK
case 34:
YY_RULE_SETUP
{ return yytext[0]; }
	YY_BREAK
case 35:
YY_RULE_SETUP
ECHO;
	YY_BREAK
case YY_STATE_EOF(INITIAL):
	yyterminate();

	case YY_END_OF_BUFFER:
		{
		/* Amount of text matched not including the EOB char. */
		int yy_amount_of_matched_text = (int) (yy_cp - yytext_ptr) - 1;

		/* Undo the effects of YY_DO_BEFORE_ACTION. */
		*yy_cp = yy_hold_char;

		if ( yy_current_buffer->yy_buffer_status == YY_BUFFER_NEW )
			{
			/* We're scanning a new file or input source.  It's
			 * possible that this happened because the user
			 * just pointed yyin at a new source and called
			 * yylex().  If so, then we have to assure
			 * consistency between yy_current_buffer and our
			 * globals.  Here is the right place to do so, because
			 * this is the first action (other than possibly a
			 * back-up) that will match for the new input source.
			 */
			yy_n_chars = yy_current_buffer->yy_n_chars;
			yy_current_buffer->yy_input_file = yyin;
			yy_current_buffer->yy_buffer_status = YY_BUFFER_NORMAL;
			}

		/* Note that here we test for yy_c_buf_p "<=" to the position
		 * of the first EOB in the buffer, since yy_c_buf_p will
		 * already have been incremented past the NUL character
		 * (since all states make transitions on EOB to the
		 * end-of-buffer state).  Contrast this with the test
		 * in input().
		 */
		if ( yy_c_buf_p <= &yy_current_buffer->yy_ch_buf[yy_n_chars] )
			{ /* This was really a NUL. */
			yy_state_type yy_next_state;

			yy_c_buf_p = yytext_ptr + yy_amount_of_matched_text;

			yy_current_state = yy_get_previous_state();

			/* Okay, we're now positioned to make the NUL
			 * transition.  We couldn't have
			 * yy_get_previous_state() go ahead and do it
			 * for us because it doesn't know how to deal
			 * with the possibility of jamming (and we don't
			 * want to build jamming into it because then it
			 * will run more slowly).
			 */

			yy_next_state = yy_try_NUL_trans( yy_current_state );

			yy_bp = yytext_ptr + YY_MORE_ADJ;

			if ( yy_next_state )
				{
				/* Consume the NUL. */
				yy_cp = ++yy_c_buf_p;
				yy_current_state = yy_next_state;
				goto yy_match;
				}

			else
				{
				yy_cp = yy_c_buf_p;
				goto yy_find_action;
				}
			}

		else switch ( yy_get_next_buffer() )
			{
			case EOB_ACT_END_OF_FILE:
				{
				yy_did_buffer_switch_on_eof = 0;

				if ( yywrap() )
					{
					/* Note: because we've taken care in
					 * yy_get_next_buffer() to have set up
					 * yytext, we can now set up
					 * yy_c_buf_p so that if some total
					 * hoser (like flex itself) wants to
					 * call the scanner after we return the
					 * YY_NULL, it'll still work - another
					 * YY_NULL will get returned.
					 */
					yy_c_buf_p = yytext_ptr + YY_MORE_ADJ;

					yy_act = YY_STATE_EOF(YY_START);
					goto do_action;
					}

				else
					{
					if ( ! yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
					}
				break;
				}

			case EOB_ACT_CONTINUE_SCAN:
				yy_c_buf_p =
					yytext_ptr + yy_amount_of_matched_text;

				yy_current_state = yy_get_previous_state();

				yy_cp = yy_c_buf_p;
				yy_bp = yytext_ptr + YY_MORE_ADJ;
				goto yy_match;

			case EOB_ACT_LAST_MATCH:
				yy_c_buf_p =
				&yy_current_buffer->yy_ch_buf[yy_n_chars];

				yy_current_state = yy_get_previous_state();

				yy_cp = yy_c_buf_p;
				yy_bp = yytext_ptr + YY_MORE_ADJ;
				goto yy_find_action;
			}
		break;
		}

	default:
		YY_FATAL_ERROR(
			"fatal flex scanner internal error--no action found" );
	} /* end of action switch */
		} /* end of scanning one token */
	} /* end of yylex */


/* yy_get_next_buffer - try to read in a new buffer
 *
 * Returns a code representing an action:
 *	EOB_ACT_LAST_MATCH -
 *	EOB_ACT_CONTINUE_SCAN - continue scanning from current position
 *	EOB_ACT_END_OF_FILE - end of file
 */

static int yy_get_next_buffer()
	{
	char *dest = yy_current_buffer->yy_ch_buf;
	char *source = yytext_ptr;
	int number_to_move, i;
	int ret_val;

	if ( yy_c_buf_p > &yy_current_buffer->yy_ch_buf[yy_n_chars + 1] )
		YY_FATAL_ERROR(
		"fatal flex scanner internal error--end of buffer missed" );

	if ( yy_current_buffer->yy_fill_buffer == 0 )
		{ /* Don't try to fill the buffer, so this is an EOF. */
		if ( yy_c_buf_p - yytext_ptr - YY_MORE_ADJ == 1 )
			{
			/* We matched a singled characater, the EOB, so
			 * treat this as a final EOF.
			 */
			return EOB_ACT_END_OF_FILE;
			}

		else
			{
			/* We matched some text prior to the EOB, first
			 * process it.
			 */
			return EOB_ACT_LAST_MATCH;
			}
		}

	/* Try to read more data. */

	/* First move last chars to start of buffer. */
	number_to_move = (int) (yy_c_buf_p - yytext_ptr) - 1;

	for ( i = 0; i < number_to_move; ++i )
		*(dest++) = *(source++);

	if ( yy_current_buffer->yy_buffer_status == YY_BUFFER_EOF_PENDING )
		/* don't do the read, it's not guaranteed to return an EOF,
		 * just force an EOF
		 */
		yy_n_chars = 0;

	else
		{
		int num_to_read =
			yy_current_buffer->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ /* Not enough room in the buffer - grow it. */
#ifdef YY_USES_REJECT
			YY_FATAL_ERROR(
"input buffer overflow, can't enlarge buffer because scanner uses REJECT" );
#else

			/* just a shorter name for the current buffer */
			YY_BUFFER_STATE b = yy_current_buffer;

			int yy_c_buf_p_offset =
				(int) (yy_c_buf_p - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				int old_size = b->yy_buf_size + 2;
				int new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					/* Include room in for 2 EOB chars. */
					yy_flex_realloc2( (genericptr_t) b->yy_ch_buf,
							 b->yy_buf_size + 2, old_size );
				}
			else
				/* Can't grow it, we don't own it. */
				b->yy_ch_buf = 0;

			if ( ! b->yy_ch_buf )
				YY_FATAL_ERROR(
				"fatal error - scanner input buffer overflow" );

			yy_c_buf_p = &b->yy_ch_buf[yy_c_buf_p_offset];

			num_to_read = yy_current_buffer->yy_buf_size -
						number_to_move - 1;
#endif
			}

		if ( num_to_read > YY_READ_BUF_SIZE )
			num_to_read = YY_READ_BUF_SIZE;

		/* Read in more data. */
		YY_INPUT( (&yy_current_buffer->yy_ch_buf[number_to_move]),
			yy_n_chars, num_to_read );
		}

	if ( yy_n_chars == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			yyrestart( yyin );
			}

		else
			{
			ret_val = EOB_ACT_LAST_MATCH;
			yy_current_buffer->yy_buffer_status =
				YY_BUFFER_EOF_PENDING;
			}
		}

	else
		ret_val = EOB_ACT_CONTINUE_SCAN;

	yy_n_chars += number_to_move;
	yy_current_buffer->yy_ch_buf[yy_n_chars] = YY_END_OF_BUFFER_CHAR;
	yy_current_buffer->yy_ch_buf[yy_n_chars + 1] = YY_END_OF_BUFFER_CHAR;

	yytext_ptr = &yy_current_buffer->yy_ch_buf[0];

	return ret_val;
	}


/* yy_get_previous_state - get the state just before the EOB char was reached */

static yy_state_type yy_get_previous_state()
	{
	yy_state_type yy_current_state;
	char *yy_cp;

	yy_current_state = yy_start;
	yy_current_state += YY_AT_BOL();

	for ( yy_cp = yytext_ptr + YY_MORE_ADJ; yy_cp < yy_c_buf_p; ++yy_cp )
		{
		YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 1);
		if ( yy_accept[yy_current_state] )
			{
			yy_last_accepting_state = yy_current_state;
			yy_last_accepting_cpos = yy_cp;
			}
		while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
			{
			yy_current_state = (int) yy_def[yy_current_state];
			if ( yy_current_state >= 196 )
				yy_c = yy_meta[(unsigned int) yy_c];
			}
		yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
		}

	return yy_current_state;
	}


/* yy_try_NUL_trans - try to make a transition on the NUL character
 *
 * synopsis
 *	next_state = yy_try_NUL_trans( current_state );
 */

static yy_state_type yy_try_NUL_trans( yy_current_state )
yy_state_type yy_current_state;
	{
	int yy_is_jam;
	char *yy_cp = yy_c_buf_p;

	YY_CHAR yy_c = 1;
	if ( yy_accept[yy_current_state] )
		{
		yy_last_accepting_state = yy_current_state;
		yy_last_accepting_cpos = yy_cp;
		}
	while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
		{
		yy_current_state = (int) yy_def[yy_current_state];
		if ( yy_current_state >= 196 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 195);

	return yy_is_jam ? 0 : yy_current_state;
	}


#ifndef YY_NO_UNPUT
static void yyunput( c, yy_bp )
int c;
char *yy_bp;
	{
	char *yy_cp = yy_c_buf_p;

	/* undo effects of setting up yytext */
	*yy_cp = yy_hold_char;

	if ( yy_cp < yy_current_buffer->yy_ch_buf + 2 )
		{ /* need to shift things up to make room */
		/* +2 for EOB chars. */
		int number_to_move = yy_n_chars + 2;
		char *dest = &yy_current_buffer->yy_ch_buf[
					yy_current_buffer->yy_buf_size + 2];
		char *source =
				&yy_current_buffer->yy_ch_buf[number_to_move];

		while ( source > yy_current_buffer->yy_ch_buf )
			*--dest = *--source;

		yy_cp += (int) (dest - source);
		yy_bp += (int) (dest - source);
		yy_n_chars = yy_current_buffer->yy_buf_size;

		if ( yy_cp < yy_current_buffer->yy_ch_buf + 2 )
			YY_FATAL_ERROR( "flex scanner push-back overflow" );
		}

	*--yy_cp = (char) c;


	yytext_ptr = yy_bp;
	yy_hold_char = *yy_cp;
	yy_c_buf_p = yy_cp;
	}
#endif	/* ifndef YY_NO_UNPUT */


static int input()
	{
	int c;

	*yy_c_buf_p = yy_hold_char;

	if ( *yy_c_buf_p == YY_END_OF_BUFFER_CHAR )
		{
		/* yy_c_buf_p now points to the character we want to return.
		 * If this occurs *before* the EOB characters, then it's a
		 * valid NUL; if not, then we've hit the end of the buffer.
		 */
		if ( yy_c_buf_p < &yy_current_buffer->yy_ch_buf[yy_n_chars] )
			/* This was really a NUL. */
			*yy_c_buf_p = '\0';

		else
			{ /* need more input */
			yytext_ptr = yy_c_buf_p;
			++yy_c_buf_p;

			switch ( yy_get_next_buffer() )
				{
				case EOB_ACT_END_OF_FILE:
					{
					if ( yywrap() )
						{
						yy_c_buf_p =
						yytext_ptr + YY_MORE_ADJ;
						return EOF;
						}

					if ( ! yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
					return input();
					}

				case EOB_ACT_CONTINUE_SCAN:
					yy_c_buf_p = yytext_ptr + YY_MORE_ADJ;
					break;

				case EOB_ACT_LAST_MATCH:
					YY_FATAL_ERROR(
					"unexpected last match in input()" );
				}
			}
		}

	c = *(unsigned char *) yy_c_buf_p;	/* cast for 8-bit char's */
	*yy_c_buf_p = '\0';	/* preserve yytext */
	yy_hold_char = *++yy_c_buf_p;

	yy_current_buffer->yy_at_bol = (c == '\n');

	return c;
	}


void yyrestart( input_file )
FILE *input_file;
	{
	if ( ! yy_current_buffer )
		yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE );

	yy_init_buffer( yy_current_buffer, input_file );
	yy_load_buffer_state();
	}


void yy_switch_to_buffer( new_buffer )
YY_BUFFER_STATE new_buffer;
	{
	if ( yy_current_buffer == new_buffer )
		return;

	if ( yy_current_buffer )
		{
		/* Flush out information for old buffer. */
		*yy_c_buf_p = yy_hold_char;
		yy_current_buffer->yy_buf_pos = yy_c_buf_p;
		yy_current_buffer->yy_n_chars = yy_n_chars;
		}

	yy_current_buffer = new_buffer;
	yy_load_buffer_state();

	/* We don't actually know whether we did this switch during
	 * EOF (yywrap()) processing, but the only time this flag
	 * is looked at is after yywrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	yy_did_buffer_switch_on_eof = 1;
	}


void yy_load_buffer_state()
	{
	yy_n_chars = yy_current_buffer->yy_n_chars;
	yytext_ptr = yy_c_buf_p = yy_current_buffer->yy_buf_pos;
	yyin = yy_current_buffer->yy_input_file;
	yy_hold_char = *yy_c_buf_p;
	}


YY_BUFFER_STATE yy_create_buffer( file, size )
FILE *file;
int size;
	{
	YY_BUFFER_STATE b;

	b = (YY_BUFFER_STATE) yy_flex_alloc( sizeof( struct yy_buffer_state ) );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_buf_size = size;

	/* yy_ch_buf has to be 2 characters longer than the size given because
	 * we need to put in 2 end-of-buffer characters.
	 */
	b->yy_ch_buf = (char *) yy_flex_alloc( b->yy_buf_size + 2 );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_is_our_buffer = 1;

	yy_init_buffer( b, file );

	return b;
	}


void yy_delete_buffer( b )
YY_BUFFER_STATE b;
	{
	if ( ! b )
		return;

	if ( b == yy_current_buffer )
		yy_current_buffer = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		yy_flex_free( (genericptr_t) b->yy_ch_buf );

	yy_flex_free( (genericptr_t) b );
	}


#ifndef YY_ALWAYS_INTERACTIVE
#ifndef YY_NEVER_INTERACTIVE
extern int FDECL(isatty, (int));
#endif
#endif

void yy_init_buffer( b, file )
YY_BUFFER_STATE b;
FILE *file;
	{
	yy_flush_buffer( b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

#ifdef YY_ALWAYS_INTERACTIVE
	b->yy_is_interactive = 1;
#else
#ifdef YY_NEVER_INTERACTIVE
	b->yy_is_interactive = 0;
#else
	b->yy_is_interactive = file ? (isatty( fileno(file) ) > 0) : 0;
#endif
#endif
	}


void yy_flush_buffer( b )
YY_BUFFER_STATE b;
	{
	b->yy_n_chars = 0;

	/* We always need two end-of-buffer characters.  The first causes
	 * a transition to the end-of-buffer state.  The second causes
	 * a jam in that state.
	 */
	b->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
	b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;

	b->yy_buf_pos = &b->yy_ch_buf[0];

	b->yy_at_bol = 1;
	b->yy_buffer_status = YY_BUFFER_NEW;

	if ( b == yy_current_buffer )
		yy_load_buffer_state();
	}



#ifndef YY_EXIT_FAILURE
#define YY_EXIT_FAILURE 2
#endif

static void yy_fatal_error( msg )
const char msg[];
	{
	(void) fprintf( stderr, "%s\n", msg );
	exit( YY_EXIT_FAILURE );
	}



/* Redefine yyless() so it works in section 3 code. */

#undef yyless
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
		yytext[yyleng] = yy_hold_char; \
		yy_c_buf_p = yytext + n - YY_MORE_ADJ; \
		yy_hold_char = *yy_c_buf_p; \
		*yy_c_buf_p = '\0'; \
		yyleng = n; \
		} \
	while ( 0 )


/* Internal utility routines. */

#ifndef yytext_ptr
static void yy_flex_strncpy( s1, s2, n )
char *s1;
const char *s2;
int n;
	{
	int i;
	for ( i = 0; i < n; ++i )
		s1[i] = s2[i];
	}
#endif


static genericptr_t yy_flex_alloc( size )
yy_size_t size;
	{
	return (genericptr_t) alloc((unsigned)size);
	}

/* we want to avoid use of realloc(), so we require that caller supply the
   size of the old block of memory */
static genericptr_t yy_flex_realloc2( ptr, size, old_size )
genericptr_t ptr;
yy_size_t size;
int old_size;
	{
	genericptr_t outptr = yy_flex_alloc(size);

	if (ptr) {
	    char *p = (char *) outptr, *q = (char *) ptr;

	    while (--old_size >= 0) *p++ = *q++;
	    yy_flex_free(ptr);
	}
	return outptr;
	}

static void yy_flex_free( ptr )
genericptr_t ptr;
	{
	free( ptr );
	}

/*flexhack.skl*/


/* routine to switch to another input file; needed for flex */
void init_yyin( input_f )
FILE *input_f;
{
#if defined(FLEX_SCANNER) || defined(FLEXHACK_SCANNER)
	if (yyin)
	    yyrestart(input_f);
	else
#endif
	    yyin = input_f;
}
/* analogous routine (for completeness) */
void init_yyout( output_f )
FILE *output_f;
{
	yyout = output_f;
}

/*dgn_comp.l*/
