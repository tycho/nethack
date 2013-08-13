/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NitroHack may be freely redistributed.  See license for details. */

#include "hack.h"

/*
 * Hunger texts used on bottom line.
 * Keep this matched up with hunger states in hack.h!
 */
static const char *const hu_stat[] = {
	"Satiated",
	NULL,
	"Hungry",
	"Weak",
	"Fainting",
	"Fainted",
	"Starved"
};

static const char *const enc_stat[] = {
	NULL,
	"Burdened",
	"Stressed",
	"Strained",
	"Overtaxed",
	"Overloaded"
};

static const char *const trap_stat[] = {
	"Beartrap",
	"Pit",
	"Web",
	"Lava",
	"Infloor",
	"Swamp",
};


static int mrank_sz = 0; /* loaded by max_rank_sz (from u_init) */

static const char *rank(void);
static int xlev_to_rank(int);
static long botl_score(void);
static void describe_level(char *, int);


/* convert experience level (1..30) to rank index (0..8) */
static int xlev_to_rank(int xlev)
{
	return (xlev <= 2) ? 0 : (xlev <= 30) ? ((xlev + 2) / 4) : 8;
}

const char *rank_of(int lev, short monnum, boolean female)
{
	struct Role *role;
	int i;


	/* Find the role */
	for (role = (struct Role *) roles; role->name.m; role++)
	    if (monnum == role->malenum || monnum == role->femalenum)
	    	break;
	if (!role->name.m)
	    role = &urole;

	/* Find the rank */
	for (i = xlev_to_rank((int)lev); i >= 0; i--) {
	    if (female && role->rank[i].f) return role->rank[i].f;
	    if (role->rank[i].m) return role->rank[i].m;
	}

	/* Try the role name, instead */
	if (female && role->name.f) return role->name.f;
	else if (role->name.m) return role->name.m;
	return "Player";
}


static const char *rank(void)
{
	return rank_of(u.ulevel, Role_switch, flags.female);
}

int title_to_mon(const char *str, int *rank_indx, int *title_length)
{
	int i, j;


	/* Loop through each of the roles */
	for (i = 0; roles[i].name.m; i++)
	    for (j = 0; j < 9; j++) {
	    	if (roles[i].rank[j].m && !strncmpi(str,
	    			roles[i].rank[j].m, strlen(roles[i].rank[j].m))) {
	    	    if (rank_indx) *rank_indx = j;
	    	    if (title_length) *title_length = strlen(roles[i].rank[j].m);
	    	    return roles[i].malenum;
	    	}
	    	if (roles[i].rank[j].f && !strncmpi(str,
	    			roles[i].rank[j].f, strlen(roles[i].rank[j].f))) {
	    	    if (rank_indx) *rank_indx = j;
	    	    if (title_length) *title_length = strlen(roles[i].rank[j].f);
	    	    return ((roles[i].femalenum != NON_PM) ?
	    	    		roles[i].femalenum : roles[i].malenum);
	    	}
	    }
	return NON_PM;
}


void max_rank_sz(void)
{
	int i, r, maxr = 0;
	for (i = 0; i < 9; i++) {
	    if (urole.rank[i].m && (r = strlen(urole.rank[i].m)) > maxr) maxr = r;
	    if (urole.rank[i].f && (r = strlen(urole.rank[i].f)) > maxr) maxr = r;
	}
	mrank_sz = maxr;
	return;
}


static long botl_score(void)
{
    int deepest = deepest_lev_reached(FALSE);
    long umoney = money_cnt(invent) + hidden_gold();

    if ((umoney -= u.umoney0) < 0L) umoney = 0L;
    return umoney + u.urscore + (long)(50 * (deepest - 1))
			  + (long)(deepest > 30 ? 10000 :
				   deepest > 20 ? 1000*(deepest - 20) : 0);
}


/* provide the name of the current level for display by various ports */
/*
 * 'v'erbosity:
 * - 0 = Dlvl or only important level names (NetHack)
 * - 1 = short dungeon name (AceHack and NetHack4)
 * - 2 = full dungeon name (UnNetHack show_dgn_name option)
 */
static void describe_level(char *buf, int v)
{
	if (Is_knox(&u.uz)) {
	    sprintf(buf, "%s", dungeons[u.uz.dnum].dname);
	} else if (Is_blackmarket(&u.uz)) {
	    if (v == 1) sprintf(buf, "BlackMrkt:%d", depth(&u.uz));
	    else sprintf(buf, "Black Market");
	} else if (In_quest(&u.uz)) {
	    sprintf(buf, "Home%c%d", (v == 0 ? ' ' : ':'), dunlev(&u.uz));
	} else if (In_endgame(&u.uz)) {
	    sprintf(buf, Is_astralevel(&u.uz) ? "Astral Plane" : "End Game");

	} else if (Is_minetown_level(&u.uz)) {
	    sprintf(buf, (v == 1 ? "%s%d" : "%s%-2d"), "Mine Town:", depth(&u.uz));
	} else if (In_mines(&u.uz) && v == 1) {
	    sprintf(buf, "Mines:%d", depth(&u.uz));
	} else if (In_sokoban(&u.uz) && v == 1) {
	    sprintf(buf, "Sokoban:%d", depth(&u.uz));
	} else if (Is_town_level(&u.uz)) {
	    sprintf(buf, "Town:%d", depth(&u.uz));

	} else if (Is_valley(&u.uz) && v > 0) {
	    sprintf(buf, "Valley:%d", depth(&u.uz));
	} else if (In_V_tower(&u.uz) && v == 1) {
	    sprintf(buf, "Vlad:%d", depth(&u.uz));
	} else if (In_dragon(&u.uz) && v == 1) {
	    sprintf(buf, "Dragon:%d", depth(&u.uz));
	} else if (In_hell(&u.uz) && v == 1) {
	    sprintf(buf, "Gehennom:%d", depth(&u.uz));

	} else {
	    if (v == 1) {
		sprintf(buf, "Dungeons:%d", depth(&u.uz));
	    } else if (v == 2) {
		const char *dgn_name = dungeons[u.uz.dnum].dname;
		if (!strncmpi(dgn_name, "The ", 4)) dgn_name += 4;
		sprintf(buf, "%s:%-2d", dgn_name, depth(&u.uz));
	    } else {
		sprintf(buf, "Dlvl:%-2d", depth(&u.uz));
	    }
	}
}


static void make_player_info(struct nh_player_info *pi)
{
	int cap, advskills, i;
	
	memset(pi, 0, sizeof(struct nh_player_info));
	
	pi->moves = moves;
	strncpy(pi->plname, plname, sizeof(pi->plname));
	pi->align = u.ualign.type;
	
	/* This function could be called before the game is fully inited.
	 * Test youmonst.data as it is required for near_capacity().
	 * program_state.game_running is no good, as we need this data before 
	 * game_running is set */
	if (!youmonst.data || !api_entry_checkpoint())
	    return;
	
	pi->x = u.ux;
	pi->y = u.uy;
	pi->z = u.uz.dlevel;

	strncpy(pi->race_adj, urace.adj, sizeof(pi->race_adj));
	upstart(pi->race_adj);

	if (Upolyd) {
		char mbot[BUFSZ];
		int k = 0;

		strcpy(mbot, mons_mname(&mons[u.umonnum]));
		while (mbot[k] != 0) {
		    if ((k == 0 || (k > 0 && mbot[k-1] == ' ')) &&
					'a' <= mbot[k] && mbot[k] <= 'z')
			mbot[k] += 'A' - 'a';
		    k++;
		}
		strncpy(pi->rank, mbot, sizeof(pi->rank));
	} else
		strncpy(pi->rank, rank(), sizeof(pi->rank));
	
	pi->max_rank_sz = mrank_sz;
	
	/* abilities */
	pi->st = ACURR(A_STR);
	pi->st_extra = 0;
	if (pi->st > 118) {
		pi->st = pi->st - 100;
		pi->st_extra = 0;
	} else if (pi->st > 18) {
		pi->st_extra = pi->st - 18;
		pi->st = 18;
	}
	
	pi->dx = ACURR(A_DEX);
	pi->co = ACURR(A_CON);
	pi->in = ACURR(A_INT);
	pi->wi = ACURR(A_WIS);
	pi->ch = ACURR(A_CHA);
	
	pi->score = botl_score();
	
	/* hp and energy */
	pi->hp = Upolyd ? u.mh : u.uhp;
	pi->hpmax = Upolyd ? u.mhmax : u.uhpmax;
	if (pi->hp < 0)
	    pi->hp = 0;
	
	pi->en = u.uen;
	pi->enmax = u.uenmax;
	pi->ac = u.uac;
	
	pi->gold = money_cnt(invent);
	pi->coinsym = def_oc_syms[COIN_CLASS];

	describe_level(pi->levdesc_dlvl, 0);
	describe_level(pi->levdesc_short, 1);
	describe_level(pi->levdesc_full, 2);

	pi->monnum = u.umonster;
	pi->cur_monnum = u.umonnum;
	
	/* level and exp points */
	if (Upolyd)
	    pi->level = mons[u.umonnum].mlevel;
	else
	    pi->level = u.ulevel;
	pi->xp = u.uexp;
	pi->xp_next = newuexp(pi->level);

	/* weight and inventory slot count */
	pi->wt = weight_cap() + inv_weight();
	pi->wtcap = weight_cap();
	pi->invslots = inv_cnt();

	/* check if any skills could be anhanced */
	advskills = 0;
	for (i = 0; i < P_NUM_SKILLS; i++) {
	    if (P_RESTRICTED(i))
		continue;
	    if (can_advance(i, FALSE))
		advskills++;
	}
	pi->can_enhance = advskills > 0;
	
	/* add status items for various problems */
	if (hu_stat[u.uhs]) /* 1 */
	    strncpy(pi->statusitems[pi->nr_items++], hu_stat[u.uhs], ITEMLEN);
	
	if (Confusion) /* 2 */
	    strncpy(pi->statusitems[pi->nr_items++], "Conf", ITEMLEN);
	
	if (Sick) {
	    if (u.usick_type & SICK_VOMITABLE) /* 3 */
		strncpy(pi->statusitems[pi->nr_items++], "FoodPois", ITEMLEN);
	    if (u.usick_type & SICK_NONVOMITABLE) /* 4 */
		strncpy(pi->statusitems[pi->nr_items++], "Ill", ITEMLEN);
	}
	if (Blind) /* 5 */
	    strncpy(pi->statusitems[pi->nr_items++], "Blind", ITEMLEN);
	if (Stunned) /* 6 */
	    strncpy(pi->statusitems[pi->nr_items++], "Stun", ITEMLEN);
	if (Hallucination) /* 7 */
	    strncpy(pi->statusitems[pi->nr_items++], "Hallu", ITEMLEN);
	if (Slimed) /* 8 */
	    strncpy(pi->statusitems[pi->nr_items++], "Slime", ITEMLEN);
	cap = near_capacity();
	if (enc_stat[cap]) /* 9 */
	    strncpy(pi->statusitems[pi->nr_items++], enc_stat[cap], ITEMLEN);
	if (Levitation) /* 10 */
	    strncpy(pi->statusitems[pi->nr_items++], "Lev", ITEMLEN);
	if (unweapon) /* 11 */
	    strncpy(pi->statusitems[pi->nr_items++], "Unarmed", ITEMLEN);
	if (u.utrap) /* 12 */
	    strncpy(pi->statusitems[pi->nr_items++], trap_stat[u.utraptype], ITEMLEN);
	if (!Blind && sengr_at("Elbereth", u.ux, u.uy)) /* 13 */
	    strncpy(pi->statusitems[pi->nr_items++], "Elbereth", ITEMLEN);
	/* maximum == STATUSITEMS_MAX (24) */

	api_exit();
}


void bot(void)
{
	struct nh_player_info pi;
	
	make_player_info(&pi);
	update_status(&pi);
	iflags.botl = 0;
}

/*botl.c*/
