#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hack.h"

extern unsigned int good_random(void);

int main(void)
{
	while(1) {
		unsigned int r[1024];
		unsigned int i;
		for (i = 0; i < sizeof(r)/sizeof(r[0]); i++)
		{
			r[i] = good_random();
		}
		write(STDOUT_FILENO, r, sizeof(r));
	}
	return 0;
}
