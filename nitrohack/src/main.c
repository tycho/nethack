/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <signal.h>


static void process_args(int, char **);
void append_slash(char *name);

struct settings settings;
struct interface_flags ui_flags;
nh_bool interrupt_multi = FALSE;
nh_bool game_is_running = FALSE;
int initrole = ROLE_NONE, initrace = ROLE_NONE;
int initgend = ROLE_NONE, initalign = ROLE_NONE;
nh_bool random_player = FALSE;

enum menuitems {
    NEWGAME = 1,
    TUTORIAL,
    LOAD,
    REPLAY,
    OPTIONS,
    TOPTEN,
    NETWORK,
    EXITGAME
};

struct nh_menuitem mainmenu_items[] = {
    {NEWGAME, MI_NORMAL, "new game", 'n'},
    {TUTORIAL, MI_NORMAL, "new game (tutorial mode)", 't'},
    {LOAD, MI_NORMAL, "load game", 'l'},
    {REPLAY, MI_NORMAL, "view replay", 'v'},
    {OPTIONS, MI_NORMAL, "set options", 'o'},
    {TOPTEN, MI_NORMAL, "show score list", 's'},
#if defined(NETCLIENT)
    {NETWORK, MI_NORMAL, "connect to server", 'c'},
#endif
    {EXITGAME, MI_NORMAL, "quit", 'q', 'x'}
};


static const char *nhlogo_small[] = {
" ____                        _    _               _    ",
"|  _ \\                      | |  | |             | | __",
"| | \\ | _   _  _____  _____ | |__| | _____  ____ | |/ /",
"| | | || | | ||  _  \\/  _  ||  __  |/  _  |/  __||   / ",
"| |_/ || |_| || | | || (_| || |  | || (_| || (__ | | \\ ",
"|____/ \\____ ||_| |_|\\_____||_|  |_|\\_____|\\____||_|\\_\\",
"       |_____/                         -- Ascend or Die",
NULL
};

static const char *nhlogo_large[] = {
" ____                        _    _               _    ",
"|  _ \\                      | |  | |             | | __",
"| | \\ | _   _  _____  _____ | |__| | _____  ____ | |/ /",
"| | | || | | ||  _  \\/  _  ||  __  |/  _  |/  __||   / ",
"| |_/ || |_| || | | || (_| || |  | || (_| || (__ | | \\ ",
"|____/ \\____ ||_| |_|\\_____||_|  |_|\\_____|\\____||_|\\_\\",
"       |_____/                         -- Ascend or Die",
NULL
};

const char **get_logo(nh_bool large)
{
    return large ? nhlogo_large : nhlogo_small;
}


#ifdef UNIX

/* the terminal went away - do not pass go, etc. */
static void sighup_handler(int signum)
{
    if (!ui_flags.done_hup++)
	nh_exit_game(EXIT_FORCE_SAVE);
    nh_lib_exit();
    exit(0);
}

static void sigint_handler(int signum)
{
    if (!game_is_running)
	return;
    
    nh_exit_game(EXIT_REQUEST_SAVE);
}

static void setup_signals(void)
{
    signal(SIGHUP, sighup_handler); 
    signal(SIGTERM, sighup_handler);
# ifdef SIGXCPU
    signal(SIGXCPU, sighup_handler);
# endif
    signal(SIGQUIT,SIG_IGN);
    
    
    signal(SIGINT, sigint_handler);
}

#else
static void setup_signals(void) {}
#endif

static char** init_game_paths(const char *argv0)
{
#ifdef WIN32
    char dirbuf[1024], docpath[MAX_PATH], *pos;
    wchar_t w_dump_dir[MAX_PATH];
#endif
    char **pathlist = malloc(sizeof(char*) * PREFIX_COUNT);
    char *dir = NULL;
    int i;
    nh_bool free_dump = FALSE;

#if defined(UNIX)
    if (getgid() == getegid()) {
	dir = getenv("DYNAHACKDIR");
	if (!dir)
	    dir = getenv("HACKDIR");
    } else
	dir = NULL;
    
    if (!dir)
	dir = DYNAHACKDIR;
    
    for (i = 0; i < PREFIX_COUNT; i++)
	pathlist[i] = dir;
    pathlist[DUMPPREFIX] = malloc(BUFSZ);
    free_dump = TRUE;
    if (!get_gamedir(DUMP_DIR, pathlist[DUMPPREFIX])) {
	free(pathlist[DUMPPREFIX]);
	free_dump = FALSE;
	pathlist[DUMPPREFIX] = getenv("HOME");
	if (!pathlist[DUMPPREFIX])
	    pathlist[DUMPPREFIX] = "./";
    }
    
#elif defined(WIN32)
    dir = getenv("DYNAHACKDIR");
    if (!dir) {
	strncpy(dirbuf, argv0, 1023);
	pos = strrchr(dirbuf, '\\');
	if (!pos)
	    pos = strrchr(dirbuf, '/');
	if (!pos) {
	    /* argv0 doesn't contain a path */
	    strcpy(dirbuf, ".\\");
	    pos = &dirbuf[1];
	}
	*(++pos) = '\0';
	dir = dirbuf;
    }
    
    for (i = 0; i < PREFIX_COUNT; i++)
	pathlist[i] = dir;

    if (get_gamedir(DUMP_DIR, w_dump_dir)) {
	int dump_sz = WideCharToMultiByte(CP_ACP, 0, w_dump_dir, -1,
					  NULL, 0, NULL, NULL);
	if (!dump_sz)
	    goto dump_fallback;
	if (!(pathlist[DUMPPREFIX] = malloc(dump_sz)))
	    goto dump_fallback;
	free_dump = TRUE;
	if (!WideCharToMultiByte(CP_ACP, 0, w_dump_dir, -1,
				 pathlist[DUMPPREFIX], dump_sz, NULL, NULL)) {
	    free(pathlist[DUMPPREFIX]);
	    free_dump = FALSE;
	    goto dump_fallback;
	}
	pathlist[DUMPPREFIX][dump_sz - 1] = 0;
    } else {
 dump_fallback:
	/* get the actual, localized path to the Documents folder */
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, docpath)))
	    pathlist[DUMPPREFIX] = docpath;
	else
	    pathlist[DUMPPREFIX] = ".\\";
    }

#else
    /* Avoid a trap for people trying to port this. */
#error You must run DynaHack under Win32 or Linux.
#endif
    
    /* alloc memory for the paths and append slashes as required */
    for (i = 0; i < PREFIX_COUNT; i++) {
	char *tmp = pathlist[i];
	pathlist[i] = malloc(strlen(tmp) + 2);
	strcpy(pathlist[i], tmp);
	if (i == DUMPPREFIX && free_dump)
	    free(tmp);
	append_slash(pathlist[i]);
    }
    
    return pathlist;
}

#define str_macro(val) #val
static void mainmenu(void)
{
    int menuresult[1];
    int n = 1, logowidth, logoheight, i;
    const char * const *copybanner = nh_get_copyright_banner();
    const char **nhlogo;
    char verstr[32];
    sprintf(verstr, "Version %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL);
    
    while (n >= 0) {
	nhlogo = get_logo(TRUE);
	logowidth = strlen(nhlogo[0]);
	if (COLS < logowidth) {
	    nhlogo = get_logo(FALSE);
	    logowidth = strlen(nhlogo[0]);
	}
	for (logoheight = 0; nhlogo[logoheight]; logoheight++)
	    /* empty */;
	wclear(basewin);
	wattron(basewin, A_BOLD | COLOR_PAIR(4));
	for (i = 0; i < logoheight; i++) {
	    wmove(basewin, i, (COLS - logowidth) / 2);
	    waddstr(basewin, nhlogo[i]);
	}
	wattroff(basewin, A_BOLD | COLOR_PAIR(4));
	mvwaddstr(basewin, LINES-3, 0, copybanner[0]);
	mvwaddstr(basewin, LINES-2, 0, copybanner[1]);
	mvwaddstr(basewin, LINES-1, 0, copybanner[2]);
	mvwaddstr(basewin, LINES-2, COLS - strlen(verstr), verstr);
	wrefresh(basewin);

	n = curses_display_menu_core(mainmenu_items, ARRAY_SIZE(mainmenu_items),
				     NULL, PICK_ONE, menuresult, 0, logoheight,
				     COLS, ROWNO+3, NULL, FALSE);
	if (n < 1)
	    continue;

	switch (menuresult[0]) {
	    case NEWGAME:
		rungame(FALSE);
		break;
		
	    case TUTORIAL:
		rungame(TRUE);
		break;
		
	    case LOAD:
		loadgame();
		break;
		
	    case REPLAY:
		replay();
		break;
		
	    case OPTIONS:
		display_options(TRUE);
		break;
		
#if defined(NETCLIENT)
	    case NETWORK:
		netgame();
		break;
#endif
		
	    case TOPTEN:
		show_topten(NULL, -1, FALSE, FALSE);
		break;
		
	    case EXITGAME:
		n = -1; /* simulate menu cancel */
		return;
	}
    }
}


int main(int argc, char *argv[])
{
    char **gamepaths;
    int i;
    
    umask(0777 & ~FCMASK);
    
    init_options();
    
    gamepaths = init_game_paths(argv[0]);
    nh_lib_init(&curses_windowprocs, gamepaths);
    for (i = 0; i < PREFIX_COUNT; i++)
	free(gamepaths[i]);
    free(gamepaths);
    
    setup_signals();
    init_curses_ui();
    read_nh_config();

    process_args(argc, argv);	/* command line options */
    init_displaychars();
    
    mainmenu();
    
    exit_curses_ui();
    nh_lib_exit();
    free_displaychars();

    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}


static int str2role(const struct nh_roles_info *ri, const char *str)
{
    int i, role = -1;
    
    for (i = 0; i < ri->num_roles && role == -1; i++)
	if (!strncasecmp(ri->rolenames_m[i], str, strlen(str)) ||
	    (ri->rolenames_f[i] && !strncasecmp(ri->rolenames_f[i], str, strlen(str))))
	    role = i;

    return role;
}


static int str2race(const struct nh_roles_info *ri, const char *str)
{
    int i, race = -1;
    
    for (i = 0; i < ri->num_races && race == -1; i++)
	if (!strncasecmp(ri->racenames[i], str, strlen(str)))
	    race = i;

    return race;
}


static void process_args(int argc, char *argv[])
{
    int i;
    const struct nh_roles_info *ri = nh_get_roles();

    /*
     * Process options.
     */
    while (argc > 1 && argv[1][0] == '-'){
	argv++;
	argc--;
	switch(argv[0][1]){
	case 'D':
	    ui_flags.playmode = MODE_WIZARD;
	    break;
		
	case 'X':
	    ui_flags.playmode = MODE_EXPLORE;
	    break;
	    
	case 'u':
	    if (argv[0][2])
		strncpy(settings.plname, argv[0]+2, sizeof(settings.plname)-1);
	    else if (argc > 1) {
		argc--;
		argv++;
		strncpy(settings.plname, argv[0], sizeof(settings.plname)-1);
	    } else
		    printf("Player name expected after -u");
	    break;
	    
	case 'p': /* profession (role) */
	    if (argv[0][2]) {
		i = str2role(ri, &argv[0][2]);
		if (i >= 0)
		    initrole = i;
	    } else if (argc > 1) {
		argc--;
		argv++;
		i = str2role(ri, argv[0]);
		if (i >= 0)
		    initrole = i;
	    }
	    break;
	    
	case 'r': /* race */
	    if (argv[0][2]) {
		i = str2race(ri, &argv[0][2]);
		if (i >= 0)
		    initrace = i;
	    } else if (argc > 1) {
		argc--;
		argv++;
		i = str2race(ri, argv[0]);
		if (i >= 0)
		    initrace = i;
	    }
	    break;
	    
	case '@':
	    random_player = TRUE;
	    break;
	    
	default:
	    i = str2role(ri, argv[0]);
	    if (i >= 0)
		initrole = i;
	}
    }
}


/*
* Add a slash to any name not ending in /. There must
* be room for the /
*/
void append_slash(char *name)
{
#if defined(WIN32)
    static const char dirsep = '\\';
#else
    static const char dirsep = '/';
#endif
    char *ptr;

    if (!*name)
	return;
    ptr = name + (strlen(name) - 1);
    if (*ptr != dirsep) {
	*++ptr = dirsep;
	*++ptr = '\0';
    }
    return;
}

/* main.c */
