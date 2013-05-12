#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hack.h"

typedef unsigned int u32;

u32 roll(u32 count, u32 sides)
{
	u32 ret = 0, c;
	for (c = 0; c < count; c++) {
		u32 v = rnd(sides);
		ret += v;
		printf("%d ", v);
		if (c != count - 1)
			printf("+ ");
	}
	return ret;
}

void roll_str(const char *str)
{
	char *t = strcpy(malloc(strlen(str)), str),
		 *p = strtok(t, " \t\r\n");
	do {
		u32 c = 1, d = 0, r;

		if (sscanf(p, "%dd%d", &c, &d) < 2) {
			if (sscanf(p, "d%d", &d) < 1) {
				fprintf(stderr, "Syntax error in roll: %s\n", p);
				continue;
			}
		}

		r = roll(c, d);
		if (c > 1)
			printf("= %d\n", r);
		else
			printf("\n");
	} while (p = strtok(NULL, " \t\r\n"));
	free(t);
}

int main(void)
{
	char buf[64];
	while (scanf("%s", buf)) {
		roll_str(buf);
	}
	return 0;
}
