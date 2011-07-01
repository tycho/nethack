/*	SCCS Id: @(#)getline.c	3.0	89/06/16
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "func_tab.h"

/*
 * Some systems may have getchar() return EOF for various reasons, and
 * we should not quit before seeing at least NR_OF_EOFS consecutive EOFs.
 */
#if defined(SYSV) || defined(DGUX) || defined(HPUX)
#define	NR_OF_EOFS	20
#endif
#ifdef MACOS
extern short macflags;
#endif
#ifdef OVL1

char morc = 0;	/* tell the outside world what char he used */

#endif /* OVL1 */

extern char erase_char, kill_char;	/* from appropriate tty.c file */

#ifdef OVL1

/*
 * Read a line closed with '\n' into the array char bufp[BUFSZ].
 * (The '\n' is not stored. The string is closed with a '\0'.)
 * Reading can be interrupted by an escape ('\033') - now the
 * resulting string is "\033".
 */
void
getlin(bufp)
register char *bufp;
{
	register char *obufp = bufp;
	register int c;
#ifdef MACOS
	short	tmpflags;
	
	tmpflags = macflags;
	macflags &= ~fDoNonKeyEvt;
#endif
	flags.toplin = 2;		/* nonempty, no --More-- required */
	for(;;) {
		(void) fflush(stdout);
		if((c = Getchar()) == EOF) {
			*bufp = 0;
#ifdef MACOS
	macflags = tmpflags;
#endif
			return;
		}
		if(c == '\033') {
			*obufp = c;
			obufp[1] = 0;
#ifdef MACOS
	macflags = tmpflags;
#endif
			return;
		}
		if(c == erase_char || c == '\b') {
			if(bufp != obufp) {
				bufp--;
				putstr("\b \b");/* putsym converts \b */
			} else	bell();
		} else if(c == '\n') {
			*bufp = 0;
#ifdef MACOS
	macflags = tmpflags;
#endif
			return;
		} else if(' ' <= c && c < '\177' && 
			    (bufp-obufp < BUFSZ-1 || bufp-obufp < COLNO)) {
				/* avoid isprint() - some people don't have it
				   ' ' is not always a printing char */
			*bufp = c;
			bufp[1] = 0;
			putstr(bufp);
			bufp++;
		} else if(c == kill_char || c == '\177') { /* Robert Viduya */
				/* this test last - @ might be the kill_char */
			while(bufp != obufp) {
				bufp--;
				if(curx == 1 && cury > 1) {
					putstr("\b \b\b");
					curx = CO;
				} else putstr("\b \b");
			}
		} else
			bell();
	}
#ifdef MACOS
	macflags = tmpflags;
#endif
}

#endif /* OVL1 */
#ifdef OVLB

void
getret() {
	cgetret("");
}

void
cgetret(s)
register const char *s;
{
	putsym('\n');
	if(flags.standout)
		standoutbeg();
	putstr("Hit ");
	putstr(flags.cbreak ? "space" : "return");
	putstr(" to continue: ");
	if(flags.standout)
		standoutend();
	xwaitforspace(s);
}

#endif /* OVLB */
#ifdef OVL1

void
xwaitforspace(s)
register const char *s;	/* chars allowed besides space or return */
{
	register int c;
#ifdef MACOS
	short	tmpflags;
#endif

	morc = 0;
#ifdef MACOS
	flags.wantspace = TRUE;
	tmpflags = macflags;
	macflags &= ~fDoNonKeyEvt;
	HideCursor();
#endif

	while((c = readchar()) != '\n') {
#ifdef MACOS
		if(c == '\r' || c == 0x3 || c == 'p') break;
#endif
	    if(flags.cbreak) {
			if(c == ' ') break;
			if(s && index(s,c)) {
				morc = c;
				break;
			}
			bell();
	    }
	}

#ifdef MACOS
	ShowCursor();
	flags.wantspace = FALSE;
	macflags = tmpflags;
#endif
}

#endif /* OVL1 */
#ifdef OVL0

static int NEARDATA last_multi;

char *
parse()
{
#ifdef LINT	/* static char in_line[COLNO]; */
	char in_line[COLNO];
#else
	static char in_line[COLNO];
#endif
	register int foo, cnt = 0;
	boolean prezero = FALSE;

	multi = 0;
	flags.move = 1;
	curs_on_u();

	if (!flags.num_pad || (foo = readchar()) == 'n')
	    do {
	    	foo = readchar();
	    	if(foo >= '0' && foo <= '9') {
				multi = 10*multi+foo-'0';
				if (multi < 0 || multi > LARGEST_INT)
					multi = LARGEST_INT;
				if (multi > 9) {
					remember_topl();
					home();
					cl_end();
					Printf("Count: %d", multi);
				}
				last_multi = multi;
				if(!cnt && foo == '0') prezero = TRUE;
				cnt++;
		    }
		    if (foo == '\033') {   /* esc cancels count (TH) */
				remember_topl();
				home();
				cl_end();
				multi = last_multi = 0;
		    }
		} while(foo >= '0' && foo <= '9');
# ifdef REDO
	if (foo == DOAGAIN || in_doagain)
		multi = last_multi;
	else {
		savech(0);	/* reset input queue */
		savech(foo);
	}
# endif
	if(multi) {
		multi--;
		save_cm = in_line;
	}
	in_line[0] = foo;
	in_line[1] = 0;
	if(foo == 'g' || foo == 'G' || (flags.num_pad && foo == '5')){
		in_line[1] = Getchar();
#ifdef REDO
		savech(in_line[1]);
#endif
		in_line[2] = 0;
	}
	if(foo == 'm' || foo == 'M'){
		in_line[1] = Getchar();
#ifdef REDO
		savech(in_line[1]);
#endif
		in_line[2] = 0;
	}
	clrlin();
	if(prezero) in_line[0] = '\033';
	return(in_line);
}

#endif /* OVL0 */
#ifdef OVLB

#ifdef UNIX
static void
end_of_input()
{
	settty("End of input?\n");
	clearlocks();
	exit(0);
}
#endif

#endif /* OVLB */
#ifdef OVL0

char
readchar() {
	register int sym;
#ifdef UNIX
	/* kludge alert: Some Unix variants return funny values if readchar
	 * is called, interrupted, and then called again from done2().  There
	 * is non-reentrant code in the internal _filbuf() routine, called by
	 * getc().
	 */
	static int nesting = 0;
	char nestbuf;
#endif

	(void) fflush(stdout);
#ifdef UNIX
	if((sym = ((++nesting == 1) ? Getchar() :
	    (read(fileno(stdin),&nestbuf,1) == 1 ? (int)nestbuf : EOF))) == EOF)
# ifdef NR_OF_EOFS
	{ /*
	   * Some SYSV systems seem to return EOFs for various reasons
	   * (?like when one hits break or for interrupted systemcalls?),
	   * and we must see several before we quit.
	   */
		register int cnt = NR_OF_EOFS;
		while (cnt--) {
		    clearerr(stdin);	/* omit if clearerr is undefined */
		    if((sym = Getchar()) != EOF) goto noteof;
		}
		end_of_input();
	     noteof:	;
	}
# else
		end_of_input();
# endif /* NR_OF_EOFS /**/
#else
	sym = Getchar();
#endif /* UNIX */
#ifdef UNIX
	nesting--;
#endif
	if(flags.toplin == 1)
		flags.toplin = 2;
	return((char) sym);
}

#endif /* OVL0 */
#ifdef OVL2

#ifdef COM_COMPL
/* Read in an extended command - doing command line completion for
 * when enough characters have been entered to make a unique command.
 * This is just a modified getlin().   -jsb
 */
void
get_ext_cmd(bufp)
register char *bufp;
{
	register char *obufp = bufp;
	register int c;
	int com_index, oindex;
#ifdef MACOS
	short tmpflags;
	
	tmpflags = macflags & ~(fExtCmdSeq1 | fExtCmdSeq2 | fExtCmdSeq3);
	macflags &= ~fDoNonKeyEvt;
#endif

	flags.toplin = 2;		/* nonempty, no --More-- required */

	for(;;) {
		(void) fflush(stdout);
		if((c = readchar()) == EOF) {
			*bufp = 0;
#ifdef MACOS
			macflags = tmpflags;
#endif
			return;
		}
		if(c == '\033') {
			*obufp = c;
			obufp[1] = 0;
#ifdef MACOS
			macflags = tmpflags;
#endif
			return;
		}
		if(c == erase_char || c == '\b') {
			if(bufp != obufp) {
				bufp--;
				putstr("\b \b"); /* putsym converts \b */
			} else	bell();
		} else if(c == '\n') {
			*bufp = 0;
#ifdef MACOS
			macflags = tmpflags;
#endif
			return;
		} else if(' ' <= c && c < '\177') {
				/* avoid isprint() - some people don't have it
				   ' ' is not always a printing char */
			*bufp = c;
			bufp[1] = 0;
			oindex = 0;
			com_index = -1;

			while(extcmdlist[oindex].ef_txt != NULL){
				if(!strncmp(obufp, extcmdlist[oindex].ef_txt,
				    strlen(obufp)))
					if(com_index == -1) /* No matches yet*/
					    com_index = oindex;
					else /* More than 1 match */
					    com_index = -2;
				oindex++;
			}
			if(com_index >= 0){
				Strcpy(obufp, extcmdlist[com_index].ef_txt);
				/* finish printing our string */
				putstr(bufp);
				bufp = obufp; /* reset it */
				if(strlen(obufp) < BUFSIZ-1 &&
				 strlen(obufp) < COLNO)
					/* set bufp at the end of our string */
					bufp += strlen(obufp);
			} else {
				putstr(bufp);
				if(bufp-obufp < BUFSZ-1 && bufp-obufp < COLNO)
					bufp++;
			}
		} else if(c == kill_char || c == '\177') { /* Robert Viduya */
				/* this test last - @ might be the kill_char */
			while(bufp != obufp) {
				bufp--;
				putstr("\b \b");
			}
		} else
			bell();
	}
#ifdef MACOS
	macflags = tmpflags;
#endif

}
#endif /* COM_COMPL */

#endif /* OVL2 */
