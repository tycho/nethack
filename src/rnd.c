/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define STRONG_RNG 1

#if defined(STRONG_RNG)
unsigned int good_random(void);
#endif

/* "Rand()"s definition is determined by [OS]conf.h */
#if defined(STRONG_RNG)
# define RND(x) (good_random() % x)
#else
# if defined(UNIX) || defined(RANDOM)
#  define RND(x) (int)(Rand() % (long)(x))
# else
/* Good luck: the bottom order bits are cyclic. */
#  define RND(x) (int)((Rand()>>3) % (x))
# endif
#endif

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <wincrypt.h>
#endif
#include <stdint.h>

#ifdef OVL0

int
rn2(x)		/* 0 <= rn2(x) < x */
int x;
{
#ifdef DEBUG
	if (x <= 0) {
		impossible("rn2(%d) attempted", x);
		return(0);
	}
	x = RND(x);
	return(x);
#else
	return(RND(x));
#endif
}

#endif /* OVL0 */
#ifdef OVLB

int
rnl(x)		/* 0 <= rnl(x) < x; sometimes subtracting Luck */
int x;	/* good luck approaches 0, bad luck approaches (x-1) */
{
	int i;

#ifdef DEBUG
	if (x <= 0) {
		impossible("rnl(%d) attempted", x);
		return(0);
	}
#endif
	i = RND(x);

	if (Luck && rn2(50 - Luck)) {
	    i -= (x <= 15 && Luck >= -5 ? Luck/3 : Luck);
	    if (i < 0) i = 0;
	    else if (i >= x) i = x-1;
	}

	return i;
}

#endif /* OVLB */
#ifdef OVL0

int
rnd(x)		/* 1 <= rnd(x) <= x */
int x;
{
#ifdef DEBUG
	if (x <= 0) {
		impossible("rnd(%d) attempted", x);
		return(1);
	}
	x = RND(x)+1;
	return(x);
#else
	return(RND(x)+1);
#endif
}

#endif /* OVL0 */
#ifdef OVL1

int
d(n,x)		/* n <= d(n,x) <= (n*x) */
int n, x;
{
	int tmp = n;

#ifdef DEBUG
	if (x < 0 || n < 0 || (x == 0 && n != 0)) {
		impossible("d(%d,%d) attempted", n, x);
		return(1);
	}
#endif
	while(n--) tmp += RND(x);
	return(tmp); /* Alea iacta est. -- J.C. */
}

#endif /* OVL1 */
#ifdef OVLB

int
rne(x)
int x;
{
	int tmp, utmp;

	utmp = (u.ulevel < 15) ? 5 : u.ulevel/3;
	tmp = 1;
	while (tmp < utmp && !rn2(x))
		tmp++;
	return tmp;

	/* was:
	 *	tmp = 1;
	 *	while(!rn2(x)) tmp++;
	 *	return(min(tmp,(u.ulevel < 15) ? 5 : u.ulevel/3));
	 * which is clearer but less efficient and stands a vanishingly
	 * small chance of overflowing tmp
	 */
}

int
rnz(i)
int i;
{
#ifdef LINT
	int x = i;
	int tmp = 1000;
#else
	long x = i;
	long tmp = 1000;
#endif
	tmp += rn2(1000);
	tmp *= rne(4);
	if (rn2(2)) { x *= tmp; x /= 1000; }
	else { x *= 1000; x /= tmp; }
	return((int)x);
}

#endif /* OVLB */

#if defined(STRONG_RNG)

#ifdef RND
# undef RND
#endif

#if defined(__SSE2__)
# define HAVE_SSE2 1
#endif
#if defined(__AVX2__)
# define HAVE_AVX2 1
#endif
#define SFMT_MEXP 19937
#include "sfmt/SFMT.h"

#define ENTROPY_BYTES 256

static void collect_entropy(char* data)
{
#if defined(UNIX)
	/* UNIX /dev/random Linux /dev/urandom entropy collector */
#ifdef LINUX
	FILE* f = fopen("/dev/urandom", "rb");
#else
	FILE* f = fopen("/dev/random", "rb");
#endif
	memset(data, 0, ENTROPY_BYTES);

	if (!f)
		return;

	fread(data, ENTROPY_BYTES, 1, f);

	fclose(f);
#elif defined(WIN32)
	/* Windows CryptoAPI PRNG */
	static HCRYPTPROV hProv = 0;
	if (hProv == 0) {
		if (!CryptAcquireContext(&hProv, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
			assert(!"Could not acquire cryptographic context");
	}
	if (!CryptGenRandom(hProv, ENTROPY_BYTES, (BYTE *)data))
		assert(!"Failed to get entropy from system");
#else
#error "Don't know how to gather entropy on this platform."
#endif
}

unsigned int good_random(void)
{
	static uint32_t counter = 0;
	static sfmt_t sfmt;
	char entropy[ENTROPY_BYTES];

	/* Collect entropy and reseed every 2M iterations */
	if ((counter & 0x1fffff) == 0)
	{
		collect_entropy(entropy);

		sfmt_init_by_array(&sfmt, (uint32_t *)entropy, sizeof(entropy) / sizeof(uint32_t));
	}

	counter++;

	return sfmt_genrand_uint32(&sfmt);
}

#include "sfmt/SFMT.c"
#endif

/*rnd.c*/
