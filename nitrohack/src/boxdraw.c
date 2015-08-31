#include "nhcurses.h"

/* -------------------------------------------------------------------------- */

enum nh_box_char {
    ULCORNER,
    LLCORNER,
    URCORNER,
    LRCORNER,

    LTEE,
    RTEE,
    BTEE,
    TTEE,

    HLINE,
    VLINE,
    PLUS,

    NH_BOX_CHAR_MAX
};

static wchar_t box_chars[2][NH_BOX_CHAR_MAX] = {
    /* ASCII box drawing characters */
    {
	'+',	/* ULCORNER */
	'+',	/* LLCORNER */
	'+',	/* URCORNER */
	'+',	/* LRCORNER */

	'+',	/* LTEE */
	'+',	/* RTEE*/
	'+',	/* BTEE */
	'+',	/* TTEE */

	'-',	/* HLINE */
	'|',	/* VLINE */
	'+'	/* PLUS */
    },

    /* Unicode box drawing characters */
    {
	0x250C,	/* ┌ ULCORNER */
	0x2514,	/* └ LLCORNER */
	0x2510,	/* ┐ URCORNER */
	0x2518,	/* ┘ LRCORNER */

	0x251C,	/* ├ LTEE */
	0x2524,	/* ┤ RTEE*/
	0x2534,	/* ┴ BTEE */
	0x252C,	/* ┬ TTEE */

	0x2500,	/* ─ HLINE */
	0x2502,	/* │ VLINE */
	0x253C	/* ┼ PLUS */
    }
};

static int box_draw_mode;	/* indexes into  */

#define NH_BOX_CHAR(c) (box_chars[box_draw_mode][c])
#define NH_BOX_SETCCHAR(v, c, a) \
    do {						\
	wchar_t tmp[2];					\
	tmp[0] = NH_BOX_CHAR(c);			\
	tmp[1] = 0;					\
	setcchar((v), tmp, (a), PAIR_NUMBER(a), NULL);	\
    } while(0)

/* -------------------------------------------------------------------------- */

void nh_box_set_graphics(enum nh_text_mode mode)
{
    if (ui_flags.unicode && mode == UNICODE_GRAPHICS) {
	box_draw_mode = 1;
    } else {
	box_draw_mode = 0;
    }
}

/* -------------------------------------------------------------------------- */

static int nh_box_mvwaddch(WINDOW *win, int y, int x, enum nh_box_char c, attr_t attrs)
{
    if (box_draw_mode == 1) {
	cchar_t ch;
	NH_BOX_SETCCHAR(&ch, c, attrs);
	return mvwadd_wch(win, y, x, &ch);

    } else {
	int r;
	wattron(win, attrs);
	r = mvwaddch(win, y, x, NH_BOX_CHAR(c));
	wattroff(win, attrs);
	return r;
    }
}

#define NH_BOX_MVWADD_CHAR(lcase, ucase) \
    int nh_box_mvwadd_ ## lcase (WINDOW *win, int y, int x, attr_t attrs) \
    { return nh_box_mvwaddch(win, y, x, (ucase), attrs); }

NH_BOX_MVWADD_CHAR(ulcorner, ULCORNER)
NH_BOX_MVWADD_CHAR(llcorner, LLCORNER)
NH_BOX_MVWADD_CHAR(urcorner, URCORNER)
NH_BOX_MVWADD_CHAR(lrcorner, LRCORNER)

NH_BOX_MVWADD_CHAR(ltee, LTEE)
NH_BOX_MVWADD_CHAR(rtee, RTEE)
NH_BOX_MVWADD_CHAR(btee, BTEE)
NH_BOX_MVWADD_CHAR(ttee, TTEE)

NH_BOX_MVWADD_CHAR(hline, HLINE)
NH_BOX_MVWADD_CHAR(vline, VLINE)
NH_BOX_MVWADD_CHAR(plus, PLUS)

/* -------------------------------------------------------------------------- */

int nh_box_mvwhline(WINDOW *win, int y, int x, int n, attr_t attrs)
{
    if (box_draw_mode == 1) {
	cchar_t ch;
	NH_BOX_SETCCHAR(&ch, HLINE, attrs);
	return mvwhline_set(win, y, x, &ch, n);

    } else {
	int r;
	wattron(win, attrs);
	r = mvwhline(win, y, x, NH_BOX_CHAR(HLINE), n);
	wattroff(win, attrs);
	return r;
    }
}

int nh_box_mvwvline(WINDOW *win, int y, int x, int n, attr_t attrs)
{
    if (box_draw_mode == 1) {
	cchar_t ch;
	NH_BOX_SETCCHAR(&ch, VLINE, attrs);
	return mvwvline_set(win, y, x, &ch, n);

    } else {
	int r;
	wattron(win, attrs);
	r = mvwvline(win, y, x, NH_BOX_CHAR(VLINE), n);
	wattroff(win, attrs);
	return r;
    }
}

int nh_box_wborder(WINDOW *win, attr_t attrs)
{
    if (box_draw_mode == 1) {
	cchar_t ls, rs, ts, bs, tl, tr, bl, br;
	NH_BOX_SETCCHAR(&ls, VLINE, attrs);
	NH_BOX_SETCCHAR(&rs, VLINE, attrs);
	NH_BOX_SETCCHAR(&ts, HLINE, attrs);
	NH_BOX_SETCCHAR(&bs, HLINE, attrs);
	NH_BOX_SETCCHAR(&tl, ULCORNER, attrs);
	NH_BOX_SETCCHAR(&tr, URCORNER, attrs);
	NH_BOX_SETCCHAR(&bl, LLCORNER, attrs);
	NH_BOX_SETCCHAR(&br, LRCORNER, attrs);
	return wborder_set(win, &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br);

    } else {
	int r;
	wattron(win, attrs);
	r = wborder(win,
		      NH_BOX_CHAR(VLINE), NH_BOX_CHAR(VLINE),
		      NH_BOX_CHAR(HLINE), NH_BOX_CHAR(HLINE),
		      NH_BOX_CHAR(ULCORNER), NH_BOX_CHAR(URCORNER),
		      NH_BOX_CHAR(LLCORNER), NH_BOX_CHAR(LRCORNER));
	wattroff(win, attrs);
	return r;
    }
}

int nh_box_whline(WINDOW *win, int n, attr_t attrs)
{
    if (box_draw_mode == 1) {
	cchar_t ch;
	NH_BOX_SETCCHAR(&ch, HLINE, attrs);
	return whline_set(win, &ch, n);

    } else {
	int r;
	wattron(win, attrs);
	r = whline(win, NH_BOX_CHAR(HLINE), n);
	wattroff(win, attrs);
	return r;
    }
}
