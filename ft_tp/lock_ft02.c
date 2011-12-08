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
	printf("\nfork lock test %d\n", num++);
}

void test001_008()
{
	/* 001 */
	title();
	lock_r(0, 4096);
	switch (fork()) {
	case 0:
		lock_r(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 002 */
	title();
	lock_r(0, 4096);
	switch (fork()) {
	case 0:
		lock_w(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 003 */
	title();
	lock_w(0, 4096);
	switch (fork()) {
	case 0:
		lock_r(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 004 */
	title();
	lock_w(0, 4096);
	switch (fork()) {
	case 0:
		lock_w(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 005 */
	title();
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 006 */
	title();
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 007 */
	title();
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 008 */
	title();
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test009_016()
{
	/* 009 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 010 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 011 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 012 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(8192, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 013 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 014 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 015 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 016 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test017_024()
{
	/* 017 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 018 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 019 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 020 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 021 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 022 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 023 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 024 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test025_032()
{
	/* 025 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 026 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 027 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 028 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 029 */
	title();
	lock_r(0, 12288);
	switch (fork()) {
	case 0:
		lock_r(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 030 */
	title();
	lock_r(0, 12288);
	switch (fork()) {
	case 0:
		lock_w(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 031 */
	title();
	lock_w(0, 12288);
	switch (fork()) {
	case 0:
		lock_r(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 032 */
	title();
	lock_w(0, 12288);
	switch (fork()) {
	case 0:
		lock_w(0, 8192);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test033_040()
{
	/* 033 */
	title();
	lock_r(0, 12288);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 034 */
	title();
	lock_r(0, 12288);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 035 */
	title();
	lock_w(0, 12288);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 036 */
	title();
	lock_w(0, 12288);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 037 */
	title();
	lock_r(0, 12288);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 038 */
	title();
	lock_r(0, 12288);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 039 */
	title();
	lock_w(0, 12288);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 040 */
	title();
	lock_w(0, 12288);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test041_048()
{
	/* 041 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 042 */
	title();
	lock_r(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 043 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 044 */
	title();
	lock_w(0, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 045 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 046 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 047 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 048 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test049_056()
{
	/* 049 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 050 */
	title();
	lock_r(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 051 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 052 */
	title();
	lock_w(4096, 8192);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 053 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 054 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 055 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 056 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test057_064()
{
	/* 057 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 058 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 059 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 060 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(4096, 4096);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 061 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 062 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 063 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 064 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test065_072()
{
	/* 065 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 066 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 067 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_r(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 068 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	switch (fork()) {
	case 0:
		lock_w(0, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 069 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 070 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 071 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 072 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
}

void test073_080()
{
	/* 073 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 074 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 075 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	switch (fork()) {
	case 0:
		lock_r(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();

	/* 076 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	switch (fork()) {
	case 0:
		lock_w(4096, 12288);
		catlog();
		exit(0);
	case - 1:
		perror("fork");
		exit(1);
	}
	wait(NULL);
	list_del();
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
	test065_072();
	test073_080();
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

