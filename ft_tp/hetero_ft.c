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
#include <signal.h>
#include <utime.h>

#define FILENAME "file"
#define FILEUTIME00 "utime00"
#define FILEUTIME01 "utime01"
#define FILEUTIME02 "utime02"

int fd;

void sig()
{
	;
}

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
}

void truncate_file(uint64_t len)
{
	static int num = 1;
	printf("create test %3d (size %-20"PRIu64") ", num++, len);
	if (ftruncate(fd, len)) {
		printf("NG(ftruncate error)\n");
	}
	stat_file(len);
}

void ll_access(uint64_t len)
{
	static int num = 1;
	char a = 's';

	printf("access test %3d seek write read (size %"PRIu64")\n", num++, len);
	printf("                ");
	if (lseek(fd, len, SEEK_SET) != len) {
		printf(" NG \n");
		return;
	} else {
		printf(" OK ");
	}
	if (write(fd, &a, 1) != 1) {
		printf("  NG  \n");
		return;
	}
	if (lseek(fd, -1, SEEK_CUR) < 0) {
		printf("(ignore seek)\n");
		return;
	} else {
		printf("  OK  ");
	}
	if (read(fd, &a, 1) != 1) {
		printf("  NG  \n");
		return;
	}
	if (a != 's') {
		printf("(ignore char)\n");
		return;
	} else {
		printf("  OK ");
	}
	printf("\n");
}

#if BITS_PER_LONG == 32
#endif

void create_test()
{
	/* 001 */
	open_file();
	truncate_file(0);
	close(fd);
	unlink(FILENAME);

	/* 002 */
	open_file();
	truncate_file(1);
	close(fd);
	unlink(FILENAME);

	/* 003 */
	open_file();
	truncate_file(0x7ffffffffffULL);
	close(fd);
	unlink(FILENAME);

	/* 004 */
	open_file();
	truncate_file(0x80000000000ULL);
	close(fd);
	unlink(FILENAME);

	/* 005 */
	open_file();
	truncate_file(0x2fffffffffffffULL);
	close(fd);
	unlink(FILENAME);

	/* 006 */
	open_file();
	truncate_file(0x30000000000000ULL);
	close(fd);
	unlink(FILENAME);
}

void access_test()
{
	/* 007 */
	open_file();
	ll_access(0x0);
	close(fd);
	unlink(FILENAME);

	/* 008 */
	open_file();
	ll_access(0x1);
	close(fd);
	unlink(FILENAME);

	/* 009 */
	open_file();
	ll_access(0x7fffffffffeULL);
	close(fd);
	unlink(FILENAME);

	/* 010 */
	open_file();
	ll_access(0x7ffffffffffULL);
	close(fd);
	unlink(FILENAME);

	/* 011 */
	open_file();
	ll_access(0x27ffffffffffffULL);
	close(fd);
	unlink(FILENAME);

	/* 012 */
	open_file();
	ll_access(0x28000000000000ULL);
	close(fd);
	unlink(FILENAME);
}

void lock_test()
{
	struct flock lock;
	lock.l_type = 0;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0x1ffffffffffULL;
	lock.l_pid = getpid();
	open_file();

	/* 013 */
	printf("SETLK     :");
	lock.l_type = F_RDLCK;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror(" NG fcntl():");
	} else {
		printf("OK\n");
	}
	lock.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror(" NG fork F_UNLCK fcntl(): ");
	}

	/* 014 */
	printf("SETLK64   :");
	lock.l_start = 0x100000000ULL;
	lock.l_type = F_RDLCK;
	if (fcntl(fd, F_SETLK64, &lock) < 0) {
		perror(" NG fcntl():");
	} else {
		printf("OK\n");
	}
	lock.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror(" NG fork F_UNLCK fcntl(): ");
	}

	/* 015-016 */
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0x40000000000ULL;
	lock.l_len = 0x1ffffffffffULL;
	lock.l_pid = getpid();

	switch (fork()) {
	case 0:
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_WRLCK fcntl(): ");
		}
		sleep(3);
		lock.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	printf("GETLK     :");
	sleep(1);
	lock.l_type = F_WRLCK;
	if (fcntl(fd, F_GETLK, &lock) < 0) {
		perror(" NG fcntl():");
	} else {
		if (lock.l_type != F_RDLCK) {
			printf(" NG fcntl(): ignore l_type (missmatch F_UNLCK)\n");
		} else {
			printf("OK\n");
		}
	}
	printf("SETLKW    :");
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		printf("OK\n");
		lock.l_type = F_RDLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
	}
	lock.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror(" NG fork F_UNLCK fcntl(): ");
	}

	/* 017-018 */
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0x40000000000ULL;
	lock.l_len = 0x1ffffffffffULL;
	lock.l_pid = getpid();

	switch (fork()) {
	case 0:
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_WRLCK fcntl(): ");
		}
		sleep(3);
		lock.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	printf("GETLK64   :");
	sleep(1);
	lock.l_type = F_WRLCK;
	if (fcntl(fd, F_GETLK64, &lock) < 0) {
		perror(" NG fcntl():");
	} else {
		if (lock.l_type != F_RDLCK) {
			printf(" NG fcntl(): ignore l_type (missmatch F_UNLCK)\n");
		} else {
			printf("OK\n");
		}
	}
	printf("SETLKW64  :");
	if (fcntl(fd, F_SETLKW64, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		printf("OK\n");
		lock.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
	}
	lock.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror(" NG fork F_UNLCK fcntl(): ");
	}
	close(fd);
}

void utime_test()
{
	struct utimbuf ut;

	if ((fd = open(FILEUTIME00, O_RDWR|O_CREAT, 0666)) < 0) {
		perror("OPEN FILEUTIME00");
		exit(1);
	}
	close(fd);
	if ((fd = open(FILEUTIME01, O_RDWR|O_CREAT, 0666)) < 0) {
		perror("OPEN FILEUTIME01");
		exit(1);
	}
	close(fd);
	if ((fd = open(FILEUTIME02, O_RDWR|O_CREAT, 0666)) < 0) {
		perror("OPEN FILEUTIME02");
		exit(1);
	}
	close(fd);

	printf("utime : [FILEUTIME00] ");
	ut.actime = 0;
	ut.modtime = 0;
	if (utime(FILEUTIME00, &ut) < 0) {
		printf("NG");
	} else {
		printf("OK");
	}

	printf(" [FILEUTIME01] ");
	ut.actime = 0x7fffffff;
	ut.modtime = 0x7fffffff;
	if (utime(FILEUTIME01, &ut) < 0) {
		printf("NG");
	} else {
		printf("OK");
	}

	printf(" [FILEUTIME02] ");
	ut.actime = 0x80000000ULL;
	ut.modtime = 0x80000000ULL;
	if (utime(FILEUTIME02, &ut) < 0) {
		printf("NG");
	} else {
		printf("OK");
	}
	printf("\n");
}

/* main */
int main(int argc, char **argv)
{
	signal(SIGXFSZ, sig);
	create_test();
	access_test();
	lock_test();
	utime_test();
	return 0;
}

