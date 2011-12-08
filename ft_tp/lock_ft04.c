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

void list_add(uint64_t seek_pt, uint64_t len)
{
	struct unlock_t *new;

	if ((new = (struct unlock_t *)calloc(1, sizeof(struct unlock_t))) == NULL) {
		perror("calloc");
		exit(1);
	}

	if (head) new->next = head;
	else new->next = NULL;
	head = new;

	new->seek_pt = seek_pt;
	new->len = len;
}

void list_del(void)
{
	struct unlock_t *pos;
	while (head) {
		pos = head->next;
		unlock(head->seek_pt, head->len);
		free(head);
		head = pos;
	}
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

/* write Lock */
int lock_w(uint64_t seek_pt, uint64_t len)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): Lock ");
		return -1;
	}
	list_add(seek_pt, len);
	return 0;
}

/* read Lock */
int lock_r(uint64_t seek_pt, uint64_t len)
{
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): Lock ");
		return -1;
	}
	list_add(seek_pt, len);
	return 0;
}

/* get write Lock */
int lock_gw(uint64_t seek_pt, uint64_t len)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_GETLK, &lock) < 0) {
		perror("fcntl(): Lock ");
		return -1;
	}
	if (lock.l_type != F_RDLCK) {
		printf(" NG fcntl(): ignore l_type (missmatch F_RDLCK)\n");
	} else {
		printf("OK\n");
	}
	return 0;
}

/* get read Lock */
int lock_gr(uint64_t seek_pt, uint64_t len)
{
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_GETLK, &lock) < 0) {
		perror("fcntl(): Lock ");
		return -1;
	}
	if (lock.l_type != F_WRLCK) {
		printf(" NG fcntl(): ignore l_type (missmatch F_WRLCK)\n");
	} else {
		printf("OK\n");
	}
	return 0;
}

void usage(char *pg)
{
	printf("usage: %s pfs_lkmgr-pid\n", pg);
	exit(1);
}

void catlog()
{
	int ld;
	memset(buf, 0, 4095);

	kill(lockd_pid, SIGUSR1);

	sleep(1);
	if ((ld = open(LOCK_LOG, O_RDONLY, 0)) < 0) {
		perror("Can't open logfile");
		exit(1);
	}

	while (read(ld, buf, 4096) > 0) {
		printf("%s", buf);
	}
	close (ld);
	list_del();
	printf("\n");
	fflush(stdout);
}

void title()
{
	static int num = 1;
	printf("\nlock test %d\n", num++);
}

void test_rlock()
{
	/* 001 */
	title();
	lock_r(0, 1);
	catlog();

	/* 002 */
	title();
	lock_r(0x80000000, 1);
	catlog();

	/* 003 */
	title();
	lock_r(0x100000000ULL, 1);
	catlog();

	/* 004 */
	title();
	lock_r(0, 0x80000000);
	catlog();

	/* 005 */
	title();
	lock_r(0, 0x100000000ULL);
	catlog();

	/* 006 */
	title();
	lock_r(0, 0x7fffffffffffffffULL);
	catlog();

	/* 007 */
	title();
	lock_r(0x7fffffff, 2);
	catlog();

	/* 008 */
	title();
	lock_r(0xffffffff, 2);
	catlog();

	/* 009 */
	title();
	lock_r(0xffffffff, 0x80000000);
	catlog();

	/* 010 */
	title();
	lock_r(0xffffffff, 0x100000000ULL);
	catlog();

	/* 011 */
	title();
	lock_r(0x0, 0x7fffffffffffffffULL);
	catlog();
}

void test_wlock()
{
	/* 012 */
	title();
	lock_w(0, 1);
	catlog();

	/* 013 */
	title();
	lock_w(0x80000000, 1);
	catlog();

	/* 014 */
	title();
	lock_w(0x100000000ULL, 1);
	catlog();

	/* 015 */
	title();
	lock_w(0, 0x80000000);
	catlog();

	/* 016 */
	title();
	lock_w(0, 0x100000000ULL);
	catlog();

	/* 017 */
	title();
	lock_w(0, 0x7fffffffffffffffULL);
	catlog();

	/* 018 */
	title();
	lock_w(0x7fffffff, 2);
	catlog();

	/* 019 */
	title();
	lock_w(0xffffffff, 2);
	catlog();

	/* 020 */
	title();
	lock_w(0xffffffff, 0x80000000);
	catlog();

	/* 021 */
	title();
	lock_w(0xffffffff, 0x100000000ULL);
	catlog();

	/* 022 */
	title();
	lock_w(0x0, 0x7fffffffffffffffULL);
	catlog();
}

void test_grlock()
{
	/* 023 */
	switch (fork()) {
	case 0:
		lock_r(0, 1);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0, 1);
	wait(NULL);

	/* 024 */
	switch (fork()) {
	case 0:
		lock_r(0x80000000, 1);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0x80000000, 1);
	wait(NULL);

	/* 025 */
	switch (fork()) {
	case 0:
		lock_r(0x100000000ULL, 1);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0x100000000ULL, 1);
	wait(NULL);

	/* 026 */
	switch (fork()) {
	case 0:
		lock_r(0, 0x80000000);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0, 0x80000000);
	wait(NULL);

	/* 027 */
	switch (fork()) {
	case 0:
		lock_r(0, 0x100000000ULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0, 0x100000000ULL);
	wait(NULL);

	/* 028 */
	switch (fork()) {
	case 0:
		lock_r(0, 0x7fffffffffffffffULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0, 0x7fffffffffffffffULL);
	wait(NULL);

	/* 029 */
	switch (fork()) {
	case 0:
		lock_r(0x7fffffff, 2);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0x7fffffff, 2);
	wait(NULL);

	/* 030 */
	switch (fork()) {
	case 0:
		lock_r(0xffffffff, 2);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0xffffffff, 2);
	wait(NULL);

	/* 031 */
	switch (fork()) {
	case 0:
		lock_r(0xffffffff, 0x80000000);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0xffffffff, 0x80000000);
	wait(NULL);

	/* 032 */
	switch (fork()) {
	case 0:
		lock_r(0xffffffff, 0x100000000ULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0xffffffff, 0x100000000ULL);
	wait(NULL);

	/* 033 */
	switch (fork()) {
	case 0:
		lock_r(0x0, 0x7fffffffffffffffULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gw(0x0, 0x7fffffffffffffffULL);
	wait(NULL);

}

void test_gwlock()
{
	/* 034 */
	switch (fork()) {
	case 0:
		lock_w(0, 1);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0, 1);
	wait(NULL);

	/* 035 */
	switch (fork()) {
	case 0:
		lock_w(0x80000000, 1);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0x80000000, 1);
	wait(NULL);

	/* 036 */
	switch (fork()) {
	case 0:
		lock_w(0x100000000ULL, 1);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0x100000000ULL, 1);
	wait(NULL);

	/* 037 */
	switch (fork()) {
	case 0:
		lock_w(0, 0x80000000);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0, 0x80000000);
	wait(NULL);

	/* 038 */
	switch (fork()) {
	case 0:
		lock_w(0, 0x100000000ULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0, 0x100000000ULL);
	wait(NULL);

	/* 039 */
	switch (fork()) {
	case 0:
		lock_w(0, 0x7fffffffffffffffULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0, 0x7fffffffffffffffULL);
	wait(NULL);

	/* 040 */
	switch (fork()) {
	case 0:
		lock_w(0x7fffffff, 2);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0x7fffffff, 2);
	wait(NULL);

	/* 041 */
	switch (fork()) {
	case 0:
		lock_w(0xffffffff, 2);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0xffffffff, 2);
	wait(NULL);

	/* 042 */
	switch (fork()) {
	case 0:
		lock_w(0xffffffff, 0x80000000);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0xffffffff, 0x80000000);
	wait(NULL);

	/* 043 */
	switch (fork()) {
	case 0:
		lock_w(0xffffffff, 0x100000000ULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0xffffffff, 0x100000000ULL);
	wait(NULL);

	/* 044 */
	switch (fork()) {
	case 0:
		lock_w(0x0, 0x7fffffffffffffffULL);
		sleep(2);
		list_del();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	title();
	sleep(1);
	lock_gr(0x0, 0x7fffffffffffffffULL);
	wait(NULL);

}

void test_start()
{
//	test_rlock();
//	test_wlock();
	test_grlock();
	test_gwlock();
}

/* main */
int main(int argc, char **argv)
{

	if (argc != 2)
		usage(argv[0]);

	lockd_pid = atoi(argv[1]);
	printf("pfs_lkmgr-pid = %d\n", lockd_pid);

	open_file();
	test_start();
	close(fd);
	return 0;
}

