/* Copyright (c) Daniel Thaler, 2011.                             */
/* NitroHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"


static WINDOW *invwin, *objwin;
static struct nh_objitem *flooritems, *inventory;
static int floor_icount, inv_icount;


static nh_bool curses_list_items_core(struct nh_objitem *items, int icount,
			              nh_bool invent, nh_bool draw)
{
    struct nh_objitem **list;
    if (invent) {
	inv_icount = icount;
	list = &inventory;
    } else {
	floor_icount = icount;
	list = &flooritems;
    }
    
    free(*list);
    if (!icount)
	*list = NULL;
    else {
	*list = malloc(icount * sizeof(struct nh_objitem));
	memcpy(*list, items, icount * sizeof(struct nh_objitem));
    }
    
    if (draw)
	draw_sidebar();
    
    return ui_flags.draw_sidebar;
}


nh_bool curses_list_items(struct nh_objitem *items, int icount, nh_bool invent)
{
    return curses_list_items_core(items, icount, invent, TRUE);
}

nh_bool curses_list_items_nonblocking(struct nh_objitem *items, int icount, nh_bool invent)
{
    return curses_list_items_core(items, icount, invent, FALSE);
}


static int count_omitted_items(struct nh_objitem *inv, int inv_icount, int pos)
{
    int omitted = 0;
    for (; pos < inv_icount; pos++)
	if (inv[pos].role == MI_NORMAL)
	    omitted++;
    return omitted;
}


/* Inventory and floor item list sizing: Each list "owns" half the sidebar,
 * but will expand into unused space in the other half.
 * If unused space remains, the inventory list is expanded to push the
 * floor item list to the bottom of the screen. */
static void split_sidebar(int *invheight, int *flheight)
{
    *invheight = min(inv_icount+1, ui_flags.viewheight / 2);
    *flheight = min(floor_icount+1, (ui_flags.viewheight-1) / 2);

    /* if there is need and available space, expand the floor item list up */
    if (floor_icount+1 > *flheight && *invheight < (ui_flags.viewheight+1) / 2)
	*flheight = min(floor_icount+1, ui_flags.viewheight - *invheight - 1);

    /* assign all unused space to the inventory list whether it is needed or not */
    *invheight = ui_flags.viewheight - *flheight - 1;
}


void draw_sidebar_divider(void)
{
    int invheight, flheight, sbwidth;
    attr_t frame_attr;

    if (!flooritems || !inventory)
	return;

    split_sidebar(&invheight, &flheight);
    sbwidth = getmaxx(sidebar);

    frame_attr = frame_hp_color();
    wattron(sidebar, frame_attr);
    mvwhline(sidebar, invheight, 0, ACS_HLINE, sbwidth);
    wattroff(sidebar, frame_attr);
}


void draw_sidebar(void)
{
    int flheight = 0, invheight = 0, invwh, flwh;
    int sbwidth = getmaxx(sidebar);
    
    if (!ui_flags.draw_sidebar)
	return;
    
    /* re-create the subwindows every time; they only exist for use by draw_objlist */
    if (objwin)
	delwin(objwin);
    if (invwin)
	delwin(invwin);
    objwin = invwin = NULL;
    
    werase(sidebar);
    if (flooritems && inventory) {
	split_sidebar(&invheight, &flheight);
	draw_sidebar_divider();
    } else if (flooritems)
	flheight = ui_flags.viewheight;
    else
	invheight = ui_flags.viewheight;
    
    if (invheight) {
	wattron(sidebar, A_UNDERLINE);
	mvwprintw(sidebar, 0, 0, "Inventory: %d/%dwt (%d of 52 slots)",
		  player.wt, player.wtcap, player.invslots);
	wattroff(sidebar, A_UNDERLINE);
	
	invwh = invheight-1;
	if (invwh < inv_icount) {
	    invwh--;
	    mvwprintw(sidebar, invheight - 1, 0, "(%d more omitted)",
		      count_omitted_items(inventory, inv_icount, invwh));
	}
	invwin = derwin(sidebar, invwh, sbwidth, 1, 0);
	draw_objlist(invwin, inv_icount, inventory, NULL, PICK_NONE);
    }
    
    if (flheight) {
	wattron(sidebar, A_UNDERLINE);
	mvwaddstr(sidebar, invheight ? invheight + 1 : 0, 0, "Things that are here:");
	wattroff(sidebar, A_UNDERLINE);
	
	flwh = flheight - 1;
	if (flwh < floor_icount) {
	    flwh--;
	    mvwprintw(sidebar, ui_flags.viewheight - 1, 0, "(%d more omitted)",
		      count_omitted_items(flooritems, floor_icount, flwh));
	}
	
	objwin = derwin(sidebar, flwh, sbwidth, invheight ? invheight + 2 : 1, 0);
	draw_objlist(objwin, floor_icount, flooritems, NULL, PICK_NONE);
    }
    
    wnoutrefresh(sidebar);
}


void cleanup_sidebar(nh_bool dealloc)
{
    if (dealloc) {
	free(flooritems);
	free(inventory);
	flooritems = inventory = NULL;
    }
    delwin(objwin);
    delwin(invwin);
    objwin = invwin = NULL;
}

