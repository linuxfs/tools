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
#include <sys/wait.h>

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
char buf[4096];
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
	if (fcntl(fd, F_SETLK, &lock) < 0) {
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
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		perror("fcntl(): Lock ");
		return -1;
	}
	list_add(seek_pt, len);
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
	printf("\nunlock test %d\n", num++);
}

void test001_008()
{
	/* 001 */
	title();
	lock_w(0, 12288);
	unlock(4096, 8192);
	catlog();

	/* 002 */
	title();
	lock_w(0, 12288);
	unlock(0, 8192);
	catlog();

	/* 003 */
	title();
	lock_r(0, 12288);
	unlock(4096, 4096);
	catlog();

	/* 004 */
	title();
	lock_w(0, 12288);
	unlock(4096, 4096);
	catlog();

	/* 005 */
	title();
	lock_w(0, 8192);
	unlock(0, 12288);
	catlog();

	/* 006 */
	title();
	lock_w(4096, 8192);
	unlock(0, 12288);
	catlog();

	/* 007 */
	title();
	lock_r(4096, 4096);
	unlock(0, 12288);
	catlog();

	/* 008 */
	title();
	lock_w(4096, 4096);
	unlock(0, 12288);
	catlog();
}

void test009_016()
{
	/* 009 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	unlock(4096, 4096);
	catlog();

	/* 010 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	unlock(4096, 4096);
	catlog();

	/* 011 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	unlock(4096, 4096);
	catlog();

	/* 012 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	unlock(4096, 4096);
	catlog();

	/* 013 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	unlock(0, 12288);
	catlog();

	/* 014 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	unlock(0, 12288);
	catlog();

	/* 015 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	unlock(0, 12288);
	catlog();

	/* 016 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	unlock(0, 12288);
	catlog();
}

void test017_024()
{
	/* 017 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	unlock(4096, 12288);
	catlog();

	/* 018 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	unlock(4096, 12288);
	catlog();

	/* 019 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	unlock(4096, 12288);
	catlog();

	/* 020 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	unlock(4096, 12288);
	catlog();

	/* 021 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 12288);
	catlog();

	/* 022 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 12288);
	catlog();

	/* 023 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 12288);
	catlog();

	/* 024 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 12288);
	catlog();
}

void test025_032()
{
	/* 025 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 12288);
	catlog();

	/* 026 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 12288);
	catlog();

	/* 027 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 12288);
	catlog();

	/* 028 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 12288);
	catlog();

	/* 029 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 16384);
	catlog();

	/* 030 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 16384);
	catlog();

	/* 031 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 16384);
	catlog();

	/* 032 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 16384);
	catlog();
}

void test033_040()
{
	/* 033 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 16384);
	catlog();

	/* 034 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 16384);
	catlog();

	/* 035 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 16384);
	catlog();

	/* 036 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 16384);
	catlog();

	/* 037 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 20480);
	catlog();

	/* 038 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 20480);
	catlog();

	/* 039 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 20480);
	catlog();

	/* 040 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 20480);
	catlog();
}

void test041_048()
{
	/* 041 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 20480);
	catlog();

	/* 042 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 20480);
	catlog();

	/* 043 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(4096, 20480);
	catlog();

	/* 044 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(4096, 20480);
	catlog();

	/* 045 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 16384);
	catlog();

	/* 046 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 16384);
	catlog();

	/* 047 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 16384);
	catlog();

	/* 048 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 16384);
	catlog();
}

void test049_056()
{
	/* 049 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 16384);
	catlog();

	/* 050 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 16384);
	catlog();

	/* 051 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(8192, 16384);
	catlog();

	/* 052 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(8192, 16384);
	catlog();

	/* 053 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(0, 28672);
	catlog();

	/* 054 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(0, 28672);
	catlog();

	/* 055 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(0, 28672);
	catlog();

	/* 056 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_r(20480, 8192);
	unlock(0, 28672);
	catlog();
}

void test057_064()
{
	/* 057 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(0, 28672);
	catlog();

	/* 058 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 4096);
	lock_w(20480, 8192);
	unlock(0, 28672);
	catlog();

	/* 059 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_r(20480, 8192);
	unlock(0, 28672);
	catlog();

	/* 060 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 4096);
	lock_w(20480, 8192);
	unlock(0, 28672);
	catlog();
}

void test_start()
{
	test001_008();
	test009_016();
	test017_024();
	test025_032();
	test033_040();
	test041_048();
	test049_056();
	test057_064();
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

