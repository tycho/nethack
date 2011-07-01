/*	SCCS Id: @(#)dogmove.c	1.4	87/08/08
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* dogmove.c - version 1.0 */

#include	"hack.h"
#include "mfndpos.h"
#include "mkroom.h"
#include "edog.h"

/* return 0 (no move), 1 (move) or 2 (dead) */
dog_move(mtmp, after) register struct monst *mtmp; {
#ifndef REGBUG
register
#endif
	 int nx,ny,omx,omy,appr,nearer,j;
int udist,chi,i,whappr;
register struct monst *mtmp2;
register struct permonst *mdat = mtmp->data;
register struct edog *edog = EDOG(mtmp);
struct obj *obj;
struct trap *trap;
xchar cnt,chcnt,nix,niy;
schar dogroom,uroom;
xchar gx,gy,gtyp,otyp;	/* current goal */
coord poss[9];
long info[9];
#define GDIST(x,y) ((x-gx)*(x-gx) + (y-gy)*(y-gy))
#define DDIST(x,y) ((x-omx)*(x-omx) + (y-omy)*(y-omy))

	if(moves <= edog->eattime) return(0);	/* dog is still eating */
	omx = mtmp->mx;
	omy = mtmp->my;
	whappr = (moves - EDOG(mtmp)->whistletime < 5);
	if(moves > edog->hungrytime + 500 && !mtmp->mconf){
		mtmp->mconf = 1;
		mtmp->mhpmax /= 3;
		if(mtmp->mhp > mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;
		if(cansee(omx,omy))
			pline("%s is confused from hunger.", Monnam(mtmp));
		else	pline("You feel worried about %s.", monnam(mtmp));
	} else
	if(moves > edog->hungrytime + 750 || mtmp->mhp < 1){
#ifdef WALKIES
		if(mtmp->mleashed) {
			mtmp->mleashed = 0;
			pline("Your leash goes slack...");
		}
#endif
		if(cansee(omx,omy))
			pline("%s dies%s.", Monnam(mtmp),
			      (mtmp->mhp >= 1) ? "" : " from hunger");
		else
			pline("You have a sad feeling for a moment, then it passes.");
		mondied(mtmp);
		return(2);
	}
	dogroom = inroom(omx,omy);
	uroom = inroom(u.ux,u.uy);
	udist = dist(omx,omy);

	/* maybe we tamed him while being swallowed --jgm */
	if(!udist) return(0);

	/* if we are carrying sth then we drop it (perhaps near @) */
	/* Note: if apport == 1 then our behaviour is independent of udist */
	if(mtmp->minvent){
		if(!rn2(udist) || !rn2((int) edog->apport))
		if(rn2(10) < edog->apport){
			relobj(mtmp, (int) mtmp->minvis);
			if(edog->apport > 1) edog->apport--;
			edog->dropdist = udist;		/* hpscdi!jon */
			edog->droptime = moves;
		}
	} else {
		if(obj = o_at(omx,omy)) if(!index("0_", obj->olet)){
		    if((otyp = dogfood(obj)) <= CADAVER){
			nix = omx;
			niy = omy;
			goto eatobj;
		    }
		    if(obj->owt < 10*mtmp->data->mlevel)
		    if(rn2(20) < edog->apport+3)
		    if(rn2(udist) || !rn2((int) edog->apport)){
			freeobj(obj);
			unpobj(obj);
			/* if(levl[omx][omy].scrsym == obj->olet)
				newsym(omx,omy); */
			mpickobj(mtmp,obj);
		    }
		}
	}

	gtyp = UNDEF;	/* no goal as yet */
	gx = gy = 0;	/* suppress 'used before set' message */
#ifdef WALKIES
	/* If he's on a leash, he's not going anywhere. */
	if(mtmp->mleashed) {

		gtyp = APPORT;
		gx = u.ux;
		gy = u.uy;
	} else
#endif
	/* first we look for food */
	    for(obj = fobj; obj; obj = obj->nobj) {
		otyp = dogfood(obj);
		if(otyp > gtyp || otyp == UNDEF) continue;
		if(inroom(obj->ox,obj->oy) != dogroom) continue;
		if(otyp < MANFOOD &&
		 (dogroom >= 0 || DDIST(obj->ox,obj->oy) < 10)) {
			if(otyp < gtyp || (otyp == gtyp &&
				DDIST(obj->ox,obj->oy) < DDIST(gx,gy))){
				gx = obj->ox;
				gy = obj->oy;
				gtyp = otyp;
			}
		} else
		if(gtyp == UNDEF && dogroom >= 0 &&
		   uroom == dogroom &&
		   !mtmp->minvent && edog->apport > rn2(8)){
			gx = obj->ox;
			gy = obj->oy;
			gtyp = APPORT;
		}
	    }

	if(gtyp == UNDEF ||
	  (gtyp != DOGFOOD && gtyp != APPORT && moves < edog->hungrytime)){
		if(dogroom < 0 || dogroom == uroom){
			gx = u.ux;
			gy = u.uy;
#ifndef QUEST
		} else {
			int tmp = rooms[dogroom].fdoor;
			    cnt = rooms[dogroom].doorct;

			gx = gy = FAR;	/* random, far away */
			while(cnt--){
			    if(dist(gx,gy) >
				dist(doors[tmp].x, doors[tmp].y)){
					gx = doors[tmp].x;
					gy = doors[tmp].y;
				}
				tmp++;
			}
			/* here gx == FAR e.g. when dog is in a vault */
			if(gx == FAR || (gx == omx && gy == omy)){
				gx = u.ux;
				gy = u.uy;
			}
#endif
		}
		appr = (udist >= 9) ? 1 : (mtmp->mflee) ? -1 : 0;
		if(after && udist <= 4 && gx == u.ux && gy == u.uy)
			return(0);
		if(udist > 1){
			if(!IS_ROOM(levl[u.ux][u.uy].typ) || !rn2(4) ||
			   whappr ||
			   (mtmp->minvent && rn2((int) edog->apport)))
				appr = 1;
		}
		/* if you have dog food he'll follow you more closely */
		if(appr == 0){
			obj = invent;
			while(obj){
				if(obj->otyp == TRIPE_RATION){
					appr = 1;
					break;
				}
				obj = obj->nobj;
			}
		}
	} else	appr = 1;	/* gtyp != UNDEF */
	if(mtmp->mconf) appr = 0;

	if(gx == u.ux && gy == u.uy && (dogroom != uroom || dogroom < 0)) {
	extern coord *gettrack();
	register coord *cp;
		cp = gettrack(omx,omy);
		if(cp){
			gx = cp->x;
			gy = cp->y;
		}
	}

	nix = omx;
	niy = omy;
	cnt = mfndpos(mtmp,poss,info,ALLOW_M | ALLOW_TRAPS);
	chcnt = 0;
	chi = -1;
	for(i=0; i<cnt; i++){
		nx = poss[i].x;
		ny = poss[i].y;
#ifdef WALKIES
		/* if leashed, we drag him along. */
		if(dist(nx, ny) > 4 && mtmp->mleashed) continue;
#endif
		if(info[i] & ALLOW_M) {
			mtmp2 = m_at(nx,ny);
			if(mtmp2)
			    if(mtmp2->data->mlevel >= mdat->mlevel+2 ||
			       mtmp2->data->mlet == 'c')
				continue;
			if(after) return(0); /* hit only once each move */

			if(hitmm(mtmp, mtmp2) == 1 && rn2(4) &&
			  mtmp2->mlstmv != moves &&
			  hitmm(mtmp2,mtmp) == 2) return(2);
			return(0);
		}

		/* dog avoids traps */
		/* but perhaps we have to pass a trap in order to follow @ */
		if((info[i] & ALLOW_TRAPS) && (trap = t_at(nx,ny))){
			if(!trap->tseen && rn2(40)) continue;
			if(rn2(10)) continue;
		}

		/* dog eschewes cursed objects */
		/* but likes dog food */
		obj = fobj;
		while(obj){
		    if(obj->ox != nx || obj->oy != ny)
			goto nextobj;
		    if(obj->cursed) goto nxti;
		    if(obj->olet == FOOD_SYM &&
			(otyp = dogfood(obj)) < MANFOOD &&
			(otyp < ACCFOOD || edog->hungrytime <= moves)){
			/* Note: our dog likes the food so much that he
			might eat it even when it conceals a cursed object */
			nix = nx;
			niy = ny;
			chi = i;
		     eatobj:
			edog->eattime =
			    moves + obj->quan * objects[obj->otyp].oc_delay;
			if(edog->hungrytime < moves)
			    edog->hungrytime = moves;
			edog->hungrytime +=
			    5*obj->quan * objects[obj->otyp].nutrition;
			mtmp->mconf = 0;
			if(cansee(nix,niy))
			    pline("%s ate %s.", Monnam(mtmp), doname(obj));
			/* perhaps this was a reward */
			if(otyp != CADAVER)
			edog->apport += 200/(edog->dropdist+moves-edog->droptime);
			delobj(obj);
			goto newdogpos;
		    }
		nextobj:
		    obj = obj->nobj;
		}

		for(j=0; j<MTSZ && j<cnt-1; j++)
			if(nx == mtmp->mtrack[j].x && ny == mtmp->mtrack[j].y)
				if(rn2(4*(cnt-j))) goto nxti;

/* Some stupid C compilers cannot compute the whole expression at once. */
		nearer = GDIST(nx,ny);
		nearer -= GDIST(nix,niy);
		nearer *= appr;
		if((nearer == 0 && !rn2(++chcnt)) || nearer<0 ||
			(nearer > 0 && !whappr &&
				((omx == nix && omy == niy && !rn2(3))
				|| !rn2(12))
			)){
			nix = nx;
			niy = ny;
			if(nearer < 0) chcnt = 0;
			chi = i;
		}
	nxti:	;
	}
newdogpos:
	if(nix != omx || niy != omy){
		if(info[chi] & ALLOW_U){
			(void) hitu(mtmp, d(mdat->damn, mdat->damd)+1);
			return(0);
		}
		mtmp->mx = nix;
		mtmp->my = niy;
		for(j=MTSZ-1; j>0; j--) mtmp->mtrack[j] = mtmp->mtrack[j-1];
		mtmp->mtrack[0].x = omx;
		mtmp->mtrack[0].y = omy;
	}
#ifdef WALKIES
	  /* an incredible kluge, but the only way to keep pooch near
	   * after he spends time eating or in a trap, etc...
	   */
	  else  if(mtmp->mleashed && dist(omx, omy) > 4) mnexto(mtmp);
#endif

	if(mintrap(mtmp) == 2)	{		/* he died */
#ifdef WALKIES
		mtmp->mleashed = 0;
#endif
		return(2);
	}
	pmon(mtmp);
	return(1);
}
