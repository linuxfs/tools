#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char *p;
	size_t sz = 50 * 1024 * 1024L;

	switch (argc) {
	case 1:
		break;
	case 2:
		sz = strtol(argv[1], NULL, 0) * 1024 * 1024;
		break;
	default:
		fprintf(stderr, "usage: %s [MB]\n", argv[0]);
		exit(1);
	}
	p = malloc(sz);
	if (p == NULL) {
		perror("malloc");
		exit(1);
	}
	daemon(0, 0);

	for (;;) {
		memset(p, 0, sz);
		sleep(10 * 60);
	}
}
