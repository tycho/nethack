/*	SCCS Id: @(#)macinit.c	3.0	88/08/05
/* Copyright (c) Johnny Lee, 1989.		 */
/* NetHack may be freely redistributed.  See license for details. */

/*	Initialization routine for the Macintosh */

#include	"hack.h"

#ifdef MACOS
# ifdef THINK_C
#include	<MemoryMgr.h>
# else
#include	<Memory.h>
#define ApplLimit 0x130         /* application limit [pointer]*/
# endif

/* Global variables */
extern WindowPtr	HackWindow;	/* points to NetHack's window */
char	*keys[8];
short macflags;
Boolean	lowMem;
long	lowMemLimit;
typedef struct defaultData {
	long	defaultFlags;
	long	lowMemLimit;
	Str255	fontName;
} defaultData;
#define	fDFZoomWindow	0x02L
#define	fDFUseDefaultFont	0x01L
short altCurs;


int
initterm(row, col)
short	row, col;
{
	register short	i, j;
	short		tempFont, tempSize, fontNum, size;
	char	*l, *m;
	EventRecord	theEvent;
	FontInfo	fInfo;
	Handle	temp;
	MenuHandle	theMenu;
	OSErr	error;
	Rect		boundsRect;
	Str255	appName, font;
	defaultData	*dD;
	term_info	*t;
	
	/* standard Mac initialization */
#ifdef THINK_C
	SetApplLimit((Ptr)ApplLimit - 8192);	/* an extra 8K for stack */
#else
	SetApplLimit(*(long *)ApplLimit - 8192);
#endif	
	MaxApplZone();
	UnloadSeg(mprintf);
	for (i = 2; i<9; i++) {
		temp = GetResource('CODE', i);
		HUnlock(temp);
		MoveHHi(temp);
		HLock(temp);
	}
	
	MoreMasters();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	lowMem = (FreeMem() < 700 *1024) ? TRUE : FALSE;
	InitGraf(&MAINGRAFPORT);
	
	InitFonts();
	InitWindows();
	InitMenus();
	InitCursor();
	FlushEvents(everyEvent, 0);
	if (error = GetVol((StringPtr)&appName, &tempSize))
		SysBeep(1);
	
	/* Application-specific startup code */
	theMenu = NewMenu(appleMenu, "\001\024");	/*  apple menu  */
	{
		char	tmp[256];
		Sprintf(&tmp[1],"About NetHack %s\311;(-", VERSION);
		tmp[0] = (char)strlen(&tmp[1]);
		AppendMenu(theMenu,tmp);
	}
	AddResMenu(theMenu, 'DRVR');
	InsertMenu(theMenu, 0);
	DisableItem(theMenu,0);

	t = (term_info *)malloc(sizeof(term_info));
	t->recordVRefNum = tempSize;
	
	for (i = fileMenu; i <= extendMenu; i++) {
		theMenu = GetMenu(i);
		if (theMenu) {
			InsertMenu(theMenu, 0);
			DisableItem(theMenu, 0);
		}
		if (i == editMenu) {
			t->shortMBarHandle = GetMenuBar();
		}
	}
	t->fullMBarHandle = GetMenuBar();
	
	DrawMenuBar();
	HiliteMenu(0);
	for (i = 0;i <= 7;i++) {
		temp = GetResource(HACK_DATA,(i + 100 + appleMenu));
		if (!temp) {
			SysBeep(1);
			panic("Can't get MENU_DATA resource");
		}
		MoveHHi(temp);
		HLock(temp);
		DetachResource(temp);
		keys[i] = *temp;
	}

	macflags = (fToggleNumPad | fDoNonKeyEvt);
	
	/* Set font to monaco, user-defined font or to Hackfont if available */
	if ((SCREEN_BITS.bounds.bottom - SCREEN_BITS.bounds.top) >400
		&& (SCREEN_BITS.bounds.right - SCREEN_BITS.bounds.left) > 580)
		size = 12;
	else
		size = 9;
	Strcpy((char *)&font[0], "\006Monaco");
	
	temp = GetResource(HACK_DATA, DEFAULT_DATA);
	if (temp) {
		HLock(temp);
		dD = (defaultData *)(*temp);
		lowMemLimit = dD->lowMemLimit;
		strncpy((char *)&font[0], (char *)&dD->fontName[0],
					(short)dD->fontName[0] + 1);
		if (dD->defaultFlags & fDFZoomWindow)
			macflags |= fZoomOnContextSwitch;
		HUnlock(temp);
		ReleaseResource(temp);
	}
			
	tempFont = MAINGRAFPORT->txFont;
	tempSize = MAINGRAFPORT->txSize;
	GetFNum(font, &fontNum);
	TextFont(fontNum);
	TextSize(size);
	GetFontInfo(&fInfo);
	TextFont(tempFont);
	TextSize(tempSize);
	
	if (!(dD->defaultFlags & fDFUseDefaultFont)) {
		Strcpy((char *)&appName[0], "\010HackFont");
		GetFNum(appName,&tempFont);
		if (tempFont) {
			fontNum = tempFont;
			tempFont = MAINGRAFPORT->txFont;
			TextFont(fontNum);
			TextSize(size);
			GetFontInfo(&fInfo);
			TextFont(tempFont);
			TextSize(tempSize);
			macflags |= fUseCustomFont;
		}
	}
	
	i = fInfo.ascent + fInfo.descent + fInfo.leading;
	j = fInfo.widMax;
	if ((row * i + 2 * Screen_Border) >
		 (SCREEN_BITS.bounds.bottom - SCREEN_BITS.bounds.top)
		 ||
		 (col * j + 2 * Screen_Border) >
		 	(SCREEN_BITS.bounds.right - SCREEN_BITS.bounds.left)) {
		size = 9;
		Strcpy((char *)&font[0], "\006Monaco");
		tempFont = MAINGRAFPORT->txFont;
		tempSize = MAINGRAFPORT->txSize;
		GetFNum(font, &fontNum);
		TextFont(fontNum);
		TextSize(size);
		GetFontInfo(&fInfo);
		TextFont(tempFont);
		TextSize(tempSize);
		i = fInfo.ascent + fInfo.descent + fInfo.leading;
		j = fInfo.widMax;
		macflags &= ~fUseCustomFont;
	}		
		 
	t->ascent = fInfo.ascent;
	t->descent = fInfo.descent;
	t->height = i;
	t->charWidth = j;
	
	t->fontNum = fontNum;
	t->fontSize = size;
	t->maxRow = row;
	t->maxCol = col;
	t->tcur_x = 0;
	t->tcur_y = 0;
	t->auxFileVRefNum = 0;
	if (error = SysEnvirons(1, &(t->system))) {
		SysBeep(1);
	}

	/* Some tweaking to allow for intl. ADB keyboard (unknown) */
	if (t->system.machineType > envMacPlus && !t->system.keyBoardType) {
		t->system.keyBoardType = envStandADBKbd;
	}

#define	KEY_MAP	103
	temp = GetResource(HACK_DATA, KEY_MAP);
	if (temp) {
		MoveHHi(temp);
		HLock(temp);
		DetachResource(temp);
		t->keyMap = (char *)(*temp);
	} else
		panic("Can't get keymap resource");

	SetRect(&boundsRect, LEFT_OFFSET, TOP_OFFSET + 10,
		(col * fInfo.widMax) + LEFT_OFFSET + 2 * Screen_Border,
		TOP_OFFSET + (row * t->height) + 2 * Screen_Border + 10);
	
	t->screen = (char **)malloc(row * sizeof(char *));
	t->scrAttr = (char **)malloc(row * sizeof(char *));
	l = malloc(row * col * sizeof(char));
	m = malloc(row * col * sizeof(char));
	for (i = 0;i < row;i++) {
		t->screen[i] = (char *)(l + (i * col * sizeof(char)));
		t->scrAttr[i] = (char *)(m + (i * col * sizeof(char)));
	}
	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j++) {
			t->screen[i][j] = ' ';
			t->scrAttr[i][j] = '\0';
		}
	}
	t->curHilite = 0;
	t->curAttr = 0;

#define all4Mods	(cmdKey | shiftKey | optionKey | controlKey)
#define nKeyCmd		(all4Mods - cmdKey)
#define nKeyShf		(all4Mods - shiftKey)
#define nKeyOpt		(all4Mods - optionKey)
#define nKeyCtl		(all4Mods - controlKey)
	/* give time for Multifinder to bring NetHack window to front */
	/* check to see if this is a "wiz bang" start */
	for(tempFont = 0; tempFont<10; tempFont++) {
		int	theMod;

		SystemTask();
		(void)GetNextEvent(nullEvent,&theEvent);

		theMod = theEvent.modifiers & all4Mods;
		if ((theMod == nKeyCmd) || (theMod == nKeyShf) ||
			(theMod == nKeyOpt) || (theMod == nKeyCtl)) {
			Strcpy(plname, "wizard");
/*			SysBeep(1);	/* useful only for debugging */
		}
	}

	HackWindow = NewWindow(0L, &boundsRect, "\016NetHack [MOVE]",
			TRUE, noGrowDocProc, (WindowPtr)-1, FALSE, (long)t);

	t->inColor = 0;
#ifdef TEXTCOLOR
	t->color[0] = blackColor;
	t->color[1] = redColor;
	t->color[2] = greenColor;
	t->color[3] = yellowColor;
	t->color[4] = blueColor;
	t->color[5] = magentaColor;
	t->color[6] = cyanColor;
	t->color[7] = whiteColor;

	if (t->system.hasColorQD) {
		Rect	r;
		GDHandle	gd;
		
		r = (**(*(WindowPeek)HackWindow).contRgn).rgnBBox;
		LocalToGlobal(&r.top);
		LocalToGlobal(&r.bottom);
		gd = GetMaxDevice(&r);
		t->inColor = (**(**gd).gdPMap).pixelSize > 1;
	}
#endif
		
	temp = GetResource(HACK_DATA, MONST_DATA);
	if (temp) {
		DetachResource(temp);
		MoveHHi(temp);
		HLock(temp);
		i = GetHandleSize(temp);
		mons = (struct permonst *)(*temp);
	} else {
		panic("Can't get MONST resource data.");
	}
	
	temp = GetResource(HACK_DATA, OBJECT_DATA);
	if (temp) {
		DetachResource(temp);
		MoveHHi(temp);
		HLock(temp);
		i = GetHandleSize(temp);
		objects = (struct objclass *)(*temp);
		for (j = 0; j< NROFOBJECTS+1; j++) {
			objects[j].oc_name = sm_obj[j].oc_name;
			objects[j].oc_descr = sm_obj[j].oc_descr;
		}
	} else {
		panic("Can't get OBJECT resource data.");
	}
	
	for (j = 30; j >= 0; j -= 10) {
		for (i = 0; i<=8; i++) {
			t->cursor[i] = GetCursor(100+i+j);	/* self-contained cursors */
		}
	}
	
	(void)aboutBox(0);	
	return 0;
}

/* not really even needed. NH never gets to the end of main(), */
/* so this never gets called */
int
freeterm()
{
	return 0;
}

#ifdef SMALLDATA
/* SOME [:-( ] Mac compilers have a 32K global & static data limit */
/* these routines help the HANDICAPPED things */
void
init_decl()
{
	short	i;
	char	*l;
	extern char **Map;
	
	l = calloc(COLNO , sizeof(struct rm **));
	level.locations = (struct rm **)l;
	l = calloc(ROWNO * COLNO , sizeof(struct rm));
	for (i = 0; i < COLNO; i++) {
	    level.locations[i] = 
		(struct rm *)(l + (i * ROWNO * sizeof(struct rm)));
	}
	
	l = calloc(COLNO , sizeof(struct obj ***));
	level.objects = (struct obj ***)l;
	l = calloc(ROWNO * COLNO , sizeof(struct obj *));
	for (i = 0; i < COLNO; i++) {
	    level.objects[i] = 
		(struct obj **)(l + (i * ROWNO * sizeof(struct obj *)));
	}
	
	l = calloc(COLNO , sizeof(struct monst ***));
	level.monsters = (struct monst ***)l;
	l = calloc(ROWNO * COLNO , sizeof(struct monst *));
	for (i = 0; i < COLNO; i++) {
	    level.monsters[i] = 
		(struct monst **)(l + (i * ROWNO * sizeof(struct monst *)));
	}
	level.objlist = (struct obj *)0L;
	level.monlist = (struct monst *)0L;
	
l = calloc(COLNO, sizeof(char *));
Map = (char **)l;
l = calloc(ROWNO * COLNO, sizeof(char));
for (i = 0; i < COLNO; i++) {
    Map[i] = 
	(char *)(l + (i * ROWNO * sizeof(char)));
}

}

/* Since NetHack usually exits before reaching end of main()	*/
/* this routine could probably left out.	- J.L.		*/
void
free_decl()
{

	free((char *)level.locations[0]);
	free((char *)level.locations);
	free((char *)level.objects[0]);
	free((char *)level.objects);
	free((char *)level.monsters[0]);
	free((char *)level.monsters);
}
#endif /* SMALLDATA */

#define	OPTIONS			"Nethack prefs"

# ifdef AZTEC
#undef OMASK
#define OMASK	O_RDONLY
# else
#undef OMASK
#define OMASK	(O_RDONLY | O_BINARY )
# endif

int
read_config_file()
{
	term_info	*t;
	int optfd;
	Str255	name;
	short	oldVol;
	
	optfd = 0;
	t = (term_info *)GetWRefCon(HackWindow);

	GetVol(name, &oldVol);
	SetVol(0L, t->system.sysVRefNum);
	if ( (optfd = open(OPTIONS, OMASK)) <= 0) {
		SetVol(0L, t->recordVRefNum);
		optfd = open(OPTIONS, OMASK);
	}
	if ( optfd > (short)NULL){
		read_opts(optfd);
		(void) close(optfd);
	}
	SetVol(0L, oldVol);
}

int
write_opts()
{
	int fd;
	short temp_flags;
	term_info	*t;
	boolean		saveWizard, newPrefs = FALSE;

	t = (term_info *)GetWRefCon(HackWindow);
	SetVol(0L, t->system.sysVRefNum);

	if((fd = open(OPTIONS, O_WRONLY | O_BINARY)) <= 0) {
		OSErr	result;
		char	*tmp;
		
		SetVol(0L, t->recordVRefNum);
		if((fd = open(OPTIONS, O_WRONLY | O_BINARY)) <= 0) {
			SetVol(0L, t->system.sysVRefNum);
			tmp = CtoPstr(OPTIONS);
			result = Create((StringPtr)tmp, (short)0, CREATOR, AUXIL_TYPE);
		 	if (result == noErr)
			{
		 		fd = open(OPTIONS, O_WRONLY | O_BINARY);
				newPrefs = TRUE;
			}
		}
	 }

	if (fd < 0)
		pline("can't create options file!");
	else {
		/* if we initially store TRUE for wizard then
		 * the user will have intrinsic wizardry!
		 */
		if (newPrefs) {
			saveWizard = wizard;
			wizard = FALSE;
		}

		write(fd, &flags, sizeof(flags));
	
		write(fd, plname, PL_NSIZ);
	
		write(fd, dogname, 63);
	
		write(fd, catname, 63);
		
		temp_flags = (macflags & fZoomOnContextSwitch) ? 1 : 0;
		write(fd, &temp_flags, sizeof(short));

		write(fd, &altCurs, sizeof(short));
	
#ifdef TUTTI_FRUTTI
		write(fd, pl_fruit, PL_FSIZ);
#endif
		write(fd, inv_order, strlen(inv_order)+1);
		close(fd);

		if (newPrefs) wizard = saveWizard;
	}
	
	SetVol(0L, t->recordVRefNum);
	
	return 0;
}

int
read_opts(fd)
int fd;
{	char tmp_order[20];
	short	temp_flags;
	
	read(fd, (char *)&flags, sizeof(flags));

	read(fd, plname, PL_NSIZ);

	read(fd, dogname, 63);
	
	read(fd, catname, 63);
	
	read(fd, &temp_flags, sizeof(short));
	if (temp_flags & 0x01)
		macflags |= fZoomOnContextSwitch;
	else
		macflags &= ~fZoomOnContextSwitch;

	read(fd, &altCurs, sizeof(short));
	
#ifdef TUTTI_FRUTTI
	read(fd, pl_fruit, PL_FSIZ);
#endif
	read(fd,tmp_order,strlen(inv_order)+1);
	if(strlen(tmp_order) == strlen(inv_order))
		Strcpy(inv_order,tmp_order);
		
	return 0;
}

#endif /* MACOS */
