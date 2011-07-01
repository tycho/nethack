/*	SCCS Id: @(#)bemain.c	3.2	96/05/23	*/
/* Copyright (c) Dean Luick, 1996. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static void whoami(void);
static void process_options(int argc, char **argv);
static void chdirx(const char *dir, boolean wr);


int main(int argc, char **argv)
{
	int fd;
	char *dir;	

	dir = getenv("NETHACKDIR");
	if (!dir) dir = getenv("HACKDIR");

	choose_windows(DEFAULT_WINDOW_SYS);
	chdirx(dir,1);
	initoptions();

	init_nhwindows(&argc, argv);
	whoami();

	/*
	 * It seems you really want to play.
	 */
	setrandom();
	u.uhp = 1;	/* prevent RIP on early quits */
	process_options(argc, argv);	/* command line options */


#ifdef WIZARD
	if (wizard)
		Strcpy(plname, "wizard");
	else
#endif
	if(!*plname || !strncmp(plname, "player", 4)
		    || !strncmp(plname, "games", 4))
		askname();
	plnamesuffix();		/* strip suffix from name; calls askname() */
						/* again if suffix was whole name */
						/* accepts any suffix */

	//Sprintf(lock,"%d%s", getuid(), plname) ;
	//getlock() ;


	//dlb_init();			/* must be before newgame() */

	/*
	 * Initialization of the boundaries of the mazes
	 * Both boundaries have to be even.
	 */
	x_maze_max = COLNO-1;
	if (x_maze_max % 2)
		x_maze_max--;
	y_maze_max = ROWNO-1;
	if (y_maze_max % 2)
		y_maze_max--;

	/*
	 * Initialize the vision system.  This must be before mklev() on a
	 * new game or before a level restore on a saved game.
	 */
	vision_init();

	display_gamewindows();

	if ((fd = restore_saved_game()) >= 0) {
#ifdef WIZARD
		/* Since wizard is actually flags.debug, restoring might
		 * overwrite it.
		 */
		boolean remember_wiz_mode = wizard;
#endif
#ifdef NEWS
		if(flags.news) {
			display_file(NEWS, FALSE);
			flags.news = FALSE;	/* in case dorecover() fails */
		}
#endif
		pline("Restoring save file...");
		mark_synch();	/* flush output */
		if(!dorecover(fd))
			goto not_recovered;
#ifdef WIZARD
		if(!wizard && remember_wiz_mode) wizard = TRUE;
#endif
		pline("Hello %s, welcome back to NetHack!", plname);
		check_special_room(FALSE);
		if (discover)
			You("are in non-scoring discovery mode.");

		if (discover || wizard) {
			if(yn("Do you want to keep the save file?") == 'n')
			    (void) delete_savefile();
			else {
			    compress(SAVEF);
			}
		}

		flags.move = 0;
	} else {
not_recovered:
		player_selection();
		newgame();
		/* give welcome message before pickup messages */
		pline("Hello %s, welcome to NetHack!", plname);
		if (discover)
			You("are in non-scoring discovery mode.");

		flags.move = 0;
		set_wear();
		pickup(1);
	}

	moveloop();
	return 0;
}

static void
whoami(void)
{
        /*
         * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS
         *                      2. Use $USER or $LOGNAME        (if 1. fails)
         * The resulting name is overridden by command line options.
         * If everything fails, or if the resulting name is some generic
         * account like "games", "play", "player", "hack" then eventually
         * we'll ask him.
         */
        char *s;

        if (*plname) return;
        if (s = getenv("USER")) {
		(void) strncpy(plname, s, sizeof(plname)-1);
		return;
	}
        if (s = getenv("LOGNAME")) {
		(void) strncpy(plname, s, sizeof(plname)-1);
		return;
	}
}

/* normalize file name - we don't like .'s, /'s, spaces */
void
regularize(char *s)
{
	register char *lp;

	while((lp=index(s, '.')) || (lp=index(s, '/')) || (lp=index(s,' ')))
		*lp = '_';
}

static void
process_options(int argc, char **argv)
{
	while (argc > 1 && argv[1][0] == '-') {
		argv++;
		argc--;
		switch (argv[0][1]) {
		case 'D':
#ifdef WIZARD
			wizard = TRUE;
			break;
#endif
			/* otherwise fall thru to discover */
		case 'X':
			discover = TRUE;
			break;
#ifdef NEWS
		case 'n':
			flags.news = FALSE;
			break;
#endif
		case 'u':
			if(argv[0][2])
				(void) strncpy(plname, argv[0]+2, sizeof(plname)-1);
			else if (argc > 1) {
				argc--;
				argv++;
				(void) strncpy(plname, argv[0], sizeof(plname)-1);
			} else
				raw_print("Player name expected after -u");
			break;
		default:
			/* allow -T for Tourist, etc. */
			(void) strncpy(pl_character, argv[0]+1,
				sizeof(pl_character)-1);

			/* raw_printf("Unknown option: %s", *argv); */
		}
	}
}

static void
chdirx(const char *dir, boolean wr)
{
	if (!dir) dir = HACKDIR;

	if (chdir(dir) < 0)
		error("Cannot chdir to %s.", dir);

	/* Warn the player if we can't write the record file */
	/* perhaps we should also test whether . is writable */
	/* unfortunately the access system-call is worthless */
	if (wr) check_recordfile(dir);
}


/*
 * This is pretty useless now, but will be needed when we add the Be GUI.
 * When that happens, the main nethack code will run in its own thread.
 * If the main code exits we must catch this and kill the GUI threads.
 */
void nethack_exit(int status);
void nethack_exit(int status)
{
	exit(status);
}
