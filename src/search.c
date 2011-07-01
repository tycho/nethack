/*	SCCS Id: @(#)search.c	3.0	88/04/13
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifdef NAMED_ITEMS
#  include "artifact.h"
#endif

#ifdef OVLB

static void FDECL(findone,(XCHAR_P,XCHAR_P,int *));

static void
findone(zx,zy,num)
xchar zx,zy;
int *num;
{
	register struct trap *ttmp;
	register struct monst *mtmp;

	if(levl[zx][zy].typ == SDOOR) {
		levl[zx][zy].typ = DOOR;
		levl[zx][zy].seen = 0;
		prl(zx, zy);
		(*num)++;
	} else if(levl[zx][zy].typ == SCORR) {
		levl[zx][zy].typ = CORR;
		levl[zx][zy].seen = 0;
		prl(zx, zy);
		(*num)++;
	} else if(ttmp = t_at(zx, zy)) {
		if(ttmp->ttyp == MONST_TRAP) {
			(void) makemon(&mons[ttmp->pm], zx, zy);
			(*num)++;
			deltrap(ttmp);
		} else if(!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
			ttmp->tseen = 1;
			if(!vism_at(zx, zy))
		    atl(zx,zy,(char)((ttmp->ttyp==WEB) ? WEB_SYM : TRAP_SYM));
			(*num)++;
		}
	} else if(MON_AT(zx, zy)) {
		mtmp = m_at(zx,zy);
		if(mtmp->mimic) {
		        seemimic(mtmp);
		        (*num)++;
		}
	}
}

int
findit()	/* returns number of things found */
{
	int num;
	register xchar zx,zy;
	xchar lx,hx,ly,hy;
	xchar lx2,hx2,ly2,hy2;

	if(u.uswallow) return(0);
	if(inroom(u.ux,u.uy) < 0) {
		lx = u.ux - 1;
		hx = u.ux + 1;
		ly = u.uy - 1;
		hy = u.uy + 1;
		lx2 = ly2 = 1;
		hx2 = hy2 = 0;
	} else
		getcorners(&lx,&hx,&ly,&hy,&lx2,&hx2,&ly2,&hy2);
	num = 0;
	for(zy = ly; zy <= hy; zy++)
		for(zx = lx; zx <= hx; zx++)
			if(isok(zx,zy))
				findone(zx,zy,&num);
	for(zy = ly2; zy <= hy2; zy++)
		for(zx = lx2; zx <= hx2; zx++)
			if(isok(zx,zy))
				findone(zx,zy,&num);
	return(num);
}

#endif /* OVLB */
#ifdef OVL1

int
dosearch()
{
	return(dosearch0(0));
}

int
dosearch0(aflag)
register int aflag;
{
	register xchar x, y;
	register struct trap *trap;
	register struct monst *mtmp;
	register struct obj *otmp;

#ifdef NAMED_ITEMS
	int fund = (spec_ability(uwep, SPFX_SEARCH)) ?
			((uwep->spe > 5) ? 5 : uwep->spe) : 0;
#endif /* NAMED_ITEMS */

	if(u.uswallow) {
		if (!aflag)
			pline("What are you looking for?  The exit?");
	} else
	    for(x = u.ux-1; x < u.ux+2; x++)
	      for(y = u.uy-1; y < u.uy+2; y++) {
		if(!isok(x,y)) continue;
		if(x != u.ux || y != u.uy) {
		    if(levl[x][y].typ == SDOOR) {
#ifdef NAMED_ITEMS
			if(rnl(7-fund)) continue;
#else
			if(rnl(7)) continue;
#endif
			levl[x][y].typ = DOOR;
			levl[x][y].seen = 0;	/* force prl */
			nomul(0);
			prl(x,y);
		    } else if(levl[x][y].typ == SCORR) {
#ifdef NAMED_ITEMS
			if(rnl(7-fund)) continue;
#else
			if(rnl(7)) continue;
#endif
			levl[x][y].typ = CORR;
			levl[x][y].seen = 0;	/* force prl */
			nomul(0);
			prl(x,y);
		    } else {
		/* Be careful not to find anything in an SCORR or SDOOR */
			if(MON_AT(x, y)) {
			    mtmp = m_at(x,y);
			    if(!aflag && mtmp->mimic) {
				seemimic(mtmp);
				You("find %s.", defmonnam(mtmp));
				return(1);
			    }
			}

			for(trap = ftrap; trap; trap = trap->ntrap)
			    if(trap->tx == x && trap->ty == y &&
				!trap->tseen && !rnl(8)) {
				nomul(0);
				if(trap->ttyp != MONST_TRAP &&
				   trap->ttyp != STATUE_TRAP)
				You("find a%s.", traps[Hallucination ?
				rn2(TRAPNUM-3)+2 : trap->ttyp ]);

				if(trap->ttyp == MONST_TRAP) {
				    if((mtmp=makemon(&mons[trap->pm], x, y)))
					You("find %s.", defmonnam(mtmp));
				    deltrap(trap);
				    return(1);
				} else if(trap->ttyp == STATUE_TRAP) {
				    if((otmp = sobj_at(STATUE, x, y)))
				      if(otmp->corpsenm == trap->pm) {
					    
					if((mtmp=makemon(&mons[trap->pm], x, y)))
					    You("find %s posing as a statue.",
						  defmonnam(mtmp));
					delobj(otmp);
					newsym(x, y);
				      }
				    deltrap(trap);
				    return(1);
				}
				trap->tseen = 1;
				if(!vism_at(x,y))
					atl(x,y,(char) ((trap->ttyp==WEB)
						? WEB_SYM : TRAP_SYM));
			    }
		    }
		}
	      }
	return(1);
}

#endif /* OVL1 */
#ifdef OVLB

int
doidtrap() {
	register struct trap *trap;
	register int x,y;

	if(!getdir(1)) return 0;
	x = u.ux + u.dx;
	y = u.uy + u.dy;
	for(trap = ftrap; trap; trap = trap->ntrap)
		if(trap->tx == x && trap->ty == y && trap->tseen) {
		    if(u.dz)
			if((u.dz < 0) != (is_maze_lev && trap->ttyp == TRAPDOOR))
			    continue;
			pline("That is a%s.",traps[ Hallucination ? rn2(TRAPNUM-3)+3 :
			trap->ttyp]);
		    return 0;
		}
	pline("I can't see a trap there.");
	return 0;
}

#endif /* OVLB */
#ifdef OVL0

void
wakeup(mtmp)
register struct monst *mtmp;
{
	mtmp->msleep = 0;
	mtmp->meating = 0;	/* assume there's no salvagable food left */
	setmangry(mtmp);
	if(mtmp->mimic) seemimic(mtmp);
}

#endif /* OVL0 */
#ifdef OVLB

/* NOTE: we must check if(mtmp->mimic) before calling this routine */
void
seemimic(mtmp)
register struct monst *mtmp;
{
	mtmp->mimic = 0;
	mtmp->m_ap_type = M_AP_NOTHING;
	mtmp->mappearance = 0;
	unpmon(mtmp);
	pmon(mtmp);
}

#endif /* OVLB */
