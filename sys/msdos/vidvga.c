/*   Copyright (c) NetHack PC Development Team 1995                 */
/*   NetHack may be freely redistributed.  See license for details. */
/*
 * vidvga.c - VGA Hardware video support
 */

#include "hack.h"

#ifdef SCREEN_VGA		/* this file is for SCREEN_VGA only    */
#include "pcvideo.h"
#include "tile.h"
#include "pctiles.h"

#include <dos.h>
#include <ctype.h>
#include "wintty.h"

# ifdef __GO32__
#include <pc.h>
#include <unistd.h>
# endif

/*=========================================================================
 * VGA Video supporting routines (for tiles).
 *
 * The following routines carry out the lower level video functions required
 * to make PC NetHack work with VGA graphics.
 *
 *   - The binary files NetHack1.tib and NetHacko.tib must be in your
 *     game directory.  Currently, unpredictable results may occur if they 
 *     aren't since the code hasn't been tested with it missing (yet).
 *
 * Notes (96/02/16):
 *
 *   - Cursor emulation on the map is now implemented.  The input routine
 *     in msdos.c calls the routine to display the cursor just before
 *     waiting for input, and hides the cursor immediately after satisfying 
 *     the input request.
 *
 *   - A check for a VGA adapter is implemented.
 *
 *   - With 640 x 480 resolution, the use of 16 x 16 icons allows only 40
 *     columns for the map display.  This makes it necessary to support the
 *     TTY CLIPPING code.  The right/left scrolling with this can be
 *     a little annoying.  Feel free to rework the routines.
 *
 *   - NetHack1.tib is built from text files derived from bitmap files 
 *     provided by Warwick Allison, using routines developed and supplied 
 *     by Janet Walz.  The icons are very well done and thanks to 
 *     Warwick Allison for an excellent effort!
 *
 *   - The text fonts that this is using while in graphics mode come from
 *     the Video BIOS ROM on board the VGA adapter.  Code in vga_WriteChar
 *     copies the appropriate pixels from the video ROM to the Video buffer.
 *
 *   - Currently, most of the routines are in an ifdef OVLB.
 *     If you change that, you may have to muck with some of the 
 *     variable declarations which now assume this.  This is not a 
 *     problem in a non-overlaid environment (djgpp for example).
 *
 *   - VGA 640 by 480, 16 colour mode (0x12) uses an odd method to 
 *     represent a colour value from the palette. There are four 
 *     planes of video memory, all overlaid at the same memory location.
 *     For example, if a pixel has the colour value 7:
 *
 *           0 1 1 1
 *            \ \ \ \
 *             \ \ \ plane 0 
 *              \ \ plane 1
 *               \ plane 2
 *                plane 3
 *
 *   - VGA write mode 2 requires that a read be done before a write to
 *     set some latches on the card.  The value read had to be placed
 *     into a variable declared 'volatile' to prevent djgpp from
 *     optimizing away the entire instruction (the value was assigned
 *     to a variable which was never used).  This corrects the striping
 *     problem that was apparent with djgpp.
 *
 *   - A check for valid mode switches has been added.
 *
 *   - No tiles are displayed on the Rogue Level in keeping with the
 *     original Rogue.  The display adapter remains in graphics mode 
 *     however.
 *
 *   - Added handling for missing NetHackX.tib files, and resort to using
 *     video:default and tty if one of them can't be located.
 *
 * ToDo (96/02/17):
 *
 *   - Nothing prior to release.
 *=========================================================================
 */


# if defined(_MSC_VER)
#  if _MSC_VER >= 700
#pragma warning(disable:4018)	/* signed/unsigned mismatch */
#pragma warning(disable:4127)	/* conditional expression is constant */
#pragma warning(disable:4131)	/* old style declarator */
#pragma warning(disable:4305)	/* prevents complaints with MK_FP */
#pragma warning(disable:4309)	/* initializing */
#    if _MSC_VER > 700
#pragma warning(disable:4759)	/* prevents complaints with MK_FP */
#    endif
#  endif
# include <conio.h>
# endif

/* STATIC_DCL void FDECL(vga_NoBorder, (int));  */
void FDECL(vga_gotoloc, (int,int));  /* This should be made a macro */
void NDECL(vga_backsp);
#ifdef SCROLLMAP
STATIC_DCL void FDECL(vga_scrollmap,(bool));
#endif
STATIC_DCL void FDECL(vga_redrawmap,(bool));
void FDECL(vga_cliparound,(int, int));
STATIC_OVL void FDECL(decal_planar,(struct planar_cell_struct *, unsigned));

#ifdef POSITIONBAR
STATIC_DCL void NDECL(positionbar);
static void FDECL(vga_special,(int, int, int));
#endif

extern int clipx, clipxmax;	/* current clipping column from wintty.c */
extern bool clipping;	/* clipping on? from wintty.c */
extern int savevmode;		/* store the original video mode */
extern int curcol,currow;	/* current column and row        */
extern int g_attribute;
extern int attrib_text_normal;	/* text mode normal attribute */
extern int attrib_gr_normal;	/* graphics mode normal attribute */
extern int attrib_text_intense;	/* text mode intense attribute */
extern int attrib_gr_intense;	/* graphics mode intense attribute */
extern bool inmap;		/* in the map window */

/*
 * Global Variables
 */ 

STATIC_VAR unsigned char __far *font;
STATIC_VAR char *screentable[SCREENHEIGHT];

STATIC_VAR char *paletteptr;
STATIC_VAR struct map_struct {
	int glyph;
	int ch;
	int attr;
	unsigned special;
}  map[ROWNO][COLNO];	/* track the glyphs */

# define vga_clearmap() { int x,y; for (y=0; y < ROWNO; ++y) \
	for (x=0; x < COLNO; ++x) { map[y][x].glyph = cmap_to_glyph(S_stone); \
	map[y][x].ch = S_stone; map[y][x].attr = 0; map[y][x].special = 0;} }
# define TOP_MAP_ROW 1
#  if defined(OVLB)
STATIC_VAR int vgacmap[CLR_MAX] = {0,3,5,9,4,8,12,14,11,2,6,7,1,8,12,13};
STATIC_VAR int viewport_size = 40;
/* STATIC_VAR char masktable[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01}; */
/* STATIC_VAR char bittable[8]= {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80}; */
#if 0
STATIC_VAR char defpalette[] = {	/* Default VGA palette         */
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xaa,
	0x00, 0xaa, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0x00,
	0xaa, 0x00, 0xaa,
	0xaa, 0xaa, 0x00,
	0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55,
	0xcc, 0xcc, 0xcc,
	0x00, 0x00, 0xff,
	0x00, 0xff, 0x00,
	0xff, 0x00, 0x00,
	0xff, 0xff, 0x00,
	0xff, 0x00, 0xff,
	0xff, 0xff, 0xff
	};
#endif

#   ifndef ALTERNATE_VIDEO_METHOD
int vp[SCREENPLANES] = {8,4,2,1};
#   endif
int vp2[SCREENPLANES] = {1,2,4,8};
#  else
extern int vgacmap[CLR_MAX];
extern int viewport_size;
extern char masktable[8];
extern char bittable[8];
extern char defpalette[];
#   ifndef ALTERNATE_VIDEO_METHOD
extern int vp[SCREENPLANES];
#   endif
extern int vp2[SCREENPLANES];
#  endif /* OVLB */

STATIC_VAR struct planar_cell_struct *planecell;
STATIC_VAR struct overview_planar_cell_struct *planecell_O;

# if defined(USE_TILES)
STATIC_VAR struct tibhdr_struct tibheader;
/* extern FILE *tilefile; */ /* Not needed in here most likely */
# endif

/* STATIC_VAR int  g_attribute;	*/	/* Current attribute to use */

#ifdef OVLB
void vga_get_scr_size(void)
{
	CO = 80;
	LI = 29;
}
#endif /*OVLB*/



# ifdef OVLB

void vga_backsp(void)
{
	int col,row;

	col = curcol; 		/* Character cell row and column */
	row = currow;

	if (col > 0) col = col-1;
	vga_gotoloc(col,row);
}

# endif /* OVLB */
# ifdef OVL0

void vga_clear_screen(int colour)
{
	char __far *pch;
	int y,j;
	char volatile a;

	outportb(0x3ce,5);
	outportb(0x3cf,2);

	for (y=0; y < SCREENHEIGHT; ++y) {
		pch = screentable[y];
		for (j=0; j < SCREENBYTES; ++j) {
			outportb(0x3ce,8);
			outportb(0x3cf,255);
			a = READ_ABSOLUTE(pch); /* Must read , then write */
			WRITE_ABSOLUTE(pch, (char)colour);
			++pch;
		}
	}
	outportb(0x3ce,5);
	outportb(0x3cf,0);
	if (iflags.tile_view) vga_clearmap();
	vga_gotoloc(0,0);	/* is this needed? */
}

void vga_cl_end(	/* clear to end of line */
	int col,
	int row
)
{
	int count;

	/*
	 * This is being done via character writes.
	 * This should perhaps be optimized for speed by using VGA write
	 * mode 2 methods as did clear_screen()
	 */
	for (count = col; count < (CO-1); ++count) {
		vga_WriteChar(' ',count,row,BACKGROUND_VGA_COLOR);
	}
}

void vga_cl_eos(	/* clear to end of screen */
	int cy
)
{
	int count;

	cl_end();
	while(cy <= LI-2) {
		for (count = 0; count < (CO-1); ++count) {
			vga_WriteChar(' ',count,cy,
				BACKGROUND_VGA_COLOR);
		}
		cy++;
	}
}


# endif /* OVL0 */

# ifdef OVLB
void vga_tty_end_screen(void)
{
	vga_clear_screen(BACKGROUND_VGA_COLOR);
	vga_SwitchMode(MODETEXT);
}


void vga_tty_startup(int *wid, int *hgt)
{

	/* code to sense display adapter is required here - MJA */

	vga_get_scr_size();
	if (CO && LI) {
		*wid = CO;
		*hgt = LI;
	}

	attrib_gr_normal    = ATTRIB_VGA_NORMAL;
	attrib_gr_intense   = ATTRIB_VGA_INTENSE;
	g_attribute         = attrib_gr_normal;	/* Give it a starting value */
}
# endif /* OVLB */

/*
 * Screen output routines (these are heavily used).
 *
 * These are the 3 routines used to place information on the screen
 * in the VGA PC tty port of NetHack.  These are the routines
 * that get called by the general interface routines in video.c.
 *
 * vga_xputs -Writes a c null terminated string at the current location.
 *
 * vga_xputc -Writes a single character at the current location. Since
 *            various places in the code assume that control characters
 *            can be used to control, we are forced to interpret some of
 *            the more common ones, in order to keep things looking correct.
 *
 * vga_xputg -This routine is used to display a graphical representation of a
 *            NetHack glyph (a tile) at the current location.  For more
 *            information on NetHack glyphs refer to the comments in
 *            include/display.h.
 *
 */

# ifdef OVL0
void vga_xputs(const char *s, int col, int row)
{

	if (s != (char *)0) {
		vga_WriteStr((char *)s,strlen(s),col,row,g_attribute);
	}
}

void vga_xputc(	/* write out character (and attribute) */
	int ch,
	int attr
)
{
	int col,row;

	col = curcol;
	row = currow;

	switch(ch) {
	    case '\n':	
			col = 0;
			++row;
			break;
	    default:
			vga_WriteChar((unsigned char)ch,col,row,attr);
			if (col < (CO -1 )) ++col;
			break;
	} /* end switch */
	vga_gotoloc(col,row);
}

#  if defined(USE_TILES)
void vga_xputg(	/* Place tile represent. a glyph at current location */
	int glyphnum,
	int ch,
	unsigned special	/* special feature: corpse, invis, detected, pet, ridden - hack.h */
)
{
	int col,row;
	int attr;
	int ry;

	row = currow;
	col = curcol;
	if ((col < 0 || col >= COLNO) ||
	    (row < TOP_MAP_ROW || row >= (ROWNO + TOP_MAP_ROW))) return;
	ry = row - TOP_MAP_ROW;
	map[ry][col].glyph = glyphnum;
	map[ry][col].ch = ch;
	map[ry][col].special = special;
	attr = (g_attribute == 0) ? attrib_gr_normal : g_attribute;
	map[ry][col].attr = attr;
	if (iflags.traditional_view) {
	    vga_WriteChar((unsigned char)ch,col,row,attr);
	} else if (!iflags.over_view) {
	    if ((col >= clipx) && (col <= clipxmax)) {
		if (!ReadPlanarTileFile(glyph2tile[glyphnum], &planecell)) {
			if (map[ry][col].special) decal_planar(planecell, special);
			vga_DisplayCell(planecell, 
					col - clipx, row);
		} else
			pline("vga_xputg: Error reading tile (%d,%d) from file",
					glyphnum,glyph2tile[glyphnum]);
	    }
	} else {
	    if (!ReadPlanarTileFile_O(glyph2tile[glyphnum], &planecell_O))
			vga_DisplayCell_O(planecell_O, col, row);
	    else
			pline("vga_xputg: Error reading tile (%d,%d) from file",
					glyphnum,glyph2tile[glyphnum]);
	}
	if (col < (CO - 1 )) ++col;
	vga_gotoloc(col,row);
}
#  endif /* USE_TILES */

/*
 * Cursor location manipulation, and location information fetching
 * routines.
 * These include:
 *
 * vga_gotoloc(x,y)     - Moves the "cursor" on screen to the specified x
 *			 and y character cell location.  This routine
 *                       determines the location where screen writes
 *                       will occur next, it does not change the location
 *                       of the player on the NetHack level.
 */
 
void vga_gotoloc(int col, int row)
{
	curcol = min(col,CO - 1); /* protection from callers */
	currow = min(row,LI - 1);
}

#  if defined(USE_TILES) && defined(CLIPPING)
void vga_cliparound(int x, int y)
{
	extern bool restoring;
	int oldx = clipx;

	if (!iflags.tile_view || iflags.over_view || iflags.traditional_view)
		return;

	if (x < clipx + 5) {
		clipx = max(0, x - (viewport_size / 2));
		clipxmax = clipx + (viewport_size - 1);
	}
	else if (x > clipxmax - 5) {
		clipxmax = min(COLNO - 1, x + (viewport_size / 2));
		clipx = clipxmax - (viewport_size - 1);
	}
	if (clipx != oldx) {
	    if (on_level(&u.uz0, &u.uz) && !restoring)
		/* (void) doredraw(); */
		vga_redrawmap(1);
	}
}

STATIC_OVL void vga_redrawmap(bool clearfirst)
{
	int j,x,y,t;
	char __far *pch;
	char volatile a;
	
	if (clearfirst) {
		/* y here is in pixel rows */
		outportb(0x3ce,5);
		outportb(0x3cf,2);
		t = TOP_MAP_ROW * ROWS_PER_CELL;
		for (y = t; y < (ROWNO * ROWS_PER_CELL) + t; ++y) {
			pch = screentable[y];
			for (j=0; j < SCREENBYTES; ++j) {
				outportb(0x3ce,8);
				outportb(0x3cf,255);
				 /* On VGA mode2, must read first, then write */
				a = READ_ABSOLUTE(pch);
				WRITE_ABSOLUTE(pch, (char)BACKGROUND_VGA_COLOR);
				++pch;
			}
		}
		outportb(0x3ce,5);
		outportb(0x3cf,0);
	}
	/* y here is in screen rows*/
#    ifdef ROW_BY_ROW
	for (y = 0; y < ROWNO; ++y)
		for (x = clipx; x <= clipxmax; ++x) {
#    else
	for (x = clipx; x <= clipxmax; ++x)
		for (y = 0; y < ROWNO; ++y) {
#    endif
		    if (iflags.traditional_view) {
			if (!(clearfirst && map[y][x].ch == S_stone))
				vga_WriteChar(
					(unsigned char)map[y][x].ch,
					x,y + TOP_MAP_ROW,map[y][x].attr);
		    } else {
		      t = map[y][x].glyph;
		      if (!(clearfirst && t == cmap_to_glyph(S_stone))) {
			if (!iflags.over_view) {
			  	if (!ReadPlanarTileFile(glyph2tile[t], 
				    &planecell)) {
				    	if (map[y][x].special)
						decal_planar(planecell, map[y][x].special);
					vga_DisplayCell(planecell,
						x - clipx, y + TOP_MAP_ROW);
		  	  	} else
			      pline("vga_redrawmap: Error reading tile (%d,%d)",
					 t,glyph2tile[t]);
		     	} else {
				if (!ReadPlanarTileFile_O(glyph2tile[t], 
				     &planecell_O)) {
					vga_DisplayCell_O(planecell_O,
						x, y + TOP_MAP_ROW);
		  	  	} else
			     pline("vga_redrawmap: Error reading tile (%d,%d)",
					t,glyph2tile[t]);
		  	}
		      }
		    }
		}
}
#  endif /* USE_TILES && CLIPPING */
# endif /* OVL0 */
# ifdef OVL2

void
vga_userpan(left)
bool left;
{
	int x;

/*	pline("Into userpan"); */
	if (iflags.over_view || iflags.traditional_view) return;
	if (left)
		x = min(COLNO - 1, clipxmax + 10);
	else 
		x = max(0, clipx - 10);
	vga_cliparound(x, 10);	/* y value is irrelevant on VGA clipping */
	positionbar();
	vga_DrawCursor();
}


void vga_overview(on)
bool on;
{
/*	vga_HideCursor(); */
	if (on) {
		iflags.over_view = TRUE;
		clipx = 0;
		clipxmax = CO - 1;
	} else {
		iflags.over_view = FALSE;
		clipx = max(0, (curcol - viewport_size / 2));
		if (clipx > ((CO - 1) - viewport_size)) 
			clipx = (CO - 1) - viewport_size;
     		clipxmax = clipx + (viewport_size - 1);
	}
}

void vga_traditional(on)
bool on;
{
/*	vga_HideCursor(); */
	if (on) {
/*		switch_graphics(ASCII_GRAPHICS); */
		iflags.traditional_view = TRUE;
		clipx = 0;
		clipxmax = CO - 1;
	} else {
		iflags.traditional_view = FALSE;
		if (!iflags.over_view) {
			clipx = max(0, (curcol - viewport_size / 2));
			if (clipx > ((CO - 1) - viewport_size)) 
				clipx = (CO - 1) - viewport_size;
     			clipxmax = clipx + (viewport_size - 1);
		}
	}
}

void vga_refresh()
{
	positionbar();
	vga_redrawmap(1);
	vga_DrawCursor();
}

#  ifdef SCROLLMAP
STATIC_OVL void
vga_scrollmap(left)
bool left;
{
	int j,x,y,t;
	int i,pixx,pixy,x1,y1,x2,y2;
	int byteoffset, vplane;
	char __far *tmp1;
	char __far *tmp2;
	unsigned char source[SCREENPLANES][80];
	unsigned char first,second;

	
	pixy = row2y(TOP_MAP_ROW);		  /* convert to pixels */
	pixx = col2x(x1);
	if (left) {
		x1 = 20;
		x2 = 0;
	} else {
		x1 = 0;
		x2 = 20;
	}
	/* read each row, all columns but the one to be replaced */
	for(i = 0;i < (ROWNO-1) * ROWS_PER_CELL; ++i) {
	    tmp1 = screentable[i + pixy];
	    tmp1 += x1;
	    for(vplane=0; vplane < SCREENPLANES; ++vplane) {
		egareadplane(vplane);
		for (byteoffset = 0; byteoffset < 20; ++byteoffset) {
			tmp2 = tmp1 + byteoffset;
			source[vplane][byteoffset] = READ_ABSOLUTE(tmp2);
		}
	    }
	    tmp1 = screentable[i + pixy];
	    tmp1 += x2;
	    for(vplane=0; vplane < SCREENPLANES; ++vplane) {
		egawriteplane(vp2[vplane]);
		for (byteoffset = 0; byteoffset < 20; ++byteoffset) {
			tmp2 = tmp1 + byteoffset;
			WRITE_ABSOLUTE(tmp2,source[vplane][byteoffset]);
		}
	    }
	    egawriteplane(15);
	}

	if (left) {
		i = clipxmax - 1;
		j = clipxmax;
	} else {
		i = clipx;
		j = clipx + 1;
	}
	for (y = 0; y < ROWNO; ++y) {
	    for (x = i; x < j; x += 2) {
		t = map[y][x].glyph;
		if (!ReadPlanarTileFile(glyph2tile[t], &planecell))
			if (map[y][x].special) decal_planar(planecell, map[y][x].special);
			vga_DisplayCell(planecell, x - clipx, y + TOP_MAP_ROW);
		else
			pline("vga_shiftmap: Error reading tile (%d,%d)",
				t, glyph2tile[t]);		
	    }
	}
}
#   endif /* SCROLLMAP */
# endif /* OVL2 */

# ifdef OVLB
STATIC_OVL void
decal_planar(gp, special)
struct planar_cell_struct *gp;
unsigned special;
{
    if (special & MG_CORPSE) {
    } else if (special & MG_INVIS)  {
    } else if (special & MG_DETECT) {
    } else if (special & MG_PET)    {
    } else if (special & MG_RIDDEN) {
    }
}

/*
 * Open tile files,
 * initialize the SCREEN, switch it to graphics mode,
 * initialize the pointers to the fonts, clear
 * the screen.
 *
 */
void vga_Init(void)
{
     int i;

#   ifdef USE_TILES
     int tilefailure = 0;
/*
 * Attempt to open the required tile files. If we can't
 * don't perform the video mode switch, use TTY code instead.
 *
 */
     if (OpenTileFile(NETHACK_PLANAR_TILEFILE, FALSE)) tilefailure |= 1;
     if (OpenTileFile(NETHACK_OVERVIEW_TILEFILE, TRUE)) tilefailure |= 2;     	
     if (ReadTileFileHeader(&tibheader, FALSE)) tilefailure |= 4;

     if (tilefailure) {
	raw_printf("Reverting to TTY mode, tile initialization failure (%d).",
		tilefailure);
	wait_synch();
	iflags.usevga = 0;
	iflags.tile_view = FALSE;
	iflags.over_view = FALSE;
	CO = 80;
	LI = 25;
/*	clear_screen()	*/ /* not vga_clear_screen() */
	return;
     }
#   endif

     if (iflags.usevga) {
	for (i=0; i < SCREENHEIGHT; ++i) {
		screentable[i]=MK_PTR(VIDEOSEG, (i * SCREENBYTES));
	}
     }
     vga_SwitchMode(MODE640x480);
     windowprocs.win_cliparound = vga_cliparound;
/*     vga_NoBorder(BACKGROUND_VGA_COLOR); */  /* Not needed after palette mod */
#   ifdef USE_TILES
     paletteptr = tibheader.palette;
     iflags.tile_view = TRUE;
     iflags.over_view = FALSE;
#   else
     paletteptr = defpalette;
#   endif
     vga_SetPalette(paletteptr);
     g_attribute  = attrib_gr_normal;
     font = vga_FontPtrs();
     clear_screen();
     clipx = 0;
     clipxmax = clipx + (viewport_size - 1);
}

/*
 * Switches modes of the video card.
 *
 * If mode == MODETEXT (0x03), then the card is placed into text
 * mode.  If mode == 640x480, then the card is placed into vga
 * mode (video mode 0x12). No other modes are currently supported.
 *
 */
void vga_SwitchMode(unsigned int mode)
{
	union REGS regs;

	if ((mode == MODE640x480) || (mode == MODETEXT)) {
		if (iflags.usevga && (mode == MODE640x480)) {
     			iflags.grmode = 1;
		} else {
	        	iflags.grmode = 0;
		}
		regs.x.ax = mode;
		(void) int86(VIDEO_BIOS, &regs, &regs);
	} else {
		iflags.grmode = 0;	/* force text mode for error msg */
		regs.x.ax = MODETEXT;
		(void) int86(VIDEO_BIOS, &regs, &regs);
		g_attribute  = attrib_text_normal;
		impossible("vga_SwitchMode: Bad video mode requested 0x%X",
			mode);
	}
}

/*
 * This allows grouping of several tasks to be done when
 * switching back to text mode. This is a public (extern) function.
 *
 */
void vga_Finish(void)
{
     CloseTileFile(0);
     CloseTileFile(1);
     vga_SwitchMode(MODETEXT);
     windowprocs.win_cliparound = tty_cliparound;
     g_attribute  = attrib_text_normal;
     iflags.tile_view = FALSE;
}

#if 0
/*
 * Turn off any border colour that might be enabled in the VGA card
 * register.
 *
 * I disabled this after modifying tile2bin.c to remap black & white
 * to a more standard values - MJA 94/04/23.
 *
 */
STATIC_OVL void 
vga_NoBorder(int bc)
{
	union REGS regs;

	regs.h.ah = (char)0x10;
	regs.h.al = (char)0x01;
	regs.h.bh = (char)bc;
	regs.h.bl = 0;
	(void) int86(VIDEO_BIOS, &regs, &regs);	
}
#endif

/*
 * 
 * Returns a far pointer (or flat 32 bit pointer under djgpp) to the
 * location of the appropriate ROM font for the _current_ video mode
 * (so you must place the card into the desired video mode before
 * calling this function).
 *
 * This function takes advantage of the video BIOS loading the
 * address of the appropriate character definition table for
 * the current graphics mode into interrupt vector 0x43 (0000:010C).
 */
char __far  *vga_FontPtrs(void)
{
	USHORT  __far *tmp;
	char __far *retval;
	USHORT fseg, foff;
	tmp  = (USHORT __far *)MK_PTR(((USHORT)FONT_PTR_SEGMENT),
					((USHORT)FONT_PTR_OFFSET));
	foff = READ_ABSOLUTE_WORD(tmp);
	++tmp;
	fseg = READ_ABSOLUTE_WORD(tmp);
	retval = (char __far *)MK_PTR(fseg,foff);
	return retval;
}

/*
 * This will verify the existance of a VGA adapter on the machine.
 * Video function call 0x1a returns 0x1a in AL if successful, and
 * returns the following values in BL for the active display:
 *
 * 0=no display, 1=MDA, 2=CGA, 4=EGA(color-monitor), 
 * 5=EGA(mono-monitor), 6=PGA, 7=VGA(mono-monitor), 8=VGA(color-monitor),
 * 0xB=MCGA(mono-monitor), 0xC=MCGA(color-monitor), 0xFF=unknown)
 */
int vga_detect()
{
	union REGS regs;
	
	regs.h.al = 0;
	regs.h.ah = 0x1a;
	(void) int86(VIDEO_BIOS, &regs, &regs);
/*
 * debug
 *
 *	printf("vga_detect returned al=%02x, bh=%02x, bl=%02x\n",
 *			(int)regs.h.al, (int)regs.h.bh, (int)regs.h.bl);
 *	getch();
 */
	if ((int)regs.h.al == 0x1a) { 
		if (((int)regs.h.bl == 8) || ((int)regs.h.bl == 7)) {
			return 1;
		}
	}
	return 0;
}

/*
 * Write character 'ch', at (x,y) and
 * do it using the colour 'colour'.
 *
 */
void 
vga_WriteChar(chr,col,row,colour)
int chr,col,row,colour;
{
	int i;
	int x,pixy;

	char volatile tc;
	char __far *cp;
	unsigned char __far *fp = font;
	unsigned char fnt;
	int actual_colour = vgacmap[colour];


	x = min(col,(CO-1));	       /* min() used protection from callers */
	pixy = min(row,(LI-1)) * 16; /* assumes 8 x 16 char set */
/*	if (chr < ' ') chr = ' ';  */  /* assumes ASCII set */

	outportb(0x3ce,5);
	outportb(0x3cf,2);
			
	chr = chr<<4;
	for (i=0; i < MAX_ROWS_PER_CELL; ++i) {
		cp = screentable[pixy+i] + x;
		fnt = READ_ABSOLUTE((fp + chr + i));
		outportb(0x3ce,8);
		outportb(0x3cf,fnt);
		tc = READ_ABSOLUTE(cp);	/* wrt mode 2, must read, then write */
		WRITE_ABSOLUTE(cp, (char)actual_colour);
		outportb(0x3ce,8);
		outportb(0x3cf,~fnt);
		tc = READ_ABSOLUTE(cp);	/* wrt mode 2, must read, then write */
		WRITE_ABSOLUTE(cp, (char)BACKGROUND_VGA_COLOR);
	}
	outportb(0x3ce,5);
	outportb(0x3cf,0);
	outportb(0x3ce,8);
	outportb(0x3cf,255);
}

/*
 * This is the routine that displays a high-res "cell" pointed to by 'gp'
 * at the desired location (col,row).
 *
 * Note: (col,row) in this case refer to the coordinate location in
 * NetHack character grid terms, (ie. the 40 x 25 character grid),
 * not the x,y pixel location.
 *
 */
void
vga_DisplayCell(gp,col,row)
struct planar_cell_struct *gp;
int col,row;
{
	int i,pixx,pixy;
	char __far *tmp_s;	/* source pointer */
	char __far *tmp_d;	/* destination pointer */
	int vplane;

	pixy = row2y(row);		/* convert to pixels */
	pixx = col2x(col);
	for(vplane=0; vplane < SCREENPLANES; ++vplane) {
		egawriteplane(vp[vplane]);
		for(i=0;i < ROWS_PER_CELL; ++i) {
			tmp_d = screentable[i+pixy];
 			tmp_d += pixx;
		/*
		 * memcpy((void *)tmp,(void *)gp->plane[vplane].image[i],
		 *         BYTES_PER_CELL);
		 */
			tmp_s = gp->plane[vplane].image[i];
			WRITE_ABSOLUTE(tmp_d, (*tmp_s));
			++tmp_s; ++tmp_d;
			WRITE_ABSOLUTE(tmp_d, (*tmp_s));
		}
	}
	egawriteplane(15);
}

void
vga_DisplayCell_O(gp,col,row)
struct overview_planar_cell_struct *gp;
int col,row;
{
	int i,pixx,pixy;
	char __far *tmp_s;	/* source pointer */
	char __far *tmp_d;	/* destination pointer */
	int vplane;

	pixy = row2y(row);		/* convert to pixels */
	pixx = col;
	for(vplane=0; vplane < SCREENPLANES; ++vplane) {
		egawriteplane(vp[vplane]);
		for(i=0;i < ROWS_PER_CELL; ++i) {
			tmp_d = screentable[i+pixy];
 			tmp_d += pixx;
		/*
		 * memcpy((void *)tmp,(void *)gp->plane[vplane].image[i],
		 *         BYTES_PER_CELL);
		 */
			tmp_s = gp->plane[vplane].image[i];
			WRITE_ABSOLUTE(tmp_d, (*tmp_s));
		}
	}
	egawriteplane(15);
}

/*
 * Write the character string pointed to by 's', whose maximum length
 * is 'len' at location (x,y) using the 'colour' colour.
 *
 */
void 
vga_WriteStr(s,len,col,row,colour)
char *s;
int len,col,row,colour;
{
	unsigned char *us;
	int i = 0;

	/* protection from callers */
	if (row > (LI-1)) return;

	i  = 0;
	us = (unsigned char *)s;
	while( (*us != 0) && (i < len) && (col < (CO - 1))) {
		vga_WriteChar(*us,col,row,colour);
		++us;
		++i;
		++col;
	}
}

# endif /* OVLB */


# ifdef OVLB
/*
 * Initialize the VGA palette with the desired colours. This
 * must be a series of 48 bytes for use with a card in
 * 16 colour mode at 640 x 480.
 *
 */
void
vga_SetPalette(p)
	char *p;
{
	union REGS regs;
	int i;

	outportb(0x3c6,0xff);
	for(i=0;i < COLORDEPTH; ++i) {
		outportb(0x3c8,i);
		outportb(0x3c9,(*p++) >> 2);
		outportb(0x3c9,(*p++) >> 2);
		outportb(0x3c9,(*p++) >> 2);
	}
	regs.x.bx = 0x0000;
	for(i=0;i < COLORDEPTH; ++i) {
		regs.x.ax = 0x1000;
		(void) int86(VIDEO_BIOS,&regs,&regs);
		regs.x.bx += 0x0101;
	}
}

/*static unsigned char colorbits[]={0x01,0x02,0x04,0x08}; */ /* wrong */
static unsigned char colorbits[]={0x08,0x04,0x02,0x01}; 

#ifdef POSITIONBAR

#define PBAR_ROW (LI - 4)
#define PBAR_COLOR_ON	  15	/* slate grey background colour of tiles */
#define PBAR_COLOR_OFF	  12	/* bluish grey, used in old style only */
#define PBAR_COLOR_STAIRS  9	/* brown */
#define PBAR_COLOR_HERO   14	/* creamy white */

static unsigned char pbar[COLNO];

void 
vga_update_positionbar(posbar)
char *posbar;
{
	char *p = pbar;
	if (posbar) while (*posbar) *p++ = *posbar++;
	*p = 0;
}

STATIC_OVL void 
positionbar()
{
	char *posbar = pbar;
	int feature, ucol;
	int k, y, colour, row;
	char __far *pch;

	int startk, stopk;
	char volatile a;
	bool nowhere = FALSE;
	int pixy = (PBAR_ROW * MAX_ROWS_PER_CELL);
	int tmp;

	if (!iflags.grmode || !iflags.tile_view) return;
	if ((clipx < 0)  || (clipxmax <= 0) || (clipx >= clipxmax)) 
		nowhere = TRUE;
	if (nowhere) {
#ifdef DEBUG
		pline("Would have put bar using %d - %d.",clipx,clipxmax);
#endif
		return;
        }
#ifdef OLD_STYLE
	outportb(0x3ce,5);
	outportb(0x3cf,2);
	for (y=pixy; y < (pixy + MAX_ROWS_PER_CELL); ++y) {
		pch = screentable[y];
		for (k=0; k < SCREENBYTES; ++k) {
			if ((k < clipx) || (k > clipxmax)) {
				colour = PBAR_COLOR_OFF;			
			} else colour = PBAR_COLOR_ON;
			outportb(0x3ce,8);
			outportb(0x3cf,255);
			a = READ_ABSOLUTE(pch); /* Must read , then write */
			WRITE_ABSOLUTE(pch, (char)colour);
			++pch;
		}
	}
	outportb(0x3ce,5);
	outportb(0x3cf,0);
#else
	colour = PBAR_COLOR_ON;
	outportb(0x3ce,5);
	outportb(0x3cf,2);
	for (y=pixy, row = 0; y < (pixy + MAX_ROWS_PER_CELL); ++y, ++row) {
		pch = screentable[y];
		if ((!row) || (row == (ROWS_PER_CELL-1))) {
			startk = 0;
			stopk  = SCREENBYTES;
		} else {
			startk = clipx;
			stopk  = clipxmax;
		}
		for (k=0; k < SCREENBYTES; ++k) {
			if ((k < startk) || (k > stopk))
				colour = BACKGROUND_VGA_COLOR;
			else
				colour = PBAR_COLOR_ON;
			outportb(0x3ce,8);
			outportb(0x3cf,255);
			a = READ_ABSOLUTE(pch); /* Must read , then write */
			WRITE_ABSOLUTE(pch, (char)colour);
			++pch;
		}
	}
	outportb(0x3ce,5);
	outportb(0x3cf,0);
#endif
	ucol = 0;
	if (posbar) {
	    while (*posbar != 0) {
		feature = *posbar++;
		switch (feature) {
		    case '>':
			vga_special(feature, (int)*posbar++, PBAR_COLOR_STAIRS);
			break;
		    case '<':
			vga_special(feature, (int)*posbar++, PBAR_COLOR_STAIRS);
			break;
		    case '@':
			ucol = (int)*posbar++;
			vga_special(feature, ucol, PBAR_COLOR_HERO);
			break;
		    default: /* unanticipated symbols */
			vga_special(feature, (int)*posbar++, PBAR_COLOR_STAIRS);
			break;
		}
	    }
	}
#  ifdef SIMULATE_CURSOR
	if (inmap) {
		tmp = curcol + 1;
		if ((tmp != ucol) && (curcol >= 0))	
			vga_special('_', tmp, PBAR_COLOR_HERO);
	}
#  endif
}

void
vga_special(chr,col,color)
int chr,col,color;
{
	int i,y,pixy;
	char __far *tmp_d;	/* destination pointer */
	int vplane;
	char fnt;
	char bits[SCREENPLANES][ROWS_PER_CELL];

	pixy = PBAR_ROW * MAX_ROWS_PER_CELL;
	for(vplane=0; vplane < SCREENPLANES; ++vplane) {
		egareadplane(vplane);
		y = pixy;
		for(i=0;i < ROWS_PER_CELL; ++i) {
			tmp_d = screentable[y++] + col;
			bits[vplane][i] = READ_ABSOLUTE(tmp_d);
			fnt = READ_ABSOLUTE((font + ((chr<<4) + i)));
			if (colorbits[vplane] & color)
				bits[vplane][i] |= fnt;
			else
				bits[vplane][i] &= ~fnt;
		}
	}
	for(vplane=0; vplane < SCREENPLANES; ++vplane) {
		egawriteplane(vp[vplane]);
		y = pixy;
		for(i=0;i < ROWS_PER_CELL; ++i) {
			tmp_d = screentable[y++] + col;
			WRITE_ABSOLUTE(tmp_d, (bits[vplane][i]));
		}
     	}
	egawriteplane(15);
}

#  endif POSITIONBAR

#  ifdef SIMULATE_CURSOR

static struct planar_cell_struct undercursor;
static struct planar_cell_struct cursor;

void
vga_DrawCursor()
{
	int i,pixx,pixy,x,y,p;
	char __far *tmp1;
	char __far *tmp2;
	unsigned char first,second;
/*	char on[2] =  {0xFF,0xFF}; */
/*	char off[2] = {0x00,0x00}; */
#ifdef REINCARNATION
	bool isrogue = Is_rogue_level(&u.uz);
	bool singlebyte = (isrogue || iflags.over_view
			      || iflags.traditional_view || !inmap);
#else
	bool singlebyte = (iflags.over_view
			      || iflags.traditional_view || !inmap);
#endif
	int curtyp;

	if (!cursor_type && inmap) return;	/* CURSOR_INVIS - nothing to do */

	x = min(curcol,(CO - 1)); /* protection from callers */
	y = min(currow,(LI - 1));		  /* protection from callers */
	if (!singlebyte && ((x < clipx) || (x > clipxmax))) return;
	    pixy = row2y(y);		  /* convert to pixels */
	    if (singlebyte)
		    pixx = x;
	    else
		    pixx = col2x((x-clipx));

	    for(i=0;i < ROWS_PER_CELL; ++i) {
		tmp1 = screentable[i+pixy];
 		tmp1 += pixx;
 		tmp2 = tmp1 + 1;
		egareadplane(3);
		/* memcpy(undercursor.plane[3].image[i],tmp1,BYTES_PER_CELL); */
		undercursor.plane[3].image[i][0] = READ_ABSOLUTE(tmp1);
		if (!singlebyte)
			undercursor.plane[3].image[i][1] = READ_ABSOLUTE(tmp2);

		egareadplane(2);
		/* memcpy(undercursor.plane[2].image[i],tmp1,BYTES_PER_CELL); */
		undercursor.plane[2].image[i][0] = READ_ABSOLUTE(tmp1);
		if (!singlebyte)
			undercursor.plane[2].image[i][1] = READ_ABSOLUTE(tmp2);

		egareadplane(1);
		/* memcpy(undercursor.plane[1].image[i],tmp1,BYTES_PER_CELL); */
		undercursor.plane[1].image[i][0] = READ_ABSOLUTE(tmp1);
		if (!singlebyte)
			undercursor.plane[1].image[i][1] = READ_ABSOLUTE(tmp2);

		egareadplane(0);
		/* memcpy(undercursor.plane[0].image[i],tmp1,BYTES_PER_CELL); */
		undercursor.plane[0].image[i][0] = READ_ABSOLUTE(tmp1);
		if (!singlebyte)
			undercursor.plane[0].image[i][1] = READ_ABSOLUTE(tmp2);
	    }

	    /*
             * Now we have a snapshot of the current cell.
             * Make a copy of it, then manipulate the copy
             * to include the cursor, and place the tinkered
             * version on the display.
             */ 

	    cursor = undercursor;
	    if (inmap) curtyp = cursor_type;
	    else curtyp = CURSOR_UNDERLINE;

	    switch(curtyp) {

		case CURSOR_CORNER:
		    for(i = 0; i < 2; ++i) {
			if (!i) {
				if (singlebyte) first = 0xC3;
				else first  = 0xC0;
				second = 0x03;
			} else {
				if (singlebyte) first = 0x81;
				else first  = 0x80;
				second = 0x01;
			}
			for (p=0; p < 4; ++p) {
				if (cursor_color & colorbits[p]) {
					cursor.plane[p].image[i][0] |= first;
					if (!singlebyte)
					cursor.plane[p].image[i][1] |= second;
				} else {
					cursor.plane[p].image[i][0] &= ~first;
					if (!singlebyte)
					cursor.plane[p].image[i][1] &= ~second;
				}
			}
		    }

		    for(i = ROWS_PER_CELL - 2; i < ROWS_PER_CELL; ++i) {
			if (i != (ROWS_PER_CELL-1)) {
				if (singlebyte) first = 0x81;
				else first  = 0x80;
				second = 0x01;
			} else {
				if (singlebyte) first = 0xC3;
				else first  = 0xC0;
				second = 0x03;
			}
			for (p=0; p < SCREENPLANES; ++p) {
				if (cursor_color & colorbits[p]) {
					cursor.plane[p].image[i][0] |= first;
					if (!singlebyte)
					cursor.plane[p].image[i][1] |= second;
				} else {
					cursor.plane[p].image[i][0] &= ~first;
					if (!singlebyte)
					cursor.plane[p].image[i][1] &= ~second;
				}
			}
		    }
		    break;

		case CURSOR_UNDERLINE:

		    i = ROWS_PER_CELL - 1;
		    first  = 0xFF;
		    second = 0xFF;
		    for (p=0; p < SCREENPLANES; ++p) {
			if (cursor_color & colorbits[p]) {
				cursor.plane[p].image[i][0] |= first;
				if (!singlebyte)
				cursor.plane[p].image[i][1] |= second;
			} else {
				cursor.plane[p].image[i][0] &= ~first;
				if (!singlebyte)
				cursor.plane[p].image[i][1] &= ~second;
			}
		    }
		    break;

		case CURSOR_FRAME:

		    /* fall through */

		default:			
		    for(i = 0; i < ROWS_PER_CELL; ++i) {

			if ((i == 0) || (i == (ROWS_PER_CELL-1))) {
				first  = 0xFF;
				second = 0xFF;
			} else {
				if (singlebyte) first = 0x81;
				else first  = 0x80;
				second = 0x01;
			}
			for (p=0; p < SCREENPLANES; ++p) {
				if (cursor_color & colorbits[p]) {
					cursor.plane[p].image[i][0] |= first;
					if (!singlebyte)
					cursor.plane[p].image[i][1] |= second;
				} else {
					cursor.plane[p].image[i][0] &= ~first;
					if (!singlebyte)
					cursor.plane[p].image[i][1] &= ~second;
				}
			}
		    }
		    break;
	    }

	   /*
            * Place the new cell onto the display.
            *
            */
	    
	    for(i=0;i < ROWS_PER_CELL; ++i) {
 		tmp1 = screentable[i+pixy];
 		tmp1 += pixx;
 		tmp2 = tmp1 + 1;
		egawriteplane(8);
		/* memcpy(tmp1,cursor.plane[3].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,cursor.plane[3].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,cursor.plane[3].image[i][1]);

		egawriteplane(4);
		/* memcpy(tmp1,cursor.plane[2].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,cursor.plane[2].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,cursor.plane[2].image[i][1]);

		egawriteplane(2);
		/* memcpy(tmp1,cursor.plane[1].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,cursor.plane[1].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,cursor.plane[1].image[i][1]);

		egawriteplane(1);
		/* memcpy(tmp1,cursor.plane[0].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,cursor.plane[0].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,cursor.plane[0].image[i][1]);
	    }
	    egawriteplane(15);
#ifdef POSITIONBAR
	    if (inmap) positionbar();
#endif
}

void
vga_HideCursor()
{

	int i,pixx,pixy,x,y;
	char __far *tmp1;
	char __far *tmp2;
#ifdef REINCARNATION
	bool isrogue = Is_rogue_level(&u.uz);
	bool singlebyte = (isrogue || iflags.over_view
			      || iflags.traditional_view || !inmap);
#else
	bool singlebyte = (iflags.over_view
			      || iflags.traditional_view || !inmap);
#endif
	int curtyp;
	
	if (inmap && !cursor_type) return;	/* CURSOR_INVIS - nothing to do */
	/* protection from callers */
	x = min(curcol,(CO - 1)); 
	y = min(currow,(LI-1));
	if (!singlebyte && ((x < clipx) || (x > clipxmax))) return;

	    pixy = row2y(y);		/* convert to pixels */
	    if (singlebyte)
		    pixx = x;
	    else
		    pixx = col2x((x-clipx));

	    if (inmap) curtyp = cursor_type;
	    else curtyp = CURSOR_UNDERLINE;

	    if (curtyp == CURSOR_UNDERLINE)  /* optimization for uline */
		i = ROWS_PER_CELL - 1;
	    else
		i = 0;

	    for(;i < ROWS_PER_CELL; ++i) {
		tmp1 = screentable[i+pixy];
 		tmp1 += pixx;
 		tmp2 = tmp1 + 1;
		egawriteplane(8);
		/* memcpy(tmp,undercursor.plane[3].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,undercursor.plane[3].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,undercursor.plane[3].image[i][1]);

		egawriteplane(4);
		/* memcpy(tmp,undercursor.plane[2].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,undercursor.plane[2].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,undercursor.plane[2].image[i][1]);

		egawriteplane(2);
		/* memcpy(tmp,undercursor.plane[1].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,undercursor.plane[1].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,undercursor.plane[1].image[i][1]);

		egawriteplane(1);
		/* memcpy(tmp,undercursor.plane[0].image[i],BYTES_PER_CELL); */
		WRITE_ABSOLUTE(tmp1,undercursor.plane[0].image[i][0]);
		if (!singlebyte)
		WRITE_ABSOLUTE(tmp2,undercursor.plane[0].image[i][1]);
	    }
	    egawriteplane(15);
}
#  endif /* SIMULATE_CURSOR */
# endif /* OVLB */
#endif /* SCREEN_VGA  */

/* vidvga.c */
