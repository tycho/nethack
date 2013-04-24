/* Copyright (c) Daniel Thaler, 2011 */
/* NitroHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"

#define array_size(x) (sizeof(x)/sizeof(x[0]))

static short colorlist[] = {
    COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, -1,
    COLOR_WHITE, COLOR_RED + 8, COLOR_GREEN + 8, COLOR_YELLOW + 8,
    COLOR_BLUE + 8, COLOR_MAGENTA + 8, COLOR_CYAN + 8, COLOR_WHITE + 8
};

static const char *colorNames[] = {
    "black", "red", "green", "yellow", "blue",
    "magenta", "cyan", "gray", "white",
    "hired", "higreen", "hiyellow", "hiblue",
    "himagenta", "hicyan", "hiwhite"
};

struct ColorMap {
    short fgColors[array_size(colorlist)];
    short bgColors[array_size(colorlist)];
};

static struct ColorMap color_map;

/* Load color map from colormap.conf config file. */
static void read_colormap(struct ColorMap *map)
{
    fnchar filename[BUFSZ];
    FILE *fp;

    char line[BUFSZ];

    int pos, idx;
    char *colorname;

    int defType, colorIndex, colorValue;

    /* Initialize the map to default values. */
    for (idx = 0; idx < array_size(colorlist); idx++) {
	map->fgColors[idx] = colorlist[idx];
	map->bgColors[idx] = colorlist[idx];
    }

    filename[0] = '\0';
    if (!get_gamedir(CONFIG_DIR, filename))
	return;
    fnncat(filename, FN("colormap.conf"), BUFSZ);

    fp = fopen(filename, "r");
    if (!fp)
	return;

    while (fgets(line, BUFSZ, fp)) {
	if (sscanf(line, " %n%*s %i", &pos, &colorValue) != 1)
	    continue;

	colorname = &line[pos];

	/* Skip comments. */
	if (colorname[0] == '#')
	    continue;

	/* If the color name starts with "fg." or "bg.", then it only
	 * applies to the foreground or background color definition.
	 * Otherwise it applies to both. */
	if (!strncmp(colorname, "fg.", 3)) {
	    defType = 1;
	    colorname += 3;
	} else if (!strncmp(colorname, "bg.", 3)) {
	    defType = 2;
	    colorname += 3;
	} else {
	    defType = 0;
	}

	colorIndex = -1;
	for (idx = 0; idx < array_size(colorNames); idx++) {
	    if (colorNames[idx] && !strncmp(colorname, colorNames[idx],
					    strlen(colorNames[idx]))) {
		colorIndex = idx;
		break;
	    }
	}

	/* If color couldn't be matched, then skip the line. */
	if (colorIndex == -1)
	    continue;

	if (colorValue < COLORS && colorValue >= 0) {
	    if (defType == 0 || defType == 1)
		map->fgColors[colorIndex] = colorValue;
	    if (defType == 0 || defType == 2)
		map->bgColors[colorIndex] = colorValue;
	}
    }

    fclose(fp);

    return;
}


/* Initialize curses color pairs based on the color map provided. */
static void apply_colormap(struct ColorMap *map)
{
    int bg, fg;
    short bgColor, fgColor;

    /* Set up all color pairs.
     * If using bold, then set up color pairs for foreground colors
     *   0-7; if not, then set up color pairs for foreground colors
     *   0-15.
     * If there are sufficient color pairs, then set them up for 6
     * possible non-default background colors (don't use white, there
     * are terminals that hate it).  So there are 112 pairs required
     * for 16 colors, or 56 required for 8 colors. */
    for (bg = 0; bg <= BG_COLOR_COUNT; bg++) {
	/* Do not set up background colors if there are not enough
	 * color pairs. */
	if (bg == 1 && !BG_COLOR_SUPPORT)
	    break;

	/* For no background, use the default background color;
	 * otherwise use the color from the color map. */
	bgColor = bg ? map->bgColors[bg] : -1;

	for (fg = 0; fg <= FG_COLOR_COUNT; fg++) {
	    fgColor = map->fgColors[fg];

	    /* Replace black with blue if darkgray is not set. */
	    if (fgColor == COLOR_BLACK && !settings.darkgray)
		fgColor = COLOR_BLUE;
	    if (fgColor == bgColor && fgColor != -1)
		fgColor = COLOR_BLACK;

	    init_pair(bg * FG_COLOR_COUNT + fg + 1, fgColor, bgColor);
	}
    }

    return;
}


/*
 * Initialize curses colors to colors used by NitroHack
 * (from Karl Garrison's curses UI for Nethack 3.4.3)
 */
void init_nhcolors(void)
{
    if (!has_colors())
	return;

    ui_flags.color = TRUE;

    start_color();
    use_default_colors();

    read_colormap(&color_map);
    apply_colormap(&color_map);
}


int curses_color_attr(int nh_color, int bg_color)
{
    int color = nh_color + 1;
    int cattr = A_NORMAL;

    if (color_map.fgColors[nh_color] == COLOR_BLACK && settings.darkgray)
	cattr |= A_BOLD;

    if (COLORS < 16 && color > 8) {
	color -= 8;
	cattr = A_BOLD;
    }
    if (BG_COLOR_SUPPORT)
	color += bg_color * FG_COLOR_COUNT;
    cattr |= COLOR_PAIR(color);

    return cattr;
}


void set_darkgray(void)
{
    apply_colormap(&color_map);
}


int darken(int nh_color)
{
    /*
     * BEWARE: Check for blank glyphs before using this with darkgray,
     * otherwise the bold-black-on-black trick will instead create a completely
     * black space that can make the terminal cursor vanish while over it.
     */
    if (nh_color == CLR_GRAY) return CLR_BLACK;
    if (nh_color > NO_COLOR && nh_color < CLR_MAX) return nh_color - 8;
    return nh_color;
}

/* color.c */
