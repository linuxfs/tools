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

#define LOCK_LOG "/var/pfs/lock_dump"
#define FILENAME "file"

struct unlock_t {
	uint64_t seek_pt;
	uint64_t len;
	struct unlock_t *next;
	char   pad[4];
};

int fd;
pid_t lockd_pid;
char buf[4095];
struct unlock_t *head = NULL;

/* UnLock */
int unlock(uint64_t seek_pt, uint64_t len)
{
	/* set Lock Table F_UNLCK */
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* UnLock */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): UnLock ");
		return -1;
	}
	return 0;
}

/* open file */
void open_file()
{
	/* write read open */
	if ((fd = open(FILENAME, O_RDWR|O_CREAT, 0666)) < 0) {
		perror("OPEN");
		exit(1);
	}
}

void title(int num)
{
	printf("lock ft %3d ", num);
}

void fcntl_test()
{
	struct flock lock;
	lock.l_type = 0;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0x1ffffffffffULL;
	lock.l_pid = getpid();

	/* 001 */
	title(1);
	if (fcntl(fd, F_GETLK, &lock) < 0) {
		perror(" NG fcntl():");
	} else {
		if (lock.l_type != F_UNLCK) {
			printf(" NG fcntl(): ignore l_type (missmatch F_UNLCK)\n");
		} else {
			printf("OK\n");
		}
	}

	/* 002 */
	title(3);
	lock.l_type = F_RDLCK;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror(" NG fcntl():");
	} else {
		printf("OK\n");
	}

	/* 003 */
	title(4);
	lock.l_type = F_WRLCK;
	lock.l_start = 0x1ffffffffffULL;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		printf("OK\n");
	}

	/* 004 */
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0x40000000000ULL;
	lock.l_len = 0x1ffffffffffULL;
	lock.l_pid = getpid();

	switch (fork()) {
	case 0:
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_WRLCK fcntl(): ");
		}
		sleep(2);
		lock.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title(2);
	sleep(1);
	if (fcntl(fd, F_GETLK, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		if (lock.l_type != F_WRLCK) {
			printf(" NG fcntl(): ignore l_type (missmatch F_WDLCK)\n");
		} else {
			printf("OK\n");
		}
	}
	wait(NULL);

	/* 005 */
	title(5);
	lock.l_type = F_UNLCK;
	lock.l_start = 0;
	lock.l_len = 0x1ffffffffffULL;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror("fcntl(): UnLock ");
	} else {
		printf("OK\n");
	}

	/* 006 */
	title(6);
	lock.l_type = F_UNLCK;
	lock.l_start = 0x1ffffffffffULL;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror("fcntl(): UnLock ");
	} else {
		printf("OK\n");
	}

	/* 007 */
	title(7);
	lock.l_type = F_RDLCK;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror(" NG fcntl():");
	} else {
		printf("OK\n");
	}

	/* 008 */
	title(8);
	lock.l_type = F_WRLCK;
	lock.l_start = 0x1ffffffffffULL;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		printf("OK\n");
	}

	/* 009 */
	title(9);
	lock.l_type = F_UNLCK;
	lock.l_start = 0;
	lock.l_len = 0x1ffffffffffULL;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): UnLock ");
	} else {
		printf("OK\n");
	}

	/* 010 */
	title(10);
	lock.l_type = F_UNLCK;
	lock.l_start = 0x1ffffffffffULL;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): UnLock ");
	} else {
		printf("OK\n");
	}

	/* 011 */
	lock.l_type = F_WRLCK;
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
	title(11);
	sleep(1);
	lock.l_type = F_RDLCK;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		printf("OK\n");
		lock.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
	}
	wait(NULL);

	/* 012 */
	lock.l_type = F_WRLCK;
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

	title(12);
	sleep(1);
	lock.l_type = F_WRLCK;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		printf("OK\n");
		lock.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
	}
	wait(NULL);

	/* 013 */
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

	title(13);
	sleep(1);
	lock.l_type = F_WRLCK;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror(" NG fcntl(): ");
	} else {
		printf("OK\n");
		lock.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &lock) < 0) {
			perror(" NG fork F_UNLCK fcntl(): ");
		}
	}
	wait(NULL);
}

void flock_test()
{
	/* 014 */
	title(14);
	if (lockf(fd, F_LOCK, 4096) < 0) {
		perror("NG lockf(): ");
	} else {
		printf("OK\n");
	}

	/* 015 */
	title(16);
	if (lockf(fd, F_ULOCK, 4096) < 0) {
		perror("NG lockf() :");
	} else {
		printf("OK\n");
	}

	/* 016 */
	switch (fork()) {
	case 0:
		if (lockf(fd, F_TLOCK, 4096) < 0) {
			perror("NG fork lockf(F_TLOCK): ");
		}
		sleep(3);
		if (lockf(fd, F_ULOCK, 4096) < 0) {
			perror("NG fork lockf(F_ULOCK): ");
		}
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title(18);
	sleep(1);
	if (lockf(fd, F_TLOCK, 4096) < 0) {
		printf("OK\n");
	} else {
		printf("NG (already lockf)\n");
	}
	wait(NULL);

	/* 017 */
	switch (fork()) {
	case 0:
		if (lockf(fd, F_TLOCK, 4096) < 0) {
			perror(" NG fork lockf(F_TLOCK): ");
		}
		sleep(3);
		if (lockf(fd, F_ULOCK, 4096) < 0) {
			perror(" NG fork lockf(F_ULOCK): ");
		}
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title(20);
	sleep(1);
	if (lockf(fd, F_TEST, 4096) < 0) {
		printf("OK\n");
	} else {
		printf("NG (already lockf)\n");
	}
	wait(NULL);

	/* 018 */
	title(19);
	if (lockf(fd, F_TEST, 4096) < 0) {
		perror("NG lockf() :");
	} else {
		printf("OK\n");
	}

	/* 019 */
	title(15);
	if (lockf(fd, F_TLOCK, 4096) < 0) {
		perror("NG lockf() :");
	} else {
		printf("OK\n");
	}
	if (lockf(fd, F_ULOCK, 4096) < 0) {
		perror("lockf() :");
		exit(1);
	}

	/* 020 */
	switch (fork()) {
	case 0:
		if (lockf(fd, F_TLOCK, 4096) < 0) {
			perror("NG fork lockf(F_TLOCK): ");
		}
		sleep(3);
		if (lockf(fd, F_ULOCK, 4096) < 0) {
			perror("NG fork lockf(F_ULOCK): ");
		}
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title(17);
	sleep(1);
	if (lockf(fd, F_LOCK, 4096) < 0) {
		perror("NG lockf(): ");
	} else {
		printf("OK\n");
		if (lockf(fd, F_ULOCK, 4096) < 0) {
			perror("NG lockf(F_ULOCK): ");
			exit(1);
		}
	}
	wait(NULL);
}

void test_start()
{
	fcntl_test();
	flock_test();
}

/* main */
int main(int argc, char **argv)
{

	open_file();
	test_start();
	close(fd);
	return 0;
}

