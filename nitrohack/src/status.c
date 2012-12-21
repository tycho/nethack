/* Copyright (c) Daniel Thaler, 2011.                             */
/* NitroHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <ctype.h>

struct nh_player_info player;

/* for coloring status differences */
static struct nh_player_info saved_player, old_player;
static int statdiff_moves;	/* stop non-actions from blanking the colors */


/* Must be called before nh[net]_start_game(), nh[net]_restore_game() and
 * nh_view_replay_start(), which all call curses_update_status[_silent]() before
 * [replay_]commandloop() runs. */
void reset_old_status(void)
{
    /* Flag statdiff system for initialization. */
    statdiff_moves = -1;
}


void update_old_status(void)
{
    saved_player = player;
}


static int status_change_color(int old1, int old2, int new1, int new2)
{
    if (new1 > old1)
	return CLR_GREEN;
    else if (new1 < old1)
	return CLR_RED;
    else if (new2 > old2)
	return CLR_GREEN;
    else if (new2 < old2)
	return CLR_RED;
    else
	return 0;
}


static void status_change_color_on(int *col, int *attr,
				   int old1, int old2, int new1, int new2)
{
    *col = status_change_color(old1, old2, new1, new2);
    if (*col) {
	*attr = curses_color_attr(*col);
	wattron(statuswin, *attr);
    }
}


static void status_change_color_off(int col, int attr)
{
    if (col)
	wattroff(statuswin, attr);
}


static void print_statdiff(const char *prefix, const char *fmt, int oldv, int newv)
{
    int col, attr;

    if (prefix && prefix[0])
	waddstr(statuswin, prefix);
    status_change_color_on(&col, &attr, oldv, 0, newv, 0);
    wprintw(statuswin, fmt, newv);
    status_change_color_off(col, attr);
}


static void print_statdiffr(const char *prefix, const char *fmt, int oldv, int newv)
{
    int col, attr;

    if (prefix && prefix[0])
	waddstr(statuswin, prefix);
    status_change_color_on(&col, &attr, -oldv, 0, -newv, 0);
    wprintw(statuswin, fmt, newv);
    status_change_color_off(col, attr);
}


static int percent_color(int val_cur, int val_max)
{
    int percent, color;

    if (val_max <= 0 || val_cur <= 0)
	percent = 0;
    else
	percent = 100 * val_cur / val_max;

    if (percent < 25)
	color = CLR_RED;
    else if (percent < 50)
	color = CLR_BROWN; /* inverted this looks orange */
    else if (percent < 95)
	color = CLR_GREEN;
    else
	color = CLR_GRAY; /* inverted this is white, with better text contrast */

    return color;
}


static void draw_string_bar(const char *str, int val_cur, int val_max)
{
    int i, len, fill_len, colorattr;

    /* Allow trailing spaces, but only after the bar. */
    len = strlen(str);
    for (i = len - 1; i >= 0; i--) {
	if (str[i] == ' ')
	    len = i;
	else
	    break;
    }

    if (val_max <= 0 || val_cur <= 0)
	fill_len = 0;
    else
	fill_len = len * val_cur / val_max;
    if (fill_len > len)
	fill_len = len;

    colorattr = curses_color_attr(percent_color(val_cur, val_max));
    waddch(statuswin, '[');
    wattron(statuswin, colorattr);
    if (settings.use_inverse) {
	/* color the whole string and inverse the bar */
	wattron(statuswin, A_REVERSE);
	wprintw(statuswin, "%.*s", fill_len, str);
	wattroff(statuswin, A_REVERSE);
	wprintw(statuswin, "%.*s", len - fill_len, &str[fill_len]);
	wattroff(statuswin, colorattr);
    } else {
	/* color the part of the string representing the bar */
	wprintw(statuswin, "%.*s", fill_len, str);
	wattroff(statuswin, colorattr);
	/* use black to avoid confusion with being full */
	colorattr = curses_color_attr(COLOR_BLACK);
	wattron(statuswin, colorattr);
	wprintw(statuswin, "%.*s", len - fill_len, &str[fill_len]);
	wattroff(statuswin, colorattr);
    }
    waddch(statuswin, ']');

    wprintw(statuswin, "%s", &str[len]);
}


static void draw_statuses(const struct nh_player_info *pi)
{
    const int notice = CLR_YELLOW;
    const int alert = CLR_ORANGE;
    int i, j, len, colorattr;

    for (i = 0; i < pi->nr_items; i++) {
	const char *st = pi->statusitems[i];

	colorattr = curses_color_attr(
			/* These are padded, unlike the others. */
			!strcmp(st, "Satiated") ? notice :
			!strncmp(st, "Hungry", 6) ? notice :
			!strncmp(st, "Weak", 4) ? alert :
			!strcmp(st, "Fainting") ? alert :
			!strncmp(st, "Fainted", 7) ? alert :
			!strncmp(st, "Starved", 7) ? alert :

			!strcmp(st, "Conf") ? notice :
			!strcmp(st, "FoodPois") ? alert :
			!strcmp(st, "Ill") ? alert :
			!strcmp(st, "Blind") ? notice :
			!strcmp(st, "Stun") ? notice :
			!strcmp(st, "Hallu") ? notice :
			!strcmp(st, "Slime") ? alert :

			!strcmp(st, "Burdened") ? notice :
			!strcmp(st, "Stressed") ? notice :
			!strcmp(st, "Strained") ? notice :
			!strcmp(st, "Overtaxed") ? notice :
			!strcmp(st, "Overloaded") ? notice :
			notice);

	/* Strip trailing spaces. */
	len = strlen(st);
	for (j = len - 1; j >= 0; j--) {
	    if (st[j] == ' ')
		len = j;
	    else
		break;
	}

	wattron(statuswin, colorattr);
	wprintw(statuswin, " %.*s", len, st);
	wattroff(statuswin, colorattr);
    }
}


static void draw_time(const struct nh_player_info *pi)
{
    const struct nh_player_info *oldpi = &old_player;
    int movediff, colorattr;
    nh_bool colorchange;

    if (!settings.time)
	return;

    /* Give players a better idea of the passage of time. */
    movediff = pi->moves - oldpi->moves;
    if (movediff > 1) {
	colorattr = curses_color_attr(
			(movediff >= 100) ? COLOR_RED :
			(movediff >=  50) ? COLOR_YELLOW :
			(movediff >=  20) ? COLOR_CYAN :
			(movediff >=  10) ? COLOR_BLUE :
					    COLOR_GREEN);
	colorchange = TRUE;
    } else {
	colorchange = FALSE;
    }

    if (colorchange)
	wattron(statuswin, colorattr);
    wprintw(statuswin, "T:%ld", pi->moves);
    if (colorchange)
	wattroff(statuswin, colorattr);
}


/*
 * longest practical second status line at the moment is
 *	Astral Plane $:12345 HP:700(700) Pw:111(111) AC:-127 Xp:30/123456789
 *	T:123456 Satiated Conf FoodPois Ill Blind Stun Hallu Overloaded
 * -- or somewhat over 130 characters
 */

static void classic_status(struct nh_player_info *pi)
{
    char buf[COLNO];
    int stat_ch_col, colorattr;
    const struct nh_player_info *oldpi = &old_player;

    /* line 1 */
    sprintf(buf, "%.10s the %-*s  ", pi->plname,
	    pi->max_rank_sz + 8 - (int)strlen(pi->plname), pi->rank);
    buf[0] = toupper(buf[0]);
    wmove(statuswin, 0, 0);
    draw_string_bar(buf, pi->hp, pi->hpmax);

    status_change_color_on(&stat_ch_col, &colorattr,
			   oldpi->st, oldpi->st_extra,
			   pi->st, pi->st_extra);
    waddstr(statuswin, "St:");
    if (pi->st == 18 && pi->st_extra) {
	if (pi->st_extra < 100)
	    wprintw(statuswin, "18/%02d", pi->st_extra);
	else
	    wprintw(statuswin,"18/**");
    } else
	wprintw(statuswin, "%-1d", pi->st);
    status_change_color_off(stat_ch_col, colorattr);

    print_statdiff(" ", "Dx:%-1d", oldpi->dx, pi->dx);
    print_statdiff(" ", "Co:%-1d", oldpi->co, pi->co);
    print_statdiff(" ", "In:%-1d", oldpi->in, pi->in);
    print_statdiff(" ", "Wi:%-1d", oldpi->wi, pi->wi);
    print_statdiff(" ", "Ch:%-1d", oldpi->ch, pi->ch);

    wprintw(statuswin, (pi->align == A_CHAOTIC) ? "  Chaotic" :
		    (pi->align == A_NEUTRAL) ? "  Neutral" : "  Lawful");

    if (settings.showscore)
	print_statdiff(" ", "S:%ld", oldpi->score, pi->score);

    wclrtoeol(statuswin);

    /* line 2 */
    mvwaddstr(statuswin, 1, 0, pi->level_desc);

    waddstr(statuswin, " ");
    status_change_color_on(&stat_ch_col, &colorattr, oldpi->gold, 0, pi->gold, 0);
    wprintw(statuswin, "%c:%-2ld", pi->coinsym, pi->gold);
    status_change_color_off(stat_ch_col, colorattr);

    waddstr(statuswin, " HP:");
    colorattr = curses_color_attr(percent_color(pi->hp, pi->hpmax));
    wattron(statuswin, colorattr);
    wprintw(statuswin, "%d(%d)", pi->hp, pi->hpmax);
    wattroff(statuswin, colorattr);

    waddstr(statuswin, " Pw:");
    colorattr = curses_color_attr(percent_color(pi->en, pi->enmax));
    wattron(statuswin, colorattr);
    wprintw(statuswin, "%d(%d)", pi->en, pi->enmax);
    wattroff(statuswin, colorattr);

    print_statdiffr(" ", "AC:%-2d", oldpi->ac, pi->ac);

    if (pi->monnum != pi->cur_monnum) {
	print_statdiff(" ", "HD:%d", oldpi->level, pi->level);
    } else if (settings.showexp) {
	print_statdiff(" ", "Xp:%u", oldpi->level, pi->level);
	print_statdiff("/", "%-1ld", oldpi->xp, pi->xp);
    } else {
	print_statdiff(" ", "Exp:%u", oldpi->level, pi->level);
    }

    waddstr(statuswin, " ");
    draw_time(pi);

    draw_statuses(pi);

    wclrtoeol(statuswin);
}


static void draw_bar(int barlen, int val_cur, int val_max, const char *prefix)
{
    char str[COLNO], bar[COLNO];
    int fill_len = 0, bl, colorattr;
    
    bl = barlen-2;
    if (val_max > 0 && val_cur > 0)
	fill_len = bl * val_cur / val_max;
    if (fill_len > bl)
	fill_len = bl;

    colorattr = curses_color_attr(percent_color(val_cur, val_max));

    sprintf(str, "%s%d(%d)", prefix, val_cur, val_max);
    sprintf(bar, "%-*s", bl, str);
    waddch(statuswin, '[');
    wattron(statuswin, colorattr);
    
    wattron(statuswin, A_REVERSE);
    wprintw(statuswin, "%.*s", fill_len, bar);
    wattroff(statuswin, A_REVERSE);
    wprintw(statuswin, "%s", &bar[fill_len]);
    
    wattroff(statuswin, colorattr);
    waddch(statuswin, ']');
}


static void status3(struct nh_player_info *pi)
{
    char buf[COLNO];
    int namelen;
    int stat_ch_col, colorattr;
    const struct nh_player_info *oldpi = &old_player;

    /* line 1 */
    namelen = strlen(pi->plname) < 13 ? strlen(pi->plname) : 13;
    sprintf(buf, "%.13s the %-*s  ", pi->plname,
	    pi->max_rank_sz + 13 - namelen, pi->rank);
    buf[0] = toupper(buf[0]);
    mvwaddstr(statuswin, 0, 0, buf);

    print_statdiff(NULL, "Con:%2d", oldpi->co, pi->co);

    waddstr(statuswin, " ");
    status_change_color_on(&stat_ch_col, &colorattr,
			   oldpi->st, oldpi->st_extra,
			   pi->st, pi->st_extra);
    waddstr(statuswin, "Str:");
    if (pi->st == 18 && pi->st_extra) {
	if (pi->st_extra < 100)
	    wprintw(statuswin, "18/%02d", pi->st_extra);
	else
	    wprintw(statuswin,"18/**");
    } else
	wprintw(statuswin, "%2d", pi->st);
    status_change_color_off(stat_ch_col, colorattr);

    waddstr(statuswin, "  ");
    waddstr(statuswin, pi->level_desc);

    waddstr(statuswin, "  ");
    draw_time(pi);

    wprintw(statuswin, (pi->align == A_CHAOTIC) ? "  Chaotic" :
		    (pi->align == A_NEUTRAL) ? "  Neutral" : "  Lawful");
    wclrtoeol(statuswin);

    /* line 2 */
    wmove(statuswin, 1, 0);
    draw_bar(18 + pi->max_rank_sz, pi->hp, pi->hpmax, "HP:");

    print_statdiff("  ", "Int:%2d", oldpi->in, pi->in);
    print_statdiff(" ", "Wis:%2d", oldpi->wi, pi->wi);

    waddstr(statuswin, "  ");
    status_change_color_on(&stat_ch_col, &colorattr, oldpi->gold, 0, pi->gold, 0);
    wprintw(statuswin, "%c:%-2ld", pi->coinsym, pi->gold);
    status_change_color_off(stat_ch_col, colorattr);

    print_statdiffr("  ", "AC:%-2d", oldpi->ac, pi->ac);
    waddstr(statuswin, "  ");

    if (pi->monnum != pi->cur_monnum) {
	print_statdiff(NULL, "HD:%d", oldpi->level, pi->level);
    } else if (settings.showexp) {
	print_statdiff(NULL, "Xp:%u", oldpi->level, pi->level);
	if (pi->xp < 1000000)
	    print_statdiff("/", "%-1ld", oldpi->xp, pi->xp);
	else
	    print_statdiff("/", "%-1ldk", oldpi->xp / 1000, pi->xp / 1000);
    } else {
	print_statdiff(NULL, "Exp:%u", oldpi->level, pi->level);
    }

    if (settings.showscore)
	print_statdiff("  ", "S:%ld", oldpi->score, pi->score);

    wclrtoeol(statuswin);

    /* line 3 */
    wmove(statuswin, 2, 0);
    draw_bar(18 + pi->max_rank_sz, pi->en, pi->enmax, "Pw:");

    print_statdiff("  ", "Dex:%2d", oldpi->dx, pi->dx);
    print_statdiff(" ", "Cha:%2d", oldpi->ch, pi->ch);
    waddstr(statuswin, " ");

    draw_statuses(pi);

    wclrtoeol(statuswin);
}


void curses_update_status_silent(struct nh_player_info *pi)
{
    if (!pi)
	return;

    player = *pi;

    if (statdiff_moves == -1) {
	saved_player = player;
	statdiff_moves = 0;
    }

    if (statdiff_moves != pi->moves) {
	old_player = saved_player;
	statdiff_moves = pi->moves;
    }
}


void curses_update_status(struct nh_player_info *pi)
{
    curses_update_status_silent(pi);

    if (player.x == 0)
	return; /* called before the game is running */

    if (ui_flags.status3)
	status3(&player);
    else
	classic_status(&player);

    /* prevent the cursor from flickering in the status line */
    wmove(mapwin, player.y, player.x - 1);

    /* if HP changes, frame color may change */
    if (ui_flags.draw_frame && settings.frame_hp_color)
	redraw_frame();

    wnoutrefresh(statuswin);
}
