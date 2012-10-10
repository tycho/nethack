/* Copyright (c) Daniel Thaler, 2011.                             */
/* NitroHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <ctype.h>

struct nh_player_info player;

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

    colorattr = curses_color_attr(percent_color(val_cur, val_max));

    waddch(statuswin, '[');
    wattron(statuswin, colorattr);

    /* use_inverse enabled colors the whole string and colors the bar inverse
     * use_inverse disabled colors part of the string to represent the bar. */
    if (settings.use_inverse)
	wattron(statuswin, A_REVERSE);
    wprintw(statuswin, "%.*s", fill_len, str);
    if (settings.use_inverse)
	wattroff(statuswin, A_REVERSE);
    else
	wattroff(statuswin, colorattr);
    wprintw(statuswin, "%.*s", len - fill_len, &str[fill_len]);

    if (settings.use_inverse)
	wattroff(statuswin, colorattr);
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
			!strcmp(st, "Satiated") ? alert :
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

/*
 * longest practical second status line at the moment is
 *	Astral Plane $:12345 HP:700(700) Pw:111(111) AC:-127 Xp:30/123456789
 *	T:123456 Satiated Conf FoodPois Ill Blind Stun Hallu Overloaded
 * -- or somewhat over 130 characters
 */

static void classic_status(struct nh_player_info *pi)
{
    char buf[COLNO];
    int colorattr;

    /* line 1 */
    sprintf(buf, "%.10s the %-*s  ", pi->plname,
	    pi->max_rank_sz + 8 - (int)strlen(pi->plname), pi->rank);
    buf[0] = toupper(buf[0]);
    wmove(statuswin, 0, 0);
    draw_string_bar(buf, pi->hp, pi->hpmax);

    if (pi->st == 18 && pi->st_extra) {
	if (pi->st_extra < 100)
	    wprintw(statuswin, "St:18/%02d ", pi->st_extra);
	else
	    wprintw(statuswin,"St:18/** ");
    } else
	wprintw(statuswin, "St:%-1d ", pi->st);
    
    wprintw(statuswin, "Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
	    pi->dx, pi->co, pi->in, pi->wi, pi->ch);
    wprintw(statuswin, (pi->align == A_CHAOTIC) ? "  Chaotic" :
		    (pi->align == A_NEUTRAL) ? "  Neutral" : "  Lawful");
    
    if (settings.showscore)
	wprintw(statuswin, " S:%ld", pi->score);
    
    wclrtoeol(statuswin);

    /* line 2 */
    mvwaddstr(statuswin, 1, 0, pi->level_desc);

    wprintw(statuswin, " %c:%-2ld", pi->coinsym, pi->gold);

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

    wprintw(statuswin, " AC:%-2d", pi->ac);

    if (pi->monnum != pi->cur_monnum)
	wprintw(statuswin, " HD:%d", pi->level);
    else if (settings.showexp)
	wprintw(statuswin, " Xp:%u/%-1ld", pi->level, pi->xp);
    else
	wprintw(statuswin, " Exp:%u", pi->level);

    if (settings.time)
	wprintw(statuswin, " T:%ld", pi->moves);

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
    int i, namelen;
    
    /* line 1 */
    namelen = strlen(pi->plname) < 13 ? strlen(pi->plname) : 13;
    sprintf(buf, "%.13s the %-*s  ", pi->plname,
	    pi->max_rank_sz + 13 - namelen, pi->rank);
    buf[0] = toupper(buf[0]);
    mvwaddstr(statuswin, 0, 0, buf);
    wprintw(statuswin, "Con:%2d Str:", pi->co);
    if (pi->st == 18 && pi->st_extra) {
	if (pi->st_extra < 100)
	    wprintw(statuswin, "18/%02d  ", pi->st_extra);
	else
	    wprintw(statuswin,"18/**  ");
    } else
	wprintw(statuswin, "%2d  ", pi->st);

    waddstr(statuswin, pi->level_desc);

    if (settings.time)
	wprintw(statuswin, "  T:%ld", pi->moves);
    
    wprintw(statuswin, (pi->align == A_CHAOTIC) ? "  Chaotic" :
		    (pi->align == A_NEUTRAL) ? "  Neutral" : "  Lawful");
    wclrtoeol(statuswin);
    
    
    /* line 2 */
    wmove(statuswin, 1, 0);
    draw_bar(18 + pi->max_rank_sz, pi->hp, pi->hpmax, "HP:");
    wprintw(statuswin, "  Int:%2d Wis:%2d  %c:%-2ld  AC:%-2d  ", pi->in, pi->wi,
	    pi->coinsym, pi->gold, pi->ac);
    
    if (pi->monnum != pi->cur_monnum)
	wprintw(statuswin, "HD:%d", pi->level);
    else if (settings.showexp) {
	if (pi->xp < 1000000)
	    wprintw(statuswin, "Xp:%u/%-1ld", pi->level, pi->xp);
	else
	    wprintw(statuswin, "Xp:%u/%-1ldk", pi->level, pi->xp / 1000);
    }
    else
	wprintw(statuswin, "Exp:%u", pi->level);
    
    if (settings.showscore)
	wprintw(statuswin, "  S:%ld", pi->score);
    wclrtoeol(statuswin);

    /* line 3 */
    wmove(statuswin, 2, 0);
    draw_bar(18 + pi->max_rank_sz, pi->en, pi->enmax, "Pw:");
    wprintw(statuswin, "  Dex:%2d Cha:%2d ", pi->dx, pi->ch);
    
    wattron(statuswin, curses_color_attr(CLR_YELLOW));
    for (i = 0; i < pi->nr_items; i++)
	wprintw(statuswin, " %s", pi->statusitems[i]);
    wattroff(statuswin, curses_color_attr(CLR_YELLOW));
    wclrtoeol(statuswin);
}


void curses_update_status(struct nh_player_info *pi)
{
    if (pi)
	player = *pi;
    
    if (player.x == 0)
	return; /* called before the game is running */
    
    if (ui_flags.status3)
	status3(&player);
    else
	classic_status(&player);
    
    /* prevent the cursor from flickering in the status line */
    wmove(mapwin, player.y, player.x - 1);
    
    wnoutrefresh(statuswin);
}

void curses_update_status_silent(struct nh_player_info *pi)
{
    if (pi)
	player = *pi;
}
