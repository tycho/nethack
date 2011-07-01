/*	SCCS Id: @(#)vault.h	3.2	92/04/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef VAULT_H
#define VAULT_H

#define FCSIZ	(ROWNO+COLNO)
struct fakecorridor {
	xchar fx,fy,ftyp;
};

#ifndef DUNGEON_H
#include "dungeon.h"
#endif

struct egd {
	int fcbeg, fcend;	/* fcend: first unused pos */
	xchar gdx, gdy;		/* goal of guard's walk */
	xchar ogx, ogy;		/* guard's last position */
	d_level gdlevel;	/* level (& dungeon) guard was created in */
	xchar warncnt;		/* number of warnings to follow */
	int vroom;		/* room number of the vault */
	Bitfield(gddone,1);	/* true iff guard has released player */
	Bitfield(unused,7);
	struct fakecorridor fakecorr[FCSIZ];
};

#define EGD(mon)	((struct egd *)&(mon)->mextra[0])

#endif /* VAULT_H */
