/* Copyright (c) Daniel Thaler, 2011 */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <ctype.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define sgn(x) ((x) >= 0 ? 1 : -1)

struct coord {
    int x, y;
};

static struct nh_dbuf_entry (*display_buffer)[COLNO] = NULL;
static const int xdir[DIR_SELF+1] = { -1,-1, 0, 1, 1, 1, 0,-1, 0, 0 };
static const int ydir[DIR_SELF+1] = {  0,-1,-1,-1, 0, 1, 1, 1, 0, 0 };

/* GetTickCount() returns milliseconds since the system was started, with a
 * resolution of around 15ms. gettimeofday() returns a value since the start of
 * the epoch.
 * The difference doesn't matter here, since the value is only used to control
 * blinking.
 */
#ifdef WIN32
# define get_milliseconds GetTickCount
#else
static int get_milliseconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif



int get_map_key(int place_cursor)
{
    int key = ERR;
    
    if (settings.blink)
	wtimeout(mapwin, 666); /* wait 2/3 of a second before switching */
    
    if (player.x && place_cursor) { /* x == 0 is not a valid coordinate */
	wmove(mapwin, player.y, player.x - 1);
	curs_set(1);
    }
    
    while (1) {
	key = nh_wgetch(mapwin);
	draw_map(player.x, player.y);
	doupdate();
	if (key != ERR)
	    break;
	
    };
    wtimeout(mapwin, -1);
    
    return key;
}


void curses_update_screen(struct nh_dbuf_entry dbuf[ROWNO][COLNO], int ux, int uy)
{
    display_buffer = dbuf;
    draw_map(ux, uy);
    
    if (ux > 0) {
	wmove(mapwin, uy, ux - 1);
	curs_set(1);
    } else
	curs_set(0);
    wnoutrefresh(mapwin);
}


void draw_map(int cx, int cy)
{
    int x, y, symcount, cursx, cursy;
    attr_t attr;
    unsigned int frame;
    struct curses_symdef syms[4];
    
    if (!display_buffer || !mapwin)
	return;
    
    getyx(mapwin, cursy, cursx);
   
    frame = 0;
    if (settings.blink)
	frame = get_milliseconds() / 666;
       
    for (y = 0; y < ROWNO; y++) {
	for (x = 1; x < COLNO; x++) {
	    int bg_color = 0;
	    struct nh_dbuf_entry *dbe = &display_buffer[y][x];

	    /* set the position for each character to prevent incorrect
	     * positioning due to charset issues (IBM chars on a unicode term
	     * or vice versa) */
	    wmove(mapwin, y, x-1);

	    symcount = mapglyph(dbe, syms, &bg_color);
	    attr = A_NORMAL;
	    if ((settings.hilite_pet && (dbe->monflags & MON_TAME)) ||
		(settings.hilite_peaceful && (dbe->monflags & MON_PEACEFUL)) ||
		/* reverse object piles, but don't override stair background */
		(bg_color == 0 && dbe->obj && (dbe->objflags & DOBJ_STACKS))) {
		attr |= A_REVERSE;
		bg_color = 0;
	    }

	    print_sym(mapwin, &syms[frame % symcount], attr, bg_color);
	}
    }
    
    wmove(mapwin, cursy, cursx);
    wnoutrefresh(mapwin);
}


static int compare_coord_dist(const void *p1, const void *p2)
{
    const struct coord *c1 = p1;
    const struct coord *c2 = p2;
    int dx, dy, dist1, dist2;
    
    dx = player.x - c1->x; dy = player.y - c1->y;
    dist1 = dx * dx + dy * dy;
    dx = player.x - c2->x; dy = player.y - c2->y;
    dist2 = dx * dx + dy * dy;
    
    return dist1 - dist2;
}


static void place_desc_message(WINDOW *win, int *x, int *y, char *b)
{
    if (!b || !*b)
	return;

    /* Arrange description messages aligned to 40-character columns,
     * accounting for overflow if needed. */
    b[78] = '\0';
    if (strlen(b) >= 40 && *x >= 40) { (*y)++; *x = 0; }
    if (*y <= 1) mvwaddstr(statuswin, *y, *x, b);
    (*x) += (strlen(b) >= 38 ? 80 : 40);
    if (*x > 40) { (*y)++; *x = 0; }
}


int curses_getpos(int *x, int *y, nh_bool force, const char *goal)
{
    int result = 0;
    int cx, cy;
    int key, dx, dy;
    int sidx;
    static const char pick_chars[] = " \r\n.,;:";
    static const int pick_vals[] = {1, 1, 1, 1, 2, 3, 4};
    const char *cp;
    char printbuf[BUFSZ];
    char *matching = NULL;
    enum nh_direction dir;
    struct coord *monpos = NULL;
    int moncount, monidx;
    nh_bool first_move = TRUE;

    werase(statuswin);
    mvwaddstr(statuswin, 0, 0, "Select a target location with the cursor: "
			       "'.' to confirm");
    if (!force) waddstr(statuswin, ", ESC to cancel.");
    else waddstr(statuswin, ".");
    mvwaddstr(statuswin, 1, 0, "Press '?' for help and tips.");
    wrefresh(statuswin);
    
    cx = *x >= 1 ? *x : player.x;
    cy = *y >= 0 ? *y : player.y;
    wmove(mapwin, cy, cx-1);
    
    while (1) {
	if (first_move) {
	    /* Give initial message a chance to be seen. */
	    first_move = FALSE;
	} else {
	    struct nh_desc_buf descbuf;
	    int mx = 0, my = 0;

	    /* Describe what's under the cursor in the status window. */
	    nh_describe_pos(cx, cy, &descbuf);

	    werase(statuswin);
	    place_desc_message(statuswin, &mx, &my, descbuf.effectdesc);
	    place_desc_message(statuswin, &mx, &my, descbuf.invisdesc);
	    place_desc_message(statuswin, &mx, &my, descbuf.mondesc);
	    place_desc_message(statuswin, &mx, &my, descbuf.objdesc);
	    place_desc_message(statuswin, &mx, &my, descbuf.trapdesc);
	    place_desc_message(statuswin, &mx, &my, descbuf.bgdesc);
	    wrefresh(statuswin);

	    wmove(mapwin, cy, cx-1);
	}

	dx = dy = 0;
	key = get_map_key(FALSE);
	if (key == KEY_ESC) {
	    cx = cy = -10;
	    result = -1;
	    break;
	}
	
	if ((cp = strchr(pick_chars, (char)key)) != 0) {
	    result = pick_vals[cp - pick_chars];
	    break;
	}

	dir = key_to_dir(key);
	if (dir != DIR_NONE) {
	    dx = xdir[dir];
	    dy = ydir[dir];
	} else if ( (dir = key_to_dir(tolower((unsigned char)key))) != DIR_NONE ) {
	    /* a shifted movement letter */
	    dx = xdir[dir] * 8;
	    dy = ydir[dir] * 8;
	}
	
	if (dx || dy) {
	    /* truncate at map edge */
	    if (cx + dx < 1)
		dx = 1 - cx;
	    if (cx + dx > COLNO-1)
		dx = COLNO - 1 - cx;
	    if (cy + dy < 0)
		dy = -cy;
	    if (cy + dy > ROWNO-1)
		dy = ROWNO - 1 - cy;
	    cx += dx;
	    cy += dy;
	    goto nxtc;
	}

	if (key == 'm' || key == 'M') {
	    if (!monpos) {
		int i, j;
		moncount = 0;
		for (i = 0; i < ROWNO; i++)
		    for (j = 0; j < COLNO; j++)
			if (display_buffer[i][j].mon && (j != player.x || i != player.y))
			    moncount++;
		monpos = malloc(moncount * sizeof(struct coord));
		monidx = 0;
		for (i = 0; i < ROWNO; i++)
		    for (j = 0; j < COLNO; j++)
			if (display_buffer[i][j].mon && (j != player.x || i != player.y)) {
			    monpos[monidx].x = j;
			    monpos[monidx].y = i;
			    monidx++;
			}
		qsort(monpos, moncount, sizeof(struct coord), compare_coord_dist);
		/* ready this for first increment/decrement to change to zero */
		monidx = (key == 'm') ? -1 : 1;
	    }
	    
	    if (moncount) { /* there is at least one monster to move to */
		if (key == 'm') {
		    monidx = (monidx + 1) % moncount;
		} else {
		    monidx--;
		    if (monidx < 0) monidx += moncount;
		}
		cx = monpos[monidx].x;
		cy = monpos[monidx].y;
	    }
	} else if (key == '@') {
	    cx = player.x;
	    cy = player.y;
	} else if (key == '?') {
	    static struct nh_menuitem helpitems[] = {
		{0, MI_TEXT, "Move the cursor with the direction keys:"},
		{0, MI_TEXT, ""},
		{0, MI_TEXT, "7 8 9   y k u"},
		{0, MI_TEXT, " \\|/     \\|/"},
		{0, MI_TEXT, "4- -6   h- -l"},
		{0, MI_TEXT, " /|\\     /|\\"},
		{0, MI_TEXT, "1 2 3   b j n"},
		{0, MI_TEXT, ""},
		{0, MI_TEXT, "Hold shift to move the cursor further."},
		{0, MI_TEXT, "Confirm your chosen location with one of .,;:"},
		{0, MI_TEXT, "Cancel with ESC (not always possible)."},
		{0, MI_TEXT, ""},
		{0, MI_TEXT, "Targeting shortcuts:"},
		{0, MI_TEXT, ""},
		{0, MI_TEXT, "@    self"},
		{0, MI_TEXT, "m/M  next/previous monster"},
		{0, MI_TEXT, "<    upstairs"},
		{0, MI_TEXT, ">    downstairs"},
		{0, MI_TEXT, "_    altar"},
		{0, MI_TEXT, "#    sink"},
		{0, MI_TEXT, "     (other dungeon symbols also work)"},
	    };
	    curses_display_menu(helpitems, ARRAY_SIZE(helpitems),
				"Targeting Help (default keys)",
				PICK_NONE, NULL);
	} else {
	    int k = 0, tx, ty;
	    int pass, lo_x, lo_y, hi_x, hi_y;
	    matching = malloc(default_drawing->num_bgelements);
	    memset(matching, 0, default_drawing->num_bgelements);
	    for (sidx = default_drawing->bg_feature_offset;
		 sidx < default_drawing->num_bgelements; sidx++)
		if (key == default_drawing->bgelements[sidx].ch)
		    matching[sidx] = (char) ++k;
	    if (k || key == '^') {
		for (pass = 0; pass <= 1; pass++) {
		    /* pass 0: just past current pos to lower right;
			pass 1: upper left corner to current pos */
		    lo_y = (pass == 0) ? cy : 0;
		    hi_y = (pass == 0) ? ROWNO - 1 : cy;
		    for (ty = lo_y; ty <= hi_y; ty++) {
			lo_x = (pass == 0 && ty == lo_y) ? cx + 1 : 1;
			hi_x = (pass == 1 && ty == hi_y) ? cx : COLNO - 1;
			for (tx = lo_x; tx <= hi_x; tx++) {
			    k = display_buffer[ty][tx].bg;
			    if ((k && matching[k]) ||
				(key == '^' && display_buffer[ty][tx].trap)) {
				cx = tx;
				cy = ty;
				goto nxtc;
			    }
			}	/* column */
		    }	/* row */
		}		/* pass */
		sprintf(printbuf, "Can't find dungeon feature '%c'.", (char)key);
		curses_msgwin(printbuf);
	    } else {
		sprintf(printbuf, "Unknown targeting key %s.",
			!force ? "(ESC to abort, '?' for help)" : "('?' for help)");
		curses_msgwin(printbuf);
	    }
	}

nxtc:
	wmove(mapwin, cy, cx-1);
	wrefresh(mapwin);
    }
    
    *x = cx;
    *y = cy;
    if (monpos)
	free(monpos);
    if (matching)
	free(matching);
    curses_update_status(NULL); /* clear the help message */
    return result;
}
