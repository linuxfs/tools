#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <sys/file.h>

#define ERR (-1)

/* for stat item map */
typedef struct string_stat_map {
	char *title;
	uint64_t size;
} filename_t;

filename_t table[] = {
	{ "zero.test",  0x0,              },
	{ "8k.test",    0x2000,           },
	{ "8k+1.test",  0x2001,           },
	{ "16k.test",   0x4000,           },
	{ "16k+1.test", 0x4001,           },
	{ "32k.test",   0x8000,           },
	{ "64k.test",   0x10000,          },
	{ "128k.test",  0x20000,          },
	{ "2t-4k.test", 0x1fffffff000ULL, },
	{ "4t.test",    0x40000000000ULL, },
	{ "8t.test",    0x80000000000ULL, },
	{ "16t.test",   0x100000000000ULL, },
	{ "32t.test",   0x200000000000ULL, },
	{ "64t.test",   0x400000000000ULL, },
	{ "100t.test",  0x640000000000ULL, },
	{ "200t.test",  0xc80000000000ULL, },
	{ "300t.test",  0x12c0000000000ULL, },
	{ "350t.test",  0x15e0000000000ULL, },
	{ NULL,         0x0,              }
};

/** open file */
int open_file(const char* filename)
{
	int fd;
	/* write read open */
	if ((fd = open(filename, O_RDWR|O_CREAT, 0666)) < 0) {
		printf(" NG \n");
		perror("OPEN");
		return ERR;
	}
	return fd;
}

/** truncate_file */
void truncate_file(int fd, uint64_t size)
{
	if (ftruncate(fd, size) < 0) {
		printf(" NG \n");
		perror("TRUNCATE");
		close(fd);
		return;
	}
	close(fd);
	printf(" OK \n");
}

/** title */
void title(const char* filename)
{
	static int num = 1;
	printf("create open truncate test %3d  %10s", num++, filename);
}

/* main */
int main(int argc, char **argv)
{
	int fd, i;

	i = 0;
	for (i = 0; table[i].title != NULL; i++)
	{
		title(table[i].title);
		if ((fd = open_file(table[i].title)) < 0) {
			continue;
		}
		truncate_file(fd, table[i].size);
	}
	return 0;
}

