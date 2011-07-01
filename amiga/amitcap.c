/*    SCCS Id: @(#)amitcap.c    3.0    89/07/18
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* block some unused #defines to avoid overloading some cpp's */
#define MONATTK_H

#include "hack.h"       /* for ROWNO, COLNO, *HI, *HE, *AS, *AE */

#ifdef LATTICE
#undef TRUE
#undef FALSE
#undef COUNT
#undef NULL
#include <proto/dos.h>
#endif

#ifndef LATTICE
extern void FDECL(Delay, (unsigned long));
#endif
extern void NDECL(Initialize);

static char HO[] = "\233H";         /* Home         CSI H */
static char CL[] = "\f";            /* Clear        FF */
static char CE[] = "\233K";         /* Erase EOL    CSI K */
static char UP[] = "\x0B";          /* Cursor up    VT */
static char ND[] = "\233C";         /* Cursor right CSI C */
static char XD[] = "\233B";         /* Cursor down  CSI B */
static char BC[] = "\b";            /* Cursor left  BS */
static char MR[] = "\2337m";        /* Reverse on   CSI 7 m */
static char ME[] = "\2330m";        /* Reverse off  CSI 0 m */

#ifdef TEXTCOLOR
static char SO[] = "\23337m";       /* Use colormap entry #7 (red) */
static char SE[] = "\2330m";
#else
static char SO[] = "\2337m";        /* Inverse video */
static char SE[] = "\2330m";
#endif

#ifdef TEXTCOLOR
/*
 * Map our amiga-specific colormap into the colormap specified in color.h.
 * See amiwind.c for the amiga specific colormap.
 */
static int foreg[16] = { 0, 7, 4, 2, 6, 5, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
static int backg[16] = { 1, 0, 0, 0, 0, 0, 0, 0, 1, 7, 4, 2, 6, 5, 3, 1 };
#endif

void
startup()
{
#ifdef TEXTCOLOR
    register int c;
#endif
    (void) Initialize();        /* This opens screen, window, console, &c */
    CO = COLNO;
    LI = ROWNO+3;               /* used in pri.c and pager.c */

    set_whole_screen();
    CD = "\233J";               /* used in pager.c */

#ifdef TEXTCOLOR
    /*
     * Perform amiga to color.h colormap conversion - Please note that the
     * console device can only handle 8 foreground and 8 background colors
     * while color.h defines 8 basic and 8 hilite colors.  Hilite colors
     * are handled as inverses.  For instance, a hilited green color will
     * appear as green background with a black foreground.
     */
    for (c = 0; c < SIZE(hilites); c++) {
        hilites[c] = (char *) alloc(sizeof("E0;33;44m"));
        Sprintf(hilites[c], "\2333%d;4%dm", foreg[c], backg[c]);
    }

    HI = "\2331m";              /* Bold (hilight) */
    HE = "\2330m";              /* Plain */
#else
    HI = "\2334m";              /* Underline */
    HE = "\2330m";              /* Plain */
#endif
}

void
start_screen()
{
}

void
end_screen()
{
    clear_screen();
}

/* Cursor movements */
extern xchar curx, cury;

void
curs(x, y)
register int x, y;
{
    if (x != curx || y != cury) {
        /* Test a few simple cases */
        if (x == 1) {
            if (y == cury) {
                putchar('\r');
                goto done;
            }
            if (y == cury+1) {
                putchar('\n');  /* console.device is in crmod mode */
                goto done;
            }
        } else if (x == curx) {
            if (y == cury-1) {
                putchar('\x0B');
                goto done;
            }
            if (y == cury+1) {
                xputs(XD);
                goto done;
            }
        } else if (y == cury) {
            if (x == curx-1) {
                putchar('\b');
                goto done;
            }
            if (x == curx+1) {
                xputs(ND);
                goto done;
            }
        }
        {
            static char CM[] = "\233--;--H";
            CM[1] = '0' + y/10;   /* Assumes 0 <= y < 100 */
            CM[2] = '0' + y%10;
            CM[4] = '0' + x/10;   /* Assumes 0 <= x < 100 */
            CM[5] = '0' + x%10;
            xputs(CM);
        }

done:
        cury = y;
        curx = x;
    }
}

void
cl_end()
{
    xputs(CE);
}

void
clear_screen()
{
    xputs(CL);
    home();
}

void
home()
{
    xputs(HO);
    curx = cury = 1;
}

void
standoutbeg()
{
    xputs(SO);
}

void
standoutend()
{
    xputs(SE);
}

void
revbeg()
{
        xputs(MR);
}

#if 0   /* if you need one of these, uncomment it */
void
boldbeg()
{
        xputs("\2331m");        /* CSI 1 m */
}

void
blinkbeg()
{
        /* No blink available */
}

void
dimbeg()
/* not in most termcap entries */
{
        /* No dim available, use italics */
        xputs("\2333m");        /* CSI 3 m */
}
#endif

void
m_end()
{
        xputs(ME);
}

void
backsp()
{
    xputs(BC);
}

void
bell()
{
    if (flags.silent) return;
    (void) putchar('\007');        /* curx does not change */
    (void) fflush(stdout);
}

void
delay_output() {
    /* delay 50 ms */
    (void) fflush(stdout);
    Delay(2L);
}

void
cl_eos()
{                /* must only be called with curx = 1 */
    xputs(CD);
}
