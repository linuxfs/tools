#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>

int	fd;

static void do_signal(int signr)
{
	printf("\n");
	printf("signal: %s(%d)\n", strsignal(signr), signr);
/*	printf("signal exit: errno  %s(%d)\n", strerror(errsv), errsv);
	exit (errno);
*/
}

void print_size(uint64_t size)
{
	char unit[8][2]={ " ", "K", "M", "G", "T", "P", "E" };
	int i = 0;

	/* change value */
	while (size >= 1024 && i < 8)
	{
		size /= 1024;
		i++;
	}

	if (size < 1024) printf("seek %5"PRIu64" %s ", size, unit[i]);
	else printf("-----  ");
	fflush(stdout);
}

void seekrw(uint64_t size)
{
	char	 buf='\0';

	if (lseek(fd, size, SEEK_SET) < 0) {
		printf("lseek(1) %s(%d)\n", strerror(errno), errno);
		exit(1);
	}
	printf(".");
	fflush(stdout);

	if (write(fd, &buf, sizeof(char)) != sizeof(char)) {
		printf("write %s(%d)\n", strerror(errno), errno);
		exit(1);
	}
	printf(".");
	fflush(stdout);

	if (lseek(fd, size, SEEK_SET) < 0) {
		printf("lseek(1) %s(%d)\n", strerror(errno), errno);
		exit(1);
	}
	printf(".");
	fflush(stdout);

	if (read(fd, &buf, sizeof(char)) != sizeof(char)) {
		printf("read %s(%d)\n", strerror(errno), errno);
		exit(1);
	}
	printf(". \n");
	fflush(stdout);
}

void set_signal(void)
{
	struct sigaction act;
	sigset_t block_mask;

	sigemptyset(&block_mask);
	sigaddset(&block_mask, SIGXFSZ);

	act.sa_handler = do_signal;
	act.sa_mask = block_mask;
	act.sa_flags = 0;

	sigaction(SIGXFSZ, &act, NULL);
}



int main(int argc, char **argv)
{
	uint64_t size;
	char *file;

	set_signal();
	file = (argc > 1) ? argv[1] : "io-bench.tmp";


	if ((fd = open(file, O_RDWR|O_CREAT, 0666)) == -1) {
		perror("open");
		exit(255);
	}

	for (size = 8192; size <= 0xFFFFFFFFFFFFFFFFULL;) {

		print_size(size);
		printf("-4096-2 ");
		seekrw(size - 4096 - 2);

		print_size(size);
		printf("-4096-1 ");
		seekrw(size - 4096 - 1);

		print_size(size);
		printf("-4096 ");
		seekrw(size - 4096);

		print_size(size);
		printf("-2 ");
		seekrw(size - 2);

		print_size(size);
		printf("-1 ");
		seekrw(size - 1);


		print_size(size);
		printf("   ");
		seekrw(size);

		print_size(size);
		printf("+1 ");
		seekrw(size + 1);
		size = (size > 0x20000000000ULL) ? (size + 0x20000000000ULL) : (size * 2);
	}
	close(fd);
	return 0;
}

