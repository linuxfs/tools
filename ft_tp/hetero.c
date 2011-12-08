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
#include <sys/stat.h>

#define FILENAME "file"

int fd;

/* open file */
void open_file(void)
{
	/* write read open */
	if ((fd = open(FILENAME, O_RDWR|O_CREAT, 0666)) < 0) {
		perror("OPEN");
		exit(1);
	}
}

void stat_file(uint64_t size)
{
	struct stat st;

	/* stat */
	if ((fd = lstat(FILENAME, &st)) < 0) {
		printf("NG(stat error)\n");
		exit(1);
	}
	if (size != st.st_size) {
		printf("NG(ignore size) ");
	}
	printf("OK\n");
	unlink(FILENAME);
}

void truncate_file(uint64_t len)
{
	static int num = 1;
	printf("create test %d ", num++);
	if (ftruncate(fd, len)) {
		printf("NG(ftruncate error)\n");
	}
	stat_file(len);
}

#if BITS_PER_LONG == 32
#endif

void create_test()
{
	/* 001 */
	open_file();
	truncate_file(0);
	close(fd);

	/* 002 */
	open_file();
	truncate_file(1);
	close(fd);

	/* 003 */
	open_file();
	truncate_file(0x7ffffffffff);
	close(fd);

	/* 004 */
	open_file();
	truncate_file(0x80000000000);
	close(fd);

	/* 005 */
	open_file();
	truncate_file(0x2fffffffffffff);
	close(fd);

	/* 006 */
	open_file();
	truncate_file(0x30000000000000);
	close(fd);
}

/* main */
int main(int argc, char **argv)
{
	create_test();
	return 0;
}

