/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2013-11-16 */
/* Copyright (c) 2013 Alex Smith. */
/* The 'uncursed' rendering library may be distributed under either of the
 * following licenses:
 *  - the NetHack General Public License
 *  - the GNU General Public License v2 or later
 * If you obtained uncursed as part of NetHack 4, you can find these licenses in
 * the files libnethack/dat/license and libnethack/dat/gpl respectively.
 */
/*
 * This gives a terminal backend for the uncursed rendering library, designed to
 * work well on virtual terminals, but sacrificing support for physical
 * terminals (at least, those that require padding). The minimum feature set
 * required by the terminal is 8 colors + bold for another 8, basic cursor
 * movement commands, and understanding either code page 437 or Unicode
 * (preferably both). This file is tested against the following terminal
 * emulators, and intended to work correctly on all of them:
 *
 * Terminals built into operating systems:
 * Linux console (fbcon)
 * DOSBOx emulation of the DOS console
 *
 * Terminal emulation software:
 * xterm
 * gnome-terminal
 * konsole
 * PuTTY
 * urxvt
 * lxterminal
 *
 * Terminal multiplexers:
 * screen
 * tmux
 *
 * Terminal recording renderers:
 * termplay
 * jettyplay
 * (Note that some ttyrec players, such as ttyplay, use the terminal for
 * rendering rather than doing their own rendering; those aren't included on
 * this list. ipbt is not included because it does not support character sets
 * beyond ASCII; it also has a major issue where it understands the codes for
 * colors above 8, but is subsequently unable to render them.)
 *
 * Probably the most commonly used terminal emulator not on this list is
 * original rxvt, which has very poor support for all sorts of features we would
 * want to use (although the main reason it isn't supported is that it relies on
 * the font used for encoding support rather than trying to do any encoding
 * handling itself, meaning that text that's meant to be CP437 tends to get
 * printed as Latin-1 in practice). It's entirely possible that you could manage
 * to configure rxvt to work with uncursed, but save yourself some trouble and
 * use urxvt instead :)
 *
 * This file is written to be platform-agnostic, but contains platform-specific
 * code on occasion (e.g. signals, delays). Where there's a choice of platform-
 * specific function, the most portable is used (e.g. select() rather than
 * usleep() for delays, because it's in older versions of POSIX).
 */

/* Detect OS. */
#ifdef AIMAKE_BUILDOS_MSWin32
# undef WIN32
# define WIN32
#endif

#ifdef WIN32
# error !AIMAKE_FAIL_SILENTLY! tty.c does not work on Windows. \
    Use wincon.c instead.
#endif

/* UNIX-specific headers */
#define _POSIX_SOURCE 1
#define _DARWIN_C_SOURCE 1      /* needed for SIGWINCH on OS X; no effect on
                                   Linux */
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

/* Linux, and maybe other UNIX, -specific headers */
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

/* tty.c is always linked statically. */
#define UNCURSED_MAIN_PROGRAM

#include "uncursed_hooks.h"
#include "uncursed.h"
#include "uncursed_tty.h"

/* Note: ifile only uses platform-specific read functions like read(); ofile
   only uses stdio for output (e.g. fputs), but all output to ofile is
   abstracted via ofile_output anyway. Try not to muddle these! */
#define ofile stdout
#define ifile stdin

#define CSI "\x1b["
#define OSC "\x1b]"
#define ST  "\x1b\\"

#define OFILE_BUFFER_SIZE (1 << 18)

static int is_inited = 0;

/* Portable helper functions */
/* Returns the first codepoint in the given UTF-8 string, or -1 if it's
   incomplete, or -2 if it's invalid. */
static long
parse_utf8(char *s)
{
    /* This signedness trick is normally illegal, but fortunately, it's legal
       for chars in particular */
    unsigned char *t = (unsigned char *)s;

    int charcount;
    wchar_t r;

    if (*t < 0x80)
        return *s;
    if (*t < 0xc0)
        return -2;
    if (*t < 0xe0) {
        charcount = 2;
        r = *t & 0x1f;
    } else if (*t < 0xf0) {
        charcount = 3;
        r = *t & 0x0f;
    } else if (*t < 0xf8) {
        charcount = 4;
        r = *t & 0x07;
    } else
        return -2;

    while (--charcount) {
        t++;
        if (!*t)
            return -1;  /* incomplete */
        if (*t < 0x80 || *t >= 0xc0)
            return -2;
        r *= 0x40;
        r += *t & 0x3f;
    }

    if (r >= 0x110000)
        return -2;

    return r;
}

static int chars_since_flush = 0;
static void
ofile_output(char *format, ...)
{
    va_list v;

    va_start(v, format);
    chars_since_flush += vfprintf(ofile, format, v);
    va_end(v);

    if (chars_since_flush > OFILE_BUFFER_SIZE - 200) {
        /* Older versions of Konsole dislike it when stdio flushes in the
           middle of a UTF-8 character. We solve the issue by flushing ourself,
           before stdio gets the chance. We told it to flush once every
           OFILE_BUFFER_SIZE bytes; allow some headroom on that because we often
           send more than one byte at a time. */
        tty_hook_flush();
    }
}

static void
ofile_outputs(char *s)
{
    ofile_output("%s", s);
}

/* Linux-specific functions */
static void
measure_terminal_size(int *h, int *w)
{
    /* We need to determine the size of the terminal, which is nontrivial to do
       in a vaguely portable manner. On Linux, and maybe other UNIXes, we can do
       it like this: */
    struct winsize ws;

    ioctl(fileno(ofile), TIOCGWINSZ, &ws);
    *h = ws.ws_row;
    *w = ws.ws_col;
}

/* UNIX-specific functions */
static int selfpipe[2] = { -1, -1 };

static struct termios ti, ti_orig, to, to_orig;
static int sighup_mode = 0;     /* return KEY_HANGUP as every input if set */

static int raw_isig = 0;
static void
platform_specific_rawsignals(int raw)
{
    if (raw)
        ti.c_lflag &= ~ISIG;
    else
        ti.c_lflag |= ISIG;

    tcsetattr(fileno(ifile), TCSANOW, &ti);

    raw_isig = raw;
}

/* TODO: make selfpipe non-blocking for writes, so that being spammed signals
   doesn't lock up the program; this is particularly important with SIGPIPE */
static void
handle_sigwinch(int unused)
{
    (void)unused;
    write(selfpipe[1], "r", 1);
}

static void
handle_sighup(int unused)
{
    (void)unused;
    write(selfpipe[1], "h", 1);
}

static void
handle_sigtstp(int unused)
{
    (void)unused;
    write(selfpipe[1], "s", 1);
}

static void
handle_sigcont(int unused)
{
    (void)unused;
    write(selfpipe[1], "c", 1);
}

/* These standard hooks need to be implemented in a platform-specific manner */
void
tty_hook_signal_getch(void)
{
    write(selfpipe[1], "g", 1);
}

static fd_set watchfds;
static int watchfds_inited = 0;
static int watchfd_max = 0;

void
tty_hook_watch_fd(int fd, int watch)
{
    if (fd >= FD_SETSIZE)
        abort(); /* avoid UB using defined, noticeable behaviour */

    if (watch) {
        FD_SET(fd, &watchfds);
        if (fd >= watchfd_max) watchfd_max = fd+1;
    } else {
        FD_CLR(fd, &watchfds);
        while (watchfd_max && !FD_ISSET(watchfd_max - 1, &watchfds))
            watchfd_max--;
    }   
}

static void
platform_specific_init(void)
{
    /* Set up the self-pipe. (Do this first, so we can exit early if it fails. */
    if (selfpipe[0] == -1) {
        if (pipe(selfpipe) != 0) {
            perror("Could not initialise uncursed tty library");
            exit(1);
        }
    }

    /* We initially aren't watching any FDs. */
    if (!watchfds_inited)
        FD_ZERO(&watchfds);
    watchfds_inited = 1;

    /* Set up the terminal control flags. */
    tcgetattr(fileno(ifile), &ti_orig);
    tcgetattr(fileno(ofile), &to_orig);

    /*
     * Terminal control flags to turn off for the input file:
     * ISTRIP    bit 8 stripping (incompatible with Unicode)
     * INPCK     parity checking (ditto)
     * PARENB    more parity checking (ditto)
     * IGNCR     ignore CR (means there's a key we can't read)
     * INLCR     translate NL to CR (ditto)
     * ICRNL     translate CR to NL (ditto)
     * IXON      insert XON/XOFF into the stream (confuses the client)
     * IXOFF     respect XON/XOFF (causes the process to lock up on ^S)
     * ICANON    read a line at a time (means we can't read keys)
     * ECHO      echo typed characters (confuses our view of the screen)
     * We also set ISIG based on raw_isig, and VMIN to 1, VTIME to 0,
     * which gives select() the correct semantics; and we turn on CS8,
     * to be able to read Unicode (although on Unicode systems it seems
     * to be impossible to turn it off anyway).
     *
     * Terminal control flags to turn off for the output file:
     * OPOST     general output postprocessing (portability nightmare)
     * OCRNL     map CR to NL (we need to stay on the same line with CR)
     * ONLRET    delete CR (we want to be able to use CR)
     * OFILL     delay using padding (only works on physical terminals)
     * We modify the flags for to only after modifying the flags for ti,
     * so that one set of changes doesn't overwrite the other.
     */
    tcgetattr(fileno(ifile), &ti);
    ti.c_iflag &= ~(ISTRIP | INPCK | IGNCR | INLCR | ICRNL | IXON | IXOFF);
    ti.c_cflag &= ~PARENB;
    ti.c_cflag |= CS8;
    ti.c_lflag &= ~(ISIG | ICANON | ECHO);
    ti.c_cc[VMIN] = 1;
    ti.c_cc[VTIME] = 0;
    if (raw_isig)
        ti.c_lflag |= ISIG;
    tcsetattr(fileno(ifile), TCSANOW, &ti);

    tcgetattr(fileno(ofile), &to);
    to.c_oflag &= ~(OPOST | OCRNL | ONLRET | OFILL);
    tcsetattr(fileno(ofile), TCSADRAIN, &to);

    /* Set up signal handlers. (Luckily, sigaction is just as portable as
       tcsetattr, so we don't have to deal with the inconsistency of signal.) */
    struct sigaction sa;
    sa.sa_flags = 0;    /* SA_RESTART would be nice, but too recent */
    sigemptyset(&(sa.sa_mask));

    sa.sa_handler = handle_sigwinch;
    sigaction(SIGWINCH, &sa, 0);

    sa.sa_handler = handle_sighup;
    sigaction(SIGHUP, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGPIPE, &sa, 0);

    sa.sa_handler = handle_sigtstp;
    sigaction(SIGTSTP, &sa, 0);

    sa.sa_handler = handle_sigcont;
    sigaction(SIGCONT, &sa, 0);
}

static void
platform_specific_exit(void)
{
    /* Put the terminal back to the way the user had it. */
    tcsetattr(fileno(ifile), TCSANOW, &ti_orig);
    tcsetattr(fileno(ofile), TCSADRAIN, &to_orig);
}

static char keystrings[5];      /* used to create unique pointers as sentinels */
static char *KEYSTRING_HANGUP = keystrings + 0;
static char *KEYSTRING_RESIZE = keystrings + 1;
static char *KEYSTRING_INVALID = keystrings + 2;
static char *KEYSTRING_SIGNAL = keystrings + 3;
static char *KEYSTRING_OTHERFD = keystrings + 4;

static int inited_when_stopped = 0;

static char *
platform_specific_getkeystring(int timeout_ms)
{
    /* We want to wait for a character from the terminal (i.e. a keypress or
       mouse click), or for a signal (we handle SIGWINCH, SIGTSTP/SIGCONT, and
       SIGHUP/SIGTERM/SIGPIPE; SIGINT and friends are left unhandled because
       that's what rawsignals is for, SIGTTOU and friends are left unhandled
       because the default behaviour is what we want anyway, and SIGSEGV and
       friends can't meaningfully be handled). Note that we read bytes here;
       Unicode conversion is done later.

       There are two difficulties: reading the multiple input sources at once,
       and stopping at the right moment. We wait timeout_ms for the first byte
       of the key (which could be 0, or could be negative = infinite); then we
       wait 200ms for the subsequent characters /if/ we have reason to believe
       that there might be subsequent characters. (Exception: ESC ESC is parsed
       as ESC, so that the user can send ESC quickly.) */

    struct timeval t;
    t.tv_sec = timeout_ms / 1000;
    t.tv_usec = (timeout_ms % 1000) * 1000;

    static char keystring[80];
    char *r = keystring;

    while (1) {
        if (sighup_mode)
            return KEYSTRING_HANGUP;

        fd_set readfds;
        memcpy(&readfds, &watchfds, sizeof (fd_set));
        FD_SET(fileno(ifile), &readfds);

        int max = fileno(ifile);

        if (r == keystring) {
            /* Look for something on the selfpipe, as well as the "rest" of
               the key */
            FD_SET(selfpipe[0], &readfds);
            if (max < selfpipe[0])
                max = selfpipe[0];

        } else {
            /* Ignore the selfpipe; wait 200ms for the rest of the key */
            t.tv_sec = 0;
            t.tv_usec = 200000;
        }

        errno = 0;
        int s = select(max + 1, &readfds, 0, 0,
                       (timeout_ms >= 0 || r != keystring) ? &t : 0);
        if (!s)
            break;      /* timeout */

        if (s < 0 && errno == EINTR)
            continue;   /* EINTR means try again */
        if (s < 0)
            sighup_mode = 1;        /* something went wrong */

        /* If this is not the first character, we don't read from the signal
           pipe anyway, so no need to worry about handling a partial character
           read on a signal. If it is the first character, signals take
           precedence, and we just leave any character in the input queue. */
        if (FD_ISSET(selfpipe[0], &readfds)) {

            char signalcode[1];
            read(selfpipe[0], signalcode, 1);

            switch (*signalcode) {
            case 'g':  /* uncursed_signal_getch */
                return KEYSTRING_SIGNAL;

            case 'r':  /* SIGWINCH */
                return KEYSTRING_RESIZE;

            case 'h':  /* SIGHUP, SIGTERM, SIGPIPE */
                sighup_mode = 1;
                return KEYSTRING_HANGUP;

            case 's':  /* SIGTSTP */
                if (is_inited) {
                    tty_hook_exit();
                    inited_when_stopped = 1;
                }
                raise(SIGSTOP);
                break;

            case 'c':  /* SIGCONT */
                if (is_inited) {
                    /* We were stopped unexpectedly; completely reset the
                       terminal */
                    tty_hook_exit();
                    inited_when_stopped = 1;
                }
                if (inited_when_stopped) {
                    int h, w;

                    tty_hook_init(&h, &w, NULL);
                    inited_when_stopped = 0;
                    tty_hook_fullredraw();
                }
                break;
            }

        } else if (!FD_ISSET(fileno(ifile), &readfds)) {

            /* Activity on one of the watch fds. */
            return KEYSTRING_OTHERFD;

        } else {
            /*
             * The user pressed a key. We need to return immediately if:
             * - It wasn't ESC, and was the last character of a UTF-8
             *   encoding (including ASCII), or
             * - It's the second character, the first was ESC, and it
             *   isn't [ or O;
             * - It's the third or subsequent character, the first was
             *   ESC, and it isn't [, ;, or a digit;
             * - We're running out of room in the buffer (malicious
             *   input).
             */

            if (read(fileno(ifile), r++, 1) == 0) {
                /* EOF on the input is unrecoverable, so the best we can do is
                   to treat it as a hangup. */
                sighup_mode = 1;
                return KEYSTRING_HANGUP;
            }

            if (r - keystring > 75) {
                /* Someone's trying to overflow the buffer... */
                return KEYSTRING_INVALID;
            }

            if (*keystring == 27) {

                /* Escape */
                if (r - keystring == 2 && r[-1] != '[' && r[-1] != 'O')
                    break;
                if (r - keystring > 2 && r[-1] != '[' && r[-1] != ';' &&
                    (r[-1] < '0' || r[-1] > '9'))
                    break;

            } else {

                /* not Escape */
                *r = 0;
                long c = parse_utf8(keystring);

                if (c >= 0)
                    break;
                if (c == -2)
                    return KEYSTRING_INVALID;   /* invalid input */
            }
        }
    }

    if (r == keystring)
        return 0;

    *r = '\0';
    return keystring;
}

static void
platform_specific_delay(int ms)
{
    struct timeval t;
    t.tv_sec = ms / 1000;
    t.tv_usec = (ms % 1000) * 1000;

    select(0, 0, 0, 0, &t);
}

/* (mostly) portable functions */
static int terminal_contents_unknown = 1;
static int last_color = -1;
static int last_cursor = -1;
static int supports_utf8 = 0;

static int last_y = -1, last_x = -1;
static int last_h = -1, last_w = -1;

static void
record_and_measure_terminal_size(int *h, int *w)
{
    measure_terminal_size(h, w);
    if (*h != last_h || *w != last_w) {
        /* Send a terminal resize code, for watchers/recorders. */
        fprintf(ofile, CSI "8;%d;%dt", *h, *w);
    }
    last_h = *h;
    last_w = *w;
}

static void
setcursorsize(int size)
{
    /* Change whether the cursor is drawn at all. TODO: Shield fom DOS. */
    if (size == 0) {
        if (last_cursor != 0)
            ofile_outputs(CSI "?25l");                   /* hide cursor */
        last_cursor = 0;
    } else {
        if (last_cursor != 1)
            ofile_outputs(CSI "?25h");                   /* show cursor */
        last_cursor = 1;
    }

    /* Don't change the cursor size; we'd have no way to undo the change, and
       it leads to garbage on many terminals. */
}

/* (y, x) are the coordinates of a character that can safely be clobbered,
   or (-1, -1) if it doesn't matter if we print garbage; the cursor is left
   in an unknown location */
static void
set_charset(int y, int x)
{
    if (x > -1)
        fprintf(ofile, CSI "%d;%dH", y + 1, x + 1);     /* move cursor */

    /* Tell the terminal the cursor status we remembered, if there is one;
       this means that anyone watching will have their cursor sync up with the
       actual game. */
    if (last_cursor != -1) {
        int lc = last_cursor;
        last_cursor = -1;
        setcursorsize(lc);
    }

    if (x > -1)
        fprintf(ofile, CSI "%d;%dH", y + 1, x + 1);     /* move cursor */

    ofile_outputs("\x0f");            /* select character set G0 */

    if (x > -1)
        fprintf(ofile, CSI "%d;%dH", y + 1, x + 1);

    if (supports_utf8) {

        ofile_outputs("\x1b(B");      /* select default character set for G0 */

        if (x > -1)
            fprintf(ofile, CSI "%d;%dH", y + 1, x + 1);

        ofile_outputs("\x1b%G");      /* set character set as UTF-8 */

    } else {

        ofile_outputs("\x1b%@");      /* disable Unicode, set default
                                           character set */
        if (x > -1)
            fprintf(ofile, CSI "%d;%dH", y + 1, x + 1);

        ofile_outputs("\x1b(U");      /* select null mapping, = cp437 on a PC */
    }

    last_y = last_x = -1;
}

static int palette[][3] = {
    {0x00, 0x00, 0x00}, {0xaf, 0x00, 0x00}, {0x00, 0x87, 0x00},
    {0xaf, 0x5f, 0x00}, {0x00, 0x00, 0xaf}, {0x87, 0x00, 0x87},
    {0x00, 0xaf, 0x87}, {0xaf, 0xaf, 0xaf}, {0x5f, 0x5f, 0x5f},
    {0xff, 0x5f, 0x00}, {0x00, 0xff, 0x00}, {0xff, 0xff, 0x00},
    {0x87, 0x5f, 0xff}, {0xff, 0x5f, 0xaf}, {0x00, 0xd7, 0xff},
    {0xff, 0xff, 0xff}
};

/* Spouts a bunch of garbage to the screen. Use this only immediately before
   clearing the screen. */
static void
setup_palette(void)
{
    int i;

    for (i = 0; i < 16; i++) {

        /* Setup for xterm, gnome-terminal */
        fprintf(ofile, "\r" OSC "4;%d;rgb:%02x/%02x/%02x" ST, i, palette[i][0],
                palette[i][1], palette[i][2]);

        /* Setup for Linux console; I think PuTTY parses this too. The Linux
           console doesn't need the ST on the end, but without it, other
           terminals may end up waiting forever for the ST. */
        fprintf(ofile, "\r" OSC "P%01x%02x%02x%02x" ST, i, palette[i][0],
                palette[i][1], palette[i][2]);

    }
}

static void
reset_palette(void)
{
    fprintf(ofile, "\r" OSC "104" ST "\r" OSC "R" ST);
}

void
tty_hook_beep(void)
{
    ofile_outputs("\x07");
    tty_hook_flush();
}

void
tty_hook_setcursorsize(int size)
{
    setcursorsize(size);
    tty_hook_flush();
}

void
tty_hook_positioncursor(int y, int x)
{
    if (last_y == y && last_x == x)
        return;

    fprintf(ofile, CSI "%d;%dH", y + 1, x + 1);          /* move cursor */
    tty_hook_flush();

    last_y = y;
    last_x = x;
}

void
tty_hook_init(int *h, int *w, char *title)
{
    (void)title;

    /* Switch to block buffering, to avoid an early flush that confuses some
       broken terminals (e.g. old versions of Konsole), and to try to cut down
       on frames in a ttyrec.

       C11 requires setvbuf to be the first operation performed on a stream. I
       don't think (although am not 100% sure) that that applies to POSIX,
       though. We flush before the setvbuf in order to reduce issues that might
       be caused by that restriction on unusual platforms. */
    tty_hook_flush();
    setvbuf(ofile, NULL, _IOFBF, OFILE_BUFFER_SIZE);

    platform_specific_init();

    /* Save the character sets. */
    ofile_outputs("\x1b""7");                            /* save state */

    /* Switch to the alternate screen, if possible, so that we don't overwrite
       the user's scrollback. */
    ofile_outputs(CSI "?1049h");                         /* alternate screen */

    /* Hide any scrollbar, because it's useless. */
    ofile_outputs(CSI "?30l");                           /* hide scrollbar */

    /* Work out the terminal size; also set it to itself (so that people
       watching, and recordings, have the terminal size known). */
    record_and_measure_terminal_size(h, w);

    /* Take control over the numeric keypad, if possible. Even when it isn't
       (e.g. xterm), this lets us distinguish Enter from Return. */
    ofile_outputs("\x1b=");                              /* app keypad mode */

    /* TODO: turn on mouse tracking? */

    /*
     * Work out the Unicodiness of the terminal. We do this by sending 4 bytes
     * = 2 Unicode codepoints, and seeing how far the cursor moves.
     *
     * \xc2\xa0 = non-breaking space in UTF-8
     * \x1b[6n  = report cursor position
     */
    ofile_outputs("\r\nConfiguring terminal, please wait.\r\n"
                  "\xc2\xa0\xc2\xa0\x1b[6n");
    tty_hook_flush();

    /* Very primitive terminals might not send any reply. So we wait 2 seconds
       for a reply before moving on. The reply is written as a PF3 with
       modifiers; the row is eaten, and the column is returned in the modifiers
       (column 3 = unicode = KEY_PF3 | (KEY_SHIFT*2), column 5 = not unicode =
       KEY_PF3 | (KEY_SHIFT*4)). VT100 is weird!

       If we get a hangup at this prompt, we continue anyway to avoid an
       infinite loop; we don't exit so that the process can do a safe shutdown. */

    supports_utf8 = 0;
    int kp = 0;

    while (kp != (KEY_PF3 | (KEY_SHIFT * 2)) && kp != KEY_HANGUP &&
           kp != (KEY_PF3 | (KEY_SHIFT * 4)) && kp != KEY_SILENCE) {

        kp = tty_hook_getkeyorcodepoint(2000);

        if (kp < 0x110000)
            kp = 0;
        else
            kp -= KEY_BIAS;
    }

    if (kp == (KEY_PF3 | (KEY_SHIFT * 2)))
        supports_utf8 = 1;

    /* Tell the terminal what Unicodiness we detected from it. There are two
       reasons to do this: a) if we're somehow wrong, this might make our
       output work anyway; b) other people watching/replaying will have the
       same Unicode settings, if the terminal supports them. */
    set_charset(-1, -1);

    /* Note: we intentionally don't support DECgraphics, because there's no way
       to tell whether it's working or not and it has a rather limited
       character set. However, the portable way to do it is to switch to G1 for
       DECgraphics, defining both G0 and G1 as DECgraphics; then switch back to
       G0 for normal text, defining just G0 as normal text. */


    setup_palette();

    /* Clear the terminal. */
    ofile_outputs(CSI "2J");                             /* clear terminal */

    terminal_contents_unknown = 1;

    last_color = -1;
    last_cursor = -1;
    last_y = last_x = -1;
    is_inited = 1;
}

void
tty_hook_exit(void)
{
    /* It'd be great to reset the terminal right now, but that wipes out
       scrollback. We can try to undo as many of our settings as we can, but
       that's not very many, due to problems with the protocol. */
    ofile_outputs("\x1b>");                              /* no keypad mode */
    ofile_outputs(CSI "?25h");                           /* show cursor */
    reset_palette();
    ofile_outputs(CSI "2J");                             /* clear terminal */

    /* Switch back to the main screen. */
    ofile_outputs(CSI "?1049l");                         /* main screen */

    /* Restore G0/G1. */
    ofile_outputs("\x1b" "8");                           /* restore state */

    platform_specific_exit();

    /* We can't know what the old buffering discipline was, but given that
       uncursed is designed to be attached to a terminal, we can have a very
       good guess. Line-buffering also won't hurt if it does happen to be
       something unusual; flushing a FILE* too often causes no issues but a
       slight performanc degradation. */
    setvbuf(ofile, NULL, _IOLBF, BUFSIZ);

    is_inited = 0;
}

void
tty_hook_delay(int ms)
{
    last_color = -1;    /* send a new SGR on the next redraw */
    platform_specific_delay(ms);
}

void
tty_hook_rawsignals(int raw)
{
    platform_specific_rawsignals(raw);
}

int
tty_hook_getkeyorcodepoint(int timeout_ms)
{
    last_color = -1;    /* send a new SGR on the next redraw */

    char *ks = platform_specific_getkeystring(timeout_ms);

    if (ks == KEYSTRING_SIGNAL)
        return KEY_SIGNAL + KEY_BIAS;

    if (ks == KEYSTRING_OTHERFD)
        return KEY_OTHERFD + KEY_BIAS;

    if (ks == KEYSTRING_HANGUP)
        return KEY_HANGUP + KEY_BIAS;

    if (ks == KEYSTRING_RESIZE) {
        int h, w;

        record_and_measure_terminal_size(&h, &w);
        uncursed_rhook_setsize(h, w);
        return KEY_RESIZE + KEY_BIAS;
    }

    if (ks == KEYSTRING_INVALID)
        return KEY_INVALID + KEY_BIAS;

    if (!ks)
        return KEY_SILENCE + KEY_BIAS;
    if (!*ks)
        return KEY_SILENCE + KEY_BIAS;  /* should never happen */

    if (*ks != '\x1b') {
        if (*ks == 127)
            return KEY_BACKSPACE + KEY_BIAS;    /* a special case */

        /* Interpret the key as UTF-8, if we can. Otherwise guess Latin-1;
           nobody uses CP437 for /input/. */
        long c = parse_utf8(ks);

        if (c == -2 && ks[1] == 0)
            return *ks;

        if (c < 0)
            return KEY_INVALID + KEY_BIAS;
        return c;

    } else if (ks[1] == 0) {
        /* ESC, by itself. */
        return KEY_ESCAPE + KEY_BIAS;

    } else if ((ks[1] != '[' && ks[1] != 'O') || !ks[2]) {
        /* An Alt-modified key. The curses API doesn't understand alt plus
           arbitrary unicode, so for now we just send the key without alt if
           it's outside the ASCII range. */
        if (!ks[2] && ks[1] < 127)
            return (KEY_ALT | ks[1]) + KEY_BIAS;
        long c = parse_utf8(ks + 1);

        if (c < 0)
            return KEY_INVALID + KEY_BIAS;
        return c;

    } else {
        /* The string consists of ESC, [ or O, then 0 to 2 semicolon-separated
           decimal numbers, then one character that's neither a digit nor a
           semicolon. getkeystring doesn't guarantee it's not malformed, so
           care is needed. */
        int args[2] = { 0, 0 };
        int argp = 0;

        char *r = ks + 2;

        /* Linux console function keys are a special case. */
        if (*r == '[' && r[1] && !r[2]) {
            return KEY_BIAS + (KEY_NONDEC | r[1]);
        }

        /* Parse the semicolon-separated decimal numbers. */
        while (*r && ((*r >= '0' && *r <= '9') || *r == ';')) {
            if (*r == ';')
                argp++;
            else if (argp < 2) {
                args[argp] *= 10;
                args[argp] += *r - '0';
            }
            r++;
        }

        /* Numbers are out of range. (This can happen on malicious input, or if
           the key string isn't actually a key string, but something like a
           cursor position report.) */
        if (args[0] > 256 || args[1] > 8)
            return KEY_INVALID + KEY_BIAS;

        if (args[1] == 0)
            args[1] = 1;

        /* A special case where ESC O and ESC [ are different */
        if (*r == 'P' && ks[1] == '[')
            return KEY_BREAK + KEY_BIAS;

        if (*r == '~') {
            /* A function key. */
            return KEY_BIAS +
                (KEY_FUNCTION | args[0] | KEY_SHIFT * (args[1] - 1));
        } else {
            /* A keypad key. */
            return KEY_BIAS + (KEY_KEYPAD | *r | KEY_SHIFT * (args[1] - 1));
        }
    }

    /* Unreachable, but gcc seems not to realise that */
    return KEY_BIAS + KEY_INVALID;
}

void
tty_hook_update(int y, int x)
{
    int j, i;

    /* If we need to do a full redraw, do so. */
    if (terminal_contents_unknown) {
        terminal_contents_unknown = 0;
        last_color = -1;
        last_cursor = -1;
        last_x = -1;

        for (j = 0; j < last_h; j++)
            for (i = 0; i < last_w; i++)
                tty_hook_update(j, i);
        return;
    }

    if (last_color == -1) {
        /* In addition to resending color, resend the other font information
           too. */
        set_charset(y, x);
    } else if (!uncursed_rhook_needsupdate(y, x))
        return;

    if (last_y != y || last_x == -1) {
        ofile_output(CSI "%d;%dH", y + 1, x + 1);        /* move cursor */
    } else if (last_x > x) {
        if (last_x == x + 1)
            ofile_outputs(CSI "D");                      /* move left */
        else
            ofile_output(CSI "%dD", last_x - x);         /* move left */
    } else if (last_x < x) {
        if (last_x == x-1)
            ofile_outputs(CSI "C");                      /* move right */
        else
            ofile_output(CSI "%dC", x - last_x);         /* move right */
    }

    int color = uncursed_rhook_color_at(y, x);

    if (color != last_color) {
        last_color = color;

        /* The general idea here is to specify bold for bright foreground, but
           blink for bright background only on terminals without 256-color
           support (via exploiting the "5" in the code for setting 256-color
           background). We set the colors using the 8-color code first, then
           the 16-color code, to get support for 16 colors without losing
           support for 8 colors. */
        ofile_outputs(CSI "0;");                         /* CSI reset */
        if ((color & 31) == 16)         /* default fg */
            ofile_outputs("39;");                        /* CSI default fg */
        else if ((color & 31) >= 8)     /* bright fg */
            /* CSI bold; CSI 8 color (fg & 8); CSI 16 color (fg) */
            ofile_output("1;%d;%d;", (color & 31) + 22, (color & 31) + 82);
        else    /* dark fg */
            ofile_output("%d;", (color & 31) + 30);      /* CSI 8 color (fg) */
        color >>= 5;
        if (color & 32)
            ofile_outputs("4;");                         /* CSI underline */
        color &= 31;
        if (color == 16)        /* default bg */
            ofile_outputs("49m");                        /* CSI default bg */
        else if (color >= 8)    /* bright bg */
            /* CSI 256 color 5 /or/ CSI blink (depending on color depth);
               CSI 8 color (bg & 8); CSI 16 color (bg) */
            ofile_output("48;5;5;%d;%dm", color + 32, color + 92);
        else
            ofile_output("%dm", color + 40);             /* CSI 8 color (bg) */
    }

    if (supports_utf8)
        ofile_outputs(uncursed_rhook_utf8_at(y, x));
    else
        ofile_output("%c", uncursed_rhook_cp437_at(y, x));

    uncursed_rhook_updated(y, x);

    last_x = x + 1;
    last_y = y;

    /* To save on output, redraw up to three characters that don't need
       redrawing rather than skipping them. We first check to see if the second,
       third or fourth character needs an update; if none do, then we're
       skipping at least four characters (first character doesn't need an
       update) or zero characters (first character needs an update), and
       shouldn't do any redundant drawing in either case. If one of those does
       need an update, we draw up to the first character that needs a
       non-redundant update using redundant updates. */

    int any_nearby_updates = 0;
    for (i = last_x + 1; i < last_x + 4 && i < last_w; i++)
        any_nearby_updates |= uncursed_rhook_needsupdate(y, i);

    while (any_nearby_updates && last_x < last_w) {
        if (uncursed_rhook_needsupdate(y, last_x))
            break;
        if (uncursed_rhook_color_at(y, last_x) != last_color)
            break;
        if (supports_utf8)
            ofile_outputs(uncursed_rhook_utf8_at(y, last_x));
        else
            ofile_output("%c", uncursed_rhook_cp437_at(y, last_x));

        /* futureproofing; we checked that the update isn't needed, so this
           should technically always be a no-op */
        uncursed_rhook_updated(y, last_x);

        last_x++;
    }

    if (last_x == last_w)
        last_x = -1;
}

void
tty_hook_fullredraw(void)
{
    terminal_contents_unknown = 1;
    setup_palette();
    ofile_outputs(CSI "2J");
    tty_hook_update(0, 0);
}

void
tty_hook_flush(void)
{
    fflush(ofile);
    chars_since_flush = 0;
}